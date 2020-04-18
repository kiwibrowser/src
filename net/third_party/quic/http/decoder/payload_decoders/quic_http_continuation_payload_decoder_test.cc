// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_continuation_payload_decoder.h"

#include <stddef.h>

#include <cstdint>
#include <type_traits>

#include "base/logging.h"
#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_payload_decoder_base_test_util.h"
#include "net/third_party/quic/http/decoder/quic_http_frame_decoder_listener.h"
#include "net/third_party/quic/http/quic_http_constants.h"
#include "net/third_party/quic/http/quic_http_structures.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts_collector.h"
#include "net/third_party/quic/http/tools/quic_http_random_decoder_test.h"
#include "net/third_party/quic/platform/api/quic_string.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

// Provides friend access to an instance of the payload decoder, and also
// provides info to aid in testing.
class QuicHttpContinuationQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::CONTINUATION;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpContinuationQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    // QuicHttpContinuationQuicHttpPayloadDecoder has no fields,
    // so there is nothing to randomize.
    static_assert(
        std::is_empty<QuicHttpContinuationQuicHttpPayloadDecoder>::value,
        "Need to randomize fields of "
        "QuicHttpContinuationQuicHttpPayloadDecoder");
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnContinuationStart(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnContinuationStart: " << header;
    StartFrame(header)->OnContinuationStart(header);
  }

  void OnHpackFragment(const char* data, size_t len) override {
    VLOG(1) << "OnHpackFragment: len=" << len;
    CurrentFrame()->OnHpackFragment(data, len);
  }

  void OnContinuationEnd() override {
    VLOG(1) << "OnContinuationEnd";
    EndFrame()->OnContinuationEnd();
  }
};

class QuicHttpContinuationQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpContinuationQuicHttpPayloadDecoder,
          QuicHttpContinuationQuicHttpPayloadDecoderPeer,
          Listener>,
      public ::testing::WithParamInterface<uint32_t> {
 protected:
  QuicHttpContinuationQuicHttpPayloadDecoderTest() : length_(GetParam()) {
    VLOG(1) << "################  length_=" << length_ << "  ################";
  }

  const uint32_t length_;
};

INSTANTIATE_TEST_CASE_P(VariousLengths,
                        QuicHttpContinuationQuicHttpPayloadDecoderTest,
                        ::testing::Values(0, 1, 2, 3, 4, 5, 6));

TEST_P(QuicHttpContinuationQuicHttpPayloadDecoderTest, ValidLength) {
  QuicString hpack_payload = Random().RandString(length_);
  QuicHttpFrameHeader frame_header(length_, QuicHttpFrameType::CONTINUATION,
                                   RandFlags(), RandStreamId());
  set_frame_header(frame_header);
  QuicHttpFrameParts expected(frame_header, hpack_payload);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(hpack_payload, expected));
}

}  // namespace
}  // namespace test
}  // namespace net
