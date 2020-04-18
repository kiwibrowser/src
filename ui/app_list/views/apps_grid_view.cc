// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/apps_grid_view.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_item.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/guid.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/ranges.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "ui/app_list/app_list_metrics.h"
#include "ui/app_list/app_list_util.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/app_list/pagination_controller.h"
#include "ui/app_list/views/app_list_drag_and_drop_host.h"
#include "ui/app_list/views/app_list_folder_view.h"
#include "ui/app_list/views/app_list_item_view.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/expand_arrow_view.h"
#include "ui/app_list/views/indicator_chip_view.h"
#include "ui/app_list/views/pulsing_block_view.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/search_result_tile_item_view.h"
#include "ui/app_list/views/suggestions_container_view.h"
#include "ui/app_list/views/top_icon_animation_view.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/geometry/vector2d_conversions.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view_model_utils.h"
#include "ui/views/widget/widget.h"

namespace app_list {

namespace {

// The preferred width for apps grid.
constexpr int kAppsGridPreferredWidth = 576;

// 32px page break space adjustment needed to keep 48px page break space due to
// the fact that page #02 and all the followings have bottom 56px padding.
constexpr int kPageBreakSpaceAdjustment = 32;

// Distance a drag needs to be from the app grid to be considered 'outside', at
// which point we rearrange the apps to their pre-drag configuration, as a drop
// then would be canceled. We have a buffer to make it easier to drag apps to
// other pages.
constexpr int kDragBufferPx = 20;

// Padding space in pixels between pages.
constexpr int kPagePadding = 40;

// Padding on each side of a tile.
constexpr int kTileHorizontalPadding = 12;
constexpr int kTileVerticalPadding = 6;

// Padding of a tile within the folder view.
constexpr int kFolderTilePadding = 6;

// Width in pixels of the area on the sides that triggers a page flip.
constexpr int kPageFlipZoneSize = 40;

// Delay in milliseconds to do the page flip in fullscreen app list.
constexpr int kPageFlipDelayInMsFullscreen = 500;

// The drag and drop proxy should get scaled by this factor.
constexpr float kDragAndDropProxyScale = 1.2f;

// Delays in milliseconds to show folder dropping preview circle.
constexpr int kFolderDroppingDelay = 150;

// Delays in milliseconds to show re-order preview.
constexpr int kReorderDelay = 120;

// Delays in milliseconds to show folder item reparent UI.
constexpr int kFolderItemReparentDelay = 50;

// Radius of the circle, in which if entered, show folder dropping preview
// UI.
constexpr int kFolderDroppingCircleRadius = 39;

// Padding between suggested apps tiles and all apps indicator.
constexpr int kSuggestionsAllAppsIndicatorPadding = 26;

// Padding between all apps indicator and all apps.
constexpr int kAllAppsIndicatorBottomPadding = 2;

// The height of gradient fade-out zones.
constexpr int kFadeoutZoneHeight = 24;

// Range of dragging amount fraction for suggested apps to change opacity from
// 0.f to 1.0f.
constexpr float kSuggestedAppsOpacityStartFraction = 0.3f;
constexpr float kSuggestedAppsOpacityEndFraction = 0.7f;

// Range of dragging amount fraction for all apps indicator to change opacity
// from 0.f to 1.0f.
constexpr float kAllAppsIndicatorOpacityStartFraction = 0.7f;
constexpr float kAllAppsIndicatorOpacityEndFraction = 1.0f;

// Range of dragging amount fraction for expand arrow to change opacity from
// 1.0f to 0.f.
constexpr float kExpandArrowDismissStartFraction = 0.f;
constexpr float kExpandArrowDismissEndFraction = 0.2f;

// Range of dragging amount fraction for expand arrow to change opacity from 0.f
// to 1.0f.
constexpr float kExpandArrowShowStartFraction = 0.5f;
constexpr float kExpandArrowShowEndFraction = 1.0f;

// Returns the size of a tile view excluding its padding.
gfx::Size GetTileViewSize() {
  return gfx::Size(kGridTileWidth, kGridTileHeight);
}

// RowMoveAnimationDelegate is used when moving an item into a different row.
// Before running the animation, the item's layer is re-created and kept in
// the original position, then the item is moved to just before its target
// position and opacity set to 0. When the animation runs, this delegate moves
// the layer and fades it out while fading in the item at the same time.
class RowMoveAnimationDelegate : public gfx::AnimationDelegate {
 public:
  RowMoveAnimationDelegate(views::View* view,
                           ui::Layer* layer,
                           const gfx::Rect& layer_target)
      : view_(view),
        layer_(layer),
        layer_start_(layer ? layer->bounds() : gfx::Rect()),
        layer_target_(layer_target) {}
  ~RowMoveAnimationDelegate() override {}

  // gfx::AnimationDelegate overrides:
  void AnimationProgressed(const gfx::Animation* animation) override {
    view_->layer()->SetOpacity(animation->GetCurrentValue());
    view_->layer()->ScheduleDraw();

    if (layer_) {
      layer_->SetOpacity(1 - animation->GetCurrentValue());
      layer_->SetBounds(
          animation->CurrentValueBetween(layer_start_, layer_target_));
      layer_->ScheduleDraw();
    }
  }
  void AnimationEnded(const gfx::Animation* animation) override {
    view_->layer()->SetOpacity(1.0f);
    view_->SchedulePaint();
  }
  void AnimationCanceled(const gfx::Animation* animation) override {
    view_->layer()->SetOpacity(1.0f);
    view_->SchedulePaint();
  }

 private:
  // The view that needs to be wrapped. Owned by views hierarchy.
  views::View* view_;

  std::unique_ptr<ui::Layer> layer_;
  const gfx::Rect layer_start_;
  const gfx::Rect layer_target_;

  DISALLOW_COPY_AND_ASSIGN(RowMoveAnimationDelegate);
};

// ItemRemoveAnimationDelegate is used to show animation for removing an item.
// This happens when user drags an item into a folder. The dragged item will
// be removed from the original list after it is dropped into the folder.
class ItemRemoveAnimationDelegate : public gfx::AnimationDelegate {
 public:
  explicit ItemRemoveAnimationDelegate(views::View* view) : view_(view) {}

  ~ItemRemoveAnimationDelegate() override {}

  // gfx::AnimationDelegate overrides:
  void AnimationProgressed(const gfx::Animation* animation) override {
    view_->layer()->SetOpacity(1 - animation->GetCurrentValue());
    view_->layer()->ScheduleDraw();
  }

 private:
  std::unique_ptr<views::View> view_;

  DISALLOW_COPY_AND_ASSIGN(ItemRemoveAnimationDelegate);
};

// ItemMoveAnimationDelegate observes when an item finishes animating when it is
// not moving between rows. This is to ensure an item is repainted for the
// "zoom out" case when releasing an item being dragged.
class ItemMoveAnimationDelegate : public gfx::AnimationDelegate {
 public:
  explicit ItemMoveAnimationDelegate(views::View* view) : view_(view) {}

  void AnimationEnded(const gfx::Animation* animation) override {
    view_->SchedulePaint();
  }
  void AnimationCanceled(const gfx::Animation* animation) override {
    view_->SchedulePaint();
  }

 private:
  views::View* view_;

  DISALLOW_COPY_AND_ASSIGN(ItemMoveAnimationDelegate);
};

// Returns true if the |item| is a folder item.
bool IsFolderItem(AppListItem* item) {
  return (item->GetItemType() == AppListFolderItem::kItemType);
}

bool IsOEMFolderItem(AppListItem* item) {
  return IsFolderItem(item) &&
         (static_cast<AppListFolderItem*>(item))->folder_type() ==
             AppListFolderItem::FOLDER_TYPE_OEM;
}

int GetCompositorActivatedFrameCount(ui::Compositor* compositor) {
  return compositor ? compositor->activated_frame_count() : 0;
}

}  // namespace

// A layer delegate used for AppsGridView's mask layer, with top and bottom
// gradient fading out zones.
class AppsGridView::FadeoutLayerDelegate : public ui::LayerDelegate {
 public:
  FadeoutLayerDelegate() : layer_(ui::LAYER_TEXTURED) {
    layer_.set_delegate(this);
    layer_.SetFillsBoundsOpaquely(false);
  }

  ~FadeoutLayerDelegate() override { layer_.set_delegate(nullptr); }

  ui::Layer* layer() { return &layer_; }

 private:
  // ui::LayerDelegate overrides:
  // TODO(warx): using a mask is expensive. It would be more efficient to avoid
  // the mask for the central area and only use it for top/bottom areas.
  void OnPaintLayer(const ui::PaintContext& context) override {
    const gfx::Size size = layer()->size();
    gfx::Rect top_rect(0, 0, size.width(), kFadeoutZoneHeight);
    gfx::Rect bottom_rect(0, size.height() - kFadeoutZoneHeight, size.width(),
                          kFadeoutZoneHeight);
    ui::PaintRecorder recorder(context, size);
    gfx::Canvas* canvas = recorder.canvas();
    // Clear the canvas.
    canvas->DrawColor(SK_ColorBLACK, SkBlendMode::kSrc);
    // Draw top gradient zone.
    cc::PaintFlags flags;
    flags.setBlendMode(SkBlendMode::kSrc);
    flags.setAntiAlias(false);
    flags.setShader(gfx::CreateGradientShader(
        0, kFadeoutZoneHeight, SK_ColorTRANSPARENT, SK_ColorBLACK));
    canvas->DrawRect(top_rect, flags);
    // Draw bottom gradient zone.
    flags.setShader(gfx::CreateGradientShader(
        size.height() - kFadeoutZoneHeight, size.height(), SK_ColorBLACK,
        SK_ColorTRANSPARENT));
    canvas->DrawRect(bottom_rect, flags);
  }
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}

  ui::Layer layer_;

  DISALLOW_COPY_AND_ASSIGN(FadeoutLayerDelegate);
};

AppsGridView::AppsGridView(ContentsView* contents_view,
                           AppsGridViewFolderDelegate* folder_delegate)
    : folder_delegate_(folder_delegate),
      contents_view_(contents_view),
      bounds_animator_(this),
      page_flip_delay_in_ms_(kPageFlipDelayInMsFullscreen),
      pagination_animation_start_frame_number_(0) {
  DCHECK(contents_view_);
  SetPaintToLayer();
  // Clip any icons that are outside the grid view's bounds. These icons would
  // otherwise be visible to the user when the grid view is off screen.
  layer()->SetMasksToBounds(true);
  layer()->SetFillsBoundsOpaquely(false);

  if (!folder_delegate_) {
    suggestions_container_ =
        new SuggestionsContainerView(contents_view_, &pagination_model_);
    AddChildView(suggestions_container_);
    UpdateSuggestions();

    all_apps_indicator_ = new IndicatorChipView(
        l10n_util::GetStringUTF16(IDS_ALL_APPS_INDICATOR));
    AddChildView(all_apps_indicator_);
    if (features::IsBackgroundBlurEnabled()) {
      // TODO(newcomer): Improve implementation of the mask layer so we can
      // enable it on all devices crbug.com/765292.
      fadeout_layer_delegate_.reset(new FadeoutLayerDelegate);
      layer()->SetMaskLayer(fadeout_layer_delegate_->layer());
    }

    expand_arrow_view_ =
        new ExpandArrowView(contents_view_, contents_view_->app_list_view());
    AddChildView(expand_arrow_view_);
  }

  pagination_model_.SetTransitionDurations(kPageTransitionDurationInMs,
                                           kOverscrollPageTransitionDurationMs);

  pagination_model_.AddObserver(this);

  pagination_controller_.reset(new PaginationController(
      &pagination_model_, folder_delegate_
                              ? PaginationController::SCROLL_AXIS_HORIZONTAL
                              : PaginationController::SCROLL_AXIS_VERTICAL));
}

