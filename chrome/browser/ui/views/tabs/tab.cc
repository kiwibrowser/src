// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab.h"

#include <stddef.h>
#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/debug/alias.h"
#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/paint_recorder.h"
#include "cc/paint/paint_shader.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tab_contents/core_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "chrome/browser/ui/views/tabs/tab_icon.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/touch_uma/touch_uma.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/models/list_selection_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/compositor/clip_recorder.h"
#include "ui/gfx/animation/animation_container.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_analysis.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/path.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/rect_based_targeting_utils.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/tooltip_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

#if defined(USE_AURA)
#include "ui/aura/env.h"
#endif

using base::UserMetricsAction;
using MD = ui::MaterialDesignController;

namespace {

constexpr int kExtraLeftPaddingToBalanceCloseButtonPadding = 2;

// When a non-pinned tab becomes a pinned tab the width of the tab animates. If
// the width of a pinned tab is at least kPinnedTabExtraWidthToRenderAsNormal
// larger than the desired pinned tab width then the tab is rendered as a normal
// tab. This is done to avoid having the title immediately disappear when
// transitioning a tab from normal to pinned tab.
constexpr int kPinnedTabExtraWidthToRenderAsNormal = 30;

// How opaque to make the hover state (out of 1).
constexpr double kHoverOpacity = 0.33;

// Opacity of the active tab background painted over inactive selected tabs.
constexpr double kSelectedTabOpacity = 0.3;

// Inactive selected tabs have their throb value scaled by this.
constexpr double kSelectedTabThrobScale = 0.95 - kSelectedTabOpacity;

// Height of the separator painted on the left edge of the tab for the material
// refresh mode.
constexpr int kTabSeparatorHeight = 20;
constexpr int kTabSeparatorTouchHeight = 24;

////////////////////////////////////////////////////////////////////////////////
// Drawing and utility functions

// Returns the width of the tab endcap in DIP.  More precisely, this is the
// width of the curve making up either the outer or inner edge of the stroke.
//
// For non-material-refresh mode, these two curves are horizontally offset by
// 1 px (regardless of scale), the total width of the endcap from tab outer
// edge to the inside end of the stroke inner edge is
// (GetUnscaledEndcapWidth() * scale) + 1.
//
// The value returned here must be at least Tab::kMinimumEndcapWidth.
float GetTabEndcapWidth() {
  // TODO(pkasting): This should become a member function and vary with
  // GetCornerRadius().
  return GetLayoutInsets(TAB).left() - (MD::IsRefreshUi() ? 0.0f : 0.5f);
}

void DrawHighlight(gfx::Canvas* canvas,
                   const SkPoint& p,
                   SkScalar radius,
                   SkColor color) {
  const SkColor colors[2] = { color, SkColorSetA(color, 0) };
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setShader(cc::PaintShader::MakeRadialGradient(
      p, radius, colors, nullptr, 2, SkShader::kClamp_TileMode));
  canvas->sk_canvas()->drawRect(
      SkRect::MakeXYWH(p.x() - radius, p.y() - radius, radius * 2, radius * 2),
      flags);
}

// Scale and align the vertical portions of the end-caps on a pixel boundary.
// This ensures there is no anti-alias background bleed-through between the tabs
// due to coordinates landing between pixel boundaries.
const gfx::RectF ScaleAndAlignBounds(const gfx::Rect& bounds,
                                     float endcap_width,
                                     float scale) {
  gfx::RectF aligned_bounds(bounds);
  aligned_bounds.Inset(endcap_width / 2, 0);
  aligned_bounds.Scale(scale);
  const float left = std::round(aligned_bounds.x());
  const float right = std::round(aligned_bounds.right());
  aligned_bounds.set_x(left);
  aligned_bounds.set_width(right - left);
  aligned_bounds.Inset(-(endcap_width / 2) * scale, 0);
  return aligned_bounds;
}

// Offset each path inward by |scaled_horizontal_inset| then intersect them
// together.
gfx::Path OffsetAndIntersectPaths(gfx::Path& left_path,
                                  gfx::Path& right_path,
                                  float scaled_horizontal_inset) {
  gfx::Path complete_path;
  left_path.offset(scaled_horizontal_inset, 0);
  right_path.offset(-scaled_horizontal_inset, 0);
  Op(left_path, right_path, SkPathOp::kIntersect_SkPathOp, &complete_path);
  return complete_path;
}

// Returns a path corresponding to the tab's content region. The sides of the
// path will be inset by |horizontal_inset|; this is useful when trying to clip
// favicons to match the overall tab shape but be inset from the edge.
gfx::Path GetRefreshInteriorPath(float scale,
                                 const gfx::Rect& bounds,
                                 float endcap_width,
                                 float horizontal_inset) {
  gfx::Path left_path;
  gfx::Path right_path;
  const float radius = (endcap_width / 2) * scale;
  const float stroke_thickness = TabStrip::ShouldDrawStrokes() ? 1 : 0;
  const gfx::RectF layout_bounds =
      ScaleAndAlignBounds(bounds, endcap_width, scale);

  const float left = layout_bounds.x();
  const float top = layout_bounds.y();
  const float right = layout_bounds.right();
  const float bottom = std::ceil(layout_bounds.bottom());

  // Bottom right.
  right_path.moveTo(right, bottom);
  right_path.rLineTo(0, stroke_thickness);

  right_path.arcTo(radius, radius, 0, SkPath::kSmall_ArcSize,
                   SkPath::kCW_Direction, right - radius, bottom - radius);

  // Right vertical.
  right_path.lineTo(right - radius, top + radius - stroke_thickness);

  // Top right.
  right_path.arcTo(radius, radius, 0, SkPath::kSmall_ArcSize,
                   SkPath::kCCW_Direction, right - radius * 2,
                   top + stroke_thickness);

  // Top edge.
  right_path.lineTo(left, stroke_thickness);
  right_path.lineTo(left, bottom);
  right_path.close();

  // Top left.
  left_path.moveTo(left + radius * 2, top + stroke_thickness);

  left_path.arcTo(radius, radius, 0, SkPath::kSmall_ArcSize,
                  SkPath::kCCW_Direction, left + radius, top + radius);

  // Left vertical.
  left_path.lineTo(left + radius, bottom - radius);

  // Bottom left.
  left_path.arcTo(radius, radius, 0, SkPath::kSmall_ArcSize,
                  SkPath::kCW_Direction, left, bottom + stroke_thickness);

  // Bottom edge.
  left_path.lineTo(right, bottom + stroke_thickness);
  left_path.lineTo(right, top + stroke_thickness);
  left_path.close();

  // Convert paths to being relative to bounds origin.
  gfx::PointF origin(bounds.origin());
  origin.Scale(scale);
  SkMatrix translate = SkMatrix::MakeTrans(-origin.x(), -origin.y());
  right_path.transform(translate);
  left_path.transform(translate);

  return OffsetAndIntersectPaths(left_path, right_path,
                                 horizontal_inset * scale);
}

// Returns a path corresponding to the tab's content region inside the outer
// stroke. The sides of the path will be inset by |horizontal_inset|; this is
// useful when trying to clip favicons to match the overall tab shape but be
// inset from the edge.
gfx::Path GetInteriorPath(float scale,
                          const gfx::Rect& bounds,
                          float endcap_width,
                          float horizontal_inset = 0) {
  // Construct the interior path by intersecting paths representing the left
  // and right halves of the tab.  Compared to computing the full path at once,
  // this makes it easier to avoid overdraw in the top center near minimum
  // width, and to implement cases where |horizontal_inset| != 0.
  if (MD::IsRefreshUi()) {
    return GetRefreshInteriorPath(scale, bounds, endcap_width,
                                  horizontal_inset);
  }

  gfx::Path left_path;
  gfx::Path right_path;
  const float right = bounds.width() * scale;
  // The bottom of the tab needs to be pixel-aligned or else when we call
  // ClipPath with anti-aliasing enabled it can cause artifacts.
  const float bottom = std::ceil(bounds.height() * scale);

  right_path.moveTo(right - 1, bottom);
  right_path.rCubicTo(-0.75 * scale, 0, -1.625 * scale, -0.5 * scale,
                      -2 * scale, -1.5 * scale);
  right_path.lineTo(right - 1 - (endcap_width - 2) * scale, 2.5 * scale);
  right_path.rCubicTo(-0.375 * scale, -1 * scale, -1.25 * scale, -1.5 * scale,
                      -2 * scale, -1.5 * scale);
  right_path.lineTo(0, scale);
  right_path.lineTo(0, bottom);
  right_path.close();

  const float scaled_endcap_width = 1 + endcap_width * scale;
  left_path.moveTo(scaled_endcap_width, scale);
  left_path.rCubicTo(-0.75 * scale, 0, -1.625 * scale, 0.5 * scale, -2 * scale,
                     1.5 * scale);
  left_path.lineTo(1 + 2 * scale, bottom - 1.5 * scale);
  left_path.rCubicTo(-0.375 * scale, scale, -1.25 * scale, 1.5 * scale,
                     -2 * scale, 1.5 * scale);
  left_path.lineTo(right, bottom);
  left_path.lineTo(right, scale);
  left_path.close();

  return OffsetAndIntersectPaths(left_path, right_path,
                                 horizontal_inset * scale);
}

// Returns the border path under material refresh mode for a given tab |bounds|,
// |scale| and |endcap_width|.
gfx::Path GetRefreshBorderPath(const gfx::Rect& bounds,
                               bool extend_to_top,
                               float scale,
                               float endcap_width,
                               float stroke_thickness) {
  gfx::Path path;
  const float radius = (endcap_width / 2) * scale;
  const float bottom_radius = radius - stroke_thickness;
  const float top_radius = radius + stroke_thickness;
  const gfx::RectF layout_bounds =
      ScaleAndAlignBounds(bounds, endcap_width, scale);

  const float left = layout_bounds.x();
  const float top = layout_bounds.y();
  const float right = layout_bounds.right();
  const float bottom = std::ceil(layout_bounds.bottom());

  path.moveTo(left, bottom);
  path.rLineTo(0, -stroke_thickness);

  // bottom left
  path.arcTo(bottom_radius, bottom_radius, 0, SkPath::kSmall_ArcSize,
             SkPath::kCCW_Direction, left + bottom_radius, bottom - radius);

  if (extend_to_top) {
    // If |extend_to_top| is true, the top of the tab is simply squared off.
    // This is used for the mouse hit test mask in full screen mode in order to
    // eliminate the little divot between the tabs at the top.

    // left vertical
    path.lineTo(left + bottom_radius, top);
    // top line
    path.lineTo(right - bottom_radius, top);
  } else {
    // left vertical
    path.lineTo(left + bottom_radius, top + top_radius);
    // top left
    path.arcTo(top_radius, top_radius, 0, SkPath::kSmall_ArcSize,
               SkPath::kCW_Direction, left + radius * 2, top);
    // top line
    path.lineTo(right - radius * 2, top);
    // top right
    path.arcTo(top_radius, top_radius, 0, SkPath::kSmall_ArcSize,
               SkPath::kCW_Direction, right - bottom_radius, top + radius);
  }
  // right vertical
  path.lineTo(right - bottom_radius, bottom - radius);
  // bottom right
  path.arcTo(bottom_radius, bottom_radius, 0, SkPath::kSmall_ArcSize,
             SkPath::kCCW_Direction, right, bottom - stroke_thickness);

  path.rLineTo(0, stroke_thickness);
  path.close();

  // Convert path to be relative to the tab bounds.
  gfx::PointF origin(bounds.origin());
  origin.Scale(scale);
  path.transform(SkMatrix::MakeTrans(-origin.x(), -origin.y()));

  return path;
}

// Returns a path corresponding to the tab's outer border for a given tab
// |bounds|, |scale|, and |endcap_width|.  If |unscale_at_end| is true, this
// path will be normalized to a 1x scale by scaling by 1/scale before returning.
// If |extend_to_top| is true, the path is extended vertically to the top of the
// tab bounds.  The caller uses this for Fitts' Law purposes in
// maximized/fullscreen mode.
gfx::Path GetBorderPath(float scale,
                        bool unscale_at_end,
                        bool extend_to_top,
                        float endcap_width,
                        const gfx::Rect& bounds) {
  const float stroke_thickness = TabStrip::ShouldDrawStrokes() ? 1 : 0;

  gfx::Path path;

  if (MD::IsRefreshUi()) {
    path = GetRefreshBorderPath(bounds, extend_to_top, scale, endcap_width,
                                stroke_thickness);
  } else {
    const float top = scale - stroke_thickness;
    const float right = bounds.width() * scale;
    const float bottom = bounds.height() * scale;

    path.moveTo(0, bottom);
    path.rLineTo(0, -stroke_thickness);

    path.rCubicTo(0.75 * scale, 0, 1.625 * scale, -0.5 * scale, 2 * scale,
                  -1.5 * scale);
    path.lineTo((endcap_width - 2) * scale, top + 1.5 * scale);
    if (extend_to_top) {
      // Create the vertical extension by extending the side diagonals until
      // they reach the top of the bounds.
      const float dy = 2.5 * scale - 1;
      const float dx = Tab::GetInverseDiagonalSlope() * dy;
      path.rLineTo(dx, -dy);
      path.lineTo(right - (endcap_width - 2) * scale - dx, 0);
      path.rLineTo(dx, dy);
    } else {
      path.rCubicTo(0.375 * scale, -scale, 1.25 * scale, -1.5 * scale,
                    2 * scale, -1.5 * scale);
      path.lineTo(right - endcap_width * scale, top);
      path.rCubicTo(0.75 * scale, 0, 1.625 * scale, 0.5 * scale, 2 * scale,
                    1.5 * scale);
    }
    path.lineTo(right - 2 * scale, bottom - 1 - 1.5 * scale);
    path.rCubicTo(0.375 * scale, scale, 1.25 * scale, 1.5 * scale, 2 * scale,
                  1.5 * scale);
    path.rLineTo(0, stroke_thickness);
    path.close();
  }


  if (unscale_at_end && (scale != 1))
    path.transform(SkMatrix::MakeScale(1.f / scale));

  return path;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Tab, public:

// static
const char Tab::kViewClassName[] = "Tab";

Tab::Tab(TabController* controller, gfx::AnimationContainer* container)
    : controller_(controller),
      pulse_animation_(this),
      animation_container_(container),
      title_(new views::Label()),
      title_animation_(this),
      hover_controller_(this) {
  DCHECK(controller);

  // So we get don't get enter/exit on children and don't prematurely stop the
  // hover.
  set_notify_enter_exit_on_child(true);

  set_id(VIEW_ID_TAB);

  // This will cause calls to GetContentsBounds to return only the rectangle
  // inside the tab shape, rather than to its extents.
  SetBorder(views::CreateEmptyBorder(GetLayoutInsets(TAB)));

  title_->SetHorizontalAlignment(gfx::ALIGN_TO_HEAD);
  title_->SetElideBehavior(gfx::FADE_TAIL);
  title_->SetHandlesTooltips(false);
  title_->SetAutoColorReadabilityEnabled(false);
  title_->SetText(CoreTabHelper::GetDefaultTitle());
  AddChildView(title_);

  SetEventTargeter(std::make_unique<views::ViewTargeter>(this));

  icon_ = new TabIcon;
  AddChildView(icon_);

  alert_indicator_button_ = new AlertIndicatorButton(this);
  AddChildView(alert_indicator_button_);

  // Unretained is safe here because this class outlives its close button, and
  // the controller outlives this Tab.
  close_button_ = new TabCloseButton(
      this, base::BindRepeating(&TabController::OnMouseEventInTab,
                                base::Unretained(controller_)));
  AddChildView(close_button_);

  set_context_menu_controller(this);

  constexpr int kPulseDurationMs = 200;
  pulse_animation_.SetSlideDuration(kPulseDurationMs);
  pulse_animation_.SetContainer(animation_container_.get());

  title_animation_.SetDuration(base::TimeDelta::FromMilliseconds(100));
  title_animation_.SetContainer(animation_container_.get());

  hover_controller_.SetAnimationContainer(animation_container_.get());
}

Tab::~Tab() {
}

int Tab::GetCornerRadius() const {
  // TODO(pkasting): This should vary as the tab width decreases.
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_MEDIUM);
}

SkColor Tab::GetAlertIndicatorColor(TabAlertState state) const {
  const bool is_touch_optimized = MD::IsTouchOptimizedUiEnabled();
  // If theme provider is not yet available, return the default button
  // color.
  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  if (!theme_provider)
    return button_color_;

  switch (state) {
    case TabAlertState::AUDIO_PLAYING:
    case TabAlertState::AUDIO_MUTING:
      return is_touch_optimized ? theme_provider->GetColor(
                                      ThemeProperties::COLOR_TAB_ALERT_AUDIO)
                                : button_color_;
    case TabAlertState::MEDIA_RECORDING:
      return theme_provider->GetColor(
          ThemeProperties::COLOR_TAB_ALERT_RECORDING);
    case TabAlertState::TAB_CAPTURING:
      return is_touch_optimized
                 ? theme_provider->GetColor(
                       ThemeProperties::COLOR_TAB_ALERT_CAPTURING)
                 : button_color_;
    case TabAlertState::PIP_PLAYING:
      return theme_provider->GetColor(ThemeProperties::COLOR_TAB_PIP_PLAYING);
    case TabAlertState::BLUETOOTH_CONNECTED:
    case TabAlertState::USB_CONNECTED:
    case TabAlertState::NONE:
      return button_color_;
    default:
      NOTREACHED();
      return button_color_;
  }
}

SkColor Tab::GetCloseTabButtonColor(
    views::Button::ButtonState button_state) const {
  // The theme provider may be null if we're not currently in a widget
  // hierarchy.
  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  if (!theme_provider)
    return SK_ColorTRANSPARENT;

  int color_id;
  switch (button_state) {
    case views::Button::STATE_HOVERED:
      color_id = ThemeProperties::COLOR_TAB_CLOSE_BUTTON_BACKGROUND_HOVER;
      break;
    case views::Button::STATE_PRESSED:
      color_id = ThemeProperties::COLOR_TAB_CLOSE_BUTTON_BACKGROUND_PRESSED;
      break;
    default:
      color_id = IsActive() ? ThemeProperties::COLOR_TAB_CLOSE_BUTTON_ACTIVE
                            : ThemeProperties::COLOR_TAB_CLOSE_BUTTON_INACTIVE;
  }
  return theme_provider->GetColor(color_id);
}

bool Tab::IsActive() const {
  return controller_->IsActiveTab(this);
}

void Tab::ActiveStateChanged() {
  if (IsActive()) {
    // Clear the blocked WebContents for active tabs because it's distracting.
    icon_->SetAttention(TabIcon::AttentionType::kBlockedWebContents, false);
  }
  OnButtonColorMaybeChanged();
  alert_indicator_button_->UpdateEnabledForMuteToggle();
  if (MD::IsRefreshUi())
    RepaintSubsequentTab();
  Layout();
}

void Tab::AlertStateChanged() {
  Layout();
}

bool Tab::IsSelected() const {
  return controller_->IsTabSelected(this);
}

void Tab::SetData(TabRendererData data) {
  DCHECK(GetWidget());

  if (data_ == data)
    return;

  TabRendererData old(std::move(data_));
  data_ = std::move(data);

  // Icon updating must be done first because the title depends on whether the
  // loading animation is showing.
  icon_->SetIcon(data_.url, data_.favicon);
  icon_->SetNetworkState(data_.network_state, data_.should_hide_throbber);
  icon_->SetCanPaintToLayer(controller_->CanPaintThrobberToLayer());
  icon_->SetIsCrashed(data_.IsCrashed());
  if (IsActive()) {
    icon_->SetAttention(TabIcon::AttentionType::kBlockedWebContents, false);
  } else {
    // Only non-active WebContents get the blocked attention type because it's
    // confusing on the active tab.
    icon_->SetAttention(TabIcon::AttentionType::kBlockedWebContents,
                        data_.blocked);
  }

  base::string16 title = data_.title;
  if (title.empty()) {
    title = icon_->ShowingLoadingAnimation()
                ? l10n_util::GetStringUTF16(IDS_TAB_LOADING_TITLE)
                : CoreTabHelper::GetDefaultTitle();
  } else {
    Browser::FormatTitleForDisplay(&title);
  }
  title_->SetText(title);

  if (data_.alert_state != old.alert_state)
    alert_indicator_button_->TransitionToAlertState(data_.alert_state);
  if (old.pinned != data_.pinned)
    showing_alert_indicator_ = false;

  if (data_.alert_state != old.alert_state || data_.title != old.title)
    TooltipTextChanged();

  Layout();
  SchedulePaint();
}

void Tab::StepLoadingAnimation() {
  icon_->StepLoadingAnimation();

  // Update the layering if necessary.
  //
  // TODO(brettw) this design should be changed to be a push state when the tab
  // can't be painted to a layer, rather than continually polling the
  // controller about the state and reevaulating that state in the icon. This
  // is both overly aggressive and wasteful in the common case, and not
  // frequent enough in other cases since the state can be updated and the tab
  // painted before the animation is stepped.
  icon_->SetCanPaintToLayer(controller_->CanPaintThrobberToLayer());
}

void Tab::StartPulse() {
  pulse_animation_.StartThrobbing(std::numeric_limits<int>::max());
}

void Tab::StopPulse() {
  pulse_animation_.Stop();
}

void Tab::SetTabNeedsAttention(bool attention) {
  icon_->SetAttention(TabIcon::AttentionType::kTabWantsAttentionStatus,
                      attention);
  SchedulePaint();
}

int Tab::GetWidthOfLargestSelectableRegion() const {
  // Assume the entire region to the left of the alert indicator and/or close
  // buttons is available for click-to-select.  If neither are visible, the
  // entire tab region is available.
  const int indicator_left =
      showing_alert_indicator_ ? alert_indicator_button_->x() : width();
  const int close_button_left = showing_close_button_ ?
      close_button_->x() : width();
  return std::min(indicator_left, close_button_left);
}

// static
gfx::Size Tab::GetMinimumInactiveSize() {
  return gfx::Size(GetLayoutInsets(TAB).width(), GetLayoutConstant(TAB_HEIGHT));
}

// static
gfx::Size Tab::GetMinimumActiveSize() {
  gfx::Size minimum_size = GetMinimumInactiveSize();
  minimum_size.Enlarge(gfx::kFaviconSize, 0);
  return minimum_size;
}

// static
gfx::Size Tab::GetStandardSize() {
  const int kNetTabWidth = GetLayoutConstant(TAB_STANDARD_WIDTH);
  const int overlap = GetOverlap();
  return gfx::Size(kNetTabWidth + overlap, GetMinimumInactiveSize().height());
}

// static
int Tab::GetPinnedWidth() {
  constexpr int kTabPinnedContentWidth = 23;
  return GetMinimumInactiveSize().width() + kTabPinnedContentWidth;
}

// static
float Tab::GetInverseDiagonalSlope() {
  // This is computed from the border path as follows:
  // * The endcap width is enough for the whole stroke outer curve, i.e. the
  //   side diagonal plus the curves on both its ends.
  // * The bottom and top curve together are kMinimumEndcapWidth DIP wide, so
  //   the diagonal is (endcap_width - kMinimumEndcapWidth) DIP wide.
  // * The bottom and top curve are each 1.5 px high.  Additionally, there is an
  //   extra 1 px below the bottom curve and (scale - 1) px above the top curve,
  //   so the diagonal is ((height - 1.5 - 1.5) * scale - 1 - (scale - 1)) px
  //   high.  Simplifying this gives (height - 4) * scale px, or (height - 4)
  //   DIP.
  return (GetTabEndcapWidth() - kMinimumEndcapWidth) /
         (Tab::GetMinimumInactiveSize().height() - 4);
}

// static
int Tab::GetOverlap() {
  // We want to overlap the endcap portions entirely.
  return gfx::ToCeiledInt(GetTabEndcapWidth());
}

////////////////////////////////////////////////////////////////////////////////
// Tab, AnimationDelegate overrides:

void Tab::AnimationProgressed(const gfx::Animation* animation) {
  if (animation == &title_animation_) {
    title_->SetBoundsRect(gfx::Tween::RectValueBetween(
        gfx::Tween::CalculateValue(gfx::Tween::FAST_OUT_SLOW_IN,
                                   animation->GetCurrentValue()),
        start_title_bounds_, target_title_bounds_));
    return;
  }

  // Ignore if the pulse animation is being performed on active tab because
  // it repaints the same image. See PaintTab().
  if (animation == &pulse_animation_ && IsActive())
    return;

  SchedulePaint();
}

void Tab::AnimationCanceled(const gfx::Animation* animation) {
  SchedulePaint();
}

void Tab::AnimationEnded(const gfx::Animation* animation) {
  if (animation == &title_animation_)
    title_->SetBoundsRect(target_title_bounds_);
  else
    SchedulePaint();
}

////////////////////////////////////////////////////////////////////////////////
// Tab, views::ButtonListener overrides:

void Tab::ButtonPressed(views::Button* sender, const ui::Event& event) {
  if (!alert_indicator_button_ || !alert_indicator_button_->visible())
    base::RecordAction(UserMetricsAction("CloseTab_NoAlertIndicator"));
  else if (alert_indicator_button_->enabled())
    base::RecordAction(UserMetricsAction("CloseTab_MuteToggleAvailable"));
  else if (data_.alert_state == TabAlertState::AUDIO_PLAYING)
    base::RecordAction(UserMetricsAction("CloseTab_AudioIndicator"));
  else
    base::RecordAction(UserMetricsAction("CloseTab_RecordingIndicator"));

  const CloseTabSource source =
      (event.type() == ui::ET_MOUSE_RELEASED &&
       !(event.flags() & ui::EF_FROM_TOUCH)) ? CLOSE_TAB_FROM_MOUSE
                                             : CLOSE_TAB_FROM_TOUCH;
  DCHECK_EQ(close_button_, sender);
  controller_->CloseTab(this, source);
  if (event.type() == ui::ET_GESTURE_TAP)
    TouchUMA::RecordGestureAction(TouchUMA::GESTURE_TABCLOSE_TAP);
}

////////////////////////////////////////////////////////////////////////////////
// Tab, views::ContextMenuController overrides:

void Tab::ShowContextMenuForView(views::View* source,
                                 const gfx::Point& point,
                                 ui::MenuSourceType source_type) {
  if (!closing())
    controller_->ShowContextMenuForTab(this, point, source_type);
}

////////////////////////////////////////////////////////////////////////////////
// Tab, views::MaskedTargeterDelegate overrides:

bool Tab::GetHitTestMask(gfx::Path* mask) const {
  // When the window is maximized we don't want to shave off the edges or top
  // shadow of the tab, such that the user can click anywhere along the top
  // edge of the screen to select a tab. Ditto for immersive fullscreen.
  const views::Widget* widget = GetWidget();
  *mask =
      GetBorderPath(GetWidget()->GetCompositor()->device_scale_factor(), true,
                    widget && (widget->IsMaximized() || widget->IsFullscreen()),
                    GetTabEndcapWidth(), bounds());
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Tab, views::View overrides:

void Tab::ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) {
  // If this hierarchy changed has resulted in us being part of a widget
  // hierarchy for the first time, we can now get at the theme provider, and
  // should recalculate the button color.
  if (details.is_add)
    OnButtonColorMaybeChanged();
}

void Tab::OnPaint(gfx::Canvas* canvas) {
  // Don't paint if we're narrower than we can render correctly. (This should
  // only happen during animations).
  if (width() < GetMinimumInactiveSize().width() && !data().pinned)
    return;

  gfx::Path clip;
  if (!controller_->ShouldPaintTab(
          this,
          base::BindRepeating(&GetBorderPath, canvas->image_scale(), true,
                              false, GetTabEndcapWidth()),
          &clip))
    return;

  PaintTab(canvas, clip);
}

void Tab::PaintChildren(const views::PaintInfo& info) {
  // Clip children to 1 dp inside the tab's fill path.  This has no effect
  // except when the tab is too narrow to completely show even one icon, at
  // which point this serves to clip the favicon.
  ui::ClipRecorder clip_recorder(info.context());
  // The paint recording scale for tabs is consistent along the x and y axis.
  const float paint_recording_scale = info.paint_recording_scale_x();
  clip_recorder.ClipPathWithAntiAliasing(GetInteriorPath(
      paint_recording_scale, bounds(), GetTabEndcapWidth(), 1 /* padding */));
  View::PaintChildren(info);
}

void Tab::Layout() {
  const gfx::Rect lb = GetContentsBounds();
  const bool was_showing_icon = showing_icon_;
  UpdateIconVisibility();

  const int extra_padding = extra_padding_before_content_
                                ? kExtraLeftPaddingToBalanceCloseButtonPadding
                                : 0;
  const int start = lb.x() + extra_padding;

  // The bounds for the favicon will include extra width for the attention
  // indicator, but visually it will be smaller at kFaviconSize wide.
  gfx::Rect favicon_bounds(start, lb.y(), 0, 0);
  if (showing_icon_) {
    // Height should go to the bottom of the tab for the crashed tab animation
    // to pop out of the bottom.
    favicon_bounds.set_y(lb.y() + (lb.height() - gfx::kFaviconSize + 1) / 2);
    favicon_bounds.set_size(gfx::Size(icon_->GetPreferredSize().width(),
                                      lb.height() - favicon_bounds.y()));
    if (center_favicon_) {
      favicon_bounds.set_x(GetContentsBounds().CenterPoint().x() -
                           gfx::kFaviconSize / 2);
    } else {
      MaybeAdjustLeftForPinnedTab(&favicon_bounds, gfx::kFaviconSize);
    }
  }

  icon_->SetBoundsRect(favicon_bounds);
  icon_->SetVisible(showing_icon_);

  const int after_title_padding = GetLayoutConstant(TAB_AFTER_TITLE_PADDING);

  if (showing_close_button_) {
    // If the ratio of the close button size to tab width exceeds the maximum.
    // The close button should be as large as possible so that there is a larger
    // hit-target for touch events. So the close button bounds extends to the
    // edges of the tab. However, the larger hit-target should be active only
    // for touch events, and the close-image should show up in the right place.
    // So a border is added to the button with necessary padding. The close
    // button (Tab::TabCloseButton) makes sure the padding is a hit-target only
    // for touch events.
    // TODO(pkasting): The padding should maybe be removed, see comments in
    // TabCloseButton::TargetForRect().
    close_button_->SetBorder(views::NullBorder());
    const gfx::Size close_button_size(close_button_->GetPreferredSize());
    const int top = lb.y() + (lb.height() - close_button_size.height() + 1) / 2;
    const int left = after_title_padding;
    const int close_button_end = lb.right();
    close_button_->SetPosition(
        gfx::Point(close_button_end - close_button_size.width() - left, 0));
    const int bottom = height() - close_button_size.height() - top;
    const int right = width() - close_button_end;
    close_button_->SetBorder(
        views::CreateEmptyBorder(top, left, bottom, right));
    close_button_->SizeToPreferredSize();
  }
  close_button_->SetVisible(showing_close_button_);

  if (showing_alert_indicator_) {
    const bool is_touch_optimized = MD::IsTouchOptimizedUiEnabled();
    const gfx::Size image_size(alert_indicator_button_->GetPreferredSize());
    const int alert_to_close_spacing =
        is_touch_optimized ? after_title_padding : 0;
    const int right = showing_close_button_
                          ? close_button_->x() +
                                close_button_->GetInsets().left() -
                                alert_to_close_spacing
                          : lb.right();
    gfx::Rect bounds(
        std::max(lb.x(), right - image_size.width()),
        lb.y() + (lb.height() - image_size.height() + 1) / 2,
        image_size.width(),
        image_size.height());
    MaybeAdjustLeftForPinnedTab(&bounds, bounds.width());
    alert_indicator_button_->SetBoundsRect(bounds);
  }
  alert_indicator_button_->SetVisible(showing_alert_indicator_);

  // Size the title to fill the remaining width and use all available height.
  bool show_title = ShouldRenderAsNormalTab();
  if (show_title) {
    // When computing the spacing from the favicon, don't count the actual
    // icon view width (which will include extra room for the alert indicator),
    // but rather the normal favicon width which is what it will look like.
    const int title_left = showing_icon_
                               ? (favicon_bounds.x() + gfx::kFaviconSize +
                                  GetLayoutConstant(TAB_PRE_TITLE_PADDING))
                               : start;
    int title_right = lb.right();
    if (showing_alert_indicator_) {
      title_right = alert_indicator_button_->x() - after_title_padding;
    } else if (showing_close_button_) {
      // Allow the title to overlay the close button's empty border padding.
      title_right = close_button_->x() + close_button_->GetInsets().left() -
                    after_title_padding;
    }
    const int title_width = std::max(title_right - title_left, 0);
    // The Label will automatically center the font's cap height within the
    // provided vertical space.
    const gfx::Rect title_bounds(title_left, lb.y(), title_width, lb.height());
    show_title = title_width > 0;

    if (title_bounds != target_title_bounds_) {
      target_title_bounds_ = title_bounds;
      if (was_showing_icon == showing_icon_ || title_->bounds().IsEmpty() ||
          title_bounds.IsEmpty()) {
        title_animation_.Stop();
        title_->SetBoundsRect(title_bounds);
      } else if (!title_animation_.is_animating()) {
        start_title_bounds_ = title_->bounds();
        title_animation_.Start();
      }
    }
  }
  title_->SetVisible(show_title);
}

void Tab::OnThemeChanged() {
  OnButtonColorMaybeChanged();
}

const char* Tab::GetClassName() const {
  return kViewClassName;
}

bool Tab::GetTooltipText(const gfx::Point& p, base::string16* tooltip) const {
  // Note: Anything that affects the tooltip text should be accounted for when
  // calling TooltipTextChanged() from Tab::SetData().
  *tooltip = chrome::AssembleTabTooltipText(data_.title, data_.alert_state);
  return !tooltip->empty();
}

bool Tab::GetTooltipTextOrigin(const gfx::Point& p, gfx::Point* origin) const {
  origin->set_x(title_->x() + 10);
  origin->set_y(-4);
  return true;
}

bool Tab::OnMousePressed(const ui::MouseEvent& event) {
  controller_->OnMouseEventInTab(this, event);

  // Allow a right click from touch to drag, which corresponds to a long click.
  if (event.IsOnlyLeftMouseButton() ||
      (event.IsOnlyRightMouseButton() && event.flags() & ui::EF_FROM_TOUCH)) {
    ui::ListSelectionModel original_selection;
    original_selection = controller_->GetSelectionModel();
    // Changing the selection may cause our bounds to change. If that happens
    // the location of the event may no longer be valid. Create a copy of the
    // event in the parents coordinate, which won't change, and recreate an
    // event after changing so the coordinates are correct.
    ui::MouseEvent event_in_parent(event, static_cast<View*>(this), parent());
    if (controller_->SupportsMultipleSelection()) {
      if (event.IsShiftDown() && event.IsControlDown()) {
        controller_->AddSelectionFromAnchorTo(this);
      } else if (event.IsShiftDown()) {
        controller_->ExtendSelectionTo(this);
      } else if (event.IsControlDown()) {
        controller_->ToggleSelected(this);
        if (!IsSelected()) {
          // Don't allow dragging non-selected tabs.
          return false;
        }
      } else if (!IsSelected()) {
        controller_->SelectTab(this);
        base::RecordAction(UserMetricsAction("SwitchTab_Click"));
      }
    } else if (!IsSelected()) {
      controller_->SelectTab(this);
      base::RecordAction(UserMetricsAction("SwitchTab_Click"));
    }
    ui::MouseEvent cloned_event(event_in_parent, parent(),
                                static_cast<View*>(this));
    controller_->MaybeStartDrag(this, cloned_event, original_selection);
  }
  return true;
}

bool Tab::OnMouseDragged(const ui::MouseEvent& event) {
  controller_->ContinueDrag(this, event);
  return true;
}

void Tab::OnMouseReleased(const ui::MouseEvent& event) {
  controller_->OnMouseEventInTab(this, event);

  // Notify the drag helper that we're done with any potential drag operations.
  // Clean up the drag helper, which is re-created on the next mouse press.
  // In some cases, ending the drag will schedule the tab for destruction; if
  // so, bail immediately, since our members are already dead and we shouldn't
  // do anything else except drop the tab where it is.
  if (controller_->EndDrag(END_DRAG_COMPLETE))
    return;

  // Close tab on middle click, but only if the button is released over the tab
  // (normal windows behavior is to discard presses of a UI element where the
  // releases happen off the element).
  if (event.IsMiddleMouseButton()) {
    if (HitTestPoint(event.location())) {
      controller_->CloseTab(this, CLOSE_TAB_FROM_MOUSE);
    } else if (closing_) {
      // We're animating closed and a middle mouse button was pushed on us but
      // we don't contain the mouse anymore. We assume the user is clicking
      // quicker than the animation and we should close the tab that falls under
      // the mouse.
      Tab* closest_tab = controller_->GetTabAt(this, event.location());
      if (closest_tab)
        controller_->CloseTab(closest_tab, CLOSE_TAB_FROM_MOUSE);
    }
  } else if (event.IsOnlyLeftMouseButton() && !event.IsShiftDown() &&
             !event.IsControlDown()) {
    // If the tab was already selected mouse pressed doesn't change the
    // selection. Reset it now to handle the case where multiple tabs were
    // selected.
    controller_->SelectTab(this);

    if (alert_indicator_button_ && alert_indicator_button_->visible() &&
        alert_indicator_button_->bounds().Contains(event.location())) {
      base::RecordAction(UserMetricsAction("TabAlertIndicator_Clicked"));
    }
  }
}

void Tab::OnMouseCaptureLost() {
  controller_->EndDrag(END_DRAG_CAPTURE_LOST);
}

void Tab::OnMouseEntered(const ui::MouseEvent& event) {
  hover_controller_.Show(GlowHoverController::SUBTLE);
  if (MD::IsRefreshUi())
    RepaintSubsequentTab();
  Layout();
}

void Tab::OnMouseMoved(const ui::MouseEvent& event) {
  hover_controller_.SetLocation(event.location());
  controller_->OnMouseEventInTab(this, event);
}

void Tab::OnMouseExited(const ui::MouseEvent& event) {
  hover_controller_.Hide();
  if (MD::IsRefreshUi())
    RepaintSubsequentTab();
  Layout();
}

void Tab::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kTab;
  node_data->SetName(controller_->GetAccessibleTabName(this));
  node_data->AddState(ax::mojom::State::kMultiselectable);
  node_data->AddBoolAttribute(ax::mojom::BoolAttribute::kSelected,
                              IsSelected());
}

