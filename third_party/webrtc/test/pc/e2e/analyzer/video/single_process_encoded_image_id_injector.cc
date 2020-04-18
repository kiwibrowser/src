/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/pc/e2e/analyzer/video/single_process_encoded_image_id_injector.h"

#include <cstddef>

#include "absl/memory/memory.h"
#include "api/video/encoded_image.h"
#include "rtc_base/checks.h"

namespace webrtc {
namespace test {
namespace {

// Number of bytes from the beginning of the EncodedImage buffer that will be
// used to store frame id and sub id.
constexpr size_t kUsedBufferSize = 3;

}  // namespace

SingleProcessEncodedImageIdInjector::SingleProcessEncodedImageIdInjector() =
    default;
SingleProcessEncodedImageIdInjector::~SingleProcessEncodedImageIdInjector() =
    default;

EncodedImage SingleProcessEncodedImageIdInjector::InjectId(
    uint16_t id,
    const EncodedImage& source,
    int coding_entity_id) {
  RTC_CHECK(source.size() >= kUsedBufferSize);

  ExtractionInfo info;
  info.length = source.size();
  memcpy(info.origin_data, source.data(), kUsedBufferSize);
  {
    rtc::CritScope crit(&lock_);
    // Will create new one if missed.
    ExtractionInfoVector& ev = extraction_cache_[id];
    info.sub_id = ev.next_sub_id++;
    ev.infos[info.sub_id] = info;
  }

  EncodedImage out = source;
  out.data()[0] = id & 0x00ff;
  out.data()[1] = (id & 0xff00) >> 8;
  out.data()[2] = info.sub_id;
  return out;
}

EncodedImageWithId SingleProcessEncodedImageIdInjector::ExtractId(
    const EncodedImage& source,
    int coding_entity_id) {
  EncodedImage out = source;

  size_t pos = 0;
  absl::optional<uint16_t> id = absl::nullopt;
  while (pos < source.size()) {
    // Extract frame id from first 2 bytes of the payload.
    uint16_t next_id = source.data()[pos] + (source.data()[pos + 1] << 8);
    // Extract frame sub id from second 2 byte of the payload.
    uint16_t sub_id = source.data()[pos + 2];

    RTC_CHECK(!id || id.value() == next_id)
        << "Different frames encoded into single encoded image: " << id.value()
        << " vs " << next_id;
    id = next_id;

    ExtractionInfo info;
    {
      rtc::CritScope crit(&lock_);
      auto ext_vector_it = extraction_cache_.find(next_id);
      RTC_CHECK(ext_vector_it != extraction_cache_.end())
          << "Unknown frame id " << next_id;

      auto info_it = ext_vector_it->second.infos.find(sub_id);
      RTC_CHECK(info_it != ext_vector_it->second.infos.end())
          << "Unknown sub id " << sub_id << " for frame " << next_id;
      info = info_it->second;
      ext_vector_it->second.infos.erase(info_it);
    }

    memcpy(&out.data()[pos], info.origin_data, kUsedBufferSize);
    pos += info.length;
  }
  out.set_size(pos);

  return EncodedImageWithId{id.value(), out};
}

SingleProcessEncodedImageIdInjector::ExtractionInfoVector::
    ExtractionInfoVector() = default;
SingleProcessEncodedImageIdInjector::ExtractionInfoVector::
    ~ExtractionInfoVector() = default;

}  // namespace test
}  // namespace webrtc
