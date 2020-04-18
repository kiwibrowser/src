// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_view.h"

#include <algorithm>
#include <memory>

#include "ash/drag_drop/drag_image_view.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/shelf_item_delegate.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/scoped_root_window_for_new_windows.h"
#include "ash/screen_util.h"
#include "ash/shelf/app_list_button.h"
#include "ash/shelf/back_button.h"
#include "ash/shelf/overflow_bubble.h"
#include "ash/shelf/overflow_bubble_view.h"
#include "ash/shelf/overflow_button.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_application_menu_model.h"
#include "ash/shelf/shelf_button.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_context_menu_model.h"
#include "ash/shelf/shelf_controller.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/shell_port.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/root_window_finder.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/auto_reset.h"
#include "base/metrics/histogram_macros.h"
#include "chromeos/chromeos_switches.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/ui_base_features.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/focus/focus_search.h"
#include "ui/views/view_model.h"
#include "ui/views/view_model_utils.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"

using gfx::Animation;
using views::View;

namespace ash {

// The proportion of the shelf space reserved for non-panel icons. Panels
// may flow into this space but will be put into the overflow bubble if there
// is contention for the space.
constexpr float kReservedNonPanelIconProportion = 0.67f;

// The distance of the cursor from the outer rim of the shelf before it
// separates.
constexpr int kRipOffDistance = 48;

// The rip off drag and drop proxy image should get scaled by this factor.
constexpr float kDragAndDropProxyScale = 1.2f;

// The opacity represents that this partially disappeared item will get removed.
constexpr float kDraggedImageOpacity = 0.5f;

namespace {

// Helper to check if tablet mode is enabled.
bool IsTabletModeEnabled() {
  return Shell::Get()->tablet_mode_controller() &&
         Shell::Get()
             ->tablet_mode_controller()
             ->IsTabletModeWindowManagerEnabled();
}
// The UMA histogram that logs the commands which are executed on non-app
// context menus.
constexpr char kNonAppContextMenuExecuteCommand[] =
    "Apps.ContextMenuExecuteCommand.NotFromApp";

// The UMA histogram that logs the commands which are executed on app context
// menus.
constexpr char kAppContextMenuExecuteCommand[] =
    "Apps.ContextMenuExecuteCommand.FromApp";

// A class to temporarily disable a given bounds animator.
class BoundsAnimatorDisabler {
 public:
  explicit BoundsAnimatorDisabler(views::BoundsAnimator* bounds_animator)
      : old_duration_(bounds_animator->GetAnimationDuration()),
        bounds_animator_(bounds_animator) {
    bounds_animator_->SetAnimationDuration(1);
  }

  ~BoundsAnimatorDisabler() {
    bounds_animator_->SetAnimationDuration(old_duration_);
  }

 private:
  // The previous animation duration.
  int old_duration_;
  // The bounds animator which gets used.
  views::BoundsAnimator* bounds_animator_;

  DISALLOW_COPY_AND_ASSIGN(BoundsAnimatorDisabler);
};

// Custom FocusSearch used to navigate the shelf in the order items are in
// the ViewModel.
class ShelfFocusSearch : public views::FocusSearch {
 public:
  explicit ShelfFocusSearch(views::ViewModel* view_model)
      : FocusSearch(nullptr, true, true), view_model_(view_model) {}
  ~ShelfFocusSearch() override = default;

  // views::FocusSearch:
  View* FindNextFocusableView(
      View* starting_view,
      FocusSearch::SearchDirection search_direction,
      FocusSearch::TraversalDirection traversal_direction,
      FocusSearch::StartingViewPolicy check_starting_view,
      FocusSearch::AnchoredDialogPolicy can_go_into_anchored_dialog,
      views::FocusTraversable** focus_traversable,
      View** focus_traversable_view) override {
    int index = view_model_->GetIndexOfView(starting_view);
    // The back button (item with index 0 on the model) only exists in tablet
    // mode, so punt focus to the app list button (item with index 1 on the
    // model).
    const bool tablet_mode = IsTabletModeEnabled();
    if (!tablet_mode && index == 0)
      ++index;

    // Increment or decrement index based on the cycle, unless we are at either
    // edge, then we loop to the back or front. Skip the back button (item with
    // index 0) when not in tablet mode.
    if (search_direction == FocusSearch::SearchDirection::kBackwards) {
      --index;
      if (index < 0 || (index == 0 && !tablet_mode))
        index = view_model_->view_size() - 1;
    } else {
      ++index;
      if (index >= view_model_->view_size())
        index = tablet_mode ? 0 : 1;
    }

    return view_model_->view_at(index);
  }

 private:
  views::ViewModel* view_model_;

  DISALLOW_COPY_AND_ASSIGN(ShelfFocusSearch);
};

// AnimationDelegate used when inserting a new item. This steadily increases the
// opacity of the layer as the animation progress.
class FadeInAnimationDelegate : public gfx::AnimationDelegate {
 public:
  explicit FadeInAnimationDelegate(views::View* view) : view_(view) {}
  ~FadeInAnimationDelegate() override = default;

  // AnimationDelegate overrides:
  void AnimationProgressed(const Animation* animation) override {
    view_->layer()->SetOpacity(animation->GetCurrentValue());
    view_->layer()->ScheduleDraw();
  }
  void AnimationEnded(const Animation* animation) override {
    view_->layer()->SetOpacity(1.0f);
    view_->layer()->ScheduleDraw();
  }
  void AnimationCanceled(const Animation* animation) override {
    view_->layer()->SetOpacity(1.0f);
    view_->layer()->ScheduleDraw();
  }

 private:
  views::View* view_;

  DISALLOW_COPY_AND_ASSIGN(FadeInAnimationDelegate);
};

void ReflectItemStatus(const ShelfItem& item, ShelfButton* button) {
  switch (item.status) {
    case STATUS_CLOSED:
      button->ClearState(ShelfButton::STATE_RUNNING);
      button->ClearState(ShelfButton::STATE_ATTENTION);
      break;
    case STATUS_RUNNING:
      button->AddState(ShelfButton::STATE_RUNNING);
      button->ClearState(ShelfButton::STATE_ATTENTION);
      break;
    case STATUS_ATTENTION:
      button->ClearState(ShelfButton::STATE_RUNNING);
      button->AddState(ShelfButton::STATE_ATTENTION);
      break;
  }

  if (features::IsTouchableAppContextMenuEnabled()) {
    if (item.has_notification)
      button->AddState(ShelfButton::STATE_NOTIFICATION);
    else
      button->ClearState(ShelfButton::STATE_NOTIFICATION);
  }
}

// Returns the id of the display on which |view| is shown.
int64_t GetDisplayIdForView(View* view) {
  aura::Window* window = view->GetWidget()->GetNativeWindow();
  return display::Screen::GetScreen()->GetDisplayNearestWindow(window).id();
}

// Whether |item_view| is a ShelfButton and its state is STATE_DRAGGING.
bool ShelfButtonIsInDrag(const ShelfItemType item_type,
                         const views::View* item_view) {
  switch (item_type) {
    case TYPE_PINNED_APP:
    case TYPE_BROWSER_SHORTCUT:
    case TYPE_APP:
      return static_cast<const ShelfButton*>(item_view)->state() &
             ShelfButton::STATE_DRAGGING;
    case TYPE_DIALOG:
    case TYPE_APP_PANEL:
    case TYPE_BACK_BUTTON:
    case TYPE_APP_LIST:
    case TYPE_UNDEFINED:
      return false;
  }
}

}  // namespace

// AnimationDelegate used when deleting an item. This steadily decreased the
// opacity of the layer as the animation progress.
class ShelfView::FadeOutAnimationDelegate : public gfx::AnimationDelegate {
 public:
  FadeOutAnimationDelegate(ShelfView* host, views::View* view)
      : shelf_view_(host), view_(view) {}
  ~FadeOutAnimationDelegate() override = default;

  // AnimationDelegate overrides:
  void AnimationProgressed(const Animation* animation) override {
    view_->layer()->SetOpacity(1 - animation->GetCurrentValue());
    view_->layer()->ScheduleDraw();
  }
  void AnimationEnded(const Animation* animation) override {
    shelf_view_->OnFadeOutAnimationEnded();
  }
  void AnimationCanceled(const Animation* animation) override {}

 private:
  ShelfView* shelf_view_;
  std::unique_ptr<views::View> view_;

  DISALLOW_COPY_AND_ASSIGN(FadeOutAnimationDelegate);
};

// AnimationDelegate used to trigger fading an element in. When an item is
// inserted this delegate is attached to the animation that expands the size of
// the item.  When done it kicks off another animation to fade the item in.
class ShelfView::StartFadeAnimationDelegate : public gfx::AnimationDelegate {
 public:
  StartFadeAnimationDelegate(ShelfView* host, views::View* view)
      : shelf_view_(host), view_(view) {}
  ~StartFadeAnimationDelegate() override = default;

  // AnimationDelegate overrides:
  void AnimationEnded(const Animation* animation) override {
    shelf_view_->FadeIn(view_);
  }
  void AnimationCanceled(const Animation* animation) override {
    view_->layer()->SetOpacity(1.0f);
  }

 private:
  ShelfView* shelf_view_;
  views::View* view_;

