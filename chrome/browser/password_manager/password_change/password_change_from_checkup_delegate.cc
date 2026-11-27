// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_change/password_change_from_checkup_delegate.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "components/password_manager/core/browser/password_string.h"
#include "content/public/browser/web_contents.h"

PasswordChangeFromCheckupDelegate::PasswordChangeFromCheckupDelegate() =
    default;

PasswordChangeFromCheckupDelegate::~PasswordChangeFromCheckupDelegate() {
  actuator_.reset();
}

void PasswordChangeFromCheckupDelegate::StartPasswordChangeFlow(
    password_manager::StoredCredential credential,
    base::WeakPtr<content::WebContents> web_contents,
    StateChangeCallback callback) {
  if (!web_contents) {
    return;
  }
  web_contents_ = web_contents;
  state_change_callback_ = std::move(callback);

  if (!actuator_) {
    if (state_change_callback_) {
      state_change_callback_.Run(PasswordAutomaticChangeState::kError);
    }
    return;
  }
  actuator_->Start();
}


void PasswordChangeFromCheckupDelegate::OnActuationStateChanged(
    PasswordChangeActuator::State new_state) {
  if (!state_change_callback_) {
    return;
  }

  switch (new_state) {
    case PasswordChangeActuator::State::kWaitingForChangePasswordForm:
      state_change_callback_.Run(
          PasswordAutomaticChangeState::kAttemptingSignIn);
      break;
    case PasswordChangeActuator::State::kChangingPassword:
      state_change_callback_.Run(
          PasswordAutomaticChangeState::kChangingPassword);
      break;
    case PasswordChangeActuator::State::kPasswordSuccessfullyChanged:
      state_change_callback_.Run(
          PasswordAutomaticChangeState::kPasswordChangedSuccessfully);
      break;
    case PasswordChangeActuator::State::kPasswordChangeFailed:
    case PasswordChangeActuator::State::kChangePasswordFormNotFound:
    case PasswordChangeActuator::State::kOtpDetected:
      state_change_callback_.Run(PasswordAutomaticChangeState::kError);
      break;
  }
}

void PasswordChangeFromCheckupDelegate::OpenActuationTab() {
  if (actuator_) {
    actuator_->OpenPasswordChangeTab(web_contents_.get());
  }
}

password_manager::PasswordString
PasswordChangeFromCheckupDelegate::generated_password() const {
  return actuator_ ? actuator_->GetGeneratedPassword()
                   : password_manager::PasswordString();
}
