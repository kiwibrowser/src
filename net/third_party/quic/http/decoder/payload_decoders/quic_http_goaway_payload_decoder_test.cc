// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_goaway_payload_decoder.h"

#include <stddef.h>

#include <cstdint>

#include "base/logging.h"
#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_payload_decoder_base_test_util.h"
#include "net/third_party/quic/http/decoder/quic_http_frame_decoder_listener.h"
#include "net/third_party/quic/http/quic_http_constants.h"
#include "net/third_party/quic/http/quic_http_structures_test_util.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts_collector.h"
#include "net/third_party/quic/http/tools/quic_http_frame_builder.h"
#include "net/third_party/quic/http/tools/quic_http_random_decoder_test.h"
#include "net/third_party/quic/platform/api/quic_string.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

class QuicHttpGoAwayQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::GOAWAY;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpGoAwayQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    CorruptEnum(&p->payload_state_, rng);
    test::Randomize(&p->goaway_fields_, rng);
    VLOG(1)
        << "QuicHttpGoAwayQuicHttpPayloadDecoderPeer::Randomize goaway_fields: "
        << p->goaway_fields_;
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnGoAwayStart(const QuicHttpFrameHeader& header,
                     const QuicHttpGoAwayFields& goaway) override {
    VLOG(1) << "OnGoAwayStart header: " << header << "; goaway: " << goaway;
    StartFrame(header)->OnGoAwayStart(header, goaway);
  }

  void OnGoAwayOpaqueData(const char* data, size_t len) override {
    VLOG(1) << "OnGoAwayOpaqueData: len=" << len;
    CurrentFrame()->OnGoAwayOpaqueData(data, len);
  }

  void OnGoAwayEnd() override {
    VLOG(1) << "OnGoAwayEnd";
    EndFrame()->OnGoAwayEnd();
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpGoAwayQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpGoAwayQuicHttpPayloadDecoder,
          QuicHttpGoAwayQuicHttpPayloadDecoderPeer,
          Listener> {};

// Confirm we get an error if the payload is not long enough to hold
// QuicHttpGoAwayFields.
TEST_F(QuicHttpGoAwayQuicHttpPayloadDecoderTest, Truncated) {
  auto approve_size = [](size_t size) {
    return size != QuicHttpGoAwayFields::EncodedSize();
  };
  QuicHttpFrameBuilder fb;
  fb.Append(QuicHttpGoAwayFields(123, QuicHttpErrorCode::ENHANCE_YOUR_CALM));
  EXPECT_TRUE(VerifyDetectsFrameSizeError(0, fb.buffer(), approve_size));
}

class QuicHttpGoAwayOpaqueDataLengthTests
    : public QuicHttpGoAwayQuicHttpPayloadDecoderTest,
      public ::testing::WithParamInterface<uint32_t> {
 protected:
  QuicHttpGoAwayOpaqueDataLengthTests() : length_(GetParam()) {
    VLOG(1) << "################  length_=" << length_ << "  ################";
  }

  const uint32_t length_;
};

INSTANTIATE_TEST_CASE_P(VariousLengths,
                        QuicHttpGoAwayOpaqueDataLengthTests,
                        ::testing::Values(0, 1, 2, 3, 4, 5, 6));

TEST_P(QuicHttpGoAwayOpaqueDataLengthTests, ValidLength) {
  QuicHttpGoAwayFields goaway;
  Randomize(&goaway, RandomPtr());
  QuicString opaque_bytes = Random().RandString(length_);
  QuicHttpFrameBuilder fb;
  fb.Append(goaway);
  fb.Append(opaque_bytes);
  QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::GOAWAY, RandFlags(),
                             RandStreamId());
  set_frame_header(header);
  QuicHttpFrameParts expected(header, opaque_bytes);
  expected.opt_goaway = goaway;
  ASSERT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
}

}  // namespace
}  // namespace test
}  // namespace net
