// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/raster_source.h"

#include <stddef.h>

#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/math_util.h"
#include "cc/base/region.h"
#include "cc/debug/debug_colors.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/image_provider.h"
#include "cc/paint/skia_paint_canvas.h"
#include "components/viz/common/traced_value.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "ui/gfx/geometry/axis_transform2d.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace cc {
namespace {

// These enum values are persisted to logs and must never by renumbered or
// reused.
enum class RasterSourceClearType {
  kNone = 0,
  kFull = 1,
  kBorder = 2,
  kCount = 3
};

void TrackRasterSourceNeededClear(RasterSourceClearType clear_type) {
  UMA_HISTOGRAM_ENUMERATION("Renderer4.RasterSourceClearType", clear_type,
                            RasterSourceClearType::kCount);
}

}  // namespace

RasterSource::RasterSource(const RecordingSource* other)
    : display_list_(other->display_list_),
      painter_reported_memory_usage_(other->painter_reported_memory_usage_),
      background_color_(other->background_color_),
      requires_clear_(other->requires_clear_),
      is_solid_color_(other->is_solid_color_),
      solid_color_(other->solid_color_),
      recorded_viewport_(other->recorded_viewport_),
      size_(other->size_),
      slow_down_raster_scale_factor_for_debug_(
          other->slow_down_raster_scale_factor_for_debug_),
      recording_scale_factor_(other->recording_scale_factor_) {}
RasterSource::~RasterSource() = default;

void RasterSource::ClearForFullRaster(
    SkCanvas* raster_canvas,
    const gfx::Size& content_size,
    const gfx::Rect& canvas_bitmap_rect,
    const gfx::Rect& canvas_playback_rect) const {
  // If this raster source has opaque contents, it is guaranteeing that it
  // will draw an opaque rect the size of the layer.  If it is not, then we
  // must clear this canvas ourselves (i.e. requires_clear_).
  if (requires_clear_) {
    TrackRasterSourceNeededClear(RasterSourceClearType::kFull);
    raster_canvas->clear(SK_ColorTRANSPARENT);
    return;
  }

  // The last texel of this content is not guaranteed to be fully opaque, so
  // inset by one to generate the fully opaque coverage rect .  This rect is
  // in device space.
  SkIRect coverage_device_rect =
      SkIRect::MakeWH(content_size.width() - canvas_bitmap_rect.x() - 1,
                      content_size.height() - canvas_bitmap_rect.y() - 1);

  // Remove playback rect offset, which is equal to bitmap rect offset,
  // as this is full raster.
  SkIRect playback_device_rect = SkIRect::MakeWH(canvas_playback_rect.width(),
                                                 canvas_playback_rect.height());

  // If not fully covered, we need to clear one texel inside the coverage
  // rect (because of blending during raster) and one texel outside the full
  // raster rect (because of bilinear filtering during draw).  See comments
  // in RasterSource.
  SkIRect device_column = SkIRect::MakeXYWH(coverage_device_rect.right(), 0, 2,
                                            coverage_device_rect.bottom());
  // row includes the corner, column excludes it.
  SkIRect device_row = SkIRect::MakeXYWH(0, coverage_device_rect.bottom(),
                                         coverage_device_rect.right() + 2, 2);

  RasterSourceClearType clear_type = RasterSourceClearType::kNone;
  // Only bother clearing if we need to.
  if (SkIRect::Intersects(device_column, playback_device_rect)) {
    clear_type = RasterSourceClearType::kBorder;
    raster_canvas->save();
    raster_canvas->clipRect(SkRect::MakeFromIRect(device_column),
                            SkClipOp::kIntersect, false);
    raster_canvas->drawColor(background_color_, SkBlendMode::kSrc);
    raster_canvas->restore();
  }
  if (SkIRect::Intersects(device_row, playback_device_rect)) {
    clear_type = RasterSourceClearType::kBorder;
    raster_canvas->save();
    raster_canvas->clipRect(SkRect::MakeFromIRect(device_row),
                            SkClipOp::kIntersect, false);
    raster_canvas->drawColor(background_color_, SkBlendMode::kSrc);
    raster_canvas->restore();
  }
  TrackRasterSourceNeededClear(clear_type);
}

