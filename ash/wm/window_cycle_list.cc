// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_cycle_list.h"

#include <list>
#include <map>
#include <memory>

#include "ash/public/cpp/ash_features.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_mirror_view.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/painter.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/visibility_controller.h"
#include "ui/wm/core/window_animations.h"

namespace ash {

namespace {

bool g_disable_initial_delay = false;

// Used for the highlight view and the shield (black background).
constexpr float kBackgroundCornerRadius = 4.f;

// This background paints a |Painter| but fills the view's layer's size rather
// than the view's size.
class LayerFillBackgroundPainter : public views::Background {
 public:
  explicit LayerFillBackgroundPainter(std::unique_ptr<views::Painter> painter)
      : painter_(std::move(painter)) {}

  ~LayerFillBackgroundPainter() override = default;

  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    views::Painter::PaintPainterAt(canvas, painter_.get(),
                                   gfx::Rect(view->layer()->size()));
  }

 private:
  std::unique_ptr<views::Painter> painter_;

  DISALLOW_COPY_AND_ASSIGN(LayerFillBackgroundPainter);
};

}  // namespace

// This view represents a single aura::Window by displaying a title and a
// thumbnail of the window's contents.
class WindowPreviewView : public views::View, public aura::WindowObserver {
 public:
  explicit WindowPreviewView(aura::Window* window)
      : window_title_(new views::Label),
        preview_background_(new views::View),
        mirror_view_(
            new wm::WindowMirrorView(window,
                                     /*trilinear_filtering_on_init=*/
                                     features::IsTrilinearFilteringEnabled())),
        window_observer_(this) {
    window_observer_.Add(window);
    window_title_->SetText(window->GetTitle());
    window_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    window_title_->SetEnabledColor(SK_ColorWHITE);
    window_title_->SetAutoColorReadabilityEnabled(false);
    // Background is not fully opaque, so subpixel rendering won't look good.
    window_title_->SetSubpixelRenderingEnabled(false);
    // The base font is 12pt (for English) so this comes out to 14pt.
    const int kLabelSizeDelta = 2;
    window_title_->SetFontList(
        window_title_->font_list().DeriveWithSizeDelta(kLabelSizeDelta));
    const int kAboveLabelPadding = 5;
    const int kBelowLabelPadding = 10;
    window_title_->SetBorder(
        views::CreateEmptyBorder(kAboveLabelPadding, 0, kBelowLabelPadding, 0));
    AddChildView(window_title_);

    // Preview padding is black at 50% opacity.
    preview_background_->SetBackground(
        views::CreateSolidBackground(SkColorSetA(SK_ColorBLACK, 0xFF / 2)));
    AddChildView(preview_background_);

    AddChildView(mirror_view_);

    SetFocusBehavior(FocusBehavior::ALWAYS);
  }
  ~WindowPreviewView() override = default;

  // views::View:
  gfx::Size CalculatePreferredSize() const override {
    gfx::Size size = GetSizeForPreviewArea();
    size.Enlarge(0, window_title_->GetPreferredSize().height());
    return size;
  }

