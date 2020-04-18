// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/devtools_instrumentation.h"

namespace cc {
namespace devtools_instrumentation {

namespace internal {
const char kCategory[] = TRACE_DISABLED_BY_DEFAULT("devtools.timeline");
const char kCategoryFrame[] =
    TRACE_DISABLED_BY_DEFAULT("devtools.timeline.frame");
const char kData[] = "data";
const char kFrameId[] = "frameId";
const char kLayerId[] = "layerId";
const char kLayerTreeId[] = "layerTreeId";
const char kPixelRefId[] = "pixelRefId";

const char kImageDecodeTask[] = "ImageDecodeTask";
const char kBeginFrame[] = "BeginFrame";
const char kNeedsBeginFrameChanged[] = "NeedsBeginFrameChanged";
const char kActivateLayerTree[] = "ActivateLayerTree";
const char kRequestMainThreadFrame[] = "RequestMainThreadFrame";
const char kBeginMainThreadFrame[] = "BeginMainThreadFrame";
const char kDrawFrame[] = "DrawFrame";
const char kCompositeLayers[] = "CompositeLayers";
}  // namespace internal

const char kPaintSetup[] = "PaintSetup";
const char kUpdateLayer[] = "UpdateLayer";

ScopedImageDecodeTask::ScopedImageDecodeTask(const void* image_ptr,
                                             DecodeType decode_type,
                                             TaskType task_type)
    : decode_type_(decode_type),
      task_type_(task_type),
      start_time_(base::TimeTicks::Now()) {
  TRACE_EVENT_BEGIN1(internal::kCategory, internal::kImageDecodeTask,
                     internal::kPixelRefId,
                     reinterpret_cast<uint64_t>(image_ptr));
}

ScopedImageDecodeTask::~ScopedImageDecodeTask() {
  TRACE_EVENT_END0(internal::kCategory, internal::kImageDecodeTask);
  base::TimeDelta duration = base::TimeTicks::Now() - start_time_;
  switch (task_type_) {
    case kInRaster:
      switch (decode_type_) {
        case kSoftware:
          UMA_HISTOGRAM_COUNTS_1M(
              "Renderer4.ImageDecodeTaskDurationUs.Software",
              duration.InMicroseconds());
          break;
        case kGpu:
          UMA_HISTOGRAM_COUNTS_1M("Renderer4.ImageDecodeTaskDurationUs.Gpu",
                                  duration.InMicroseconds());
          break;
      }
      break;
    case kOutOfRaster:
      switch (decode_type_) {
        case kSoftware:
          UMA_HISTOGRAM_COUNTS_1M(
              "Renderer4.ImageDecodeTaskDurationUs.OutOfRaster.Software",
              duration.InMicroseconds());
          break;
        case kGpu:
          UMA_HISTOGRAM_COUNTS_1M(
              "Renderer4.ImageDecodeTaskDurationUs.OutOfRaster.Gpu",
              duration.InMicroseconds());
          break;
      }
      break;
  }
}

}  // namespace devtools_instrumentation
}  // namespace cc
