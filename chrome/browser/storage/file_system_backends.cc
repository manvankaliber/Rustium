// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/storage/file_system_backends.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "extensions/buildflags/buildflags.h"
#include "storage/browser/file_system/file_system_backend.h"


#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/sync_file_system/local/sync_file_system_backend.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

void GetAdditionalFileSystemBackends(
    content::BrowserContext* browser_context,
    const base::FilePath& storage_partition_path,
    std::vector<std::unique_ptr<storage::FileSystemBackend>>*
        additional_backends) {
  // A BrowserContext without a path keeps all its storage in memory, and has
  // nowhere for these backends to put their files.
  if (browser_context->GetPath().empty()) {
    return;
  }


#if BUILDFLAG(ENABLE_EXTENSIONS)
  additional_backends->push_back(
      std::make_unique<sync_file_system::SyncFileSystemBackend>(
          Profile::FromBrowserContext(browser_context)));
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}
