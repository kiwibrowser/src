// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_folder_view.h"

#include <algorithm>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/app_list/app_list_metrics.h"
#include "ui/app_list/app_list_util.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/views/app_list_item_view.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/folder_background_view.h"
#include "ui/app_list/views/folder_header_view.h"
#include "ui/app_list/views/page_switcher.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/top_icon_animation_view.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/painter.h"
#include "ui/views/view_model.h"
#include "ui/views/view_model_utils.h"

namespace app_list {

namespace {

constexpr int kFolderBackgroundCornerRadius = 4;
constexpr int kFolderIconCornerRadius = 24;
constexpr int kItemGridsBottomPadding = 24;
constexpr int kFolderPadding = 12;
constexpr int kOnscreenKeyboardTopPadding = 8;

// Indexes of interesting views in ViewModel of AppListFolderView.
constexpr int kIndexBackground = 0;
constexpr int kIndexContentsContainer = 1;
constexpr int kIndexChildItems = 2;
constexpr int kIndexFolderHeader = 3;
constexpr int kIndexPageSwitcher = 4;

int GetCompositorActivatedFrameCount(ui::Compositor* compositor) {
  return compositor ? compositor->activated_frame_count() : 0;
}

// Transit from the background of the folder item's icon to the opened
// folder's background when opening the folder. Transit the other way when
// closing the folder.
class BackgroundAnimation : public gfx::SlideAnimation,
                            public gfx::AnimationDelegate {
 public:
  BackgroundAnimation(bool show, AppListFolderView* folder_view)
      : gfx::SlideAnimation(this), show_(show), folder_view_(folder_view) {
    // Calculate the source and target states.
    from_radius_ =
        show_ ? kFolderIconCornerRadius : kFolderBackgroundCornerRadius;
    to_radius_ =
        show_ ? kFolderBackgroundCornerRadius : kFolderIconCornerRadius;
    from_rect_ = show ? folder_view_->folder_item_icon_bounds()
                      : folder_view_->background_view()->bounds();
    to_rect_ = show ? folder_view_->background_view()->bounds()
                    : folder_view_->folder_item_icon_bounds();
    from_color_ =
        show_ ? FolderImage::kFolderBubbleColor : kCardBackgroundColor;
    to_color_ = show_ ? kCardBackgroundColor : FolderImage::kFolderBubbleColor;

    SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN);
    SetSlideDuration(kFolderTransitionInDurationMs);
  }

  ~BackgroundAnimation() override = default;

 private:
  // gfx::AnimationDelegate
  void AnimationProgressed(const gfx::Animation* animation) override {
    const double progress = animation->GetCurrentValue();
    const int current_radius =
        gfx::Tween::IntValueBetween(progress, from_radius_, to_radius_);
    const SkColor current_color =
        gfx::Tween::ColorValueBetween(progress, from_color_, to_color_);
    const gfx::Rect current_rect = gfx::Tween::RectValueBetween(
        animation->GetCurrentValue(), from_rect_, to_rect_);
    folder_view_->background_view()->SetBoundsRect(current_rect);
    folder_view_->background_view()->SetBackground(
        views::CreateBackgroundFromPainter(
            views::Painter::CreateSolidRoundRectPainter(current_color,
                                                        current_radius)));
    folder_view_->background_view()->SchedulePaint();
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    if (!show_) {
      folder_view_->background_view()->SetBackground(nullptr);
      folder_view_->background_view()->SchedulePaint();
    }
    folder_view_->RecordAnimationSmoothness();
  }

  void AnimationCanceled(const gfx::Animation* animation) override {
    AnimationEnded(animation);
  }

  // True if opening the folder.
  const bool show_;

  // The source and target state of the background's corner radius.
  int from_radius_;
  int to_radius_;

  // The source and target state of the background's bounds.
  gfx::Rect from_rect_;
  gfx::Rect to_rect_;

  // The source and target state of the background's color.
  SkColor from_color_;
  SkColor to_color_;

