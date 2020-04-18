// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_THUMBNAIL_DECODER_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_THUMBNAIL_DECODER_IMPL_H_

#include <memory>

#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "components/offline_pages/core/thumbnail_decoder.h"

namespace offline_pages {

class ThumbnailDecoderImpl : public ThumbnailDecoder {
 public:
  explicit ThumbnailDecoderImpl(
      std::unique_ptr<image_fetcher::ImageDecoder> decoder);
  ~ThumbnailDecoderImpl() override;

  void DecodeAndCropThumbnail(const std::string& thumbnail_data,
                              DecodeComplete complete_callback) override;

 private:
  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_THUMBNAIL_DECODER_IMPL_H_
