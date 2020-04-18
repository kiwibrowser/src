// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_NATIVE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_NATIVE_VIEWS_H_

#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/browser/ui/views/autofill/autofill_popup_base_view.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/path.h"
#include "ui/views/bubble/bubble_border.h"

#include <memory>
#include <vector>

namespace autofill {

class AutofillPopupController;

// Child view representing one row in the Autofill Popup. This could represent
// a UI control (e.g., a suggestion which can be autofilled), or decoration like
// separators.
class AutofillPopupRowView : public views::View {
 public:
  ~AutofillPopupRowView() override = default;
  void SetSelected(bool is_selected);

  // views::View:
  // Drags and presses on any row should be a no-op; subclasses instead rely on
  // entry/release events. Returns true to indicate that those events have been
  // processed (i.e., intentionally ignored).
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

 protected:
  AutofillPopupRowView(AutofillPopupController* controller, int line_number);

  // Init handles initialization tasks which require virtual methods. Subclasses
  // should have private/protected constructors and implement a static Create
  // method which calls Init before returning.
  void Init();

  virtual void CreateContent() = 0;
  virtual void RefreshStyle() = 0;
  virtual std::unique_ptr<views::Background> CreateBackground() = 0;

  AutofillPopupController* controller_;
  const int line_number_;
  bool is_warning_ = false;  // overwritten in ctor
  bool is_selected_ = false;
};

// Views implementation for the autofill and password suggestion.
// TODO(https://crbug.com/768881): Once this implementation is complete, this
// class should be renamed to AutofillPopupViewViews and old
// AutofillPopupViewViews should be removed. The main difference of
// AutofillPopupViewNativeViews from AutofillPopupViewViews is that child views
// are drawn using toolkit-views framework, in contrast to
// AutofillPopupViewViews, where individuals rows are drawn directly on canvas.
class AutofillPopupViewNativeViews : public AutofillPopupBaseView,
                                     public AutofillPopupView {
 public:
  AutofillPopupViewNativeViews(AutofillPopupController* controller,
                               views::Widget* parent_widget);
  ~AutofillPopupViewNativeViews() override;

  const std::vector<AutofillPopupRowView*>& GetRowsForTesting() {
    return rows_;
  }

  // AutofillPopupView:
  void Show() override;
  void Hide() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;

  // AutofillPopupBaseView:
  // TODO(crbug.com/831603): Remove these overrides and the corresponding
  // methods in AutofillPopupBaseView once deprecation of
  // AutofillPopupViewViews is complete.
  void OnMouseMoved(const ui::MouseEvent& event) override {}

 private:
  // views::View:
  void VisibilityChanged(View* starting_from, bool is_visible) override;

  void OnSelectedRowChanged(base::Optional<int> previous_row_selection,
                            base::Optional<int> current_row_selection) override;
  void OnSuggestionsChanged() override;

  // Creates child views based on the suggestions given by |controller_|.
  void CreateChildViews();

  // AutofillPopupBaseView:
  void AddExtraInitParams(views::Widget::InitParams* params) override;
  std::unique_ptr<views::View> CreateWrapperView() override;
  std::unique_ptr<views::Border> CreateBorder() override;
  void DoUpdateBoundsAndRedrawPopup() override;

  // Controller for this view.
  AutofillPopupController* controller_;

  std::vector<AutofillPopupRowView*> rows_;

  views::BubbleBorder* bubble_border_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupViewNativeViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_NATIVE_VIEWS_H_