  AppListFolderView* const folder_view_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(BackgroundAnimation);
};

// Decrease the opacity of the folder item's title when opening the folder.
// Increase it when closing the folder.
class FolderItemTitleAnimation : public gfx::SlideAnimation,
                                 public gfx::AnimationDelegate {
 public:
  FolderItemTitleAnimation(bool show, AppListFolderView* folder_view)
      : gfx::SlideAnimation(this), show_(show), folder_view_(folder_view) {
    // Calculate the source and target states.
    from_color_ = show_ ? kGridTitleColor : SK_ColorTRANSPARENT;
    to_color_ = show_ ? SK_ColorTRANSPARENT : kGridTitleColor;

    SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN);
    SetSlideDuration(kFolderTransitionInDurationMs);
  }

  ~FolderItemTitleAnimation() override = default;

 private:
  // gfx::AnimationDelegate
  void AnimationProgressed(const gfx::Animation* animation) override {
    if (!folder_view_->GetActivatedFolderItemView())
      return;
    folder_view_->GetActivatedFolderItemView()->title()->SetEnabledColor(
        gfx::Tween::ColorValueBetween(animation->GetCurrentValue(), from_color_,
                                      to_color_));
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    if (!folder_view_->GetActivatedFolderItemView())
      return;
    folder_view_->GetActivatedFolderItemView()->title()->SetEnabledColor(
        to_color_);
    folder_view_->RecordAnimationSmoothness();
  }

  void AnimationCanceled(const gfx::Animation* animation) override {
    AnimationEnded(animation);
  }

  // True if opening the folder.
  const bool show_;

  // The source and target state of the title's color.
  SkColor from_color_;
  SkColor to_color_;

  AppListFolderView* const folder_view_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(FolderItemTitleAnimation);
};

