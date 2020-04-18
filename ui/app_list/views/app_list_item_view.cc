// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_item_view.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_item.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_features.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/animation/throb_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/transform_util.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/drag_controller.h"

namespace app_list {

namespace {

// Radius of the folder dropping preview circle.
constexpr int kFolderPreviewRadius = 40;

// Delay in milliseconds of when the dragging UI should be shown for mouse drag.
constexpr int kMouseDragUIDelayInMs = 200;

// Delay in milliseconds of when the dragging UI should be shown for touch drag.
// Note: For better user experience, this is made shorter than
// ET_GESTURE_LONG_PRESS delay, which is too long for this case, e.g., about
// 650ms.
constexpr int kTouchLongpressDelayInMs = 300;

// The color of the title for the tiles within folder.
constexpr SkColor kFolderGridTitleColor = SK_ColorBLACK;

// The color of the selected item view within folder.
constexpr SkColor kFolderGridSelectedColor = SkColorSetARGB(31, 0, 0, 0);

}  // namespace

// static
const char AppListItemView::kViewClassName[] = "ui/app_list/AppListItemView";

AppListItemView::AppListItemView(AppsGridView* apps_grid_view,
                                 AppListItem* item,
                                 AppListViewDelegate* delegate)
    : Button(apps_grid_view),
      is_folder_(item->GetItemType() == AppListFolderItem::kItemType),
      is_in_folder_(item->IsInFolder()),
      item_weak_(item),
      delegate_(delegate),
      apps_grid_view_(apps_grid_view),
      icon_(new views::ImageView),
      title_(new views::Label),
      progress_bar_(new views::ProgressBar),
      context_menu_(this),
      weak_ptr_factory_(this) {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  icon_->set_can_process_events_within_subtree(false);
  icon_->SetVerticalAlignment(views::ImageView::LEADING);

  title_->SetBackgroundColor(SK_ColorTRANSPARENT);
  title_->SetAutoColorReadabilityEnabled(false);
  title_->SetHandlesTooltips(false);
  const gfx::FontList& font = AppListAppTitleFont();
  title_->SetFontList(font);
  title_->SetLineHeight(font.GetHeight());
  title_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  title_->SetEnabledColor(apps_grid_view_->is_in_folder()
                              ? kFolderGridTitleColor
                              : kGridTitleColor);

  SetTitleSubpixelAA();

  AddChildView(icon_);
  AddChildView(title_);
  AddChildView(progress_bar_);

  SetIcon(item->icon());
  SetItemName(base::UTF8ToUTF16(item->GetDisplayName()),
              base::UTF8ToUTF16(item->name()));
  SetItemIsInstalling(item->is_installing());
  SetItemIsHighlighted(item->highlighted());
  item->AddObserver(this);

  set_context_menu_controller(this);

  SetAnimationDuration(0);
}

AppListItemView::~AppListItemView() {
  if (item_weak_)
    item_weak_->RemoveObserver(this);
}

void AppListItemView::SetIcon(const gfx::ImageSkia& icon) {
  // Clear icon and bail out if item icon is empty.
  if (icon.isNull()) {
    icon_->SetImage(NULL);
    return;
  }

  gfx::ImageSkia resized(gfx::ImageSkiaOperations::CreateResizedImage(
      icon, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(kGridIconDimension, kGridIconDimension)));
  if (shadow_animator_)
    shadow_animator_->SetOriginalImage(resized);
  else
    icon_->SetImage(resized);
}

void AppListItemView::SetUIState(UIState ui_state) {
  if (ui_state_ == ui_state)
    return;

  ui_state_ = ui_state;

  switch (ui_state_) {
    case UI_STATE_NORMAL:
      title_->SetVisible(!is_installing_);
      progress_bar_->SetVisible(is_installing_);
      ScaleAppIcon(false);
      break;
    case UI_STATE_DRAGGING:
      title_->SetVisible(false);
      progress_bar_->SetVisible(false);
      ScaleAppIcon(true);
      break;
    case UI_STATE_DROPPING_IN_FOLDER:
      break;
  }

  SetTitleSubpixelAA();
  SchedulePaint();
}

void AppListItemView::ScaleAppIcon(bool scale_up) {
  const gfx::Rect bounds(layer()->bounds().size());
  gfx::Transform transform =
      gfx::GetScaleTransform(bounds.CenterPoint(), kDragDropAppIconScale);

  ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
  settings.SetTransitionDuration(
      base::TimeDelta::FromMilliseconds((kDragDropAppIconScaleTransitionInMs)));
  if (scale_up)
    layer()->SetTransform(transform);
  else
    layer()->SetTransform(gfx::Transform());
}

void AppListItemView::SetTouchDragging(bool touch_dragging) {
  if (touch_dragging_ == touch_dragging)
    return;

  touch_dragging_ = touch_dragging;

  if (!touch_dragging)
    apps_grid_view_->EndDrag(false);

  SetState(STATE_NORMAL);
  SetUIState(touch_dragging_ ? UI_STATE_DRAGGING : UI_STATE_NORMAL);
}

void AppListItemView::SetMouseDragging(bool mouse_dragging) {
  mouse_dragging_ = mouse_dragging;

  if (!mouse_dragging_) {
    apps_grid_view_->EndDrag(false);
    mouse_drag_proxy_created_ = false;
  }

  SetState(STATE_NORMAL);
  SetUIState(mouse_dragging_ ? UI_STATE_DRAGGING : UI_STATE_NORMAL);
}

void AppListItemView::OnMouseDragTimer() {
  // Show scaled up app icon to indicate draggable state.
  SetMouseDragging(true);
}

void AppListItemView::OnTouchDragTimer(
    const gfx::Point& tap_down_location,
    const gfx::Point& tap_down_root_location) {
  // Show scaled up app icon to indicate draggable state.
  apps_grid_view_->InitiateDrag(this, AppsGridView::TOUCH, tap_down_location,
                                tap_down_root_location);
  SetTouchDragging(true);
}

void AppListItemView::CancelContextMenu() {
  context_menu_.Cancel();
}

gfx::ImageSkia AppListItemView::GetDragImage() {
  return icon_->GetImage();
}

void AppListItemView::OnDragEnded() {
  mouse_drag_timer_.Stop();
  touch_drag_timer_.Stop();
  SetUIState(UI_STATE_NORMAL);
}

gfx::Point AppListItemView::GetDragImageOffset() {
  gfx::Point image = icon_->GetImageBounds().origin();
  return gfx::Point(icon_->x() + image.x(), icon_->y() + image.y());
}

void AppListItemView::SetAsAttemptedFolderTarget(bool is_target_folder) {
  if (is_target_folder)
    SetUIState(UI_STATE_DROPPING_IN_FOLDER);
  else
    SetUIState(UI_STATE_NORMAL);
}

void AppListItemView::SetItemName(const base::string16& display_name,
                                  const base::string16& full_name) {
  title_->SetText(display_name);

  tooltip_text_ = display_name == full_name ? base::string16() : full_name;

  // Use full name for accessibility.
  SetAccessibleName(
      is_folder_ ? l10n_util::GetStringFUTF16(
                       IDS_APP_LIST_FOLDER_BUTTON_ACCESSIBILE_NAME, full_name)
                 : full_name);
  Layout();
}

void AppListItemView::SetItemIsHighlighted(bool is_highlighted) {
  is_highlighted_ = is_highlighted;
  SetTitleSubpixelAA();
  SchedulePaint();
}

void AppListItemView::SetItemIsInstalling(bool is_installing) {
  is_installing_ = is_installing;
  if (ui_state_ == UI_STATE_NORMAL) {
    title_->SetVisible(!is_installing);
    progress_bar_->SetVisible(is_installing);
  }
  SetTitleSubpixelAA();
  SchedulePaint();
}

void AppListItemView::SetItemPercentDownloaded(int percent_downloaded) {
  // A percent_downloaded() of -1 can mean it's not known how much percent is
  // completed, or the download hasn't been marked complete, as is the case
  // while an extension is being installed after being downloaded.
  if (percent_downloaded == -1)
    return;
  progress_bar_->SetValue(percent_downloaded / 100.0);
}

void AppListItemView::OnContextMenuClosed(const base::TimeTicks& open_time) {
  UMA_HISTOGRAM_TIMES("Apps.ContextMenuUserJourneyTime.AppGrid",
                      base::TimeTicks::Now() - open_time);
}

void AppListItemView::OnContextMenuModelReceived(
    const gfx::Point& point,
    ui::MenuSourceType source_type,
    std::vector<ash::mojom::MenuItemPtr> menu) {
  if (menu.empty() || context_menu_.IsRunning())
    return;

  UMA_HISTOGRAM_ENUMERATION("Apps.ContextMenuShowSource.AppGrid", source_type,
                            ui::MENU_SOURCE_TYPE_LAST);

  if (!apps_grid_view_->IsSelectedView(this))
    apps_grid_view_->ClearAnySelectedView();
  int run_types = views::MenuRunner::HAS_MNEMONICS;

  if (source_type == ui::MENU_SOURCE_TOUCH)
    run_types |= views::MenuRunner::SEND_GESTURE_EVENTS_TO_OWNER;

  views::MenuAnchorPosition anchor_position = views::MENU_ANCHOR_TOPLEFT;
  gfx::Rect anchor_rect = gfx::Rect(point, gfx::Size());

  if (features::IsTouchableAppContextMenuEnabled()) {
    run_types |= views::MenuRunner::USE_TOUCHABLE_LAYOUT |
                 views::MenuRunner::FIXED_ANCHOR |
                 views::MenuRunner::CONTEXT_MENU;
    anchor_position = views::MENU_ANCHOR_BUBBLE_TOUCHABLE_LEFT;
    if (source_type == ui::MENU_SOURCE_TOUCH) {
      anchor_rect = apps_grid_view_->GetIdealBounds(this);
      // Anchor the menu to the same rect that is used for selection highlight.
      anchor_rect.ClampToCenteredSize(
          gfx::Size(kGridSelectedSize, kGridSelectedSize));
      views::View::ConvertRectToScreen(apps_grid_view_, &anchor_rect);
    }
  }

  context_menu_.Build(
      std::move(menu), run_types,
      base::Bind(&AppListItemView::OnContextMenuClosed,
                 weak_ptr_factory_.GetWeakPtr(), base::TimeTicks::Now()));
  context_menu_.Run(GetWidget(), nullptr, anchor_rect, anchor_position,
                    source_type);
}

void AppListItemView::ShowContextMenuForView(views::View* source,
                                             const gfx::Point& point,
                                             ui::MenuSourceType source_type) {
  delegate_->GetContextMenuModel(
      item_weak_->id(),
      base::BindOnce(&AppListItemView::OnContextMenuModelReceived,
                     weak_ptr_factory_.GetWeakPtr(), point, source_type));
}

void AppListItemView::ExecuteCommand(int command_id, int event_flags) {
  if (item_weak_) {
    delegate_->ContextMenuItemSelected(item_weak_->id(), command_id,
                                       event_flags);
  }
}

void AppListItemView::StateChanged(ButtonState old_state) {
  if (state() == STATE_HOVERED || state() == STATE_PRESSED) {
    if (shadow_animator_)
      shadow_animator_->animation()->Show();
    // Show the hover/tap highlight: for tap, lighter highlight replaces darker
    // keyboard selection; for mouse hover, keyboard selection takes precedence.
    if (!apps_grid_view_->IsSelectedView(this) || state() == STATE_PRESSED)
      SetItemIsHighlighted(true);
  } else {
    if (shadow_animator_)
      shadow_animator_->animation()->Hide();
    SetItemIsHighlighted(false);
    if (item_weak_)
      item_weak_->set_highlighted(false);
  }
  SetTitleSubpixelAA();
}

bool AppListItemView::ShouldEnterPushedState(const ui::Event& event) {
  // Don't enter pushed state for ET_GESTURE_TAP_DOWN so that hover gray
  // background does not show up during scroll.
  if (event.type() == ui::ET_GESTURE_TAP_DOWN)
    return false;

  return views::Button::ShouldEnterPushedState(event);
}

void AppListItemView::PaintButtonContents(gfx::Canvas* canvas) {
  if (apps_grid_view_->IsDraggedView(this))
    return;

  gfx::Rect rect(GetContentsBounds());
  if (apps_grid_view_->IsSelectedView(this)) {
    rect.ClampToCenteredSize(gfx::Size(kGridSelectedSize, kGridSelectedSize));
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(apps_grid_view_->is_in_folder() ? kFolderGridSelectedColor
                                                   : kGridSelectedColor);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRoundRect(gfx::RectF(rect), kGridSelectedCornerRadius, flags);
  }

  if (ui_state_ == UI_STATE_DROPPING_IN_FOLDER) {
    // Draw folder dropping preview circle.
    gfx::Point center = gfx::Point(icon_->x() + icon_->size().width() / 2,
                                   icon_->y() + icon_->size().height() / 2);
    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    flags.setColor(FolderImage::kFolderBubbleColor);
    canvas->DrawCircle(center, kFolderPreviewRadius, flags);
  }
}

bool AppListItemView::OnMousePressed(const ui::MouseEvent& event) {
  Button::OnMousePressed(event);

  if (!ShouldEnterPushedState(event))
    return true;

  apps_grid_view_->InitiateDrag(this, AppsGridView::MOUSE, event.location(),
                                event.root_location());

  if (apps_grid_view_->IsDraggedView(this)) {
    mouse_drag_timer_.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(kMouseDragUIDelayInMs),
        this, &AppListItemView::OnMouseDragTimer);
  }
  return true;
}