  void Layout() override {
    const gfx::Size preview_area_size = GetSizeForPreviewArea();
    // The window title is positioned above the preview area.
    window_title_->SetBounds(0, 0, width(),
                             height() - preview_area_size.height());

    gfx::Rect preview_area_bounds(preview_area_size);
    preview_area_bounds.set_y(height() - preview_area_size.height());
    mirror_view_->SetSize(GetMirrorViewScaledSize());
    if (mirror_view_->size() == preview_area_size) {
      // Padding is not needed, hide the background and set the mirror view
      // to take up the entire preview area.
      mirror_view_->SetPosition(preview_area_bounds.origin());
      preview_background_->SetVisible(false);
      return;
    }

    // Padding is needed, so show the background and set the mirror view to be
    // centered within it.
    preview_background_->SetBoundsRect(preview_area_bounds);
    preview_background_->SetVisible(true);
    preview_area_bounds.ClampToCenteredSize(mirror_view_->size());
    mirror_view_->SetPosition(preview_area_bounds.origin());
  }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->role = ax::mojom::Role::kWindow;
    node_data->SetName(window_title_->text());
  }

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override {
    window_observer_.Remove(window);
  }

  void OnWindowTitleChanged(aura::Window* window) override {
    window_title_->SetText(window->GetTitle());
  }

 private:
  // The maximum width of a window preview.
  static const int kMaxPreviewWidth = 512;
  // All previews are the same height (this is achieved via a combination of
  // scaling and padding).
  static const int kFixedPreviewHeight = 256;

  // Returns the size for the mirror view, scaled to fit within the max bounds.
  // Scaling is always 1:1 and we only scale down, never up.
  gfx::Size GetMirrorViewScaledSize() const {
    gfx::Size mirror_pref_size = mirror_view_->GetPreferredSize();

    if (mirror_pref_size.width() > kMaxPreviewWidth ||
        mirror_pref_size.height() > kFixedPreviewHeight) {
      float scale = std::min(
          kMaxPreviewWidth / static_cast<float>(mirror_pref_size.width()),
          kFixedPreviewHeight / static_cast<float>(mirror_pref_size.height()));
      mirror_pref_size =
          gfx::ScaleToFlooredSize(mirror_pref_size, scale, scale);
    }

    return mirror_pref_size;
  }

  // Returns the size for the entire preview area (mirror view and additional
  // padding). All previews will be the same height, so if the mirror view isn't
  // tall enough we will add top and bottom padding. Previews can range in width
  // from kMaxPreviewWidth down to half that value. Again, padding will be added
  // to the sides to achieve this if the preview is too narrow.
  gfx::Size GetSizeForPreviewArea() const {
    gfx::Size mirror_size = GetMirrorViewScaledSize();
    float aspect_ratio =
        static_cast<float>(mirror_size.width()) / mirror_size.height();
    gfx::Size preview_size = mirror_size;
    // Very narrow windows get vertical bars of padding on the sides.
    if (aspect_ratio < 0.5f)
      preview_size.set_width(mirror_size.height() / 2);

    // All previews are the same height (this may add padding on top and
    // bottom).
    preview_size.set_height(kFixedPreviewHeight);
    // Previews should never be narrower than half their max width (128dip).
    preview_size.set_width(
        std::max(preview_size.width(), kMaxPreviewWidth / 2));

    return preview_size;
  }

  // Displays the title of the window above the preview.
  views::Label* window_title_;
  // When visible, shows a darkened background area behind |mirror_view_|
  // (effectively padding the preview to fit the desired bounds).
  views::View* preview_background_;
  // The view that actually renders a thumbnail version of the window.
  views::View* mirror_view_;

  ScopedObserver<aura::Window, aura::WindowObserver> window_observer_;

  DISALLOW_COPY_AND_ASSIGN(WindowPreviewView);
};

