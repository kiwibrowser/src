// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_WEBSHARE_WEBSHARE_TARGET_PICKER_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_WEBSHARE_WEBSHARE_TARGET_PICKER_VIEW_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/webshare/webshare_target.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/controls/table/table_view_observer.h"
#include "ui/views/window/dialog_delegate.h"

class TargetPickerTableModel;
class WebShareTargetPickerViewTest;

namespace views {
class TableView;
}

// Dialog that presents the user with a list of share target apps. Allows the
// user to pick one target to be opened and have data passed to it.
//
// NOTE: This dialog has *not* been UI-reviewed, and is being used by an
// in-development feature (Web Share) behind a runtime flag. It should not be
// used by any released code until going through UI review.
class WebShareTargetPickerView : public views::DialogDelegateView,
                                 public views::TableViewObserver {
 public:
  // |targets| is a list of app title and manifest URL pairs that will be shown
  // in a list. If the user picks a target, this calls |callback| with the
  // manifest URL of the chosen target, or returns null if the user cancelled
  // the share.
  WebShareTargetPickerView(std::vector<WebShareTarget> targets,
                           chrome::WebShareTargetPickerCallback close_callback);
  ~WebShareTargetPickerView() override;

  // views::View overrides:
  gfx::Size CalculatePreferredSize() const override;

  // views::WidgetDelegate overrides:
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;

  // views::DialogDelegate overrides:
  bool Cancel() override;
  bool Accept() override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;

  // views::TableViewObserver overrides:
  void OnSelectionChanged() override;
  void OnDoubleClick() override;

 private:
  // For access to |table_|.
  friend class WebShareTargetPickerViewTest;

  views::TableView* table_ = nullptr;

  const std::vector<WebShareTarget> targets_;
  std::unique_ptr<TargetPickerTableModel> table_model_;

  chrome::WebShareTargetPickerCallback close_callback_;

  DISALLOW_COPY_AND_ASSIGN(WebShareTargetPickerView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_WEBSHARE_WEBSHARE_TARGET_PICKER_VIEW_H_