AppsGridView::~AppsGridView() {
  // Coming here |drag_view_| should already be canceled since otherwise the
  // drag would disappear after the app list got animated away and closed,
  // which would look odd.
  DCHECK(!drag_view_);
  if (drag_view_)
    EndDrag(true);

  if (model_)
    model_->RemoveObserver(this);
  pagination_model_.RemoveObserver(this);

  if (item_list_)
    item_list_->RemoveObserver(this);

  view_model_.Clear();
  RemoveAllChildViews(true);
}

void AppsGridView::SetLayout(int cols, int rows_per_page) {
  cols_ = cols;
  rows_per_page_ = rows_per_page;
}

gfx::Size AppsGridView::GetTotalTileSize() const {
  gfx::Rect rect(GetTileViewSize());
  rect.Inset(GetTilePadding());
  return rect.size();
}

gfx::Insets AppsGridView::GetTilePadding() const {
  if (folder_delegate_)
    return gfx::Insets(-kFolderTilePadding, -kFolderTilePadding);
  return gfx::Insets(-kTileVerticalPadding, -kTileHorizontalPadding);
}

gfx::Size AppsGridView::GetTileGridSizeWithoutPadding() const {
  gfx::Size size = GetTileGridSize();
  gfx::Insets grid_padding = GetTilePadding();
  size.Enlarge(grid_padding.width(), grid_padding.height());
  return size;
}

void AppsGridView::ResetForShowApps() {
  UpdateSuggestions();
  ClearDragState();
  layer()->SetOpacity(1.0f);
  SetVisible(true);
  // Set all views to visible in case they weren't made visible again by an
  // incomplete animation.
  for (int i = 0; i < view_model_.view_size(); ++i) {
    view_model_.view_at(i)->SetVisible(true);
  }
  CHECK_EQ(item_list_->item_count(),
           static_cast<size_t>(view_model_.view_size()));
}

void AppsGridView::DisableFocusForShowingActiveFolder(bool disabled) {
  for (auto* v : suggestions_container_->tile_views())
    v->SetEnabled(!disabled);
  for (int i = 0; i < view_model_.view_size(); ++i) {
    view_model_.view_at(i)->SetEnabled(!disabled);
  }
}

void AppsGridView::SetModel(AppListModel* model) {
  if (model_)
    model_->RemoveObserver(this);

  model_ = model;
  if (model_)
    model_->AddObserver(this);

  Update();
}

void AppsGridView::SetItemList(AppListItemList* item_list) {
  if (item_list_)
    item_list_->RemoveObserver(this);
  item_list_ = item_list;
  if (item_list_)
    item_list_->AddObserver(this);
  Update();
}

void AppsGridView::SetSelectedView(AppListItemView* view) {
  if (IsSelectedView(view) || IsDraggedView(view))
    return;

  Index index = GetIndexOfView(view);
  if (IsValidIndex(index))
    SetSelectedItemByIndex(index);
}

void AppsGridView::ClearSelectedView(AppListItemView* view) {
  if (view && IsSelectedView(view)) {
    selected_view_->SchedulePaint();
    selected_view_ = nullptr;
  }
}

void AppsGridView::ClearAnySelectedView() {
  if (selected_view_) {
    selected_view_->SchedulePaint();
    selected_view_ = nullptr;
  }
  if (suggestions_container_)
    suggestions_container_->ClearSelectedIndex();
}

bool AppsGridView::IsSelectedView(const AppListItemView* view) const {
  return selected_view_ == view;
}

views::View* AppsGridView::GetSelectedView() const {
  if (selected_view_)
    return selected_view_;
  if (expand_arrow_view_ && expand_arrow_view_->HasFocus())
    return expand_arrow_view_;
  if (suggestions_container_)
    return suggestions_container_->GetSelectedView();
  return nullptr;
}

void AppsGridView::InitiateDrag(AppListItemView* view,
                                Pointer pointer,
                                const gfx::Point& location,
                                const gfx::Point& root_location) {
  DCHECK(view);
  if (drag_view_ || pulsing_blocks_model_.view_size())
    return;

  drag_view_ = view;
  drag_view_init_index_ = GetIndexOfView(drag_view_);
  drag_view_offset_ = location;
  drag_start_page_ = pagination_model_.selected_page();
  reorder_placeholder_ = drag_view_init_index_;
  ExtractDragLocation(root_location, &drag_start_grid_view_);
  drag_view_start_ = gfx::Point(drag_view_->x(), drag_view_->y());
}

void AppsGridView::StartDragAndDropHostDragAfterLongPress(Pointer pointer) {
  TryStartDragAndDropHostDrag(pointer, drag_start_grid_view_);
}

void AppsGridView::TryStartDragAndDropHostDrag(
    Pointer pointer,
    const gfx::Point& grid_location) {
  drag_pointer_ = pointer;
  // Move the view to the front so that it appears on top of other views.
  ReorderChildView(drag_view_, -1);
  bounds_animator_.StopAnimatingView(drag_view_);
  // Stopping the animation may have invalidated our drag view due to the
  // view hierarchy changing.
  if (!drag_view_)
    return;

  if (!dragging_for_reparent_item_)
    StartDragAndDropHostDrag(grid_location);
}

bool AppsGridView::UpdateDragFromItem(Pointer pointer,
                                      const ui::LocatedEvent& event) {
  if (!drag_view_)
    return false;  // Drag canceled.

  gfx::Point drag_point_in_grid_view;
  ExtractDragLocation(event.root_location(), &drag_point_in_grid_view);
  UpdateDrag(pointer, drag_point_in_grid_view);
  if (!dragging())
    return false;

  // If a drag and drop host is provided, see if the drag operation needs to be
  // forwarded.
  gfx::Point location_in_screen = drag_point_in_grid_view;
  views::View::ConvertPointToScreen(this, &location_in_screen);
  DispatchDragEventToDragAndDropHost(location_in_screen);
  if (drag_and_drop_host_) {
    drag_and_drop_host_->UpdateDragIconProxyByLocation(
        drag_view_->icon()->GetBoundsInScreen().origin());
  }
  return true;
}

void AppsGridView::UpdateDrag(Pointer pointer, const gfx::Point& point) {
  if (folder_delegate_)
    UpdateDragStateInsideFolder(pointer, point);

  if (!drag_view_)
    return;  // Drag canceled.

  gfx::Vector2d drag_vector(point - drag_start_grid_view_);
  if (!dragging() && ExceededDragThreshold(drag_vector))
    TryStartDragAndDropHostDrag(pointer, point);

  if (drag_pointer_ != pointer)
    return;

  drag_view_->SetPosition(drag_view_start_ + drag_vector);

  last_drag_point_ = point;
  const Index last_reorder_drop_target = reorder_drop_target_;
  const Index last_folder_drop_target = folder_drop_target_;
  DropAttempt last_drop_attempt = drop_attempt_;
  CalculateDropTarget();

  MaybeStartPageFlipTimer(last_drag_point_);

  if (last_folder_drop_target != folder_drop_target_ ||
      last_reorder_drop_target != reorder_drop_target_ ||
      last_drop_attempt != drop_attempt_) {
    if (drop_attempt_ == DROP_FOR_REORDER) {
      folder_dropping_timer_.Stop();
      reorder_timer_.Start(FROM_HERE,
                           base::TimeDelta::FromMilliseconds(kReorderDelay),
                           this, &AppsGridView::OnReorderTimer);
    } else if (drop_attempt_ == DROP_FOR_FOLDER) {
      reorder_timer_.Stop();
      folder_dropping_timer_.Start(
          FROM_HERE, base::TimeDelta::FromMilliseconds(kFolderDroppingDelay),
          this, &AppsGridView::OnFolderDroppingTimer);
    }

    // Reset the previous drop target.
    SetAsFolderDroppingTarget(last_folder_drop_target, false);
  }
}

void AppsGridView::EndDrag(bool cancel) {
  // EndDrag was called before if |drag_view_| is NULL.
  if (!drag_view_)
    return;

  // Coming here a drag and drop was in progress.
  bool landed_in_drag_and_drop_host = forward_events_to_drag_and_drop_host_;
  if (forward_events_to_drag_and_drop_host_) {
    DCHECK(!IsDraggingForReparentInRootLevelGridView());
    forward_events_to_drag_and_drop_host_ = false;
    drag_and_drop_host_->EndDrag(cancel);
    if (IsDraggingForReparentInHiddenGridView()) {
      folder_delegate_->DispatchEndDragEventForReparent(
          true /* events_forwarded_to_drag_drop_host */,
          cancel /* cancel_drag */);
    }
  } else {
    if (IsDraggingForReparentInHiddenGridView()) {
      // Forward the EndDrag event to the root level grid view.
      folder_delegate_->DispatchEndDragEventForReparent(
          false /* events_forwarded_to_drag_drop_host */,
          cancel /* cancel_drag */);
      EndDragForReparentInHiddenFolderGridView();
      return;
    }

    if (IsDraggingForReparentInRootLevelGridView()) {
      // An EndDrag can be received during a reparent via a model change. This
      // is always a cancel and needs to be forwarded to the folder.
      DCHECK(cancel);
      contents_view_->GetAppListMainView()->CancelDragInActiveFolder();
      return;
    }

    if (!cancel && dragging()) {
      // Regular drag ending path, ie, not for reparenting.
      CalculateDropTarget();
      if (EnableFolderDragDropUI() && drop_attempt_ == DROP_FOR_FOLDER &&
          IsValidIndex(folder_drop_target_)) {
        MoveItemToFolder(drag_view_, folder_drop_target_);
      } else if (IsValidIndex(reorder_drop_target_)) {
        MoveItemInModel(drag_view_, reorder_drop_target_);
      }
    }
  }

  if (drag_and_drop_host_) {
    // If we had a drag and drop proxy icon, we delete it and make the real
    // item visible again.
    drag_and_drop_host_->DestroyDragIconProxy();
    // Issue 439055: MoveItemToFolder() can sometimes delete |drag_view_|
    if (drag_view_) {
      if (landed_in_drag_and_drop_host) {
        // Move the item directly to the target location, avoiding the
        // "zip back" animation if the user was pinning it to the shelf.
        int i = reorder_drop_target_.slot;
        gfx::Rect bounds = view_model_.ideal_bounds(i);
        drag_view_->SetBoundsRect(bounds);
      }
      // Fade in slowly if it landed in the shelf.
      SetViewHidden(drag_view_, false /* show */,
                    !landed_in_drag_and_drop_host /* animate */);
    }
  }

  SetAsFolderDroppingTarget(folder_drop_target_, false);
  ClearDragState();
  AnimateToIdealBounds();

  StopPageFlipTimer();
}

void AppsGridView::StopPageFlipTimer() {
  page_flip_timer_.Stop();
  page_flip_target_ = -1;
}

const gfx::Rect& AppsGridView::GetIdealBounds(AppListItemView* view) const {
  const int index = view_model_.GetIndexOfView(view);
  DCHECK_NE(-1, index);
  return view_model_.ideal_bounds(index);
}

AppListItemView* AppsGridView::GetItemViewAt(int index) const {
  return view_model_.view_at(index);
}

