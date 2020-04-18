// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_VIEWS_H_

#include "base/macros.h"
#include "chrome/browser/ui/autofill/password_generation_popup_view.h"
#include "chrome/browser/ui/views/autofill/autofill_popup_base_view.h"
#include "ui/views/controls/styled_label_listener.h"

namespace views {
class StyledLabel;
}

namespace autofill {

class PasswordGenerationPopupController;

class PasswordGenerationPopupViewViews : public AutofillPopupBaseView,
                                         public PasswordGenerationPopupView,
                                         public views::StyledLabelListener {
 public:
  PasswordGenerationPopupViewViews(
      PasswordGenerationPopupController* controller,
      views::Widget* parent_widget);

  // PasswordGenerationPopupView implementation
  void Show() override;
  void Hide() override;
  gfx::Size GetPreferredSizeOfPasswordView() override;
  void UpdateBoundsAndRedrawPopup() override;
  void PasswordSelectionUpdated() override;
  bool IsPointInPasswordBounds(const gfx::Point& point) override;

 private:
  // Helper class to do layout of the password portion of the popup.
  class PasswordBox;

  ~PasswordGenerationPopupViewViews() override;

  // Helper function to create |password_view_|.
  void CreatePasswordView();

  // views:Views implementation.
  void Layout() override;
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::StyledLabelListener implementation
  void StyledLabelLinkClicked(views::StyledLabel* label,
                              const gfx::Range& range,
                              int event_flags) override;

  // Sub views. Used to change bounds when updating. Weak references.
  PasswordBox* password_view_;
  views::StyledLabel* help_label_;

  // Size of the divider between the password and the help text.
  gfx::Rect divider_bounds_;

  // Controller for this view. Weak reference.
  PasswordGenerationPopupController* controller_;

  DISALLOW_COPY_AND_ASSIGN(PasswordGenerationPopupViewViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_VIEWS_H_
