/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_ANALYZER_VIDEO_SINGLE_PROCESS_ENCODED_IMAGE_ID_INJECTOR_H_
#define TEST_PC_E2E_ANALYZER_VIDEO_SINGLE_PROCESS_ENCODED_IMAGE_ID_INJECTOR_H_

#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "api/video/encoded_image.h"
#include "rtc_base/critical_section.h"
#include "test/pc/e2e/analyzer/video/encoded_image_id_injector.h"

namespace webrtc {
namespace test {

// Based on assumption that all call participants are in the same OS process
// and uses same QualityAnalyzingVideoContext to obtain EncodedImageIdInjector.
//
// To inject frame id into EncodedImage injector uses first 2 bytes of
// EncodedImage payload. Then it uses 3rd byte for frame sub id, that is
// required to distinguish different spatial layers. The origin data from these
// 3 bytes will be stored inside injector's internal storage and then will be
// restored during extraction phase.
//
// This injector won't add any extra overhead into EncodedImage payload and
// support frames with any size of payload. Also assumes that every EncodedImage
// payload size is greater or equals to 3 bytes
class SingleProcessEncodedImageIdInjector : public EncodedImageIdInjector,
                                            public EncodedImageIdExtractor {
 public:
  SingleProcessEncodedImageIdInjector();
  ~SingleProcessEncodedImageIdInjector() override;

  // Id will be injected into EncodedImage buffer directly. This buffer won't
  // be fully copied, so |source| image buffer will be also changed.
  EncodedImage InjectId(uint16_t id,
                        const EncodedImage& source,
                        int coding_entity_id) override;
  EncodedImageWithId ExtractId(const EncodedImage& source,
                               int coding_entity_id) override;

 private:
  // Contains data required to extract frame id from EncodedImage and restore
  // original buffer.
  struct ExtractionInfo {
    // Frame sub id to distinguish encoded images for different spatial layers.
    uint8_t sub_id;
    // Length of the origin buffer encoded image.
    size_t length;
    // Data from first 3 bytes of origin encoded image's payload.
    uint8_t origin_data[3];
  };

  struct ExtractionInfoVector {
    ExtractionInfoVector();
    ~ExtractionInfoVector();

    // Next sub id, that have to be used for this frame id.
    uint8_t next_sub_id = 0;
    std::map<uint8_t, ExtractionInfo> infos;
  };

  rtc::CriticalSection lock_;
  // Stores a mapping from frame id to extraction info for spatial layers
  // for this frame id. There can be a lot of them, because if frame was
  // dropped we can't clean it up, because we won't receive a signal on
  // decoder side about that frame. In such case it will be replaced
  // when sub id will overlap.
  std::map<uint16_t, ExtractionInfoVector> extraction_cache_
      RTC_GUARDED_BY(lock_);
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_PC_E2E_ANALYZER_VIDEO_SINGLE_PROCESS_ENCODED_IMAGE_ID_INJECTOR_H_
