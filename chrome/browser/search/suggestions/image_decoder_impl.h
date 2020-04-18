// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_SUGGESTIONS_IMAGE_DECODER_IMPL_H_
#define CHROME_BROWSER_SEARCH_SUGGESTIONS_IMAGE_DECODER_IMPL_H_

#include <memory>
#include <vector>

#include "chrome/browser/image_decoder.h"
#include "components/image_fetcher/core/image_decoder.h"

namespace suggestions {

// image_fetcher::ImageDecoder implementation.
// TODO(treib,markusheintz): Move this to a better place - it really has
// nothing to do with suggestions. crbug.com/624761
class ImageDecoderImpl : public image_fetcher::ImageDecoder {
 public:
  ImageDecoderImpl();
  ~ImageDecoderImpl() override;

  void DecodeImage(
      const std::string& image_data,
      const gfx::Size& desired_image_frame_size,
      const image_fetcher::ImageDecodedCallback& callback) override;

 private:
  class DecodeImageRequest;

  // Removes the passed image decode |request| from the internal request queue.
  void RemoveDecodeImageRequest(DecodeImageRequest* request);

  // All active image decoding requests.
  std::vector<std::unique_ptr<DecodeImageRequest>> decode_image_requests_;

  DISALLOW_COPY_AND_ASSIGN(ImageDecoderImpl);
};

}  // namespace suggestions

#endif  // CHROME_BROWSER_SEARCH_SUGGESTIONS_IMAGE_DECODER_IMPL_H_

