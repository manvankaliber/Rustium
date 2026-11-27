#!/usr/bin/env python3
# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

# //testing/buildbot imports.
import buildbot_json_magic_substitutions as magic_substitutions


def CreateConfigWithGpu(gpu):
  return {
    'swarming': {
      'dimensions': {
        'gpu': gpu,
      },
    },
  }


class GPUExpectedVendorId(unittest.TestCase):
  def testSingleGpuSingleDimension(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    self.assertEqual(
      magic_substitutions.GPUExpectedVendorId(test_config, None, {}),
      ['--expected-vendor-id', 'vendor'],
    )

  def testDoubleGpuSingleDimension(self):
    test_config = CreateConfigWithGpu(
      'vendor:device1-driver|vendor:device2-driver'
    )
    self.assertEqual(
      magic_substitutions.GPUExpectedVendorId(test_config, None, {}),
      ['--expected-vendor-id', 'vendor'],
    )

  def testDoubleGpuSingleDimensionDifferentVendors(self):
    test_config = CreateConfigWithGpu(
      'vendor:device1-driver|vendor2:device2-driver'
    )
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUExpectedVendorId(test_config, None, {})

  def testAppleSilicon(self):
    test_config = CreateConfigWithGpu('apple:m1')
    self.assertEqual(
      magic_substitutions.GPUExpectedVendorId(test_config, None, {}),
      ['--expected-vendor-id', '106b'],
    )

  def testNoGpu(self):
    test_config = {
      'swarming': {
        'dimensions': {},
      },
    }
    self.assertEqual(
      magic_substitutions.GPUExpectedVendorId(test_config, None, {}),
      ['--expected-vendor-id', '0'],
    )

  def testNoDimensions(self):
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUExpectedVendorId({}, None, {})


class GPUExpectedDeviceId(unittest.TestCase):
  def assertDeviceIdCorrectness(self, retval, device_ids):
    self.assertEqual(len(retval), 2 * len(device_ids))
    for i in range(0, len(retval), 2):
      self.assertEqual(retval[i], '--expected-device-id')
    for d in device_ids:
      self.assertIn(d, retval)

  def testSingleGpuSingleDimension(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    self.assertDeviceIdCorrectness(
      magic_substitutions.GPUExpectedDeviceId(test_config, None, {}),
      ['device1'],
    )

  def testDoubleGpuSingleDimension(self):
    test_config = CreateConfigWithGpu(
      'vendor:device1-driver|vendor:device2-driver'
    )
    self.assertDeviceIdCorrectness(
      magic_substitutions.GPUExpectedDeviceId(test_config, None, {}),
      ['device1', 'device2'],
    )

  def testAppleSilicon(self):
    test_config = CreateConfigWithGpu('apple:m1')
    self.assertDeviceIdCorrectness(
      magic_substitutions.GPUExpectedDeviceId(test_config, None, {}), ['0']
    )

  def testNoGpu(self):
    self.assertDeviceIdCorrectness(
      magic_substitutions.GPUExpectedDeviceId(
        {'swarming': {'dimensions': {}}}, None, {}
      ),
      ['0'],
    )

  def testNoDimensions(self):
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUExpectedDeviceId({}, None, {})


class GPUNoXvfbForRealHardware(unittest.TestCase):
  def testNoOsType(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUNoXvfbForRealHardware(test_config, 'name', {})

  def testNonLinuxOsType(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    for os_type in ['mac', 'win']:
      with self.assertRaises(AssertionError):
        magic_substitutions.GPUNoXvfbForRealHardware(
          test_config, 'name', {'os_type': os_type}
        )

  def testGceExplicitlyRequested(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    test_config['swarming']['dimensions']['gce'] = '1'
    tester_config = {'os_type': 'linux'}
    retval = magic_substitutions.GPUNoXvfbForRealHardware(
      test_config, 'name', tester_config
    )
    self.assertEqual(retval, [])

  def testGceNoGpu(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    del test_config['swarming']['dimensions']['gpu']
    tester_config = {'os_type': 'linux'}
    retval = magic_substitutions.GPUNoXvfbForRealHardware(
      test_config, 'name', tester_config
    )
    self.assertEqual(retval, [])

  def testGceNoneGpu(self):
    test_config = CreateConfigWithGpu('none')
    tester_config = {'os_type': 'linux'}
    retval = magic_substitutions.GPUNoXvfbForRealHardware(
      test_config, 'name', tester_config
    )
    self.assertEqual(retval, [])

  def testNonGce(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    tester_config = {'os_type': 'linux'}
    retval = magic_substitutions.GPUNoXvfbForRealHardware(
      test_config, 'name', tester_config
    )
    self.assertEqual(retval, ['--no-xvfb'])


class GPUParallelJobs(unittest.TestCase):
  def testNoOsType(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUParallelJobs(test_config, 'name', {})

  def testParallelJobs(self):
    test_config = CreateConfigWithGpu('vendor:device1-driver')
    for os_type in ['linux', 'mac', 'win']:
      retval = magic_substitutions.GPUParallelJobs(
        test_config, 'name', {'os_type': os_type}
      )
      self.assertEqual(retval, ['--jobs=4'])

  def testWebGPUCTSWindowsIntelSerialJobs(self):
    intel_config = CreateConfigWithGpu('8086:device1-driver')
    amd_config = CreateConfigWithGpu('1002:device1-driver')

    for gpu_config in [intel_config, amd_config]:
      for name, telemetry_test_name in [
        ('webgpu_cts', None),
        (None, 'webgpu_cts'),
      ]:
        is_intel = intel_config == gpu_config
        c = gpu_config.copy()
        if name:
          c['name'] = name
        if telemetry_test_name:
          c['telemetry_test_name'] = telemetry_test_name
        for os_type in ['linux', 'mac', 'win']:
          retval = magic_substitutions.GPUParallelJobs(
            c, 'name', {'os_type': os_type}
          )
          if is_intel and os_type == 'win':
            self.assertEqual(retval, ['--jobs=1'])
          else:
            self.assertEqual(retval, ['--jobs=4'])

  def testWebGLWindowsIntelParallelJobs(self):
    intel_config = CreateConfigWithGpu('8086:device1-driver')
    amd_config = CreateConfigWithGpu('1002:device1-driver')
    for gpu_config in [intel_config, amd_config]:
      for name, telemetry_test_name in [
        ('webgl_conformance', None),
        ('webgl1_conformance', None),
        ('webgl2_conformance', None),
        (None, 'webgl1_conformance'),
        (None, 'webgl2_conformance'),
      ]:
        is_intel = intel_config == gpu_config
        c = gpu_config.copy()
        if name:
          c['name'] = name
        if telemetry_test_name:
          c['telemetry_test_name'] = telemetry_test_name
        for os_type in ['linux', 'mac', 'win']:
          retval = magic_substitutions.GPUParallelJobs(
            c, 'name', {'os_type': os_type}
          )
          if is_intel and os_type == 'win':
            self.assertEqual(retval, ['--jobs=2'])
          else:
            self.assertEqual(retval, ['--jobs=4'])

  def testWebGLMacNvidiaParallelJobs(self):
    amd_config = CreateConfigWithGpu('1002:device1-driver')
    nvidia_config = CreateConfigWithGpu('10de:device1-driver')

    for gpu_config in [nvidia_config, amd_config]:
      for name, telemetry_test_name in [
        ('webgl1_conformance', None),
        (None, 'webgl1_conformance'),
      ]:
        is_nvidia = gpu_config == nvidia_config
        c = gpu_config.copy()
        if name:
          c['name'] = name
        if telemetry_test_name:
          c['telemetry_test_name'] = telemetry_test_name
        for os_type in ['linux', 'mac', 'win']:
          retval = magic_substitutions.GPUParallelJobs(
            c, 'name', {'os_type': os_type}
          )
          if is_nvidia and os_type == 'mac':
            self.assertEqual(retval, ['--jobs=3'])
          else:
            self.assertEqual(retval, ['--jobs=4'])

  def testPixelMacDebugParallelJobs(self):
    gpu_config = CreateConfigWithGpu('1002:device1-driver')
    for name, telemetry_test_name in [
      ('pixel_skia_gold_test', None),
      (None, 'pixel'),
    ]:
      c = gpu_config.copy()
      if name:
        c['name'] = name
      if telemetry_test_name:
        c['telemetry_test_name'] = telemetry_test_name
      for os_type in ['linux', 'mac', 'win']:
        for tester_name in ('Name Debug', 'Name Dbg', 'name debug', 'name dbg'):
          retval = magic_substitutions.GPUParallelJobs(
            c, tester_name, {'os_type': os_type}
          )
          if os_type == 'mac':
            self.assertEqual(retval, ['--jobs=1'])
          else:
            self.assertEqual(retval, ['--jobs=4'])
      # Double check that non-debug Mac pixel tests still get parallelized.
      retval = magic_substitutions.GPUParallelJobs(
        c, 'name release', {'os_type': 'mac'}
      )
      self.assertEqual(retval, ['--jobs=4'])

  def testPixelMacNvidiaParallelJobs(self):
    gpu_config = CreateConfigWithGpu('10de:device1-driver')
    for name, telemetry_test_name in [
      ('pixel_skia_gold_test', None),
      (None, 'pixel'),
    ]:
      c = gpu_config.copy()
      if name:
        c['name'] = name
      if telemetry_test_name:
        c['telemetry_test_name'] = telemetry_test_name
      for os_type in ['linux', 'mac', 'win']:
        retval = magic_substitutions.GPUParallelJobs(
          c, 'name', {'os_type': os_type}
        )
        if os_type == 'mac':
          self.assertEqual(retval, ['--jobs=1'])
        else:
          self.assertEqual(retval, ['--jobs=4'])


class GPUWebGLRuntimeFile(unittest.TestCase):
  def testNoOsType(self):
    test_config = {'telemetry_test_name': 'webgl1_conformance'}
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUWebGLRuntimeFile(test_config, None, {})

  def testNoSuite(self):
    tester_config = {'os_type': 'linux'}
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUWebGLRuntimeFile({}, None, tester_config)

  def testUnknownSuite(self):
    test_config = {'telemetry_test_name': 'foo'}
    tester_config = {'os_type': 'linux'}
    with self.assertRaises(AssertionError):
      magic_substitutions.GPUWebGLRuntimeFile(test_config, None, tester_config)

  def testKnownOsTypes(self):
    for os_type in ('linux', 'mac', 'win'):
      for suite in ('webgl1_conformance', 'webgl2_conformance'):
        retval = magic_substitutions.GPUWebGLRuntimeFile(
          {'telemetry_test_name': suite}, None, {'os_type': os_type}
        )
        self.assertEqual(
          retval,
          [
            '--read-abbreviated-json-results-from=../../content/test/data/gpu/'
            f'{suite}_{os_type}_runtimes.json'
          ],
        )

  def testUnknownOsType(self):
    for suite in ('webgl1_conformance', 'webgl2_conformance'):
      retval = magic_substitutions.GPUWebGLRuntimeFile(
        {'telemetry_test_name': suite}, None, {'os_type': 'foo'}
      )
      self.assertEqual(
        retval,
        [
          '--read-abbreviated-json-results-from=../../content/test/data/gpu/'
          f'{suite}_linux_runtimes.json'
        ],
      )


if __name__ == '__main__':
  unittest.main()
