// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_GPU_JPEG_ENCODE_ACCELERATOR_FACTORY_H_
#define MEDIA_GPU_GPU_JPEG_ENCODE_ACCELERATOR_FACTORY_H_

#include "base/memory/ref_counted.h"
#include "media/gpu/media_gpu_export.h"
#include "media/video/jpeg_encode_accelerator.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class MEDIA_GPU_EXPORT GpuJpegEncodeAcceleratorFactory {
 public:
  using CreateAcceleratorCB =
      base::RepeatingCallback<std::unique_ptr<JpegEncodeAccelerator>(
          scoped_refptr<base::SingleThreadTaskRunner>)>;

  // Static query for JPEG supported. This query calls the appropriate
  // platform-specific version.
  static bool IsAcceleratedJpegEncodeSupported();

  static std::vector<CreateAcceleratorCB> GetAcceleratorFactories();
};

}  // namespace media

#endif  // MEDIA_GPU_GPU_JPEG_ENCODE_ACCELERATOR_FACTORY_H_
