// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/scoped_transform_overview_window.h"

#include <algorithm>

#include "ash/public/cpp/ash_features.h"
#include "ash/shell.h"
#include "ash/wm/overview/cleanup_animation_observer.h"
#include "ash/wm/overview/overview_utils.h"
#include "ash/wm/overview/overview_window_animation_observer.h"
#include "ash/wm/overview/scoped_overview_animation_settings.h"
#include "ash/wm/overview/window_grid.h"
#include "ash/wm/overview/window_selector.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/window_mirror_view.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_transient_descendant_iterator.h"
#include "ash/wm/window_util.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_observer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/transform_util.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_util.h"

namespace ash {

namespace {

// When set to true by tests makes closing the widget synchronous.
bool immediate_close_for_tests = false;

// Delay closing window to allow it to shrink and fade out.
constexpr int kCloseWindowDelayInMilliseconds = 150;

// The amount of rounding on window edges in overview mode.
constexpr int kOverviewWindowRoundingDp = 4;

aura::Window* GetTransientRoot(aura::Window* window) {
  while (window && ::wm::GetTransientParent(window))
    window = ::wm::GetTransientParent(window);
  return window;
}

ScopedTransformOverviewWindow::GridWindowFillMode GetWindowDimensionsType(
    aura::Window* window) {
  if (window->bounds().width() >
      window->bounds().height() *
          ScopedTransformOverviewWindow::kExtremeWindowRatioThreshold) {
    return ScopedTransformOverviewWindow::GridWindowFillMode::kLetterBoxed;
  }

  if (window->bounds().height() >
      window->bounds().width() *
          ScopedTransformOverviewWindow::kExtremeWindowRatioThreshold) {
    return ScopedTransformOverviewWindow::GridWindowFillMode::kPillarBoxed;
  }

  return ScopedTransformOverviewWindow::GridWindowFillMode::kNormal;
}

}  // namespace

class ScopedTransformOverviewWindow::LayerCachingAndFilteringObserver
    : public ui::LayerObserver {
 public:
  LayerCachingAndFilteringObserver(ui::Layer* layer) : layer_(layer) {
    layer_->AddObserver(this);
    layer_->AddCacheRenderSurfaceRequest();
    layer_->AddTrilinearFilteringRequest();
  }
  ~LayerCachingAndFilteringObserver() override {
    if (layer_) {
      layer_->RemoveTrilinearFilteringRequest();
      layer_->RemoveCacheRenderSurfaceRequest();
      layer_->RemoveObserver(this);
    }
  }

  // ui::LayerObserver overrides:
  void LayerDestroyed(ui::Layer* layer) override {
    layer_->RemoveObserver(this);
    layer_ = nullptr;
  }

 private:
  ui::Layer* layer_;

  DISALLOW_COPY_AND_ASSIGN(LayerCachingAndFilteringObserver);
};

// WindowMask is applied to overview windows to give them rounded edges while
// they are in overview mode.
class ScopedTransformOverviewWindow::WindowMask : public ui::LayerDelegate,
                                                  public aura::WindowObserver {
 public:
  explicit WindowMask(aura::Window* window)
      : layer_(ui::LAYER_TEXTURED), window_(window) {
    window_->AddObserver(this);
    layer_.set_delegate(this);
    layer_.SetFillsBoundsOpaquely(false);
  }

  ~WindowMask() override {
    if (window_)
      window_->RemoveObserver(this);
    layer_.set_delegate(nullptr);
  }

  void set_top_inset(int top_inset) { top_inset_ = top_inset; }
  ui::Layer* layer() { return &layer_; }

 private:
  // ui::LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override {
    cc::PaintFlags flags;
    flags.setAlpha(255);
    flags.setAntiAlias(true);
    flags.setStyle(cc::PaintFlags::kFill_Style);

    // The amount of round applied on the mask gets scaled as |window_| gets
    // transformed, so reverse the transform so the final scaled round matches
    // |kOverviewWindowRoundingDp|.
    const gfx::Vector2dF scale = window_->transform().Scale2d();
    const SkScalar r_x =
        SkIntToScalar(std::round(kOverviewWindowRoundingDp / scale.x()));
    const SkScalar r_y =
        SkIntToScalar(std::round(kOverviewWindowRoundingDp / scale.y()));

    SkPath path;
    SkScalar radii[8] = {r_x, r_y, r_x, r_y, r_x, r_y, r_x, r_y};
    gfx::Rect bounds(layer()->size());
    bounds.Inset(0, top_inset_, 0, 0);
    path.addRoundRect(gfx::RectToSkRect(bounds), radii);

    ui::PaintRecorder recorder(context, layer()->size());
    recorder.canvas()->DrawPath(path, flags);
  }

  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}

  // aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override {
    layer_.SetBounds(new_bounds);
  }

  void OnWindowDestroying(aura::Window* window) override {
    window_->RemoveObserver(this);
    window_ = nullptr;
  }

  ui::Layer layer_;
  int top_inset_ = 0;
  // Pointer to the window of which this is a mask to.
  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(WindowMask);
};

