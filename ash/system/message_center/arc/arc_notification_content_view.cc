// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/message_center/arc/arc_notification_content_view.h"

#include "ash/system/message_center/arc/arc_notification_surface.h"
#include "ash/system/message_center/arc/arc_notification_view.h"
// TODO(https://crbug.com/768439): Remove nogncheck when moved to ash.
#include "ash/wm/window_util.h"  // nogncheck
#include "base/auto_reset.h"
#include "components/exo/notification_surface.h"
#include "components/exo/surface.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/transform.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/window_util.h"

namespace ash {

namespace {

SkColor GetControlButtonBackgroundColor(
    const arc::mojom::ArcNotificationShownContents& shown_contents) {
  if (shown_contents ==
      arc::mojom::ArcNotificationShownContents::CONTENTS_SHOWN)
    return message_center::kControlButtonBackgroundColor;
  else
    return SK_ColorTRANSPARENT;
}

}  // namespace

class ArcNotificationContentView::MouseEnterExitHandler
    : public ui::EventHandler {
 public:
  explicit MouseEnterExitHandler(ArcNotificationContentView* owner)
      : owner_(owner) {
    DCHECK(owner);
  }
  ~MouseEnterExitHandler() override = default;

  // ui::EventHandler
  void OnMouseEvent(ui::MouseEvent* event) override {
    ui::EventHandler::OnMouseEvent(event);
    if (event->type() == ui::ET_MOUSE_ENTERED ||
        event->type() == ui::ET_MOUSE_EXITED) {
      owner_->UpdateControlButtonsVisibility();
    }
  }

 private:
  ArcNotificationContentView* const owner_;

  DISALLOW_COPY_AND_ASSIGN(MouseEnterExitHandler);
};

class ArcNotificationContentView::EventForwarder : public ui::EventHandler {
 public:
  explicit EventForwarder(ArcNotificationContentView* owner) : owner_(owner) {}
  ~EventForwarder() override = default;

 private:
  // Some swipes are handled by Android alone. We don't want to capture swipe
  // events if we started a swipe on the chrome side then moved into the Android
  // swipe region. So, keep track of whether swipe has been 'captured' by
  // Android.
  bool swipe_captured_ = false;

