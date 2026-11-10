// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEND_TAB_TO_SELF_FEATURES_H_
#define COMPONENTS_SEND_TAB_TO_SELF_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"
#include "build/buildflag.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace send_tab_to_self {

// If this feature is enabled, the back/forward history of the shared tab will
// be captured and propagated to the target device.
BASE_DECLARE_FEATURE(kSendTabToSelfPropagateNavigationHistory);


#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// If this feature is enabled, a QR code promo is shown in the Send Tab to Self
// bubble on desktop when no target devices are found. Guarded to Windows, Mac,
// and Linux because it relies on `switches::kCrossDeviceSigninFromDesktopUrl`
// in `components/signin/public/base/signin_switches.h`.
BASE_DECLARE_FEATURE(kSendTabToSelfNoTargetDeviceQrCode);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

// If this feature is enabled, desktop entry points get submenu treatment as
// part of the enhanced desktop UI v2.
BASE_DECLARE_FEATURE(kSendTabToSelfEnhancedDesktopUIv2);

// If this feature is enabled, desktop entry points show a submenu with a
// sign-in promo for the signed-out, sign-in pending (reauth), and
// no-target-device (when `kSendTabToSelfNoTargetDeviceQrCode` is also enabled)
// states instead of a single command item.
BASE_DECLARE_FEATURE(kSendTabToSelfSubmenuSigninPromos);

// If this feature is enabled, Send Tab to Self promo bubbles on desktop
// (sign-in, reauth, and no-target-device) anchor to the profile pill and stay
// open across tab navigations instead of popping out and anchoring to the
// Send Tab to Self toolbar icon.
BASE_DECLARE_FEATURE(kSendTabToSelfAnchorPromosToProfilePill);

// If this feature is enabled, multi-tab sharing via Send Tab to Self is
// supported.
BASE_DECLARE_FEATURE(kSendTabToSelfMultiTabShare);




}  // namespace send_tab_to_self

#endif  // COMPONENTS_SEND_TAB_TO_SELF_FEATURES_H_
