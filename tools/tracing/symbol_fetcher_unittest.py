#!/usr/bin/env vpython3
# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import shutil
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), os.pardir, 'perf'))

from core import path_util

path_util.AddPyUtilsToPath()
path_util.AddTracingToPath()

import py_utils.cloud_storage as cloud_storage
import metadata_extractor
from metadata_extractor import OSName
import symbol_fetcher
import breakpad_file_extractor
import rename_breakpad
import mock


class SymbolFetcherTestBase(unittest.TestCase):
  """Super class holding shared parameters and methods.

  All function stashing and unstashing happens here to ensure that tests
  do not mutate each other's functions. General mocks can be written here
  and overridden in respective subclasses or tests as needed.
  """

  def setUp(self):
    self.trace_processor_path = 'trace_processor_shell'
    self.trace_file = 'trace_file.proto'
    self.cloud_storage_bucket = 'chrome-unsigned'
    self.breakpad_output_dir = tempfile.mkdtemp()
    self.breakpad_zip_file = os.path.join(self.breakpad_output_dir,
                                          'breakpad-info.zip')
    self.dump_syms_dir = tempfile.mkdtemp()
    self.dump_syms_path = os.path.join(self.dump_syms_dir, 'dump_syms')
    with open(self.dump_syms_path, 'w') as _:
      pass

    # Stash mocked functions.
    self.Exists_stash = cloud_storage.Exists
    self.Get_stash = cloud_storage.Get
    self.UnzipFile_stash = symbol_fetcher._UnzipFile
    self.FetchGCSFile_stash = symbol_fetcher._FetchGCSFile
    self.FetchAndUnzipGCSFile_stash = symbol_fetcher._FetchAndUnzipGCSFile
    self.RunDumpSyms_stash = breakpad_file_extractor._RunDumpSyms
    self.RenameBreakpadFiles_stash = rename_breakpad.RenameBreakpadFiles

    # Ignore this function. It is tested separately.
    rename_breakpad.RenameBreakpadFiles = mock.MagicMock()

  def tearDown(self):
    shutil.rmtree(self.breakpad_output_dir)
    shutil.rmtree(self.dump_syms_dir)

    # Unstash functions.
    cloud_storage.Exists = self.Exists_stash
    cloud_storage.Get = self.Get_stash
    symbol_fetcher._UnzipFile = self.UnzipFile_stash
    symbol_fetcher._FetchGCSFile = self.FetchGCSFile_stash
    symbol_fetcher._FetchAndUnzipGCSFile = self.FetchAndUnzipGCSFile_stash
    breakpad_file_extractor._RunDumpSyms = self.RunDumpSyms_stash
    rename_breakpad.RenameBreakpadFiles = self.RenameBreakpadFiles_stash

  def _createMetadataExtractor(
      self,
      version_number=None,
      os_name=None,
      architecture=None,
      bitness=None,
      modules=None,
  ):
    metadata = metadata_extractor.MetadataExtractor(self.trace_processor_path,
                                                    self.trace_file)
    metadata.InitializeForTesting(version_number, os_name, architecture,
                                  bitness, modules)
    return metadata

  def _ensureRenameCalled(self):
    rename_breakpad.RenameBreakpadFiles.assert_called_once_with(
        self.breakpad_output_dir, self.breakpad_output_dir)

  def testNoOSName(self):
    metadata = self._createMetadataExtractor(version_number='123',
                                             os_name=None,
                                             architecture='x86_64',
                                             bitness='64')

    exception_msg = 'Failed to extract trace OS name: ' + self.trace_file
    with self.assertRaises(Exception) as e:
      symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket,
                                             metadata, self.breakpad_output_dir)
    self.assertIn(exception_msg, str(e.exception))

  def testOSNameNotRecognized(self):
    metadata = self._createMetadataExtractor(version_number='123',
                                             os_name='blah',
                                             architecture='x86_64',
                                             bitness='64')

    exception_msg = 'Trace OS "blah" is not supported: ' + self.trace_file
    with self.assertRaises(Exception) as e:
      symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket,
                                             metadata, self.breakpad_output_dir)
    self.assertIn(exception_msg, str(e.exception))

  def testNoVersionNumber(self):
    metadata = self._createMetadataExtractor(
        version_number=None,
        os_name=OSName.LINUX,
        architecture='x86_64',
        bitness='64',
    )

    exception_msg = 'Failed to extract trace version number: ' + self.trace_file
    with self.assertRaises(Exception) as e:
      symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket,
                                             metadata, self.breakpad_output_dir)
    self.assertIn(exception_msg, str(e.exception))


