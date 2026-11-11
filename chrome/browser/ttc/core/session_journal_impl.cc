// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ttc/core/session_journal_impl.h"

#include <optional>
#include <utility>

#include "base/check_op.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/cstring_view.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "chrome/browser/ttc/core/switches.h"
#include "components/actor/core/aggregated_journal_file_serializer.h"
#include "components/actor/core/journal_details_builder.h"
#include "components/ttc/app/public/server_journal_event.h"
#include "url/gurl.h"

namespace ttc {

namespace {


void EnsureUniquePath(
    const base::FilePath& file_path,
    base::OnceCallback<void(const base::FilePath&)> callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&base::GetUniquePathWithSuffixFormat, file_path,
                     base::cstring_view("_%d")),
      std::move(callback));
}

}  // namespace


SessionJournalImpl::~SessionJournalImpl() = default;



void SessionJournalImpl::LogAudio(std::string_view mime_type,
                                  base::span<const uint8_t> data) {
  journal_->LogScreenshot(GURL(), task_id_, mime_type, data,
                          /*iframe_data=*/std::nullopt);
}


void SessionJournalImpl::HandleServerJournalEvent(
    const ServerJournalEvent& event) {
  for (const ServerJournalEvent::Details& detail : event.details) {
    details_builder.Add(detail.key, detail.value);
  }
  const base::Time timestamp = event.timestamp + server_clock_delta_;

  if (event.type != ServerJournalEvent::Type::kClockSync &&
      !received_clock_sync_) {
    // The ClockSync event must be the first journal event received.
  }

  uint64_t track_uuid;
  switch (event.track) {
    case ServerJournalEvent::Track::kUnspecified:
    case ServerJournalEvent::Track::kMain:
      break;
    case ServerJournalEvent::Track::kTurn:
      break;
  }

  switch (event.type) {
    case ServerJournalEvent::Type::kInstant:
      journal_->Log(GURL(), task_id_, track_uuid, timestamp, event.name,
                    std::move(details_builder).Build());
      break;
    case ServerJournalEvent::Type::kAsyncBegin:
      // If there is a matching ID, make sure it terminates before the new
      // event is created.
      pending_server_async_events_.erase(event.async_event_id);
      pending_server_async_events_[event.async_event_id] =
          journal_->CreatePendingAsyncEntry(GURL(), task_id_, track_uuid,
                                            timestamp, event.name,
                                            std::move(details_builder).Build());
      break;
    case ServerJournalEvent::Type::kAsyncEnd: {
      auto it = pending_server_async_events_.find(event.async_event_id);
      if (it == pending_server_async_events_.end()) {
        details_builder.Add("event_name", event.name)
            .Add("async_event_id", event.async_event_id);
        journal_->Log(GURL(), task_id_, track_uuid, timestamp,
                      "UnmatchedAsyncEnd", std::move(details_builder).Build());
        break;
      }
      it->second->EndEntry(timestamp, std::move(details_builder).Build());
      pending_server_async_events_.erase(it);
      break;
    }
    case ServerJournalEvent::Type::kClockSync: {
      received_clock_sync_ = true;
      if (event.sync_timestamp <= base::Time::UnixEpoch()) {
        // Avoid adjusting if the server doesn't provide a timestamp or sends
        // an obviously bogus one.
        break;
      }
      const base::Time client_receive_time = base::Time::Now();
      const base::Time client_midpoint =
          event.sync_timestamp +
          (client_receive_time - event.sync_timestamp) / 2;
      server_clock_delta_ = client_midpoint - event.timestamp;
      details_builder.Add("clock_delta", server_clock_delta_);
      journal_->Log(GURL(), task_id_, track_uuid, client_midpoint,
                    event.name.empty() ? "ClockSync" : event.name,
                    std::move(details_builder).Build());
      break;
    }
    case ServerJournalEvent::Type::kUnspecified:
      break;
  }
}

void SessionJournalImpl::StartSerializingToFile(
    const base::FilePath& file_path) {
  LOG(ERROR) << "Ttc Journal: " << file_path;
  file_journal_serializer_->Init(
      file_path, base::BindOnce(&SessionJournalImpl::OnSerializerInitDone,
                                weak_ptr_factory_.GetWeakPtr()));
}

void SessionJournalImpl::OnSerializerInitDone(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to serialize Ttc Journal";
    file_journal_serializer_.reset();
  }
}

}  // namespace ttc
