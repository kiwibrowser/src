// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/message_view.h"

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/features.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/views/notification_control_buttons_view.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/painter.h"
#include "ui/views/widget/widget.h"

namespace message_center {

namespace {

const SkColor kBorderColor = SkColorSetARGB(0x1F, 0x0, 0x0, 0x0);
const int kShadowCornerRadius = 0;
const int kShadowElevation = 2;

// The global flag of Sidebar enability.
bool sidebar_enabled = false;

// Creates a text for spoken feedback from the data contained in the
// notification.
base::string16 CreateAccessibleName(const Notification& notification) {
  if (!notification.accessible_name().empty())
    return notification.accessible_name();

  // Fall back to a text constructed from the notification.
  std::vector<base::string16> accessible_lines = {
      notification.title(), notification.message(),
      notification.context_message()};
  std::vector<NotificationItem> items = notification.items();
  for (size_t i = 0; i < items.size() && i < kNotificationMaximumItems; ++i) {
    accessible_lines.push_back(items[i].title + base::ASCIIToUTF16(" ") +
                               items[i].message);
  }
  return base::JoinString(accessible_lines, base::ASCIIToUTF16("\n"));
}

bool ShouldRoundMessageViewCorners() {
  return base::FeatureList::IsEnabled(message_center::kNewStyleNotifications);
}

}  // namespace

// static
const char MessageView::kViewClassName[] = "MessageView";

// static
void MessageView::SetSidebarEnabled() {
  sidebar_enabled = true;
}

MessageView::MessageView(const Notification& notification)
    : notification_id_(notification.id()), slide_out_controller_(this, this) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Paint to a dedicated layer to make the layer non-opaque.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Create the opaque background that's above the view's shadow.
  background_view_ = new views::View();
  background_view_->SetBackground(
      views::CreateSolidBackground(kNotificationBackgroundColor));
  AddChildView(background_view_);

  focus_painter_ = views::Painter::CreateSolidFocusPainter(
      kFocusBorderColor, gfx::Insets(0, 0, 1, 1));

  UpdateWithNotification(notification);
}

MessageView::~MessageView() {}

void MessageView::UpdateWithNotification(const Notification& notification) {
  pinned_ = notification.pinned();
  base::string16 new_accessible_name = CreateAccessibleName(notification);
  if (new_accessible_name != accessible_name_) {
    accessible_name_ = new_accessible_name;
    NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
  }
  slide_out_controller_.set_enabled(!GetPinned());
}

void MessageView::SetIsNested() {
  is_nested_ = true;

  if (sidebar_enabled) {
    DCHECK(ShouldRoundMessageViewCorners());
    SetBorder(views::CreateRoundedRectBorder(0, kNotificationCornerRadius,
                                             kBorderColor));
  } else {
    if (ShouldRoundMessageViewCorners()) {
      SetBorder(views::CreateRoundedRectBorder(kNotificationBorderThickness,
                                               kNotificationCornerRadius,
                                               kBorderColor));
    } else {
      const auto& shadow =
          gfx::ShadowDetails::Get(kShadowElevation, kShadowCornerRadius);
      gfx::Insets ninebox_insets =
          gfx::ShadowValue::GetBlurRegion(shadow.values) +
          gfx::Insets(kShadowCornerRadius);
      SetBorder(views::CreateBorderPainter(
          std::unique_ptr<views::Painter>(views::Painter::CreateImagePainter(
              shadow.ninebox_image, ninebox_insets)),
          -gfx::ShadowValue::GetMargin(shadow.values)));
    }
  }
}

bool MessageView::IsCloseButtonFocused() const {
  auto* control_buttons_view = GetControlButtonsView();
  return control_buttons_view ? control_buttons_view->IsCloseButtonFocused()
                              : false;
}

void MessageView::RequestFocusOnCloseButton() {
  auto* control_buttons_view = GetControlButtonsView();
  if (!control_buttons_view)
    return;

  control_buttons_view->RequestFocusOnCloseButton();
  UpdateControlButtonsVisibility();
}

void MessageView::SetExpanded(bool expanded) {
  // Not implemented by default.
}

bool MessageView::IsExpanded() const {
  // Not implemented by default.
  return false;
}

bool MessageView::IsAutoExpandingAllowed() const {
  // Allowed by default.
  return true;
}

bool MessageView::IsManuallyExpandedOrCollapsed() const {
  // Not implemented by default.
  return false;
}

void MessageView::SetManuallyExpandedOrCollapsed(bool value) {
  // Not implemented by default.
}

void MessageView::OnContainerAnimationStarted() {
  // Not implemented by default.
}

void MessageView::OnContainerAnimationEnded() {
  // Not implemented by default.
}