// Transit from the items within the folder item icon to the same items in the
// opened folder when opening the folder. Transit the other way when closing the
// folder.
class TopIconAnimation : public AppListFolderView::Animation,
                         public TopIconAnimationObserver {
 public:
  TopIconAnimation(bool show,
                   bool hide_for_reparent,
                   AppListFolderView* folder_view)
      : show_(show),
        hide_for_reparent_(hide_for_reparent),
        folder_view_(folder_view) {}

  ~TopIconAnimation() override {
    for (auto* view : top_icon_views_)
      view->RemoveObserver(this);
    top_icon_views_.clear();
  }

  // AppListFolderView::Animation
  void ScheduleAnimation() override {
    // Hide the original items in the folder until the animation ends.
    SetFirstPageItemViewsVisible(false);
    DCHECK(folder_view_->GetActivatedFolderItemView());
    folder_view_->GetActivatedFolderItemView()->icon()->SetVisible(false);

    // Calculate the start and end bounds of the top item icons in the
    // animation.
    std::vector<gfx::Rect> top_item_views_bounds =
        GetTopItemViewsBoundsInFolderIcon();
    std::vector<gfx::Rect> first_page_item_views_bounds =
        GetFirstPageItemViewsBounds();

    top_icon_views_.clear();

    for (size_t i = 0; i < first_page_item_views_bounds.size(); ++i) {
      const AppListItem* top_item =
          folder_view_->folder_item()->item_list()->item_at(i);
      if (top_item->icon().isNull() ||
          (folder_view_->items_grid_view()->drag_view() &&
           top_item == folder_view_->items_grid_view()->drag_view()->item())) {
        // The item being dragged should be excluded.
        continue;
      }

      bool item_in_folder_icon = i < top_item_views_bounds.size();
      gfx::Rect scaled_rect = item_in_folder_icon
                                  ? top_item_views_bounds[i]
                                  : folder_view_->folder_item_icon_bounds();

      TopIconAnimationView* icon_view = new TopIconAnimationView(
          top_item->icon(), base::ASCIIToUTF16(top_item->GetDisplayName()),
          scaled_rect, show_, item_in_folder_icon);

      icon_view->AddObserver(this);
      top_icon_views_.push_back(icon_view);

      // Add the transitional views into child views, and set its bounds to the
      // same location of the item in the folder list view.
      folder_view_->AddChildView(top_icon_views_.back());
      icon_view->SetBoundsRect(first_page_item_views_bounds[i]);
      icon_view->TransformView();
    }
  }

  bool IsAnimationRunning() override { return !top_icon_views_.empty(); }

  // TopIconAnimationObserver
  void OnTopIconAnimationsComplete(TopIconAnimationView* view) override {
    // Clean up the transitional view for which the animation completes.
    view->RemoveObserver(this);
    auto to_delete =
        std::find(top_icon_views_.begin(), top_icon_views_.end(), view);
    DCHECK(to_delete != top_icon_views_.end());
    top_icon_views_.erase(to_delete);

    folder_view_->RecordAnimationSmoothness();

    // An empty list indicates that all animations are done.
    if (!top_icon_views_.empty())
      return;

    // Set top item views visible when opening the folder.
    if (show_)
      SetFirstPageItemViewsVisible(true);

    // Show the folder icon when closing the folder.
    if ((!show_ || hide_for_reparent_) &&
        folder_view_->GetActivatedFolderItemView()) {
      folder_view_->GetActivatedFolderItemView()->icon()->SetVisible(true);
    }
  }

 private:
  std::vector<gfx::Rect> GetTopItemViewsBoundsInFolderIcon() {
    std::vector<gfx::Rect> top_icons_bounds =
        FolderImage::GetTopIconsBounds(folder_view_->folder_item_icon_bounds());
    std::vector<gfx::Rect> top_item_views_bounds;

    for (gfx::Rect bounds : top_icons_bounds) {
      // Calculate the item view's bounds based on the icon bounds.
      int scale = kGridIconDimension / bounds.width();
      bounds.set_y(bounds.y() - kGridIconTopPadding / scale);
      bounds.set_x(bounds.x() -
                   (kGridTileWidth - kGridIconDimension) / 2 / scale);
      bounds.set_size(
          gfx::Size(kGridTileWidth / scale, kGridTileHeight / scale));
      top_item_views_bounds.emplace_back(bounds);
    }
    return top_item_views_bounds;
  }

  void SetFirstPageItemViewsVisible(bool visible) {
    // Items grid view has to be visible in case an item is being reparented, so
    // only set the opacity here.
    folder_view_->items_grid_view()->layer()->SetOpacity(visible ? 1.0f : 0.0f);
  }

  // Get the bounds of the items in the first page of the opened folder relative
  // to AppListFolderView.
  std::vector<gfx::Rect> GetFirstPageItemViewsBounds() {
    std::vector<gfx::Rect> items_bounds;
    const size_t count = std::min(
        kMaxFolderItemsPerPage, folder_view_->folder_item()->ChildItemCount());
    for (size_t i = 0; i < count; ++i) {
      const gfx::Rect rect =
          folder_view_->items_grid_view()->GetItemViewAt(i)->bounds();
      const gfx::Rect to_container =
          folder_view_->items_grid_view()->ConvertRectToParent(rect);
      const gfx::Rect to_folder =
          folder_view_->contents_container()->ConvertRectToParent(to_container);
      items_bounds.emplace_back(to_folder);
    }
    return items_bounds;
  }

  // True if opening the folder.
  const bool show_;

  // True if an item in the folder is being reparented to root grid view.
  const bool hide_for_reparent_;

  AppListFolderView* const folder_view_;  // Not owned.

  std::vector<TopIconAnimationView*> top_icon_views_;

  DISALLOW_COPY_AND_ASSIGN(TopIconAnimation);
};