  // ui::EventHandler
  void OnEvent(ui::Event* event) override {
    // Do not forward event targeted to the floating close button so that
    // keyboard press and tap are handled properly.
    if (owner_->floating_control_buttons_widget_ && event->target() &&
        owner_->floating_control_buttons_widget_->GetNativeWindow() ==
            event->target()) {
      return;
    }

    // TODO(sarakato): Use a better tigger (eg. focusing EditText on
    // notification) than clicking (b/78604162).
    if (event->type() == ui::ET_MOUSE_PRESSED ||
        event->type() == ui::ET_GESTURE_TAP) {
      // Remove the focus from the currently focused view-control in the message
      // center before activating the window of ARC notification, so that
      // unexpected key handling doesn't happen (b/74415372).
      // Focusing notification surface window doesn't steal the focus from
      // the focucued view control in the message center, so that input events
      // handles on both side wrongly without this.
      owner_->GetFocusManager()->ClearFocus();

      owner_->Activate();
    }

    views::Widget* widget = owner_->GetWidget();
    if (!widget)
      return;

    // Forward the events to the containing widget, except for:
    // 1. Touches, because View should no longer receive touch events.
    //    See View::OnTouchEvent.
    // 2. Tap gestures are handled on the Android side, so ignore them.
    //    See https://crbug.com/709911.
    // 3. Key events. These are already forwarded by NotificationSurface's
    //    WindowDelegate.
    if (event->IsLocatedEvent()) {
      ui::LocatedEvent* located_event = event->AsLocatedEvent();
      located_event->target()->ConvertEventToTarget(widget->GetNativeWindow(),
                                                    located_event);
      if (located_event->type() == ui::ET_MOUSE_ENTERED ||
          located_event->type() == ui::ET_MOUSE_EXITED) {
        owner_->UpdateControlButtonsVisibility();
        return;
      }

      if (located_event->type() == ui::ET_MOUSE_MOVED ||
          located_event->IsMouseWheelEvent()) {
        widget->OnMouseEvent(located_event->AsMouseEvent());
      } else if (located_event->IsScrollEvent()) {
        widget->OnScrollEvent(located_event->AsScrollEvent());
      } else if (located_event->IsGestureEvent() &&
                 event->type() != ui::ET_GESTURE_TAP) {
        bool event_for_android_only = false;
        if ((event->type() == ui::ET_GESTURE_SCROLL_BEGIN ||
             event->type() == ui::ET_GESTURE_SCROLL_UPDATE ||
             event->type() == ui::ET_GESTURE_SCROLL_END ||
             event->type() == ui::ET_GESTURE_SWIPE) &&
            owner_->surface_) {
          gfx::RectF rect(owner_->item_->GetSwipeInputRect());
          owner_->surface_->GetContentWindow()->transform().TransformRect(
              &rect);
          gfx::Point location = located_event->location();
          views::View::ConvertPointFromWidget(owner_, &location);
          bool contains = rect.Contains(gfx::PointF(location));

          if (contains && event->type() == ui::ET_GESTURE_SCROLL_BEGIN)
            swipe_captured_ = true;

          event_for_android_only = contains && swipe_captured_;
        }

        if (event->type() == ui::ET_GESTURE_SCROLL_END)
          swipe_captured_ = false;

        if (!event_for_android_only)
          widget->OnGestureEvent(located_event->AsGestureEvent());
      }
    }

    // If AXTree is attached to notification content view, notification surface
    // always gets focus. Tab key events are consumed by the surface, and tab
    // focus traversal gets stuck at Android notification. To prevent it, always
    // pass tab key event to focus manager of content view.
    // TODO(yawano): include elements inside Android notification in tab focus
    // traversal rather than skipping them.
    if (owner_->surface_ && owner_->surface_->GetAXTreeId() != -1 &&
        event->IsKeyEvent()) {
      ui::KeyEvent* key_event = event->AsKeyEvent();
      if (key_event->key_code() == ui::VKEY_TAB &&
          (key_event->flags() == ui::EF_NONE ||
           key_event->flags() == ui::EF_SHIFT_DOWN)) {
        widget->GetFocusManager()->OnKeyEvent(*key_event);
      }
    }
  }

  ArcNotificationContentView* const owner_;

