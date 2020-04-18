// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/cast_dialog_view.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/media_router/cast_dialog_controller.h"
#include "chrome/browser/ui/media_router/cast_dialog_model.h"
#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "chrome/browser/ui/media_router/ui_media_sink.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/media_router/cast_dialog_sink_button.h"
#include "chrome/common/media_router/media_sink.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_client_view.h"

namespace media_router {

// static
void CastDialogView::ShowDialog(views::View* anchor_view,
                                CastDialogController* controller) {
  DCHECK(!instance_);
  instance_ = new CastDialogView(anchor_view, controller);
  views::Widget* widget =
      views::BubbleDialogDelegateView::CreateBubble(instance_);
  widget->Show();
}

// static
void CastDialogView::HideDialog() {
  if (IsShowing())
    instance_->GetWidget()->Close();
  // We also set |instance_| to nullptr in WindowClosing() which is called
  // asynchronously, because not all paths to close the dialog go through
  // HideDialog(). We set it here because IsShowing() should be false after
  // HideDialog() is called.
  instance_ = nullptr;
}

// static
bool CastDialogView::IsShowing() {
  return instance_ != nullptr;
}

// static
views::Widget* CastDialogView::GetCurrentDialogWidget() {
  return instance_ ? instance_->GetWidget() : nullptr;
}

bool CastDialogView::ShouldShowCloseButton() const {
  return true;
}

base::string16 CastDialogView::GetWindowTitle() const {
  return dialog_title_;
}

base::string16 CastDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return sink_buttons_.empty()
             ? base::string16()
             : sink_buttons_.at(selected_sink_index_)->GetActionText();
}

int CastDialogView::GetDialogButtons() const {
  return sink_buttons_.empty() ? ui::DIALOG_BUTTON_NONE : ui::DIALOG_BUTTON_OK;
}

bool CastDialogView::Accept() {
  scroll_position_ = scroll_view_->GetVisibleRect().y();
  const UIMediaSink& sink = sink_buttons_.at(selected_sink_index_)->sink();
  if (sink.allowed_actions & static_cast<int>(UICastAction::CAST_TAB)) {
    controller_->StartCasting(sink.id, TAB_MIRROR);
  } else if (sink.allowed_actions & static_cast<int>(UICastAction::STOP)) {
    controller_->StopCasting(sink.route_id);
  }
  return false;
}

bool CastDialogView::Close() {
  return Cancel();
}

void CastDialogView::OnModelUpdated(const CastDialogModel& model) {
  PopulateScrollView(model);
  RestoreSinkListState();
}

void CastDialogView::OnControllerInvalidated() {
  controller_ = nullptr;
  // The widget may be null if this is called while the dialog is opening.
  if (GetWidget())
    GetWidget()->Close();
}

CastDialogView::CastDialogView(views::View* anchor_view,
                               CastDialogController* controller)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT),
      controller_(controller) {}

CastDialogView::~CastDialogView() {
  if (controller_)
    controller_->RemoveObserver(this);
}

void CastDialogView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  SelectSinkAtIndex(sender->tag());
}

void CastDialogView::Init() {
  auto* provider = ChromeLayoutProvider::Get();
  set_margins(
      gfx::Insets(provider->GetDistanceMetric(
                      views::DISTANCE_DIALOG_CONTENT_MARGIN_TOP_CONTROL),
                  0,
                  provider->GetDistanceMetric(
                      views::DISTANCE_DIALOG_CONTENT_MARGIN_BOTTOM_CONTROL),
                  0));
  SetLayoutManager(std::make_unique<views::FillLayout>());
  controller_->AddObserver(this);
}

void CastDialogView::WindowClosing() {
  if (instance_ == this)
    instance_ = nullptr;
}

void CastDialogView::RestoreSinkListState() {
  if (selected_sink_index_ < sink_buttons_.size()) {
    sink_buttons_.at(selected_sink_index_)->SnapInkDropToActivated();
    SelectSinkAtIndex(selected_sink_index_);
  } else if (sink_buttons_.size() > 0u) {
    sink_buttons_.at(0)->SnapInkDropToActivated();
    SelectSinkAtIndex(0);
  }

  views::ScrollBar* scroll_bar =
      const_cast<views::ScrollBar*>(scroll_view_->vertical_scroll_bar());
  if (scroll_bar)
    scroll_view_->ScrollToPosition(scroll_bar, scroll_position_);
}

void CastDialogView::PopulateScrollView(const CastDialogModel& model) {
  dialog_title_ = model.dialog_header;
  if (!scroll_view_) {
    scroll_view_ = new views::ScrollView();
    AddChildView(scroll_view_);
    constexpr int kSinkButtonHeight = 50;
    scroll_view_->ClipHeightTo(0, kSinkButtonHeight * 10);
  }
  scroll_view_->SetContents(CreateSinkListView(model.media_sinks));

  // The widget may be null if this is called while the dialog is opening.
  if (GetWidget())
    SizeToContents();
  Layout();
}

views::View* CastDialogView::CreateSinkListView(
    const std::vector<UIMediaSink>& sinks) {
  sink_buttons_.clear();
  views::View* view = new views::View();
  view->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  for (size_t i = 0; i < sinks.size(); i++) {
    const UIMediaSink& sink = sinks.at(i);
    CastDialogSinkButton* sink_button = new CastDialogSinkButton(this, sink);
    sink_button->set_tag(i);
    sink_buttons_.push_back(sink_button);
    view->AddChildView(sink_button);
  }
  return view;
}

void CastDialogView::SelectSinkAtIndex(size_t index) {
  if (selected_sink_index_ != index &&
      selected_sink_index_ < sink_buttons_.size()) {
    sink_buttons_.at(selected_sink_index_)->SetSelected(false);
  }
  CastDialogSinkButton* selected_button = sink_buttons_.at(index);
  selected_button->SetSelected(true);
  selected_sink_index_ = index;

  // Update the text on the main action button.
  DialogModelChanged();
}

// static
CastDialogView* CastDialogView::instance_ = nullptr;

}  // namespace media_router