  DISALLOW_COPY_AND_ASSIGN(StartFadeAnimationDelegate);
};

// static
const int ShelfView::kMinimumDragDistance = 8;

ShelfView::ShelfView(ShelfModel* model, Shelf* shelf, ShelfWidget* shelf_widget)
    : model_(model),
      shelf_(shelf),
      shelf_widget_(shelf_widget),
      view_model_(new views::ViewModel),
      tooltip_(this),
      shelf_item_background_color_(kShelfDefaultBaseColor),
      weak_factory_(this) {
  DCHECK(model_);
  DCHECK(shelf_);
  DCHECK(shelf_widget_);
  bounds_animator_.reset(new views::BoundsAnimator(this));
  bounds_animator_->AddObserver(this);
  set_context_menu_controller(this);
  focus_search_.reset(new ShelfFocusSearch(view_model_.get()));
}

ShelfView::~ShelfView() {
  bounds_animator_->RemoveObserver(this);
  model_->RemoveObserver(this);
}

void ShelfView::Init() {
  model_->AddObserver(this);

  const ShelfItems& items(model_->items());
  for (ShelfItems::const_iterator i = items.begin(); i != items.end(); ++i) {
    views::View* child = CreateViewForItem(*i);
    child->SetPaintToLayer();
    view_model_->Add(child, static_cast<int>(i - items.begin()));
    AddChildView(child);
  }
  overflow_button_ = new OverflowButton(this, shelf_);
  overflow_button_->set_context_menu_controller(this);
  ConfigureChildView(overflow_button_);
  AddChildView(overflow_button_);
  UpdateBackButton();
  // We'll layout when our bounds change.
}

void ShelfView::OnShelfAlignmentChanged() {
  overflow_button_->OnShelfAlignmentChanged();
  LayoutToIdealBounds();
  for (int i = 0; i < view_model_->view_size(); ++i) {
    if (i >= first_visible_index_ && i <= last_visible_index_)
      view_model_->view_at(i)->Layout();
  }
  tooltip_.Close();
  if (overflow_bubble_)
    overflow_bubble_->Hide();
  // For crbug.com/587931, because AppListButton layout logic is in OnPaint.
  AppListButton* app_list_button = GetAppListButton();
  if (app_list_button)
    app_list_button->SchedulePaint();
}

gfx::Rect ShelfView::GetIdealBoundsOfItemIcon(const ShelfID& id) {
  int index = model_->ItemIndexByID(id);
  if (index < 0 || last_visible_index_ < 0 || index >= view_model_->view_size())
    return gfx::Rect();

  // Map items in the overflow bubble to the overflow button.
  if (index > last_visible_index_ && index < model_->FirstPanelIndex())
    return GetMirroredRect(overflow_button_->bounds());

  const gfx::Rect& ideal_bounds(view_model_->ideal_bounds(index));
  views::View* view = view_model_->view_at(index);
  // The app list and back button are not ShelfButton subclass instances.
  if (view == GetAppListButton() || view == GetBackButton())
    return GetMirroredRect(ideal_bounds);

  CHECK_EQ(ShelfButton::kViewClassName, view->GetClassName());
  ShelfButton* button = static_cast<ShelfButton*>(view);
  gfx::Rect icon_bounds = button->GetIconBounds();
  return gfx::Rect(GetMirroredXWithWidthInView(
                       ideal_bounds.x() + icon_bounds.x(), icon_bounds.width()),
                   ideal_bounds.y() + icon_bounds.y(), icon_bounds.width(),
                   icon_bounds.height());
}

void ShelfView::UpdatePanelIconPosition(const ShelfID& id,
                                        const gfx::Point& midpoint) {
  int current_index = model_->ItemIndexByID(id);
  int first_panel_index = model_->FirstPanelIndex();
  if (current_index < first_panel_index)
    return;

  gfx::Point midpoint_in_view(GetMirroredXInView(midpoint.x()), midpoint.y());
  int target_index = current_index;
  while (target_index > first_panel_index &&
         shelf_->PrimaryAxisValue(view_model_->ideal_bounds(target_index).x(),
                                  view_model_->ideal_bounds(target_index).y()) >
             shelf_->PrimaryAxisValue(midpoint_in_view.x(),
                                      midpoint_in_view.y())) {
    --target_index;
  }
  while (target_index < view_model_->view_size() - 1 &&
         shelf_->PrimaryAxisValue(
             view_model_->ideal_bounds(target_index).right(),
             view_model_->ideal_bounds(target_index).bottom()) <
             shelf_->PrimaryAxisValue(midpoint_in_view.x(),
                                      midpoint_in_view.y())) {
    ++target_index;
  }
  if (current_index != target_index)
    model_->Move(current_index, target_index);
}

bool ShelfView::IsShowingMenu() const {
  return launcher_menu_runner_.get() && launcher_menu_runner_->IsRunning();
}

bool ShelfView::IsShowingMenuForView(views::View* view) const {
  return IsShowingMenu() && menu_owner_ == view;
}

bool ShelfView::IsShowingOverflowBubble() const {
  return overflow_bubble_.get() && overflow_bubble_->IsShowing();
}

AppListButton* ShelfView::GetAppListButton() const {
  for (int i = 0; i < model_->item_count(); ++i) {
    if (model_->items()[i].type == TYPE_APP_LIST) {
      views::View* view = view_model_->view_at(i);
      CHECK_EQ(AppListButton::kViewClassName, view->GetClassName());
      return static_cast<AppListButton*>(view);
    }
  }

  NOTREACHED() << "Applist button not found";
  return nullptr;
}

BackButton* ShelfView::GetBackButton() const {
  for (int i = 0; i < model_->item_count(); ++i) {
    if (model_->items()[i].type == TYPE_BACK_BUTTON) {
      views::View* view = view_model_->view_at(i);
      CHECK_EQ(BackButton::kViewClassName, view->GetClassName());
      return static_cast<BackButton*>(view);
    }
  }

  NOTREACHED() << "Back button not found";
  return nullptr;
}

bool ShelfView::ShouldHideTooltip(const gfx::Point& cursor_location) const {
  gfx::Rect tooltip_bounds;
  for (int i = 0; i < child_count(); ++i) {
    const views::View* child = child_at(i);
    if (!IsTabletModeEnabled() && child == GetBackButton())
      continue;
    if (child != overflow_button_ && ShouldShowTooltipForView(child))
      tooltip_bounds.Union(child->GetMirroredBounds());
  }
  return !tooltip_bounds.Contains(cursor_location);
}

bool ShelfView::ShouldShowTooltipForView(const views::View* view) const {
  // TODO(msw): Push this app list state into ShelfItem::shows_tooltip.
  if (view == GetAppListButton() && GetAppListButton()->is_showing_app_list())
    return false;
  const ShelfItem* item = ShelfItemForView(view);
  return item && item->shows_tooltip;
}

base::string16 ShelfView::GetTitleForView(const views::View* view) const {
  const ShelfItem* item = ShelfItemForView(view);
  return item ? item->title : base::string16();
}

gfx::Rect ShelfView::GetVisibleItemsBoundsInScreen() {
  gfx::Size preferred_size = GetPreferredSize();
  gfx::Point origin(GetMirroredXWithWidthInView(0, preferred_size.width()), 0);
  ConvertPointToScreen(this, &origin);
  return gfx::Rect(origin, preferred_size);
}

void ShelfView::ButtonPressed(views::Button* sender,
                              const ui::Event& event,
                              views::InkDrop* ink_drop) {
  if (sender == overflow_button_) {
    ToggleOverflowBubble();
    shelf_button_pressed_metric_tracker_.ButtonPressed(event, sender,
                                                       SHELF_ACTION_NONE);
    return;
  }

  // None of the checks in ShouldEventActivateButton() affects overflow button.
  // So, it is safe to be checked after handling overflow button.
  if (!ShouldEventActivateButton(sender, event))
    return;

  // Close the overflow bubble if an item on either shelf is clicked. Press
  // events elsewhere will close the overflow shelf via OverflowBubble's
  // PointerWatcher functionality.
  ShelfView* shelf_view = main_shelf_ ? main_shelf_ : this;
  if (shelf_view->IsShowingOverflowBubble())
    shelf_view->ToggleOverflowBubble();

  // Record the index for the last pressed shelf item.
  last_pressed_index_ = view_model_->GetIndexOfView(sender);
  DCHECK_LT(-1, last_pressed_index_);

  // Place new windows on the same display as the button.
  aura::Window* window = sender->GetWidget()->GetNativeWindow();
  scoped_root_window_for_new_windows_.reset(
      new ScopedRootWindowForNewWindows(window->GetRootWindow()));

  // Slow down activation animations if shift key is pressed.
  std::unique_ptr<ui::ScopedAnimationDurationScaleMode> slowing_animations;
  if (event.IsShiftDown()) {
    slowing_animations.reset(new ui::ScopedAnimationDurationScaleMode(
        ui::ScopedAnimationDurationScaleMode::SLOW_DURATION));
  }

  // Collect usage statistics before we decide what to do with the click.
  switch (model_->items()[last_pressed_index_].type) {
    case TYPE_PINNED_APP:
    case TYPE_BROWSER_SHORTCUT:
    case TYPE_APP:
      Shell::Get()->metrics()->RecordUserMetricsAction(
          UMA_LAUNCHER_CLICK_ON_APP);
      break;

    case TYPE_APP_LIST:
      Shell::Get()->metrics()->RecordUserMetricsAction(
          UMA_LAUNCHER_CLICK_ON_APPLIST_BUTTON);
      break;

    case TYPE_APP_PANEL:
    case TYPE_BACK_BUTTON:
    case TYPE_DIALOG:
      break;

    case TYPE_UNDEFINED:
      NOTREACHED() << "ShelfItemType must be set.";
      break;
  }

  // Run AfterItemSelected directly if the item has no delegate (ie. in tests).
  const ShelfItem& item = model_->items()[last_pressed_index_];
  if (!model_->GetShelfItemDelegate(item.id)) {
    AfterItemSelected(item, sender, ui::Event::Clone(event), ink_drop,
                      SHELF_ACTION_NONE, base::nullopt);
    return;
  }

  // Notify the item of its selection; handle the result in AfterItemSelected.
  model_->GetShelfItemDelegate(item.id)->ItemSelected(
      ui::Event::Clone(event), GetDisplayIdForView(this), LAUNCH_FROM_UNKNOWN,
      base::Bind(&ShelfView::AfterItemSelected, weak_factory_.GetWeakPtr(),
                 item, sender, base::Passed(ui::Event::Clone(event)),
                 ink_drop));
}

////////////////////////////////////////////////////////////////////////////////
// ShelfView, FocusTraversable implementation:

views::FocusSearch* ShelfView::GetFocusSearch() {
  return focus_search_.get();
}

views::FocusTraversable* ShelfView::GetFocusTraversableParent() {
  return parent()->GetFocusTraversable();
}

View* ShelfView::GetFocusTraversableParentView() {
  return this;
}

void ShelfView::CreateDragIconProxy(
    const gfx::Point& location_in_screen_coordinates,
    const gfx::ImageSkia& icon,
    views::View* replaced_view,
    const gfx::Vector2d& cursor_offset_from_center,
    float scale_factor) {
  drag_replaced_view_ = replaced_view;
  aura::Window* root_window =
      drag_replaced_view_->GetWidget()->GetNativeWindow()->GetRootWindow();
  drag_image_ = std::make_unique<DragImageView>(
      root_window, ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE);
  drag_image_->SetImage(icon);
  gfx::Size size = drag_image_->GetPreferredSize();
  size.set_width(size.width() * scale_factor);
  size.set_height(size.height() * scale_factor);
  drag_image_offset_ = gfx::Vector2d(size.width() / 2, size.height() / 2) +
                       cursor_offset_from_center;
  gfx::Rect drag_image_bounds(
      location_in_screen_coordinates - drag_image_offset_, size);
  drag_image_->SetBoundsInScreen(drag_image_bounds);
  drag_image_->SetWidgetVisible(true);
}

void ShelfView::CreateDragIconProxyByLocationWithNoAnimation(
    const gfx::Point& origin_in_screen_coordinates,
    const gfx::ImageSkia& icon,
    views::View* replaced_view,
    float scale_factor) {
  drag_replaced_view_ = replaced_view;
  aura::Window* root_window =
      drag_replaced_view_->GetWidget()->GetNativeWindow()->GetRootWindow();
  drag_image_ = std::make_unique<DragImageView>(
      root_window, ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE);
  drag_image_->SetImage(icon);
  gfx::Size size = drag_image_->GetPreferredSize();
  size.set_width(size.width() * scale_factor);
  size.set_height(size.height() * scale_factor);
  gfx::Rect drag_image_bounds(origin_in_screen_coordinates, size);
  drag_image_->SetBoundsInScreen(drag_image_bounds);

  // Turn off the default visibility animation.
  drag_image_->GetWidget()->SetVisibilityAnimationTransition(
      views::Widget::ANIMATE_NONE);
  drag_image_->SetWidgetVisible(true);
}

void ShelfView::UpdateDragIconProxy(
    const gfx::Point& location_in_screen_coordinates) {
  // TODO(jennyz): Investigate why drag_image_ becomes null at this point per
  // crbug.com/34722, while the app list item is still being dragged around.
  if (drag_image_) {
    drag_image_->SetScreenPosition(location_in_screen_coordinates -
                                   drag_image_offset_);
  }
}

void ShelfView::UpdateDragIconProxyByLocation(
    const gfx::Point& origin_in_screen_coordinates) {
  if (drag_image_)
    drag_image_->SetScreenPosition(origin_in_screen_coordinates);
}

bool ShelfView::IsDraggedView(const ShelfButton* view) const {
  return drag_view_ == view;
}

const std::vector<aura::Window*> ShelfView::GetOpenWindowsForShelfView(
    views::View* view) {
  std::vector<aura::Window*> window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  std::vector<aura::Window*> open_windows;
  const std::string shelf_item_app_id = ShelfItemForView(view)->id.app_id;
  for (auto* window : window_list) {
    const std::string window_app_id =
        ShelfID::Deserialize(window->GetProperty(kShelfIDKey)).app_id;
    if (window_app_id == shelf_item_app_id) {
      // TODO: In the very first version we only show one window. Add the proper
      // UI to show all windows for a given open app.
      open_windows.push_back(window);
    }
  }
  return open_windows;
}

void ShelfView::DestroyDragIconProxy() {
  drag_image_.reset();
  drag_image_offset_ = gfx::Vector2d(0, 0);
}

bool ShelfView::StartDrag(const std::string& app_id,
                          const gfx::Point& location_in_screen_coordinates) {
  // Bail if an operation is already going on - or the cursor is not inside.
  // This could happen if mouse / touch operations overlap.
  if (!drag_and_drop_shelf_id_.IsNull() || app_id.empty() ||
      !GetBoundsInScreen().Contains(location_in_screen_coordinates))
    return false;

  // If the AppsGridView (which was dispatching this event) was opened by our
  // button, ShelfView dragging operations are locked and we have to unlock.
  CancelDrag(-1);
  drag_and_drop_item_pinned_ = false;
  drag_and_drop_shelf_id_ = ShelfID(app_id);
  // Check if the application is pinned - if not, we have to pin it so
  // that we can re-arrange the shelf order accordingly. Note that items have
  // to be pinned to give them the same (order) possibilities as a shortcut.
  // When an item is dragged from overflow to shelf, IsShowingOverflowBubble()
  // returns true. At this time, we don't need to pin the item.
  if (!IsShowingOverflowBubble() && !model_->IsAppPinned(app_id)) {
    model_->PinAppWithID(app_id);
    drag_and_drop_item_pinned_ = true;
  }
  views::View* drag_and_drop_view =
      view_model_->view_at(model_->ItemIndexByID(drag_and_drop_shelf_id_));
  DCHECK(drag_and_drop_view);

  // Since there is already an icon presented by the caller, we hide this item
  // for now. That has to be done by reducing the size since the visibility will
  // change once a regrouping animation is performed.
  pre_drag_and_drop_size_ = drag_and_drop_view->size();
  drag_and_drop_view->SetSize(gfx::Size());

  // First we have to center the mouse cursor over the item.
  gfx::Point pt = drag_and_drop_view->GetBoundsInScreen().CenterPoint();
  views::View::ConvertPointFromScreen(drag_and_drop_view, &pt);
  gfx::Point point_in_root = location_in_screen_coordinates;
  ::wm::ConvertPointFromScreen(
      wm::GetRootWindowAt(location_in_screen_coordinates), &point_in_root);
  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, pt, point_in_root,
                       ui::EventTimeForNow(), 0, 0);
  PointerPressedOnButton(drag_and_drop_view, DRAG_AND_DROP, event);

