// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/themes/cross_device/theme_translation.h"

#include "build/build_config.h"
#include "build/buildflag.h"
#include "components/themes/common/image_url_options.h"
#include "url/gurl.h"

namespace themes {

namespace {

// Normalizes the image URL in `ntp_background` by replacing any source
// platform's FIFE image options suffix (e.g., "=w3840-h2160-p-k-no-nd-mv" on
// Desktop or "=s2556-k-no-nd" on Android/iOS) with the local platform's
// `GetImageOptions()`. Leaves `ntp_background` unchanged if `collection_id` is
// unset/empty or if `url` is not a valid HTTP(S) URL.
void NormalizeNtpBackgroundUrl(sync_pb::NtpCustomBackground& ntp_background) {
  if (!ntp_background.has_collection_id() ||
      ntp_background.collection_id().empty()) {
    return;
  }

  // Validate before calling `GURL::spec()`, which must not be called on an
  // invalid GURL (e.g., malformed sync data from a remote client).
  const GURL source_url(ntp_background.url());
  if (!source_url.is_valid() || !source_url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  const GURL url_without_options = RemoveOptionsFromImageURL(source_url.spec());
  if (!url_without_options.is_valid()) {
    return;
  }

  const GURL normalized_url =
      AddOptionsToImageURL(url_without_options.spec(), GetImageOptions());
  if (normalized_url.is_valid()) {
    ntp_background.set_url(normalized_url.spec());
  }
}

}  // namespace

DeviceThemeInfo<sync_pb::ThemeSpecifics> TranslateAndroid(
    const sync_pb::ThemeAndroidSpecifics& android_specifics) {
  DeviceThemeInfo<sync_pb::ThemeSpecifics> info;
  info.data_type = syncer::THEMES_ANDROID;

  if (android_specifics.has_use_custom_theme()) {
    info.theme.set_use_custom_theme(android_specifics.use_custom_theme());
  }
  if (android_specifics.has_ntp_background()) {
    *info.theme.mutable_ntp_background() = android_specifics.ntp_background();
    NormalizeNtpBackgroundUrl(*info.theme.mutable_ntp_background());
  }
  if (android_specifics.has_user_color_theme()) {
    *info.theme.mutable_user_color_theme() =
        android_specifics.user_color_theme();
  }
  return info;
}

DeviceThemeInfo<sync_pb::ThemeSpecifics> TranslateIos(
    const sync_pb::ThemeIosSpecifics& ios_specifics) {
  DeviceThemeInfo<sync_pb::ThemeSpecifics> info;
  info.data_type = syncer::THEMES_IOS;

  if (ios_specifics.has_user_color_theme()) {
    *info.theme.mutable_user_color_theme() = ios_specifics.user_color_theme();
  }
  if (ios_specifics.has_ntp_background()) {
    *info.theme.mutable_ntp_background() = ios_specifics.ntp_background();
    NormalizeNtpBackgroundUrl(*info.theme.mutable_ntp_background());
  }
  return info;
}

}  // namespace themes