void Tab::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_TAP_DOWN: {
      // TAP_DOWN is only dispatched for the first touch point.
      DCHECK_EQ(1, event->details().touch_points());

      // See comment in OnMousePressed() as to why we copy the event.
      ui::GestureEvent event_in_parent(*event, static_cast<View*>(this),
                                       parent());
      ui::ListSelectionModel original_selection;
      original_selection = controller_->GetSelectionModel();
      tab_activated_with_last_tap_down_ = !IsActive();
      if (!IsSelected())
        controller_->SelectTab(this);
      gfx::Point loc(event->location());
      views::View::ConvertPointToScreen(this, &loc);
      ui::GestureEvent cloned_event(event_in_parent, parent(),
                                    static_cast<View*>(this));
      controller_->MaybeStartDrag(this, cloned_event, original_selection);
      break;
    }

    case ui::ET_GESTURE_END:
      controller_->EndDrag(END_DRAG_COMPLETE);
      break;

    case ui::ET_GESTURE_SCROLL_UPDATE:
      controller_->ContinueDrag(this, *event);
      break;

    default:
      break;
  }
  event->SetHandled();
}

////////////////////////////////////////////////////////////////////////////////
// Tab, private

void Tab::RepaintSubsequentTab() {
  Tab* adjacent_tab = controller_->GetAdjacentTab(this, TabController::FORWARD);
  if (adjacent_tab)
    adjacent_tab->SchedulePaint();
}

