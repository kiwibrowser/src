// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_window_update_payload_decoder.h"

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

class QuicHttpWindowUpdateQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::WINDOW_UPDATE;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() { return 0; }

  static void Randomize(QuicHttpWindowUpdateQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    test::Randomize(&p->window_update_fields_, rng);
    VLOG(1) << "QuicHttpWindowUpdateQuicHttpPayloadDecoderPeer::Randomize "
            << "window_update_fields_: " << p->window_update_fields_;
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnWindowUpdate(const QuicHttpFrameHeader& header,
                      uint32_t window_size_increment) override {
    VLOG(1) << "OnWindowUpdate: " << header
            << "; window_size_increment=" << window_size_increment;
    EXPECT_EQ(QuicHttpFrameType::WINDOW_UPDATE, header.type);
    StartAndEndFrame(header)->OnWindowUpdate(header, window_size_increment);
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpWindowUpdateQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpWindowUpdateQuicHttpPayloadDecoder,
          QuicHttpWindowUpdateQuicHttpPayloadDecoderPeer,
          Listener> {
 protected:
  QuicHttpWindowUpdateFields RandWindowUpdateFields() {
    QuicHttpWindowUpdateFields fields;
    test::Randomize(&fields, RandomPtr());
    VLOG(3) << "RandWindowUpdateFields: " << fields;
    return fields;
  }
};

// Confirm we get an error if the payload is not the correct size to hold
// exactly one QuicHttpWindowUpdateFields.
TEST_F(QuicHttpWindowUpdateQuicHttpPayloadDecoderTest, WrongSize) {
  auto approve_size = [](size_t size) {
    return size != QuicHttpWindowUpdateFields::EncodedSize();
  };
  QuicHttpFrameBuilder fb;
  fb.Append(RandWindowUpdateFields());
  fb.Append(RandWindowUpdateFields());
  fb.Append(RandWindowUpdateFields());
  EXPECT_TRUE(VerifyDetectsFrameSizeError(0, fb.buffer(), approve_size));
}

TEST_F(QuicHttpWindowUpdateQuicHttpPayloadDecoderTest, VariousPayloads) {
  for (int n = 0; n < 100; ++n) {
    uint32_t stream_id = n == 0 ? 0 : RandStreamId();
    QuicHttpWindowUpdateFields fields = RandWindowUpdateFields();
    QuicHttpFrameBuilder fb;
    fb.Append(fields);
    QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::WINDOW_UPDATE,
                               RandFlags(), stream_id);
    set_frame_header(header);
    QuicHttpFrameParts expected(header);
    expected.opt_window_update_increment = fields.window_size_increment;
    EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
  }
}

}  // namespace
}  // namespace test
}  // namespace net