const char* AppListItemView::GetClassName() const {
  return kViewClassName;
}

void AppListItemView::Layout() {
  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty())
    return;

  icon_->SetBoundsRect(GetIconBoundsForTargetViewBounds(GetContentsBounds()));

  rect.Inset(kGridTitleHorizontalPadding,
             kGridIconTopPadding + kGridIconDimension + kGridTitleSpacing,
             kGridTitleHorizontalPadding, 0);
  rect.set_height(title_->GetPreferredSize().height());
  title_->SetBoundsRect(rect);
  SetTitleSubpixelAA();

  gfx::Rect progress_bar_bounds(progress_bar_->GetPreferredSize());
  progress_bar_bounds.set_x(
      (GetContentsBounds().width() - progress_bar_bounds.width()) / 2);
  progress_bar_bounds.set_y(rect.y());
  progress_bar_->SetBoundsRect(progress_bar_bounds);
}

gfx::Size AppListItemView::CalculatePreferredSize() const {
  gfx::Size size = gfx::Size(kGridTileWidth, kGridTileHeight);
  return size;
}

bool AppListItemView::OnKeyPressed(const ui::KeyEvent& event) {
  // Disable space key to press the button. The keyboard events received
  // by this view are forwarded from a Textfield (SearchBoxView) and key
  // released events are not forwarded. This leaves the button in pressed
  // state.
  if (event.key_code() == ui::VKEY_SPACE)
    return false;

  return Button::OnKeyPressed(event);
}