void AppsGridView::ScheduleShowHideAnimation(bool show) {
  // Stop any previous animation.
  layer()->GetAnimator()->StopAnimating();

  // Set initial state.
  SetVisible(true);
  layer()->SetOpacity(show ? 0.0f : 1.0f);

  ui::ScopedLayerAnimationSettings animation(layer()->GetAnimator());
  animation.AddObserver(this);
  animation.SetTweenType(show ? kFolderFadeInTweenType
                              : kFolderFadeOutTweenType);
  animation.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
      show ? kFolderTransitionInDurationMs : kFolderTransitionOutDurationMs));

  layer()->SetOpacity(show ? 1.0f : 0.0f);
}

void AppsGridView::InitiateDragFromReparentItemInRootLevelGridView(
    AppListItemView* original_drag_view,
    const gfx::Rect& drag_view_rect,
    const gfx::Point& drag_point,
    bool has_native_drag) {
  DCHECK(original_drag_view && !drag_view_);
  DCHECK(!dragging_for_reparent_item_);

  // Since the item is new, its placeholder is conceptually at the back of the
  // entire apps grid.
  reorder_placeholder_ = GetLastViewIndex();

  // Create a new AppListItemView to duplicate the original_drag_view in the
  // folder's grid view.
  AppListItemView* view = new AppListItemView(
      this, original_drag_view->item(),
      contents_view_->GetAppListMainView()->view_delegate());
  AddChildView(view);
  drag_view_ = view;
  drag_view_->SetPaintToLayer();
  drag_view_->layer()->SetFillsBoundsOpaquely(false);
  drag_view_->SetBoundsRect(drag_view_rect);
  drag_view_->SetDragUIState();  // Hide the title of the drag_view_.

  // Hide the drag_view_ for drag icon proxy when a native drag is responsible
  // for showing the icon.
  if (has_native_drag)
    SetViewHidden(drag_view_, true /* hide */, true /* no animate */);

  // Add drag_view_ to the end of the view_model_.
  view_model_.Add(drag_view_, view_model_.view_size());

  drag_start_page_ = pagination_model_.selected_page();
  drag_start_grid_view_ = drag_point;

  drag_view_start_ = gfx::Point(drag_view_->x(), drag_view_->y());

  // Set the flag in root level grid view.
  dragging_for_reparent_item_ = true;
}

void AppsGridView::UpdateDragFromReparentItem(Pointer pointer,
                                              const gfx::Point& drag_point) {
  // Note that if a cancel ocurrs while reparenting, the |drag_view_| in both
  // root and folder grid views is cleared, so the check in UpdateDragFromItem()
  // for |drag_view_| being NULL (in the folder grid) is sufficient.
  DCHECK(drag_view_);
  DCHECK(IsDraggingForReparentInRootLevelGridView());

  UpdateDrag(pointer, drag_point);
}

bool AppsGridView::IsDraggedView(const AppListItemView* view) const {
  return drag_view_ == view;
}

void AppsGridView::ClearDragState() {
  drop_attempt_ = DROP_FOR_NONE;
  drag_pointer_ = NONE;
  reorder_drop_target_ = Index();
  folder_drop_target_ = Index();
  reorder_placeholder_ = Index();
  drag_start_grid_view_ = gfx::Point();
  drag_start_page_ = -1;
  drag_view_offset_ = gfx::Point();

  if (drag_view_) {
    drag_view_->OnDragEnded();
    if (IsDraggingForReparentInRootLevelGridView()) {
      const int drag_view_index = view_model_.GetIndexOfView(drag_view_);
      CHECK_EQ(view_model_.view_size() - 1, drag_view_index);
      DeleteItemViewAtIndex(drag_view_index);
    }
  }
  drag_view_ = nullptr;
  dragging_for_reparent_item_ = false;
}

void AppsGridView::SetDragViewVisible(bool visible) {
  DCHECK(drag_view_);
  SetViewHidden(drag_view_, !visible, true);
}

void AppsGridView::SetDragAndDropHostOfCurrentAppList(
    ApplicationDragAndDropHost* drag_and_drop_host) {
  drag_and_drop_host_ = drag_and_drop_host;
}

bool AppsGridView::IsAnimatingView(AppListItemView* view) {
  return bounds_animator_.IsAnimating(view);
}

gfx::Size AppsGridView::CalculatePreferredSize() const {
  if (folder_delegate_)
    return GetTileGridSize();

  gfx::Size size =
      gfx::Size(kAppsGridPreferredWidth, kHorizontalPagePreferredHeight);
  return size;
}

bool AppsGridView::GetDropFormats(
    int* formats,
    std::set<ui::Clipboard::FormatType>* format_types) {
  // TODO(koz): Only accept a specific drag type for app shortcuts.
  *formats = OSExchangeData::FILE_NAME;
  return true;
}

bool AppsGridView::CanDrop(const OSExchangeData& data) {
  return true;
}

int AppsGridView::OnDragUpdated(const ui::DropTargetEvent& event) {
  return ui::DragDropTypes::DRAG_MOVE;
}

const char* AppsGridView::GetClassName() const {
  return "AppsGridView";
}

void AppsGridView::Layout() {
  if (bounds_animator_.IsAnimating())
    bounds_animator_.Cancel();

  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty())
    return;

  if (fadeout_layer_delegate_)
    fadeout_layer_delegate_->layer()->SetBounds(layer()->bounds());
  rect.Inset(0, kSearchBoxBottomPadding, 0, 0);

  gfx::Rect indicator_rect(rect);
  gfx::Rect arrow_rect(rect);
  if (suggestions_container_) {
    gfx::Rect suggestions_rect(rect);
    suggestions_rect.set_height(
        suggestions_container_->GetHeightForWidth(suggestions_rect.width()));
    suggestions_rect.Offset((suggestions_rect.width() - kGridTileWidth) / 2 -
                                (kGridTileWidth + kGridTileSpacing) * 2,
                            0);
    suggestions_rect.Offset(CalculateTransitionOffset(0));
    suggestions_container_->SetBoundsRect(suggestions_rect);
    indicator_rect.Inset(0,
                         suggestions_container_->GetPreferredSize().height() +
                             kSuggestionsAllAppsIndicatorPadding,
                         0, 0);
    arrow_rect.Inset(0,
                     suggestions_container_->GetPreferredSize().height() +
                         kExpandArrowTopPadding,
                     0, 0);
  }

  if (all_apps_indicator_) {
    gfx::Size indicator_size;
    indicator_size = all_apps_indicator_->GetPreferredSize();
    indicator_rect.Inset((indicator_rect.width() - indicator_size.width()) / 2,
                         0);
    indicator_rect.Offset(CalculateTransitionOffset(0));
    all_apps_indicator_->SetBoundsRect(indicator_rect);
  }

  if (expand_arrow_view_) {
    int left_right_padding =
        (arrow_rect.width() - expand_arrow_view_->GetPreferredSize().width()) /
        2;
    arrow_rect.Inset(left_right_padding, 0);
    arrow_rect.set_height(expand_arrow_view_->GetPreferredSize().height());
    expand_arrow_view_->SetBoundsRect(arrow_rect);
  }

  CalculateIdealBounds();
  for (int i = 0; i < view_model_.view_size(); ++i) {
    AppListItemView* view = GetItemViewAt(i);
    if (view != drag_view_)
      view->SetBoundsRect(view_model_.ideal_bounds(i));
  }
  views::ViewModelUtils::SetViewBoundsToIdealBounds(pulsing_blocks_model_);
}

void AppsGridView::UpdateControlVisibility(AppListViewState app_list_state,
                                           bool is_in_drag) {
  const bool fullscreen_apps_in_drag =
      app_list_state == AppListViewState::FULLSCREEN_ALL_APPS || is_in_drag;
  if (all_apps_indicator_)
    all_apps_indicator_->SetVisible(fullscreen_apps_in_drag);

  if (expand_arrow_view_) {
    expand_arrow_view_->SetVisible(
        app_list_state == AppListViewState::PEEKING ? true : false);
  }

  for (int i = 0; i < view_model_.view_size(); ++i) {
    AppListItemView* view = GetItemViewAt(i);
    view->SetVisible(fullscreen_apps_in_drag);
  }
}

bool AppsGridView::OnKeyPressed(const ui::KeyEvent& event) {
  // Let the FocusManager handle Left/Right keys.
  if (!CanProcessUpDownKeyTraversal(event))
    return false;

  const AppListViewState state =
      contents_view_->app_list_view()->app_list_state();
  const bool arrow_up = event.key_code() == ui::VKEY_UP;
  if (state == AppListViewState::PEEKING)
    return HandleFocusMovementInPeekingState(arrow_up);

  DCHECK(state == AppListViewState::FULLSCREEN_ALL_APPS);
  return HandleFocusMovementInFullscreenAllAppsState(arrow_up);
}

void AppsGridView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (!details.is_add && details.parent == this) {
    // The view being delete should not have reference in |view_model_|.
    CHECK_EQ(-1, view_model_.GetIndexOfView(details.child));

    if (selected_view_ == details.child)
      selected_view_ = nullptr;
    if (activated_folder_item_view_ == details.child)
      activated_folder_item_view_ = nullptr;

    if (drag_view_ == details.child)
      EndDrag(true);

    bounds_animator_.StopAnimatingView(details.child);
  }
}

void AppsGridView::OnGestureEvent(ui::GestureEvent* event) {
  // Bail on STATE_START or no apps page to make PaginationModel happy.
  if (contents_view_->GetActiveState() == ash::AppListState::kStateStart ||
      pagination_model_.total_pages() <= 0) {
    return;
  }

  // Scroll begin events should not be passed to ancestor views if it occurs
  // inside the folder bounds even it is not handled. This prevents user from
  // closing the folder when scrolling inside it.
  if (pagination_controller_->OnGestureEvent(*event, GetContentsBounds()) ||
      (folder_delegate_ && event->type() == ui::ET_GESTURE_SCROLL_BEGIN)) {
    event->SetHandled();
  }
}

void AppsGridView::Update() {
  DCHECK(!selected_view_ && !drag_view_);
  view_model_.Clear();
  if (!item_list_ || !item_list_->item_count())
    return;
  for (size_t i = 0; i < item_list_->item_count(); ++i) {
    AppListItemView* view = CreateViewForItemAtIndex(i);
    view_model_.Add(view, i);
    AddChildView(view);
  }
  UpdateColsAndRowsForFolder();
  UpdatePaging();
  UpdatePulsingBlockViews();
  Layout();
  SchedulePaint();
}

void AppsGridView::UpdateSuggestions() {
  if (!suggestions_container_)
    return;
  suggestions_container_->SetResults(contents_view_->GetAppListMainView()
                                         ->view_delegate()
                                         ->GetSearchModel()
                                         ->results());
}

int AppsGridView::TilesPerPage(int page) const {
  if (folder_delegate_)
    return kMaxFolderItemsPerPage;
  if (page == 0)
    return cols_ * (rows_per_page_ - 1);
  return cols_ * rows_per_page_;
}

void AppsGridView::UpdatePaging() {
  if (!view_model_.view_size() || !TilesPerPage(0)) {
    pagination_model_.SetTotalPages(0);
    return;
  }

  int total_pages = 0;
  if (view_model_.view_size() <= TilesPerPage(0)) {
    total_pages = 1;
  } else {
    total_pages =
        (view_model_.view_size() - TilesPerPage(0) - 1) / TilesPerPage(1) + 2;
  }

  pagination_model_.SetTotalPages(total_pages);
}

