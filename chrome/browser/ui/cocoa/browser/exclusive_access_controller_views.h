// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_EXCLUSIVE_ACCESS_CONTROLLER_VIEWS_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_EXCLUSIVE_ACCESS_CONTROLLER_VIEWS_H_

// Note this file has a _views suffix so that it may have an optional runtime
// dependency on toolkit-views UI.

#import <CoreGraphics/CoreGraphics.h>

#include <memory>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/views/exclusive_access_bubble_views_context.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/accelerators/accelerator.h"

class Browser;
class BrowserWindow;
@class BrowserWindowController;
class ExclusiveAccessBubbleViews;
class FullscreenControlHost;
class GURL;

namespace views {
class EventMonitor;
}

// Component placed into a browser window controller to manage communication
// with subtle notification bubbles, which appear for events such as entering
// fullscreen.
class ExclusiveAccessController : public ExclusiveAccessContext,
                                  public ui::AcceleratorProvider,
                                  public ExclusiveAccessBubbleViewsContext {
 public:
  ExclusiveAccessController(BrowserWindowController* controller,
                            Browser* browser);
  ~ExclusiveAccessController() override;

  const GURL& url() const { return url_; }
  ExclusiveAccessBubbleType bubble_type() const { return bubble_type_; }

  // Shows the bubble once the NSWindow has received -windowDidEnterFullScreen:.
  void Show();

  // Closes any open bubble.
  void Destroy();

  // ExclusiveAccessContext:
  Profile* GetProfile() override;
  bool IsFullscreen() const override;
  void UpdateUIForTabFullscreen(TabFullscreenState state) override;
  void UpdateFullscreenToolbar() override;
  void EnterFullscreen(const GURL& url,
                       ExclusiveAccessBubbleType type) override;
  void ExitFullscreen() override;
  void UpdateExclusiveAccessExitBubbleContent(
      const GURL& url,
      ExclusiveAccessBubbleType bubble_type,
      ExclusiveAccessBubbleHideCallback bubble_first_hide_callback,
      bool force_update) override;
  void OnExclusiveAccessUserInput() override;
  content::WebContents* GetActiveWebContents() override;
  void UnhideDownloadShelf() override;
  void HideDownloadShelf() override;
  bool ShouldHideUIForFullscreen() const override;
  ExclusiveAccessBubbleViews* GetExclusiveAccessBubble() override;

  // ui::AcceleratorProvider:
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;

  // ExclusiveAccessBubbleViewsContext:
  ExclusiveAccessManager* GetExclusiveAccessManager() override;
  views::Widget* GetBubbleAssociatedWidget() override;
  ui::AcceleratorProvider* GetAcceleratorProvider() override;
  gfx::NativeView GetBubbleParentView() const override;
  gfx::Point GetCursorPointInParent() const override;
  gfx::Rect GetClientAreaBoundsInScreen() const override;
  bool IsImmersiveModeEnabled() const override;
  gfx::Rect GetTopContainerBoundsInScreen() override;
  void DestroyAnyExclusiveAccessBubble() override;
  bool CanTriggerOnMouse() const override;

 private:
  BrowserWindow* GetBrowserWindow() const;

  // Gets the FullscreenControlHost for this BrowserView, creating it if it does
  // not yet exist.
  FullscreenControlHost* GetFullscreenControlHost();

  BrowserWindowController* controller_;  // Weak. Owns |this|.
  Browser* browser_;                     // Weak. Owned by controller.

  // When going fullscreen for a tab, we need to store the URL and the
  // fullscreen type, since we can't show the bubble until
  // -windowDidEnterFullScreen: gets called.
  GURL url_;
  ExclusiveAccessBubbleType bubble_type_;
  ExclusiveAccessBubbleHideCallback bubble_first_hide_callback_;

  std::unique_ptr<ExclusiveAccessBubbleViews> views_bubble_;
  std::unique_ptr<FullscreenControlHost> fullscreen_control_host_;
  std::unique_ptr<views::EventMonitor> fullscreen_control_host_event_monitor_;

  // Used to keep track of the kShowFullscreenToolbar preference.
  PrefChangeRegistrar pref_registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExclusiveAccessController);
};

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_EXCLUSIVE_ACCESS_CONTROLLER_VIEWS_H_
