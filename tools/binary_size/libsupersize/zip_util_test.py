#!/usr/bin/env python3
# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import io
import unittest
import zipfile

import zip_util


class ZipUtilTest(unittest.TestCase):

  def testReadZipInfoExtraFieldLength(self):
    data = io.BytesIO()
    with zipfile.ZipFile(data, 'w') as z:
      for name, extra in [('a', b'\x01\x00\x00\x00'),
                          ('b', b'\x02\x00\x04\x00data')]:
        info = zipfile.ZipInfo(name)
        info.extra = extra
        z.writestr(info, name)
    with zipfile.ZipFile(data) as z:
      infos = z.infolist()
      for info in infos:
        info.extra = b''  # Prove that the local header is read independently.
      self.assertEqual(
          [4, 8], [zip_util.ReadZipInfoExtraFieldLength(z, i) for i in infos])


if __name__ == '__main__':
  unittest.main()
