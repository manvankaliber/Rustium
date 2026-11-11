// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip_action_container.h"

#include <memory>

#include "base/feature_list.h"
#include "base/notimplemented.h"
#include "base/task/single_thread_task_runner.h"
#include "chrome/browser/tab_list/tab_list_interface.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/user_education/browser_user_education_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/frame_separator.h"
#include "chrome/browser/ui/views/tabs/tab_strip_nudge_button.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/branded_strings.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/animation/tween.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/mouse_watcher.h"
#include "ui/views/mouse_watcher_view_host.h"
#include "ui/views/view_class_properties.h"

#include "base/feature_list.h"
#include "base/time/time.h"

namespace {

constexpr base::TimeDelta kExpansionInDuration = base::Milliseconds(500);
constexpr base::TimeDelta kExpansionOutDuration = base::Milliseconds(250);
constexpr base::TimeDelta kOpacityInDuration = base::Milliseconds(300);
constexpr base::TimeDelta kOpacityOutDuration = base::Milliseconds(100);
constexpr base::TimeDelta kOpacityDelay = base::Milliseconds(100);

}  // namespace

TabStripActionContainer::TabStripNudgeAnimationSession::
    TabStripNudgeAnimationSession(TabStripNudgeButton* button,
                                  TabStripActionContainer* container,
                                  AnimationSessionType session_type,
                                  base::OnceCallback<void()> on_animation_ended,
                                  bool is_opacity_animated)
    : button_(button),
      container_(container),
      expansion_animation_(container),
      opacity_animation_(container),
      session_type_(session_type),
      on_animation_ended_(std::move(on_animation_ended)),
      is_opacity_animated_(is_opacity_animated),
      is_executing_show_or_hide_(false) {
  if (session_type_ == AnimationSessionType::kHide) {
    expansion_animation_.Reset(1);
    if (is_opacity_animated) {
      opacity_animation_.Reset(1);
    }
  }
}

TabStripActionContainer::TabStripNudgeAnimationSession::
    ~TabStripNudgeAnimationSession() = default;

void TabStripActionContainer::TabStripNudgeAnimationSession::
    ApplyAnimationValue(const gfx::Animation* animation) {
  float value = animation->GetCurrentValue();
  if (animation == &expansion_animation_) {
    button_->SetWidthFactor(value);
  } else if (animation == &opacity_animation_) {
    button_->SetOpacity(value);
  }
}

void TabStripActionContainer::TabStripNudgeAnimationSession::MarkAnimationDone(
    const gfx::Animation* animation) {
  if (animation == &expansion_animation_) {
    expansion_animation_done_ = true;
  } else {
    opacity_animation_done_ = true;
  }

  const bool opacity_animation_not_running =
      opacity_animation_done_ || !is_opacity_animated_;

  if (expansion_animation_done_ && opacity_animation_not_running) {
    if (on_animation_ended_) {
      if (is_executing_show_or_hide_) {
        base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, std::move(on_animation_ended_));
      } else {
        std::move(on_animation_ended_).Run();
      }
    }
  }
}

void TabStripActionContainer::TabStripNudgeAnimationSession::Start() {
  if (session_type_ ==
      TabStripNudgeAnimationSession::AnimationSessionType::kShow) {
    Show();
  } else {
    Hide();
  }
}

void TabStripActionContainer::TabStripNudgeAnimationSession::
    ResetExpansionAnimationForTesting(double value) {
  expansion_animation_.Reset(value);
}

void TabStripActionContainer::TabStripNudgeAnimationSession::
    ResetOpacityAnimationForTesting(double value) {
  if (is_opacity_animated_) {
    if (opacity_animation_delay_timer_.IsRunning()) {
      opacity_animation_delay_timer_.FireNow();
    }
  }

  if (is_opacity_animated_) {
    opacity_animation_.Reset(value);
  }
}

void TabStripActionContainer::TabStripNudgeAnimationSession::Hide() {
  base::AutoReset<bool> resetter(&is_executing_show_or_hide_, true);
  // Animate and hide existing chip.
  if (session_type_ ==
      TabStripNudgeAnimationSession::AnimationSessionType::kShow) {
    if (is_opacity_animated_ && opacity_animation_delay_timer_.IsRunning()) {
      opacity_animation_delay_timer_.FireNow();
    }
    session_type_ = TabStripNudgeAnimationSession::AnimationSessionType::kHide;
  }

  expansion_animation_.SetTweenType(gfx::Tween::Type::ACCEL_20_DECEL_100);

  expansion_animation_.SetSlideDuration(
      GetAnimationDuration(kExpansionOutDuration));
  expansion_animation_.Hide();

  if (is_opacity_animated_) {
    opacity_animation_.SetTweenType(gfx::Tween::Type::LINEAR);
    opacity_animation_.SetSlideDuration(
        GetAnimationDuration(kOpacityOutDuration));
    opacity_animation_.Hide();
  }
}