void AppListItemView::OnMouseReleased(const ui::MouseEvent& event) {
  Button::OnMouseReleased(event);
  SetMouseDragging(false);
}

bool AppListItemView::OnMouseDragged(const ui::MouseEvent& event) {
  Button::OnMouseDragged(event);
  if (apps_grid_view_->IsDraggedView(this) && mouse_dragging_) {
    // Update the drag location of the drag proxy if it has been created.
    // If the drag is no longer happening, it could be because this item
    // got removed, in which case this item has been destroyed. So, bail out
    // now as there will be nothing else to do anyway as
    // apps_grid_view_->dragging() will be false.
    if (!apps_grid_view_->UpdateDragFromItem(AppsGridView::MOUSE, event))
      return true;
  }

  if (!apps_grid_view_->IsSelectedView(this))
    apps_grid_view_->ClearAnySelectedView();

  // Show dragging UI when it's confirmed without waiting for the timer.
  if (ui_state_ != UI_STATE_DRAGGING && apps_grid_view_->dragging() &&
      apps_grid_view_->IsDraggedView(this)) {
    mouse_drag_timer_.Stop();
    SetUIState(UI_STATE_DRAGGING);
  }
  return true;
}

void AppListItemView::OnFocus() {
  apps_grid_view_->SetSelectedView(this);
}