void AppsGridView::UpdatePulsingBlockViews() {
  const int existing_items = item_list_ ? item_list_->item_count() : 0;
  int current_page = pagination_model_.selected_page();
  const int available_slots =
      TilesPerPage(current_page) - existing_items % TilesPerPage(current_page);
  const int desired =
      model_->status() == ash::AppListModelStatus::kStatusSyncing
          ? available_slots
          : 0;

  if (pulsing_blocks_model_.view_size() == desired)
    return;

  while (pulsing_blocks_model_.view_size() > desired) {
    PulsingBlockView* view = pulsing_blocks_model_.view_at(0);
    pulsing_blocks_model_.Remove(0);
    delete view;
  }

  while (pulsing_blocks_model_.view_size() < desired) {
    PulsingBlockView* view = new PulsingBlockView(GetTotalTileSize(), true);
    pulsing_blocks_model_.Add(view, 0);
    AddChildView(view);
  }
}

AppListItemView* AppsGridView::CreateViewForItemAtIndex(size_t index) {
  // The drag_view_ might be pending for deletion, therefore view_model_
  // may have one more item than item_list_.
  DCHECK_LE(index, item_list_->item_count());
  AppListItemView* view = new AppListItemView(
      this, item_list_->item_at(index),
      contents_view_->GetAppListMainView()->view_delegate());
  view->SetPaintToLayer();
  view->layer()->SetFillsBoundsOpaquely(false);
  return view;
}

bool AppsGridView::HandleScroll(int offset, ui::EventType type) {
  // Bail on STATE_START or no apps page to make PaginationModel happy.
  if (contents_view_->GetActiveState() == ash::AppListState::kStateStart ||
      pagination_model_.total_pages() <= 0) {
    return false;
  }

  const gfx::Vector2dF scroll_offset_vector(0, offset);
  return pagination_controller_->OnScroll(
      gfx::ToFlooredVector2d(scroll_offset_vector), type);
}

AppsGridView::Index AppsGridView::GetIndexFromModelIndex(
    int model_index) const {
  if (model_index < TilesPerPage(0))
    return Index(0, model_index);

  return Index(1 + (model_index - TilesPerPage(0)) / TilesPerPage(1),
               (model_index - TilesPerPage(0)) % TilesPerPage(1));
}

int AppsGridView::GetModelIndexFromIndex(const Index& index) const {
  if (index.page == 0)
    return index.slot;

  return TilesPerPage(0) + (index.page - 1) * TilesPerPage(1) + index.slot;
}

void AppsGridView::EnsureViewVisible(const Index& index) {
  if (pagination_model_.has_transition())
    return;

  if (IsValidIndex(index))
    pagination_model_.SelectPage(index.page, false);
}

void AppsGridView::SetSelectedItemByIndex(const Index& index) {
  if (GetIndexOfView(selected_view_) == index)
    return;

  AppListItemView* new_selection = GetViewAtIndex(index);
  if (!new_selection)
    return;  // Keep current selection.

  if (selected_view_)
    selected_view_->SchedulePaint();

  EnsureViewVisible(index);
  selected_view_ = new_selection;
  selected_view_->SetTitleSubpixelAA();
  selected_view_->SchedulePaint();
  selected_view_->NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
}

bool AppsGridView::IsValidIndex(const Index& index) const {
  return index.page >= 0 && index.page < pagination_model_.total_pages() &&
         index.slot >= 0 && index.slot < TilesPerPage(index.page) &&
         GetModelIndexFromIndex(index) < view_model_.view_size();
}

AppsGridView::Index AppsGridView::GetIndexOfView(
    const AppListItemView* view) const {
  const int model_index = view_model_.GetIndexOfView(view);
  if (model_index == -1)
    return Index();

  return GetIndexFromModelIndex(model_index);
}

AppListItemView* AppsGridView::GetViewAtIndex(const Index& index) const {
  if (!IsValidIndex(index))
    return nullptr;

  const int model_index = GetModelIndexFromIndex(index);
  return GetItemViewAt(model_index);
}

AppsGridView::Index AppsGridView::GetLastViewIndex() const {
  DCHECK_LT(0, view_model_.view_size());
  int view_index = view_model_.view_size() - 1;
  return GetIndexFromModelIndex(view_index);
}

const gfx::Vector2d AppsGridView::CalculateTransitionOffset(
    int page_of_view) const {
  gfx::Size grid_size = GetTileGridSize();

  // If there is a transition, calculates offset for current and target page.
  const int current_page = pagination_model_.selected_page();
  const PaginationModel::Transition& transition =
      pagination_model_.transition();
  const bool is_valid = pagination_model_.is_valid_page(transition.target_page);

  // Transition to previous page means negative offset.
  const int dir = transition.target_page > current_page ? -1 : 1;

  int x_offset = 0;
  int y_offset = 0;

  if (pagination_controller_->scroll_axis() ==
      PaginationController::SCROLL_AXIS_HORIZONTAL) {
    // Page size including padding pixels. A tile.x + page_width means the same
    // tile slot in the next page.
    const int page_width = grid_size.width() + kPagePadding;
    if (page_of_view < current_page)
      x_offset = -page_width;
    else if (page_of_view > current_page)
      x_offset = page_width;

    if (is_valid) {
      if (page_of_view == current_page ||
          page_of_view == transition.target_page) {
        x_offset += transition.progress * page_width * dir;
      }
    }
  } else {
    const int page_height = grid_size.height();
    if (page_of_view < current_page)
      y_offset = -page_height;
    else if (page_of_view > current_page)
      y_offset = page_height;

    if (is_valid) {
      if (page_of_view == current_page ||
          page_of_view == transition.target_page) {
        y_offset += transition.progress * page_height * dir;
      }
    }

    // Adjust pages with bottom 56px spaces to have 48px page break space, but
    // do not over adjust for ideal offset.
    if (page_of_view > current_page && current_page >= 1)
      y_offset = std::max(y_offset - kPageBreakSpaceAdjustment, 0);
    else if (page_of_view < current_page && page_of_view >= 1)
      y_offset = std::min(y_offset + kPageBreakSpaceAdjustment, 0);
  }

  return gfx::Vector2d(x_offset, y_offset);
}

void AppsGridView::CalculateIdealBounds() {
  const int total_views =
      view_model_.view_size() + pulsing_blocks_model_.view_size();
  int slot_index = 0;
  for (int i = 0; i < total_views; ++i) {
    if (i < view_model_.view_size() && view_model_.view_at(i) == drag_view_)
      continue;

    Index view_index = GetIndexFromModelIndex(slot_index);

    // Leaves a blank space in the grid for the current reorder placeholder.
    if (reorder_placeholder_ == view_index) {
      ++slot_index;
      view_index = GetIndexFromModelIndex(slot_index);
    }

    gfx::Rect tile_slot = GetExpectedTileBounds(view_index);
    tile_slot.Offset(CalculateTransitionOffset(view_index.page));
    if (i < view_model_.view_size()) {
      view_model_.set_ideal_bounds(i, tile_slot);
    } else {
      pulsing_blocks_model_.set_ideal_bounds(i - view_model_.view_size(),
                                             tile_slot);
    }

    ++slot_index;
  }
}

void AppsGridView::AnimateToIdealBounds() {
  const gfx::Rect visible_bounds(GetVisibleBounds());

  CalculateIdealBounds();
  for (int i = 0; i < view_model_.view_size(); ++i) {
    AppListItemView* view = GetItemViewAt(i);
    if (view == drag_view_)
      continue;

    const gfx::Rect& target = view_model_.ideal_bounds(i);
    if (bounds_animator_.GetTargetBounds(view) == target)
      continue;

    const gfx::Rect& current = view->bounds();
    const bool current_visible = visible_bounds.Intersects(current);
    const bool target_visible = visible_bounds.Intersects(target);
    const bool visible = current_visible || target_visible;

    const int y_diff = target.y() - current.y();
    if (visible && y_diff && y_diff % GetTotalTileSize().height() == 0) {
      AnimationBetweenRows(view, current_visible, current, target_visible,
                           target);
    } else if (visible || bounds_animator_.IsAnimating(view)) {
      bounds_animator_.AnimateViewTo(view, target);
      bounds_animator_.SetAnimationDelegate(
          view, std::unique_ptr<gfx::AnimationDelegate>(
                    new ItemMoveAnimationDelegate(view)));
    } else {
      view->SetBoundsRect(target);
    }
  }
}

void AppsGridView::AnimationBetweenRows(AppListItemView* view,
                                        bool animate_current,
                                        const gfx::Rect& current,
                                        bool animate_target,
                                        const gfx::Rect& target) {
  // Determine page of |current| and |target|. -1 means in the left invisible
  // page, 0 is the center visible page and 1 means in the right invisible page.
  const int current_page =
      current.x() < 0 ? -1 : current.x() >= width() ? 1 : 0;
  const int target_page = target.x() < 0 ? -1 : target.x() >= width() ? 1 : 0;

  const int dir = current_page < target_page || (current_page == target_page &&
                                                 current.y() < target.y())
                      ? 1
                      : -1;

  std::unique_ptr<ui::Layer> layer;
  if (animate_current) {
    layer = view->RecreateLayer();
    layer->SuppressPaint();

    view->layer()->SetFillsBoundsOpaquely(false);
    view->layer()->SetOpacity(0.f);
  }

  const gfx::Size total_tile_size = GetTotalTileSize();
  gfx::Rect current_out(current);
  current_out.Offset(dir * total_tile_size.width(), 0);

  gfx::Rect target_in(target);
  if (animate_target)
    target_in.Offset(-dir * total_tile_size.width(), 0);
  view->SetBoundsRect(target_in);
  bounds_animator_.AnimateViewTo(view, target);

  bounds_animator_.SetAnimationDelegate(
      view,
      std::unique_ptr<gfx::AnimationDelegate>(
          new RowMoveAnimationDelegate(view, layer.release(), current_out)));
}

void AppsGridView::ExtractDragLocation(const gfx::Point& root_location,
                                       gfx::Point* drag_point) {
  // Use root location of |event| instead of location in |drag_view_|'s
  // coordinates because |drag_view_| has a scale transform and location
  // could have integer round error and causes jitter.
  *drag_point = root_location;

  DCHECK(GetWidget());
  aura::Window::ConvertPointToTarget(
      GetWidget()->GetNativeWindow()->GetRootWindow(),
      GetWidget()->GetNativeWindow(), drag_point);
  views::View::ConvertPointFromWidget(this, drag_point);
  // Ensure that |drag_point| is correct if RTL.
  drag_point->set_x(GetMirroredXInView(drag_point->x()));
}

void AppsGridView::CalculateDropTarget() {
  DCHECK(drag_view_);

  gfx::Point point = drag_view_->icon()->bounds().CenterPoint();
  views::View::ConvertPointToTarget(drag_view_, this, &point);
  // Ensure that the drop target location is correct if RTL.
  point.set_x(GetMirroredXInView(point.x()));
  if (!IsPointWithinDragBuffer(point)) {
    // Reset the reorder target to the original position if the cursor is
    // outside the drag buffer.
    if (IsDraggingForReparentInRootLevelGridView()) {
      drop_attempt_ = DROP_FOR_NONE;
      return;
    }

    reorder_drop_target_ = drag_view_init_index_;
    drop_attempt_ = DROP_FOR_REORDER;
    return;
  }

  if (EnableFolderDragDropUI() &&
      CalculateFolderDropTarget(point, &folder_drop_target_)) {
    drop_attempt_ = DROP_FOR_FOLDER;
    return;
  }

  drop_attempt_ = DROP_FOR_REORDER;
  CalculateReorderDropTarget(point, &reorder_drop_target_);
}

