# Copyright 2024 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@chromium-luci//xcode.star", _xcode = "xcode")

# Xcode versions used by desktop macOS builders.
xcode = struct(
    # Default Xcode Version (Xcode 27.0 RC)
    xcode_default = _xcode.for_mac("27a266a"),

    # Xcode beta version used on beta bots.
    xcode_beta = _xcode.for_mac("27c5020n"),
)