  DISALLOW_COPY_AND_ASSIGN(EventForwarder);
};

class ArcNotificationContentView::SlideHelper
    : public ui::LayerAnimationObserver {
 public:
  explicit SlideHelper(ArcNotificationContentView* owner) : owner_(owner) {
    GetSlideOutLayer()->GetAnimator()->AddObserver(this);

    // Reset opacity to 1 to handle to case when the surface is sliding before
    // getting managed by this class, e.g. sliding in a popup before showing
    // in a message center view.
    if (owner_->surface_) {
      DCHECK(owner_->surface_->GetWindow());
      owner_->surface_->GetWindow()->layer()->SetOpacity(1.0f);
    }
  }
  ~SlideHelper() override {
    if (GetSlideOutLayer())
      GetSlideOutLayer()->GetAnimator()->RemoveObserver(this);
  }

  void Update() {
    const bool has_animation =
        GetSlideOutLayer()->GetAnimator()->is_animating();
    const bool has_transform = !GetSlideOutLayer()->transform().IsIdentity();
    const bool sliding = has_transform || has_animation;
    if (sliding_ == sliding)
      return;

    sliding_ = sliding;

    if (sliding_)
      OnSlideStart();
    else
      OnSlideEnd();
  }

 private:
  // This is a temporary hack to address https://crbug.com/718965
  ui::Layer* GetSlideOutLayer() {
    ui::Layer* layer = owner_->parent()->layer();
    return layer ? layer : owner_->GetWidget()->GetLayer();
  }

  void OnSlideStart() { owner_->ShowCopiedSurface(); }

  void OnSlideEnd() { owner_->HideCopiedSurface(); }

  // ui::LayerAnimationObserver
  void OnLayerAnimationEnded(ui::LayerAnimationSequence* seq) override {
    Update();
  }
  void OnLayerAnimationAborted(ui::LayerAnimationSequence* seq) override {
    Update();
  }
  void OnLayerAnimationScheduled(ui::LayerAnimationSequence* seq) override {}

  ArcNotificationContentView* const owner_;
  bool sliding_ = false;

  DISALLOW_COPY_AND_ASSIGN(SlideHelper);
};

// static, for ArcNotificationContentView::GetClassName().
const char ArcNotificationContentView::kViewClassName[] =
    "ArcNotificationContentView";

ArcNotificationContentView::ArcNotificationContentView(
    ArcNotificationItem* item,
    const message_center::Notification& notification,
    message_center::MessageView* message_view)
    : item_(item),
      notification_key_(item->GetNotificationKey()),
      event_forwarder_(new EventForwarder(this)),
      mouse_enter_exit_handler_(new MouseEnterExitHandler(this)),
      control_buttons_view_(message_view) {
  // kNotificationWidth must be 360, since this value is separately defiend in
  // ArcNotificationWrapperView class in Android side.
  DCHECK_EQ(360, message_center::kNotificationWidth);

  SetFocusBehavior(FocusBehavior::ALWAYS);
  set_notify_enter_exit_on_child(true);

  item_->IncrementWindowRefCount();
  item_->AddObserver(this);

  auto* surface_manager = ArcNotificationSurfaceManager::Get();
  if (surface_manager) {
    surface_manager->AddObserver(this);
    ArcNotificationSurface* surface =
        surface_manager->GetArcSurface(notification_key_);
    if (surface)
      OnNotificationSurfaceAdded(surface);
  }

  // Creates the control_buttons_view_, which collects all control buttons into
  // a horizontal box.
  control_buttons_view_.set_owned_by_client();
  control_buttons_view_.SetBackgroundColor(
      GetControlButtonBackgroundColor(item_->GetShownContents()));

  Update(message_view, notification);

  // Create a layer as an anchor to insert surface copy during a slide.
  SetPaintToLayer();
  UpdatePreferredSize();
}

ArcNotificationContentView::~ArcNotificationContentView() {
  SetSurface(nullptr);

  auto* surface_manager = ArcNotificationSurfaceManager::Get();
  if (surface_manager)
    surface_manager->RemoveObserver(this);
  if (item_) {
    item_->RemoveObserver(this);
    item_->DecrementWindowRefCount();
  }
}

const char* ArcNotificationContentView::GetClassName() const {
  return kViewClassName;
}

void ArcNotificationContentView::Update(
    message_center::MessageView* message_view,
    const message_center::Notification& notification) {
  control_buttons_view_.ShowSettingsButton(
      notification.should_show_settings_button());
  control_buttons_view_.ShowCloseButton(!message_view->GetPinned());
  control_buttons_view_.SetBackgroundColor(
      GetControlButtonBackgroundColor(item_->GetShownContents()));
  UpdateControlButtonsVisibility();

  accessible_name_ = notification.accessible_name();
  UpdateSnapshot();
}

message_center::NotificationControlButtonsView*
ArcNotificationContentView::GetControlButtonsView() {
  // |control_buttons_view_| is hosted in |floating_control_buttons_widget_| and
  // should not be used when there is no |floating_control_buttons_widget_|.
  return floating_control_buttons_widget_ ? &control_buttons_view_ : nullptr;
}

void ArcNotificationContentView::UpdateControlButtonsVisibility() {
  if (!control_buttons_view_.parent())
    return;

  // If the visibility change is ongoing, skip this method to prevent an
  // infinite loop.
  if (updating_control_buttons_visibility_)
    return;

  DCHECK(floating_control_buttons_widget_);

  const bool target_visiblity =
      IsMouseHovered() || (control_buttons_view_.IsCloseButtonFocused()) ||
      (control_buttons_view_.IsSettingsButtonFocused());

  if (target_visiblity == floating_control_buttons_widget_->IsVisible())
    return;

  // Add the guard to prevent an infinite loop. Changing visibility may generate
  // an event and it may call thie method again.
  base::AutoReset<bool> reset(&updating_control_buttons_visibility_, true);

  if (target_visiblity)
    floating_control_buttons_widget_->Show();
  else
    floating_control_buttons_widget_->Hide();
}

void ArcNotificationContentView::OnSlideChanged() {
  if (slide_helper_)
    slide_helper_->Update();
}

void ArcNotificationContentView::OnContainerAnimationStarted() {
  ShowCopiedSurface();
}

void ArcNotificationContentView::OnContainerAnimationEnded() {
  HideCopiedSurface();
}

void ArcNotificationContentView::MaybeCreateFloatingControlButtons() {
  // Floating close button is a transient child of |surface_| and also part
  // of the hosting widget's focus chain. It could only be created when both
  // are present. Further, if we are being destroyed (|item_| is null), don't
  // create the control buttons.
  if (!surface_ || !GetWidget() || !item_)
    return;

  DCHECK(!control_buttons_view_.parent());
  DCHECK(!floating_control_buttons_widget_);

  views::Widget::InitParams params(views::Widget::InitParams::TYPE_CONTROL);
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = surface_->GetWindow();

  floating_control_buttons_widget_.reset(new views::Widget);
  floating_control_buttons_widget_->Init(params);
  floating_control_buttons_widget_->SetContentsView(&control_buttons_view_);
  floating_control_buttons_widget_->GetNativeWindow()->AddPreTargetHandler(
      mouse_enter_exit_handler_.get());

  // Put the close button into the focus chain.
  floating_control_buttons_widget_->SetFocusTraversableParent(
      GetWidget()->GetFocusTraversable());
  floating_control_buttons_widget_->SetFocusTraversableParentView(this);

  Layout();
}

void ArcNotificationContentView::SetSurface(ArcNotificationSurface* surface) {
  if (surface_ == surface)
    return;

  // Reset |floating_control_buttons_widget_| when |surface_| is changed.
  floating_control_buttons_widget_.reset();

  if (surface_) {
    DCHECK(surface_->GetWindow());
    DCHECK(surface_->GetContentWindow());
    surface_->GetContentWindow()->RemoveObserver(this);
    surface_->GetWindow()->RemovePreTargetHandler(event_forwarder_.get());

    if (surface_->GetAttachedHost() == this) {
      DCHECK_EQ(this, surface_->GetAttachedHost());
      surface_->Detach();
    }
  }

  surface_ = surface;

  if (surface_) {
    DCHECK(surface_->GetWindow());
    DCHECK(surface_->GetContentWindow());
    surface_->GetContentWindow()->AddObserver(this);
    surface_->GetWindow()->AddPreTargetHandler(event_forwarder_.get());

    if (GetWidget()) {
      // Force to detach the surface.
      if (surface_->IsAttached()) {
        // The attached host must not be this. Since if it is, this should
        // already be detached above.
        DCHECK_NE(this, surface_->GetAttachedHost());
        surface_->Detach();
      }
      AttachSurface();
    }
  }
}

void ArcNotificationContentView::UpdatePreferredSize() {
  gfx::Size preferred_size;
  if (surface_)
    preferred_size = surface_->GetSize();
  else if (item_)
    preferred_size = item_->GetSnapshot().size();

  if (preferred_size.IsEmpty())
    return;

  if (preferred_size.width() != message_center::kNotificationWidth) {
    const float scale = static_cast<float>(message_center::kNotificationWidth) /
                        preferred_size.width();
    preferred_size.SetSize(message_center::kNotificationWidth,
                           preferred_size.height() * scale);
  }

  SetPreferredSize(preferred_size);
}

void ArcNotificationContentView::UpdateSnapshot() {
  // Bail if we have a |surface_| because it controls the sizes and paints UI.
  if (surface_)
    return;

  UpdatePreferredSize();
  SchedulePaint();
}

void ArcNotificationContentView::AttachSurface() {
  DCHECK(!native_view());

  if (!GetWidget())
    return;

  UpdatePreferredSize();
  surface_->Attach(this);

  // The texture for this window can be placed at subpixel position
  // with fractional scale factor. Force to align it at the pixel
  // boundary here, and when layout is updated in Layout().
  wm::SnapWindowToPixelBoundary(surface_->GetWindow());

  // Creates slide helper after this view is added to its parent.
  slide_helper_.reset(new SlideHelper(this));

  // Invokes Update() in case surface is attached during a slide.
  slide_helper_->Update();

  // (Re-)create the floating buttons after |surface_| is attached to a widget.
  MaybeCreateFloatingControlButtons();
}

void ArcNotificationContentView::ShowCopiedSurface() {
  if (!surface_)
    return;
  DCHECK(surface_->GetWindow());
  surface_copy_ = ::wm::RecreateLayers(surface_->GetWindow());
  // |surface_copy_| is at (0, 0) in owner_->layer().
  surface_copy_->root()->SetBounds(gfx::Rect(surface_copy_->root()->size()));
  layer()->Add(surface_copy_->root());
  surface_->GetWindow()->layer()->SetOpacity(0.0f);
}

void ArcNotificationContentView::HideCopiedSurface() {
  if (!surface_)
    return;
  DCHECK(surface_->GetWindow());
  surface_->GetWindow()->layer()->SetOpacity(1.0f);
  Layout();
  surface_copy_.reset();
}

void ArcNotificationContentView::ViewHierarchyChanged(
    const views::View::ViewHierarchyChangedDetails& details) {
  views::Widget* widget = GetWidget();

  if (!details.is_add) {
    // Resets slide helper when this view is removed from its parent.
    slide_helper_.reset();

    // Bail if this view is no longer attached to a widget or native_view() has
    // attached to a different widget.
    if (!widget ||
        (native_view() && views::Widget::GetTopLevelWidgetForNativeView(
                              native_view()) != widget)) {
      return;
    }
  }

  views::NativeViewHost::ViewHierarchyChanged(details);

  if (!widget || !surface_ || !details.is_add)
    return;

  if (surface_->IsAttached())
    surface_->Detach();
  AttachSurface();
}

void ArcNotificationContentView::Layout() {
  base::AutoReset<bool> auto_reset_in_layout(&in_layout_, true);

  views::NativeViewHost::Layout();

  if (!surface_ || !GetWidget())
    return;

  const gfx::Rect contents_bounds = GetContentsBounds();

  // Scale notification surface if necessary.
  gfx::Transform transform;
  const gfx::Size surface_size = surface_->GetSize();
  if (!surface_size.IsEmpty()) {
    const float factor =
        static_cast<float>(message_center::kNotificationWidth) /
        surface_size.width();
    transform.Scale(factor, factor);
  }

  // Apply the transform to the surface content so that close button can
  // be positioned without the need to consider the transform.
  surface_->GetContentWindow()->SetTransform(transform);

  if (floating_control_buttons_widget_) {
    gfx::Rect control_buttons_bounds(contents_bounds);
    const gfx::Size button_size = control_buttons_view_.GetPreferredSize();

    control_buttons_bounds.set_x(control_buttons_bounds.right() -
                                 button_size.width() -
                                 message_center::kControlButtonPadding);
    control_buttons_bounds.set_y(control_buttons_bounds.y() +
                                 message_center::kControlButtonPadding);
    control_buttons_bounds.set_width(button_size.width());
    control_buttons_bounds.set_height(button_size.height());
    floating_control_buttons_widget_->SetBounds(control_buttons_bounds);
  }

  UpdateControlButtonsVisibility();

  wm::SnapWindowToPixelBoundary(surface_->GetWindow());
}

void ArcNotificationContentView::OnPaint(gfx::Canvas* canvas) {
  views::NativeViewHost::OnPaint(canvas);

  if (!surface_ && item_ && !item_->GetSnapshot().isNull()) {
    // Draw the snapshot if there is no surface and the snapshot is available.
    const gfx::Rect contents_bounds = GetContentsBounds();
    canvas->DrawImageInt(
        item_->GetSnapshot(), 0, 0, item_->GetSnapshot().width(),
        item_->GetSnapshot().height(), contents_bounds.x(), contents_bounds.y(),
        contents_bounds.width(), contents_bounds.height(), false);
  } else {
    // Draw a blank background otherwise. The height of the view and surface are
    // not exactly synced and user may see the blank area out of the surface.
    // This code prevetns an ugly blank area and show white color instead.
    // This should be removed after b/35786193 is done.
    canvas->DrawColor(SK_ColorWHITE);
  }
}

void ArcNotificationContentView::OnMouseEntered(const ui::MouseEvent&) {
  UpdateControlButtonsVisibility();
}

void ArcNotificationContentView::OnMouseExited(const ui::MouseEvent&) {
  UpdateControlButtonsVisibility();
}

void ArcNotificationContentView::OnFocus() {
  auto* notification_view = ArcNotificationView::FromView(parent());
  CHECK(notification_view);

  NativeViewHost::OnFocus();
  notification_view->OnContentFocused();

  if (surface_ && surface_->GetAXTreeId() != -1)
    Activate();
}

void ArcNotificationContentView::OnBlur() {
  if (!parent()) {
    // OnBlur may be called when this view is being removed.
    return;
  }

  auto* notification_view = ArcNotificationView::FromView(parent());
  CHECK(notification_view);

  NativeViewHost::OnBlur();
  notification_view->OnContentBlurred();
}

void ArcNotificationContentView::Activate() {
  if (!GetWidget())
    return;

  // Make the widget active.
  if (!GetWidget()->IsActive()) {
    GetWidget()->widget_delegate()->set_can_activate(true);
    GetWidget()->Activate();
  }

  // Focus the surface window.
  surface_->FocusSurfaceWindow();
}

views::FocusTraversable* ArcNotificationContentView::GetFocusTraversable() {
  if (floating_control_buttons_widget_)
    return static_cast<views::internal::RootView*>(
        floating_control_buttons_widget_->GetRootView());
  return nullptr;
}

void ArcNotificationContentView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  if (surface_ && surface_->GetAXTreeId() != -1) {
    node_data->role = ax::mojom::Role::kClient;
    node_data->AddIntAttribute(ax::mojom::IntAttribute::kChildTreeId,
                               surface_->GetAXTreeId());
  } else {
    node_data->role = ax::mojom::Role::kButton;
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kRoleDescription,
        l10n_util::GetStringUTF8(
            IDS_MESSAGE_NOTIFICATION_SETTINGS_BUTTON_ACCESSIBLE_NAME));
  }
  node_data->SetName(accessible_name_);
}