// Transit from the bounds of the folder item icon to the opened folder's
// bounds and transit opacity from 0 to 1 when opening the folder. Transit the
// other way when closing the folder.
class ContentsContainerAnimation : public AppListFolderView::Animation,
                                   public ui::ImplicitAnimationObserver {
 public:
  ContentsContainerAnimation(bool show,
                             bool hide_for_reparent,
                             AppListFolderView* folder_view)
      : show_(show),
        hide_for_reparent_(hide_for_reparent),
        folder_view_(folder_view) {}

  ~ContentsContainerAnimation() override { StopObservingImplicitAnimations(); }

  // AppListFolderView::Animation
  void ScheduleAnimation() override {
    // Transform used to scale the folder's contents container from the bounds
    // of the folder icon to that of the opened folder.
    gfx::Transform transform;
    const gfx::Rect scaled_rect(folder_view_->folder_item_icon_bounds());
    const gfx::Rect rect(folder_view_->contents_container()->bounds());
    ui::Layer* layer = folder_view_->contents_container()->layer();
    transform.Translate(scaled_rect.x() - rect.x(), scaled_rect.y() - rect.y());
    transform.Scale(static_cast<double>(scaled_rect.width()) / rect.width(),
                    static_cast<double>(scaled_rect.height()) / rect.height());

    if (show_)
      layer->SetTransform(transform);
    layer->SetOpacity(show_ ? 0.0f : 1.0f);

    // The folder should be set visible only after it is scaled down and
    // transparent to prevent the flash of the view right before the animation.
    folder_view_->SetVisible(true);

    ui::ScopedLayerAnimationSettings animation(layer->GetAnimator());
    animation.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN);
    animation.AddObserver(this);
    animation.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kFolderTransitionInDurationMs));
    layer->SetTransform(show_ ? gfx::Transform() : transform);
    layer->SetOpacity(show_ ? 1.0f : 0.0f);

    is_animation_running_ = true;
  }

  bool IsAnimationRunning() override { return is_animation_running_; }

  // ui::ImplicitAnimationObserver
  void OnImplicitAnimationsCompleted() override {
    is_animation_running_ = false;

    // If the view is hidden for reparenting a folder item, it has to be
    // visible, so that drag_view_ can keep receiving mouse events.
    if (!show_ && !hide_for_reparent_)
      folder_view_->SetVisible(false);

    // Set the view bounds to a small rect, so that it won't overlap the root
    // level apps grid view during folder item reprenting transitional period.
    if (hide_for_reparent_) {
      gfx::Rect rect(folder_view_->bounds());
      folder_view_->SetBoundsRect(gfx::Rect(rect.x(), rect.y(), 1, 1));
    }

    // Reset the transform after animation so that the following folder's
    // preferred bounds is calculated correctly.
    folder_view_->contents_container()->layer()->SetTransform(gfx::Transform());
    folder_view_->RecordAnimationSmoothness();
  }

 private:
  // True if opening the folder.
  const bool show_;

  // True if an item in the folder is being reparented to root grid view.
  const bool hide_for_reparent_;

  AppListFolderView* const folder_view_;

  bool is_animation_running_ = false;

  DISALLOW_COPY_AND_ASSIGN(ContentsContainerAnimation);
};

}  // namespace

AppListFolderView::AppListFolderView(AppsContainerView* container_view,
                                     AppListModel* model,
                                     ContentsView* contents_view)
    : container_view_(container_view),
      contents_view_(contents_view),
      background_view_(new views::View),
      contents_container_(new views::View),
      folder_header_view_(new FolderHeaderView(this)),
      view_model_(new views::ViewModel),
      model_(model),
      folder_item_(NULL),
      hide_for_reparent_(false),
      animation_start_frame_number_(0) {
  // The background's corner radius cannot be changed in the same layer of the
  // contents container using layer animation, so use another layer to perform
  // such changes.
  background_view_->SetPaintToLayer();
  background_view_->layer()->SetFillsBoundsOpaquely(false);
  AddChildView(background_view_);
  view_model_->Add(background_view_, kIndexBackground);

  contents_container_->SetPaintToLayer();
  contents_container_->layer()->SetFillsBoundsOpaquely(false);
  AddChildView(contents_container_);
  view_model_->Add(contents_container_, kIndexContentsContainer);

  items_grid_view_ = new AppsGridView(contents_view_, this);
  items_grid_view_->SetModel(model);
  contents_container_->AddChildView(items_grid_view_);
  view_model_->Add(items_grid_view_, kIndexChildItems);

  contents_container_->AddChildView(folder_header_view_);
  view_model_->Add(folder_header_view_, kIndexFolderHeader);

  page_switcher_ = new PageSwitcher(items_grid_view_->pagination_model(),
                                    false /* vertical */);
  contents_container_->AddChildView(page_switcher_);
  view_model_->Add(page_switcher_, kIndexPageSwitcher);

  model_->AddObserver(this);
}

AppListFolderView::~AppListFolderView() {
  model_->RemoveObserver(this);

  // This prevents the AppsGridView's destructor from calling the now-deleted
  // AppListFolderView's methods if a drag is in progress at the time.
  items_grid_view_->set_folder_delegate(nullptr);

  // Make sure |page_switcher_| is deleted before |items_grid_view_| because
  // |page_switcher_| uses the PaginationModel owned by |items_grid_view_|.
  delete page_switcher_;
}