  // Drag the item where it really belongs.
  Drag(location_in_screen_coordinates);
  return true;
}

bool ShelfView::Drag(const gfx::Point& location_in_screen_coordinates) {
  if (drag_and_drop_shelf_id_.IsNull() ||
      !GetBoundsInScreen().Contains(location_in_screen_coordinates))
    return false;

  gfx::Point pt = location_in_screen_coordinates;
  views::View* drag_and_drop_view =
      view_model_->view_at(model_->ItemIndexByID(drag_and_drop_shelf_id_));
  ConvertPointFromScreen(drag_and_drop_view, &pt);
  gfx::Point point_in_root = location_in_screen_coordinates;
  ::wm::ConvertPointFromScreen(
      wm::GetRootWindowAt(location_in_screen_coordinates), &point_in_root);
  ui::MouseEvent event(ui::ET_MOUSE_DRAGGED, pt, point_in_root,
                       ui::EventTimeForNow(), 0, 0);
  PointerDraggedOnButton(drag_and_drop_view, DRAG_AND_DROP, event);
  return true;
}

void ShelfView::EndDrag(bool cancel) {
  if (drag_and_drop_shelf_id_.IsNull())
    return;

  views::View* drag_and_drop_view =
      view_model_->view_at(model_->ItemIndexByID(drag_and_drop_shelf_id_));
  PointerReleasedOnButton(drag_and_drop_view, DRAG_AND_DROP, cancel);

  // Either destroy the temporarily created item - or - make the item visible.
  if (drag_and_drop_item_pinned_ && cancel) {
    model_->UnpinAppWithID(drag_and_drop_shelf_id_.app_id);
  } else if (drag_and_drop_view) {
    if (cancel) {
      // When a hosted drag gets canceled, the item can remain in the same slot
      // and it might have moved within the bounds. In that case the item need
      // to animate back to its correct location.
      AnimateToIdealBounds();
    } else {
      drag_and_drop_view->SetSize(pre_drag_and_drop_size_);
    }
  }

  drag_and_drop_shelf_id_ = ShelfID();
}

bool ShelfView::ShouldEventActivateButton(View* view, const ui::Event& event) {
  if (dragging())
    return false;

  // Ignore if we are already in a pointer event sequence started with a repost
  // event on the same shelf item. See crbug.com/343005 for more detail.
  if (is_repost_event_on_same_item_)
    return false;

  // Don't activate the item twice on double-click. Otherwise the window starts
  // animating open due to the first click, then immediately minimizes due to
  // the second click. The user most likely intended to open or minimize the
  // item once, not do both.
  if (event.flags() & ui::EF_IS_DOUBLE_CLICK)
    return false;

  // Ignore if this is a repost event on the last pressed shelf item.
  int index = view_model_->GetIndexOfView(view);
  if (index == -1)
    return false;
  return !IsRepostEvent(event) || last_pressed_index_ != index;
}

void ShelfView::PointerPressedOnButton(views::View* view,
                                       Pointer pointer,
                                       const ui::LocatedEvent& event) {
  if (drag_view_)
    return;

  if (IsShowingMenu())
    launcher_menu_runner_->Cancel();

  int index = view_model_->GetIndexOfView(view);
  if (index == -1 || view_model_->view_size() <= 1)
    return;  // View is being deleted, ignore request.

  if (view == GetAppListButton() || view == GetBackButton())
    return;  // Views are not draggable, ignore request.

  // Only when the repost event occurs on the same shelf item, we should ignore
  // the call in ShelfView::ButtonPressed(...).
  is_repost_event_on_same_item_ =
      IsRepostEvent(event) && (last_pressed_index_ == index);

  CHECK_EQ(ShelfButton::kViewClassName, view->GetClassName());
  drag_view_ = static_cast<ShelfButton*>(view);
  drag_origin_ = gfx::Point(event.x(), event.y());
  UMA_HISTOGRAM_ENUMERATION("Ash.ShelfAlignmentUsage",
                            static_cast<ShelfAlignmentUmaEnumValue>(
                                shelf_->SelectValueForShelfAlignment(
                                    SHELF_ALIGNMENT_UMA_ENUM_VALUE_BOTTOM,
                                    SHELF_ALIGNMENT_UMA_ENUM_VALUE_LEFT,
                                    SHELF_ALIGNMENT_UMA_ENUM_VALUE_RIGHT)),
                            SHELF_ALIGNMENT_UMA_ENUM_VALUE_COUNT);
}

void ShelfView::PointerDraggedOnButton(views::View* view,
                                       Pointer pointer,
                                       const ui::LocatedEvent& event) {
  if (CanPrepareForDrag(pointer, event))
    PrepareForDrag(pointer, event);

  if (drag_pointer_ == pointer)
    ContinueDrag(event);
}

void ShelfView::PointerReleasedOnButton(views::View* view,
                                        Pointer pointer,
                                        bool canceled) {
  is_repost_event_on_same_item_ = false;

  if (canceled) {
    CancelDrag(-1);
  } else if (drag_pointer_ == pointer) {
    FinalizeRipOffDrag(false);
    drag_pointer_ = NONE;
    AnimateToIdealBounds();
  }
  // If the drag pointer is NONE, no drag operation is going on and the
  // drag_view can be released.
  if (drag_pointer_ == NONE)
    drag_view_ = nullptr;
}

void ShelfView::LayoutToIdealBounds() {
  if (bounds_animator_->IsAnimating()) {
    AnimateToIdealBounds();
    return;
  }

  gfx::Rect overflow_bounds;
  CalculateIdealBounds(&overflow_bounds);
  views::ViewModelUtils::SetViewBoundsToIdealBounds(*view_model_);
  overflow_button_->SetBoundsRect(overflow_bounds);
  UpdateBackButton();
}

