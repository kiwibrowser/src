// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_ping_payload_decoder.h"

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

class QuicHttpPingQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::PING;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpPingQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    VLOG(1) << "QuicHttpPingQuicHttpPayloadDecoderPeer::Randomize";
    test::Randomize(&p->ping_fields_, rng);
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnPing(const QuicHttpFrameHeader& header,
              const QuicHttpPingFields& ping) override {
    VLOG(1) << "OnPing: " << header << "; " << ping;
    StartAndEndFrame(header)->OnPing(header, ping);
  }

  void OnPingAck(const QuicHttpFrameHeader& header,
                 const QuicHttpPingFields& ping) override {
    VLOG(1) << "OnPingAck: " << header << "; " << ping;
    StartAndEndFrame(header)->OnPingAck(header, ping);
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpPingQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpPingQuicHttpPayloadDecoder,
          QuicHttpPingQuicHttpPayloadDecoderPeer,
          Listener> {
 protected:
  QuicHttpPingFields RandPingFields() {
    QuicHttpPingFields fields;
    test::Randomize(&fields, RandomPtr());
    return fields;
  }
};

// Confirm we get an error if the payload is not the correct size to hold
// exactly one QuicHttpPingFields.
TEST_F(QuicHttpPingQuicHttpPayloadDecoderTest, WrongSize) {
  auto approve_size = [](size_t size) {
    return size != QuicHttpPingFields::EncodedSize();
  };
  QuicHttpFrameBuilder fb;
  fb.Append(RandPingFields());
  fb.Append(RandPingFields());
  fb.Append(RandPingFields());
  EXPECT_TRUE(VerifyDetectsFrameSizeError(0, fb.buffer(), approve_size));
}

TEST_F(QuicHttpPingQuicHttpPayloadDecoderTest, Ping) {
  for (int n = 0; n < 100; ++n) {
    QuicHttpPingFields fields = RandPingFields();
    QuicHttpFrameBuilder fb;
    fb.Append(fields);
    QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::PING,
                               RandFlags() & ~QuicHttpFrameFlag::QUIC_HTTP_ACK,
                               RandStreamId());
    set_frame_header(header);
    QuicHttpFrameParts expected(header);
    expected.opt_ping = fields;
    EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
  }
}

TEST_F(QuicHttpPingQuicHttpPayloadDecoderTest, PingAck) {
  for (int n = 0; n < 100; ++n) {
    QuicHttpPingFields fields;
    Randomize(&fields, RandomPtr());
    QuicHttpFrameBuilder fb;
    fb.Append(fields);
    QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::PING,
                               RandFlags() | QuicHttpFrameFlag::QUIC_HTTP_ACK,
                               RandStreamId());
    set_frame_header(header);
    QuicHttpFrameParts expected(header);
    expected.opt_ping = fields;
    EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
  }
}

}  // namespace
}  // namespace test
}  // namespace net
