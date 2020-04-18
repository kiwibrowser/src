// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_priority_payload_decoder.h"

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
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

class QuicHttpPriorityQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::QUIC_HTTP_PRIORITY;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpPriorityQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    VLOG(1) << "QuicHttpPriorityQuicHttpPayloadDecoderPeer::Randomize";
    test::Randomize(&p->priority_fields_, rng);
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnPriorityFrame(const QuicHttpFrameHeader& header,
                       const QuicHttpPriorityFields& priority_fields) override {
    VLOG(1) << "OnPriority: " << header << "; " << priority_fields;
    StartAndEndFrame(header)->OnPriorityFrame(header, priority_fields);
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpPriorityQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpPriorityQuicHttpPayloadDecoder,
          QuicHttpPriorityQuicHttpPayloadDecoderPeer,
          Listener> {
 protected:
  QuicHttpPriorityFields RandPriorityFields() {
    QuicHttpPriorityFields fields;
    test::Randomize(&fields, RandomPtr());
    return fields;
  }
};

// Confirm we get an error if the payload is not the correct size to hold
// exactly one QuicHttpPriorityFields.
TEST_F(QuicHttpPriorityQuicHttpPayloadDecoderTest, WrongSize) {
  auto approve_size = [](size_t size) {
    return size != QuicHttpPriorityFields::EncodedSize();
  };
  QuicHttpFrameBuilder fb;
  fb.Append(RandPriorityFields());
  fb.Append(RandPriorityFields());
  EXPECT_TRUE(VerifyDetectsFrameSizeError(0, fb.buffer(), approve_size));
}

TEST_F(QuicHttpPriorityQuicHttpPayloadDecoderTest, VariousPayloads) {
  for (int n = 0; n < 100; ++n) {
    QuicHttpPriorityFields fields = RandPriorityFields();
    QuicHttpFrameBuilder fb;
    fb.Append(fields);
    QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::QUIC_HTTP_PRIORITY,
                               RandFlags(), RandStreamId());
    set_frame_header(header);
    QuicHttpFrameParts expected(header);
    expected.opt_priority = fields;
    EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
  }
}

}  // namespace
}  // namespace test
}  // namespace net
