// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_DELEGATE_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_DELEGATE_H_

class ExclusiveAccessBubbleViews;

namespace gfx {
class Rect;
}
namespace views {
class View;
}

// Delegate class to allow BrowserViewLayout to be decoupled from BrowserView
// for testing.
class BrowserViewLayoutDelegate {
 public:
  virtual ~BrowserViewLayoutDelegate() {}

  virtual views::View* GetContentsWebView() const = 0;
  virtual bool IsTabStripVisible() const = 0;
  virtual gfx::Rect GetBoundsForTabStripInBrowserView() const = 0;
  virtual int GetTopInsetInBrowserView(bool restored) const = 0;
  virtual int GetThemeBackgroundXInset() const = 0;
  virtual bool IsToolbarVisible() const = 0;
  virtual bool IsBookmarkBarVisible() const = 0;
  virtual bool DownloadShelfNeedsLayout() const = 0;
  virtual ExclusiveAccessBubbleViews* GetExclusiveAccessBubble() const = 0;
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_LAYOUT_DELEGATE_H_
