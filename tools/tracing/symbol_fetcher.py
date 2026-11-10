# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Extracts breakpad symbol file from Google Cloud Platform."""

import logging
import os
import zipfile

import flag_utils
from metadata_extractor import OSName
import py_utils.cloud_storage as cloud_storage
import rename_breakpad


def GetTraceBreakpadSymbols(
    cloud_storage_bucket,
    metadata,
    breakpad_output_dir,
):
  """Fetch trace symbols from GCS and convert to breakpad format, if needed.

  Args:
    cloud_storage_bucket: bucket in cloud storage where symbols reside.
    metadata: MetadataExtractor class that contains necessary trace file
      metadata for fetching its symbol file.
    breakpad_output_dir: local path to store trace symbol breakpad file.

  Raises:
    Exception: if failed to extract trace OS name or version number, or
      they are not supported or recognized.
  """
  metadata.Initialize()
  if metadata.os_name is None:
    raise Exception('Failed to extract trace OS name: ' + metadata.trace_file)
  if metadata.version_number is None:
    raise Exception('Failed to extract trace version number: ' +
                    metadata.trace_file)
  if metadata.os_name == OSName.WINDOWS:
    raise Exception(
        'Windows platform is not currently supported for symbolization.')
  elif metadata.os_name == OSName.LINUX or metadata.os_name == OSName.MAC:
    _FetchBreakpadSymbols(cloud_storage_bucket, metadata, breakpad_output_dir)
    rename_breakpad.RenameBreakpadFiles(breakpad_output_dir,
                                        breakpad_output_dir)
  else:
    raise Exception('Trace OS "%s" is not supported: %s' %
                    (metadata.os_name, metadata.trace_file))

  flag_utils.GetTracingLogger().info('Breakpad symbols located at: %s',
                                     os.path.abspath(breakpad_output_dir))


def _FetchBreakpadSymbols(cloud_storage_bucket, metadata, breakpad_output_dir):
  """Fetches and extracts Mac or Linux breakpad format symbolization file.

  Args:
    cloud_storage_bucket: bucket in cloud storage where symbols reside.
    metadata: MetadataExtractor class that contains necessary trace file
      metadata for fetching its symbol file.
    breakpad_output_dir: local path to store trace symbol breakpad file.

  Raises:
    Exception: if trace OS is not mac or linux, or failed to extract
      version number.
    ValueError: if linux trace is of 32 bit bitness.
  """
  # Determine GCS folder.
  folder = None
  if metadata.os_name == OSName.LINUX:
    if metadata.bitness == '32':
      raise ValueError('32 bit Linux traces are not supported.')
    folder = 'linux64'
  elif metadata.os_name == OSName.MAC:
    if (metadata.architecture
        is not None) and 'arm' in metadata.architecture.lower():
      folder = 'mac-arm64'
    else:
      if metadata.architecture is None:
        flag_utils.GetTracingLogger().warning(
            'Architecture not found, so using x86-64.')
      folder = 'mac64'
  else:
    raise Exception('Expected OS "%s" to be Linux or Mac: %s' %
                    (metadata.os_name, metadata.trace_file))

  # Build Google Cloud Storage path to the symbols.
  assert folder is not None
  gcs_folder = 'desktop-*/' + metadata.version_number + '/' + folder
  gcs_file = gcs_folder + '/breakpad-info'
  gcs_zip_file = gcs_file + '.zip'

  # Local path to downloaded symbols.
  breakpad_zip = os.path.join(breakpad_output_dir, 'breakpad-info.zip')

  # Fetch and unzip symbol files from GCS. Some version, like mac,
  # don't have the .zip extension on GCS. Assumes that 'breakpad-info'
  # file (without .zip extension) from GCS is a zip file.
  if not _FetchAndUnzipGCSFile(cloud_storage_bucket, gcs_zip_file, breakpad_zip,
                               breakpad_output_dir):
    if not _FetchAndUnzipGCSFile(cloud_storage_bucket, gcs_file, breakpad_zip,
                                 breakpad_output_dir):
      raise Exception('Failed to find symbols on GCS: %s[.zip].' % (gcs_file))


def _FetchAndUnzipGCSFile(cloud_storage_bucket, gcs_file, gcs_output,
                          output_dir):
  """Fetch file from GCS to local |gcs_output|, then unzip it into |output_dir|.

  Returns:
    True if successfully fetches and unzips file; false, otherwise.

  Raises:
    zipfile.BadZipfile: if file is not a zip file
  """
  if _FetchGCSFile(cloud_storage_bucket, gcs_file, gcs_output):
    _UnzipFile(gcs_output, output_dir)
    return True
  return False


def _FetchGCSFile(cloud_storage_bucket, gcs_file, output_file):
  """Fetch and save file from GCS to |output_file|.

  Args:
    cloud_storage_bucket: bucket in cloud storage where symbols reside.
    gcs_file: path to file in GCS.
    output_file: local file to store fetched GCS file.

  Returns:
    True if successfully fetches file; False, otherwise.
  """
  if cloud_storage.Exists(cloud_storage_bucket, gcs_file):
    flag_utils.GetTracingLogger().info('Downloading files from GCS: %s',
                                       gcs_file)
    cloud_storage.Get(cloud_storage_bucket, gcs_file, output_file)
    flag_utils.GetTracingLogger().info('Saved file locally to: %s', output_file)
    return True
  return False


def _UnzipFile(zip_file, output_dir):
  """Unzips file into provided output directory.

  Raises:
    zipfile.BadZipfile: if file is not a zip file
  """
  with zipfile.ZipFile(zip_file, 'r') as zip_f:
    zip_f.extractall(output_dir)
