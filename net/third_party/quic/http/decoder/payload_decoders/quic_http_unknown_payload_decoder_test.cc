// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_unknown_payload_decoder.h"

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
namespace {
QuicHttpFrameType g_unknown_frame_type;
}  // namespace

// Provides friend access to an instance of the payload decoder, and also
// provides info to aid in testing.
class QuicHttpUnknownQuicHttpPayloadDecoderPeer {
 public:
  static QuicHttpFrameType FrameType() { return g_unknown_frame_type; }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpUnknownQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    // QuicHttpUnknownQuicHttpPayloadDecoder has no fields, so there is nothing
    // to randomize.
    static_assert(
        std::is_empty<QuicHttpUnknownQuicHttpPayloadDecoder>::value,
        "Need to randomize fields of QuicHttpUnknownQuicHttpPayloadDecoder");
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnUnknownStart(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnUnknownStart: " << header;
    StartFrame(header)->OnUnknownStart(header);
  }

  void OnUnknownPayload(const char* data, size_t len) override {
    VLOG(1) << "OnUnknownPayload: len=" << len;
    CurrentFrame()->OnUnknownPayload(data, len);
  }

  void OnUnknownEnd() override {
    VLOG(1) << "OnUnknownEnd";
    EndFrame()->OnUnknownEnd();
  }
};

constexpr bool SupportedFrameType = false;

class QuicHttpUnknownQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpUnknownQuicHttpPayloadDecoder,
          QuicHttpUnknownQuicHttpPayloadDecoderPeer,
          Listener,
          SupportedFrameType>,
      public ::testing::WithParamInterface<uint32_t> {
 protected:
  QuicHttpUnknownQuicHttpPayloadDecoderTest() : length_(GetParam()) {
    VLOG(1) << "################  length_=" << length_ << "  ################";

    // Each test case will choose a random frame type that isn't supported.
    do {
      g_unknown_frame_type = static_cast<QuicHttpFrameType>(Random().Rand8());
    } while (IsSupportedQuicHttpFrameType(g_unknown_frame_type));
  }

  const uint32_t length_;
};

INSTANTIATE_TEST_CASE_P(VariousLengths,
                        QuicHttpUnknownQuicHttpPayloadDecoderTest,
                        ::testing::Values(0, 1, 2, 3, 255, 256));

TEST_P(QuicHttpUnknownQuicHttpPayloadDecoderTest, ValidLength) {
  QuicString unknown_payload = Random().RandString(length_);
  QuicHttpFrameHeader frame_header(length_, g_unknown_frame_type,
                                   Random().Rand8(), RandStreamId());
  set_frame_header(frame_header);
  QuicHttpFrameParts expected(frame_header, unknown_payload);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(unknown_payload, expected));
  // TODO(jamessynge): Check here (and in other such tests) that the fast
  // and slow decode counts are both non-zero. Perhaps also add some kind of
  // test for the listener having been called. That could simply be a test
  // that there is a single collected QuicHttpFrameParts instance, and that it
  // matches expected.
}

}  // namespace
}  // namespace test
}  // namespace net
