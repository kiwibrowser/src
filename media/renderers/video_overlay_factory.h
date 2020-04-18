// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERERS_VIDEO_OVERLAY_FACTORY_H_
#define MEDIA_RENDERERS_VIDEO_OVERLAY_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/media_export.h"

namespace gfx {
class Size;
}  // namespace gfx

namespace media {

class GpuVideoAcceleratorFactories;
class VideoFrame;

// Creates video overlay frames - native textures that get turned into
// transparent holes in the browser compositor using overlay system.
// This class must be used on GpuVideoAcceleratorFactories::GetTaskRunner().
class MEDIA_EXPORT VideoOverlayFactory {
 public:
  explicit VideoOverlayFactory(
      ::media::GpuVideoAcceleratorFactories* gpu_factories);
  ~VideoOverlayFactory();

  scoped_refptr<::media::VideoFrame> CreateFrame(const gfx::Size& size);

 private:
  class Texture;
  Texture* GetTexture();

  ::media::GpuVideoAcceleratorFactories* gpu_factories_;
  std::unique_ptr<Texture> texture_;

  DISALLOW_COPY_AND_ASSIGN(VideoOverlayFactory);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_VIDEO_OVERLAY_FACTORY_H_
