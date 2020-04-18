// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_headers_payload_decoder.h"

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

class QuicHttpHeadersQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::HEADERS;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() {
    return QuicHttpFrameFlag::QUIC_HTTP_PADDED |
           QuicHttpFrameFlag::QUIC_HTTP_PRIORITY;
  }

  static void Randomize(QuicHttpHeadersQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    CorruptEnum(&p->payload_state_, rng);
    test::Randomize(&p->priority_fields_, rng);
    VLOG(1) << "QuicHttpHeadersQuicHttpPayloadDecoderPeer::Randomize "
               "priority_fields_: "
            << p->priority_fields_;
  }
};

namespace {

// Listener handles all On* methods that are expected to be called. If any other
// On* methods of QuicHttpFrameDecoderListener is called then the test fails;
// this is achieved by way of FailingQuicHttpFrameDecoderListener, the base
// class of QuicHttpFramePartsCollector. These On* methods make use of
// StartFrame, EndFrame, etc. of the base class to create and access to
// QuicHttpFrameParts instance(s) that will record the details. After decoding,
// the test validation code can access the FramePart instance(s) via the public
// methods of QuicHttpFramePartsCollector.
struct Listener : public QuicHttpFramePartsCollector {
  void OnHeadersStart(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnHeadersStart: " << header;
    StartFrame(header)->OnHeadersStart(header);
  }

  void OnHeadersPriority(const QuicHttpPriorityFields& priority) override {
    VLOG(1) << "OnHeadersPriority: " << priority;
    CurrentFrame()->OnHeadersPriority(priority);
  }

  void OnHpackFragment(const char* data, size_t len) override {
    VLOG(1) << "OnHpackFragment: len=" << len;
    CurrentFrame()->OnHpackFragment(data, len);
  }

  void OnHeadersEnd() override {
    VLOG(1) << "OnHeadersEnd";
    EndFrame()->OnHeadersEnd();
  }

  void OnPadLength(size_t pad_length) override {
    VLOG(1) << "OnPadLength: " << pad_length;
    CurrentFrame()->OnPadLength(pad_length);
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

class QuicHttpHeadersQuicHttpPayloadDecoderTest
    : public AbstractPaddableQuicHttpPayloadDecoderTest<
          QuicHttpHeadersQuicHttpPayloadDecoder,
          QuicHttpHeadersQuicHttpPayloadDecoderPeer,
          Listener> {};

INSTANTIATE_TEST_CASE_P(VariousPadLengths,
                        QuicHttpHeadersQuicHttpPayloadDecoderTest,
                        ::testing::Values(0, 1, 2, 3, 4, 254, 255, 256));

// Decode various sizes of (fake) HPQUIC_HTTP_ACK payload, both with and without
// the QUIC_HTTP_PRIORITY flag set.
TEST_P(QuicHttpHeadersQuicHttpPayloadDecoderTest, VariousHpackPayloadSizes) {
  for (size_t hpack_size : {0, 1, 2, 3, 255, 256, 1024}) {
    LOG(INFO) << "###########   hpack_size = " << hpack_size << "  ###########";
    QuicHttpPriorityFields priority(RandStreamId(), 1 + Random().Rand8(),
                                    Random().OneIn(2));

    for (bool has_priority : {false, true}) {
      Reset();
      ASSERT_EQ(IsPadded() ? 1u : 0u, frame_builder_.size());
      uint8_t flags = RandFlags();
      if (has_priority) {
        flags |= QuicHttpFrameFlag::QUIC_HTTP_PRIORITY;
        frame_builder_.Append(priority);
      }

      QuicString hpack_payload = Random().RandString(hpack_size);
      frame_builder_.Append(hpack_payload);

      MaybeAppendTrailingPadding();
      QuicHttpFrameHeader frame_header(frame_builder_.size(),
                                       QuicHttpFrameType::HEADERS, flags,
                                       RandStreamId());
      set_frame_header(frame_header);
      ScrubFlagsOfHeader(&frame_header);
      QuicHttpFrameParts expected(frame_header, hpack_payload,
                                  total_pad_length_);
      if (has_priority) {
        expected.opt_priority = priority;
      }
      EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(frame_builder_.buffer(),
                                                      expected));
    }
  }
}

// Confirm we get an error if the QUIC_HTTP_PRIORITY flag is set but the payload
// is not long enough, regardless of the amount of (valid) padding.
TEST_P(QuicHttpHeadersQuicHttpPayloadDecoderTest, Truncated) {
  auto approve_size = [](size_t size) {
    return size != QuicHttpPriorityFields::EncodedSize();
  };
  QuicHttpFrameBuilder fb;
  fb.Append(QuicHttpPriorityFields(RandStreamId(), 1 + Random().Rand8(),
                                   Random().OneIn(2)));
  EXPECT_TRUE(VerifyDetectsMultipleFrameSizeErrors(
      QuicHttpFrameFlag::QUIC_HTTP_PRIORITY, fb.buffer(), approve_size,
      total_pad_length_));
}

// Confirm we get an error if the QUIC_HTTP_PADDED flag is set but the payload
// is not long enough to hold even the Pad Length amount of padding.
TEST_P(QuicHttpHeadersQuicHttpPayloadDecoderTest, PaddingTooLong) {
  EXPECT_TRUE(VerifyDetectsPaddingTooLong());
}

}  // namespace
}  // namespace test
}  // namespace net