void Tab::MaybeAdjustLeftForPinnedTab(gfx::Rect* bounds,
                                      int visual_width) const {
  if (ShouldRenderAsNormalTab())
    return;
  const int ideal_delta = width() - GetPinnedWidth();
  const int ideal_x = (GetPinnedWidth() - visual_width) / 2;
  bounds->set_x(
      bounds->x() + static_cast<int>(
          (1 - static_cast<float>(ideal_delta) /
              static_cast<float>(kPinnedTabExtraWidthToRenderAsNormal)) *
          (ideal_x - bounds->x())));
}

void Tab::PaintTab(gfx::Canvas* canvas, const gfx::Path& clip) {
  int active_tab_fill_id = 0;
  int active_tab_y_offset = 0;
  if (GetThemeProvider()->HasCustomImage(IDR_THEME_TOOLBAR)) {
    active_tab_fill_id = IDR_THEME_TOOLBAR;
    active_tab_y_offset = -GetLayoutInsets(TAB).top();
  }

  if (IsActive()) {
    PaintTabBackground(canvas, true /* active */, active_tab_fill_id,
                       active_tab_y_offset, nullptr /* clip */);
  } else {
    PaintInactiveTabBackground(canvas, clip);

    const double throb_value = GetThrobValue();
    if (throb_value > 0) {
      canvas->SaveLayerAlpha(gfx::ToRoundedInt(throb_value * 0xff),
                             GetLocalBounds());
      PaintTabBackground(canvas, true /* active */, active_tab_fill_id,
                         active_tab_y_offset, nullptr /* clip */);
      canvas->Restore();
    }
  }
}

