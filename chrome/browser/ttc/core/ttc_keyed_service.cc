// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ttc/core/ttc_keyed_service.h"

#include <utility>

namespace ttc {

TtcKeyedService* TtcKeyedService::Get(content::BrowserContext* context) {
  return nullptr;
}

TtcKeyedService::TtcKeyedService(Profile* profile,
                                 ConversationFactory conversation_factory)
    : profile_(profile), conversation_factory_(std::move(conversation_factory)) {}
TtcKeyedService::~TtcKeyedService() = default;

void TtcKeyedService::Shutdown() {}
bool TtcKeyedService::IsEnabled() const { return false; }
ServiceState TtcKeyedService::GetState() const {
  return ServiceState::kProfileIneligible;
}
void TtcKeyedService::StartSession(
    std::unique_ptr<AudioController> audio_controller) {}
void TtcKeyedService::EndSession() {}
base::CallbackListSubscription TtcKeyedService::RegisterStateChangedCallback(
    base::RepeatingCallback<void(ServiceState)> callback) {
  return state_changed_callbacks_.Add(std::move(callback));
}
SessionController* TtcKeyedService::session_controller() { return nullptr; }
bool TtcKeyedService::SetMicMuted(bool muted) { return false; }
bool TtcKeyedService::IsMicMuted() const { return false; }
base::WeakPtr<TtcKeyedService> TtcKeyedService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ttc
