// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/webshare/webshare_target_picker_view.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/table_model.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/table/table_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

int kDialogWidth = 500;
int kDialogHeight = 400;

}

// Supplies data to the table view.
class TargetPickerTableModel : public ui::TableModel {
 public:
  explicit TargetPickerTableModel(const std::vector<WebShareTarget>& targets);

 private:
  // ui::TableModel overrides:
  int RowCount() override;
  base::string16 GetText(int row, int column_id) override;
  void SetObserver(ui::TableModelObserver* observer) override;

  // Owned by WebShareTargetPickerView.
  const std::vector<WebShareTarget>& targets_;

  DISALLOW_COPY_AND_ASSIGN(TargetPickerTableModel);
};

TargetPickerTableModel::TargetPickerTableModel(
    const std::vector<WebShareTarget>& targets)
    : targets_(targets) {}

int TargetPickerTableModel::RowCount() {
  return targets_.size();
}

base::string16 TargetPickerTableModel::GetText(int row, int /*column_id*/) {
  // Show "title (origin)", to disambiguate titles that are the same, and as a
  // security measure.
  return l10n_util::GetStringFUTF16(
      IDS_WEBSHARE_TARGET_DIALOG_ITEM_TEXT,
      base::UTF8ToUTF16(targets_[row].name()),
      base::UTF8ToUTF16(targets_[row].manifest_url().GetOrigin().spec()));
}

void TargetPickerTableModel::SetObserver(ui::TableModelObserver* observer) {}

namespace chrome {

void ShowWebShareTargetPickerDialog(
    gfx::NativeWindow parent_window,
    std::vector<WebShareTarget> targets,
    chrome::WebShareTargetPickerCallback callback) {
  constrained_window::CreateBrowserModalDialogViews(
      new WebShareTargetPickerView(std::move(targets), std::move(callback)),
      parent_window)
      ->Show();
}

}  // namespace chrome

WebShareTargetPickerView::WebShareTargetPickerView(
    std::vector<WebShareTarget> targets,
    chrome::WebShareTargetPickerCallback close_callback)
    : targets_(std::move(targets)),
      table_model_(std::make_unique<TargetPickerTableModel>(targets_)),
      close_callback_(std::move(close_callback)) {
  const ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  views::BoxLayout* layout =
      SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::kVertical,
          provider->GetDialogInsetsForContentType(views::TEXT, views::CONTROL),
          provider->GetDistanceMetric(
              views::DISTANCE_RELATED_CONTROL_VERTICAL)));

  views::Label* overview_label = new views::Label(
      l10n_util::GetStringUTF16(IDS_WEBSHARE_TARGET_PICKER_LABEL));
  AddChildView(overview_label);

  std::vector<ui::TableColumn> table_columns{ui::TableColumn()};
  table_ = new views::TableView(table_model_.get(), table_columns,
                                views::TEXT_ONLY, true);
  // Select the first row.
  if (targets_.size() > 0)
    table_->Select(0);

  table_->set_observer(this);

  // Create the table parent (a ScrollView which includes the scroll bars and
  // border). We add this parent (not the table itself) to the dialog.
  views::View* table_parent = table_->CreateParentIfNecessary();
  AddChildView(table_parent);
  // Make the table expand to fill the space.
  layout->SetFlexForView(table_parent, 1);
  chrome::RecordDialogCreation(
      chrome::DialogIdentifier::WEB_SHARE_TARGET_PICKER);
}

WebShareTargetPickerView::~WebShareTargetPickerView() {
  // Clear the pointer from |table_| which currently points at |table_model_|.
  // Otherwise, |table_model_| will be deleted before |table_|, and |table_|'s
  // destructor will try to call a method on the model.
  table_->SetModel(nullptr);
}

gfx::Size WebShareTargetPickerView::CalculatePreferredSize() const {
  return gfx::Size(kDialogWidth, kDialogHeight);
}

ui::ModalType WebShareTargetPickerView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 WebShareTargetPickerView::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_WEBSHARE_TARGET_PICKER_TITLE);
}

bool WebShareTargetPickerView::Cancel() {
  if (!close_callback_.is_null())
    std::move(close_callback_).Run(nullptr);

  return true;
}

bool WebShareTargetPickerView::Accept() {
  if (!close_callback_.is_null()) {
    DCHECK(!table_->selection_model().empty());
    std::move(close_callback_).Run(&targets_[table_->FirstSelectedRow()]);
  }

  return true;
}

base::string16 WebShareTargetPickerView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return l10n_util::GetStringUTF16(IDS_WEBSHARE_TARGET_PICKER_COMMIT);

  return views::DialogDelegateView::GetDialogButtonLabel(button);
}

bool WebShareTargetPickerView::IsDialogButtonEnabled(
    ui::DialogButton button) const {
  // User shouldn't select OK button if they haven't selected a target.
  if (button == ui::DIALOG_BUTTON_OK)
    return !table_->selection_model().empty();

  return true;
}

void WebShareTargetPickerView::OnSelectionChanged() {
  DialogModelChanged();
}

void WebShareTargetPickerView::OnDoubleClick() {
  GetDialogClientView()->AcceptWindow();
}
