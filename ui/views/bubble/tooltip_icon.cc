// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/bubble/tooltip_icon.h"

#include "base/timer/timer.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/bubble/info_bubble.h"
#include "ui/views/mouse_watcher_view_host.h"

namespace views {

TooltipIcon::TooltipIcon(const base::string16& tooltip)
    : tooltip_(tooltip),
      mouse_inside_(false),
      bubble_(nullptr),
      preferred_width_(0),
      observer_(this) {
  SetDrawAsHovered(false);
}

TooltipIcon::~TooltipIcon() {
  HideBubble();
}

const char* TooltipIcon::GetClassName() const {
  return "TooltipIcon";
}

void TooltipIcon::OnMouseEntered(const ui::MouseEvent& event) {
  mouse_inside_ = true;
  show_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(150), this,
                    &TooltipIcon::ShowBubble);
}

void TooltipIcon::OnMouseExited(const ui::MouseEvent& event) {
  show_timer_.Stop();
}

bool TooltipIcon::OnMousePressed(const ui::MouseEvent& event) {
  // Swallow the click so that the parent doesn't process it.
  return true;
}

void TooltipIcon::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    ShowBubble();
    event->SetHandled();
  }
}

void TooltipIcon::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kTooltip;
  node_data->SetName(tooltip_);
}

void TooltipIcon::MouseMovedOutOfHost() {
  if (IsMouseHovered()) {
    mouse_watcher_->Start();
    return;
  }

  mouse_inside_ = false;
  HideBubble();
}

void TooltipIcon::SetDrawAsHovered(bool hovered) {
  SetImage(gfx::CreateVectorIcon(vector_icons::kInfoOutlineIcon, 18,
                                 hovered
                                     ? SkColorSetARGB(0xBD, 0, 0, 0)
                                     : SkColorSetARGB(0xBD, 0x44, 0x44, 0x44)));
}

void TooltipIcon::ShowBubble() {
  if (bubble_)
    return;

  SetDrawAsHovered(true);

  bubble_ = new InfoBubble(this, tooltip_);
  bubble_->set_preferred_width(preferred_width_);
  bubble_->set_arrow(BubbleBorder::TOP_RIGHT);
  // When shown due to a gesture event, close on deactivate (i.e. don't use
  // "focusless").
  bubble_->set_can_activate(!mouse_inside_);

  bubble_->Show();
  observer_.Add(bubble_->GetWidget());

  if (mouse_inside_) {
    View* frame = bubble_->GetWidget()->non_client_view()->frame_view();
    mouse_watcher_ = std::make_unique<MouseWatcher>(
        std::make_unique<MouseWatcherViewHost>(frame, gfx::Insets()), this);
    mouse_watcher_->Start();
  }
}

void TooltipIcon::HideBubble() {
  if (bubble_)
    bubble_->Hide();
}

void TooltipIcon::OnWidgetDestroyed(Widget* widget) {
  observer_.Remove(widget);

  SetDrawAsHovered(false);
  mouse_watcher_.reset();
  bubble_ = nullptr;
}

}  // namespace views
