// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_ACTION_CONTAINER_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_ACTION_CONTAINER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chrome/common/buildflags.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/animation/animation_delegate_views.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/mouse_watcher.h"
#include "ui/views/view.h"

namespace gfx {
class Insets;
}
class BrowserWindowInterface;
class TabStripNudgeButton;

class ScopedTabStripModalUI;

enum class LockedExpansionMode {
  kNone = 0,
  kWillShow,
  kWillHide,
};

class TabStripActionContainer : public views::View,
                                public views::AnimationDelegateViews,
                                public views::MouseWatcherListener {
  METADATA_HEADER(TabStripActionContainer, views::View)

 public:
  class TabStripNudgeAnimationSession {
   public:
    enum class AnimationSessionType { kShow, kHide };

    TabStripNudgeAnimationSession(TabStripNudgeButton* button,
                                  TabStripActionContainer* container,
                                  AnimationSessionType session_type,
                                  base::OnceCallback<void()> on_animation_ended,
                                  bool animate_opacity = true);
    ~TabStripNudgeAnimationSession();
    void ApplyAnimationValue(const gfx::Animation* animation);
    void MarkAnimationDone(const gfx::Animation* animation);
    void Start();
    AnimationSessionType session_type() { return session_type_; }

    gfx::SlideAnimation* expansion_animation() { return &expansion_animation_; }
    void ResetExpansionAnimationForTesting(double value);
    void ResetOpacityAnimationForTesting(double value);

    void Hide();
    TabStripNudgeButton* button() { return button_; }

   private:
    base::TimeDelta GetAnimationDuration(base::TimeDelta duration);
    void ShowOpacityAnimation();
    void Show();
    raw_ptr<TabStripNudgeButton> button_ = nullptr;
    raw_ptr<TabStripActionContainer> container_ = nullptr;

    gfx::SlideAnimation expansion_animation_;
    gfx::SlideAnimation opacity_animation_;

    bool expansion_animation_done_ = false;
    bool opacity_animation_done_ = false;
    AnimationSessionType session_type_;

    // Timer for initiating the opacity animation during show.
    base::OneShotTimer opacity_animation_delay_timer_;

    // Callback to container after animation has ended.
    base::OnceCallback<void()> on_animation_ended_;

    // Adding boolean since the glic nudge is always opaque.
    bool is_opacity_animated_;

    // track animations to delay posting calls that might delete this class.
    bool is_executing_show_or_hide_ = false;
  };

  explicit TabStripActionContainer(
      BrowserWindowInterface* browser_window_interface);
  TabStripActionContainer(const TabStripActionContainer&) = delete;
  TabStripActionContainer& operator=(const TabStripActionContainer&) = delete;
  ~TabStripActionContainer() override;

  TabStripNudgeAnimationSession* animation_session_for_testing() {
    return animation_session_.get();
  }

  views::Separator* GetSeparatorForTesting() { return separator_; }


  // views::View:
  void AddedToWidget() override;

  // views::MouseWatcherListener:
  void MouseMovedOutOfHost() override;

  void UpdateButtonBorders(gfx::Insets button_insets);

 private:
  friend class TabStripActionContainerBrowserTest;

  void ShowTabStripNudge(TabStripNudgeButton* button);
  void HideTabStripNudge(TabStripNudgeButton* button);

  // Update the expansion mode to be executed once the mouse is no longer over
  // the nudge.
  void SetLockedExpansionMode(LockedExpansionMode mode,
                              TabStripNudgeButton* button);



  // TODO(crbug.com/431015299): Clean up when GlicButton and GlicActorTaskIcon
  // have been combined.
  // Container to store the GlicButton and GlicActorTaskIcon when a task is
  // active.
  // Adds a toggle-like background.
  // Update the Glic and GlicActor button borders when showing or hiding the
  // task icon container.

  void OnTabStripNudgeButtonTimeout(TabStripNudgeButton* button);

  // View where, if the mouse is currently over its bounds, the expansion state
  // will not change. Changes will be staged until after the mouse exits the
  // bounds of this View.
  raw_ptr<View, DanglingUntriaged> locked_expansion_view_ = nullptr;

  // MouseWatcher is used to lock and unlock the expansion state of this
  // container.
  std::unique_ptr<views::MouseWatcher> mouse_watcher_;

  // views::AnimationDelegateViews
  void AnimationCanceled(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;

  void ExecuteShowTabStripNudge(TabStripNudgeButton* button);
  void ExecuteHideTabStripNudge(TabStripNudgeButton* button);

  void OnAnimationSessionEnded();

  bool ButtonOwnsAnimation(const TabStripNudgeButton* button) const;

  // Helper to handles teardown logic when the task icon is fully gone.

  // Update visibility of glic actor button

  // Update visibility of glic button and action container

  // Update visibility of separator.

  // The button currently holding the lock to be shown/hidden.
  raw_ptr<TabStripNudgeButton> locked_expansion_button_ = nullptr;

  raw_ptr<views::Separator> separator_ = nullptr;


  const raw_ptr<BrowserWindowInterface> browser_window_interface_ = nullptr;

  // Timer for hiding tab_strip_nudge_button_ after show.
  base::OneShotTimer hide_tab_strip_nudge_timer_;

  // When locked, the container is unable to change its expanded state.
  // Changes will be staged until after this is unlocked.
  LockedExpansionMode locked_expansion_mode_ = LockedExpansionMode::kNone;

  // Prevents other features from showing tabstrip-modal UI.
  std::unique_ptr<ScopedTabStripModalUI> scoped_tab_strip_modal_ui_;

  std::unique_ptr<TabStripNudgeAnimationSession> animation_session_;

  // Border insets as passed down from the HorizontalTabStripRegionView, used to
  // update button view borders.
  gfx::Insets border_insets_;

  base::WeakPtrFactory<TabStripActionContainer> weak_factory_{this};
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_ACTION_CONTAINER_H_
