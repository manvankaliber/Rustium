# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os




def getTargetPath(gen_dir, root_gen_dir):
  root_gen_dir_from_build = os.path.normpath(
    os.path.join(gen_dir, root_gen_dir)
  ).replace('\\', '/')
  return os.path.relpath(gen_dir, root_gen_dir_from_build).replace('\\', '/')