void Tab::PaintInactiveTabBackground(gfx::Canvas* canvas,
                                     const gfx::Path& clip) {
  bool has_custom_image;
  int fill_id = controller_->GetBackgroundResourceId(&has_custom_image);

  // The offset used to read from the image specified by |fill_id|.
  int y_offset = 0;

  if (!has_custom_image) {
    fill_id = 0;
  } else if (!GetThemeProvider()->HasCustomImage(fill_id)) {
    // If there's a custom frame image but no custom image for the tab itself,
    // then the tab's background will be the frame's image, so we need to
    // provide an offset into the image to read from.
    y_offset = background_offset_.y();
  }

  PaintTabBackground(canvas, false /* active */, fill_id, y_offset,
                     controller_->MaySetClip() ? &clip : nullptr);
}

void Tab::PaintTabBackground(gfx::Canvas* canvas,
                             bool active,
                             int fill_id,
                             int y_offset,
                             const gfx::Path* clip) {
  // |y_offset| is only set when |fill_id| is being used.
  DCHECK(!y_offset || fill_id);

  const float endcap_width = GetTabEndcapWidth();
  const ui::ThemeProvider* tp = GetThemeProvider();
  const SkColor active_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR);
  const SkColor inactive_color =
      tp->GetColor(ThemeProperties::COLOR_BACKGROUND_TAB);
  const SkColor stroke_color = controller_->GetToolbarTopSeparatorColor();
  const bool paint_hover_effect = !active && hover_controller_.ShouldDraw();

  // If there is a |fill_id| we don't try to cache. This could be improved
  // but would require knowing then the image from the ThemeProvider had been
  // changed, and invalidating when the tab's x-coordinate or background_offset_
  // changed.
  // Similarly, if |paint_hover_effect|, we don't try to cache since hover
  // effects change on every invalidation and we would need to invalidate the
  // cache based on the hover states.
  if (fill_id || paint_hover_effect) {
    gfx::Path fill_path =
        GetInteriorPath(canvas->image_scale(), bounds(), endcap_width);
    PaintTabBackgroundFill(canvas, fill_path, active, paint_hover_effect,
                           active_color, inactive_color, fill_id, y_offset);
    if (TabStrip::ShouldDrawStrokes()) {
      gfx::Path stroke_path = GetBorderPath(canvas->image_scale(), false, false,
                                            endcap_width, bounds());
      gfx::ScopedCanvas scoped_canvas(clip ? canvas : nullptr);
      if (clip)
        canvas->sk_canvas()->clipPath(*clip, SkClipOp::kDifference, true);
      PaintTabBackgroundStroke(canvas, fill_path, stroke_path, active,
                               stroke_color);
    }
  } else {
    BackgroundCache& cache =
        active ? background_active_cache_ : background_inactive_cache_;
    if (!cache.CacheKeyMatches(canvas->image_scale(), size(), active_color,
                               inactive_color, stroke_color)) {
      gfx::Path fill_path =
          GetInteriorPath(canvas->image_scale(), bounds(), endcap_width);
      gfx::Path stroke_path = GetBorderPath(canvas->image_scale(), false, false,
                                            endcap_width, bounds());
      cc::PaintRecorder recorder;

      {
        gfx::Canvas cache_canvas(
            recorder.beginRecording(size().width(), size().height()),
            canvas->image_scale());
        PaintTabBackgroundFill(&cache_canvas, fill_path, active,
                               paint_hover_effect, active_color, inactive_color,
                               fill_id, y_offset);
        cache.fill_record = recorder.finishRecordingAsPicture();
      }
      if (TabStrip::ShouldDrawStrokes()) {
        gfx::Canvas cache_canvas(
            recorder.beginRecording(size().width(), size().height()),
            canvas->image_scale());
        PaintTabBackgroundStroke(&cache_canvas, fill_path, stroke_path, active,
                                 stroke_color);
        cache.stroke_record = recorder.finishRecordingAsPicture();
      }

      cache.SetCacheKey(canvas->image_scale(), size(), active_color,
                        inactive_color, stroke_color);
    }

    canvas->sk_canvas()->drawPicture(cache.fill_record);
    if (TabStrip::ShouldDrawStrokes()) {
      gfx::ScopedCanvas scoped_canvas(clip ? canvas : nullptr);
      if (clip)
        canvas->sk_canvas()->clipPath(*clip, SkClipOp::kDifference, true);
      canvas->sk_canvas()->drawPicture(cache.stroke_record);
    }
  }

  if (!active)
    PaintSeparator(canvas, inactive_color);
}