void ShelfView::UpdateShelfItemBackground(SkColor color) {
  shelf_item_background_color_ = color;
  overflow_button_->UpdateShelfItemBackground(color);
  SchedulePaint();
}

void ShelfView::UpdateAllButtonsVisibilityInOverflowMode() {
  // The overflow button is not shown in overflow mode.
  overflow_button_->SetVisible(false);
  DCHECK_LT(last_visible_index_, view_model_->view_size());
  for (int i = 0; i < view_model_->view_size(); ++i) {
    bool visible = i >= first_visible_index_ && i <= last_visible_index_;
    // To track the dragging of |drag_view_| continuously, its visibility
    // should be always true regardless of its position.
    if (dragged_to_another_shelf_ && view_model_->view_at(i) == drag_view_)
      view_model_->view_at(i)->SetVisible(true);
    else
      view_model_->view_at(i)->SetVisible(visible);
  }
}

void ShelfView::CalculateIdealBounds(gfx::Rect* overflow_bounds) const {
  DCHECK(model_->item_count() == view_model_->view_size());

  int available_size = shelf_->PrimaryAxisValue(width(), height());
  int first_panel_index = model_->FirstPanelIndex();
  int last_button_index = first_panel_index - 1;

  int x = 0;
  int y = 0;

  int w = shelf_->PrimaryAxisValue(kShelfButtonSize, width());
  int h = shelf_->PrimaryAxisValue(height(), kShelfButtonSize);

  for (int i = 0; i < view_model_->view_size(); ++i) {
    if (i < first_visible_index_) {
      view_model_->set_ideal_bounds(i, gfx::Rect(x, y, 0, 0));
      continue;
    }

    view_model_->set_ideal_bounds(i, gfx::Rect(x, y, w, h));
    // If not in tablet mode do not increase |x| or |y|. Instead just let the
    // next item (app list button) cover the back button, which will have
    // opacity 0 anyways.
    if (i == 0 && !IsTabletModeEnabled())
      continue;

    // There is no spacing between the first two elements. Do not worry about y
    // since the back button only appears in tablet mode, which forces the shelf
    // to be bottom aligned.
    x = shelf_->PrimaryAxisValue(x + w + (i == 0 ? 0 : kShelfButtonSpacing), x);
    y = shelf_->PrimaryAxisValue(y, y + h + kShelfButtonSpacing);
  }

  if (is_overflow_mode()) {
    const_cast<ShelfView*>(this)->UpdateAllButtonsVisibilityInOverflowMode();
    return;
  }

  // Right aligned icons.
  int end_position = available_size;
  x = shelf_->PrimaryAxisValue(end_position, 0);
  y = shelf_->PrimaryAxisValue(0, end_position);
  for (int i = view_model_->view_size() - 1; i >= first_panel_index; --i) {
    x = shelf_->PrimaryAxisValue(x - w - kShelfButtonSpacing, x);
    y = shelf_->PrimaryAxisValue(y, y - h - kShelfButtonSpacing);
    view_model_->set_ideal_bounds(i, gfx::Rect(x, y, w, h));
    end_position = shelf_->PrimaryAxisValue(x, y);
  }

  // Icons on the left / top are guaranteed up to kLeftIconProportion of
  // the available space.
  int last_icon_position =
      shelf_->PrimaryAxisValue(
          view_model_->ideal_bounds(last_button_index).right(),
          view_model_->ideal_bounds(last_button_index).bottom()) +
      kShelfButtonSpacing;
  int reserved_icon_space = available_size * kReservedNonPanelIconProportion;
  if (last_icon_position < reserved_icon_space)
    end_position = last_icon_position;
  else
    end_position = std::max(end_position, reserved_icon_space);

  overflow_bounds->set_size(gfx::Size(shelf_->PrimaryAxisValue(w, width()),
                                      shelf_->PrimaryAxisValue(height(), h)));

  last_visible_index_ =
      DetermineLastVisibleIndex(end_position - kShelfButtonSpacing);
  last_hidden_index_ = DetermineFirstVisiblePanelIndex(end_position) - 1;
  bool show_overflow = last_visible_index_ < last_button_index ||
                       last_hidden_index_ >= first_panel_index;

  // Create Space for the overflow button
  if (show_overflow) {
    // The following code makes sure that platform apps icons (aligned to left /
    // top) are favored over panel apps icons (aligned to right / bottom).
    if (last_visible_index_ > 0 && last_visible_index_ < last_button_index) {
      // This condition means that we will take one platform app and replace it
      // with the overflow button and put the app in the overflow bubble.
      // This happens when the space needed for platform apps exceeds the
      // reserved area for non-panel icons,
      // (i.e. |last_icon_position| > |reserved_icon_space|).
      --last_visible_index_;
    } else if (last_hidden_index_ >= first_panel_index &&
               last_hidden_index_ < view_model_->view_size() - 1) {
      // This condition means that we will take a panel app icon and replace it
      // with the overflow button.
      // This happens when there is still room for platform apps in the reserved
      // area for non-panel icons,
      // (i.e. |last_icon_position| < |reserved_icon_space|).
      ++last_hidden_index_;
    }
  }

  for (int i = 0; i < view_model_->view_size(); ++i) {
    bool visible = i <= last_visible_index_ || i > last_hidden_index_;
    // To receive drag event continuously from |drag_view_| during the dragging
    // off from the shelf, don't make |drag_view_| invisible. It will be
    // eventually invisible and removed from the |view_model_| by
    // FinalizeRipOffDrag().
    if (dragged_off_shelf_ && view_model_->view_at(i) == drag_view_)
      continue;
    view_model_->view_at(i)->SetVisible(visible);
  }

  overflow_button_->SetVisible(show_overflow);
  if (show_overflow) {
    DCHECK_NE(0, view_model_->view_size());
    if (last_visible_index_ == -1) {
      x = 0;
      y = 0;
    } else {
      x = shelf_->PrimaryAxisValue(
          view_model_->ideal_bounds(last_visible_index_).right(),
          view_model_->ideal_bounds(last_visible_index_).x());
      y = shelf_->PrimaryAxisValue(
          view_model_->ideal_bounds(last_visible_index_).y(),
          view_model_->ideal_bounds(last_visible_index_).bottom());
    }

    if (last_visible_index_ >= 0) {
      // Add more space between last visible item and overflow button.
      // Without this, two buttons look too close compared with other items.
      x = shelf_->PrimaryAxisValue(x + kShelfButtonSpacing, x);
      y = shelf_->PrimaryAxisValue(y, y + kShelfButtonSpacing);
    }

    // Set all hidden panel icon positions to be on the overflow button.
    for (int i = first_panel_index; i <= last_hidden_index_; ++i)
      view_model_->set_ideal_bounds(i, gfx::Rect(x, y, w, h));

    overflow_bounds->set_x(x);
    overflow_bounds->set_y(y);
    if (overflow_bubble_.get() && overflow_bubble_->IsShowing())
      UpdateOverflowRange(overflow_bubble_->shelf_view());
  } else {
    if (overflow_bubble_)
      overflow_bubble_->Hide();
  }
}

int ShelfView::DetermineLastVisibleIndex(int max_value) const {
  int index = model_->FirstPanelIndex() - 1;
  while (index >= 0 &&
         shelf_->PrimaryAxisValue(view_model_->ideal_bounds(index).right(),
                                  view_model_->ideal_bounds(index).bottom()) >
             max_value) {
    index--;
  }
  return index;
}

int ShelfView::DetermineFirstVisiblePanelIndex(int min_value) const {
  int index = model_->FirstPanelIndex();
  while (index < view_model_->view_size() &&
         shelf_->PrimaryAxisValue(view_model_->ideal_bounds(index).x(),
                                  view_model_->ideal_bounds(index).y()) <
             min_value) {
    ++index;
  }
  return index;
}

void ShelfView::AnimateToIdealBounds() {
  gfx::Rect overflow_bounds;
  CalculateIdealBounds(&overflow_bounds);
  for (int i = 0; i < view_model_->view_size(); ++i) {
    View* view = view_model_->view_at(i);
    bounds_animator_->AnimateViewTo(view, view_model_->ideal_bounds(i));
    // Now that the item animation starts, we have to make sure that the
    // padding of the first gets properly transferred to the new first item.
    if (i && view->border())
      view->SetBorder(views::NullBorder());
  }
  overflow_button_->SetBoundsRect(overflow_bounds);
}

views::View* ShelfView::CreateViewForItem(const ShelfItem& item) {
  views::View* view = nullptr;
  switch (item.type) {
    case TYPE_APP_PANEL:
    case TYPE_PINNED_APP:
    case TYPE_BROWSER_SHORTCUT:
    case TYPE_APP:
    case TYPE_DIALOG: {
      ShelfButton* button = new ShelfButton(this, this);
      button->SetImage(item.image);
      ReflectItemStatus(item, button);
      view = button;
      break;
    }

    case TYPE_APP_LIST: {
      view = new AppListButton(this, this, shelf_);
      break;
    }

    case TYPE_BACK_BUTTON: {
      view = new BackButton();
      break;
    }

    case TYPE_UNDEFINED:
      return nullptr;
  }

  view->set_context_menu_controller(this);
  ConfigureChildView(view);
  return view;
}

void ShelfView::FadeIn(views::View* view) {
  view->SetVisible(true);
  view->layer()->SetOpacity(0);
  AnimateToIdealBounds();
  bounds_animator_->SetAnimationDelegate(
      view, std::unique_ptr<gfx::AnimationDelegate>(
                new FadeInAnimationDelegate(view)));
}

void ShelfView::PrepareForDrag(Pointer pointer, const ui::LocatedEvent& event) {
  DCHECK(!dragging());
  DCHECK(drag_view_);
  drag_pointer_ = pointer;
  start_drag_index_ = view_model_->GetIndexOfView(drag_view_);

  if (start_drag_index_ == -1) {
    CancelDrag(-1);
    return;
  }

  // Move the view to the front so that it appears on top of other views.
  ReorderChildView(drag_view_, -1);
  bounds_animator_->StopAnimatingView(drag_view_);

  drag_view_->OnDragStarted(&event);
}