bool AppsGridView::CalculateFolderDropTarget(const gfx::Point& point,
                                             Index* drop_target) const {
  // Folders can't be dropped into other folders.
  if (IsFolderItem(drag_view_->item()))
    return false;

  // A folder drop shouldn't happen on the reorder placeholder since that would
  // be merging an item with itself.
  Index nearest_tile_index(GetNearestTileIndexForPoint(point));
  if (!IsValidIndex(nearest_tile_index) ||
      nearest_tile_index == reorder_placeholder_) {
    return false;
  }

  int distance_to_tile_center =
      (point - GetExpectedTileBounds(nearest_tile_index).CenterPoint())
          .Length();
  if (distance_to_tile_center > kFolderDroppingCircleRadius)
    return false;

  AppListItemView* target_view =
      GetViewDisplayedAtSlotOnCurrentPage(nearest_tile_index.slot);
  if (!target_view)
    return false;

  AppListItem* target_item = target_view->item();

  // Items can only be dropped into non-folders (which have no children) or
  // folders that have fewer than the max allowed items.
  // The OEM folder does not allow drag/drop of other items into it.
  const size_t kMaxItemCount = kMaxFolderItemsPerPage * kMaxFolderPages;
  if (target_item->ChildItemCount() >= kMaxItemCount ||
      IsOEMFolderItem(target_item)) {
    return false;
  }

  *drop_target = nearest_tile_index;
  DCHECK(IsValidIndex(*drop_target));
  return true;
}

void AppsGridView::CalculateReorderDropTarget(const gfx::Point& point,
                                              Index* drop_target) const {
  gfx::Rect bounds = GetContentsBounds();
  Index grid_index = GetNearestTileIndexForPoint(point);
  gfx::Point reorder_placeholder_center =
      GetExpectedTileBounds(reorder_placeholder_).CenterPoint();

  int x_offset_direction = 0;
  if (grid_index == reorder_placeholder_) {
    x_offset_direction = reorder_placeholder_center.x() <= point.x() ? -1 : 1;
  } else {
    x_offset_direction = reorder_placeholder_ < grid_index ? -1 : 1;
  }

  const gfx::Size total_tile_size = GetTotalTileSize();
  int row = grid_index.slot / cols_;

  // Offset the target column based on the direction of the target. This will
  // result in earlier targets getting their reorder zone shifted backwards
  // and later targets getting their reorder zones shifted forwards.
  //
  // This makes eordering feel like the user is slotting items into the spaces
  // between apps.
  int x_offset = x_offset_direction *
                 (total_tile_size.width() - kFolderDroppingCircleRadius) / 2;
  int col = (point.x() - bounds.x() + x_offset) / total_tile_size.width();
  col = base::ClampToRange(col, 0, cols_ - 1);
  *drop_target =
      std::min(Index(pagination_model_.selected_page(), row * cols_ + col),
               GetLastViewIndex());
  DCHECK(IsValidIndex(*drop_target));
}

void AppsGridView::OnReorderTimer() {
  if (drop_attempt_ == DROP_FOR_REORDER) {
    reorder_placeholder_ = reorder_drop_target_;
    AnimateToIdealBounds();
  }
}

void AppsGridView::OnFolderItemReparentTimer() {
  DCHECK(folder_delegate_);
  if (drag_out_of_folder_container_ && drag_view_) {
    bool has_native_drag = drag_and_drop_host_ != nullptr;
    folder_delegate_->ReparentItem(drag_view_, last_drag_point_,
                                   has_native_drag);

    // Set the flag in the folder's grid view.
    dragging_for_reparent_item_ = true;

    // Do not observe any data change since it is going to be hidden.
    item_list_->RemoveObserver(this);
    item_list_ = nullptr;
  }
}

void AppsGridView::OnFolderDroppingTimer() {
  if (drop_attempt_ == DROP_FOR_FOLDER)
    SetAsFolderDroppingTarget(folder_drop_target_, true);
}

void AppsGridView::UpdateDragStateInsideFolder(Pointer pointer,
                                               const gfx::Point& drag_point) {
  if (IsUnderOEMFolder())
    return;

  if (IsDraggingForReparentInHiddenGridView()) {
    // Dispatch drag event to root level grid view for re-parenting folder
    // folder item purpose.
    DispatchDragEventForReparent(pointer, drag_point);
    return;
  }

  // Calculate if the drag_view_ is dragged out of the folder's container
  // ink bubble.
  gfx::Rect bounds_to_folder_view = ConvertRectToParent(drag_view_->bounds());
  gfx::Point pt = bounds_to_folder_view.CenterPoint();
  bool is_item_dragged_out_of_folder =
      folder_delegate_->IsPointOutsideOfFolderBoundary(pt);
  if (is_item_dragged_out_of_folder) {
    if (!drag_out_of_folder_container_) {
      folder_item_reparent_timer_.Start(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kFolderItemReparentDelay), this,
          &AppsGridView::OnFolderItemReparentTimer);
      drag_out_of_folder_container_ = true;
    }
  } else {
    folder_item_reparent_timer_.Stop();
    drag_out_of_folder_container_ = false;
  }
}

bool AppsGridView::IsDraggingForReparentInRootLevelGridView() const {
  return (!folder_delegate_ && dragging_for_reparent_item_);
}

bool AppsGridView::IsDraggingForReparentInHiddenGridView() const {
  return (folder_delegate_ && dragging_for_reparent_item_);
}

gfx::Rect AppsGridView::GetTargetIconRectInFolder(
    AppListItemView* drag_item_view,
    AppListItemView* folder_item_view) {
  gfx::Rect view_ideal_bounds =
      view_model_.ideal_bounds(view_model_.GetIndexOfView(folder_item_view));
  gfx::Rect icon_ideal_bounds =
      folder_item_view->GetIconBoundsForTargetViewBounds(view_ideal_bounds);
  AppListFolderItem* folder_item =
      static_cast<AppListFolderItem*>(folder_item_view->item());
  return folder_item->GetTargetIconRectInFolderForItem(drag_item_view->item(),
                                                       icon_ideal_bounds);
}

bool AppsGridView::IsUnderOEMFolder() {
  if (!folder_delegate_)
    return false;

  return folder_delegate_->IsOEMFolder();
}

bool AppsGridView::HandleFocusMovementInPeekingState(bool arrow_up) {
  DCHECK(expand_arrow_view_);
  if (expand_arrow_view_->HasFocus())
    return false;
  // In peeking mode, the focus is now in suggestions container.
  if (arrow_up)
    contents_view_->GetSearchBoxView()->search_box()->RequestFocus();
  else
    expand_arrow_view_->RequestFocus();
  return true;
}

bool AppsGridView::HandleFocusMovementInFullscreenAllAppsState(bool arrow_up) {
  // The global index is the index of focused app in all pages, assuming
  // all pages except last one have |cols_*rows_per_page_| apps, including
  // apps in both |view_model_| and |suggestions_container_|. For example, if
  // the focused app is the n-th app in p-th page, the global index is
  // |(p-1)*(cols_*rows_per_page_)+(n-1)|.
  int global_index;

  // |suggestions_container_| does not exist in folder view.
  bool has_suggestions_app =
      suggestions_container_ && suggestions_container_->num_results() > 0;
  const int suggestions_app_max_num = has_suggestions_app ? cols_ : 0;

  // Calculate current global focus index.
  views::View* focused_view = GetFocusManager()->GetFocusedView();
  if (has_suggestions_app && suggestions_container_->Contains(focused_view)) {
    const std::vector<SearchResultTileItemView*>& tile_views =
        suggestions_container_->tile_views();
    global_index =
        std::find(tile_views.begin(), tile_views.end(), focused_view) -
        tile_views.begin();
    DCHECK(global_index < cols_);
  } else {
    global_index = view_model_.GetIndexOfView(
                       static_cast<const AppListItemView*>(focused_view)) +
                   suggestions_app_max_num;
  }

  // Calculate target global focus index.
  const int tile_total = view_model_.view_size() + suggestions_app_max_num;
  const int row_total = tile_total / cols_ + ((tile_total % cols_) ? 1 : 0);
  int target_global_index = global_index + (arrow_up ? -cols_ : cols_);
  if (target_global_index >= tile_total &&
      target_global_index < cols_ * row_total) {
    // Target index is on last row, so set it to last app's global index.
    target_global_index = tile_total - 1;
  } else if (has_suggestions_app && target_global_index >= 0 &&
             target_global_index < cols_) {
    // Target index is in |suggestions_container|, set it to last suggestion
    // app's index if it is outside range.
    target_global_index = std::min(suggestions_container_->num_results() - 1,
                                   target_global_index);
  }

  // Move focus based on target global focus index.
  if (folder_delegate_ &&
      (target_global_index < 0 || target_global_index >= cols_ * row_total ||
       target_global_index / kMaxFolderItemsPerPage !=
           global_index / kMaxFolderItemsPerPage)) {
    // The pagination inside a folder is set horizontally, so focus should be
    // set on the search box when it is moved up outside the current page and
    // should be set on the folder title when it is moved down outside the
    // current page.
    if (arrow_up) {
      contents_view_->GetSearchBoxView()->search_box()->RequestFocus();
    } else {
      contents_view_->GetAppsContainerView()
          ->app_list_folder_view()
          ->folder_header_view()
          ->SetTextFocus();
    }
    return true;
  }

  if (target_global_index < 0 || target_global_index >= cols_ * row_total) {
    // Target index is outside apps grid view.
    contents_view_->GetSearchBoxView()->search_box()->RequestFocus();
  } else if (has_suggestions_app &&
             target_global_index < suggestions_container_->num_results()) {
    suggestions_container_->tile_views()[target_global_index]->RequestFocus();
  } else {
    view_model_.view_at(target_global_index - suggestions_app_max_num)
        ->RequestFocus();
  }
  return true;
}

void AppsGridView::UpdateColsAndRowsForFolder() {
  if (!folder_delegate_ || !item_list_->item_count())
    return;

  // Try to shape the apps grid into a square.
  int items_in_one_page =
      std::min(kMaxFolderItemsPerPage, item_list_->item_count());
  cols_ = std::sqrt(items_in_one_page - 1) + 1;
  rows_per_page_ = (items_in_one_page - 1) / cols_ + 1;
}

size_t AppsGridView::GetAppListItemViewIndexOffset() const {
  if (folder_delegate_)
    return 0;

  // The first app list item view must be right behind the expand arrow view.
  DCHECK(expand_arrow_view_);
  return GetIndexOf(expand_arrow_view_) + 1;
}

void AppsGridView::DispatchDragEventForReparent(Pointer pointer,
                                                const gfx::Point& drag_point) {
  folder_delegate_->DispatchDragEventForReparent(pointer, drag_point);
}

