// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_VIEWS_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/browser/ui/views/autofill/autofill_popup_base_view.h"

namespace autofill {

class AutofillPopupController;

namespace {
class AutofillPopupChildView;
}

// Views toolkit implementation for AutofillPopupView.
class AutofillPopupViewViews : public AutofillPopupBaseView,
                               public AutofillPopupView {
 public:
  // |controller| should not be null.
  AutofillPopupViewViews(AutofillPopupController* controller,
                         views::Widget* parent_widget);
  ~AutofillPopupViewViews() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(AutofillPopupViewViewsTest, OnSelectedRowChanged);

  // AutofillPopupView implementation.
  void Show() override;
  void Hide() override;
  void OnSelectedRowChanged(base::Optional<int> previous_row_selection,
                            base::Optional<int> current_row_selection) override;
  void OnSuggestionsChanged() override;

  // views::Views implementation
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // Draw the given autofill entry in |entry_rect|.
  void DrawAutofillEntry(gfx::Canvas* canvas,
                         int index,
                         const gfx::Rect& entry_rect);

  // Creates child views based on the suggestions given by |controller_|. These
  // child views are used for accessibility events only. We need child views to
  // populate the correct |AXNodeData| when user selects a suggestion.
  void CreateChildViews();

  AutofillPopupChildView* GetChildRow(size_t child_index) const;

  AutofillPopupController* controller_;  // Weak reference.

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupViewViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_VIEWS_H_
