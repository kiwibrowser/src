// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/new_tab_button.h"

#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/feature_promos/new_tab_promo_bubble_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/theme_resources.h"
#include "components/feature_engagement/buildflags.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/effects/SkLayerDrawLooper.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/base/default_theme_provider.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "ui/display/win/screen_win.h"
#include "ui/gfx/win/hwnd_util.h"
#include "ui/views/win/hwnd_util.h"
#endif

#if BUILDFLAG(ENABLE_DESKTOP_IN_PRODUCT_HELP)
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker_factory.h"
#include "chrome/browser/ui/views/feature_promos/new_tab_promo_bubble_view.h"
#endif

using MD = ui::MaterialDesignController;

namespace {

constexpr int kDistanceBetweenIcons = 6;

constexpr int kStrokeThickness = 1;

sk_sp<SkDrawLooper> CreateShadowDrawLooper(SkColor color) {
  SkLayerDrawLooper::Builder looper_builder;
  looper_builder.addLayer();

  SkLayerDrawLooper::LayerInfo layer_info;
  layer_info.fPaintBits |= SkLayerDrawLooper::kMaskFilter_Bit;
  layer_info.fPaintBits |= SkLayerDrawLooper::kColorFilter_Bit;
  layer_info.fColorMode = SkBlendMode::kDst;
  layer_info.fOffset.set(0, 1);
  SkPaint* layer_paint = looper_builder.addLayer(layer_info);
  layer_paint->setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 0.5));
  layer_paint->setColorFilter(
      SkColorFilter::MakeModeFilter(color, SkBlendMode::kSrcIn));
  return looper_builder.detach();
}

// Returns the ID of the resource that should be used for the button fill if
// any. |has_custom_image| will be set to true if the images of either the
// tab, the frame background, (or the toolbar if |is_touch_ui| is true) have
// been customized.
int GetButtonFillResourceIdIfAny(const TabStrip* tab_strip,
                                 const ui::ThemeProvider* theme_provider,
                                 bool is_touch_ui,
                                 bool* has_custom_image) {
  if (!is_touch_ui)
    return tab_strip->GetBackgroundResourceId(has_custom_image);

  constexpr int kTouchBackgroundId = IDR_THEME_TOOLBAR;
  *has_custom_image = theme_provider->HasCustomImage(kTouchBackgroundId);
  return kTouchBackgroundId;
}

}  // namespace

NewTabButton::NewTabButton(TabStrip* tab_strip, views::ButtonListener* listener)
    : views::ImageButton(listener),
      tab_strip_(tab_strip),
      is_incognito_(tab_strip->IsIncognito()) {
  set_animate_on_state_change(true);
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  set_triggerable_event_flags(triggerable_event_flags() |
                              ui::EF_MIDDLE_MOUSE_BUTTON);
#endif

  if (MD::IsNewerMaterialUi()) {
    // Initialize the ink drop mode for a ripple highlight on button press.
    ink_drop_container_ = new views::InkDropContainerView();
    AddChildView(ink_drop_container_);
    ink_drop_container_->SetVisible(false);

    SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);
    set_ink_drop_visible_opacity(0.14f);
  }
}

NewTabButton::~NewTabButton() {
  if (destroyed_)
    *destroyed_ = true;
}

// static
int NewTabButton::GetTopOffset() {
  // We're only interested in the button's height which doesn't change based
  // on incognito or not, so we used `is_incognito=false`.
  const int extra_vertical_space =
      Tab::GetMinimumInactiveSize().height() -
      GetLayoutSize(NEW_TAB_BUTTON, false /* is_incognito */).height();

  // In newer material UI, the button is placed vertically exactly in the
  // center of the tabstrip.
  if (MD::IsNewerMaterialUi())
    return extra_vertical_space / 2;

  // In the non-touch-optimized UI, the new tab button is placed at a fixed
  // distance from the bottom of the tabstrip.
  constexpr int kNewTabButtonBottomOffset = 4;
  return extra_vertical_space - kNewTabButtonBottomOffset;
}