void Tab::PaintTabBackgroundFill(gfx::Canvas* canvas,
                                 const gfx::Path& fill_path,
                                 bool active,
                                 bool paint_hover_effect,
                                 SkColor active_color,
                                 SkColor inactive_color,
                                 int fill_id,
                                 int y_offset) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  canvas->ClipPath(fill_path, true);
  if (fill_id) {
    gfx::ScopedCanvas scale_scoper(canvas);
    canvas->sk_canvas()->scale(scale, scale);
    canvas->TileImageInt(*GetThemeProvider()->GetImageSkiaNamed(fill_id),
                         GetMirroredX() + background_offset_.x(), y_offset, 0,
                         0, width(), height());
  } else {
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(active ? active_color : inactive_color);
    canvas->DrawRect(gfx::ScaleToEnclosingRect(GetLocalBounds(), scale), flags);
  }

  if (paint_hover_effect) {
    SkPoint hover_location(gfx::PointToSkPoint(hover_controller_.location()));
    hover_location.scale(SkFloatToScalar(scale));
    const SkScalar kMinHoverRadius = 16;
    const SkScalar radius =
        std::max(SkFloatToScalar(width() / 4.f), kMinHoverRadius);
    DrawHighlight(canvas, hover_location, radius * scale,
                  SkColorSetA(active_color, hover_controller_.GetAlpha()));
  }
}

