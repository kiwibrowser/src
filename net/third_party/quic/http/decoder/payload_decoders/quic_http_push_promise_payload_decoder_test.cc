// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_push_promise_payload_decoder.h"

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
class QuicHttpPushPromiseQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::PUSH_PROMISE;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() {
    return QuicHttpFrameFlag::QUIC_HTTP_PADDED;
  }

  static void Randomize(QuicHttpPushPromiseQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    VLOG(1) << "QuicHttpPushPromiseQuicHttpPayloadDecoderPeer::Randomize";
    CorruptEnum(&p->payload_state_, rng);
    test::Randomize(&p->push_promise_fields_, rng);
  }
};

namespace {

// Listener listens for only those methods expected by the payload decoder
// under test, and forwards them onto the QuicHttpFrameParts instance for the
// current frame.
struct Listener : public QuicHttpFramePartsCollector {
  void OnPushPromiseStart(const QuicHttpFrameHeader& header,
                          const QuicHttpPushPromiseFields& promise,
                          size_t total_padding_length) override {
    VLOG(1) << "OnPushPromiseStart header: " << header
            << "  promise: " << promise
            << "  total_padding_length: " << total_padding_length;
    EXPECT_EQ(QuicHttpFrameType::PUSH_PROMISE, header.type);
    StartFrame(header)->OnPushPromiseStart(header, promise,
                                           total_padding_length);
  }

  void OnHpackFragment(const char* data, size_t len) override {
    VLOG(1) << "OnHpackFragment: len=" << len;
    CurrentFrame()->OnHpackFragment(data, len);
  }

  void OnPushPromiseEnd() override {
    VLOG(1) << "OnPushPromiseEnd";
    EndFrame()->OnPushPromiseEnd();
  }

  void OnPadding(const char* padding, size_t skipped_length) override {
    VLOG(1) << "OnPadding: " << skipped_length;
    CurrentFrame()->OnPadding(padding, skipped_length);
  }

  void OnPaddingTooLong(const QuicHttpFrameHeader& header,
                        size_t missing_length) override {
    VLOG(1) << "OnPaddingTooLong: " << header
            << "; missing_length: " << missing_length;
    FrameError(header)->OnPaddingTooLong(header, missing_length);
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpPushPromiseQuicHttpPayloadDecoderTest
    : public AbstractPaddableQuicHttpPayloadDecoderTest<
          QuicHttpPushPromiseQuicHttpPayloadDecoder,
          QuicHttpPushPromiseQuicHttpPayloadDecoderPeer,
          Listener> {};

INSTANTIATE_TEST_CASE_P(VariousPadLengths,
                        QuicHttpPushPromiseQuicHttpPayloadDecoderTest,
                        ::testing::Values(0, 1, 2, 3, 4, 254, 255, 256));

// Payload contains the required QuicHttpPushPromiseFields, followed by some
// (fake) HPQUIC_HTTP_ACK payload.
TEST_P(QuicHttpPushPromiseQuicHttpPayloadDecoderTest,
       VariousHpackPayloadSizes) {
  for (size_t hpack_size : {0, 1, 2, 3, 255, 256, 1024}) {
    LOG(INFO) << "###########   hpack_size = " << hpack_size << "  ###########";
    Reset();
    QuicString hpack_payload = Random().RandString(hpack_size);
    QuicHttpPushPromiseFields push_promise{RandStreamId()};
    frame_builder_.Append(push_promise);
    frame_builder_.Append(hpack_payload);
    MaybeAppendTrailingPadding();
    QuicHttpFrameHeader frame_header(frame_builder_.size(),
                                     QuicHttpFrameType::PUSH_PROMISE,
                                     RandFlags(), RandStreamId());
    set_frame_header(frame_header);
    QuicHttpFrameParts expected(frame_header, hpack_payload, total_pad_length_);
    expected.opt_push_promise = push_promise;
    EXPECT_TRUE(
        DecodePayloadAndValidateSeveralWays(frame_builder_.buffer(), expected));
  }
}

// Confirm we get an error if the payload is not long enough for the required
// portion of the payload, regardless of the amount of (valid) padding.
TEST_P(QuicHttpPushPromiseQuicHttpPayloadDecoderTest, Truncated) {
  auto approve_size = [](size_t size) {
    return size != QuicHttpPushPromiseFields::EncodedSize();
  };
  QuicHttpPushPromiseFields push_promise{RandStreamId()};
  QuicHttpFrameBuilder fb;
  fb.Append(push_promise);
  EXPECT_TRUE(VerifyDetectsMultipleFrameSizeErrors(0, fb.buffer(), approve_size,
                                                   total_pad_length_));
}

// Confirm we get an error if the QUIC_HTTP_PADDED flag is set but the payload
// is not long enough to hold even the Pad Length amount of padding.
TEST_P(QuicHttpPushPromiseQuicHttpPayloadDecoderTest, PaddingTooLong) {
  EXPECT_TRUE(VerifyDetectsPaddingTooLong());
}

}  // namespace
}  // namespace test
}  // namespace net