// static
void NewTabButton::ShowPromoForLastActiveBrowser() {
  BrowserView* browser = static_cast<BrowserView*>(
      BrowserList::GetInstance()->GetLastActive()->window());

  browser->tabstrip()->new_tab_button()->ShowPromo();
}

// static
void NewTabButton::CloseBubbleForLastActiveBrowser() {
  BrowserView* browser = static_cast<BrowserView*>(
      BrowserList::GetInstance()->GetLastActive()->window());
  browser->tabstrip()->new_tab_button()->CloseBubble();
}

void NewTabButton::ShowPromo() {
  DCHECK(!new_tab_promo_);
  // Owned by its native widget. Will be destroyed as its widget is destroyed.
  new_tab_promo_ = NewTabPromoBubbleView::CreateOwned(this);
  new_tab_promo_observer_.Add(new_tab_promo_->GetWidget());
  SchedulePaint();
}

void NewTabButton::CloseBubble() {
  if (new_tab_promo_)
    new_tab_promo_->CloseBubble();
}

void NewTabButton::AnimateInkDropToStateForTesting(views::InkDropState state) {
  GetInkDrop()->AnimateToState(state);
}

#if defined(OS_WIN)
void NewTabButton::OnMouseReleased(const ui::MouseEvent& event) {
  if (event.IsOnlyRightMouseButton()) {
    gfx::Point point = event.location();
    views::View::ConvertPointToScreen(this, &point);
    point = display::win::ScreenWin::DIPToScreenPoint(point);
    bool destroyed = false;
    destroyed_ = &destroyed;
    gfx::ShowSystemMenuAtPoint(views::HWNDForView(this), point);
    if (destroyed)
      return;

    destroyed_ = NULL;
    SetState(views::Button::STATE_NORMAL);
    return;
  }
  views::ImageButton::OnMouseReleased(event);
}
#endif

void NewTabButton::OnGestureEvent(ui::GestureEvent* event) {
  // Consume all gesture events here so that the parent (Tab) does not
  // start consuming gestures.
  views::ImageButton::OnGestureEvent(event);
  event->SetHandled();
}

void NewTabButton::AddInkDropLayer(ui::Layer* ink_drop_layer) {
  DCHECK(ink_drop_layer->bounds().size() == GetVisibleBounds().size());
  DCHECK(ink_drop_container_->bounds().size() == GetVisibleBounds().size());
  ink_drop_container_->AddInkDropLayer(ink_drop_layer);
  InstallInkDropMask(ink_drop_layer);
}

void NewTabButton::RemoveInkDropLayer(ui::Layer* ink_drop_layer) {
  ResetInkDropMask();
  ink_drop_container_->RemoveInkDropLayer(ink_drop_layer);
}

std::unique_ptr<views::InkDropRipple> NewTabButton::CreateInkDropRipple()
    const {
  return std::make_unique<views::FloodFillInkDropRipple>(
      GetVisibleBounds().size(), gfx::Insets(),
      GetInkDropCenterBasedOnLastEvent(), GetInkDropBaseColor(),
      ink_drop_visible_opacity());
}

void NewTabButton::NotifyClick(const ui::Event& event) {
  ImageButton::NotifyClick(event);
  GetInkDrop()->AnimateToState(views::InkDropState::ACTION_TRIGGERED);
}

std::unique_ptr<views::InkDrop> NewTabButton::CreateInkDrop() {
  std::unique_ptr<views::InkDropImpl> ink_drop =
      std::make_unique<views::InkDropImpl>(this, GetVisibleBounds().size());
  ink_drop->SetAutoHighlightMode(views::InkDropImpl::AutoHighlightMode::NONE);
  ink_drop->SetShowHighlightOnHover(false);
  UpdateInkDropBaseColor();
  return ink_drop;
}

std::unique_ptr<views::InkDropMask> NewTabButton::CreateInkDropMask() const {
  return std::make_unique<views::RoundRectInkDropMask>(
      GetVisibleBounds().size(), gfx::Insets(), GetCornerRadius());
}

void NewTabButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  const int visible_height =
      GetLayoutSize(NEW_TAB_BUTTON, is_incognito_).height();
  canvas->Translate(gfx::Vector2d(0, height() - visible_height));
  const float scale = canvas->image_scale();
  const bool pressed = state() == views::Button::STATE_PRESSED;
  const SkColor stroke_color =
      new_tab_promo_observer_.IsObservingSources()
          ? color_utils::AlphaBlend(
                SK_ColorBLACK,
                GetNativeTheme()->GetSystemColor(
                    ui::NativeTheme::kColorId_ProminentButtonColor),
                0x70)
          : tab_strip_->GetToolbarTopSeparatorColor();

  // Fill.
  SkPath fill, stroke;
  const bool is_touch_ui = MD::IsTouchOptimizedUiEnabled();
  if (!MD::IsRefreshUi()) {
    fill = is_touch_ui ? GetTouchOptimizedButtonPath(0, scale, false, true)
                       : GetNonTouchOptimizedButtonPath(0, visible_height,
                                                        scale, false, true);
    PaintFill(pressed, scale, fill, canvas);

    // Stroke.
    GetBorderPath(0, scale, false, &stroke);
  }

  if (MD::IsNewerMaterialUi()) {
    cc::PaintFlags paint_flags;
    paint_flags.setAntiAlias(true);

    const int plus_icon_offset = GetCornerRadius() - (plus_icon_.width() / 2);
    canvas->DrawImageInt(plus_icon_, plus_icon_offset, plus_icon_offset,
                         paint_flags);
    if (ShouldDrawIncognitoIcon()) {
      DCHECK(!incognito_icon_.isNull());
      canvas->DrawImageInt(
          incognito_icon_,
          plus_icon_offset + plus_icon_.width() + kDistanceBetweenIcons,
          plus_icon_offset, paint_flags);
    }

    if (is_touch_ui) {
      // Draw stroke.
      // In the touch-optimized UI design, the new tab button is rendered flat,
      // regardless of whether pressed or not (i.e. we don't emulate a pushed
      // button by drawing a drop shadow). Instead, we're using an ink drop
      // ripple effect.
      // Here we want to make sure the stroke width is 1px regardless of the
      // device scale factor, so undo the scale.
      canvas->UndoDeviceScaleFactor();
      Op(stroke, fill, kDifference_SkPathOp, &stroke);
      paint_flags.setColor(stroke_color);
      canvas->DrawPath(stroke, paint_flags);
    }
  } else {
    // We want to draw a drop shadow either inside or outside the stroke,
    // depending on whether we're pressed; so, either clip out what's outside
    // the stroke, or clip out the fill inside it.
    canvas->UndoDeviceScaleFactor();
    if (pressed)
      canvas->ClipPath(stroke, true);
    Op(stroke, fill, kDifference_SkPathOp, &stroke);
    if (!pressed)
      canvas->sk_canvas()->clipPath(fill, SkClipOp::kDifference, true);
    // Now draw the stroke and shadow; the stroke will always be visible, while
    // the shadow will be affected by the clip we set above.
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    const float alpha = SkColorGetA(stroke_color);
    const SkAlpha shadow_alpha =
        base::saturated_cast<SkAlpha>(std::round(2.1875f * alpha));
    flags.setLooper(
        CreateShadowDrawLooper(SkColorSetA(stroke_color, shadow_alpha)));
    const SkAlpha path_alpha = static_cast<SkAlpha>(
        std::round((pressed ? 0.875f : 0.609375f) * alpha));
    flags.setColor(SkColorSetA(stroke_color, path_alpha));
    canvas->DrawPath(stroke, flags);
  }
}

void NewTabButton::Layout() {
  ImageButton::Layout();

  if (MD::IsNewerMaterialUi()) {
    // If icons are not initialized, initialize them now. Icons are always
    // initialized together so it's enough to check the |plus_icon_|.
    if (plus_icon_.isNull())
      InitButtonIcons();

    const gfx::Rect ink_drop_bounds(gfx::Point(0, GetTopOffset()),
                                    GetVisibleBounds().size());
    ink_drop_container_->SetBoundsRect(ink_drop_bounds);
  }
}

