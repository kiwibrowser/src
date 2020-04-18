// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_NON_CLIENT_FRAME_VIEW_MUS_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_NON_CLIENT_FRAME_VIEW_MUS_H_

#include <memory>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tab_icon_view_model.h"

#if !defined(OS_CHROMEOS)
#include "chrome/browser/ui/views/frame/avatar_button_manager.h"
#endif

class TabIconView;

// TODO: Make sure caption buttons ink drop effects work with immersive
// fullscreen mode browsers. https://crbug.com/840242.
class BrowserNonClientFrameViewMus : public BrowserNonClientFrameView,
                                     public TabIconViewModel {
 public:
  static const char kViewClassName[];

  BrowserNonClientFrameViewMus(BrowserFrame* frame, BrowserView* browser_view);
  ~BrowserNonClientFrameViewMus() override;

  void Init();

  // BrowserNonClientFrameView:
  void OnBrowserViewInitViewsComplete() override;
  gfx::Rect GetBoundsForTabStrip(views::View* tabstrip) const override;
  int GetTopInset(bool restored) const override;
  int GetThemeBackgroundXInset() const override;
  void UpdateThrobber(bool running) override;
  void UpdateClientArea() override;
  void UpdateMinimumSize() override;
  int GetTabStripLeftInset() const override;
  void OnTabsMaxXChanged() override;

  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask) override;
  void ResetWindowControls() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;
  const char* GetClassName() const override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  gfx::Size GetMinimumSize() const override;
  void OnThemeChanged() override;

  // TabIconViewModel:
  bool ShouldTabIconViewAnimate() const override;
  gfx::ImageSkia GetFaviconForTabIconView() override;

 protected:
  // BrowserNonClientFrameView:
  AvatarButtonStyle GetAvatarButtonStyle() const override;

 private:
  // Distance between the right edge of the NonClientFrameView and the tab
  // strip.
  int GetTabStripRightInset() const;

  // Returns true if the header should be painted so that it looks the same as
  // the header used for packaged apps. Packaged apps use a different color
  // scheme than browser windows.
  bool UsePackagedAppHeaderStyle() const;

  // Layout the profile switcher (if there is one).
  void LayoutProfileSwitcher();

  // Returns true if there is anything to paint. Some fullscreen windows do not
  // need their frames painted.
  bool ShouldPaint() const;

  // Draws the line under the header for windows without a toolbar and not using
  // the packaged app header style.
  void PaintContentEdge(gfx::Canvas* canvas);

  // Returns the height for the header (non-client frame area).
  int GetHeaderHeight() const;

  // TODO(sky): Figure out how to support WebAppLeftHeaderView.

  // For popups, the window icon.
  TabIconView* window_icon_;

  TabStrip* tab_strip_;

  DISALLOW_COPY_AND_ASSIGN(BrowserNonClientFrameViewMus);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_NON_CLIENT_FRAME_VIEW_MUS_H_