// A view that shows a collection of windows the user can tab through.
class WindowCycleView : public views::WidgetDelegateView {
 public:
  explicit WindowCycleView(const WindowCycleList::WindowList& windows)
      : mirror_container_(new views::View()),
        highlight_view_(new views::View()),
        target_window_(nullptr) {
    DCHECK(!windows.empty());
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    layer()->SetMasksToBounds(true);
    layer()->SetOpacity(0.0);
    {
      ui::ScopedLayerAnimationSettings animate_fade(layer()->GetAnimator());
      animate_fade.SetTransitionDuration(
          base::TimeDelta::FromMilliseconds(100));
      layer()->SetOpacity(1.0);
    }

    const int kInsideBorderPaddingDip = 64;
    const int kBetweenChildPaddingDip = 10;
    auto layout = std::make_unique<views::BoxLayout>(
        views::BoxLayout::kHorizontal, gfx::Insets(kInsideBorderPaddingDip),
        kBetweenChildPaddingDip);
    layout->set_cross_axis_alignment(
        views::BoxLayout::CROSS_AXIS_ALIGNMENT_START);
    mirror_container_->SetLayoutManager(std::move(layout));
    mirror_container_->SetPaintToLayer();
    mirror_container_->layer()->SetFillsBoundsOpaquely(false);

    for (auto* window : windows) {
      // |mirror_container_| owns |view|.
      // The |mirror_view_| in |view| will use trilinear filtering in
      // InitLayerOwner().
      views::View* view = new WindowPreviewView(window);
      window_view_map_[window] = view;
      mirror_container_->AddChildView(view);
    }

    // The background needs to be painted to fill the layer, not the View,
    // because the layer animates bounds changes but the View's bounds change
    // immediately.
    highlight_view_->SetBackground(std::make_unique<LayerFillBackgroundPainter>(
        views::Painter::CreateRoundRectWith1PxBorderPainter(
            SkColorSetA(SK_ColorWHITE, 0x4D), SkColorSetA(SK_ColorWHITE, 0x33),
            kBackgroundCornerRadius)));
    highlight_view_->SetPaintToLayer();

    highlight_view_->layer()->SetFillsBoundsOpaquely(false);

    AddChildView(highlight_view_);
    AddChildView(mirror_container_);
  }

  ~WindowCycleView() override = default;

  void SetTargetWindow(aura::Window* target) {
    target_window_ = target;
    if (GetWidget()) {
      Layout();
      if (target_window_)
        window_view_map_[target_window_]->RequestFocus();
    }
  }

  void HandleWindowDestruction(aura::Window* destroying_window,
                               aura::Window* new_target) {
    auto view_iter = window_view_map_.find(destroying_window);
    views::View* preview = view_iter->second;
    views::View* parent = preview->parent();
    DCHECK_EQ(mirror_container_, parent);
    window_view_map_.erase(view_iter);
    delete preview;
    // With one of its children now gone, we must re-layout |mirror_container_|.
    // This must happen before SetTargetWindow() to make sure our own Layout()
    // works correctly when it's calculating highlight bounds.
    parent->Layout();
    SetTargetWindow(new_target);
  }

  void DestroyContents() {
    window_view_map_.clear();
    RemoveAllChildViews(true);
  }

  // views::WidgetDelegateView overrides:
  gfx::Size CalculatePreferredSize() const override {
    return mirror_container_->GetPreferredSize();
  }