void Tab::PaintTabBackgroundStroke(gfx::Canvas* canvas,
                                   const gfx::Path& fill_path,
                                   const gfx::Path& stroke_path,
                                   bool active,
                                   SkColor color) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  if (!active) {
    // Clip out the bottom line; this will be drawn for us by
    // TabStrip::PaintChildren().
    canvas->ClipRect(gfx::RectF(width() * scale, height() * scale - 1));
  }
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(color);
  SkPath path;
  Op(stroke_path, fill_path, kDifference_SkPathOp, &path);
  canvas->DrawPath(path, flags);
}

void Tab::PaintSeparator(gfx::Canvas* canvas, SkColor inactive_color) {
  if (!MD::IsRefreshUi())
    return;

  // If the tab to the left is active, the separator on this tab should not be
  // painted.
  Tab* previous_tab =
      controller_->GetAdjacentTab(this, TabController::BACKWARD);
  if (previous_tab && previous_tab->IsActive())
    return;

  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  const float endcap_width = GetTabEndcapWidth();
  const gfx::RectF layout_bounds =
      ScaleAndAlignBounds(bounds(), endcap_width, scale);

  gfx::RectF separator_bounds;
  separator_bounds.set_size(gfx::SizeF(
      scale, (MD::IsTouchOptimizedUiEnabled() ? kTabSeparatorTouchHeight
                                              : kTabSeparatorHeight) *
                 scale));
  separator_bounds.set_origin(gfx::PointF(
      layout_bounds.x() + (endcap_width / 2) * scale,
      layout_bounds.y() +
          (layout_bounds.height() - separator_bounds.height()) / 2));

  gfx::PointF origin(bounds().origin());
  origin.Scale(scale);
  separator_bounds.Offset(-origin.x(), -origin.y());
  // The following will paint the separator using an opacity that should
  // cross-fade with the maximum hover animation value of this tab or the
  // tab to the left. This will have the effect of fading out the separator
  // while this tab's or the tab to the left's hover animation is progressing.
  const double max_hover_value = std::max(
      hover_controller_.GetAnimationValue(),
      previous_tab ? previous_tab->hover_controller()->GetAnimationValue() : 0);
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(
      SkColorSetA(color_utils::BlendTowardOppositeLuma(inactive_color, 0x5a),
                  gfx::Tween::IntValueBetween(max_hover_value, 255, 0)));
  canvas->DrawRect(separator_bounds, flags);
}

