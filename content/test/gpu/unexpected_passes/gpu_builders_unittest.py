#!/usr/bin/env vpython3
# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import unittest

from unexpected_passes_common import constants
from unexpected_passes_common import data_types

from unexpected_passes import gpu_builders


class BuilderRunsTestOfInterestUnittest(unittest.TestCase):
  def setUp(self) -> None:
    self.instance = gpu_builders.GpuBuilders('webgl_conformance', False)

  def testMatch(self) -> None:
    """Tests that a match can be successfully found."""
    test_map = {
      'isolated_scripts': [
        {
          'args': [
            'webgl_conformance',
          ],
          'test': 'telemetry_gpu_integration_test',
        },
      ],
    }
    self.assertTrue(self.instance._BuilderRunsTestOfInterest(test_map))

  def testNoMatchIsolate(self) -> None:
    """Tests that a match is not found if the isolate name is not valid."""
    test_map = {
      'isolated_scripts': [
        {
          'args': [
            'webgl_conformance',
          ],
          'test': 'not_telemetry',
        },
      ],
    }
    self.assertFalse(self.instance._BuilderRunsTestOfInterest(test_map))

  def testNoMatchSuite(self) -> None:
    """Tests that a match is not found if the suite name is not valid."""
    test_map = {
      'isolated_scripts': [
        {
          'args': [
            'not_a_suite',
          ],
          'test': 'telemetry_gpu_integration_test',
        },
      ],
    }
    self.assertFalse(self.instance._BuilderRunsTestOfInterest(test_map))

  def testRetainedDesktopIsolateNames(self) -> None:
    """Tests that only the retained desktop GPU isolate is queried."""
    self.assertEqual(
      {'telemetry_gpu_integration_test'}, self.instance.GetIsolateNames()
    )


class GetNonChromiumBuildersUnittest(unittest.TestCase):
  def testStringsConvertedToBuilderEntries(self) -> None:
    """Tests that the easier-to-read strings get converted to BuilderEntry."""
    instance = gpu_builders.GpuBuilders('webgl_conformance', False)
    builder = data_types.BuilderEntry(
      'Win V8 FYI Release (NVIDIA)', constants.BuilderTypes.CI, False
    )
    self.assertIn(builder, instance.GetNonChromiumBuilders())
    expected_builders = {
      data_types.BuilderEntry(name, constants.BuilderTypes.CI, False)
      for name in {
        'Win V8 FYI Release (NVIDIA)',
        'Mac V8 FYI Release (Apple M2)',
        'Mac V8 FYI Release (Intel)',
        'Linux V8 FYI Release - pointer compression (NVIDIA)',
        'Linux V8 FYI Release (NVIDIA)',
      }
    }
    self.assertEqual(expected_builders, instance.GetNonChromiumBuilders())


if __name__ == '__main__':
  unittest.main(verbosity=2)
