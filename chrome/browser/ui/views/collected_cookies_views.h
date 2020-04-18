// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_COLLECTED_COOKIES_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_COLLECTED_COOKIES_VIEWS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane_listener.h"
#include "ui/views/controls/tree/tree_view_controller.h"
#include "ui/views/window/dialog_delegate.h"

class CookieInfoView;
class CookiesTreeModel;
class CookiesTreeViewDrawingProvider;
class InfobarView;

namespace content {
class WebContents;
}

namespace views {
class Label;
class LabelButton;
class TreeView;
}

// This is the Views implementation of the collected cookies dialog.
//
// CollectedCookiesViews is a dialog that displays the allowed and blocked
// cookies of the current tab contents. To display the dialog, invoke
// ShowCollectedCookiesDialog() on the delegate of the WebContents's
// content settings tab helper.
class CollectedCookiesViews : public views::DialogDelegateView,
                              public content::NotificationObserver,
                              public views::ButtonListener,
                              public views::TabbedPaneListener,
                              public views::TreeViewController {
 public:
  // Use BrowserWindow::ShowCollectedCookiesDialog to show.
  explicit CollectedCookiesViews(content::WebContents* web_contents);

  // views::DialogDelegate:
  base::string16 GetWindowTitle() const override;
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool Accept() override;
  ui::ModalType GetModalType() const override;
  bool ShouldShowCloseButton() const override;
  views::View* CreateExtraView() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::TabbedPaneListener:
  void TabSelectedAt(int index) override;

  // views::TreeViewController:
  void OnTreeViewSelectionChanged(views::TreeView* tree_view) override;

  // views::View:
  gfx::Size GetMinimumSize() const override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;

 private:
  friend class CollectedCookiesViewsTest;

  ~CollectedCookiesViews() override;

  void Init();

  views::View* CreateAllowedPane();

  views::View* CreateBlockedPane();

  // Creates and returns the "buttons pane", which is the view in the
  // bottom-leading edge of this dialog containing the action buttons for the
  // currently selected host or cookie.
  std::unique_ptr<views::View> CreateButtonsPane();

  // Creates and returns a containing ScrollView around the given tree view.
  views::View* CreateScrollView(views::TreeView* pane);

  void EnableControls();

  void ShowCookieInfo();

  void AddContentException(views::TreeView* tree_view, ContentSetting setting);

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;

  // The web contents.
  content::WebContents* web_contents_;

  // Assorted views.
  views::Label* allowed_label_;
  views::Label* blocked_label_;

  views::TreeView* allowed_cookies_tree_;
  views::TreeView* blocked_cookies_tree_;

  views::LabelButton* block_allowed_button_;
  views::LabelButton* delete_allowed_button_;
  views::LabelButton* allow_blocked_button_;
  views::LabelButton* for_session_blocked_button_;

  std::unique_ptr<CookiesTreeModel> allowed_cookies_tree_model_;
  std::unique_ptr<CookiesTreeModel> blocked_cookies_tree_model_;

  CookiesTreeViewDrawingProvider* allowed_cookies_drawing_provider_;
  CookiesTreeViewDrawingProvider* blocked_cookies_drawing_provider_;

  CookieInfoView* cookie_info_view_;

  InfobarView* infobar_;

  // The buttons pane is owned by this class until the containing
  // DialogClientView requests it via |CreateExtraView|, at which point
  // ownership is handed off and this pointer becomes null.
  std::unique_ptr<views::View> buttons_pane_;

  // Weak pointers to the allowed and blocked panes so that they can be
  // shown/hidden as needed.
  views::View* allowed_buttons_pane_;
  views::View* blocked_buttons_pane_;

  bool status_changed_;

  DISALLOW_COPY_AND_ASSIGN(CollectedCookiesViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_COLLECTED_COOKIES_VIEWS_H_