void ShelfView::ContinueDrag(const ui::LocatedEvent& event) {
  DCHECK(dragging());
  DCHECK(drag_view_);
  // Due to a syncing operation the application might have been removed.
  // Bail if it is gone.
  int current_index = view_model_->GetIndexOfView(drag_view_);
  DCHECK_NE(-1, current_index);

  // If this is not a drag and drop host operation and not the app list item,
  // check if the item got ripped off the shelf - if it did we are done.
  if (drag_and_drop_shelf_id_.IsNull() &&
      RemovableByRipOff(current_index) != NOT_REMOVABLE) {
    if (HandleRipOffDrag(event))
      return;
    // The rip off handler could have changed the location of the item.
    current_index = view_model_->GetIndexOfView(drag_view_);
  }

  // TODO: I don't think this works correctly with RTL.
  gfx::Point drag_point(event.location());
  ConvertPointToTarget(drag_view_, this, &drag_point);

  // Constrain the location to the range of valid indices for the type.
  std::pair<int, int> indices(GetDragRange(current_index));
  int first_drag_index = indices.first;
  int last_drag_index = indices.second;
  // If the last index isn't valid, we're overflowing. Constrain to the app list
  // (which is the last visible item).
  if (first_drag_index < model_->FirstPanelIndex() &&
      last_drag_index > last_visible_index_)
    last_drag_index = last_visible_index_;
  int x = 0, y = 0;
  if (shelf_->IsHorizontalAlignment()) {
    x = std::max(view_model_->ideal_bounds(indices.first).x(),
                 drag_point.x() - drag_origin_.x());
    x = std::min(view_model_->ideal_bounds(last_drag_index).right() -
                     view_model_->ideal_bounds(current_index).width(),
                 x);
    if (drag_view_->x() == x)
      return;
    drag_view_->SetX(x);
  } else {
    y = std::max(view_model_->ideal_bounds(indices.first).y(),
                 drag_point.y() - drag_origin_.y());
    y = std::min(view_model_->ideal_bounds(last_drag_index).bottom() -
                     view_model_->ideal_bounds(current_index).height(),
                 y);
    if (drag_view_->y() == y)
      return;
    drag_view_->SetY(y);
  }

  int target_index = views::ViewModelUtils::DetermineMoveIndex(
      *view_model_, drag_view_,
      shelf_->IsHorizontalAlignment() ? views::ViewModelUtils::HORIZONTAL
                                      : views::ViewModelUtils::VERTICAL,
      x, y);
  target_index =
      std::min(indices.second, std::max(target_index, indices.first));

  // The back button and app list button are always first, and they are the
  // only non-draggable items.
  int first_draggable_item = model_->GetItemIndexForType(TYPE_APP_LIST) + 1;
  DCHECK_EQ(2, first_draggable_item);
  target_index = std::max(target_index, first_draggable_item);
  DCHECK_LT(target_index, model_->item_count());

  if (target_index == current_index)
    return;

  // Change the model, the ShelfItemMoved() callback will handle the
  // |view_model_| update.
  model_->Move(current_index, target_index);
  bounds_animator_->StopAnimatingView(drag_view_);
}

void ShelfView::EndDragOnOtherShelf(bool cancel) {
  if (is_overflow_mode()) {
    main_shelf_->EndDrag(cancel);
  } else {
    DCHECK(overflow_bubble_->IsShowing());
    overflow_bubble_->shelf_view()->EndDrag(cancel);
  }
}

bool ShelfView::HandleRipOffDrag(const ui::LocatedEvent& event) {
  int current_index = view_model_->GetIndexOfView(drag_view_);
  DCHECK_NE(-1, current_index);
  std::string dragged_app_id = model_->items()[current_index].id.app_id;

  gfx::Point screen_location = event.root_location();
  ::wm::ConvertPointToScreen(GetWidget()->GetNativeWindow()->GetRootWindow(),
                             &screen_location);

  // To avoid ugly forwards and backwards flipping we use different constants
  // for ripping off / re-inserting the items.
  if (dragged_off_shelf_) {
    // If the shelf/overflow bubble bounds contains |screen_location| we insert
    // the item back into the shelf.
    if (GetBoundsForDragInsertInScreen().Contains(screen_location)) {
      if (dragged_to_another_shelf_) {
        // During the dragging an item from Shelf to Overflow, it can enter here
        // directly because both are located very closely.
        EndDragOnOtherShelf(true /* cancel */);

        // Stops the animation of |drag_view_| and sets its bounds explicitly
        // because ContinueDrag() stops its animation. Without this, unexpected
        // bounds will be set.
        bounds_animator_->StopAnimatingView(drag_view_);
        int drag_view_index = view_model_->GetIndexOfView(drag_view_);
        drag_view_->SetBoundsRect(view_model_->ideal_bounds(drag_view_index));
        dragged_to_another_shelf_ = false;
      }
      // Destroy our proxy view item.
      DestroyDragIconProxy();
      // Re-insert the item and return simply false since the caller will handle
      // the move as in any normal case.
      dragged_off_shelf_ = false;
      drag_view_->layer()->SetOpacity(1.0f);
      // The size of Overflow bubble should be updated immediately when an item
      // is re-inserted.
      if (is_overflow_mode())
        PreferredSizeChanged();
      return false;
    } else if (is_overflow_mode() &&
               main_shelf_->GetBoundsForDragInsertInScreen().Contains(
                   screen_location)) {
      // The item was dragged from the overflow shelf to the main shelf.
      if (!dragged_to_another_shelf_) {
        dragged_to_another_shelf_ = true;
        drag_image_->SetOpacity(1.0f);
        main_shelf_->StartDrag(dragged_app_id, screen_location);
      } else {
        main_shelf_->Drag(screen_location);
      }
    } else if (!is_overflow_mode() && overflow_bubble_ &&
               overflow_bubble_->IsShowing() &&
               overflow_bubble_->shelf_view()
                   ->GetBoundsForDragInsertInScreen()
                   .Contains(screen_location)) {
      // The item was dragged from the main shelf to the overflow shelf.
      if (!dragged_to_another_shelf_) {
        dragged_to_another_shelf_ = true;
        drag_image_->SetOpacity(1.0f);
        overflow_bubble_->shelf_view()->StartDrag(dragged_app_id,
                                                  screen_location);
      } else {
        overflow_bubble_->shelf_view()->Drag(screen_location);
      }
    } else if (dragged_to_another_shelf_) {
      // Makes the |drag_image_| partially disappear again.
      dragged_to_another_shelf_ = false;
      drag_image_->SetOpacity(kDraggedImageOpacity);

      EndDragOnOtherShelf(true /* cancel */);
      if (!is_overflow_mode()) {
        // During dragging, the position of the dragged item is moved to the
        // back. If the overflow bubble is showing, a copy of the dragged item
        // will appear at the end of the overflow shelf. Decrement the last
        // visible index of the overflow shelf to hide this copy.
        overflow_bubble_->shelf_view()->last_visible_index_--;
      }

      bounds_animator_->StopAnimatingView(drag_view_);
      int drag_view_index = view_model_->GetIndexOfView(drag_view_);
      drag_view_->SetBoundsRect(view_model_->ideal_bounds(drag_view_index));
    }
    // Move our proxy view item.
    UpdateDragIconProxy(screen_location);
    return true;
  }

  // Mark the item as dragged off the shelf if the drag distance exceeds
  // |kRipOffDistance|, or if it's dragged between the main and overflow shelf.
  int delta = CalculateShelfDistance(screen_location);
  bool dragged_off_shelf = delta > kRipOffDistance;
  dragged_off_shelf |=
      (is_overflow_mode() &&
       main_shelf_->GetBoundsForDragInsertInScreen().Contains(screen_location));
  dragged_off_shelf |= (!is_overflow_mode() && overflow_bubble_ &&
                        overflow_bubble_->IsShowing() &&
                        overflow_bubble_->shelf_view()
                            ->GetBoundsForDragInsertInScreen()
                            .Contains(screen_location));

  if (dragged_off_shelf) {
    // Create a proxy view item which can be moved anywhere.
    CreateDragIconProxy(event.root_location(), drag_view_->GetImage(),
                        drag_view_, gfx::Vector2d(0, 0),
                        kDragAndDropProxyScale);
    drag_view_->layer()->SetOpacity(0.0f);
    dragged_off_shelf_ = true;
    if (RemovableByRipOff(current_index) == REMOVABLE) {
      // Move the item to the front of the first panel item and hide it.
      // ShelfItemMoved() callback will handle the |view_model_| update and
      // call AnimateToIdealBounds().
      if (current_index != model_->FirstPanelIndex() - 1) {
        model_->Move(current_index, model_->FirstPanelIndex() - 1);
        StartFadeInLastVisibleItem();

        // During dragging, the position of the dragged item is moved to the
        // back. If the overflow bubble is showing, a copy of the dragged item
        // will appear at the end of the overflow shelf. Decrement the last
        // visible index of the overflow shelf to hide this copy.
        if (overflow_bubble_ && overflow_bubble_->IsShowing())
          overflow_bubble_->shelf_view()->last_visible_index_--;
      } else if (is_overflow_mode()) {
        // Overflow bubble should be shrunk when an item is ripped off.
        PreferredSizeChanged();
      }
      // Make the item partially disappear to show that it will get removed if
      // dropped.
      drag_image_->SetOpacity(kDraggedImageOpacity);
    }
    return true;
  }
  return false;
}

void ShelfView::FinalizeRipOffDrag(bool cancel) {
  if (!dragged_off_shelf_)
    return;
  // Make sure we do not come in here again.
  dragged_off_shelf_ = false;

  // Coming here we should always have a |drag_view_|.
  DCHECK(drag_view_);
  int current_index = view_model_->GetIndexOfView(drag_view_);
  // If the view isn't part of the model anymore (|current_index| == -1), a sync
  // operation must have removed it. In that case we shouldn't change the model
  // and only delete the proxy image.
  if (current_index == -1) {
    DestroyDragIconProxy();
    return;
  }

  // Set to true when the animation should snap back to where it was before.
  bool snap_back = false;
  // Items which cannot be dragged off will be handled as a cancel.
  if (!cancel) {
    if (dragged_to_another_shelf_) {
      dragged_to_another_shelf_ = false;
      EndDragOnOtherShelf(false /* cancel */);
      drag_view_->layer()->SetOpacity(1.0f);
    } else if (RemovableByRipOff(current_index) != REMOVABLE) {
      // Make sure we do not try to remove un-removable items like items which
      // were not pinned or have to be always there.
      cancel = true;
      snap_back = true;
    } else {
      // Make sure the item stays invisible upon removal.
      drag_view_->SetVisible(false);
      model_->UnpinAppWithID(model_->items()[current_index].id.app_id);
    }
  }
  if (cancel || snap_back) {
    if (dragged_to_another_shelf_) {
      dragged_to_another_shelf_ = false;
      // Other shelf handles revert of dragged item.
      EndDragOnOtherShelf(false /* true */);
      drag_view_->layer()->SetOpacity(1.0f);
    } else if (!cancelling_drag_model_changed_) {
      // Only do something if the change did not come through a model change.
      gfx::Rect drag_bounds = drag_image_->GetBoundsInScreen();
      gfx::Point relative_to = GetBoundsInScreen().origin();
      gfx::Rect target(
          gfx::PointAtOffsetFromOrigin(drag_bounds.origin() - relative_to),
          drag_bounds.size());
      drag_view_->SetBoundsRect(target);
      // Hide the status from the active item since we snap it back now. Upon
      // animation end the flag gets cleared if |snap_back_from_rip_off_view_|
      // is set.
      snap_back_from_rip_off_view_ = drag_view_;
      drag_view_->AddState(ShelfButton::STATE_HIDDEN);
      // When a canceling drag model is happening, the view model is diverged
      // from the menu model and movements / animations should not be done.
      model_->Move(current_index, start_drag_index_);
      AnimateToIdealBounds();
    }
    drag_view_->layer()->SetOpacity(1.0f);
  }
  DestroyDragIconProxy();
}