void AppListFolderView::SetAppListFolderItem(AppListFolderItem* folder) {
  accessible_name_ = ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_APP_LIST_FOLDER_OPEN_FOLDER_ACCESSIBILE_NAME);
  NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);

  folder_item_ = folder;
  items_grid_view_->SetItemList(folder_item_->item_list());
  folder_header_view_->SetFolderItem(folder_item_);

  UpdatePreferredBounds();
}

void AppListFolderView::ScheduleShowHideAnimation(bool show,
                                                  bool hide_for_reparent) {
  animation_start_frame_number_ =
      GetCompositorActivatedFrameCount(GetCompositor());

  hide_for_reparent_ = hide_for_reparent;

  items_grid_view_->pagination_model()->SelectPage(0, false);

  // Animate the background corner raidus, opacity and bounds.
  background_animation_ = std::make_unique<BackgroundAnimation>(show, this);
  background_animation_->Show();

  // Animate the folder item's title's opacity.
  folder_item_title_animation_ =
      std::make_unique<FolderItemTitleAnimation>(show, this);
  folder_item_title_animation_->Show();

  // Animate the bounds and opacity of items in the first page of the opened
  // folder.
  top_icon_animation_ =
      std::make_unique<TopIconAnimation>(show, hide_for_reparent, this);
  top_icon_animation_->ScheduleAnimation();

  // Animate the bounds and opacity of the contents container.
  contents_container_animation_ = std::make_unique<ContentsContainerAnimation>(
      show, hide_for_reparent, this);
  contents_container_animation_->ScheduleAnimation();
}

gfx::Size AppListFolderView::CalculatePreferredSize() const {
  gfx::Size size = items_grid_view_->GetTileGridSizeWithoutPadding();
  gfx::Size header_size = folder_header_view_->GetPreferredSize();
  size.Enlarge(0, kItemGridsBottomPadding + header_size.height());
  size.Enlarge(kFolderPadding * 2, kFolderPadding * 2);
  return size;
}

void AppListFolderView::Layout() {
  CalculateIdealBounds();
  views::ViewModelUtils::SetViewBoundsToIdealBounds(*view_model_);
}

bool AppListFolderView::OnKeyPressed(const ui::KeyEvent& event) {
  // Let the FocusManager handle Left/Right keys.
  if (!CanProcessUpDownKeyTraversal(event))
    return false;

  if (folder_header_view_->HasTextFocus() && event.key_code() == ui::VKEY_UP) {
    // Move focus to the last app list item view in the selected page.
    items_grid_view_->GetCurrentPageLastItemViewInFolder()->RequestFocus();
    return true;
  }
  return false;
}

void AppListFolderView::OnAppListItemWillBeDeleted(AppListItem* item) {
  if (item == folder_item_) {
    items_grid_view_->OnFolderItemRemoved();
    folder_header_view_->OnFolderItemRemoved();
    folder_item_ = NULL;

    // Do not change state if it is hidden.
    if (hide_for_reparent_ || contents_container_->layer()->opacity() == 0.0f)
      return;

    // If the folder item associated with this view is removed from the model,
    // (e.g. the last item in the folder was deleted), reset the view and signal
    // the container view to show the app list instead.
    // Pass NULL to ShowApps() to avoid triggering animation from the deleted
    // folder.
    container_view_->ShowApps(NULL);
  }
}

void AppListFolderView::UpdatePreferredBounds() {
  const AppListItemView* activated_folder_item_view =
      GetActivatedFolderItemView();
  DCHECK(activated_folder_item_view);

  // Calculate the folder icon's bounds relative to AppsContainerView.
  gfx::RectF rect(activated_folder_item_view->GetIconBounds());
  ConvertRectToTarget(activated_folder_item_view, container_view_, &rect);
  gfx::Rect icon_bounds_in_container = gfx::ToEnclosingRect(rect);

  // The opened folder view's center should try to overlap with the folder
  // item's center while it must fit within the bounds of AppsContainerView.
  preferred_bounds_ = gfx::Rect(GetPreferredSize());
  preferred_bounds_ += (icon_bounds_in_container.CenterPoint() -
                        preferred_bounds_.CenterPoint());
  preferred_bounds_.AdjustToFit(container_view_->GetContentsBounds());

  keyboard::KeyboardController* const keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (keyboard_controller &&
      contents_view_->app_list_view()->onscreen_keyboard_shown()) {
    // This view should be on top of on-screen keyboard to prevent the folder
    // title from being blocked.
    gfx::Point keyboard_top_right =
        keyboard_controller->GetWorkspaceOccludedBounds().top_right();
    ConvertPointFromScreen(parent(), &keyboard_top_right);
    int y_offset = keyboard_top_right.y() - kOnscreenKeyboardTopPadding -
                   preferred_bounds_.bottom();
    preferred_bounds_.Offset(0, std::min(0, y_offset));
  }

  // Calculate the folder icon's bounds relative to this view.
  folder_item_icon_bounds_ =
      icon_bounds_in_container - preferred_bounds_.OffsetFromOrigin();
}

