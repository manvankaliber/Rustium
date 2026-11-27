// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/infobars/confirm_infobar_creator.h"

#include <utility>

#include "build/build_config.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"

#include "chrome/browser/ui/views/infobars/confirm_infobar.h"
#include "ui/views/view.h"

std::unique_ptr<infobars::InfoBar> CreateConfirmInfoBar(
    base::PassKey<ConfirmInfoBarCreationAllowlist> pass_key,
    std::unique_ptr<ConfirmInfoBarDelegate> delegate) {
  return ConfirmInfoBar::Create(pass_key, std::move(delegate));
}

std::unique_ptr<infobars::InfoBar> CreateConfirmInfoBar(
    base::PassKey<ConfirmInfoBarCreationAllowlist> pass_key,
    std::unique_ptr<ConfirmInfoBarDelegate> delegate,
    std::unique_ptr<views::View> custom_message_view) {
  return ConfirmInfoBar::Create(pass_key, std::move(delegate),
                                std::move(custom_message_view));
}