ShelfView::RemovableState ShelfView::RemovableByRipOff(int index) const {
  DCHECK(index >= 0 && index < model_->item_count());
  ShelfItemType type = model_->items()[index].type;
  if (type == TYPE_APP_LIST || type == TYPE_DIALOG || type == TYPE_BACK_BUTTON)
    return NOT_REMOVABLE;

  if (model_->items()[index].pinned_by_policy)
    return NOT_REMOVABLE;

  // Note: Only pinned app shortcuts can be removed!
  const std::string& app_id = model_->items()[index].id.app_id;
  return (type == TYPE_PINNED_APP && model_->IsAppPinned(app_id)) ? REMOVABLE
                                                                  : DRAGGABLE;
}

bool ShelfView::SameDragType(ShelfItemType typea, ShelfItemType typeb) const {
  switch (typea) {
    case TYPE_PINNED_APP:
    case TYPE_BROWSER_SHORTCUT:
      return (typeb == TYPE_PINNED_APP || typeb == TYPE_BROWSER_SHORTCUT);
    case TYPE_APP_PANEL:
    case TYPE_APP_LIST:
    case TYPE_APP:
    case TYPE_BACK_BUTTON:
    case TYPE_DIALOG:
      return typeb == typea;
    case TYPE_UNDEFINED:
      NOTREACHED() << "ShelfItemType must be set.";
      return false;
  }
  NOTREACHED();
  return false;
}

std::pair<int, int> ShelfView::GetDragRange(int index) {
  int min_index = -1;
  int max_index = -1;
  ShelfItemType type = model_->items()[index].type;
  for (int i = 0; i < model_->item_count(); ++i) {
    if (SameDragType(model_->items()[i].type, type)) {
      if (min_index == -1)
        min_index = i;
      max_index = i;
    }
  }
  return std::pair<int, int>(min_index, max_index);
}

void ShelfView::ConfigureChildView(views::View* view) {
  view->SetPaintToLayer();
  view->layer()->SetFillsBoundsOpaquely(false);
}

void ShelfView::ToggleOverflowBubble() {
  if (IsShowingOverflowBubble()) {
    overflow_bubble_->Hide();
    return;
  }

  if (!overflow_bubble_)
    overflow_bubble_.reset(new OverflowBubble(shelf_));

  ShelfView* overflow_view = new ShelfView(model_, shelf_, shelf_widget_);
  overflow_view->overflow_mode_ = true;
  overflow_view->Init();
  overflow_view->set_owner_overflow_bubble(overflow_bubble_.get());
  overflow_view->OnShelfAlignmentChanged();
  overflow_view->main_shelf_ = this;
  UpdateOverflowRange(overflow_view);

  overflow_bubble_->Show(overflow_button_, overflow_view);

  shelf_->UpdateVisibilityState();
}

void ShelfView::OnFadeOutAnimationEnded() {
  AnimateToIdealBounds();
  StartFadeInLastVisibleItem();
}

void ShelfView::StartFadeInLastVisibleItem() {
  // If overflow button is visible and there is a valid new last item, fading
  // the new last item in after sliding animation is finished.
  if (overflow_button_->visible() && last_visible_index_ >= 0) {
    views::View* last_visible_view = view_model_->view_at(last_visible_index_);
    last_visible_view->layer()->SetOpacity(0);
    bounds_animator_->SetAnimationDelegate(
        last_visible_view,
        std::unique_ptr<gfx::AnimationDelegate>(
            new StartFadeAnimationDelegate(this, last_visible_view)));
  }
}

void ShelfView::UpdateOverflowRange(ShelfView* overflow_view) const {
  const int first_overflow_index = last_visible_index_ + 1;
  const int last_overflow_index = last_hidden_index_;
  DCHECK_LE(first_overflow_index, last_overflow_index);
  DCHECK_LT(last_overflow_index, view_model_->view_size());

  overflow_view->first_visible_index_ = first_overflow_index;
  overflow_view->last_visible_index_ = last_overflow_index;
}

gfx::Rect ShelfView::GetMenuAnchorRect(const views::View* source,
                                       const gfx::Point& location,
                                       ui::MenuSourceType source_type,
                                       bool context_menu) const {
  if (context_menu) {
    if (source_type == ui::MenuSourceType::MENU_SOURCE_TOUCH)
      return GetTouchMenuAnchorRect(source, location);
    return gfx::Rect(location, gfx::Size());
  }

  // The menu is for an application list.
  DCHECK(source) << "Application lists require a source button view.";
  // Application lists use a bubble. It is possible to invoke the menu while
  // it is sliding into view. To cover that case, the screen coordinates are
  // offsetted by the animation delta.
  aura::Window* window = GetWidget()->GetNativeWindow();
  gfx::Rect anchor =
      source->GetBoundsInScreen() +
      (window->GetTargetBounds().origin() - window->bounds().origin());
  if (source->border())
    anchor.Inset(source->border()->GetInsets());
  return anchor;
}

gfx::Rect ShelfView::GetTouchMenuAnchorRect(const views::View* source,
                                            const gfx::Point& location) const {
  const bool for_item = ShelfItemForView(source);
  const bool use_touchable_menu_alignment =
      features::IsTouchableAppContextMenuEnabled() && for_item;
  const gfx::Rect shelf_bounds =
      is_overflow_mode()
          ? owner_overflow_bubble_->bubble_view()->GetBubbleBounds()
          : screen_util::GetDisplayBoundsWithShelf(
                shelf_widget_->GetNativeWindow());
  const gfx::Rect& source_bounds_in_screen = source->GetBoundsInScreen();
  gfx::Point origin;
  switch (shelf_->alignment()) {
    case SHELF_ALIGNMENT_BOTTOM:
    case SHELF_ALIGNMENT_BOTTOM_LOCKED:
      origin =
          gfx::Point(use_touchable_menu_alignment ? source_bounds_in_screen.x()
                                                  : location.x(),
                     shelf_bounds.bottom() - kShelfSize);
      break;
    case SHELF_ALIGNMENT_LEFT:
      if (use_touchable_menu_alignment)
        origin = gfx::Point(shelf_bounds.x(), source_bounds_in_screen.y());
      else
        origin = gfx::Point(shelf_bounds.x() + kShelfSize, location.y());
      break;
    case SHELF_ALIGNMENT_RIGHT:
      origin =
          gfx::Point(shelf_bounds.right() - kShelfSize,
                     use_touchable_menu_alignment ? source_bounds_in_screen.y()
                                                  : location.y());
      break;
  }
  return gfx::Rect(origin,
                   for_item ? source_bounds_in_screen.size() : gfx::Size());
}

views::MenuAnchorPosition ShelfView::GetMenuAnchorPosition(
    bool for_item,
    bool context_menu) const {
  if (features::IsTouchableAppContextMenuEnabled() && for_item) {
    return shelf_->IsHorizontalAlignment()
               ? views::MENU_ANCHOR_BUBBLE_TOUCHABLE_ABOVE
               : views::MENU_ANCHOR_BUBBLE_TOUCHABLE_LEFT;
  }
  if (!context_menu) {
    switch (shelf_->alignment()) {
      case SHELF_ALIGNMENT_BOTTOM:
      case SHELF_ALIGNMENT_BOTTOM_LOCKED:
        return views::MENU_ANCHOR_BUBBLE_ABOVE;
      case SHELF_ALIGNMENT_LEFT:
        return views::MENU_ANCHOR_BUBBLE_RIGHT;
      case SHELF_ALIGNMENT_RIGHT:
        return views::MENU_ANCHOR_BUBBLE_LEFT;
    }
  }
  return shelf_->IsHorizontalAlignment() ? views::MENU_ANCHOR_FIXED_BOTTOMCENTER
                                         : views::MENU_ANCHOR_FIXED_SIDECENTER;
}

gfx::Rect ShelfView::GetBoundsForDragInsertInScreen() {
  gfx::Size preferred_size;
  if (is_overflow_mode()) {
    DCHECK(owner_overflow_bubble_);
    gfx::Rect bubble_bounds =
        owner_overflow_bubble_->bubble_view()->GetBubbleBounds();
    preferred_size = bubble_bounds.size();
  } else {
    const int last_button_index = view_model_->view_size() - 1;
    gfx::Rect last_button_bounds =
        view_model_->view_at(last_button_index)->bounds();
    if (overflow_button_->visible() &&
        model_->GetItemIndexForType(TYPE_APP_PANEL) == -1) {
      // When overflow button is visible and shelf has no panel items,
      // last_button_bounds should be overflow button's bounds.
      last_button_bounds = overflow_button_->bounds();
    }

    if (shelf_->IsHorizontalAlignment()) {
      preferred_size = gfx::Size(last_button_bounds.right(), kShelfSize);
    } else {
      preferred_size = gfx::Size(kShelfSize, last_button_bounds.bottom());
    }
  }
  gfx::Point origin(GetMirroredXWithWidthInView(0, preferred_size.width()), 0);

  // In overflow mode, we should use OverflowBubbleView as a source for
  // converting |origin| to screen coordinates. When a scroll operation is
  // occurred in OverflowBubble, the bounds of ShelfView in OverflowBubble can
  // be changed.
  if (is_overflow_mode())
    ConvertPointToScreen(owner_overflow_bubble_->bubble_view(), &origin);
  else
    ConvertPointToScreen(this, &origin);

  return gfx::Rect(origin, preferred_size);
}

