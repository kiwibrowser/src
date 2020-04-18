// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_altsvc_payload_decoder.h"

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

// Provides friend access to an instance of the payload decoder, and also
// provides info to aid in testing.
class QuicHttpAltSvcQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::ALTSVC;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpAltSvcQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    CorruptEnum(&p->payload_state_, rng);
    test::Randomize(&p->altsvc_fields_, rng);
    VLOG(1)
        << "QuicHttpAltSvcQuicHttpPayloadDecoderPeer::Randomize altsvc_fields_="
        << p->altsvc_fields_;
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnAltSvcStart(const QuicHttpFrameHeader& header,
                     size_t origin_length,
                     size_t value_length) override {
    VLOG(1) << "OnAltSvcStart header: " << header
            << "; origin_length=" << origin_length
            << "; value_length=" << value_length;
    StartFrame(header)->OnAltSvcStart(header, origin_length, value_length);
  }

  void OnAltSvcOriginData(const char* data, size_t len) override {
    VLOG(1) << "OnAltSvcOriginData: len=" << len;
    CurrentFrame()->OnAltSvcOriginData(data, len);
  }

  void OnAltSvcValueData(const char* data, size_t len) override {
    VLOG(1) << "OnAltSvcValueData: len=" << len;
    CurrentFrame()->OnAltSvcValueData(data, len);
  }

  void OnAltSvcEnd() override {
    VLOG(1) << "OnAltSvcEnd";
    EndFrame()->OnAltSvcEnd();
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpAltSvcQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpAltSvcQuicHttpPayloadDecoder,
          QuicHttpAltSvcQuicHttpPayloadDecoderPeer,
          Listener> {};

// Confirm we get an error if the payload is not long enough to hold
// QuicHttpAltSvcFields and the indicated length of origin.
TEST_F(QuicHttpAltSvcQuicHttpPayloadDecoderTest, Truncated) {
  QuicHttpFrameBuilder fb;
  fb.Append(
      QuicHttpAltSvcFields{0xffff});  // The longest possible origin length.
  fb.Append("Too little origin!");
  EXPECT_TRUE(
      VerifyDetectsFrameSizeError(0, fb.buffer(), /*approve_size*/ nullptr));
}

class QuicHttpAltSvcPayloadLengthTests
    : public QuicHttpAltSvcQuicHttpPayloadDecoderTest,
      public ::testing::WithParamInterface<
          ::testing::tuple<uint16_t, uint32_t>> {
 protected:
  QuicHttpAltSvcPayloadLengthTests()
      : origin_length_(::testing::get<0>(GetParam())),
        value_length_(::testing::get<1>(GetParam())) {
    VLOG(1) << "################  origin_length_=" << origin_length_
            << "   value_length_=" << value_length_ << "  ################";
  }

  const uint16_t origin_length_;
  const uint32_t value_length_;
};

INSTANTIATE_TEST_CASE_P(VariousOriginAndValueLengths,
                        QuicHttpAltSvcPayloadLengthTests,
                        ::testing::Combine(::testing::Values(0, 1, 3, 65535),
                                           ::testing::Values(0, 1, 3, 65537)));

TEST_P(QuicHttpAltSvcPayloadLengthTests, ValidOriginAndValueLength) {
  QuicString origin = Random().RandString(origin_length_);
  QuicString value = Random().RandString(value_length_);
  QuicHttpFrameBuilder fb;
  fb.Append(QuicHttpAltSvcFields{origin_length_});
  fb.Append(origin);
  fb.Append(value);
  QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::ALTSVC, RandFlags(),
                             RandStreamId());
  set_frame_header(header);
  QuicHttpFrameParts expected(header);
  expected.SetAltSvcExpected(origin, value);
  ASSERT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
}

}  // namespace
}  // namespace test
}  // namespace net