class MacAndLinuxTestCase(SymbolFetcherTestBase):

  def setUp(self):
    super().setUp()

    cloud_storage.Get = mock.Mock()
    # Default: Cloud storage has |gcs_file|.zip file. We use |side_effect|
    # because tests that overwrite this will need to use |side_effect|.
    cloud_storage.Exists = mock.Mock(side_effect=[True])
    # Default: Ignore |_UnzipFile| function so we don't unzip fake files.
    symbol_fetcher._UnzipFile = mock.MagicMock()

  def _ensureUnzipAndRenameCalls(self):
    zip_file = os.path.join(self.breakpad_output_dir, 'breakpad-info.zip')
    symbol_fetcher._UnzipFile.assert_called_once_with(zip_file,
                                                      self.breakpad_output_dir)
    self._ensureRenameCalled()

  def testLinux(self):
    metadata = self._createMetadataExtractor(
        version_number='123',
        os_name=OSName.LINUX,
        architecture='x86_64',
        bitness='64',
    )

    symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket, metadata,
                                           self.breakpad_output_dir)

    cloud_storage.Get.assert_called_once_with(
        self.cloud_storage_bucket,
        'desktop-*/123/linux64/breakpad-info.zip',
        self.breakpad_zip_file,
    )
    cloud_storage.Exists.assert_called_once()

    self._ensureUnzipAndRenameCalls()

  def testMacArm(self):
    metadata = self._createMetadataExtractor(
        version_number='123',
        os_name=OSName.MAC,
        architecture='armv7l',
        bitness='64',
    )

    symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket, metadata,
                                           self.breakpad_output_dir)

    cloud_storage.Get.assert_called_once_with(
        self.cloud_storage_bucket,
        'desktop-*/123/mac-arm64/breakpad-info.zip',
        self.breakpad_zip_file,
    )
    cloud_storage.Exists.assert_called_once()

    self._ensureUnzipAndRenameCalls()

  def testMac86(self):
    metadata = self._createMetadataExtractor(
        version_number='123',
        os_name=OSName.MAC,
        architecture='x86_64',
        bitness='64',
    )

    symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket, metadata,
                                           self.breakpad_output_dir)

    cloud_storage.Get.assert_called_once_with(
        self.cloud_storage_bucket,
        'desktop-*/123/mac64/breakpad-info.zip',
        self.breakpad_zip_file,
    )
    cloud_storage.Exists.assert_called_once()

    self._ensureUnzipAndRenameCalls()

  def testMacNoArchitecture(self):
    metadata = self._createMetadataExtractor(version_number='123',
                                             os_name=OSName.MAC,
                                             architecture=None,
                                             bitness='64')

    symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket, metadata,
                                           self.breakpad_output_dir)

    cloud_storage.Get.assert_called_once_with(
        self.cloud_storage_bucket,
        'desktop-*/123/mac64/breakpad-info.zip',
        self.breakpad_zip_file,
    )
    cloud_storage.Exists.assert_called_once()

    self._ensureUnzipAndRenameCalls()

  def testMacNotZip(self):
    metadata = self._createMetadataExtractor(
        version_number='123',
        os_name=OSName.MAC,
        architecture='x86_64',
        bitness='64',
    )
    # Cloud storage has |gcs_file| without a .zip extension.
    cloud_storage.Exists.side_effect = [False, True]

    symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket, metadata,
                                           self.breakpad_output_dir)

    cloud_storage.Get.assert_called_once_with(
        self.cloud_storage_bucket,
        'desktop-*/123/mac64/breakpad-info',
        self.breakpad_zip_file,
    )
    # |cloud_storage.Exists| called with |gcs_file|.zip, and with |gcs_file|
    # without the .zip extension.
    self.assertEqual(cloud_storage.Exists.call_count, 2)

    self._ensureUnzipAndRenameCalls()

  def testLinuxFetchFailure(self):
    metadata = self._createMetadataExtractor(
        version_number='123',
        os_name=OSName.LINUX,
        architecture='x86_64',
        bitness='64',
    )
    # Cloud storage does not have |gcs_file| or |gcs_file|.zip extension.
    cloud_storage.Exists.side_effect = [False, False]

    with self.assertRaises(Exception):
      symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket,
                                             metadata, self.breakpad_output_dir)
    self.assertEqual(cloud_storage.Exists.call_count, 2)
    cloud_storage.Get.assert_not_called()

  def testLinux32BitFailure(self):
    metadata = self._createMetadataExtractor(
        version_number='123',
        os_name=OSName.LINUX,
        architecture='x86_64',
        bitness='32',
    )

    with self.assertRaises(ValueError):
      symbol_fetcher.GetTraceBreakpadSymbols(self.cloud_storage_bucket,
                                             metadata, self.breakpad_output_dir)

    cloud_storage.Exists.assert_not_called()
    cloud_storage.Get.assert_not_called()


if __name__ == '__main__':
  unittest.main()