void Tab::UpdateIconVisibility() {
  center_favicon_ = false;
  showing_icon_ = showing_alert_indicator_ = showing_close_button_ = false;
  extra_padding_before_content_ = false;

  const gfx::Size min_size(GetMinimumInactiveSize());
  if (height() < min_size.height())
    return;

  int available_width = std::max(0, width() - min_size.width());

  const bool is_touch_optimized = MD::IsTouchOptimizedUiEnabled();
  const int favicon_width = gfx::kFaviconSize;
  const int alert_icon_width =
      alert_indicator_button_->GetPreferredSize().width();
  // In case of touch optimized UI, the close button has an extra padding on the
  // left that needs to be considered.
  const int close_button_width =
      close_button_->GetPreferredSize().width() -
      (is_touch_optimized ? close_button_->GetInsets().right()
                          : close_button_->GetInsets().width());

  int extra_padding = kExtraLeftPaddingToBalanceCloseButtonPadding;

  const bool is_pinned = data().pinned;
  const bool is_active = IsActive();
  const bool has_favicon = data().show_icon;
  const bool has_alert_icon =
      (alert_indicator_button_ ? alert_indicator_button_->showing_alert_state()
                               : data().alert_state) != TabAlertState::NONE;
  const bool hide_inactive_close_button =
      controller_->ShouldHideCloseButtonForInactiveTabs();
  const bool show_close_button_on_hover =
      controller_->ShouldShowCloseButtonOnHover();

  if (is_pinned) {
    // When the tab is pinned, we can show one of the two icons. Alert icon
    // is given priority over the favicon. We never show the close buton if the
    // tab is pinned.
    showing_alert_indicator_ = has_alert_icon;
    showing_icon_ = has_favicon && !has_alert_icon;
  } else {
    if (is_active) {
      // The close button is always visible for an active tab.
      showing_close_button_ = true;
      available_width -= close_button_width;

      showing_alert_indicator_ =
          has_alert_icon && alert_icon_width <= available_width;
      available_width -= showing_alert_indicator_ ? alert_icon_width : 0;

      // If all 3 icons are visible, we add an extra left padding for favicon.
      // See comment for |extra_padding_before_content_|.
      if (!showing_alert_indicator_)
        extra_padding = 0;

      showing_icon_ =
          has_favicon && favicon_width + extra_padding <= available_width;
    } else {
      showing_alert_indicator_ =
          has_alert_icon && alert_icon_width <= available_width;
      available_width -= showing_alert_indicator_ ? alert_icon_width : 0;

      showing_icon_ = has_favicon && favicon_width <= available_width;
      available_width -= showing_icon_ ? favicon_width : 0;

      // If all 3 icons are visible, we add an extra padding to the left of
      // favicon. See comment for |extra_padding_before_content_|.
      if (!showing_icon_ || !showing_alert_indicator_)
        extra_padding = 0;

      // For an inactive tab, the close button will be visible only when
      // it is not forced to hide and the total width can accomodate all 3
      // icons. When favicon or alert button is not visible, its space
      // will be occupied by the title of this tab.
      int title_width =
          (!showing_icon_ + !showing_alert_indicator_) * favicon_width;
      if ((!hide_inactive_close_button ||
           (show_close_button_on_hover && hover_controller_.ShouldDraw())) &&
          (title_width + close_button_width + extra_padding <=
           available_width)) {
        showing_close_button_ = true;
      }

      // If no other controls are visible, show favicon even though we
      // don't have enough space. We'll clip the favicon in PaintChildren().
      if (!showing_close_button_ && !showing_alert_indicator_ &&
          !showing_icon_ && has_favicon) {
        showing_icon_ = true;
        center_favicon_ = true;
      }
    }
    extra_padding_before_content_ =
        showing_close_button_ && showing_icon_ && showing_alert_indicator_;
  }
}