void MessageView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kButton;
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kRoleDescription,
      l10n_util::GetStringUTF8(
          IDS_MESSAGE_NOTIFICATION_SETTINGS_BUTTON_ACCESSIBLE_NAME));
  node_data->SetName(accessible_name_);
}

bool MessageView::OnMousePressed(const ui::MouseEvent& event) {
  return true;
}

bool MessageView::OnMouseDragged(const ui::MouseEvent& event) {
  return true;
}

void MessageView::OnMouseReleased(const ui::MouseEvent& event) {
  if (!event.IsOnlyLeftMouseButton())
    return;

  MessageCenter::Get()->ClickOnNotification(notification_id_);
}

bool MessageView::OnKeyPressed(const ui::KeyEvent& event) {
  if (event.flags() != ui::EF_NONE)
    return false;

  if (event.key_code() == ui::VKEY_RETURN) {
    MessageCenter::Get()->ClickOnNotification(notification_id_);
    return true;
  } else if ((event.key_code() == ui::VKEY_DELETE ||
              event.key_code() == ui::VKEY_BACK)) {
    MessageCenter::Get()->RemoveNotification(notification_id_,
                                             true /* by_user */);
    return true;
  }

  return false;
}

bool MessageView::OnKeyReleased(const ui::KeyEvent& event) {
  // Space key handling is triggerred at key-release timing. See
  // ui/views/controls/buttons/button.cc for why.
  if (event.flags() != ui::EF_NONE || event.key_code() != ui::VKEY_SPACE)
    return false;

  MessageCenter::Get()->ClickOnNotification(notification_id_);
  return true;
}

void MessageView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  views::Painter::PaintFocusPainter(this, canvas, focus_painter_.get());
}

void MessageView::OnFocus() {
  views::View::OnFocus();
  // We paint a focus indicator.
  SchedulePaint();
}

void MessageView::OnBlur() {
  views::View::OnBlur();
  // We paint a focus indicator.
  SchedulePaint();
}

void MessageView::Layout() {
  views::View::Layout();

  gfx::Rect content_bounds = GetContentsBounds();

  // Background.
  background_view_->SetBoundsRect(content_bounds);

  // ChromeOS rounds the corners of the message view. TODO(estade): should we do
  // this for all platforms?
  if (ShouldRoundMessageViewCorners()) {
    gfx::Path path;
    constexpr SkScalar kCornerRadius = SkIntToScalar(kNotificationCornerRadius);
    path.addRoundRect(gfx::RectToSkRect(background_view_->GetLocalBounds()),
                      kCornerRadius, kCornerRadius);
    background_view_->set_clip_path(path);
  }
}

const char* MessageView::GetClassName() const {
  return kViewClassName;
}

void MessageView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_TAP_DOWN: {
      SetDrawBackgroundAsActive(true);
      break;
    }
    case ui::ET_GESTURE_TAP_CANCEL:
    case ui::ET_GESTURE_END: {
      SetDrawBackgroundAsActive(false);
      break;
    }
    case ui::ET_GESTURE_TAP: {
      SetDrawBackgroundAsActive(false);
      MessageCenter::Get()->ClickOnNotification(notification_id_);
      event->SetHandled();
      return;
    }
    default: {
      // Do nothing
    }
  }

  if (!event->IsScrollGestureEvent() && !event->IsFlingScrollEvent())
    return;

  if (scroller_)
    scroller_->OnGestureEvent(event);
  event->SetHandled();
}

ui::Layer* MessageView::GetSlideOutLayer() {
  return is_nested_ ? layer() : GetWidget()->GetLayer();
}

void MessageView::OnSlideChanged() {}

void MessageView::OnSlideOut() {
  // As a workaround for a MessagePopupCollection bug https://crbug.com/805208,
  // pass false to by_user although it is triggered by user.
  // TODO(tetsui): Rewrite MessagePopupCollection and remove this hack.
  if (pinned_) {
    // Also a workaround to not break notification pinning.
    MessageCenter::Get()->MarkSinglePopupAsShown(
        notification_id_, true /* mark_notification_as_read */);
  } else {
    MessageCenter::Get()->RemoveNotification(notification_id_,
                                             true /* by_user */);
  }
}

bool MessageView::GetPinned() const {
  // Only nested notifications can be pinned. Standalones (i.e. popups) can't
  // be.
  return pinned_ && is_nested_;
}

void MessageView::OnCloseButtonPressed() {
  MessageCenter::Get()->RemoveNotification(notification_id_,
                                           true /* by_user */);
}

void MessageView::OnSettingsButtonPressed(const ui::Event& event) {
  MessageCenter::Get()->ClickOnSettingsButton(notification_id_);
}

void MessageView::SetDrawBackgroundAsActive(bool active) {
  background_view_->background()->
      SetNativeControlColor(active ? kHoveredButtonBackgroundColor :
                                     kNotificationBackgroundColor);
  SchedulePaint();
}

}  // namespace message_center
