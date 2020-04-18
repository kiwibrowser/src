// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_DEVICE_CHOOSER_CONTENT_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_DEVICE_CHOOSER_CONTENT_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#include "ui/base/models/table_model.h"
#include "ui/gfx/range/range.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label_listener.h"
#include "ui/views/view.h"

namespace views {
class LabelButton;
class StyledLabel;
class TableView;
class TableViewObserver;
class Throbber;
}

// A bubble or dialog view for choosing among several options in a table.
// Used for WebUSB/WebBluetooth device selection for Chrome and extensions.
class DeviceChooserContentView : public views::View,
                                 public ui::TableModel,
                                 public ChooserController::View,
                                 public views::StyledLabelListener,
                                 public views::ButtonListener {
 public:
  DeviceChooserContentView(
      views::TableViewObserver* table_view_observer,
      std::unique_ptr<ChooserController> chooser_controller);
  ~DeviceChooserContentView() override;

  // views::View:
  gfx::Size GetMinimumSize() const override;
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

  // ui::TableModel:
  int RowCount() override;
  base::string16 GetText(int row, int column_id) override;
  void SetObserver(ui::TableModelObserver* observer) override;
  gfx::ImageSkia GetIcon(int row) override;

  // ChooserController::View:
  void OnOptionsInitialized() override;
  void OnOptionAdded(size_t index) override;
  void OnOptionRemoved(size_t index) override;
  void OnOptionUpdated(size_t index) override;
  void OnAdapterEnabledChanged(bool enabled) override;
  void OnRefreshStateChanged(bool refreshing) override;

  // views::StyledLabelListener:
  void StyledLabelLinkClicked(views::StyledLabel* label,
                              const gfx::Range& range,
                              int event_flags) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  base::string16 GetWindowTitle() const;
  std::unique_ptr<views::View> CreateExtraView();
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const;
  bool IsDialogButtonEnabled(ui::DialogButton button) const;
  void Accept();
  void Cancel();
  void Close();
  void UpdateTableView();

 private:
  friend class ChooserDialogViewTest;
  friend class DeviceChooserContentViewTest;
  FRIEND_TEST_ALL_PREFIXES(DeviceChooserContentViewTest, ClickRescanLink);
  FRIEND_TEST_ALL_PREFIXES(DeviceChooserContentViewTest, ClickGetHelpLink);

  class BluetoothStatusContainer : public views::View {
   public:
    explicit BluetoothStatusContainer(views::ButtonListener* listener);

    // view::Views:
    gfx::Size CalculatePreferredSize() const override;
    void Layout() override;

    void ShowScanningLabelAndThrobber();
    void ShowReScanButton(bool enabled);

    views::LabelButton* re_scan_button() { return re_scan_button_; }
    views::Throbber* throbber() { return throbber_; }
    views::Label* scanning_label() { return scanning_label_; }

   private:
    int GetThrobberLabelSpacing() const;
    void CenterVertically(views::View* view);

    views::LabelButton* re_scan_button_;
    views::Throbber* throbber_;
    views::Label* scanning_label_;

    DISALLOW_COPY_AND_ASSIGN(BluetoothStatusContainer);
  };

  std::unique_ptr<ChooserController> chooser_controller_;

  // True when the devices are refreshing (i.e. when OnRefreshStateChanged(true)
  // is called).
  bool refreshing_ = false;

  views::TableView* table_view_ = nullptr;
  views::View* table_parent_ = nullptr;
  views::StyledLabel* adapter_off_help_ = nullptr;
  BluetoothStatusContainer* bluetooth_status_container_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(DeviceChooserContentView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_DEVICE_CHOOSER_CONTENT_VIEW_H_