bool Tab::ShouldRenderAsNormalTab() const {
  return !data().pinned ||
      (width() >= (GetPinnedWidth() + kPinnedTabExtraWidthToRenderAsNormal));
}

double Tab::GetThrobValue() {
  const bool is_selected = IsSelected();
  double val = is_selected ? kSelectedTabOpacity : 0;
  const double offset =
      is_selected ? (kSelectedTabThrobScale * kHoverOpacity) : kHoverOpacity;

  if (pulse_animation_.is_animating())
    val += pulse_animation_.GetCurrentValue() * offset;
  else if (hover_controller_.ShouldDraw())
    val += hover_controller_.GetAnimationValue() * offset;
  return val;
}

void Tab::OnButtonColorMaybeChanged() {
  // The theme provider may be null if we're not currently in a widget
  // hierarchy.
  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  if (!theme_provider)
    return;

  const SkColor title_color = theme_provider->GetColor(IsActive() ?
      ThemeProperties::COLOR_TAB_TEXT :
      ThemeProperties::COLOR_BACKGROUND_TAB_TEXT);

  SkColor new_button_color = title_color;
  if (IsActive()) {
    // This alpha value (0x2f) blends GoogleGrey800 close to GoogleGrey700.
    new_button_color = color_utils::BlendTowardOppositeLuma(title_color, 0x2f);
  }

  if (button_color_ != new_button_color) {
    button_color_ = new_button_color;
    title_->SetEnabledColor(title_color);
    alert_indicator_button_->OnParentTabButtonColorChanged();
    if (!MD::IsTouchOptimizedUiEnabled()) {
      close_button_->SetTabColor(button_color_,
                                 color_utils::IsDark(theme_provider->GetColor(
                                     ThemeProperties::COLOR_TOOLBAR)));
    }
  }
  if (MD::IsTouchOptimizedUiEnabled())
    close_button_->ActiveStateChanged(this);
}

Tab::BackgroundCache::BackgroundCache() = default;
Tab::BackgroundCache::~BackgroundCache() = default;