void ArcNotificationContentView::OnAccessibilityEvent(ax::mojom::Event event) {
  if (event == ax::mojom::Event::kTextSelectionChanged) {
    // Activate and request focus on notification content view. If text
    // selection changed event is dispatched, it indicates that user is going to
    // type something inside Android notification. Widget of message center is
    // not activated by default. We need to activate the widget. If other view
    // in message center has focus, it can consume key event. We need to request
    // focus to move it to this content view.
    Activate();
    RequestFocus();
  }
}

void ArcNotificationContentView::OnWindowBoundsChanged(
    aura::Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    ui::PropertyChangeReason reason) {
  if (in_layout_)
    return;

  UpdatePreferredSize();
  Layout();
}

void ArcNotificationContentView::OnWindowDestroying(aura::Window* window) {
  SetSurface(nullptr);
}

void ArcNotificationContentView::OnItemDestroying() {
  item_->RemoveObserver(this);
  item_ = nullptr;

  // Reset |surface_| with |item_| since no one is observing the |surface_|
  // after |item_| is gone and this view should be removed soon.
  SetSurface(nullptr);
}

void ArcNotificationContentView::OnNotificationSurfaceAdded(
    ArcNotificationSurface* surface) {
  if (surface->GetNotificationKey() != notification_key_)
    return;

  SetSurface(surface);

  // Notify ax::mojom::Event::kChildrenChanged to force AXNodeData of this view
  // updated. As order of OnNotificationSurfaceAdded call is not guaranteed, we
  // are dispatching the event in both ArcNotificationContentView and
  // ArcAccessibilityHelperBridge.
  NotifyAccessibilityEvent(ax::mojom::Event::kChildrenChanged, false);
}

void ArcNotificationContentView::OnNotificationSurfaceRemoved(
    ArcNotificationSurface* surface) {
  if (surface->GetNotificationKey() != notification_key_)
    return;

  SetSurface(nullptr);
}

}  // namespace ash
