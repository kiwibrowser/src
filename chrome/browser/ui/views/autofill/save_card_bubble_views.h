// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEWS_H_

#include "base/macros.h"
#include "chrome/browser/ui/autofill/save_card_bubble_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "components/autofill/core/browser/ui/save_card_bubble_controller.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/controls/styled_label_listener.h"

namespace content {
class WebContents;
}

namespace views {
class Link;
class StyledLabel;
}

namespace autofill {

// This class displays the "Save credit card?" bubble that is shown when the
// user submits a form with a credit card number that Autofill has not
// previously saved.
class SaveCardBubbleViews : public SaveCardBubbleView,
                            public LocationBarBubbleDelegateView,
                            public views::LinkListener,
                            public views::StyledLabelListener {
 public:
  // Bubble will be anchored to |anchor_view|.
  SaveCardBubbleViews(views::View* anchor_view,
                      const gfx::Point& anchor_point,
                      content::WebContents* web_contents,
                      SaveCardBubbleController* controller);

  void Show(DisplayReason reason);

  // SaveCardBubbleView:
  void Hide() override;

  // views::BubbleDialogDelegateView:
  views::View* CreateExtraView() override;
  views::View* CreateFootnoteView() override;
  bool Accept() override;
  bool Cancel() override;
  bool Close() override;
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;

  // views::WidgetDelegate:
  bool ShouldShowCloseButton() const override;
  base::string16 GetWindowTitle() const override;
  gfx::ImageSkia GetWindowIcon() override;
  bool ShouldShowWindowIcon() const override;
  void WindowClosing() override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  // views::StyledLabelListener:
  void StyledLabelLinkClicked(views::StyledLabel* label,
                              const gfx::Range& range,
                              int event_flags) override;

  // Returns the footnote view, so it can be searched for clickable views.
  // Exists for testing (specifically, browsertests).
  views::View* GetFootnoteViewForTesting();

 private:
  FRIEND_TEST_ALL_PREFIXES(
      SaveCardBubbleViewsFullFormBrowserTest,
      Upload_ClickingCloseClosesBubbleIfSecondaryUiMdExpOn);
  FRIEND_TEST_ALL_PREFIXES(
      SaveCardBubbleViewsFullFormBrowserTest,
      Upload_DecliningUploadDoesNotLogUserAcceptedCardOriginUMA);

  // The current step of the save card flow.  Accounts for:
  //  1) Local save vs. Upload save
  //  2) Upload save can have all information or be missing CVC
  enum CurrentFlowStep {
    UNKNOWN_STEP,
    LOCAL_SAVE_ONLY_STEP,
    UPLOAD_SAVE_ONLY_STEP,
    UPLOAD_SAVE_CVC_FIX_FLOW_STEP_1_OFFER_UPLOAD,
    UPLOAD_SAVE_CVC_FIX_FLOW_STEP_2_REQUEST_CVC,
  };

  ~SaveCardBubbleViews() override;

  CurrentFlowStep GetCurrentFlowStep() const;
  // Create the dialog's content view containing everything except for the
  // footnote.
  std::unique_ptr<views::View> CreateMainContentView();

  // Attributes IDs to the DialogClientView and its buttons.
  void AssignIdsToDialogClientView();

  // views::BubbleDialogDelegateView:
  void Init() override;

  SaveCardBubbleController* controller_;  // Weak reference.

  views::View* footnote_view_ = nullptr;
  views::Link* learn_more_link_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEWS_H_
