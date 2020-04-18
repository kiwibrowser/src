// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_DECODE_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_DECODE_H_

#include <stdint.h>

#include <vector>

#include "services/data_decoder/public/mojom/image_decoder.mojom.h"

namespace gfx {
class Size;
}

namespace service_manager {
class Connector;
}

namespace data_decoder {

const uint64_t kDefaultMaxSizeInBytes = 128 * 1024 * 1024;

// Helper function to decode an image via the data_decoder service. For images
// with multiple frames (e.g. ico files), a frame with a size as close as
// possible to |desired_image_frame_size| is chosen (tries to take one in larger
// size if there's no precise match). Passing gfx::Size() as
// |desired_image_frame_size| is also supported and will result in chosing the
// smallest available size.
// Upon completion, |callback| is invoked on the calling thread TaskRunner with
// an SkBitmap argument. The SkBitmap will be null on failure and non-null on
// success.
void DecodeImage(service_manager::Connector* connector,
                 const std::vector<uint8_t>& encoded_bytes,
                 mojom::ImageCodec codec,
                 bool shrink_to_fit,
                 uint64_t max_size_in_bytes,
                 const gfx::Size& desired_image_frame_size,
                 mojom::ImageDecoder::DecodeImageCallback callback);

// Helper function to decode an animation via the data_decoder service. Any
// image with multiple frames is considered an animation, so long as the frames
// are all the same size.
void DecodeAnimation(service_manager::Connector* connector,
                     const std::vector<uint8_t>& encoded_bytes,
                     bool shrink_to_fit,
                     uint64_t max_size_in_bytes,
                     mojom::ImageDecoder::DecodeAnimationCallback callback);

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_DECODE_H_