bool AppListFolderView::IsAnimationRunning() const {
  return top_icon_animation_ && top_icon_animation_->IsAnimationRunning();
}

AppListItemView* AppListFolderView::GetActivatedFolderItemView() {
  return container_view_->apps_grid_view()->activated_folder_item_view();
}

void AppListFolderView::RecordAnimationSmoothness() {
  ui::Compositor* compositor = GetCompositor();
  // Do not record animation smoothness if |compositor| is nullptr.
  if (!compositor)
    return;

  const int end_frame_number = GetCompositorActivatedFrameCount(compositor);
  if (end_frame_number > animation_start_frame_number_) {
    RecordFolderShowHideAnimationSmoothness(
        end_frame_number - animation_start_frame_number_,
        kFolderTransitionInDurationMs, compositor->refresh_rate());
  }
}

void AppListFolderView::CalculateIdealBounds() {
  gfx::Rect rect(GetContentsBounds());
  if (rect.IsEmpty())
    return;

  view_model_->set_ideal_bounds(kIndexBackground, GetContentsBounds());
  view_model_->set_ideal_bounds(kIndexContentsContainer, GetContentsBounds());

  rect.Inset(kFolderPadding, kFolderPadding);

  // Calculate bounds for items grid view.
  gfx::Rect grid_frame(rect);
  grid_frame.Inset(items_grid_view_->GetTilePadding());
  grid_frame.set_height(items_grid_view_->GetPreferredSize().height());
  view_model_->set_ideal_bounds(kIndexChildItems, grid_frame);

  // Calculate bounds for folder header view.
  gfx::Rect header_frame(rect);
  header_frame.set_y(grid_frame.bottom() +
                     items_grid_view_->GetTilePadding().bottom() +
                     kItemGridsBottomPadding);
  header_frame.set_height(folder_header_view_->GetPreferredSize().height());
  view_model_->set_ideal_bounds(kIndexFolderHeader, header_frame);

  // Calculate bounds for page_switcher.
  gfx::Rect page_switcher_frame(rect);
  gfx::Size page_switcher_size = page_switcher_->GetPreferredSize();
  page_switcher_frame.set_x(page_switcher_frame.right() -
                            page_switcher_size.width());
  page_switcher_frame.set_y(header_frame.y());
  page_switcher_frame.set_size(page_switcher_size);
  view_model_->set_ideal_bounds(kIndexPageSwitcher, page_switcher_frame);
}

void AppListFolderView::StartSetupDragInRootLevelAppsGridView(
    AppListItemView* original_drag_view,
    const gfx::Point& drag_point_in_root_grid,
    bool has_native_drag) {
  // Converts the original_drag_view's bounds to the coordinate system of
  // root level grid view.
  gfx::RectF rect_f(original_drag_view->bounds());
  views::View::ConvertRectToTarget(items_grid_view_,
                                   container_view_->apps_grid_view(), &rect_f);
  gfx::Rect rect_in_root_grid_view = gfx::ToEnclosingRect(rect_f);

  container_view_->apps_grid_view()
      ->InitiateDragFromReparentItemInRootLevelGridView(
          original_drag_view, rect_in_root_grid_view, drag_point_in_root_grid,
          has_native_drag);
}

bool AppListFolderView::IsPointOutsideOfFolderBoundary(
    const gfx::Point& point) {
  return !GetLocalBounds().Contains(point);
}