ScopedTransformOverviewWindow::ScopedTransformOverviewWindow(
    WindowSelectorItem* selector_item,
    aura::Window* window)
    : selector_item_(selector_item),
      window_(window),
      ignored_by_shelf_(wm::GetWindowState(window)->ignored_by_shelf()),
      overview_started_(false),
      original_opacity_(window->layer()->GetTargetOpacity()),
      weak_ptr_factory_(this) {
  type_ = GetWindowDimensionsType(window);
}

ScopedTransformOverviewWindow::~ScopedTransformOverviewWindow() = default;

void ScopedTransformOverviewWindow::RestoreWindow(bool reset_transform) {
  Shell::Get()->shadow_controller()->UpdateShadowForWindow(window_);
  wm::GetWindowState(window_)->set_ignored_by_shelf(ignored_by_shelf_);
  if (minimized_widget_) {
    mask_.reset();
    // Fade out the minimized widget. This animation continues past the
    // lifetime of |this|.
    FadeOutWidgetOnExit(std::move(minimized_widget_),
                        OVERVIEW_ANIMATION_EXIT_OVERVIEW_MODE_FADE_OUT);
    return;
  }

  if (reset_transform) {
    ScopedAnimationSettings animation_settings_list;
    BeginScopedAnimation(selector_item_->GetExitTransformAnimationType(),
                         &animation_settings_list);
    // Use identity transform directly to reset window's transform when exiting
    // overview.
    SetTransform(window()->GetRootWindow(), gfx::Transform());
    // Add requests to cache render surface and perform trilinear filtering for
    // the exit animation of overview mode. The requests will be removed when
    // the exit animation finishes.
    if (features::IsTrilinearFilteringEnabled()) {
      for (auto& settings : animation_settings_list) {
        settings->CacheRenderSurface();
        settings->TrilinearFiltering();
      }
    }
  }

  ScopedOverviewAnimationSettings animation_settings(
      selector_item_->GetExitOverviewAnimationType(), window_);
  SetOpacity(original_opacity_);
}