base::TimeDelta
TabStripActionContainer::TabStripNudgeAnimationSession::GetAnimationDuration(
    base::TimeDelta duration) {
  return gfx::Animation::ShouldRenderRichAnimation() ? duration
                                                     : base::TimeDelta();
}

void TabStripActionContainer::TabStripNudgeAnimationSession::
    ShowOpacityAnimation() {
  opacity_animation_.Show();
}

void TabStripActionContainer::TabStripNudgeAnimationSession::Show() {
  base::AutoReset<bool> resetter(&is_executing_show_or_hide_, true);
  expansion_animation_.SetTweenType(gfx::Tween::Type::ACCEL_20_DECEL_100);
  if (is_opacity_animated_) {
    opacity_animation_.SetTweenType(gfx::Tween::Type::LINEAR);
  }
  expansion_animation_.SetSlideDuration(
      GetAnimationDuration(kExpansionInDuration));
  expansion_animation_.Show();

  if (is_opacity_animated_) {
    opacity_animation_.SetSlideDuration(
        GetAnimationDuration(kOpacityInDuration));

    const base::TimeDelta delay = GetAnimationDuration(kOpacityDelay);
    opacity_animation_delay_timer_.Start(
        FROM_HERE, delay, this,
        &TabStripActionContainer::TabStripNudgeAnimationSession::
            ShowOpacityAnimation);
  }
}

TabStripActionContainer::TabStripActionContainer(
    BrowserWindowInterface* browser_window_interface)
    : AnimationDelegateViews(this),
      locked_expansion_view_(this),
      browser_window_interface_(browser_window_interface) {
  SetProperty(views::kElementIdentifierKey, kTabStripActionContainerElementId);

  mouse_watcher_ = std::make_unique<views::MouseWatcher>(
      std::make_unique<views::MouseWatcherViewHost>(locked_expansion_view_,
                                                    gfx::Insets()),
      this);



  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCollapseMargins(false);

}

TabStripActionContainer::~TabStripActionContainer() {
  if (scoped_tab_strip_modal_ui_) {
    scoped_tab_strip_modal_ui_.reset();
  }
}

void TabStripActionContainer::AddedToWidget() {
  views::View::AddedToWidget();
}

void TabStripActionContainer::MouseMovedOutOfHost() {
  SetLockedExpansionMode(LockedExpansionMode::kNone, nullptr);
}
















void TabStripActionContainer::UpdateButtonBorders(
    const gfx::Insets border_insets) {
  border_insets_ = border_insets;

}

void TabStripActionContainer::ShowTabStripNudge(TabStripNudgeButton* button) {
  if (locked_expansion_view_->IsMouseHovered()) {
    SetLockedExpansionMode(LockedExpansionMode::kWillShow, button);
  }
  if (locked_expansion_mode_ == LockedExpansionMode::kNone) {
    ExecuteShowTabStripNudge(button);
  }
}

void TabStripActionContainer::HideTabStripNudge(TabStripNudgeButton* button) {
  if (locked_expansion_view_->IsMouseHovered()) {
    SetLockedExpansionMode(LockedExpansionMode::kWillHide, button);
  }
  if (locked_expansion_mode_ == LockedExpansionMode::kNone) {
    ExecuteHideTabStripNudge(button);
  }
}

void TabStripActionContainer::SetLockedExpansionMode(
    LockedExpansionMode mode,
    TabStripNudgeButton* button) {
  if (mode == LockedExpansionMode::kNone) {
    if (locked_expansion_mode_ == LockedExpansionMode::kWillShow) {
      ExecuteShowTabStripNudge(locked_expansion_button_);
    } else if (locked_expansion_mode_ == LockedExpansionMode::kWillHide) {
      ExecuteHideTabStripNudge(locked_expansion_button_);
    }
    locked_expansion_button_ = nullptr;
  } else {
    locked_expansion_button_ = button;
    mouse_watcher_->Start(GetWidget()->GetNativeWindow());
  }
  locked_expansion_mode_ = mode;
}