void NewTabButton::OnThemeChanged() {
  ImageButton::OnThemeChanged();

  if (!MD::IsNewerMaterialUi())
    return;

  InitButtonIcons();
  UpdateInkDropBaseColor();
}

void NewTabButton::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  const gfx::Size ink_drop_size = GetVisibleBounds().size();
  GetInkDrop()->HostSizeChanged(ink_drop_size);
  UpdateInkDropMaskLayerSize(ink_drop_size);
}

bool NewTabButton::GetHitTestMask(gfx::Path* mask) const {
  DCHECK(mask);

  SkPath border;
  const float scale = GetWidget()->GetCompositor()->device_scale_factor();
  GetBorderPath(GetTopOffset() * scale, scale,
                tab_strip_->SizeTabButtonToTopOfTabStrip(), &border);
  mask->addPath(border, SkMatrix::MakeScale(1 / scale));
  return true;
}

void NewTabButton::OnWidgetDestroying(views::Widget* widget) {
#if BUILDFLAG(ENABLE_DESKTOP_IN_PRODUCT_HELP)
  feature_engagement::NewTabTrackerFactory::GetInstance()
      ->GetForProfile(tab_strip_->controller()->GetProfile())
      ->OnPromoClosed();
#endif
  new_tab_promo_observer_.Remove(widget);
  new_tab_promo_ = nullptr;
  // When the promo widget is destroyed, the NewTabButton needs to be
  // recolored.
  SchedulePaint();
}

bool NewTabButton::ShouldDrawIncognitoIcon() const {
  return is_incognito_ && MD::IsTouchOptimizedUiEnabled();
}

gfx::Rect NewTabButton::GetVisibleBounds() const {
  // TODO(pkasting): This is correct but inefficient for newer material UI.
  SkPath border;
  GetBorderPath(GetTopOffset(), 1.0f /* scale */, false, &border);
  gfx::Rect rect = gfx::ToEnclosingRect(gfx::SkRectToRectF(border.getBounds()));
  ConvertRectToScreen(this, &rect);
  return rect;
}

int NewTabButton::GetCornerRadius() const {
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_HIGH, GetLayoutSize(NEW_TAB_BUTTON, is_incognito_));
}

void NewTabButton::GetBorderPath(float button_y,
                                 float scale,
                                 bool extend_to_top,
                                 SkPath* path) const {
  const int button_height =
      GetLayoutSize(NEW_TAB_BUTTON, is_incognito_).height();

  if (MD::IsRefreshUi()) {
    path->addRect(0, extend_to_top ? 0 : button_y, width() * scale,
                  button_y + button_height * scale);
    return;
  }

  *path =
      MD::IsTouchOptimizedUiEnabled()
          ? GetTouchOptimizedButtonPath(button_y, scale, extend_to_top, false)
          : GetNonTouchOptimizedButtonPath(button_y, button_height, scale,
                                           extend_to_top, false);
}