void ScopedTransformOverviewWindow::BeginScopedAnimation(
    OverviewAnimationType animation_type,
    ScopedAnimationSettings* animation_settings) {
  if (animation_type == OverviewAnimationType::OVERVIEW_ANIMATION_NONE)
    return;

  // Remove the mask before animating because masks affect animation
  // performance. Observe the animation and add the mask after animating if the
  // animation type is layouting selector items.
  mask_.reset();
  selector_item_->SetShadowBounds(base::nullopt);
  selector_item_->DisableBackdrop();

  if (window_->GetProperty(aura::client::kShowStateKey) !=
      ui::SHOW_STATE_MINIMIZED) {
    window_->layer()->SetMaskLayer(original_mask_layer_);
  }

  for (auto* window : wm::GetTransientTreeIterator(GetOverviewWindow())) {
    auto settings = std::make_unique<ScopedOverviewAnimationSettings>(
        animation_type, window);
    settings->DeferPaint();

    // If current |window_| is the first MRU window covering the available
    // workspace, add the |window_animation_observer| to its
    // ScopedOverviewAnimationSettings in order to monitor the complete of its
    // exiting animation.
    if (window == GetOverviewWindow() &&
        selector_item_->ShouldBeObservedWhenExiting()) {
      auto window_animation_observer_weak_ptr =
          selector_item_->window_grid()->window_animation_observer();
      if (window_animation_observer_weak_ptr)
        settings->AddObserver(window_animation_observer_weak_ptr.get());
    }

    animation_settings->push_back(std::move(settings));
  }

  if (animation_type == OVERVIEW_ANIMATION_LAY_OUT_SELECTOR_ITEMS &&
      animation_settings->size() > 0u) {
    animation_settings->front()->AddObserver(this);
  }
}

bool ScopedTransformOverviewWindow::Contains(const aura::Window* target) const {
  for (auto* window : wm::GetTransientTreeIterator(window_)) {
    if (window->Contains(target))
      return true;
  }
  aura::Window* mirror = GetOverviewWindowForMinimizedState();
  return mirror && mirror->Contains(target);
}

gfx::Rect ScopedTransformOverviewWindow::GetTargetBoundsInScreen() const {
  gfx::Rect bounds;
  aura::Window* overview_window = GetOverviewWindow();
  for (auto* window : wm::GetTransientTreeIterator(overview_window)) {
    // Ignore other window types when computing bounding box of window
    // selector target item.
    if (window != overview_window &&
        window->type() != aura::client::WINDOW_TYPE_NORMAL &&
        window->type() != aura::client::WINDOW_TYPE_PANEL) {
      continue;
    }
    gfx::Rect target_bounds = window->GetTargetBounds();
    ::wm::ConvertRectToScreen(window->parent(), &target_bounds);
    bounds.Union(target_bounds);
  }
  return bounds;
}

gfx::Rect ScopedTransformOverviewWindow::GetTransformedBounds() const {
  const int top_inset = GetTopInset();
  gfx::Rect bounds;
  aura::Window* overview_window = GetOverviewWindow();
  for (auto* window : wm::GetTransientTreeIterator(overview_window)) {
    // Ignore other window types when computing bounding box of window
    // selector target item.
    if (window != overview_window &&
        (window->type() != aura::client::WINDOW_TYPE_NORMAL &&
         window->type() != aura::client::WINDOW_TYPE_PANEL)) {
      continue;
    }
    gfx::RectF window_bounds(window->GetTargetBounds());
    gfx::Transform new_transform =
        TransformAboutPivot(gfx::Point(window_bounds.x(), window_bounds.y()),
                            window->layer()->GetTargetTransform());
    new_transform.TransformRect(&window_bounds);

    // The preview title is shown above the preview window. Hide the window
    // header for apps or browser windows with no tabs (web apps) to avoid
    // showing both the window header and the preview title.
    if (top_inset > 0) {
      gfx::RectF header_bounds(window_bounds);
      header_bounds.set_height(top_inset);
      new_transform.TransformRect(&header_bounds);
      window_bounds.Inset(0, gfx::ToCeiledInt(header_bounds.height()), 0, 0);
    }
    gfx::Rect enclosing_bounds = ToEnclosingRect(window_bounds);
    ::wm::ConvertRectToScreen(window->parent(), &enclosing_bounds);
    bounds.Union(enclosing_bounds);
  }
  return bounds;
}

SkColor ScopedTransformOverviewWindow::GetTopColor() const {
  for (auto* window : wm::GetTransientTreeIterator(window_)) {
    // If there are regular windows in the transient ancestor tree, all those
    // windows are shown in the same overview item and the header is not masked.
    if (window != window_ &&
        (window->type() == aura::client::WINDOW_TYPE_NORMAL ||
         window->type() == aura::client::WINDOW_TYPE_PANEL)) {
      return SK_ColorTRANSPARENT;
    }
  }
  return window_->GetProperty(aura::client::kTopViewColor);
}

