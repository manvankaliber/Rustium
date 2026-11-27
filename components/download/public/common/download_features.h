// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_FEATURES_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/download/public/common/download_export.h"

namespace download {
namespace features {
// Whether a download can be handled by parallel jobs.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kParallelDownloading);

// Whether we allow the download job resume in a backoff.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kBackoffInDownloading);
#endif
COMPONENTS_DOWNLOAD_EXPORT bool IsBackoffInDownloadingEnabled();


// Whether download notification service uses new unified API based on offline
// item and native persistence of notification IDs.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(
    kDownloadNotificationServiceUnifiedAPI);

// Whether in-progress download manager will be used to initialize download
// service.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(
    kUseInProgressDownloadManagerForDownloadService);

// Whether download resumption is allowed when there are no strong validators.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(
    kAllowDownloadResumptionWithoutStrongValidators);

// Whether parallel download is used for HTTP2 connections.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kUseParallelRequestsForHTTP2);

// Whether parallel download is used for QUIC connections.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kUseParallelRequestsForQUIC);

// Whether to delete downloads that are overwritten by others.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kDeleteOverwrittenDownloads);

// When enabled, completed downloads that started more than
// `kRemoveOldDownloadsFromHistoryMinAge` ago are removed from download history
// when it is loaded, instead of being loaded. Downloaded files are not deleted.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kRemoveOldDownloadsFromHistory);
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE_PARAM(
    base::TimeDelta,
    kRemoveOldDownloadsFromHistoryMinAge);

// Whether to allow changing the size of file buffer.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kAllowFileBufferSizeControl);

// Whether mixed-content PDF links can be downloaded if opening inline.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kAllowedMixedContentInlinePdf);

// Whether to enable async notification manager for downloads.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(
    kEnableAsyncNotificationManagerForDownload);


// Whether to defer download history loading until first use.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(
    kDeferredDownloadHistoryLoading);

#if BUILDFLAG(IS_WIN)
// Whether to enable the "Open with..." context menu option for finished
// downloads on Windows.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(
    kEnableDownloadOpenWithContextMenu);
#endif  // BUILDFLAG(IS_WIN)

// Whether to preserve the HTTP Last-Modified timestamp on downloaded files.
COMPONENTS_DOWNLOAD_EXPORT BASE_DECLARE_FEATURE(kPreserveDownloadLastModified);
}  // namespace features

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_FEATURES_H_