void AppsGridView::EndDragFromReparentItemInRootLevel(
    bool events_forwarded_to_drag_drop_host,
    bool cancel_drag) {
  // EndDrag was called before if |drag_view_| is NULL.
  if (!drag_view_)
    return;

  DCHECK(IsDraggingForReparentInRootLevelGridView());
  bool cancel_reparent = cancel_drag || drop_attempt_ == DROP_FOR_NONE;
  if (!events_forwarded_to_drag_drop_host && !cancel_reparent) {
    CalculateDropTarget();
    if (drop_attempt_ == DROP_FOR_REORDER &&
        IsValidIndex(reorder_drop_target_)) {
      ReparentItemForReorder(drag_view_, reorder_drop_target_);
    } else if (drop_attempt_ == DROP_FOR_FOLDER &&
               IsValidIndex(folder_drop_target_)) {
      cancel_reparent =
          !ReparentItemToAnotherFolder(drag_view_, folder_drop_target_);
    } else {
      NOTREACHED();
    }
    SetViewHidden(drag_view_, false /* show */, true /* no animate */);
  }

  SetAsFolderDroppingTarget(folder_drop_target_, false);
  if (cancel_reparent) {
    CancelFolderItemReparent(drag_view_);
  } else {
    // By setting |drag_view_| to NULL here, we prevent ClearDragState() from
    // cleaning up the newly created AppListItemView, effectively claiming
    // ownership of the newly created drag view.
    drag_view_->OnDragEnded();
    drag_view_ = nullptr;
  }
  ClearDragState();
  AnimateToIdealBounds();

  StopPageFlipTimer();
}

void AppsGridView::EndDragForReparentInHiddenFolderGridView() {
  if (drag_and_drop_host_) {
    // If we had a drag and drop proxy icon, we delete it and make the real
    // item visible again.
    drag_and_drop_host_->DestroyDragIconProxy();
  }

  SetAsFolderDroppingTarget(folder_drop_target_, false);
  ClearDragState();
}

void AppsGridView::OnFolderItemRemoved() {
  DCHECK(folder_delegate_);
  if (item_list_)
    item_list_->RemoveObserver(this);
  item_list_ = nullptr;
}

void AppsGridView::UpdateOpacity() {
  int app_list_y_position_in_screen =
      contents_view_->app_list_view()->app_list_y_position_in_screen();
  AppListView* app_list_view = contents_view_->app_list_view();
  int screen_bottom = app_list_view->GetScreenBottom();
  bool should_restore_opacity =
      !app_list_view->is_in_drag() &&
      (app_list_view->app_list_state() != AppListViewState::CLOSED);

  // The opacity of suggested apps is a function of the fractional displacement
  // of the app list from collapsed(0) to peeking(1) state. When the fraction
  // changes from |kSuggestedAppsOpacityStartFraction| to
  // |kSuggestedAppsOpacityEndFraction|, the opacity of suggested apps changes
  // from 0.f to 1.0f.
  float fraction =
      std::max<float>(
          screen_bottom - kShelfSize - app_list_y_position_in_screen, 0) /
      (kPeekingAppListHeight - kShelfSize);
  float opacity =
      std::min(std::max((fraction - kSuggestedAppsOpacityStartFraction) /
                            (kSuggestedAppsOpacityEndFraction -
                             kSuggestedAppsOpacityStartFraction),
                        0.f),
               1.0f);
  if (suggestions_container_) {
    suggestions_container_->layer()->SetOpacity(
        should_restore_opacity ? 1.0f : opacity);
  }

  // The opacity of expand arrow during dragging from collapsed(0) to peeking(1)
  // state. When the dragging amount fraction changes from
  // |kExpandArrowShowStartFraction| to |kExpandArrowShowEndFraction|, the
  // opacity changes from 0.f to 1.0f.
  float arrow_peeking_opacity = std::min(
      std::max(
          (fraction + kExpandArrowShowEndFraction -
           kExpandArrowShowStartFraction - 1.0f) /
              (kExpandArrowShowEndFraction - kExpandArrowShowStartFraction),
          0.f),
      1.0f);

  // The opacity of all apps indicator is a function of the fractional
  // displacement of the app list from peeking(0) to fullscreen(1) state. When
  // the fraction changes from |kAllAppsIndicatorOpacityStartFraction| to
  // |kAllAppsIndicatorOpacityEndFraction|, the opacity of all apps indicator
  // changes from 0.f to 1.0f.
  float peeking_to_fullscreen_height =
      contents_view_->GetDisplayHeight() - kPeekingAppListHeight;
  float drag_amount =
      screen_bottom - app_list_y_position_in_screen - kPeekingAppListHeight;
  fraction = std::max(drag_amount / peeking_to_fullscreen_height, 0.f);
  opacity = std::min(std::max((fraction + kAllAppsIndicatorOpacityEndFraction -
                               kAllAppsIndicatorOpacityStartFraction - 1.0f) /
                                  (kAllAppsIndicatorOpacityEndFraction -
                                   kAllAppsIndicatorOpacityStartFraction),
                              0.f),
                     1.0f);
  if (all_apps_indicator_) {
    all_apps_indicator_->layer()->SetOpacity(should_restore_opacity ? 1.0f
                                                                    : opacity);
  }

  // The opacity of expand arrow during dragging from peeking(0) to
  // fullscreen(1) state. When the dragging amount fraction changes from
  // |kExpandArrowDismissStartFraction| to |kExpandArrowDismissEndFraction|, the
  // opacity changes from 1.0f to 0.f;
  float arrow_fullscreen_opacity =
      std::min(std::max((fraction - kExpandArrowDismissEndFraction) /
                            (kExpandArrowDismissStartFraction -
                             kExpandArrowDismissEndFraction),
                        0.f),
               1.0f);
  if (expand_arrow_view_) {
    if (app_list_y_position_in_screen <
        (screen_bottom - kPeekingAppListHeight)) {
      expand_arrow_view_->layer()->SetOpacity(
          should_restore_opacity ? 1.0f : arrow_fullscreen_opacity);
    } else {
      expand_arrow_view_->layer()->SetOpacity(
          should_restore_opacity ? 1.0f : arrow_peeking_opacity);
    }
  }

  // Updates the opacity of all apps. The opacity of the app starting at 0.f
  // when the ceterline of the app is |kAllAppsOpacityStartPx| above the bottom
  // of work area and transitioning to 1.0f by the time the centerline reaches
  // |kAllAppsOpacityEndPx| above the work area bottom.
  float centerline_above_work_area = 0.f;
  const float drag_amount_above_peeking =
      screen_bottom - app_list_y_position_in_screen - kPeekingAppListHeight;
  const float opacity_factor = drag_amount_above_peeking / kShelfSize;
  for (int i = 0; i < view_model_.view_size(); ++i) {
    AppListItemView* item_view = GetItemViewAt(i);
    if (item_view == drag_view_)
      continue;

    gfx::Rect view_bounds = view_model_.ideal_bounds(i);
    views::View::ConvertRectToScreen(this, &view_bounds);
    centerline_above_work_area =
        std::max<float>(screen_bottom - view_bounds.CenterPoint().y(), 0.f);
    opacity = std::min(
        std::max((centerline_above_work_area - kAllAppsOpacityStartPx) /
                     (kAllAppsOpacityEndPx - kAllAppsOpacityStartPx),
                 0.f),
        1.0f);

    // The first row of apps should be shown gradually if start with dragging up
    // from PEEKING and should not be shown if start with dragging down from
    // PEEKING.
    Index index = GetIndexOfView(item_view);
    if ((index.page == 0 && index.slot < cols_ &&
         contents_view_->app_list_view()->drag_started_from_peeking()) &&
        ((drag_amount_above_peeking >= 0 &&
          drag_amount_above_peeking <= kShelfSize) ||
         (drag_amount_above_peeking < 0 &&
          centerline_above_work_area >= kAllAppsOpacityStartPx))) {
      opacity = std::max(opacity * opacity_factor, 0.f);
    }

    item_view->layer()->SetOpacity(should_restore_opacity ? 1.0f : opacity);
  }
}

bool AppsGridView::HandleScrollFromAppListView(int offset, ui::EventType type) {
  // Scroll up at first page should close the launcher.
  if (offset > 0 && !pagination_model()->IsValidPageRelative(-1))
    return false;

  HandleScroll(offset, type);
  return true;
}

bool AppsGridView::IsEventNearAppIcon(const ui::LocatedEvent& event) {
  // Convert the event location to AppsGridView coordinates.
  std::unique_ptr<ui::Event> cloned_event = ui::Event::Clone(event);
  ui::LocatedEvent* cloned_located_event = cloned_event->AsLocatedEvent();
  event.target()->ConvertEventToTarget(this, cloned_located_event);
  const gfx::Point point_in_apps_grid_view = cloned_located_event->location();

  // GetNearestTileIndexForPoint will always return a slot, even if the point is
  // outside of this.
  if (!bounds().Contains(point_in_apps_grid_view))
    return false;

  return GetViewDisplayedAtSlotOnCurrentPage(
      GetNearestTileIndexForPoint(point_in_apps_grid_view).slot);
}

AppListItemView* AppsGridView::GetCurrentPageFirstItemViewInFolder() {
  DCHECK(folder_delegate_);
  int first_index = pagination_model_.selected_page() * kMaxFolderItemsPerPage;
  return view_model_.view_at(first_index);
}

AppListItemView* AppsGridView::GetCurrentPageLastItemViewInFolder() {
  DCHECK(folder_delegate_);
  int last_index = std::min(
      (pagination_model_.selected_page() + 1) * kMaxFolderItemsPerPage - 1,
      item_list_->item_count() - 1);
  return view_model_.view_at(last_index);
}

void AppsGridView::StartDragAndDropHostDrag(const gfx::Point& grid_location) {
  // When a drag and drop host is given, the item can be dragged out of the app
  // list window. In that case a proxy widget needs to be used.
  // Note: This code has very likely to be changed for Windows (non metro mode)
  // when a |drag_and_drop_host_| gets implemented.
  if (!drag_view_ || !drag_and_drop_host_)
    return;

  gfx::Point screen_location = grid_location;
  views::View::ConvertPointToScreen(this, &screen_location);

  // Determine the mouse offset to the center of the icon so that the drag and
  // drop host follows this layer.
  gfx::Vector2d delta =
      drag_view_offset_ - drag_view_->GetLocalBounds().CenterPoint();
  delta.set_y(delta.y() + drag_view_->title()->size().height() / 2);

  // We have to hide the original item since the drag and drop host will do
  // the OS dependent code to "lift off the dragged item".
  DCHECK(!IsDraggingForReparentInRootLevelGridView());
  drag_and_drop_host_->CreateDragIconProxyByLocationWithNoAnimation(
      drag_view_->icon()->GetBoundsInScreen().origin(),
      drag_view_->item()->icon(), drag_view_, kDragAndDropProxyScale);

  SetViewHidden(drag_view_, true /* hide */, true /* no animation */);
}

void AppsGridView::DispatchDragEventToDragAndDropHost(
    const gfx::Point& location_in_screen_coordinates) {
  if (!drag_view_ || !drag_and_drop_host_)
    return;

  if (GetLocalBounds().Contains(last_drag_point_)) {
    // The event was issued inside the app menu and we should get all events.
    if (forward_events_to_drag_and_drop_host_) {
      // The DnD host was previously called and needs to be informed that the
      // session returns to the owner.
      forward_events_to_drag_and_drop_host_ = false;
      drag_and_drop_host_->EndDrag(true);
    }
  } else {
    if (IsFolderItem(drag_view_->item()))
      return;

    // The event happened outside our app menu and we might need to dispatch.
    if (forward_events_to_drag_and_drop_host_) {
      // Dispatch since we have already started.
      if (!drag_and_drop_host_->Drag(location_in_screen_coordinates)) {
        // The host is not active any longer and we cancel the operation.
        forward_events_to_drag_and_drop_host_ = false;
        drag_and_drop_host_->EndDrag(true);
      }
    } else {
      if (drag_and_drop_host_->StartDrag(drag_view_->item()->id(),
                                         location_in_screen_coordinates)) {
        // From now on we forward the drag events.
        forward_events_to_drag_and_drop_host_ = true;
        // Any flip operations are stopped.
        StopPageFlipTimer();
      }
    }
  }
}