void TabStripActionContainer::OnTabStripNudgeButtonTimeout(
    TabStripNudgeButton* button) {
  // Hide the button if not pressed. Use locked expansion mode to avoid
  // disrupting the user.
  HideTabStripNudge(button);
}

void TabStripActionContainer::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

void TabStripActionContainer::AnimationEnded(const gfx::Animation* animation) {
  animation_session_->ApplyAnimationValue(animation);
  animation_session_->MarkAnimationDone(animation);
}

void TabStripActionContainer::AnimationProgressed(
    const gfx::Animation* animation) {
  animation_session_->ApplyAnimationValue(animation);
}

void TabStripActionContainer::ExecuteShowTabStripNudge(
    TabStripNudgeButton* button) {
  bool can_show_modal_ui =
      browser_window_interface_->GetTabStripModel()->CanShowModalUI();
  // The tab strip might currently be animating a hide for this button. If we
  // receive a show request during this time, we allow it to interrupt the hide
  // animation so the button can re-expand immediately.
  if (!can_show_modal_ui && animation_session_ &&
      animation_session_->session_type() ==
          TabStripNudgeAnimationSession::AnimationSessionType::kHide &&
      animation_session_->button() == button) {
    can_show_modal_ui = true;
  }
  if (!can_show_modal_ui) {
    return;
  }

  button->SetIsShowingNudge(true);

  button->SetProperty(views::kMarginsKey, gfx::Insets());
  scoped_tab_strip_modal_ui_.reset();
  scoped_tab_strip_modal_ui_ =
      browser_window_interface_->GetTabStripModel()->ShowModalUI();

  if (!ButtonOwnsAnimation(button)) {
    animation_session_ = std::make_unique<TabStripNudgeAnimationSession>(
        button, this, TabStripNudgeAnimationSession::AnimationSessionType::kShow,
        base::BindOnce(&TabStripActionContainer::OnAnimationSessionEnded,
                       weak_factory_.GetWeakPtr()));
    animation_session_->Start();
  }
}

void TabStripActionContainer::ExecuteHideTabStripNudge(
    TabStripNudgeButton* button) {
  // Hide the current animation if the shown button is the same button. Do not
  // create a new animation session.
  if (animation_session_ &&
      animation_session_->session_type() ==
          TabStripNudgeAnimationSession::AnimationSessionType::kShow &&
      animation_session_->button() == button) {
    hide_tab_strip_nudge_timer_.Stop();
    animation_session_->Hide();
    return;
  }

  if (!button->GetVisible()) {
    return;
  }
  button->SetIsShowingNudge(false);

  // Reset the modal UI lock immediately when hiding starts, so that subsequent
  // show requests (e.g. from a quick selection change) aren't blocked by
  // CanAcquireLock checks.
  scoped_tab_strip_modal_ui_.reset();

  // Stop the timer since the chip might be getting hidden on user actions like
  // dismissal or click and not timeout.
  hide_tab_strip_nudge_timer_.Stop();
  if (!ButtonOwnsAnimation(button)) {
    animation_session_ = std::make_unique<TabStripNudgeAnimationSession>(
        button, this, TabStripNudgeAnimationSession::AnimationSessionType::kHide,
        base::BindOnce(&TabStripActionContainer::OnAnimationSessionEnded,
                       weak_factory_.GetWeakPtr()));
    animation_session_->Start();
  }
}

void TabStripActionContainer::OnAnimationSessionEnded() {
  // If the button went from shown -> hidden, unblock the tab strip from
  // showing other modal UIs.
  bool should_reset_modal_ui =
      (animation_session_ &&
       animation_session_->session_type() ==
           TabStripNudgeAnimationSession::AnimationSessionType::kHide);

  // Handle state transitions for the actor task icon, which uses two distinct
  // animation modes:
  // 1. kEntry: The icon animating into existence.
  // 2. kNudge: The label expanding/collapsing.
  if (should_reset_modal_ui) {
    scoped_tab_strip_modal_ui_.reset();

    if (locked_expansion_button_) {
      locked_expansion_button_->SetIsShowingNudge(false);
    }
  }

  animation_session_.reset();
}

bool TabStripActionContainer::ButtonOwnsAnimation(
    const TabStripNudgeButton* button) const {
  return false;
}




BEGIN_METADATA(TabStripActionContainer)
END_METADATA
