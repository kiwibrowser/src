// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_THUMBNAIL_DECODER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_THUMBNAIL_DECODER_H_

#include <string>
#include "base/callback.h"

namespace gfx {
class Image;
}

namespace offline_pages {

// Decodes thumbnails. Provided so that this component does not need to
// depend on ImageSkiaOperations which isn't available everywhere.
class ThumbnailDecoder {
 public:
  using DecodeComplete = base::OnceCallback<void(const gfx::Image&)>;
  virtual ~ThumbnailDecoder() {}

  // Decode a thumbnail image and crop it square. Calls complete_callback
  // when decoding completes successfully or otherwise. If decoding fails,
  // the returned image is empty.
  virtual void DecodeAndCropThumbnail(const std::string& thumbnail_data,
                                      DecodeComplete complete_callback) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_THUMBNAIL_DECODER_H_
