// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/layout/layout_manager.h"

class BookmarkBarView;
class Browser;
class BrowserViewLayoutDelegate;
class ContentsLayoutManager;
class ImmersiveModeController;
class InfoBarContainerView;
class TabStrip;

namespace gfx {
class Point;
class Size;
}

namespace views {
class ClientView;
}

namespace web_modal {
class WebContentsModalDialogHost;
}

// The layout manager used in chrome browser.
class BrowserViewLayout : public views::LayoutManager {
 public:
  BrowserViewLayout();
  ~BrowserViewLayout() override;

  // Sets all the views to be managed. Takes ownership of |delegate|.
  // |browser_view| may be null in tests.
  void Init(BrowserViewLayoutDelegate* delegate,
            Browser* browser,
            views::ClientView* browser_view,
            views::View* top_container,
            TabStrip* tab_strip,
            views::View* toolbar,
            InfoBarContainerView* infobar_container,
            views::View* contents_container,
            ContentsLayoutManager* contents_layout_manager,
            ImmersiveModeController* immersive_mode_controller);

  // Sets or updates views that are not available when |this| is initialized.
  void set_tab_strip(TabStrip* tab_strip) {
    tab_strip_ = tab_strip;
  }
  void set_bookmark_bar(BookmarkBarView* bookmark_bar) {
    bookmark_bar_ = bookmark_bar;
  }
  void set_download_shelf(views::View* download_shelf) {
    download_shelf_ = download_shelf;
  }

  web_modal::WebContentsModalDialogHost* GetWebContentsModalDialogHost();

  // Returns the minimum size of the browser view.
  gfx::Size GetMinimumSize();

  // Returns the bounding box, in widget coordinates,  for the find bar.
  gfx::Rect GetFindBarBoundingBox() const;

  // Tests to see if the specified |point| (in nonclient view's coordinates)
  // is within the views managed by the laymanager. Returns one of
  // HitTestCompat enum defined in ui/base/hit_test.h.
  // See also ClientView::NonClientHitTest.
  int NonClientHitTest(const gfx::Point& point);

  // views::LayoutManager overrides:
  void Layout(views::View* host) override;
  gfx::Size GetPreferredSize(const views::View* host) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BrowserViewLayoutTest, BrowserViewLayout);
  FRIEND_TEST_ALL_PREFIXES(BrowserViewLayoutTest, Layout);
  FRIEND_TEST_ALL_PREFIXES(BrowserViewLayoutTest, LayoutDownloadShelf);
  class WebContentsModalDialogHostViews;

  Browser* browser() { return browser_; }

  // Layout the following controls, starting at |top|, returns the coordinate
  // of the bottom of the control, for laying out the next control.
  int LayoutTabStripRegion(int top);
  int LayoutToolbar(int top);
  int LayoutBookmarkAndInfoBars(int top, int browser_view_y);
  int LayoutBookmarkBar(int top);
  int LayoutInfoBar(int top);

  // Layout the |contents_container_| view between the coordinates |top| and
  // |bottom|. See browser_view.h for details of the relationship between
  // |contents_container_| and other views.
  void LayoutContentsContainerView(int top, int bottom);

  // Updates |top_container_|'s bounds. The new bounds depend on the size of
  // the bookmark bar and the toolbar.
  void UpdateTopContainerBounds();

  // Returns the vertical offset for the web contents to account for a
  // detached bookmarks bar.
  int GetContentsOffsetForBookmarkBar();

  // Returns the top margin to adjust the contents_container_ by. This is used
  // to make the bookmark bar and contents_container_ overlap so that the
  // preview contents hides the bookmark bar.
  int GetTopMarginForActiveContent();

  // Layout the Download Shelf, returns the coordinate of the top of the
  // control, for laying out the previous control.
  int LayoutDownloadShelf(int bottom);

  // Returns true if an infobar is showing.
  bool InfobarVisible() const;

  // The delegate interface. May be a mock in tests.
  std::unique_ptr<BrowserViewLayoutDelegate> delegate_;

  // The browser from the owning BrowserView.
  Browser* browser_;

  // The owning browser view.
  views::ClientView* browser_view_;

  // Child views that the layout manager manages.
  // NOTE: If you add a view, try to add it as a views::View, which makes
  // testing much easier.
  views::View* top_container_;
  TabStrip* tab_strip_;
  views::View* toolbar_;
  BookmarkBarView* bookmark_bar_;
  InfoBarContainerView* infobar_container_;
  views::View* contents_container_;
  ContentsLayoutManager* contents_layout_manager_;
  views::View* download_shelf_;

  ImmersiveModeController* immersive_mode_controller_;

  // The bounds within which the vertically-stacked contents of the BrowserView
  // should be laid out within. This is just the local bounds of the
  // BrowserView.
  // TODO(jamescook): Remove this and just use browser_view_->GetLocalBounds().
  gfx::Rect vertical_layout_rect_;

  // The host for use in positioning the web contents modal dialog.
  std::unique_ptr<WebContentsModalDialogHostViews> dialog_host_;

  // The latest dialog bounds applied during a layout pass.
  gfx::Rect latest_dialog_bounds_;

  // The distance the web contents modal dialog is from the top of the window,
  // in pixels.
  int web_contents_modal_dialog_top_y_;

  DISALLOW_COPY_AND_ASSIGN(BrowserViewLayout);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_H_