int ShelfView::CancelDrag(int modified_index) {
  FinalizeRipOffDrag(true);
  if (!drag_view_)
    return modified_index;
  bool was_dragging = dragging();
  int drag_view_index = view_model_->GetIndexOfView(drag_view_);
  drag_pointer_ = NONE;
  drag_view_ = nullptr;
  if (drag_view_index == modified_index) {
    // The view that was being dragged is being modified. Don't do anything.
    return modified_index;
  }
  if (!was_dragging)
    return modified_index;

  // Restore previous position, tracking the position of the modified view.
  bool at_end = modified_index == view_model_->view_size();
  views::View* modified_view = (modified_index >= 0 && !at_end)
                                   ? view_model_->view_at(modified_index)
                                   : nullptr;
  model_->Move(drag_view_index, start_drag_index_);

  // If the modified view will be at the end of the list, return the new end of
  // the list.
  if (at_end)
    return view_model_->view_size();
  return modified_view ? view_model_->GetIndexOfView(modified_view) : -1;
}

gfx::Size ShelfView::CalculatePreferredSize() const {
  gfx::Rect overflow_bounds;
  CalculateIdealBounds(&overflow_bounds);

  int last_button_index = last_visible_index_;
  if (!is_overflow_mode()) {
    if (last_hidden_index_ < view_model_->view_size() - 1)
      last_button_index = view_model_->view_size() - 1;
    else if (overflow_button_ && overflow_button_->visible())
      last_button_index++;
  }

  // When an item is dragged off from the overflow bubble, it is moved to last
  // position and and changed to invisible. Overflow bubble size should be
  // shrunk to fit only for visible items.
  // If |dragged_to_another_shelf_| is set, there will be no
  // invisible items in the shelf.
  if (is_overflow_mode() && dragged_off_shelf_ && !dragged_to_another_shelf_ &&
      RemovableByRipOff(view_model_->GetIndexOfView(drag_view_)) == REMOVABLE)
    last_button_index--;

  const gfx::Rect last_button_bounds =
      last_button_index >= first_visible_index_
          ? view_model_->ideal_bounds(last_button_index)
          : gfx::Rect(gfx::Size(kShelfSize, kShelfSize));

  if (shelf_->IsHorizontalAlignment())
    return gfx::Size(last_button_bounds.right(), kShelfSize);

  return gfx::Size(kShelfSize, last_button_bounds.bottom());
}

void ShelfView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  // This bounds change is produced by the shelf movement (rotation, alignment
  // change, etc.) and all content has to follow. Using an animation at that
  // time would produce a time lag since the animation of the BoundsAnimator has
  // itself a delay before it arrives at the required location. As such we tell
  // the animator to go there immediately. We still want to use an animation
  // when the bounds change is caused by entering or exiting tablet mode.
  if (shelf_->is_tablet_mode_animation_running()) {
    AnimateToIdealBounds();
    return;
  }

  BoundsAnimatorDisabler disabler(bounds_animator_.get());

  LayoutToIdealBounds();
  shelf_->NotifyShelfIconPositionsChanged();

  if (IsShowingOverflowBubble())
    overflow_bubble_->Hide();
}

void ShelfView::OnPaint(gfx::Canvas* canvas) {
  if (overflow_mode_)
    return;

  cc::PaintFlags flags;
  flags.setColor(shelf_item_background_color_);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  // Draws a round rect around the back button and app list button. This will
  // just be a circle if the back button is hidden.
  const gfx::PointF circle_center(
      GetMirroredRect(GetAppListButton()->bounds()).origin() +
      gfx::Vector2d(GetAppListButton()->GetCenterPoint().x(),
                    GetAppListButton()->GetCenterPoint().y()));
  if (GetBackButton()->layer()->opacity() <= 0.f) {
    canvas->DrawCircle(circle_center, kAppListButtonRadius, flags);
    return;
  }

  const gfx::PointF back_center(
      GetMirroredRect(GetBackButton()->bounds()).x() + kShelfButtonSize / 2,
      GetBackButton()->bounds().y() + kShelfButtonSize / 2);
  const gfx::RectF background_bounds(
      std::min(back_center.x(), circle_center.x()) - kAppListButtonRadius,
      back_center.y() - kAppListButtonRadius,
      std::abs(circle_center.x() - back_center.x()) + 2 * kAppListButtonRadius,
      2 * kAppListButtonRadius);

  canvas->DrawRoundRect(background_bounds, kAppListButtonRadius, flags);
}

views::FocusTraversable* ShelfView::GetPaneFocusTraversable() {
  return this;
}

void ShelfView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kToolbar;
  node_data->SetName(l10n_util::GetStringUTF8(IDS_ASH_SHELF_ACCESSIBLE_NAME));
}

void ShelfView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this)
    tooltip_.Init();
}

void ShelfView::OnGestureEvent(ui::GestureEvent* event) {
  // Convert the event location from current view to screen, since swiping up on
  // the shelf can open the fullscreen app list. Updating the bounds of the app
  // list during dragging is based on screen coordinate space.
  gfx::Point location_in_screen(event->location());
  View::ConvertPointToScreen(this, &location_in_screen);
  event->set_location(location_in_screen);
  if (shelf_->ProcessGestureEvent(*event))
    event->StopPropagation();
}

bool ShelfView::OnMouseWheel(const ui::MouseWheelEvent& event) {
  shelf_->ProcessMouseWheelEvent(event);
  return true;
}

void ShelfView::ShelfItemAdded(int model_index) {
  {
    base::AutoReset<bool> cancelling_drag(&cancelling_drag_model_changed_,
                                          true);
    model_index = CancelDrag(model_index);
  }
  const ShelfItem& item(model_->items()[model_index]);
  views::View* view = CreateViewForItem(item);
  AddChildView(view);
  // Hide the view, it'll be made visible when the animation is done. Using
  // opacity 0 here to avoid messing with CalculateIdealBounds which touches
  // the view's visibility.
  view->layer()->SetOpacity(0);
  view_model_->Add(view, model_index);

  // Give the button its ideal bounds. That way if we end up animating the
  // button before this animation completes it doesn't appear at some random
  // spot (because it was in the middle of animating from 0,0 0x0 to its
  // target).
  gfx::Rect overflow_bounds;
  CalculateIdealBounds(&overflow_bounds);
  view->SetBoundsRect(view_model_->ideal_bounds(model_index));

  if (ShouldShowShelfItem(item)) {
    // The first animation moves all the views to their target position. |view|
    // is hidden, so it visually appears as though we are providing space for
    // it. When done we'll fade the view in.
    AnimateToIdealBounds();
    if (model_index <= last_visible_index_ ||
        model_index >= model_->FirstPanelIndex()) {
      bounds_animator_->SetAnimationDelegate(
          view, std::unique_ptr<gfx::AnimationDelegate>(
                    new StartFadeAnimationDelegate(this, view)));
    } else {
      // Undo the hiding if animation does not run.
      view->layer()->SetOpacity(1.0f);
    }
  } else {
    view->SetVisible(false);
  }
}

void ShelfView::ShelfItemRemoved(int model_index, const ShelfItem& old_item) {
  if (old_item.id == context_menu_id_)
    launcher_menu_runner_->Cancel();

  views::View* view = view_model_->view_at(model_index);
  view_model_->Remove(model_index);

  {
    base::AutoReset<bool> cancelling_drag(&cancelling_drag_model_changed_,
                                          true);
    CancelDrag(-1);
  }

  // When the overflow bubble is visible, the overflow range needs to be set
  // before CalculateIdealBounds() gets called. Otherwise CalculateIdealBounds()
  // could trigger a ShelfItemChanged() by hiding the overflow bubble and
  // since the overflow bubble is not yet synced with the ShelfModel this
  // could cause a crash.
  if (overflow_bubble_ && overflow_bubble_->IsShowing()) {
    last_hidden_index_ =
        std::min(last_hidden_index_, view_model_->view_size() - 1);
    UpdateOverflowRange(overflow_bubble_->shelf_view());
  }

  if (view->visible()) {
    // The first animation fades out the view. When done we'll animate the rest
    // of the views to their target location.
    bounds_animator_->AnimateViewTo(view, view->bounds());
    bounds_animator_->SetAnimationDelegate(
        view, std::unique_ptr<gfx::AnimationDelegate>(
                  new FadeOutAnimationDelegate(this, view)));
  } else {
    // We don't need to show a fade out animation for invisible |view|. When an
    // item is ripped out from the shelf, its |view| is already invisible.
    AnimateToIdealBounds();
  }

  if (view == tooltip_.GetCurrentAnchorView())
    tooltip_.Close();
}

void ShelfView::ShelfItemChanged(int model_index, const ShelfItem& old_item) {
  const ShelfItem& item(model_->items()[model_index]);

  // Bail if the view and shelf sizes do not match. ShelfItemChanged may be
  // called here before ShelfItemAdded, due to ChromeLauncherController's
  // item initialization, which calls SetItem during ShelfItemAdded.
  if (static_cast<int>(model_->items().size()) != view_model_->view_size())
    return;

  if (old_item.type != item.type) {
    // Type changed, swap the views.
    model_index = CancelDrag(model_index);
    std::unique_ptr<views::View> old_view(view_model_->view_at(model_index));
    bounds_animator_->StopAnimatingView(old_view.get());
    // Removing and re-inserting a view in our view model will strip the ideal
    // bounds from the item. To avoid recalculation of everything the bounds
    // get remembered and restored after the insertion to the previous value.
    gfx::Rect old_ideal_bounds = view_model_->ideal_bounds(model_index);
    view_model_->Remove(model_index);
    views::View* new_view = CreateViewForItem(item);
    AddChildView(new_view);
    view_model_->Add(new_view, model_index);
    view_model_->set_ideal_bounds(model_index, old_ideal_bounds);

    bool new_item_is_visible = ShouldShowShelfItem(item);
    if (ShouldShowShelfItem(old_item) != new_item_is_visible) {
      views::View* view = view_model_->view_at(model_index);
      view->SetVisible(new_item_is_visible);
      if (!new_item_is_visible) {
        // Nothing else to do.
        return;
      }
    }

    new_view->SetBoundsRect(old_view->bounds());
    if (overflow_button_ && overflow_button_->visible())
      AnimateToIdealBounds();
    else
      bounds_animator_->AnimateViewTo(new_view, old_ideal_bounds);
    return;
  }

  views::View* view = view_model_->view_at(model_index);
  switch (item.type) {
    case TYPE_APP_PANEL:
    case TYPE_PINNED_APP:
    case TYPE_BROWSER_SHORTCUT:
    case TYPE_APP:
    case TYPE_DIALOG: {
      CHECK_EQ(ShelfButton::kViewClassName, view->GetClassName());
      ShelfButton* button = static_cast<ShelfButton*>(view);
      ReflectItemStatus(item, button);
      button->SetImage(item.image);
      button->SchedulePaint();
      break;
    }

    default:
      break;
  }
}