void NewTabButton::PaintFill(bool pressed,
                             float scale,
                             const SkPath& fill,
                             gfx::Canvas* canvas) const {
  gfx::ScopedCanvas scoped_canvas(canvas);
  canvas->UndoDeviceScaleFactor();
  cc::PaintFlags flags;
  flags.setAntiAlias(true);

  // For unpressed buttons, draw the fill and its shadow.
  // Note that for touch-optimized UI, we always draw the fill since the button
  // has a flat design with no hover highlight.
  const bool is_touch_ui = MD::IsTouchOptimizedUiEnabled();
  if (is_touch_ui || !pressed) {
    // First we compute the background image coordinates and scale, in case we
    // need to draw a custom background image.
    const ui::ThemeProvider* tp = GetThemeProvider();
    bool custom_image;
    const int bg_id = GetButtonFillResourceIdIfAny(tab_strip_, tp, is_touch_ui,
                                                   &custom_image);
    if (custom_image && !new_tab_promo_observer_.IsObservingSources()) {
      // For non-touch, the background starts at |background_offset_| unless
      // there's a custom tab background image, which starts at the top of
      // the tabstrip (which is also the top of this button, i.e. y = 0).
      const int non_touch_offset_y =
          tp->HasCustomImage(bg_id) ? 0 : background_offset_.y();
      // For touch, the background matches the active tab background
      // positioning in Tab::PaintTab().
      const int offset_y =
          is_touch_ui ? -GetLayoutInsets(TAB).top() : non_touch_offset_y;
      // The new tab background is mirrored in RTL mode, but the theme
      // background should never be mirrored. Mirror it here to compensate.
      float x_scale = 1.0f;
      int x = GetMirroredX() + background_offset_.x();
      const gfx::Size size(GetLayoutSize(NEW_TAB_BUTTON, is_incognito_));
      if (base::i18n::IsRTL()) {
        x_scale = -1.0f;
        // Offset by |width| such that the same region is painted as if there
        // was no flip.
        x += size.width();
      }

      const bool succeeded = canvas->InitPaintFlagsForTiling(
          *tp->GetImageSkiaNamed(bg_id), x, GetTopOffset() + offset_y,
          x_scale * scale, scale, 0, 0, &flags);
      DCHECK(succeeded);
    } else {
      flags.setColor(GetButtonFillColor());
    }

    const SkColor stroke_color = tab_strip_->GetToolbarTopSeparatorColor();
    const SkAlpha alpha =
        static_cast<SkAlpha>(std::round(SkColorGetA(stroke_color) * 0.59375f));
    cc::PaintFlags shadow_flags = flags;
    shadow_flags.setLooper(
        CreateShadowDrawLooper(SkColorSetA(stroke_color, alpha)));
    canvas->DrawPath(fill, shadow_flags);

    if (is_touch_ui) {
      // We don't have hover/pressed states in the touch-optimized UI design.
      // Instead we are using an ink drop effect.
      return;
    }
  }

  // Draw a white highlight on hover.
  const SkAlpha hover_alpha =
      static_cast<SkAlpha>(hover_animation().CurrentValueBetween(0x00, 0x4D));
  if (hover_alpha != SK_AlphaTRANSPARENT) {
    flags.setColor(SkColorSetA(SK_ColorWHITE, hover_alpha));
    canvas->DrawPath(fill, flags);
  }

  // Most states' opacities are adjusted using an opacity recorder in
  // TabStrip::PaintChildren(), but the pressed state is excluded there and
  // instead rendered using a dark overlay here.  Avoiding the use of the
  // opacity recorder keeps the stroke more visible in this state.
  if (pressed) {
    flags.setColor(SkColorSetA(SK_ColorBLACK, 0x14));
    canvas->DrawPath(fill, flags);
  }
}

SkColor NewTabButton::GetButtonFillColor() const {
  if (new_tab_promo_observer_.IsObservingSources()) {
    return GetNativeTheme()->GetSystemColor(
        ui::NativeTheme::kColorId_ProminentButtonColor);
  }

  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  DCHECK(theme_provider);
  return theme_provider->GetColor(
      ui::MaterialDesignController::IsTouchOptimizedUiEnabled()
          ? ThemeProperties::COLOR_TOOLBAR
          : ThemeProperties::COLOR_BACKGROUND_TAB);
}

void NewTabButton::InitButtonIcons() {
  DCHECK(MD::IsNewerMaterialUi());

  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  DCHECK(theme_provider);
  const SkColor icon_color =
      theme_provider->GetColor(ThemeProperties::COLOR_TAB_TEXT);
  plus_icon_ = gfx::CreateVectorIcon(kNewTabButtonPlusIcon, icon_color);
  if (ShouldDrawIncognitoIcon()) {
    incognito_icon_ =
        gfx::CreateVectorIcon(kNewTabButtonIncognitoIcon, icon_color);
  }
}