// When user drags a folder item out of the folder boundary ink bubble, the
// folder view UI will be hidden, and switch back to top level AppsGridView.
// The dragged item will seamlessly move on the top level AppsGridView.
// In order to achieve the above, we keep the folder view and its child grid
// view visible with opacity 0, so that the drag_view_ on the hidden grid view
// will keep receiving mouse event. At the same time, we initiated a new
// drag_view_ in the top level grid view, and keep it moving with the hidden
// grid view's drag_view_, so that the dragged item can be engaged in drag and
// drop flow in the top level grid view. During the reparenting process, the
// drag_view_ in hidden grid view will dispatch the drag and drop event to
// the top level grid view, until the drag ends.
void AppListFolderView::ReparentItem(
    AppListItemView* original_drag_view,
    const gfx::Point& drag_point_in_folder_grid,
    bool has_native_drag) {
  // Convert the drag point relative to the root level AppsGridView.
  gfx::Point to_root_level_grid = drag_point_in_folder_grid;
  ConvertPointToTarget(items_grid_view_, container_view_->apps_grid_view(),
                       &to_root_level_grid);
  StartSetupDragInRootLevelAppsGridView(original_drag_view, to_root_level_grid,
                                        has_native_drag);
  container_view_->ReparentFolderItemTransit(folder_item_);
}

void AppListFolderView::DispatchDragEventForReparent(
    AppsGridView::Pointer pointer,
    const gfx::Point& drag_point_in_folder_grid) {
  AppsGridView* root_grid = container_view_->apps_grid_view();
  gfx::Point drag_point_in_root_grid = drag_point_in_folder_grid;

  // Temporarily reset the transform of the contents container so that the point
  // can be correctly converted to the root grid's coordinates.
  gfx::Transform original_transform = contents_container_->GetTransform();
  contents_container_->SetTransform(gfx::Transform());
  ConvertPointToTarget(items_grid_view_, root_grid, &drag_point_in_root_grid);
  contents_container_->SetTransform(original_transform);

  root_grid->UpdateDragFromReparentItem(pointer, drag_point_in_root_grid);
}

void AppListFolderView::DispatchEndDragEventForReparent(
    bool events_forwarded_to_drag_drop_host,
    bool cancel_drag) {
  container_view_->apps_grid_view()->EndDragFromReparentItemInRootLevel(
      events_forwarded_to_drag_drop_host, cancel_drag);
  container_view_->ReparentDragEnded();

  // The view was not hidden in order to keeping receiving mouse events. Hide it
  // now as the reparenting ended.
  HideViewImmediately();
}

void AppListFolderView::HideViewImmediately() {
  SetVisible(false);
  hide_for_reparent_ = false;

  // Transit all the states immediately to the end of folder closing animation.
  background_view_->SetBackground(nullptr);
  background_view_->SchedulePaint();
  AppListItemView* activated_folder_item_view = GetActivatedFolderItemView();
  if (activated_folder_item_view) {
    activated_folder_item_view->icon()->SetVisible(true);
    activated_folder_item_view->title()->SetEnabledColor(kGridTitleColor);
    activated_folder_item_view->title()->SetVisible(true);
  }
}

void AppListFolderView::CloseFolderPage() {
  accessible_name_ = ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_APP_LIST_FOLDER_CLOSE_FOLDER_ACCESSIBILE_NAME);
  NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);

  GiveBackFocusToSearchBox();
  if (items_grid_view()->dragging())
    items_grid_view()->EndDrag(true);
  items_grid_view()->ClearAnySelectedView();
  container_view_->ShowApps(folder_item_);
}

bool AppListFolderView::IsOEMFolder() const {
  return folder_item_->folder_type() == AppListFolderItem::FOLDER_TYPE_OEM;
}

void AppListFolderView::SetRootLevelDragViewVisible(bool visible) {
  container_view_->apps_grid_view()->SetDragViewVisible(visible);
}

void AppListFolderView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->SetName(accessible_name_);
}

void AppListFolderView::NavigateBack(AppListFolderItem* item,
                                     const ui::Event& event_flags) {
  CloseFolderPage();
}

void AppListFolderView::GiveBackFocusToSearchBox() {
  contents_view_->GetSearchBoxView()->search_box()->RequestFocus();
}

void AppListFolderView::SetItemName(AppListFolderItem* item,
                                    const std::string& name) {
  model_->SetItemName(item, name);
}

ui::Compositor* AppListFolderView::GetCompositor() {
  return GetWidget()->GetCompositor();
}

}  // namespace app_list
