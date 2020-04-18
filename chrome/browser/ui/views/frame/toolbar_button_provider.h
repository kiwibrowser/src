// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_TOOLBAR_BUTTON_PROVIDER_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_TOOLBAR_BUTTON_PROVIDER_H_

class AppMenuButton;
class BrowserActionsContainer;
class PageActionIconContainerView;

namespace views {
class AccessiblePaneView;
}

// An interface implemented by a view contains and provides access to toolbar
// buttons in a BrowserView.
class ToolbarButtonProvider {
 public:
  // Gets the browser actions container.
  virtual BrowserActionsContainer* GetBrowserActionsContainer() = 0;

  // Gets the page action icon container.
  virtual PageActionIconContainerView* GetPageActionIconContainerView() = 0;

  // Gets the app menu button.
  virtual AppMenuButton* GetAppMenuButton() = 0;

  // Gives the toolbar focus.
  virtual void FocusToolbar() = 0;

  // Returns the toolbar as an AccessiblePaneView.
  virtual views::AccessiblePaneView* GetAsAccessiblePaneView() = 0;

  // TODO(calamity): Move other buttons and button actions into here.
 protected:
  virtual ~ToolbarButtonProvider() {}
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_TOOLBAR_BUTTON_PROVIDER_H_
