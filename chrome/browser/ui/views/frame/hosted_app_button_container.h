// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_BUTTON_CONTAINER_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_BUTTON_CONTAINER_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/content_setting_image_view.h"
#include "chrome/browser/ui/views/toolbar/browser_actions_container.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/view.h"

namespace {
class HostedAppNonClientFrameViewAshTest;
}

class AppMenuButton;
class BrowserView;
class HostedAppMenuButton;

namespace views {
class MenuButton;
}

// A container for hosted app buttons in the title bar.
class HostedAppButtonContainer : public views::AccessiblePaneView,
                                 public BrowserActionsContainer::Delegate,
                                 public ToolbarButtonProvider,
                                 public ImmersiveModeController::Observer {
 public:
  // |active_icon_color| and |inactive_icon_color| indicate the colors to use
  // for button icons when the window is focused and blurred respectively.
  HostedAppButtonContainer(BrowserView* browser_view,
                           SkColor active_icon_color,
                           SkColor inactive_icon_color);
  ~HostedAppButtonContainer() override;

  // Updates the visibility of each content setting.
  void RefreshContentSettingViews();

  // Sets the container to paints its buttons the active/inactive color.
  void SetPaintAsActive(bool active);

  // Animates the menu button and content setting icons. Intended to run in sync
  // with a FrameHeaderOriginText slide animation.
  void StartTitlebarAnimation(base::TimeDelta origin_text_slide_duration);

 private:
  friend class HostedAppNonClientFrameViewAshTest;
  friend class ImmersiveModeControllerAshHostedAppBrowserTest;

  static void DisableAnimationForTesting();

  class ContentSettingsContainer;

  views::View* GetContentSettingContainerForTesting();

  const std::vector<ContentSettingImageView*>&
  GetContentSettingViewsForTesting() const;

  void FadeInContentSettingButtons();

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;
  void ChildVisibilityChanged(views::View* child) override;

  // BrowserActionsContainer::Delegate:
  views::MenuButton* GetOverflowReferenceView() override;
  base::Optional<int> GetMaxBrowserActionsWidth() const override;
  std::unique_ptr<ToolbarActionsBar> CreateToolbarActionsBar(
      ToolbarActionsBarDelegate* delegate,
      Browser* browser,
      ToolbarActionsBar* main_bar) const override;

  // ToolbarButtonProvider:
  BrowserActionsContainer* GetBrowserActionsContainer() override;
  PageActionIconContainerView* GetPageActionIconContainerView() override;
  AppMenuButton* GetAppMenuButton() override;
  void FocusToolbar() override;
  views::AccessiblePaneView* GetAsAccessiblePaneView() override;

  // ImmersiveModeController::Observer:
  void OnImmersiveRevealStarted() override;

  // The containing browser view.
  BrowserView* browser_view_;

  // Button colors.
  const SkColor active_icon_color_;
  const SkColor inactive_icon_color_;

  base::OneShotTimer fade_in_content_setting_buttons_timer_;

  // Owned by the views hierarchy.
  HostedAppMenuButton* app_menu_button_ = nullptr;
  ContentSettingsContainer* content_settings_container_ = nullptr;
  PageActionIconContainerView* page_action_icon_container_view_ = nullptr;
  BrowserActionsContainer* browser_actions_container_ = nullptr;

  base::OneShotTimer opening_animation_timer_;

  DISALLOW_COPY_AND_ASSIGN(HostedAppButtonContainer);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_BUTTON_CONTAINER_H_
