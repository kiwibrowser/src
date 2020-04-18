// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/bubble/bubble_border.h"

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/no_destructor.h"
#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkDrawLooper.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/path.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/painter.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/view.h"

namespace views {

namespace internal {

BorderImages::BorderImages(const int border_image_ids[],
                           const int arrow_image_ids[],
                           int border_interior_thickness,
                           int arrow_interior_thickness,
                           int corner_radius)
    : border_thickness(border_interior_thickness),
      border_interior_thickness(border_interior_thickness),
      arrow_thickness(arrow_interior_thickness),
      arrow_interior_thickness(arrow_interior_thickness),
      arrow_width(2 * arrow_interior_thickness),
      corner_radius(corner_radius) {
  if (!border_image_ids)
    return;

  border_painter = Painter::CreateImageGridPainter(border_image_ids);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  border_thickness = rb.GetImageSkiaNamed(border_image_ids[0])->width();

  if (arrow_image_ids[0] != 0) {
    left_arrow = *rb.GetImageSkiaNamed(arrow_image_ids[0]);
    top_arrow = *rb.GetImageSkiaNamed(arrow_image_ids[1]);
    right_arrow = *rb.GetImageSkiaNamed(arrow_image_ids[2]);
    bottom_arrow = *rb.GetImageSkiaNamed(arrow_image_ids[3]);
    arrow_width = top_arrow.width();
    arrow_thickness = top_arrow.height();
  }
}

BorderImages::~BorderImages() {}

}  // namespace internal

namespace {

// The border corner radius for material design bubble borders.
constexpr int kMaterialDesignCornerRadius = 2;

// The border is stroked at 1px, but for the purposes of reserving space we have
// to deal in dip coordinates, so round up to 1dip.
constexpr int kBorderThicknessDip = 1;

// Utility functions for getting alignment points on the edge of a rectangle.
gfx::Point CenterTop(const gfx::Rect& rect) {
  return gfx::Point(rect.CenterPoint().x(), rect.y());
}

gfx::Point CenterBottom(const gfx::Rect& rect) {
  return gfx::Point(rect.CenterPoint().x(), rect.bottom());
}

gfx::Point LeftCenter(const gfx::Rect& rect) {
  return gfx::Point(rect.x(), rect.CenterPoint().y());
}

gfx::Point RightCenter(const gfx::Rect& rect) {
  return gfx::Point(rect.right(), rect.CenterPoint().y());
}

// Bubble border and arrow image resource ids. They don't use the IMAGE_GRID
// macro because there is no center image.
const int kNoShadowImages[] = {
    IDR_BUBBLE_TL, IDR_BUBBLE_T, IDR_BUBBLE_TR,
    IDR_BUBBLE_L,  0,            IDR_BUBBLE_R,
    IDR_BUBBLE_BL, IDR_BUBBLE_B, IDR_BUBBLE_BR };
const int kNoShadowArrows[] = {
    IDR_BUBBLE_L_ARROW, IDR_BUBBLE_T_ARROW,
    IDR_BUBBLE_R_ARROW, IDR_BUBBLE_B_ARROW, };

const int kBigShadowImages[] = {
    IDR_WINDOW_BUBBLE_SHADOW_BIG_TOP_LEFT,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_TOP,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_TOP_RIGHT,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_LEFT,
    0,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_RIGHT,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_BOTTOM_LEFT,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_BOTTOM,
    IDR_WINDOW_BUBBLE_SHADOW_BIG_BOTTOM_RIGHT };
const int kBigShadowArrows[] = {
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_BIG_LEFT,
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_BIG_TOP,
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_BIG_RIGHT,
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_BIG_BOTTOM };

const int kSmallShadowImages[] = {
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_TOP_LEFT,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_TOP,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_TOP_RIGHT,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_LEFT,
    0,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_RIGHT,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_BOTTOM_LEFT,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_BOTTOM,
    IDR_WINDOW_BUBBLE_SHADOW_SMALL_BOTTOM_RIGHT };
const int kSmallShadowArrows[] = {
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_SMALL_LEFT,
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_SMALL_TOP,
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_SMALL_RIGHT,
    IDR_WINDOW_BUBBLE_SHADOW_SPIKE_SMALL_BOTTOM };

using internal::BorderImages;

// Returns the cached BorderImages for the given |shadow| type.
BorderImages* GetBorderImages(BubbleBorder::Shadow shadow) {
  // Keep a cache of bubble border image-set painters, arrows, and metrics.
  static BorderImages* kBorderImages[BubbleBorder::SHADOW_COUNT] = { NULL };

  CHECK_LT(shadow, BubbleBorder::SHADOW_COUNT);
  struct BorderImages*& set = kBorderImages[shadow];
  if (set)
    return set;

  switch (shadow) {
    case BubbleBorder::NO_SHADOW:
    case BubbleBorder::NO_SHADOW_OPAQUE_BORDER:
      set = new BorderImages(kNoShadowImages, kNoShadowArrows, 6, 7, 4);
      break;
    case BubbleBorder::BIG_SHADOW:
      set = new BorderImages(kBigShadowImages, kBigShadowArrows, 23, 9, 2);
      break;
    case BubbleBorder::SMALL_SHADOW:
      set = new BorderImages(kSmallShadowImages, kSmallShadowArrows, 5, 6, 2);
      break;
    case BubbleBorder::NO_ASSETS:
      set = new BorderImages(nullptr, nullptr, 17, 8, 2);
      break;
    case BubbleBorder::SHADOW_COUNT:
      NOTREACHED();
      break;
  }

  return set;
}

}  // namespace

const int BubbleBorder::kStroke = 1;

BubbleBorder::BubbleBorder(Arrow arrow, Shadow shadow, SkColor color)
    : arrow_(arrow),
      arrow_offset_(0),
      arrow_paint_type_(PAINT_NORMAL),
      alignment_(ALIGN_ARROW_TO_MID_ANCHOR),
      shadow_(shadow),
      images_(nullptr),
      background_color_(color),
      use_theme_background_color_(false) {
  DCHECK(shadow_ < SHADOW_COUNT);
  Init();
}

BubbleBorder::~BubbleBorder() {}

// static
gfx::Insets BubbleBorder::GetBorderAndShadowInsets(
    base::Optional<int> elevation) {
  // Borders with custom shadow elevations do not draw the 1px border.
  if (elevation.has_value())
    return -gfx::ShadowValue::GetMargin(GetShadowValues(elevation));

  constexpr gfx::Insets blur(kShadowBlur + kBorderThicknessDip);
  constexpr gfx::Insets offset(-kShadowVerticalOffset, 0, kShadowVerticalOffset,
                               0);
  return blur + offset;
}

void BubbleBorder::SetCornerRadius(int corner_radius) {
  corner_radius_ = corner_radius;
  Init();
}

void BubbleBorder::set_paint_arrow(ArrowPaintType value) {
  if (UseMaterialDesign())
    return;
  arrow_paint_type_ = value;
}

gfx::Rect BubbleBorder::GetBounds(const gfx::Rect& anchor_rect,
                                  const gfx::Size& contents_size) const {
  // In MD, there are no arrows, so positioning logic is significantly simpler.
  // TODO(estade): handle more anchor positions.
  if (UseMaterialDesign() &&
      (arrow_ == TOP_RIGHT || arrow_ == TOP_LEFT || arrow_ == BOTTOM_CENTER ||
       arrow_ == TOP_CENTER || arrow_ == LEFT_CENTER ||
       arrow_ == RIGHT_CENTER)) {
    gfx::Rect contents_bounds(contents_size);
    // Apply the border part of the inset before calculating coordinates because
    // the border should align with the anchor's border. For the purposes of
    // positioning, the border is rounded up to a dip, which may mean we have
    // misalignment in scale factors greater than 1. Borders with custom shadow
    // elevations do not draw the 1px border.
    // TODO(estade): when it becomes possible to provide px bounds instead of
    // dip bounds, fix this.
    const gfx::Insets border_insets =
        shadow_ == NO_ASSETS || md_shadow_elevation_.has_value()
            ? gfx::Insets()
            : gfx::Insets(kBorderThicknessDip);
    const gfx::Insets shadow_insets = GetInsets() - border_insets;
    contents_bounds.Inset(-border_insets);
    if (arrow_ == TOP_RIGHT) {
      contents_bounds +=
          anchor_rect.bottom_right() - contents_bounds.top_right();
    } else if (arrow_ == TOP_LEFT) {
      contents_bounds +=
          anchor_rect.bottom_left() - contents_bounds.origin();
    } else if (arrow_ == BOTTOM_CENTER) {
      contents_bounds += CenterTop(anchor_rect) - CenterBottom(contents_bounds);
    } else if (arrow_ == TOP_CENTER) {
      contents_bounds += CenterBottom(anchor_rect) - CenterTop(contents_bounds);
    } else if (arrow_ == LEFT_CENTER) {
      contents_bounds += RightCenter(anchor_rect) - LeftCenter(contents_bounds);
    } else if (arrow_ == RIGHT_CENTER) {
      contents_bounds += LeftCenter(anchor_rect) - RightCenter(contents_bounds);
    }
    // With NO_ASSETS, there should be further insets, but the same logic is
    // used to position the bubble origin according to |anchor_rect|.
    DCHECK(shadow_ != NO_ASSETS || shadow_insets.IsEmpty());
    contents_bounds.Inset(-shadow_insets);
    // |arrow_offset_| is used to adjust bubbles that would normally be
    // partially offscreen.
    contents_bounds += gfx::Vector2d(-arrow_offset_, 0);
    return contents_bounds;
  }

  int x = anchor_rect.x();
  int y = anchor_rect.y();
  int w = anchor_rect.width();
  int h = anchor_rect.height();
  const gfx::Size size(GetSizeForContentsSize(contents_size));
  const int arrow_offset = GetArrowOffset(size);
  const int stroke_width = shadow_ == NO_ASSETS ? 0 : kStroke;
  // |arrow_shift| is necessary to visually align the tip of the bubble arrow
  // with the anchor point. This shift is an inverse of the shadow thickness.
  int arrow_shift = UseMaterialDesign()
                        ? 0
                        : images_->arrow_interior_thickness + stroke_width -
                              images_->arrow_thickness;
  // When arrow is painted transparently the visible border of the bubble needs
  // to be positioned at the same bounds as when the arrow is shown.
  if (arrow_paint_type_ == PAINT_TRANSPARENT)
    arrow_shift += images_->arrow_interior_thickness;
  const bool mid_anchor = alignment_ == ALIGN_ARROW_TO_MID_ANCHOR;

  // Calculate the bubble coordinates based on the border and arrow settings.
  if (is_arrow_on_horizontal(arrow_)) {
    if (is_arrow_on_left(arrow_)) {
      x += mid_anchor ? w / 2 - arrow_offset
                      : stroke_width - GetBorderThickness();
    } else if (is_arrow_at_center(arrow_)) {
      x += w / 2 - arrow_offset;
    } else {
      x += mid_anchor ? w / 2 + arrow_offset - size.width()
                      : w - size.width() + GetBorderThickness() - stroke_width;
    }
    y += is_arrow_on_top(arrow_) ? h + arrow_shift
                                 : -arrow_shift - size.height();
  } else if (has_arrow(arrow_)) {
    x += is_arrow_on_left(arrow_) ? w + arrow_shift
                                  : -arrow_shift - size.width();
    if (is_arrow_on_top(arrow_)) {
      y += mid_anchor ? h / 2 - arrow_offset
                      : stroke_width - GetBorderThickness();
    } else if (is_arrow_at_center(arrow_)) {
      y += h / 2 - arrow_offset;
    } else {
      y += mid_anchor ? h / 2 + arrow_offset - size.height()
                      : h - size.height() + GetBorderThickness() - stroke_width;
    }
  } else {
    x += (w - size.width()) / 2;
    y += (arrow_ == NONE) ? h : (h - size.height()) / 2;
  }

  return gfx::Rect(x, y, size.width(), size.height());
}

int BubbleBorder::GetBorderThickness() const {
  // TODO(estade): this shouldn't be called in MD.
  return UseMaterialDesign()
             ? 0
             : images_->border_thickness - images_->border_interior_thickness;
}

int BubbleBorder::GetBorderCornerRadius() const {
  if (UseMaterialDesign())
    return corner_radius_.value_or(kMaterialDesignCornerRadius);
  return images_->corner_radius;
}

int BubbleBorder::GetArrowOffset(const gfx::Size& border_size) const {
  if (UseMaterialDesign())
    return 0;

  const int edge_length = is_arrow_on_horizontal(arrow_) ?
      border_size.width() : border_size.height();
  if (is_arrow_at_center(arrow_) && arrow_offset_ == 0)
    return edge_length / 2;

  // Calculate the minimum offset to not overlap arrow and corner images.
  const int min = images_->border_thickness + (images_->arrow_width / 2);
  // Ensure the returned value will not cause image overlap, if possible.
  return std::max(min, std::min(arrow_offset_, edge_length - min));
}

bool BubbleBorder::GetArrowPath(const gfx::Rect& view_bounds,
                                gfx::Path* path) const {
  if (!has_arrow(arrow_) || arrow_paint_type_ != PAINT_NORMAL)
    return false;

  GetArrowPathFromArrowBounds(GetArrowRect(view_bounds), path);
  return true;
}

void BubbleBorder::SetBorderInteriorThickness(int border_interior_thickness) {
  // TODO(estade): remove this function.
  DCHECK(!UseMaterialDesign());
  images_->border_interior_thickness = border_interior_thickness;
  if (!has_arrow(arrow_) || arrow_paint_type_ != PAINT_NORMAL)
    images_->border_thickness = border_interior_thickness;
}

void BubbleBorder::Init() {
  if (UseMaterialDesign()) {
    // Harmony bubbles don't use arrows.
    alignment_ = ALIGN_EDGE_TO_ANCHOR_EDGE;
    arrow_paint_type_ = PAINT_NONE;
  } else {
    images_ = GetBorderImages(shadow_);
  }
}

void BubbleBorder::Paint(const views::View& view, gfx::Canvas* canvas) {
  if (UseMaterialDesign())
    return PaintMd(view, canvas);

  gfx::Rect bounds(view.GetContentsBounds());
  bounds.Inset(-GetBorderThickness(), -GetBorderThickness());
  const gfx::Rect arrow_bounds = GetArrowRect(view.GetLocalBounds());
  if (arrow_bounds.IsEmpty()) {
    if (images_->border_painter)
      Painter::PaintPainterAt(canvas, images_->border_painter.get(), bounds);
    return;
  }
  if (!images_->border_painter) {
    DrawArrow(canvas, arrow_bounds);
    return;
  }

  // Clip the arrow bounds out to avoid painting the overlapping edge area.
  canvas->Save();
  canvas->ClipRect(arrow_bounds, SkClipOp::kDifference);
  Painter::PaintPainterAt(canvas, images_->border_painter.get(), bounds);
  canvas->Restore();

  DrawArrow(canvas, arrow_bounds);
}

gfx::Insets BubbleBorder::GetInsets() const {
  if (UseMaterialDesign()) {
    return (shadow_ == NO_ASSETS)
               ? gfx::Insets()
               : GetBorderAndShadowInsets(md_shadow_elevation_);
  }

  // The insets contain the stroke and shadow pixels outside the bubble fill.
  const int inset = GetBorderThickness();
  if (arrow_paint_type_ != PAINT_NORMAL || !has_arrow(arrow_))
    return gfx::Insets(inset);

  int first_inset = inset;
  int second_inset = std::max(inset, images_->arrow_thickness);
  if (is_arrow_on_horizontal(arrow_) ?
      is_arrow_on_top(arrow_) : is_arrow_on_left(arrow_))
    std::swap(first_inset, second_inset);
  return is_arrow_on_horizontal(arrow_) ?
      gfx::Insets(first_inset, inset, second_inset, inset) :
      gfx::Insets(inset, first_inset, inset, second_inset);
}

gfx::Size BubbleBorder::GetMinimumSize() const {
  return GetSizeForContentsSize(gfx::Size());
}

// static
const gfx::ShadowValues& BubbleBorder::GetShadowValues(
    base::Optional<int> elevation) {
  // The shadows are always the same for any elevation, so construct them once
  // and cache.
  static base::NoDestructor<std::map<int, gfx::ShadowValues>> shadow_map;
  if (shadow_map->find(elevation.value_or(-1)) != shadow_map->end())
    return shadow_map->find(elevation.value_or(-1))->second;

  gfx::ShadowValues shadows;
  if (elevation.has_value()) {
    DCHECK(elevation.value() >= 0);
    shadows = gfx::ShadowValues(
        gfx::ShadowValue::MakeMdShadowValues(elevation.value()));
  } else {
    constexpr int kSmallShadowVerticalOffset = 2;
    constexpr int kSmallShadowBlur = 4;
    constexpr SkColor kSmallShadowColor = SkColorSetA(SK_ColorBLACK, 0x33);
    constexpr SkColor kLargeShadowColor = SkColorSetA(SK_ColorBLACK, 0x1A);
    // gfx::ShadowValue counts blur pixels both inside and outside the shape,
    // whereas these blur values only describe the outside portion, hence they
    // must be doubled.
    shadows = gfx::ShadowValues({
        {gfx::Vector2d(0, kSmallShadowVerticalOffset), 2 * kSmallShadowBlur,
         kSmallShadowColor},
        {gfx::Vector2d(0, kShadowVerticalOffset), 2 * kShadowBlur,
         kLargeShadowColor},
    });
  }

  shadow_map->insert(
      std::pair<int, gfx::ShadowValues>(elevation.value_or(-1), shadows));
  return shadow_map->find(elevation.value_or(-1))->second;
}

// static
const cc::PaintFlags& BubbleBorder::GetBorderAndShadowFlags(
    base::Optional<int> elevation) {
  // The flags are always the same for any elevation, so construct them once and
  // cache.
  static base::NoDestructor<std::map<int, cc::PaintFlags>> flag_map;

  if (flag_map->find(elevation.value_or(-1)) != flag_map->end())
    return flag_map->find(elevation.value_or(-1))->second;

  cc::PaintFlags flags;
  constexpr SkColor kBorderColor = SkColorSetA(SK_ColorBLACK, 0x26);
  flags.setColor(kBorderColor);
  flags.setAntiAlias(true);
  flags.setLooper(gfx::CreateShadowDrawLooper(GetShadowValues(elevation)));
  flag_map->insert(
      std::pair<int, cc::PaintFlags>(elevation.value_or(-1), flags));

  return flag_map->find(elevation.value_or(-1))->second;
}

gfx::Size BubbleBorder::GetSizeForContentsSize(
    const gfx::Size& contents_size) const {
  // Enlarge the contents size by the thickness of the border images.
  gfx::Size size(contents_size);
  const gfx::Insets insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  if (UseMaterialDesign())
    return size;

  // Ensure the bubble is large enough to not overlap border and arrow images.
  const int min = 2 * images_->border_thickness;
  // Only take arrow image sizes into account when the bubble tip is shown.
  if (arrow_paint_type_ != PAINT_NORMAL || !has_arrow(arrow_)) {
    size.SetToMax(gfx::Size(min, min));
    return size;
  }
  const int min_with_arrow_width = min + images_->arrow_width;
  const int min_with_arrow_thickness = images_->border_thickness +
      std::max(images_->arrow_thickness + images_->border_interior_thickness,
               images_->border_thickness);
  if (is_arrow_on_horizontal(arrow_))
    size.SetToMax(gfx::Size(min_with_arrow_width, min_with_arrow_thickness));
  else
    size.SetToMax(gfx::Size(min_with_arrow_thickness, min_with_arrow_width));
  return size;
}

gfx::ImageSkia* BubbleBorder::GetArrowImage() const {
  if (!has_arrow(arrow_))
    return NULL;
  if (is_arrow_on_horizontal(arrow_)) {
    return is_arrow_on_top(arrow_) ?
        &images_->top_arrow : &images_->bottom_arrow;
  }
  return is_arrow_on_left(arrow_) ?
      &images_->left_arrow : &images_->right_arrow;
}

gfx::Rect BubbleBorder::GetArrowRect(const gfx::Rect& bounds) const {
  if (!has_arrow(arrow_) || arrow_paint_type_ != PAINT_NORMAL)
    return gfx::Rect();

  gfx::Point origin;
  int offset = GetArrowOffset(bounds.size());
  const int half_length = images_->arrow_width / 2;
  const gfx::Insets insets = GetInsets();

  if (is_arrow_on_horizontal(arrow_)) {
    origin.set_x(is_arrow_on_left(arrow_) || is_arrow_at_center(arrow_) ?
        offset : bounds.width() - offset);
    origin.Offset(-half_length, 0);
    if (is_arrow_on_top(arrow_))
      origin.set_y(insets.top() - images_->arrow_thickness);
    else
      origin.set_y(bounds.height() - insets.bottom());
  } else {
    origin.set_y(is_arrow_on_top(arrow_)  || is_arrow_at_center(arrow_) ?
        offset : bounds.height() - offset);
    origin.Offset(0, -half_length);
    if (is_arrow_on_left(arrow_))
      origin.set_x(insets.left() - images_->arrow_thickness);
    else
      origin.set_x(bounds.width() - insets.right());
  }

  if (shadow_ != NO_ASSETS)
    return gfx::Rect(origin, GetArrowImage()->size());

  // With no assets, return the size enclosing the path filled in DrawArrow().
  DCHECK_EQ(2 * images_->arrow_interior_thickness, images_->arrow_width);
  int width = images_->arrow_width;
  int height = images_->arrow_interior_thickness;
  if (!is_arrow_on_horizontal(arrow_))
    std::swap(width, height);
  return gfx::Rect(origin, gfx::Size(width, height));
}

void BubbleBorder::GetArrowPathFromArrowBounds(const gfx::Rect& arrow_bounds,
                                               SkPath* path) const {
  DCHECK(!UseMaterialDesign());
  const bool horizontal = is_arrow_on_horizontal(arrow_);
  const int thickness = images_->arrow_interior_thickness;
  float tip_x = horizontal ? arrow_bounds.CenterPoint().x() :
      is_arrow_on_left(arrow_) ? arrow_bounds.right() - thickness :
                                 arrow_bounds.x() + thickness;
  float tip_y = !horizontal ? arrow_bounds.CenterPoint().y() + 0.5f :
      is_arrow_on_top(arrow_) ? arrow_bounds.bottom() - thickness :
                                arrow_bounds.y() + thickness;
  const bool positive_offset = horizontal ?
      is_arrow_on_top(arrow_) : is_arrow_on_left(arrow_);
  const int offset_to_next_vertex = positive_offset ?
      images_->arrow_interior_thickness : -images_->arrow_interior_thickness;

  path->incReserve(4);
  path->moveTo(SkDoubleToScalar(tip_x), SkDoubleToScalar(tip_y));
  path->lineTo(SkDoubleToScalar(tip_x + offset_to_next_vertex),
               SkDoubleToScalar(tip_y + offset_to_next_vertex));
  const int multiplier = horizontal ? 1 : -1;
  path->lineTo(SkDoubleToScalar(tip_x - multiplier * offset_to_next_vertex),
               SkDoubleToScalar(tip_y + multiplier * offset_to_next_vertex));
  path->close();
}

void BubbleBorder::DrawArrow(gfx::Canvas* canvas,
                             const gfx::Rect& arrow_bounds) const {
  DCHECK(!UseMaterialDesign());
  canvas->DrawImageInt(*GetArrowImage(), arrow_bounds.x(), arrow_bounds.y());
  SkPath path;
  GetArrowPathFromArrowBounds(arrow_bounds, &path);
  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(background_color_);

  canvas->DrawPath(path, flags);
}

SkRRect BubbleBorder::GetClientRect(const View& view) const {
  gfx::RectF bounds(view.GetLocalBounds());
  bounds.Inset(GetInsets());
  return SkRRect::MakeRectXY(gfx::RectFToSkRect(bounds),
                             GetBorderCornerRadius(), GetBorderCornerRadius());
}

void BubbleBorder::PaintMd(const View& view, gfx::Canvas* canvas) {
  if (shadow_ == NO_ASSETS)
    return PaintNoAssets(view, canvas);

  gfx::ScopedCanvas scoped(canvas);

  SkRRect r_rect = GetClientRect(view);
  canvas->sk_canvas()->clipRRect(r_rect, SkClipOp::kDifference,
                                 true /*doAntiAlias*/);

  DrawBorderAndShadow(std::move(r_rect), &cc::PaintCanvas::drawRRect, canvas,
                      md_shadow_elevation_);
}

void BubbleBorder::PaintNoAssets(const View& view, gfx::Canvas* canvas) {
  gfx::ScopedCanvas scoped(canvas);
  canvas->sk_canvas()->clipRRect(GetClientRect(view), SkClipOp::kDifference,
                                 true /*doAntiAlias*/);
  canvas->sk_canvas()->drawColor(SK_ColorTRANSPARENT, SkBlendMode::kSrc);
}

internal::BorderImages* BubbleBorder::GetImagesForTest() const {
  return images_;
}

bool BubbleBorder::UseMaterialDesign() const {
  return ui::MaterialDesignController::IsSecondaryUiMaterial() ||
         corner_radius_.has_value();
}

void BubbleBackground::Paint(gfx::Canvas* canvas, views::View* view) const {
  if (border_->shadow() == BubbleBorder::NO_SHADOW_OPAQUE_BORDER)
    canvas->DrawColor(border_->background_color());

  // Fill the contents with a round-rect region to match the border images.
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(border_->background_color());
  SkPath path;
  gfx::RectF bounds(view->GetLocalBounds());
  bounds.Inset(gfx::InsetsF(border_->GetInsets()));

  canvas->DrawRoundRect(bounds, border_->GetBorderCornerRadius(), flags);
}

}  // namespace views
