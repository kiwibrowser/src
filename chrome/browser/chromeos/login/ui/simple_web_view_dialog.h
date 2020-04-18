// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_SIMPLE_WEB_VIEW_DIALOG_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_SIMPLE_WEB_VIEW_DIALOG_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/command_updater_delegate.h"
#include "chrome/browser/ui/toolbar/chrome_toolbar_model_delegate.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/widget/widget_delegate.h"
#include "url/gurl.h"

class CommandUpdaterImpl;
class Profile;
class ReloadButton;
class ToolbarModel;

namespace views {
class WebView;
class Widget;
}  // namespace views

namespace chromeos {

class StubBubbleModelDelegate;

// View class which shows the light version of the toolbar and the web contents.
// Light version of the toolbar includes back, forward buttons and location
// bar. Location bar is shown in read only mode, because this view is designed
// to be used for sign in to captive portal on login screen (when Browser
// isn't running).
class SimpleWebViewDialog : public views::ButtonListener,
                            public views::WidgetDelegateView,
                            public LocationBarView::Delegate,
                            public ChromeToolbarModelDelegate,
                            public CommandUpdaterDelegate,
                            public content::PageNavigator,
                            public content::WebContentsDelegate {
 public:
  explicit SimpleWebViewDialog(Profile* profile);
  ~SimpleWebViewDialog() override;

  // Starts loading.
  void StartLoad(const GURL& gurl);

  // Inits view. Should be attached to a Widget before call.
  void Init();

  // Overridden from views::View:
  void Layout() override;

  // Overridden from views::WidgetDelegate:
  views::View* GetInitiallyFocusedView() override;

  // Implements views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Implements content::PageNavigator:
  content::WebContents* OpenURL(const content::OpenURLParams& params) override;

  // Implements content::WebContentsDelegate:
  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  void LoadingStateChanged(content::WebContents* source,
                           bool to_different_document) override;

  // Implements LocationBarView::Delegate:
  content::WebContents* GetWebContents() override;
  ToolbarModel* GetToolbarModel() override;
  const ToolbarModel* GetToolbarModel() const override;
  ContentSettingBubbleModelDelegate* GetContentSettingBubbleModelDelegate()
      override;

  // Implements ChromeToolbarModelDelegate:
  content::WebContents* GetActiveWebContents() const override;

  // Implements CommandUpdaterDelegate:
  void ExecuteCommandWithDisposition(int id, WindowOpenDisposition) override;

 private:
  friend class SimpleWebViewDialogTest;

  void LoadImages();
  void UpdateButtons();
  void UpdateReload(bool is_loading, bool force);

  Profile* profile_;
  std::unique_ptr<ToolbarModel> toolbar_model_;
  std::unique_ptr<CommandUpdaterImpl> command_updater_;

  // Controls
  views::ImageButton* back_ = nullptr;
  views::ImageButton* forward_ = nullptr;
  ReloadButton* reload_ = nullptr;
  LocationBarView* location_bar_ = nullptr;
  views::WebView* web_view_ = nullptr;

  // Contains |web_view_| while it isn't owned by the view.
  std::unique_ptr<views::WebView> web_view_container_;

  std::unique_ptr<StubBubbleModelDelegate> bubble_model_delegate_;

  DISALLOW_COPY_AND_ASSIGN(SimpleWebViewDialog);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_SIMPLE_WEB_VIEW_DIALOG_H_