int ScopedTransformOverviewWindow::GetTopInset() const {
  // Mirror window doesn't have insets.
  if (minimized_widget_)
    return 0;
  for (auto* window : wm::GetTransientTreeIterator(window_)) {
    // If there are regular windows in the transient ancestor tree, all those
    // windows are shown in the same overview item and the header is not masked.
    if (window != window_ &&
        (window->type() == aura::client::WINDOW_TYPE_NORMAL ||
         window->type() == aura::client::WINDOW_TYPE_PANEL)) {
      return 0;
    }
  }
  return window_->GetProperty(aura::client::kTopViewInset);
}

void ScopedTransformOverviewWindow::OnWindowDestroyed() {
  window_ = nullptr;
}

float ScopedTransformOverviewWindow::GetItemScale(const gfx::Size& source,
                                                  const gfx::Size& target,
                                                  int top_view_inset,
                                                  int title_height) {
  return std::min(2.0f, static_cast<float>((target.height() - title_height)) /
                            (source.height() - top_view_inset));
}

gfx::Transform ScopedTransformOverviewWindow::GetTransformForRect(
    const gfx::Rect& src_rect,
    const gfx::Rect& dst_rect) {
  DCHECK(!src_rect.IsEmpty());
  gfx::Transform transform;
  transform.Translate(dst_rect.x() - src_rect.x(), dst_rect.y() - src_rect.y());
  transform.Scale(static_cast<float>(dst_rect.width()) / src_rect.width(),
                  static_cast<float>(dst_rect.height()) / src_rect.height());
  return transform;
}

void ScopedTransformOverviewWindow::SetTransform(
    aura::Window* root_window,
    const gfx::Transform& transform) {
  DCHECK(overview_started_);
  auto window_animation_observer_weak_ptr =
      selector_item_->window_grid()->window_animation_observer();

  gfx::Point target_origin(GetTargetBoundsInScreen().origin());
  for (auto* window : wm::GetTransientTreeIterator(GetOverviewWindow())) {
    aura::Window* parent_window = window->parent();
    gfx::Rect original_bounds(window->GetTargetBounds());
    ::wm::ConvertRectToScreen(parent_window, &original_bounds);
    gfx::Transform new_transform =
        TransformAboutPivot(gfx::Point(target_origin.x() - original_bounds.x(),
                                       target_origin.y() - original_bounds.y()),
                            transform);
    // If current |window_| should not animate during exiting process, we defer
    // set transfrom on the window by adding the layer and transform information
    // to the |window_animation_observer|.
    if (!selector_item_->ShouldAnimateWhenExiting() &&
        window_animation_observer_weak_ptr) {
      window_animation_observer_weak_ptr->AddLayerTransformPair(window->layer(),
                                                                new_transform);
    } else {
      window->SetTransform(new_transform);
    }
  }
}

void ScopedTransformOverviewWindow::SetOpacity(float opacity) {
  for (auto* window : wm::GetTransientTreeIterator(GetOverviewWindow()))
    window->layer()->SetOpacity(opacity);
}

void ScopedTransformOverviewWindow::UpdateMirrorWindowForMinimizedState() {
  // TODO(oshima): Disable animation.
  if (window_->GetProperty(aura::client::kShowStateKey) ==
      ui::SHOW_STATE_MINIMIZED) {
    if (!minimized_widget_)
      CreateMirrorWindowForMinimizedState();
  } else {
    // If the original window is no longer minimized, make sure it will be
    // visible when we restore it when selection mode ends.
    EnsureVisible();
    minimized_widget_->CloseNow();
    minimized_widget_.reset();
  }
}