  void Layout() override {
    if (!target_window_ || bounds().IsEmpty())
      return;

    bool first_layout = mirror_container_->bounds().IsEmpty();
    // If |mirror_container_| has not yet been laid out, we must lay it and its
    // descendants out so that the calculations based on |target_view| work
    // properly.
    if (first_layout)
      mirror_container_->SizeToPreferredSize();

    views::View* target_view = window_view_map_[target_window_];
    gfx::RectF target_bounds(target_view->GetLocalBounds());
    views::View::ConvertRectToTarget(target_view, mirror_container_,
                                     &target_bounds);
    gfx::Rect container_bounds(mirror_container_->GetPreferredSize());
    // Case one: the container is narrower than the screen. Center the
    // container.
    int x_offset = (width() - container_bounds.width()) / 2;
    if (x_offset < 0) {
      // Case two: the container is wider than the screen. Center the target
      // view by moving the list just enough to ensure the target view is in the
      // center.
      x_offset = width() / 2 -
                 mirror_container_->GetMirroredXInView(
                     target_bounds.CenterPoint().x());

      // However, the container must span the screen, i.e. the maximum x is 0
      // and the minimum for its right boundary is the width of the screen.
      x_offset = std::min(x_offset, 0);
      x_offset = std::max(x_offset, width() - container_bounds.width());
    }
    container_bounds.set_x(x_offset);
    mirror_container_->SetBoundsRect(container_bounds);

    // Calculate the target preview's bounds relative to |this|.
    views::View::ConvertRectToTarget(mirror_container_, this, &target_bounds);
    const int kHighlightPaddingDip = 5;
    target_bounds.Inset(gfx::InsetsF(-kHighlightPaddingDip));
    target_bounds.set_x(
        GetMirroredXWithWidthInView(target_bounds.x(), target_bounds.width()));
    highlight_view_->SetBoundsRect(gfx::ToEnclosingRect(target_bounds));

    // Enable animations only after the first Layout() pass.
    if (first_layout) {
      // The preview list animates bounds changes (other animatable properties
      // never change).
      mirror_container_->layer()->SetAnimator(
          ui::LayerAnimator::CreateImplicitAnimator());
      // The selection highlight also animates all bounds changes and never
      // changes other animatable properties.
      highlight_view_->layer()->SetAnimator(
          ui::LayerAnimator::CreateImplicitAnimator());
    }
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    // We can't set a bg on the mirror container itself because the highlight
    // view needs to be on top of the bg but behind the target windows.
    const gfx::RectF shield_bounds(mirror_container_->bounds());
    cc::PaintFlags flags;
    flags.setColor(SkColorSetA(SK_ColorBLACK, 0xE6));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    float corner_radius = 0.f;
    if (shield_bounds.width() < width()) {
      flags.setAntiAlias(true);
      corner_radius = kBackgroundCornerRadius;
    }
    canvas->DrawRoundRect(shield_bounds, corner_radius, flags);
  }

  View* GetInitiallyFocusedView() override {
    return window_view_map_[target_window_];
  }

  aura::Window* target_window() { return target_window_; }

 private:
  std::map<aura::Window*, views::View*> window_view_map_;
  views::View* mirror_container_;
  views::View* highlight_view_;
  aura::Window* target_window_;

  DISALLOW_COPY_AND_ASSIGN(WindowCycleView);
};

WindowCycleList::WindowCycleList(const WindowList& windows)
    : windows_(windows),
      screen_observer_(this) {
  if (!ShouldShowUi())
    Shell::Get()->mru_window_tracker()->SetIgnoreActivations(true);

  for (auto* window : windows_)
    window->AddObserver(this);

  if (ShouldShowUi()) {
    if (g_disable_initial_delay) {
      InitWindowCycleView();
    } else {
      show_ui_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(150),
                           this, &WindowCycleList::InitWindowCycleView);
    }
  }
}

WindowCycleList::~WindowCycleList() {
  if (!ShouldShowUi())
    Shell::Get()->mru_window_tracker()->SetIgnoreActivations(false);

  for (auto* window : windows_)
    window->RemoveObserver(this);

  if (!windows_.empty() && user_did_accept_) {
    auto* target_window = windows_[current_index_];
    target_window->Show();
    wm::GetWindowState(target_window)->Activate();
  }

  if (cycle_ui_widget_)
    cycle_ui_widget_->Close();

  // |this| is responsible for notifying |cycle_view_| when windows are
  // destroyed. Since |this| is going away, clobber |cycle_view_|. Otherwise
  // there will be a race where a window closes after now but before the
  // Widget::Close() call above actually destroys |cycle_view_|. See
  // crbug.com/681207
  if (cycle_view_)
    cycle_view_->DestroyContents();
}

