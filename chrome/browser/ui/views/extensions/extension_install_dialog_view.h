// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_INSTALL_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_INSTALL_DIALOG_VIEW_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"

class Profile;

namespace content {
class PageNavigator;
}

namespace views {
class Link;
}

// Implements the extension installation dialog for TOOLKIT_VIEWS.
class ExtensionInstallDialogView : public views::DialogDelegateView,
                                   public views::LinkListener {
 public:
  // The views::View::id of the ratings section in the dialog.
  static const int kRatingsViewId = 1;

  ExtensionInstallDialogView(
      Profile* profile,
      content::PageNavigator* navigator,
      const ExtensionInstallPrompt::DoneCallback& done_callback,
      std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt);
  ~ExtensionInstallDialogView() override;

  // Returns the interior ScrollView of the dialog. This allows us to inspect
  // the contents of the DialogView.
  const views::ScrollView* scroll_view() const { return scroll_view_; }

  static void SetInstallButtonDelayForTesting(int timeout_in_ms);

  // Changes the widget size to accommodate the contents' preferred size.
  void ResizeWidget();

 private:
  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize() const override;
  void AddedToWidget() override;
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  int GetDefaultDialogButton() const override;
  bool Cancel() override;
  bool Accept() override;
  ui::ModalType GetModalType() const override;
  views::View* CreateExtraView() override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  // Creates the contents area that contains permissions and other extension
  // info.
  void CreateContents();

  // Enables the install button and updates the dialog buttons.
  void EnableInstallButton();

  bool is_external_install() const {
    return prompt_->type() == ExtensionInstallPrompt::EXTERNAL_INSTALL_PROMPT;
  }

  // Updates the histogram that holds installation accepted/aborted data.
  void UpdateInstallResultHistogram(bool accepted) const;

  Profile* profile_;
  content::PageNavigator* navigator_;
  ExtensionInstallPrompt::DoneCallback done_callback_;
  std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt_;

  // The scroll view containing all the details for the dialog (including all
  // collapsible/expandable sections).
  views::ScrollView* scroll_view_;

  // Set to true once the user's selection has been received and the callback
  // has been run.
  bool handled_result_;

  // Used to delay the activation of the install button.
  base::OneShotTimer timer_;

  // Used to determine whether the install button should be enabled.
  bool install_button_enabled_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallDialogView);
};

// A view that displays a list of details, along with a link that expands and
// collapses those details.
class ExpandableContainerView : public views::View, public views::LinkListener {
 public:
  ExpandableContainerView(const std::vector<base::string16>& details,
                          int available_width);
  ~ExpandableContainerView() override;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

 private:
  // Helper class representing the list of details, that can hide itself.
  class DetailsView : public views::View {
   public:
    explicit DetailsView(const std::vector<base::string16>& details);
    ~DetailsView() override {}

    // views::View:
    gfx::Size CalculatePreferredSize() const override;

    // Expands or collapses this view.
    void ToggleExpanded();

    bool expanded() { return expanded_; }

   private:
    // Whether this details section is expanded.
    bool expanded_ = false;

    DISALLOW_COPY_AND_ASSIGN(DetailsView);
  };

  // Expands or collapses |details_view_|.
  void ToggleDetailLevel();

  // The view that expands or collapses when |details_link_| is clicked.
  DetailsView* details_view_;

  // The 'Show Details' link, which changes to 'Hide Details' when the details
  // section is expanded.
  views::Link* details_link_;

  DISALLOW_COPY_AND_ASSIGN(ExpandableContainerView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSION_INSTALL_DIALOG_VIEW_H_