void RasterSource::PlaybackToCanvas(
    SkCanvas* input_canvas,
    const gfx::ColorSpace& target_color_space,
    const gfx::Size& content_size,
    const gfx::Rect& canvas_bitmap_rect,
    const gfx::Rect& canvas_playback_rect,
    const gfx::AxisTransform2d& raster_transform,
    const PlaybackSettings& settings) const {
  SkIRect raster_bounds = gfx::RectToSkIRect(canvas_bitmap_rect);
  if (!canvas_playback_rect.IsEmpty() &&
      !raster_bounds.intersect(gfx::RectToSkIRect(canvas_playback_rect)))
    return;
  // Treat all subnormal values as zero for performance.
  ScopedSubnormalFloatDisabler disabler;

  // TODO(enne): color transform needs to be replicated in gles2_cmd_decoder
  SkCanvas* raster_canvas = input_canvas;
  std::unique_ptr<SkCanvas> color_transform_canvas;
  if (target_color_space.IsValid()) {
    color_transform_canvas = SkCreateColorSpaceXformCanvas(
        input_canvas, target_color_space.ToSkColorSpace());
    raster_canvas = color_transform_canvas.get();
  }

  bool is_partial_raster = canvas_bitmap_rect != canvas_playback_rect;
  if (!is_partial_raster && settings.clear_canvas_before_raster) {
    ClearForFullRaster(raster_canvas, content_size, canvas_bitmap_rect,
                       canvas_playback_rect);
  }

  raster_canvas->save();
  raster_canvas->translate(-canvas_bitmap_rect.x(), -canvas_bitmap_rect.y());
  raster_canvas->clipRect(SkRect::MakeFromIRect(raster_bounds));
  raster_canvas->translate(raster_transform.translation().x(),
                           raster_transform.translation().y());
  raster_canvas->scale(raster_transform.scale() / recording_scale_factor_,
                       raster_transform.scale() / recording_scale_factor_);

  if (is_partial_raster && settings.clear_canvas_before_raster &&
      requires_clear_) {
    // TODO(enne): Should this be considered a partial clear?
    TrackRasterSourceNeededClear(RasterSourceClearType::kFull);
    raster_canvas->clear(SK_ColorTRANSPARENT);
  }

  PlaybackToCanvas(raster_canvas, settings.image_provider);
  raster_canvas->restore();
}

void RasterSource::PlaybackToCanvas(SkCanvas* raster_canvas,
                                    ImageProvider* image_provider) const {
  // TODO(enne): Temporary CHECK debugging for http://crbug.com/823835
  CHECK(display_list_.get());
  int repeat_count = std::max(1, slow_down_raster_scale_factor_for_debug_);
  for (int i = 0; i < repeat_count; ++i)
    display_list_->Raster(raster_canvas, image_provider);
}

sk_sp<SkPicture> RasterSource::GetFlattenedPicture() {
  TRACE_EVENT0("cc", "RasterSource::GetFlattenedPicture");

  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(size_.width(), size_.height());
  if (!size_.IsEmpty()) {
    canvas->clear(SK_ColorTRANSPARENT);
    PlaybackToCanvas(canvas, nullptr);
  }

  return recorder.finishRecordingAsPicture();
}

size_t RasterSource::GetMemoryUsage() const {
  if (!display_list_)
    return 0;
  return display_list_->BytesUsed() + painter_reported_memory_usage_;
}

bool RasterSource::PerformSolidColorAnalysis(gfx::Rect layer_rect,
                                             SkColor* color) const {
  TRACE_EVENT0("cc", "RasterSource::PerformSolidColorAnalysis");

  layer_rect.Intersect(gfx::Rect(size_));
  layer_rect = gfx::ScaleToRoundedRect(layer_rect, recording_scale_factor_);
  return display_list_->GetColorIfSolidInRect(layer_rect, color);
}

void RasterSource::GetDiscardableImagesInRect(
    const gfx::Rect& layer_rect,
    std::vector<const DrawImage*>* images) const {
  DCHECK_EQ(0u, images->size());
  display_list_->discardable_image_map().GetDiscardableImagesInRect(layer_rect,
                                                                    images);
}

base::flat_map<PaintImage::Id, PaintImage::DecodingMode>
RasterSource::TakeDecodingModeMap() {
  return display_list_->TakeDecodingModeMap();
}

bool RasterSource::CoversRect(const gfx::Rect& layer_rect) const {
  if (size_.IsEmpty())
    return false;
  gfx::Rect bounded_rect = layer_rect;
  bounded_rect.Intersect(gfx::Rect(size_));
  return recorded_viewport_.Contains(bounded_rect);
}

gfx::Size RasterSource::GetSize() const {
  return size_;
}

gfx::Size RasterSource::GetContentSize(float content_scale) const {
  return gfx::ScaleToCeiledSize(GetSize(), content_scale);
}

bool RasterSource::IsSolidColor() const {
  return is_solid_color_;
}

SkColor RasterSource::GetSolidColor() const {
  DCHECK(IsSolidColor());
  return solid_color_;
}

bool RasterSource::HasRecordings() const {
  return !!display_list_.get();
}

gfx::Rect RasterSource::RecordedViewport() const {
  return recorded_viewport_;
}

void RasterSource::AsValueInto(base::trace_event::TracedValue* array) const {
  if (display_list_.get())
    viz::TracedValue::AppendIDRef(display_list_.get(), array);
}

void RasterSource::DidBeginTracing() {
  if (display_list_.get())
    display_list_->EmitTraceSnapshot();
}

RasterSource::PlaybackSettings::PlaybackSettings() = default;

RasterSource::PlaybackSettings::PlaybackSettings(const PlaybackSettings&) =
    default;

RasterSource::PlaybackSettings::PlaybackSettings(PlaybackSettings&&) = default;

RasterSource::PlaybackSettings::~PlaybackSettings() = default;

}  // namespace cc
