// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RASTER_TEXTURE_COMPRESSOR_H_
#define CC_RASTER_TEXTURE_COMPRESSOR_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "cc/cc_export.h"

namespace cc {

class CC_EXPORT TextureCompressor {
 public:
  enum Format {
    kFormatETC1,
  };

  enum Quality {
    kQualityLow,
    kQualityMedium,
    kQualityHigh,
  };

  static std::unique_ptr<TextureCompressor> Create(Format format);
  virtual ~TextureCompressor() {}

  virtual void Compress(const uint8_t* src,
                        uint8_t* dst,
                        int width,
                        int height,
                        Quality quality) = 0;

 protected:
  TextureCompressor() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TextureCompressor);
};

}  // namespace cc

#endif  // CC_RASTER_TEXTURE_COMPRESSOR_H_