void AppsGridView::MaybeStartPageFlipTimer(const gfx::Point& drag_point) {
  if (!IsPointWithinPageFlipBuffer(drag_point))
    StopPageFlipTimer();
  int new_page_flip_target = -1;

  // Drag zones are at the edges of the scroll axis.
  if (pagination_controller_->scroll_axis() ==
      PaginationController::SCROLL_AXIS_VERTICAL) {
    if (drag_point.y() < kPageFlipZoneSize) {
      new_page_flip_target = pagination_model_.selected_page() - 1;
    } else if (IsPointWithinBottomDragBuffer(drag_point)) {
      // If the drag point is within the drag buffer, but not over the shelf.
      new_page_flip_target = pagination_model_.selected_page() + 1;
    }
  } else {
    // TODO(xiyuan): Fix this for RTL.
    if (new_page_flip_target == -1 && drag_point.x() < kPageFlipZoneSize)
      new_page_flip_target = pagination_model_.selected_page() - 1;

    if (new_page_flip_target == -1 &&
        drag_point.x() > width() - kPageFlipZoneSize) {
      new_page_flip_target = pagination_model_.selected_page() + 1;
    }
  }

  if (new_page_flip_target == page_flip_target_)
    return;

  StopPageFlipTimer();
  if (pagination_model_.is_valid_page(new_page_flip_target)) {
    page_flip_target_ = new_page_flip_target;

    if (page_flip_target_ != pagination_model_.selected_page()) {
      page_flip_timer_.Start(
          FROM_HERE, base::TimeDelta::FromMilliseconds(page_flip_delay_in_ms_),
          this, &AppsGridView::OnPageFlipTimer);
    }
  }
}

void AppsGridView::OnPageFlipTimer() {
  DCHECK(pagination_model_.is_valid_page(page_flip_target_));
  pagination_model_.SelectPage(page_flip_target_, true);
  UMA_HISTOGRAM_ENUMERATION(kAppListPageSwitcherSourceHistogram,
                            kDragAppToBorder, kMaxAppListPageSwitcherSource);
}

void AppsGridView::MoveItemInModel(AppListItemView* item_view,
                                   const Index& target) {
  int current_model_index = view_model_.GetIndexOfView(item_view);
  DCHECK_GE(current_model_index, 0);

  int target_model_index = GetModelIndexFromIndex(target);

  // Reorder the app list item views in accordance with |view_model_|.
  ReorderChildView(item_view,
                   GetAppListItemViewIndexOffset() + target_model_index);

  if (target_model_index == current_model_index)
    return;

  item_list_->RemoveObserver(this);
  item_list_->MoveItem(current_model_index, target_model_index);
  view_model_.Move(current_model_index, target_model_index);
  item_list_->AddObserver(this);

  if (pagination_model_.selected_page() != target.page)
    pagination_model_.SelectPage(target.page, false);
}

void AppsGridView::MoveItemToFolder(AppListItemView* item_view,
                                    const Index& target) {
  const std::string& source_item_id = item_view->item()->id();
  AppListItemView* target_view =
      GetViewDisplayedAtSlotOnCurrentPage(target.slot);
  DCHECK(target_view);
  const std::string& target_view_item_id = target_view->item()->id();

  // Check that the item is not being dropped onto itself; this should not
  // happen, but it can if something allows multiple views to share an
  // item (e.g., if a folder drop does not clean up properly).
  DCHECK_NE(source_item_id, target_view_item_id);

  // Make change to data model.
  item_list_->RemoveObserver(this);
  std::string folder_item_id =
      model_->MergeItems(target_view_item_id, source_item_id);
  item_list_->AddObserver(this);
  if (folder_item_id.empty()) {
    LOG(ERROR) << "Unable to merge into item id: " << target_view_item_id;
    return;
  }
  if (folder_item_id != target_view_item_id) {
    // New folder was created, change the view model to replace the old target
    // view with the new folder item view.
    size_t folder_item_index;
    if (item_list_->FindItemIndex(folder_item_id, &folder_item_index)) {
      int target_view_index = view_model_.GetIndexOfView(target_view);
      gfx::Rect target_view_bounds = target_view->bounds();
      DeleteItemViewAtIndex(target_view_index);
      AppListItemView* target_folder_view =
          CreateViewForItemAtIndex(folder_item_index);
      target_folder_view->SetBoundsRect(target_view_bounds);
      view_model_.Add(target_folder_view, target_view_index);

      // use |folder_item_index| instead of |target_view_index| because the
      // dragged item has not yet been removed from |view_model_| and
      // |target_view_index| is 1 greater than |folder_item_index| if target
      // item is behind the dragged item.
      AddChildViewAt(target_folder_view,
                     GetAppListItemViewIndexOffset() + folder_item_index);
    } else {
      LOG(ERROR) << "Folder no longer in item_list: " << folder_item_id;
    }
  }

  // Fade out the drag_view_ and delete it when animation ends.
  int drag_view_index = view_model_.GetIndexOfView(drag_view_);
  view_model_.Remove(drag_view_index);
  bounds_animator_.AnimateViewTo(drag_view_, drag_view_->bounds());
  bounds_animator_.SetAnimationDelegate(
      drag_view_, std::unique_ptr<gfx::AnimationDelegate>(
                      new ItemRemoveAnimationDelegate(drag_view_)));
  UpdateColsAndRowsForFolder();
  UpdatePaging();
}

void AppsGridView::ReparentItemForReorder(AppListItemView* item_view,
                                          const Index& target) {
  item_list_->RemoveObserver(this);
  model_->RemoveObserver(this);

  AppListItem* reparent_item = item_view->item();
  DCHECK(reparent_item->IsInFolder());
  const std::string source_folder_id = reparent_item->folder_id();
  AppListFolderItem* source_folder =
      static_cast<AppListFolderItem*>(item_list_->FindItem(source_folder_id));

  int target_model_index = GetModelIndexFromIndex(target);

  // Remove the source folder view if there is only 1 item in it, since the
  // source folder will be deleted after its only child item removed from it.
  if (source_folder->ChildItemCount() == 1u) {
    const int deleted_folder_index =
        view_model_.GetIndexOfView(activated_folder_item_view());
    DeleteItemViewAtIndex(deleted_folder_index);

    // Adjust |target_model_index| if it is beyond the deleted folder index.
    if (target_model_index > deleted_folder_index)
      --target_model_index;
  }

  // Move the item from its parent folder to top level item list.
  // Must move to target_model_index, the location we expect the target item
  // to be, not the item location we want to insert before.
  int current_model_index = view_model_.GetIndexOfView(item_view);
  syncer::StringOrdinal target_position;
  if (target_model_index < static_cast<int>(item_list_->item_count()))
    target_position = item_list_->item_at(target_model_index)->position();
  model_->MoveItemToFolderAt(reparent_item, "", target_position);
  view_model_.Move(current_model_index, target_model_index);
  ReorderChildView(item_view,
                   GetAppListItemViewIndexOffset() + target_model_index);

  RemoveLastItemFromReparentItemFolderIfNecessary(source_folder_id);

  item_list_->AddObserver(this);
  model_->AddObserver(this);
  UpdateColsAndRowsForFolder();
  UpdatePaging();
}

bool AppsGridView::ReparentItemToAnotherFolder(AppListItemView* item_view,
                                               const Index& target) {
  DCHECK(IsDraggingForReparentInRootLevelGridView());

  AppListItemView* target_view =
      GetViewDisplayedAtSlotOnCurrentPage(target.slot);
  if (!target_view)
    return false;

  AppListItem* reparent_item = item_view->item();
  DCHECK(reparent_item->IsInFolder());
  const std::string source_folder_id = reparent_item->folder_id();
  AppListFolderItem* source_folder =
      static_cast<AppListFolderItem*>(item_list_->FindItem(source_folder_id));

  AppListItem* target_item = target_view->item();

  // An app is being reparented to its original folder. Just cancel the
  // reparent.
  if (target_item->id() == reparent_item->folder_id())
    return false;

  // Make change to data model.
  item_list_->RemoveObserver(this);

  // Remove the source folder view if there is only 1 item in it, since the
  // source folder will be deleted after its only child item merged into the
  // target item.
  if (source_folder->ChildItemCount() == 1u)
    DeleteItemViewAtIndex(
        view_model_.GetIndexOfView(activated_folder_item_view()));

  // Move item to the target folder.
  std::string target_id_after_merge =
      model_->MergeItems(target_item->id(), reparent_item->id());
  if (target_id_after_merge.empty()) {
    LOG(ERROR) << "Unable to reparent to item id: " << target_item->id();
    item_list_->AddObserver(this);
    return false;
  }

  if (target_id_after_merge != target_item->id()) {
    // New folder was created, change the view model to replace the old target
    // view with the new folder item view.
    const std::string& new_folder_id = reparent_item->folder_id();
    size_t new_folder_index;
    if (item_list_->FindItemIndex(new_folder_id, &new_folder_index)) {
      // Save the target view's bounds before deletion, which will be used as
      // new folder view's bounds.
      gfx::Rect target_rect = target_view->bounds();
      int target_view_index = view_model_.GetIndexOfView(target_view);
      DeleteItemViewAtIndex(target_view_index);
      AppListItemView* new_folder_view =
          CreateViewForItemAtIndex(new_folder_index);
      new_folder_view->SetBoundsRect(target_rect);
      view_model_.Add(new_folder_view, target_view_index);
      AddChildViewAt(new_folder_view,
                     GetAppListItemViewIndexOffset() + new_folder_index);
    } else {
      LOG(ERROR) << "Folder no longer in item_list: " << new_folder_id;
    }
  }

  RemoveLastItemFromReparentItemFolderIfNecessary(source_folder_id);

  item_list_->AddObserver(this);

  // Fade out the drag_view_ and delete it when animation ends.
  int drag_view_index = view_model_.GetIndexOfView(drag_view_);
  view_model_.Remove(drag_view_index);
  bounds_animator_.AnimateViewTo(drag_view_, drag_view_->bounds());
  bounds_animator_.SetAnimationDelegate(
      drag_view_, std::unique_ptr<gfx::AnimationDelegate>(
                      new ItemRemoveAnimationDelegate(drag_view_)));
  UpdateColsAndRowsForFolder();
  UpdatePaging();

  return true;
}

// After moving the re-parenting item out of the folder, if there is only 1 item
// left, remove the last item out of the folder, delete the folder and insert it
// to the data model at the same position. Make the same change to view_model_
// accordingly.
void AppsGridView::RemoveLastItemFromReparentItemFolderIfNecessary(
    const std::string& source_folder_id) {
  AppListFolderItem* source_folder =
      static_cast<AppListFolderItem*>(item_list_->FindItem(source_folder_id));
  if (!source_folder || source_folder->ChildItemCount() != 1u)
    return;

  // Save the folder item view's bounds before deletion, which will be used as
  // last item view's bounds.
  gfx::Rect folder_rect = activated_folder_item_view()->bounds();
  // Delete view associated with the folder item to be removed.
  DeleteItemViewAtIndex(
      view_model_.GetIndexOfView(activated_folder_item_view()));

  // Now make the data change to remove the folder item in model.
  AppListItem* last_item = source_folder->item_list()->item_at(0);
  model_->MoveItemToFolderAt(last_item, "", source_folder->position());

  // Create a new item view for the last item in folder.
  size_t last_item_index;
  if (!item_list_->FindItemIndex(last_item->id(), &last_item_index) ||
      last_item_index > static_cast<size_t>(view_model_.view_size())) {
    NOTREACHED();
    return;
  }
  AppListItemView* last_item_view = CreateViewForItemAtIndex(last_item_index);
  last_item_view->SetBoundsRect(folder_rect);
  view_model_.Add(last_item_view, last_item_index);
  AddChildViewAt(last_item_view,
                 GetAppListItemViewIndexOffset() + last_item_index);
}