gfx::Rect ScopedTransformOverviewWindow::ShrinkRectToFitPreservingAspectRatio(
    const gfx::Rect& rect,
    const gfx::Rect& bounds,
    int top_view_inset,
    int title_height) {
  DCHECK(!rect.IsEmpty());
  DCHECK_LE(top_view_inset, rect.height());
  const float scale =
      GetItemScale(rect.size(), bounds.size(), top_view_inset, title_height);
  const int horizontal_offset = gfx::ToFlooredInt(
      0.5 * (bounds.width() - gfx::ToFlooredInt(scale * rect.width())));
  const int width = bounds.width() - 2 * horizontal_offset;
  const int vertical_offset =
      title_height - gfx::ToCeiledInt(scale * top_view_inset);
  const int height = std::min(gfx::ToCeiledInt(scale * rect.height()),
                              bounds.height() - vertical_offset);
  gfx::Rect new_bounds(bounds.x() + horizontal_offset,
                       bounds.y() + vertical_offset, width, height);

  switch (type()) {
    case ScopedTransformOverviewWindow::GridWindowFillMode::kLetterBoxed:
    case ScopedTransformOverviewWindow::GridWindowFillMode::kPillarBoxed: {
      // Attempt to scale |rect| to fit |bounds|. Maintain the aspect ratio of
      // |rect|. Letter boxed windows' width will match |bounds|'s height and
      // pillar boxed windows' height will match |bounds|'s height.
      const bool is_pillar =
          type() ==
          ScopedTransformOverviewWindow::GridWindowFillMode::kPillarBoxed;
      gfx::Rect src = rect;
      new_bounds = bounds;
      src.Inset(0, top_view_inset, 0, 0);
      new_bounds.Inset(0, title_height, 0, 0);
      float scale = is_pillar ? static_cast<float>(new_bounds.height()) /
                                    static_cast<float>(src.height())
                              : static_cast<float>(new_bounds.width()) /
                                    static_cast<float>(src.width());
      gfx::Size size(is_pillar ? src.width() * scale : new_bounds.width(),
                     is_pillar ? new_bounds.height() : src.height() * scale);
      new_bounds.ClampToCenteredSize(size);

      // Extend |new_bounds| in the vertical direction to account for the header
      // that will be hidden.
      if (top_view_inset > 0)
        new_bounds.Inset(0, -(scale * top_view_inset), 0, 0);

      // Save the original bounds minus the title into |window_selector_bounds_|
      // so a larger backdrop can be drawn behind the window after.
      window_selector_bounds_ = bounds;
      window_selector_bounds_->Inset(0, title_height, 0, 0);
      break;
    }
    default:
      break;
  }

  return new_bounds;
}

gfx::Rect ScopedTransformOverviewWindow::GetMaskBoundsForTesting() const {
  if (!mask_)
    return gfx::Rect();
  return mask_->layer()->bounds();
}

void ScopedTransformOverviewWindow::Close() {
  if (immediate_close_for_tests) {
    CloseWidget();
    return;
  }
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&ScopedTransformOverviewWindow::CloseWidget,
                            weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kCloseWindowDelayInMilliseconds));
}

void ScopedTransformOverviewWindow::PrepareForOverview() {
  Shell::Get()->shadow_controller()->UpdateShadowForWindow(window_);

  DCHECK(!overview_started_);
  overview_started_ = true;
  wm::GetWindowState(window_)->set_ignored_by_shelf(true);
  if (window_->GetProperty(aura::client::kShowStateKey) ==
      ui::SHOW_STATE_MINIMIZED) {
    CreateMirrorWindowForMinimizedState();
  }

  // Add requests to cache render surface and perform trilinear filtering. The
  // requests will be removed in dtor. So the requests will be valid during the
  // enter animation and the whole time during overview mode. For the exit
  // animation of overview mode, we need to add those requests again.
  if (features::IsTrilinearFilteringEnabled()) {
    for (auto* window : wm::GetTransientTreeIterator(GetOverviewWindow())) {
      cached_and_filtered_layer_observers_.push_back(
          std::make_unique<LayerCachingAndFilteringObserver>(window->layer()));
    }
  }

  // Apply rounded edge mask. Windows which are animated into overview mode
  // will have their mask removed before the animation begins and reapplied
  // after the animation ends.
  CreateAndApplyMaskAndShadow();
}