void WindowCycleList::Step(WindowCycleController::Direction direction) {
  if (windows_.empty())
    return;

  // When there is only one window, we should give feedback to the user. If the
  // window is minimized, we should also show it.
  if (windows_.size() == 1) {
    ::wm::AnimateWindow(windows_[0], ::wm::WINDOW_ANIMATION_TYPE_BOUNCE);
    windows_[0]->Show();
    wm::GetWindowState(windows_[0])->Activate();
    return;
  }

  DCHECK(static_cast<size_t>(current_index_) < windows_.size());

  if (!cycle_view_ && current_index_ == 0) {
    // Special case the situation where we're cycling forward but the MRU window
    // is not active. This occurs when all windows are minimized. The starting
    // window should be the first one rather than the second.
    if (direction == WindowCycleController::FORWARD &&
        !wm::IsActiveWindow(windows_[0]))
      current_index_ = -1;
  }

  // We're in a valid cycle, so step forward or backward.
  current_index_ += direction == WindowCycleController::FORWARD ? 1 : -1;

  // Wrap to window list size.
  current_index_ = (current_index_ + windows_.size()) % windows_.size();
  DCHECK(windows_[current_index_]);

  if (ShouldShowUi()) {
    if (current_index_ > 1)
      InitWindowCycleView();

    if (cycle_view_)
      cycle_view_->SetTargetWindow(windows_[current_index_]);
  }
}

// static
void WindowCycleList::DisableInitialDelayForTesting() {
  g_disable_initial_delay = true;
}

void WindowCycleList::OnWindowDestroying(aura::Window* window) {
  window->RemoveObserver(this);

  WindowList::iterator i = std::find(windows_.begin(), windows_.end(), window);
  // TODO(oshima): Change this back to DCHECK once crbug.com/483491 is fixed.
  CHECK(i != windows_.end());
  int removed_index = static_cast<int>(i - windows_.begin());
  windows_.erase(i);
  if (current_index_ > removed_index ||
      current_index_ == static_cast<int>(windows_.size())) {
    current_index_--;
  }

  if (cycle_view_) {
    auto* new_target_window =
        windows_.empty() ? nullptr : windows_[current_index_];
    cycle_view_->HandleWindowDestruction(window, new_target_window);
    if (windows_.empty()) {
      // This deletes us.
      Shell::Get()->window_cycle_controller()->CancelCycling();
      return;
    }
  }
}

void WindowCycleList::OnDisplayAdded(const display::Display& new_display) {}

void WindowCycleList::OnDisplayRemoved(const display::Display& old_display) {}

void WindowCycleList::OnDisplayMetricsChanged(const display::Display& display,
                                              uint32_t changed_metrics) {
  if (cycle_ui_widget_ &&
      display.id() ==
          display::Screen::GetScreen()
              ->GetDisplayNearestWindow(cycle_ui_widget_->GetNativeWindow())
              .id() &&
      (changed_metrics & (DISPLAY_METRIC_BOUNDS | DISPLAY_METRIC_ROTATION))) {
    Shell::Get()->window_cycle_controller()->CancelCycling();
    // |this| is deleted.
    return;
  }
}

bool WindowCycleList::ShouldShowUi() {
  return windows_.size() > 1;
}

void WindowCycleList::InitWindowCycleView() {
  if (cycle_view_)
    return;

  cycle_view_ = new WindowCycleView(windows_);
  cycle_view_->SetTargetWindow(windows_[current_index_]);

  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params;
  params.delegate = cycle_view_;
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.accept_events = true;
  params.name = "WindowCycleList (Alt+Tab)";
  // TODO(estade): make sure nothing untoward happens when the lock screen
  // or a system modal dialog is shown.
  aura::Window* root_window = Shell::GetRootWindowForNewWindows();
  params.parent = root_window->GetChildById(kShellWindowId_OverlayContainer);
  gfx::Rect widget_rect = display::Screen::GetScreen()
                              ->GetDisplayNearestWindow(root_window)
                              .bounds();
  const int widget_height = cycle_view_->GetPreferredSize().height();
  widget_rect.set_y(widget_rect.y() +
                    (widget_rect.height() - widget_height) / 2);
  widget_rect.set_height(widget_height);
  params.bounds = widget_rect;
  widget->Init(params);

  screen_observer_.Add(display::Screen::GetScreen());
  widget->Show();
  cycle_ui_widget_ = widget;
}

}  // namespace ash