void AppListItemView::OnBlur() {
  apps_grid_view_->ClearSelectedView(this);
}

void AppListItemView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_SCROLL_BEGIN:
      if (touch_dragging_) {
        CancelContextMenu();
        apps_grid_view_->StartDragAndDropHostDragAfterLongPress(
            AppsGridView::TOUCH);
        event->SetHandled();
      } else {
        touch_drag_timer_.Stop();
      }
      break;
    case ui::ET_GESTURE_SCROLL_UPDATE:
      if (touch_dragging_ && apps_grid_view_->IsDraggedView(this)) {
        apps_grid_view_->UpdateDragFromItem(AppsGridView::TOUCH, *event);
        event->SetHandled();
      }
      break;
    case ui::ET_GESTURE_SCROLL_END:
    case ui::ET_SCROLL_FLING_START:
      if (touch_dragging_) {
        SetTouchDragging(false);
        event->SetHandled();
      }
      break;
    case ui::ET_GESTURE_TAP_DOWN:
      if (state() != STATE_DISABLED) {
        SetState(STATE_PRESSED);
        touch_drag_timer_.Start(
            FROM_HERE,
            base::TimeDelta::FromMilliseconds(kTouchLongpressDelayInMs),
            base::Bind(&AppListItemView::OnTouchDragTimer,
                       base::Unretained(this), event->location(),
                       event->root_location()));
        event->SetHandled();
      }
      break;
    case ui::ET_GESTURE_TAP:
    case ui::ET_GESTURE_TAP_CANCEL:
      if (state() != STATE_DISABLED) {
        touch_drag_timer_.Stop();
        SetState(STATE_NORMAL);
      }
      break;
    case ui::ET_GESTURE_LONG_TAP:
    case ui::ET_GESTURE_END:
      touch_drag_timer_.Stop();
      SetTouchDragging(false);
      break;
    case ui::ET_GESTURE_TWO_FINGER_TAP:
      if (touch_dragging_) {
        SetTouchDragging(false);
      } else {
        touch_drag_timer_.Stop();
      }
      break;
    default:
      break;
  }
  if (!event->handled())
    Button::OnGestureEvent(event);
}

