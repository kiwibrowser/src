// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_GPU_JPEG_DECODE_ACCELERATOR_FACTORY_H_
#define MEDIA_GPU_GPU_JPEG_DECODE_ACCELERATOR_FACTORY_H_

#include "base/memory/ref_counted.h"
#include "media/gpu/media_gpu_export.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class MEDIA_GPU_EXPORT GpuJpegDecodeAcceleratorFactory {
 public:
  using CreateAcceleratorCB =
      base::Callback<std::unique_ptr<JpegDecodeAccelerator>(
          scoped_refptr<base::SingleThreadTaskRunner>)>;

  // Static query for JPEG supported. This query calls the appropriate
  // platform-specific version.
  static bool IsAcceleratedJpegDecodeSupported();

  static std::vector<CreateAcceleratorCB> GetAcceleratorFactories();
};

}  // namespace media

#endif  // MEDIA_GPU_GPU_JPEG_DECODE_ACCELERATOR_FACTORY_H_
