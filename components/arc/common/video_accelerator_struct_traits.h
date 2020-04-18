// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_COMMON_VIDEO_ACCELERATOR_STRUCT_TRAITS_H_
#define COMPONENTS_ARC_COMMON_VIDEO_ACCELERATOR_STRUCT_TRAITS_H_

#include "components/arc/common/video_common.mojom.h"
#include "components/arc/video_accelerator/video_frame_plane.h"
#include "media/base/video_codecs.h"
#include "ui/gfx/geometry/size.h"

namespace mojo {

template <>
struct EnumTraits<arc::mojom::VideoCodecProfile, media::VideoCodecProfile> {
  static arc::mojom::VideoCodecProfile ToMojom(media::VideoCodecProfile input);

  static bool FromMojom(arc::mojom::VideoCodecProfile input,
                        media::VideoCodecProfile* output);
};

template <>
struct StructTraits<arc::mojom::VideoFramePlaneDataView, arc::VideoFramePlane> {
  static int32_t offset(const arc::VideoFramePlane& r) {
    DCHECK_GE(r.offset, 0);
    return r.offset;
  }

  static int32_t stride(const arc::VideoFramePlane& r) {
    DCHECK_GE(r.stride, 0);
    return r.stride;
  }

  static bool Read(arc::mojom::VideoFramePlaneDataView data,
                   arc::VideoFramePlane* out);
};

template <>
struct StructTraits<arc::mojom::SizeDataView, gfx::Size> {
  static int width(const gfx::Size& r) {
    DCHECK_GE(r.width(), 0);
    return r.width();
  }

  static int height(const gfx::Size& r) {
    DCHECK_GE(r.height(), 0);
    return r.height();
  }

  static bool Read(arc::mojom::SizeDataView data, gfx::Size* out);
};
}  // namespace mojo

#endif  // COMPONENTS_ARC_COMMON_VIDEO_ACCELERATOR_STRUCT_TRAITS_H_