bool AppListItemView::GetTooltipText(const gfx::Point& p,
                                     base::string16* tooltip) const {
  // Use the label to generate a tooltip, so that it will consider its text
  // truncation in making the tooltip. We do not want the label itself to have a
  // tooltip, so we only temporarily enable it to get the tooltip text from the
  // label, then disable it again.
  title_->SetHandlesTooltips(true);
  title_->SetTooltipText(tooltip_text_);
  bool handled = title_->GetTooltipText(p, tooltip);
  title_->SetHandlesTooltips(false);
  return handled;
}

void AppListItemView::ImageShadowAnimationProgressed(
    ImageShadowAnimator* animator) {
  icon_->SetImage(animator->shadow_image());
  Layout();
}

void AppListItemView::OnSyncDragEnd() {
  SetUIState(UI_STATE_NORMAL);
}

const gfx::Rect& AppListItemView::GetIconBounds() const {
  return icon_->bounds();
}

void AppListItemView::SetDragUIState() {
  SetUIState(UI_STATE_DRAGGING);
}

gfx::Rect AppListItemView::GetIconBoundsForTargetViewBounds(
    const gfx::Rect& target_bounds) {
  gfx::Rect rect(target_bounds);
  rect.Inset(0, kGridIconTopPadding, 0, 0);
  rect.set_height(icon_->GetImage().height());
  rect.ClampToCenteredSize(icon_->GetImage().size());
  return rect;
}

void AppListItemView::SetTitleSubpixelAA() {
  // TODO(tapted): Enable AA for folders as well, taking care to play nice with
  // the folder bubble animation.
  bool enable_aa = !is_in_folder_ && ui_state_ == UI_STATE_NORMAL &&
                   !is_highlighted_ && !apps_grid_view_->IsSelectedView(this) &&
                   !apps_grid_view_->IsAnimatingView(this);

  title_->SetSubpixelRenderingEnabled(enable_aa);
  if (enable_aa) {
    title_->SetBackgroundColor(app_list::kLabelBackgroundColor);
    title_->SetBackground(
        views::CreateSolidBackground(app_list::kLabelBackgroundColor));
  } else {
    // In other cases, keep the background transparent to ensure correct
    // interactions with animations. This will temporarily disable subpixel AA.
    title_->SetBackgroundColor(0);
    title_->SetBackground(nullptr);
  }
  title_->SchedulePaint();
}

void AppListItemView::ItemIconChanged() {
  SetIcon(item_weak_->icon());
}

void AppListItemView::ItemNameChanged() {
  SetItemName(base::UTF8ToUTF16(item_weak_->GetDisplayName()),
              base::UTF8ToUTF16(item_weak_->name()));
}

void AppListItemView::ItemIsInstallingChanged() {
  SetItemIsInstalling(item_weak_->is_installing());
}

void AppListItemView::ItemPercentDownloadedChanged() {
  SetItemPercentDownloaded(item_weak_->percent_downloaded());
}

void AppListItemView::ItemBeingDestroyed() {
  DCHECK(item_weak_);
  item_weak_->RemoveObserver(this);
  item_weak_ = NULL;
}

}  // namespace app_list
