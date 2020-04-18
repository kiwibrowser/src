/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_ANALYZER_VIDEO_ENCODED_IMAGE_ID_INJECTOR_H_
#define TEST_PC_E2E_ANALYZER_VIDEO_ENCODED_IMAGE_ID_INJECTOR_H_

#include <cstdint>
#include <utility>

#include "api/video/encoded_image.h"

namespace webrtc {
namespace test {

// Injects frame id into EncodedImage on encoder side
class EncodedImageIdInjector {
 public:
  virtual ~EncodedImageIdInjector() = default;

  // Return encoded image with specified |id| injected into its payload.
  // |coding_entity_id| is unique id of decoder or encoder.
  virtual EncodedImage InjectId(uint16_t id,
                                const EncodedImage& source,
                                int coding_entity_id) = 0;
};

struct EncodedImageWithId {
  uint16_t id;
  EncodedImage image;
};

// Extracts frame id from EncodedImage on decoder side.
class EncodedImageIdExtractor {
 public:
  virtual ~EncodedImageIdExtractor() = default;

  // Returns encoded image id, extracted from payload and also encoded image
  // with its original payload. For concatenated spatial layers it should be the
  // same id. |coding_entity_id| is unique id of decoder or encoder.
  virtual EncodedImageWithId ExtractId(const EncodedImage& source,
                                       int coding_entity_id) = 0;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_PC_E2E_ANALYZER_VIDEO_ENCODED_IMAGE_ID_INJECTOR_H_