void ScopedTransformOverviewWindow::CloseWidget() {
  aura::Window* parent_window = GetTransientRoot(window_);
  if (parent_window)
    wm::CloseWidgetForWindow(parent_window);
}

// static
void ScopedTransformOverviewWindow::SetImmediateCloseForTests() {
  immediate_close_for_tests = true;
}

aura::Window* ScopedTransformOverviewWindow::GetOverviewWindow() const {
  if (minimized_widget_)
    return GetOverviewWindowForMinimizedState();
  return window_;
}

void ScopedTransformOverviewWindow::EnsureVisible() {
  original_opacity_ = 1.f;
}

aura::Window*
ScopedTransformOverviewWindow::GetOverviewWindowForMinimizedState() const {
  return minimized_widget_ ? minimized_widget_->GetNativeWindow() : nullptr;
}

void ScopedTransformOverviewWindow::UpdateWindowDimensionsType() {
  type_ = GetWindowDimensionsType(window_);
  window_selector_bounds_.reset();
}

void ScopedTransformOverviewWindow::CancelAnimationsListener() {
  StopObservingImplicitAnimations();
}

void ScopedTransformOverviewWindow::OnImplicitAnimationsCompleted() {
  CreateAndApplyMaskAndShadow();
  selector_item_->OnDragAnimationCompleted();
}

void ScopedTransformOverviewWindow::CreateMirrorWindowForMinimizedState() {
  DCHECK(!minimized_widget_.get());
  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.visible_on_all_workspaces = true;
  params.name = "OverviewModeMinimized";
  params.activatable = views::Widget::InitParams::Activatable::ACTIVATABLE_NO;
  params.accept_events = false;
  params.parent = window_->parent();
  minimized_widget_ = std::make_unique<views::Widget>();
  minimized_widget_->set_focus_on_creation(false);
  minimized_widget_->Init(params);

  // Trilinear filtering will be applied on the |minimized_widget_| in
  // PrepareForOverview() and RestoreWindow().
  views::View* mirror_view =
      new wm::WindowMirrorView(window_, /*trilinear_filtering_on_init=*/false);
  mirror_view->SetVisible(true);
  minimized_widget_->SetContentsView(mirror_view);
  gfx::Rect bounds(window_->GetBoundsInScreen());
  gfx::Size preferred = mirror_view->GetPreferredSize();
  // In unit tests, the content view can have empty size.
  if (!preferred.IsEmpty()) {
    int inset = bounds.height() - preferred.height();
    bounds.Inset(0, 0, 0, inset);
  }
  minimized_widget_->SetBounds(bounds);
  minimized_widget_->Show();

  minimized_widget_->SetOpacity(0.f);
  ScopedOverviewAnimationSettings animation_settings(
      OVERVIEW_ANIMATION_ENTER_OVERVIEW_MODE_TABLET_FADE_IN,
      minimized_widget_->GetNativeWindow());
  minimized_widget_->SetOpacity(1.f);
}

void ScopedTransformOverviewWindow::CreateAndApplyMaskAndShadow() {
  // Add the mask which gives the window selector items rounded corners, and add
  // the shadow around the window.
  ui::Layer* layer = minimized_widget_
                         ? minimized_widget_->GetContentsView()->layer()
                         : window_->layer();

  if (!minimized_widget_)
    original_mask_layer_ = window_->layer()->layer_mask_layer();

  mask_ = std::make_unique<WindowMask>(GetOverviewWindow());
  mask_->layer()->SetBounds(layer->bounds());
  mask_->set_top_inset(GetTopInset());
  layer->SetMaskLayer(mask_->layer());
  selector_item_->SetShadowBounds(base::make_optional(GetTransformedBounds()));
  selector_item_->EnableBackdropIfNeeded();
}

}  // namespace ash
