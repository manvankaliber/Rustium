#!/usr/bin/env python3
# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unit tests for sdk_info.py and codesign.py."""

import io
import os
import plistlib
import sys
import tempfile
import unittest
from unittest import mock

import codesign
import sdk_info

class SdkInfoAndToolsTest(unittest.TestCase):
    def test_sdk_info_non_darwin(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            ver_plist = os.path.join(tmpdir, 'Contents', 'version.plist')
            os.makedirs(os.path.dirname(ver_plist))
            with open(ver_plist, 'wb') as f:
                plistlib.dump(
                    {
                        'CFBundleShortVersionString': '27.0',
                        'ProductBuildVersion': '17A324',
                    },
                    f,
                )

            mac_sdk = os.path.join(
                tmpdir,
                'Contents',
                'Developer',
                'Platforms',
                'MacOSX.platform',
                'Developer',
                'SDKs',
                'MacOSX.sdk',
            )
            sys_ver = os.path.join(mac_sdk, 'System', 'Library', 'CoreServices')
            os.makedirs(sys_ver)
            with open(os.path.join(sys_ver, 'SystemVersion.plist'), 'wb') as f:
                plistlib.dump(
                    {
                        'ProductVersion': '27.0',
                        'ProductBuildVersion': '25A354',
                    },
                    f,
                )

            settings = {}
            sdk_info.FillXcodeVersion(settings, tmpdir)
            self.assertEqual(settings['xcode_version'], '2700')
            self.assertEqual(settings['xcode_version_verbatim'], '27.0')
            self.assertEqual(settings['xcode_build'], '17A324')

            with mock.patch.object(sys, 'platform', 'linux'):
                sdk_info.FillMachineOSBuild(settings, tmpdir)
                self.assertEqual(settings['machine_os_build'], '25A354')

                sdk_info.FillSDKPathAndVersion(
                    settings, 'macosx', '2700', tmpdir
                )
                self.assertEqual(settings['sdk_version'], '27.0')
                self.assertEqual(settings['sdk_build'], '25A354')
                self.assertEqual(settings['sdk_path'], mac_sdk)

                if os.name != 'nt':
                    links_dir = os.path.join(
                        tmpdir, 'out', 'Debug', 'sdk_links'
                    )
                    sdk_info.CreateXcodeSymlinkAt(
                        mac_sdk, links_dir, os.path.join(tmpdir, 'out', 'Debug')
                    )
                    link_path = os.path.join(links_dir, 'MacOSX.sdk')
                    self.assertTrue(os.path.islink(link_path))
                    self.assertFalse(os.path.isabs(os.readlink(link_path)))
                    self.assertEqual(
                        os.path.realpath(link_path), os.path.realpath(mac_sdk)
                    )

    def test_codesign_non_darwin(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            profile_path = os.path.join(tmpdir, 'test.mobileprovision')
            plist_bytes = plistlib.dumps(
                {
                    'ApplicationIdentifierPrefix': ['PREFIX'],
                    'Entitlements': {
                        'application-identifier': 'PREFIX.com.example.app'
                    },
                    'ExpirationDate': plistlib.datetime.datetime(
                        2030, 1, 1, tzinfo=plistlib.datetime.timezone.utc
                    ),
                    'UUID': '1234-5678',
                }
            )
            with open(profile_path, 'wb') as f:
                f.write(b'\x30\x82\x01\x00' + plist_bytes + b'\x00\x00')

            with mock.patch.object(sys, 'platform', 'linux'):
                prof = codesign.ProvisioningProfile(profile_path)
                self.assertEqual(prof.application_identifier_prefix, 'PREFIX')

                stderr_buf = io.StringIO()
                with mock.patch('sys.stderr', stderr_buf):
                    codesign.CodeSignBundle('/tmp/app', '-', [])
                self.assertEqual(stderr_buf.getvalue(), '')

                with mock.patch('sys.stderr', stderr_buf):
                    codesign.CodeSignBundle('/tmp/app', 'Developer ID Application', [])
                self.assertIn(
                    'skipping cryptographic codesign', stderr_buf.getvalue()
                )



if __name__ == '__main__':
    unittest.main()
