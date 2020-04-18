// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_CAST_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_CAST_DIALOG_VIEW_H_

#include "chrome/browser/ui/media_router/cast_dialog_controller.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"

namespace media_router {

class CastDialogSinkButton;
struct UIMediaSink;

// View component of the Cast dialog that allows users to start and stop Casting
// to devices. The list of devices used to populate the dialog is supplied by
// CastDialogModel.
class CastDialogView : public views::BubbleDialogDelegateView,
                       public views::ButtonListener,
                       public CastDialogController::Observer {
 public:
  // Instantiates and shows the singleton dialog. The dialog must not be
  // currently shown.
  static void ShowDialog(views::View* anchor_view,
                         CastDialogController* controller);

  // No-op if the dialog is currently not shown.
  static void HideDialog();

  static bool IsShowing();

  // Returns nullptr if the dialog is currently not shown.
  static views::Widget* GetCurrentDialogWidget();

  // views::WidgetDelegateView:
  bool ShouldShowCloseButton() const override;

  // views::WidgetDelegate:
  base::string16 GetWindowTitle() const override;

  // ui::DialogModel:
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  int GetDialogButtons() const override;

  // views::DialogDelegate:
  bool Accept() override;
  bool Close() override;

  // CastDialogController::Observer:
  void OnModelUpdated(const CastDialogModel& model) override;
  void OnControllerInvalidated() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Called by tests.
  const std::vector<CastDialogSinkButton*>& sink_buttons_for_test() const {
    return sink_buttons_;
  }

 private:
  CastDialogView(views::View* anchor_view, CastDialogController* controller);
  ~CastDialogView() override;

  // views::BubbleDialogDelegateView:
  void Init() override;
  void WindowClosing() override;

  // Apply the stored sink selection and scroll state.
  void RestoreSinkListState();

  // Populate the scroll view containing sinks using the data in |model|.
  void PopulateScrollView(const CastDialogModel& model);

  views::View* CreateSinkListView(const std::vector<UIMediaSink>& sinks);

  void SelectSinkAtIndex(size_t index);

  // The singleton dialog instance. This is a nullptr when a dialog is not
  // shown.
  static CastDialogView* instance_;

  // Title shown at the top of the dialog.
  base::string16 dialog_title_;

  // The index of the selected item on the sink list.
  size_t selected_sink_index_ = 0;

  // Contains references to sink buttons in the order they appear.
  std::vector<CastDialogSinkButton*> sink_buttons_;

  CastDialogController* controller_;

  // ScrollView containing the list of sink buttons.
  views::ScrollView* scroll_view_ = nullptr;

  // How much |scroll_view_| is scrolled downwards in pixels. Whenever the sink
  // list is updated the scroll position gets reset, so we must manually restore
  // it to this value.
  int scroll_position_ = 0;

  DISALLOW_COPY_AND_ASSIGN(CastDialogView);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_CAST_DIALOG_VIEW_H_