SkPath NewTabButton::GetTouchOptimizedButtonPath(float button_y,
                                                 float scale,
                                                 bool extend_to_top,
                                                 bool for_fill) const {
  DCHECK(MD::IsTouchOptimizedUiEnabled());

  const float radius = GetCornerRadius() * scale;
  const float rect_width =
      2 * radius +
      (is_incognito_ ? scale * (incognito_icon_.width() + kDistanceBetweenIcons)
                     : 0);

  const SkRect button_rect =
      SkRect::MakeXYWH(0, button_y, rect_width, 2 * radius);
  SkRRect rrect = SkRRect::MakeRectXY(button_rect, radius, radius);
  // Inset by 1px for a fill path to give room for the stroke to show up. The
  // stroke width is 1px regardless of the device scale factor.
  if (for_fill)
    rrect.inset(kStrokeThickness, kStrokeThickness);

  SkPath path;
  path.addRRect(rrect, SkPath::kCW_Direction);

  if (extend_to_top) {
    SkPath extension_path;
    extension_path.addRect(
        SkRect::MakeXYWH(0, 0, rect_width, button_y + radius),
        SkPath::kCW_Direction);
    Op(path, extension_path, kUnion_SkPathOp, &path);
  }

  path.close();
  return path;
}

SkPath NewTabButton::GetNonTouchOptimizedButtonPath(int button_y,
                                                    int button_height,
                                                    float scale,
                                                    bool extend_to_top,
                                                    bool for_fill) const {
  const float inverse_slope = Tab::GetInverseDiagonalSlope();
  float bottom = (button_height - 2) * scale;
  const float diag_height = bottom - 3.5 * scale;
  const float diag_width = diag_height * inverse_slope;
  const float right = diag_width + 4 * scale;
  const int stroke_thickness = for_fill ? 0 : kStrokeThickness;
  bottom += button_y + stroke_thickness;

  SkPath path;
  path.moveTo(right - stroke_thickness, bottom);
  path.rCubicTo(-0.75 * scale, 0, -1.625 * scale, -0.5 * scale, -2 * scale,
                -1.5 * scale);
  path.rLineTo(-diag_width, -diag_height);
  if (extend_to_top) {
    // Create the vertical extension by extending the side diagonals at the
    // upper left and lower right corners until they reach the top and bottom of
    // the border, respectively (in other words, "un-round-off" those corners
    // and turn them into sharp points).  Then extend upward from the corner
    // points to the top of the bounds.
    const float dy = scale + stroke_thickness * 2;
    const float dx = inverse_slope * dy;
    path.rLineTo(-dx, -dy);
    path.rLineTo(0, -button_y - scale + stroke_thickness);
    path.lineTo((width() - 2) * scale + stroke_thickness + dx, 0);
    path.rLineTo(0, bottom);
  } else {
    if (for_fill) {
      path.rCubicTo(0, -0.5 * scale, 0.25 * scale, -scale, scale, -scale);
    } else {
      path.rCubicTo(-0.5 * scale, -1.125 * scale, 0.5 * scale,
                    -scale - 2 * stroke_thickness, scale,
                    -scale - 2 * stroke_thickness);
    }
    path.lineTo((width() - 4) * scale - diag_width + stroke_thickness,
                button_y + scale - stroke_thickness);
    path.rCubicTo(0.75 * scale, 0, 1.625 * scale, 0.5 * scale, 2 * scale,
                  1.5 * scale);
    path.rLineTo(diag_width, diag_height);
    if (for_fill) {
      path.rCubicTo(0, 0.5 * scale, -0.25 * scale, scale, -scale, scale);
    } else {
      path.rCubicTo(0.5 * scale, 1.125 * scale, -0.5 * scale,
                    scale + 2 * stroke_thickness, -scale,
                    scale + 2 * stroke_thickness);
    }
  }
  path.close();

  return path;
}

void NewTabButton::UpdateInkDropBaseColor() {
  DCHECK(MD::IsNewerMaterialUi());

  set_ink_drop_base_color(color_utils::BlendTowardOppositeLuma(
      GetButtonFillColor(), SK_AlphaOPAQUE));
}