void AppsGridView::CancelFolderItemReparent(AppListItemView* drag_item_view) {
  // The icon of the dragged item must target to its final ideal bounds after
  // the animation completes.
  CalculateIdealBounds();

  gfx::Rect target_icon_rect =
      GetTargetIconRectInFolder(drag_item_view, activated_folder_item_view_);

  gfx::Rect drag_view_rect = drag_item_view->bounds();
  TopIconAnimationView* icon_view = new TopIconAnimationView(
      drag_item_view->item()->icon(),
      base::UTF8ToUTF16(drag_item_view->item()->GetDisplayName()),
      target_icon_rect, false, true); /* animate like closing folder */
  AddChildView(icon_view);
  icon_view->SetBoundsRect(drag_view_rect);
  icon_view->TransformView();
}

void AppsGridView::CancelContextMenusOnCurrentPage() {
  Index start_index(pagination_model_.selected_page(), 0);
  int start = GetModelIndexFromIndex(start_index);
  int end =
      std::min(view_model_.view_size(), start + TilesPerPage(start_index.page));
  for (int i = start; i < end; ++i)
    GetItemViewAt(i)->CancelContextMenu();
}

void AppsGridView::DeleteItemViewAtIndex(int index) {
  AppListItemView* item_view = GetItemViewAt(index);
  view_model_.Remove(index);
  if (item_view == drag_view_)
    drag_view_ = nullptr;
  delete item_view;
}

bool AppsGridView::IsPointWithinDragBuffer(const gfx::Point& point) const {
  gfx::Rect rect(GetLocalBounds());
  rect.Inset(-kDragBufferPx, -kDragBufferPx, -kDragBufferPx, -kDragBufferPx);
  return rect.Contains(point);
}

bool AppsGridView::IsPointWithinPageFlipBuffer(const gfx::Point& point) const {
  gfx::Point point_in_screen = point;
  ConvertPointToScreen(this, &point_in_screen);
  const display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(
          GetWidget()->GetNativeView());
  return display.work_area().Contains(point_in_screen);
}

bool AppsGridView::IsPointWithinBottomDragBuffer(
    const gfx::Point& point) const {
  gfx::Point point_in_screen = point;
  ConvertPointToScreen(this, &point_in_screen);
  const display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(
          GetWidget()->GetNativeView());

  const int kBottomDragBufferMin =
      GetBoundsInScreen().bottom() - kPageFlipZoneSize;
  const int kBottomDragBufferMax =
      display.bounds().bottom() -
      (contents_view_->app_list_view()->is_side_shelf() ? 0 : kShelfSize);

  return point_in_screen.y() > kBottomDragBufferMin &&
         point_in_screen.y() < kBottomDragBufferMax;
}

void AppsGridView::ButtonPressed(views::Button* sender,
                                 const ui::Event& event) {
  if (dragging())
    return;

  if (strcmp(sender->GetClassName(), AppListItemView::kViewClassName))
    return;

  // Always set the previous activated_folder_item_view_ to be visible. This
  // prevents a case where the item would remain hidden due the
  // |activated_folder_item_view_| changing during the animation. We only
  // need to track |activated_folder_item_view_| in the root level grid view.
  AppListItemView* pressed_item_view = static_cast<AppListItemView*>(sender);
  if (!folder_delegate_) {
    if (activated_folder_item_view_)
      activated_folder_item_view_->SetVisible(true);
    if (IsFolderItem(pressed_item_view->item()))
      activated_folder_item_view_ = pressed_item_view;
    else
      activated_folder_item_view_ = nullptr;
  }
  contents_view_->GetAppListMainView()->ActivateApp(pressed_item_view->item(),
                                                    event.flags());
}

void AppsGridView::OnListItemAdded(size_t index, AppListItem* item) {
  EndDrag(true);

  AppListItemView* view = CreateViewForItemAtIndex(index);
  view_model_.Add(view, index);
  AddChildViewAt(view, GetAppListItemViewIndexOffset() + index);

  // Ensure that AppListItems that are added to the AppListItemList are not
  // shown while in PEEKING. The visibility of the app icons will be updated
  // on drag/animation from PEEKING.
  view->SetVisible(model_->state_fullscreen() != AppListViewState::PEEKING);

  UpdateColsAndRowsForFolder();
  UpdatePaging();
  UpdatePulsingBlockViews();
  Layout();
  SchedulePaint();
}

void AppsGridView::OnListItemRemoved(size_t index, AppListItem* item) {
  EndDrag(true);

  DeleteItemViewAtIndex(index);

  UpdateColsAndRowsForFolder();
  UpdatePaging();
  UpdatePulsingBlockViews();
  Layout();
  SchedulePaint();
}

void AppsGridView::OnListItemMoved(size_t from_index,
                                   size_t to_index,
                                   AppListItem* item) {
  EndDrag(true);
  view_model_.Move(from_index, to_index);
  ReorderChildView(view_model_.view_at(to_index),
                   GetAppListItemViewIndexOffset() + to_index);

  UpdateColsAndRowsForFolder();
  UpdatePaging();
  AnimateToIdealBounds();
}

void AppsGridView::OnAppListItemHighlight(size_t index, bool highlight) {
  AppListItemView* view = GetItemViewAt(index);
  view->SetItemIsHighlighted(highlight);
  if (highlight)
    EnsureViewVisible(GetIndexFromModelIndex(index));
}

void AppsGridView::TotalPagesChanged() {}

void AppsGridView::SelectedPageChanged(int old_selected, int new_selected) {
  if (dragging()) {
    CalculateDropTarget();
    Layout();
    MaybeStartPageFlipTimer(last_drag_point_);
  } else {
    ClearSelectedView(selected_view_);
    Layout();
  }
}

void AppsGridView::TransitionStarted() {
  CancelContextMenusOnCurrentPage();
  pagination_animation_start_frame_number_ =
      GetCompositorActivatedFrameCount(layer()->GetCompositor());
}

void AppsGridView::TransitionChanged() {
  // Update layout for valid page transition only since over-scroll no longer
  // animates app icons.
  const PaginationModel::Transition& transition =
      pagination_model_.transition();
  if (pagination_model_.is_valid_page(transition.target_page))
    Layout();
}

void AppsGridView::TransitionEnded() {
  const base::TimeDelta duration =
      pagination_model_.GetTransitionAnimationSlideDuration();

  ui::Compositor* compositor = layer()->GetCompositor();
  // Do not record animation smoothness if |compositor| is nullptr.
  if (!compositor)
    return;

  const int end_frame_number = GetCompositorActivatedFrameCount(compositor);
  if (end_frame_number > pagination_animation_start_frame_number_ &&
      !duration.is_zero()) {
    RecordPaginationAnimationSmoothness(
        end_frame_number - pagination_animation_start_frame_number_,
        duration.InMilliseconds(), compositor->refresh_rate());
  }
}

void AppsGridView::OnAppListModelStatusChanged() {
  UpdatePulsingBlockViews();
  Layout();
  SchedulePaint();
}

void AppsGridView::SetViewHidden(AppListItemView* view,
                                 bool hide,
                                 bool immediate) {
  ui::ScopedLayerAnimationSettings animator(view->layer()->GetAnimator());
  animator.SetPreemptionStrategy(
      immediate ? ui::LayerAnimator::IMMEDIATELY_SET_NEW_TARGET
                : ui::LayerAnimator::REPLACE_QUEUED_ANIMATIONS);
  view->layer()->SetOpacity(hide ? 0 : 1);
}

void AppsGridView::OnImplicitAnimationsCompleted() {
  if (layer()->opacity() == 0.0f)
    SetVisible(false);
}

bool AppsGridView::EnableFolderDragDropUI() {
  // Enable drag and drop folder UI only if it is at the app list root level.
  return !folder_delegate_;
}

AppsGridView::Index AppsGridView::GetNearestTileIndexForPoint(
    const gfx::Point& point) const {
  gfx::Rect bounds = GetContentsBounds();
  const int current_page = pagination_model_.selected_page();
  bounds.Inset(0, GetHeightOnTopOfAllAppsTiles(current_page), 0, 0);
  bounds.Inset(GetTilePadding());
  const gfx::Size total_tile_size = GetTotalTileSize();
  int col = base::ClampToRange(
      (point.x() - bounds.x()) / total_tile_size.width(), 0, cols_ - 1);

  const bool show_suggested_apps = current_page == 0 && !folder_delegate_;
  int row =
      base::ClampToRange((point.y() - bounds.y()) / total_tile_size.height(), 0,
                         rows_per_page_ - (show_suggested_apps ? 2 : 1));
  return Index(current_page, row * cols_ + col);
}

gfx::Size AppsGridView::GetTileGridSize() const {
  if (!folder_delegate_)
    return gfx::Size(kAppsGridPreferredWidth, kHorizontalPagePreferredHeight);

  gfx::Rect rect(GetTotalTileSize());
  rect.set_size(
      gfx::Size(rect.width() * cols_, rect.height() * rows_per_page_));
  rect.Inset(-GetTilePadding());
  return rect.size();
}

int AppsGridView::GetHeightOnTopOfAllAppsTiles(int page) const {
  if (folder_delegate_)
    return 0;

  if (page == 0) {
    DCHECK(suggestions_container_ && all_apps_indicator_);
    return kSearchBoxBottomPadding +
           suggestions_container_->GetPreferredSize().height() +
           kSuggestionsAllAppsIndicatorPadding +
           all_apps_indicator_->GetPreferredSize().height() +
           kAllAppsIndicatorBottomPadding;
  }
  return kSearchBoxBottomPadding;
}

gfx::Rect AppsGridView::GetExpectedTileBounds(const Index& index) const {
  if (!cols_)
    return gfx::Rect();

  gfx::Rect bounds(GetContentsBounds());
  bounds.Inset(0, GetHeightOnTopOfAllAppsTiles(index.page), 0, 0);
  bounds.Inset(GetTilePadding());
  int row = index.slot / cols_;
  int col = index.slot % cols_;
  const gfx::Size total_tile_size = GetTotalTileSize();
  gfx::Rect tile_bounds(gfx::Point(bounds.x() + col * total_tile_size.width(),
                                   bounds.y() + row * total_tile_size.height()),
                        total_tile_size);
  tile_bounds.Inset(-GetTilePadding());
  return tile_bounds;
}

AppListItemView* AppsGridView::GetViewDisplayedAtSlotOnCurrentPage(
    int slot) const {
  if (slot < 0)
    return nullptr;

  // Calculate the original bound of the tile at |index|.
  gfx::Rect tile_rect =
      GetExpectedTileBounds(Index(pagination_model_.selected_page(), slot));

  for (int i = 0; i < view_model_.view_size(); ++i) {
    AppListItemView* view = GetItemViewAt(i);
    if (view->bounds() == tile_rect && view != drag_view_)
      return view;
  }
  return nullptr;
}

void AppsGridView::SetAsFolderDroppingTarget(const Index& target_index,
                                             bool is_target_folder) {
  AppListItemView* target_view =
      GetViewDisplayedAtSlotOnCurrentPage(target_index.slot);
  if (target_view)
    target_view->SetAsAttemptedFolderTarget(is_target_folder);
}

}  // namespace app_list