void ShelfView::ShelfItemMoved(int start_index, int target_index) {
  view_model_->Move(start_index, target_index);
  // When cancelling a drag due to a shelf item being added, the currently
  // dragged item is moved back to its initial position. AnimateToIdealBounds
  // will be called again when the new item is added to the |view_model_| but
  // at this time the |view_model_| is inconsistent with the |model_|.
  if (!cancelling_drag_model_changed_)
    AnimateToIdealBounds();
}

void ShelfView::ShelfItemDelegateChanged(const ShelfID& id,
                                         ShelfItemDelegate* old_delegate,
                                         ShelfItemDelegate* delegate) {}

void ShelfView::AfterItemSelected(
    const ShelfItem& item,
    views::Button* sender,
    std::unique_ptr<ui::Event> event,
    views::InkDrop* ink_drop,
    ShelfAction action,
    base::Optional<std::vector<mojom::MenuItemPtr>> menu_items) {
  shelf_button_pressed_metric_tracker_.ButtonPressed(*event, sender, action);

  // The app list handles its own ink drop effect state changes.
  if (action != SHELF_ACTION_APP_LIST_SHOWN) {
    if (action != SHELF_ACTION_NEW_WINDOW_CREATED && menu_items &&
        menu_items->size() > 1) {
      // Show the app menu with 2 or more items, if no window was created.
      ink_drop->AnimateToState(views::InkDropState::ACTIVATED);
      context_menu_id_ = item.id;
      ShowMenu(std::make_unique<ShelfApplicationMenuModel>(
                   item.title, std::move(*menu_items),
                   model_->GetShelfItemDelegate(item.id)),
               sender, gfx::Point(), false,
               ui::GetMenuSourceTypeForEvent(*event));
    } else {
      ink_drop->AnimateToState(views::InkDropState::ACTION_TRIGGERED);
    }
  }
  scoped_root_window_for_new_windows_.reset();
}

void ShelfView::AfterGetContextMenuItems(
    const ShelfID& shelf_id,
    const gfx::Point& point,
    views::View* source,
    ui::MenuSourceType source_type,
    std::vector<mojom::MenuItemPtr> menu_items) {
  context_menu_id_ = shelf_id;
  const int64_t display_id = GetDisplayIdForView(this);
  std::unique_ptr<ShelfContextMenuModel> menu_model =
      std::make_unique<ShelfContextMenuModel>(
          std::move(menu_items), model_->GetShelfItemDelegate(shelf_id),
          display_id);
  menu_model->set_histogram_name(ShelfItemForView(source)
                                     ? kAppContextMenuExecuteCommand
                                     : kNonAppContextMenuExecuteCommand);
  ShowMenu(std::move(menu_model), source, point, true /* context_menu */,
           source_type);
}

void ShelfView::ShowContextMenuForView(views::View* source,
                                       const gfx::Point& point,
                                       ui::MenuSourceType source_type) {
  last_pressed_index_ = -1;
  const ShelfItem* item = ShelfItemForView(source);
  const int64_t display_id = GetDisplayIdForView(this);
  if (!item || !model_->GetShelfItemDelegate(item->id)) {
    UMA_HISTOGRAM_ENUMERATION("Apps.ContextMenuShowSource.Shelf", source_type,
                              ui::MENU_SOURCE_TYPE_LAST);
    context_menu_id_ = ShelfID();
    std::unique_ptr<ShelfContextMenuModel> menu_model =
        std::make_unique<ShelfContextMenuModel>(
            std::vector<mojom::MenuItemPtr>(), nullptr, display_id);
    menu_model->set_histogram_name(kNonAppContextMenuExecuteCommand);
    ShowMenu(std::move(menu_model), source, point, true, source_type);
    return;
  }

  // Record the current time for the shelf button context menu user journey
  // histogram.
  shelf_button_context_menu_time_ = base::TimeTicks::Now();

  // Get any custom entries; show the context menu in AfterGetContextMenuItems.
  model_->GetShelfItemDelegate(item->id)->GetContextMenuItems(
      display_id, base::Bind(&ShelfView::AfterGetContextMenuItems,
                             weak_factory_.GetWeakPtr(), item->id, point,
                             source, source_type));
}

void ShelfView::ShowMenu(std::unique_ptr<ui::MenuModel> menu_model,
                         views::View* source,
                         const gfx::Point& click_point,
                         bool context_menu,
                         ui::MenuSourceType source_type) {
  DCHECK(!IsShowingMenu());
  if (menu_model->GetItemCount() == 0)
    return;
  menu_owner_ = source;

  menu_model_ = std::move(menu_model);
  closing_event_time_ = base::TimeTicks();
  int run_types = 0;
  if (context_menu)
    run_types |=
        views::MenuRunner::CONTEXT_MENU | views::MenuRunner::FIXED_ANCHOR;

  const ShelfItem* item = ShelfItemForView(source);
  // Only use the touchable layout if the menu is for an app.
  if (features::IsTouchableAppContextMenuEnabled() && item)
    run_types |= views::MenuRunner::USE_TOUCHABLE_LAYOUT;

  // Only selected shelf items with context menu opened can be dragged.
  if (context_menu && item && ShelfButtonIsInDrag(item->type, source) &&
      source_type == ui::MenuSourceType::MENU_SOURCE_TOUCH) {
    run_types |= views::MenuRunner::SEND_GESTURE_EVENTS_TO_OWNER;
  }

  launcher_menu_runner_ = std::make_unique<views::MenuRunner>(
      menu_model_.get(), run_types,
      base::Bind(&ShelfView::OnMenuClosed, base::Unretained(this), source));

  // NOTE: if you convert to HAS_MNEMONICS be sure to update menu building code.
  launcher_menu_runner_->RunMenuAt(
      GetWidget(), nullptr,
      GetMenuAnchorRect(source, click_point, source_type, context_menu),
      GetMenuAnchorPosition(item, context_menu), source_type);
}

void ShelfView::OnMenuClosed(views::View* source) {
  menu_owner_ = nullptr;
  context_menu_id_ = ShelfID();

  closing_event_time_ = launcher_menu_runner_->closing_event_time();

  if (shelf_button_context_menu_time_ != base::TimeTicks()) {
    // If the context menu came from a ShelfButton.
    UMA_HISTOGRAM_TIMES(
        "Apps.ContextMenuUserJourneyTime.ShelfButton",
        base::TimeTicks::Now() - shelf_button_context_menu_time_);
    shelf_button_context_menu_time_ = base::TimeTicks();
  }
  const ShelfItem* item = ShelfItemForView(source);
  if (item)
    static_cast<ShelfButton*>(source)->OnMenuClosed();

  launcher_menu_runner_.reset();
  menu_model_.reset();

  // Auto-hide or alignment might have changed, but only for this shelf.
  shelf_->UpdateVisibilityState();
}

void ShelfView::OnBoundsAnimatorProgressed(views::BoundsAnimator* animator) {
  shelf_->NotifyShelfIconPositionsChanged();
  PreferredSizeChanged();

  if (shelf_->is_tablet_mode_animation_running()) {
    float opacity = 0.f;
    const gfx::SlideAnimation* animation =
        bounds_animator_->GetAnimationForView(GetBackButton());
    if (animation)
      opacity = static_cast<float>(animation->GetCurrentValue());
    if (!IsTabletModeEnabled())
      opacity = 1.f - opacity;

    GetBackButton()->layer()->SetOpacity(opacity);
    GetBackButton()->SetFocusBehavior(FocusBehavior::ALWAYS);
  }
}

void ShelfView::OnBoundsAnimatorDone(views::BoundsAnimator* animator) {
  shelf_->set_is_tablet_mode_animation_running(false);

  if (snap_back_from_rip_off_view_ && animator == bounds_animator_.get()) {
    if (!animator->IsAnimating(snap_back_from_rip_off_view_)) {
      // Coming here the animation of the ShelfButton is finished and the
      // previously hidden status can be shown again. Since the button itself
      // might have gone away or changed locations we check that the button
      // is still in the shelf and show its status again.
      for (int index = 0; index < view_model_->view_size(); index++) {
        views::View* view = view_model_->view_at(index);
        if (view == snap_back_from_rip_off_view_) {
          CHECK_EQ(ShelfButton::kViewClassName, view->GetClassName());
          ShelfButton* button = static_cast<ShelfButton*>(view);
          button->ClearState(ShelfButton::STATE_HIDDEN);
          break;
        }
      }
      snap_back_from_rip_off_view_ = nullptr;
    }
  }

  UpdateBackButton();
}

bool ShelfView::IsRepostEvent(const ui::Event& event) {
  if (closing_event_time_.is_null())
    return false;

  // If the current (press down) event is a repost event, the time stamp of
  // these two events should be the same.
  return closing_event_time_ == event.time_stamp();
}

bool ShelfView::ShouldShowShelfItem(const ShelfItem& item) {
  // We only consider hiding shelf items in tablet mode.
  if (!IsTabletModeEnabled()) {
    return true;
  }
  // We also don't do any hiding if the relevant flag is off.
  if (!chromeos::switches::ShouldHideActiveAppsFromShelf()) {
    return true;
  }
  // Hide running apps that aren't also pinned.
  return item.type != TYPE_APP;
}

const ShelfItem* ShelfView::ShelfItemForView(const views::View* view) const {
  const int view_index = view_model_->GetIndexOfView(view);
  return (view_index < 0) ? nullptr : &(model_->items()[view_index]);
}

int ShelfView::CalculateShelfDistance(const gfx::Point& coordinate) const {
  const gfx::Rect bounds = GetBoundsInScreen();
  int distance = shelf_->SelectValueForShelfAlignment(
      bounds.y() - coordinate.y(), coordinate.x() - bounds.right(),
      bounds.x() - coordinate.x());
  return distance > 0 ? distance : 0;
}

bool ShelfView::CanPrepareForDrag(Pointer pointer,
                                  const ui::LocatedEvent& event) {
  // Bail if dragging has already begun, or if no item has been pressed.
  if (dragging() || !drag_view_)
    return false;

  // Dragging only begins once the pointer has travelled a minimum distance.
  if ((std::abs(event.x() - drag_origin_.x()) < kMinimumDragDistance) &&
      (std::abs(event.y() - drag_origin_.y()) < kMinimumDragDistance)) {
    return false;
  }

  return true;
}

void ShelfView::UpdateBackButton() {
  GetBackButton()->layer()->SetOpacity(IsTabletModeEnabled() ? 1.f : 0.f);
  GetBackButton()->SetFocusBehavior(
      IsTabletModeEnabled() ? FocusBehavior::ALWAYS : FocusBehavior::NEVER);
}

}  // namespace ash
