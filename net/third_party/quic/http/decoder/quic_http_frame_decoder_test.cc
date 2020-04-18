// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/quic_http_frame_decoder.h"

// Tests of QuicHttpFrameDecoder.

#include <vector>

#include "base/logging.h"
#include "net/third_party/quic/http/quic_http_constants.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts_collector_listener.h"
#include "net/third_party/quic/http/tools/quic_http_random_decoder_test.h"
#include "net/third_party/quic/platform/api/quic_reconstruct_object.h"
#include "net/third_party/quic/platform/api/quic_string.h"
#include "net/third_party/quic/platform/api/quic_string_piece.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::AssertionResult;
using ::testing::AssertionSuccess;

namespace net {
namespace test {
class QuicHttpFrameDecoderPeer {
 public:
  static size_t remaining_total_payload(QuicHttpFrameDecoder* decoder) {
    return decoder->frame_decoder_state_.remaining_total_payload();
  }
};

namespace {

class QuicHttpFrameDecoderTest : public QuicHttpRandomDecoderTest {
 protected:
  void SetUp() override {
    // On any one run of this suite, we'll always choose the same value for
    // use_default_reconstruct_ because the random seed is the same for each
    // test case, but across runs the random seed changes.
    use_default_reconstruct_ = Random().OneIn(2);
  }

  QuicHttpDecodeStatus StartDecoding(QuicHttpDecodeBuffer* db) override {
    DVLOG(2) << "StartDecoding, db->Remaining=" << db->Remaining();
    collector_.Reset();
    PrepareDecoder();

    QuicHttpDecodeStatus status = decoder_.DecodeFrame(db);
    if (status != QuicHttpDecodeStatus::kDecodeInProgress) {
      // Keep track of this so that a concrete test can verify that both fast
      // and slow decoding paths have been tested.
      ++fast_decode_count_;
      if (status == QuicHttpDecodeStatus::kDecodeError) {
        ConfirmDiscardsRemainingPayload();
      }
    }
    return status;
  }

  QuicHttpDecodeStatus ResumeDecoding(QuicHttpDecodeBuffer* db) override {
    DVLOG(2) << "ResumeDecoding, db->Remaining=" << db->Remaining();
    QuicHttpDecodeStatus status = decoder_.DecodeFrame(db);
    if (status != QuicHttpDecodeStatus::kDecodeInProgress) {
      // Keep track of this so that a concrete test can verify that both fast
      // and slow decoding paths have been tested.
      ++slow_decode_count_;
      if (status == QuicHttpDecodeStatus::kDecodeError) {
        ConfirmDiscardsRemainingPayload();
      }
    }
    return status;
  }

  // When an error is returned, the decoder is in state kDiscardPayload, and
  // stays there until the remaining bytes of the frame's payload have been
  // skipped over. There are no callbacks for this situation.
  void ConfirmDiscardsRemainingPayload() {
    ASSERT_TRUE(decoder_.IsDiscardingPayload());
    size_t remaining =
        QuicHttpFrameDecoderPeer::remaining_total_payload(&decoder_);
    // The decoder will discard the remaining bytes, but not go beyond that,
    // which these conditions verify.
    size_t extra = 10;
    QuicString junk(remaining + extra, '0');
    QuicHttpDecodeBuffer tmp(junk);
    EXPECT_EQ(QuicHttpDecodeStatus::kDecodeDone, decoder_.DecodeFrame(&tmp));
    EXPECT_EQ(remaining, tmp.Offset());
    EXPECT_EQ(extra, tmp.Remaining());
    EXPECT_FALSE(decoder_.IsDiscardingPayload());
  }

  void PrepareDecoder() {
    // Save and restore the maximum_payload_size when reconstructing
    // the decoder.
    size_t maximum_payload_size = decoder_.maximum_payload_size();

    // Alternate which constructor is used.
    if (use_default_reconstruct_) {
      QuicDefaultReconstructObject(&decoder_, RandomPtr());
      decoder_.set_listener(&collector_);
    } else {
      QuicReconstructObject(&decoder_, RandomPtr(), &collector_);
    }
    decoder_.set_maximum_payload_size(maximum_payload_size);

    use_default_reconstruct_ = !use_default_reconstruct_;
  }

  void ResetDecodeSpeedCounters() {
    fast_decode_count_ = 0;
    slow_decode_count_ = 0;
  }

  AssertionResult VerifyCollected(const QuicHttpFrameParts& expected) {
    VERIFY_FALSE(collector_.IsInProgress());
    VERIFY_EQ(1u, collector_.size());
    VERIFY_AND_RETURN_SUCCESS(expected.VerifyEquals(*collector_.frame(0)));
  }

  AssertionResult DecodePayloadAndValidateSeveralWays(
      QuicStringPiece payload,
      const Validator& validator) {
    QuicHttpDecodeBuffer db(payload);
    bool start_decoding_requires_non_empty = false;
    return DecodeAndValidateSeveralWays(&db, start_decoding_requires_non_empty,
                                        validator);
  }

  // Decode one frame's payload and confirm that the listener recorded the
  // expected QuicHttpFrameParts instance, and only one QuicHttpFrameParts
  // instance. The payload will be decoded several times with different
  // partitionings of the payload, and after each the validator will be called.
  AssertionResult DecodePayloadAndValidateSeveralWays(
      QuicStringPiece payload,
      const QuicHttpFrameParts& expected) {
    auto validator = [&expected, this](
                         const QuicHttpDecodeBuffer& input,
                         QuicHttpDecodeStatus status) -> AssertionResult {
      VERIFY_EQ(status, QuicHttpDecodeStatus::kDecodeDone);
      VERIFY_AND_RETURN_SUCCESS(VerifyCollected(expected));
    };
    ResetDecodeSpeedCounters();
    VERIFY_SUCCESS(DecodePayloadAndValidateSeveralWays(
        payload, ValidateDoneAndEmpty(validator)));
    VERIFY_GT(fast_decode_count_, 0u);
    VERIFY_GT(slow_decode_count_, 0u);

    // Repeat with more input; it should stop without reading that input.
    QuicString next_frame = Random().RandString(10);
    QuicString input(payload);
    input += next_frame;

    ResetDecodeSpeedCounters();
    VERIFY_SUCCESS(DecodePayloadAndValidateSeveralWays(
        payload, ValidateDoneAndOffset(payload.size(), validator)));
    VERIFY_GT(fast_decode_count_, 0u);
    VERIFY_GT(slow_decode_count_, 0u);

    return AssertionSuccess();
  }

  template <size_t N>
  AssertionResult DecodePayloadAndValidateSeveralWays(
      const char (&buf)[N],
      const QuicHttpFrameParts& expected) {
    return DecodePayloadAndValidateSeveralWays(QuicStringPiece(buf, N),
                                               expected);
  }

  template <size_t N>
  AssertionResult DecodePayloadAndValidateSeveralWays(
      const char (&buf)[N],
      const QuicHttpFrameHeader& header) {
    return DecodePayloadAndValidateSeveralWays(QuicStringPiece(buf, N),
                                               QuicHttpFrameParts(header));
  }

  template <size_t N>
  AssertionResult DecodePayloadExpectingError(
      const char (&buf)[N],
      const QuicHttpFrameParts& expected) {
    auto validator = [&expected, this](
                         const QuicHttpDecodeBuffer& input,
                         QuicHttpDecodeStatus status) -> AssertionResult {
      VERIFY_EQ(status, QuicHttpDecodeStatus::kDecodeError);
      VERIFY_AND_RETURN_SUCCESS(VerifyCollected(expected));
    };
    ResetDecodeSpeedCounters();
    EXPECT_TRUE(
        DecodePayloadAndValidateSeveralWays(ToStringPiece(buf), validator));
    EXPECT_GT(fast_decode_count_, 0u);
    EXPECT_GT(slow_decode_count_, 0u);
    return AssertionSuccess();
  }

  template <size_t N>
  AssertionResult DecodePayloadExpectingFrameSizeError(
      const char (&buf)[N],
      QuicHttpFrameParts expected) {
    expected.has_frame_size_error = true;
    VERIFY_AND_RETURN_SUCCESS(DecodePayloadExpectingError(buf, expected));
  }

  template <size_t N>
  AssertionResult DecodePayloadExpectingFrameSizeError(
      const char (&buf)[N],
      const QuicHttpFrameHeader& header) {
    return DecodePayloadExpectingFrameSizeError(buf,
                                                QuicHttpFrameParts(header));
  }

  // Count of payloads that are fully decoded by StartDecodingPayload or for
  // which an error was detected by StartDecodingPayload.
  size_t fast_decode_count_ = 0;

  // Count of payloads that required calling ResumeDecodingPayload in order to
  // decode completely, or for which an error was detected by
  // ResumeDecodingPayload.
  size_t slow_decode_count_ = 0;

  QuicHttpFramePartsCollectorListener collector_;
  QuicHttpFrameDecoder decoder_;
  bool use_default_reconstruct_;
};

////////////////////////////////////////////////////////////////////////////////
// Tests that pass the minimum allowed size for the frame type, which is often
// empty. The tests are in order by frame type value (i.e. 0 for DATA frames).

TEST_F(QuicHttpFrameDecoderTest, DataEmpty) {
  const char kFrameData[] = {
      0x00, 0x00, 0x00,        // Payload length: 0
      0x00,                    // DATA
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x00,  // Stream ID: 0 (invalid but unchecked here)
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::DATA, 0, 0);
  QuicHttpFrameParts expected(header, "");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeadersEmpty) {
  const char kFrameData[] = {
      0x00, 0x00, 0x00,        // Payload length: 0
      0x01,                    // HEADERS
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x01,  // Stream ID: 0  (REQUIRES ID)
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::HEADERS, 0, 1);
  QuicHttpFrameParts expected(header, "");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, Priority) {
  const char kFrameData[] = {
      0x00,  0x00, 0x05,        // Length: 5
      0x02,                     //   Type: QUIC_HTTP_PRIORITY
      0x00,                     //  Flags: none
      0x00,  0x00, 0x00, 0x02,  // Stream: 2
      0x80u, 0x00, 0x00, 0x01,  // Parent: 1 (Exclusive)
      0x10,                     // Weight: 17
  };
  QuicHttpFrameHeader header(5, QuicHttpFrameType::QUIC_HTTP_PRIORITY, 0, 2);
  QuicHttpFrameParts expected(header);
  expected.opt_priority = QuicHttpPriorityFields(1, 17, true);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, RstStream) {
  const char kFrameData[] = {
      0x00, 0x00, 0x04,        // Length: 4
      0x03,                    //   Type: RST_STREAM
      0x00,                    //  Flags: none
      0x00, 0x00, 0x00, 0x01,  // Stream: 1
      0x00, 0x00, 0x00, 0x01,  //  Error: PROTOCOL_ERROR
  };
  QuicHttpFrameHeader header(4, QuicHttpFrameType::RST_STREAM, 0, 1);
  QuicHttpFrameParts expected(header);
  expected.opt_rst_stream_error_code = QuicHttpErrorCode::PROTOCOL_ERROR;
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, SettingsEmpty) {
  const char kFrameData[] = {
      0x00, 0x00, 0x00,        // Length: 0
      0x04,                    //   Type: SETTINGS
      0x00,                    //  Flags: none
      0x00, 0x00, 0x00, 0x01,  // Stream: 1 (invalid but unchecked here)
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::SETTINGS, 0, 1);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, SettingsAck) {
  const char kFrameData[] = {
      0x00, 0x00, 0x00,        //   Length: 6
      0x04,                    //     Type: SETTINGS
      0x01,                    //    Flags: QUIC_HTTP_ACK
      0x00, 0x00, 0x00, 0x00,  //   Stream: 0
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::SETTINGS,
                             QuicHttpFrameFlag::QUIC_HTTP_ACK, 0);
  QuicHttpFrameParts expected(header);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, PushPromiseMinimal) {
  const char kFrameData[] = {
      0x00, 0x00, 0x04,        // Payload length: 4
      0x05,                    // PUSH_PROMISE
      0x04,                    // Flags: QUIC_HTTP_END_HEADERS
      0x00, 0x00, 0x00, 0x02,  //   Stream: 2 (invalid but unchecked here)
      0x00, 0x00, 0x00, 0x01,  // Promised: 1 (invalid but unchecked here)
  };
  QuicHttpFrameHeader header(4, QuicHttpFrameType::PUSH_PROMISE,
                             QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS, 2);
  QuicHttpFrameParts expected(header, "");
  expected.opt_push_promise = QuicHttpPushPromiseFields{1};
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, Ping) {
  const char kFrameData[] = {
      0x00,  0x00, 0x08,        //   Length: 8
      0x06,                     //     Type: PING
      0xfeu,                    //    Flags: no valid flags
      0x00,  0x00, 0x00, 0x00,  //   Stream: 0
      's',   'o',  'm',  'e',   // "some"
      'd',   'a',  't',  'a',   // "data"
  };
  QuicHttpFrameHeader header(8, QuicHttpFrameType::PING, 0, 0);
  QuicHttpFrameParts expected(header);
  expected.opt_ping =
      QuicHttpPingFields{{'s', 'o', 'm', 'e', 'd', 'a', 't', 'a'}};
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, PingAck) {
  const char kFrameData[] = {
      0x00,  0x00, 0x08,  //   Length: 8
      0x06,               //     Type: PING
      0xffu,              //    Flags: QUIC_HTTP_ACK (plus all invalid flags)
      0x00,  0x00, 0x00, 0x00,  //   Stream: 0
      's',   'o',  'm',  'e',   // "some"
      'd',   'a',  't',  'a',   // "data"
  };
  QuicHttpFrameHeader header(8, QuicHttpFrameType::PING,
                             QuicHttpFrameFlag::QUIC_HTTP_ACK, 0);
  QuicHttpFrameParts expected(header);
  expected.opt_ping =
      QuicHttpPingFields{{'s', 'o', 'm', 'e', 'd', 'a', 't', 'a'}};
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, GoAwayMinimal) {
  const char kFrameData[] = {
      0x00,  0x00, 0x08,         // Length: 8 (no opaque data)
      0x07,                      //   Type: GOAWAY
      0xffu,                     //  Flags: 0xff (no valid flags)
      0x00,  0x00, 0x00, 0x01,   // Stream: 1 (invalid but unchecked here)
      0x80u, 0x00, 0x00, 0xffu,  //   Last: 255 (plus R bit)
      0x00,  0x00, 0x00, 0x09,   //  Error: COMPRESSION_ERROR
  };
  QuicHttpFrameHeader header(8, QuicHttpFrameType::GOAWAY, 0, 1);
  QuicHttpFrameParts expected(header);
  expected.opt_goaway =
      QuicHttpGoAwayFields(255, QuicHttpErrorCode::COMPRESSION_ERROR);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, WindowUpdate) {
  const char kFrameData[] = {
      0x00,  0x00, 0x04,        // Length: 4
      0x08,                     //   Type: WINDOW_UPDATE
      0x0f,                     //  Flags: 0xff (no valid flags)
      0x00,  0x00, 0x00, 0x01,  // Stream: 1
      0x80u, 0x00, 0x04, 0x00,  //   Incr: 1024 (plus R bit)
  };
  QuicHttpFrameHeader header(4, QuicHttpFrameType::WINDOW_UPDATE, 0, 1);
  QuicHttpFrameParts expected(header);
  expected.opt_window_update_increment = 1024;
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, ContinuationEmpty) {
  const char kFrameData[] = {
      0x00, 0x00, 0x00,        // Payload length: 0
      0x09,                    // CONTINUATION
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x00,  // Stream ID: 0 (invalid but unchecked here)
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::CONTINUATION, 0, 0);
  QuicHttpFrameParts expected(header);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, AltSvcMinimal) {
  const char kFrameData[] = {
      0x00,  0x00, 0x02,        // Payload length: 2
      0x0a,                     // ALTSVC
      0xffu,                    // Flags: none (plus 0xff)
      0x00,  0x00, 0x00, 0x00,  // Stream ID: 0 (invalid but unchecked here)
      0x00,  0x00,              // Origin Length: 0
  };
  QuicHttpFrameHeader header(2, QuicHttpFrameType::ALTSVC, 0, 0);
  QuicHttpFrameParts expected(header);
  expected.opt_altsvc_origin_length = 0;
  expected.opt_altsvc_value_length = 0;
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, UnknownEmpty) {
  const char kFrameData[] = {
      0x00,  0x00, 0x00,        // Payload length: 0
      0x20,                     // 32 (unknown)
      0xffu,                    // Flags: all
      0x00,  0x00, 0x00, 0x00,  // Stream ID: 0
  };
  QuicHttpFrameHeader header(0, static_cast<QuicHttpFrameType>(32), 0xff, 0);
  QuicHttpFrameParts expected(header);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

////////////////////////////////////////////////////////////////////////////////
// Tests of longer payloads, for those frame types that allow longer payloads.

TEST_F(QuicHttpFrameDecoderTest, DataPayload) {
  const char kFrameData[] = {
      0x00,  0x00, 0x03,        // Payload length: 7
      0x00,                     // DATA
      0x80u,                    // Flags: 0x80
      0x00,  0x00, 0x02, 0x02,  // Stream ID: 514
      'a',   'b',  'c',         // Data
  };
  QuicHttpFrameHeader header(3, QuicHttpFrameType::DATA, 0, 514);
  QuicHttpFrameParts expected(header, "abc");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeadersPayload) {
  const char kFrameData[] = {
      0x00, 0x00, 0x03,  // Payload length: 3
      0x01,              // HEADERS
      0x05,              // Flags: QUIC_HTTP_END_STREAM | QUIC_HTTP_END_HEADERS
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 0  (REQUIRES ID)
      'a',  'b',  'c',  // HPQUIC_HTTP_ACK fragment (doesn't have to be valid)
  };
  QuicHttpFrameHeader header(3, QuicHttpFrameType::HEADERS,
                             QuicHttpFrameFlag::QUIC_HTTP_END_STREAM |
                                 QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS,
                             2);
  QuicHttpFrameParts expected(header, "abc");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeadersPriority) {
  const char kFrameData[] = {
      0x00,  0x00, 0x05,        // Payload length: 5
      0x01,                     // HEADERS
      0x20,                     // Flags: QUIC_HTTP_PRIORITY
      0x00,  0x00, 0x00, 0x02,  // Stream ID: 0  (REQUIRES ID)
      0x00,  0x00, 0x00, 0x01,  // Parent: 1 (Not Exclusive)
      0xffu,                    // Weight: 256
  };
  QuicHttpFrameHeader header(5, QuicHttpFrameType::HEADERS,
                             QuicHttpFrameFlag::QUIC_HTTP_PRIORITY, 2);
  QuicHttpFrameParts expected(header);
  expected.opt_priority = QuicHttpPriorityFields(1, 256, false);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, Settings) {
  const char kFrameData[] = {
      0x00, 0x00, 0x0c,        // Length: 12
      0x04,                    //   Type: SETTINGS
      0x00,                    //  Flags: none
      0x00, 0x00, 0x00, 0x00,  // Stream: 0
      0x00, 0x04,              //  Param: INITIAL_WINDOW_SIZE
      0x0a, 0x0b, 0x0c, 0x0d,  //  Value: 168496141
      0x00, 0x02,              //  Param: ENABLE_PUSH
      0x00, 0x00, 0x00, 0x03,  //  Value: 3 (invalid but unchecked here)
  };
  QuicHttpFrameHeader header(12, QuicHttpFrameType::SETTINGS, 0, 0);
  QuicHttpFrameParts expected(header);
  expected.settings.push_back(QuicHttpSettingFields(
      QuicHttpSettingsParameter::INITIAL_WINDOW_SIZE, 168496141));
  expected.settings.push_back(
      QuicHttpSettingFields(QuicHttpSettingsParameter::ENABLE_PUSH, 3));
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, PushPromisePayload) {
  const char kFrameData[] = {
      0x00, 0x00, 7,            // Payload length: 7
      0x05,                     // PUSH_PROMISE
      0x04,                     // Flags: QUIC_HTTP_END_HEADERS
      0x00, 0x00, 0x00, 0xffu,  // Stream ID: 255
      0x00, 0x00, 0x01, 0x00,   // Promised: 256
      'a',  'b',  'c',  // HPQUIC_HTTP_ACK fragment (doesn't have to be valid)
  };
  QuicHttpFrameHeader header(7, QuicHttpFrameType::PUSH_PROMISE,
                             QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS, 255);
  QuicHttpFrameParts expected(header, "abc");
  expected.opt_push_promise = QuicHttpPushPromiseFields{256};
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, GoAwayOpaqueData) {
  const char kFrameData[] = {
      0x00,  0x00, 0x0e,        // Length: 14
      0x07,                     //   Type: GOAWAY
      0xffu,                    //  Flags: 0xff (no valid flags)
      0x80u, 0x00, 0x00, 0x00,  // Stream: 0 (plus R bit)
      0x00,  0x00, 0x01, 0x00,  //   Last: 256
      0x00,  0x00, 0x00, 0x03,  //  Error: FLOW_CONTROL_ERROR
      'o',   'p',  'a',  'q',  'u', 'e',
  };
  QuicHttpFrameHeader header(14, QuicHttpFrameType::GOAWAY, 0, 0);
  QuicHttpFrameParts expected(header, "opaque");
  expected.opt_goaway =
      QuicHttpGoAwayFields(256, QuicHttpErrorCode::FLOW_CONTROL_ERROR);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, ContinuationPayload) {
  const char kFrameData[] = {
      0x00,  0x00, 0x03,        // Payload length: 3
      0x09,                     // CONTINUATION
      0xffu,                    // Flags: QUIC_HTTP_END_HEADERS | 0xfb
      0x00,  0x00, 0x00, 0x02,  // Stream ID: 2
      'a',   'b',  'c',         // Data
  };
  QuicHttpFrameHeader header(3, QuicHttpFrameType::CONTINUATION,
                             QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS, 2);
  QuicHttpFrameParts expected(header, "abc");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, AltSvcPayload) {
  const char kFrameData[] = {
      0x00, 0x00, 0x08,        // Payload length: 3
      0x0a,                    // ALTSVC
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 2
      0x00, 0x03,              // Origin Length: 0
      'a',  'b',  'c',         // Origin
      'd',  'e',  'f',         // Value
  };
  QuicHttpFrameHeader header(8, QuicHttpFrameType::ALTSVC, 0, 2);
  QuicHttpFrameParts expected(header);
  expected.SetAltSvcExpected("abc", "def");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, UnknownPayload) {
  const char kFrameData[] = {
      0x00, 0x00, 0x03,        // Payload length: 3
      0x30,                    // 48 (unknown)
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 2
      'a',  'b',  'c',         // Payload
  };
  QuicHttpFrameHeader header(3, static_cast<QuicHttpFrameType>(48), 0, 2);
  QuicHttpFrameParts expected(header, "abc");
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

////////////////////////////////////////////////////////////////////////////////
// Tests of padded payloads, for those frame types that allow padding.

TEST_F(QuicHttpFrameDecoderTest, DataPayloadAndPadding) {
  const char kFrameData[] = {
      0x00, 0x00, 0x07,        // Payload length: 7
      0x00,                    // DATA
      0x09,                    // Flags: QUIC_HTTP_END_STREAM | QUIC_HTTP_PADDED
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 0  (REQUIRES ID)
      0x03,                    // Pad Len
      'a',  'b',  'c',         // Data
      0x00, 0x00, 0x00,        // Padding
  };
  QuicHttpFrameHeader header(7, QuicHttpFrameType::DATA,
                             QuicHttpFrameFlag::QUIC_HTTP_END_STREAM |
                                 QuicHttpFrameFlag::QUIC_HTTP_PADDED,
                             2);
  size_t total_pad_length = 4;  // Including the Pad Length field.
  QuicHttpFrameParts expected(header, "abc", total_pad_length);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeadersPayloadAndPadding) {
  const char kFrameData[] = {
      0x00, 0x00, 0x07,        // Payload length: 7
      0x01,                    // HEADERS
      0x08,                    // Flags: QUIC_HTTP_PADDED
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 0  (REQUIRES ID)
      0x03,                    // Pad Len
      'a',  'b',  'c',   // HPQUIC_HTTP_ACK fragment (doesn't have to be valid)
      0x00, 0x00, 0x00,  // Padding
  };
  QuicHttpFrameHeader header(7, QuicHttpFrameType::HEADERS,
                             QuicHttpFrameFlag::QUIC_HTTP_PADDED, 2);
  size_t total_pad_length = 4;  // Including the Pad Length field.
  QuicHttpFrameParts expected(header, "abc", total_pad_length);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeadersPayloadPriorityAndPadding) {
  const char kFrameData[] = {
      0x00,  0x00, 0x0c,        // Payload length: 12
      0x01,                     // HEADERS
      0xffu,                    // Flags: all, including undefined
      0x00,  0x00, 0x00, 0x02,  // Stream ID: 0  (REQUIRES ID)
      0x03,                     // Pad Len
      0x80u, 0x00, 0x00, 0x01,  // Parent: 1 (Exclusive)
      0x10,                     // Weight: 17
      'a',   'b',  'c',   // HPQUIC_HTTP_ACK fragment (doesn't have to be valid)
      0x00,  0x00, 0x00,  // Padding
  };
  QuicHttpFrameHeader header(12, QuicHttpFrameType::HEADERS,
                             QuicHttpFrameFlag::QUIC_HTTP_END_STREAM |
                                 QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS |
                                 QuicHttpFrameFlag::QUIC_HTTP_PADDED |
                                 QuicHttpFrameFlag::QUIC_HTTP_PRIORITY,
                             2);
  size_t total_pad_length = 4;  // Including the Pad Length field.
  QuicHttpFrameParts expected(header, "abc", total_pad_length);
  expected.opt_priority = QuicHttpPriorityFields(1, 17, true);
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, PushPromisePayloadAndPadding) {
  const char kFrameData[] = {
      0x00,  0x00, 11,  // Payload length: 11
      0x05,             // PUSH_PROMISE
      0xffu,  // Flags: QUIC_HTTP_END_HEADERS | QUIC_HTTP_PADDED | 0xf3
      0x00,  0x00, 0x00, 0x01,  // Stream ID: 1
      0x03,                     // Pad Len
      0x00,  0x00, 0x00, 0x02,  // Promised: 2
      'a',   'b',  'c',   // HPQUIC_HTTP_ACK fragment (doesn't have to be valid)
      0x00,  0x00, 0x00,  // Padding
  };
  QuicHttpFrameHeader header(11, QuicHttpFrameType::PUSH_PROMISE,
                             QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS |
                                 QuicHttpFrameFlag::QUIC_HTTP_PADDED,
                             1);
  size_t total_pad_length = 4;  // Including the Pad Length field.
  QuicHttpFrameParts expected(header, "abc", total_pad_length);
  expected.opt_push_promise = QuicHttpPushPromiseFields{2};
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(kFrameData, expected));
}

////////////////////////////////////////////////////////////////////////////////
// Payload too short errors.

TEST_F(QuicHttpFrameDecoderTest, DataMissingPadLengthField) {
  const char kFrameData[] = {
      0x00, 0x00, 0x00,        // Payload length: 0
      0x00,                    // DATA
      0x08,                    // Flags: QUIC_HTTP_PADDED
      0x00, 0x00, 0x00, 0x01,  // Stream ID: 1
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::DATA,
                             QuicHttpFrameFlag::QUIC_HTTP_PADDED, 1);
  QuicHttpFrameParts expected(header);
  expected.opt_missing_length = 1;
  EXPECT_TRUE(DecodePayloadExpectingError(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeaderPaddingTooLong) {
  const char kFrameData[] = {
      0x00,  0x00, 0x02,        // Payload length: 0
      0x01,                     // HEADERS
      0x08,                     // Flags: QUIC_HTTP_PADDED
      0x00,  0x01, 0x00, 0x00,  // Stream ID: 65536
      0xffu,                    // Pad Len: 255
      0x00,                     // Only one byte of padding
  };
  QuicHttpFrameHeader header(2, QuicHttpFrameType::HEADERS,
                             QuicHttpFrameFlag::QUIC_HTTP_PADDED, 65536);
  QuicHttpFrameParts expected(header);
  expected.opt_missing_length = 254;
  EXPECT_TRUE(DecodePayloadExpectingError(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, HeaderMissingPriority) {
  const char kFrameData[] = {
      0x00, 0x00, 0x04,        // Payload length: 0
      0x01,                    // HEADERS
      0x20,                    // Flags: QUIC_HTTP_PRIORITY
      0x00, 0x01, 0x00, 0x00,  // Stream ID: 65536
      0x00, 0x00, 0x00, 0x00,  // Priority (truncated)
  };
  QuicHttpFrameHeader header(4, QuicHttpFrameType::HEADERS,
                             QuicHttpFrameFlag::QUIC_HTTP_PRIORITY, 65536);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, PriorityTooShort) {
  const char kFrameData[] = {
      0x00,  0x00, 0x04,        // Length: 5
      0x02,                     //   Type: QUIC_HTTP_PRIORITY
      0x00,                     //  Flags: none
      0x00,  0x00, 0x00, 0x02,  // Stream: 2
      0x80u, 0x00, 0x00, 0x01,  // Parent: 1 (Exclusive)
  };
  QuicHttpFrameHeader header(4, QuicHttpFrameType::QUIC_HTTP_PRIORITY, 0, 2);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, RstStreamTooShort) {
  const char kFrameData[] = {
      0x00, 0x00, 0x03,        // Length: 4
      0x03,                    //   Type: RST_STREAM
      0x00,                    //  Flags: none
      0x00, 0x00, 0x00, 0x01,  // Stream: 1
      0x00, 0x00, 0x00,        //  Truncated
  };
  QuicHttpFrameHeader header(3, QuicHttpFrameType::RST_STREAM, 0, 1);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

// SETTINGS frames must a multiple of 6 bytes long, so an 9 byte payload is
// invalid.
TEST_F(QuicHttpFrameDecoderTest, SettingsWrongSize) {
  const char kFrameData[] = {
      0x00, 0x00, 0x09,        // Length: 2
      0x04,                    //   Type: SETTINGS
      0x00,                    //  Flags: none
      0x00, 0x00, 0x00, 0x00,  // Stream: 0
      0x00, 0x02,              //  Param: ENABLE_PUSH
      0x00, 0x00, 0x00, 0x03,  //  Value: 1
      0x00, 0x04,              //  Param: INITIAL_WINDOW_SIZE
      0x00,                    //  Value: Truncated
  };
  QuicHttpFrameHeader header(9, QuicHttpFrameType::SETTINGS, 0, 0);
  QuicHttpFrameParts expected(header);
  expected.settings.push_back(
      QuicHttpSettingFields(QuicHttpSettingsParameter::ENABLE_PUSH, 3));
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, expected));
}

TEST_F(QuicHttpFrameDecoderTest, PushPromiseTooShort) {
  const char kFrameData[] = {
      0x00, 0x00, 3,           // Payload length: 3
      0x05,                    // PUSH_PROMISE
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x01,  // Stream ID: 1
      0x00, 0x00, 0x00,        // Truncated promise id
  };
  QuicHttpFrameHeader header(3, QuicHttpFrameType::PUSH_PROMISE, 0, 1);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, PushPromisePaddedTruncatedPromise) {
  const char kFrameData[] = {
      0x00, 0x00, 4,           // Payload length: 4
      0x05,                    // PUSH_PROMISE
      0x08,                    // Flags: QUIC_HTTP_PADDED
      0x00, 0x00, 0x00, 0x01,  // Stream ID: 1
      0x00,                    // Pad Len
      0x00, 0x00, 0x00,        // Truncated promise id
  };
  QuicHttpFrameHeader header(4, QuicHttpFrameType::PUSH_PROMISE,
                             QuicHttpFrameFlag::QUIC_HTTP_PADDED, 1);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, PingTooShort) {
  const char kFrameData[] = {
      0x00,  0x00, 0x07,        //   Length: 8
      0x06,                     //     Type: PING
      0xfeu,                    //    Flags: no valid flags
      0x00,  0x00, 0x00, 0x00,  //   Stream: 0
      's',   'o',  'm',  'e',   // "some"
      'd',   'a',  't',         // Too little
  };
  QuicHttpFrameHeader header(7, QuicHttpFrameType::PING, 0, 0);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, GoAwayTooShort) {
  const char kFrameData[] = {
      0x00,  0x00, 0x00,        // Length: 0
      0x07,                     //   Type: GOAWAY
      0xffu,                    //  Flags: 0xff (no valid flags)
      0x00,  0x00, 0x00, 0x00,  // Stream: 0
  };
  QuicHttpFrameHeader header(0, QuicHttpFrameType::GOAWAY, 0, 0);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, WindowUpdateTooShort) {
  const char kFrameData[] = {
      0x00,  0x00, 0x03,        // Length: 3
      0x08,                     //   Type: WINDOW_UPDATE
      0x0f,                     //  Flags: 0xff (no valid flags)
      0x00,  0x00, 0x00, 0x01,  // Stream: 1
      0x80u, 0x00, 0x04,        // Truncated
  };
  QuicHttpFrameHeader header(3, QuicHttpFrameType::WINDOW_UPDATE, 0, 1);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, AltSvcTruncatedOriginLength) {
  const char kFrameData[] = {
      0x00, 0x00, 0x01,        // Payload length: 3
      0x0a,                    // ALTSVC
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 2
      0x00,                    // Origin Length: truncated
  };
  QuicHttpFrameHeader header(1, QuicHttpFrameType::ALTSVC, 0, 2);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, AltSvcTruncatedOrigin) {
  const char kFrameData[] = {
      0x00, 0x00, 0x05,        // Payload length: 3
      0x0a,                    // ALTSVC
      0x00,                    // Flags: none
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 2
      0x00, 0x04,              // Origin Length: 4 (too long)
      'a',  'b',  'c',         // Origin
  };
  QuicHttpFrameHeader header(5, QuicHttpFrameType::ALTSVC, 0, 2);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

////////////////////////////////////////////////////////////////////////////////
// Payload too long errors.

// The decoder calls the listener's OnFrameSizeError method if the frame's
// payload is longer than the currently configured maximum payload size.
TEST_F(QuicHttpFrameDecoderTest, BeyondMaximum) {
  decoder_.set_maximum_payload_size(2);
  const char kFrameData[] = {
      0x00, 0x00, 0x07,        // Payload length: 7
      0x00,                    // DATA
      0x09,                    // Flags: QUIC_HTTP_END_STREAM | QUIC_HTTP_PADDED
      0x00, 0x00, 0x00, 0x02,  // Stream ID: 0  (REQUIRES ID)
      0x03,                    // Pad Len
      'a',  'b',  'c',         // Data
      0x00, 0x00, 0x00,        // Padding
  };
  QuicHttpFrameHeader header(7, QuicHttpFrameType::DATA,
                             QuicHttpFrameFlag::QUIC_HTTP_END_STREAM |
                                 QuicHttpFrameFlag::QUIC_HTTP_PADDED,
                             2);
  QuicHttpFrameParts expected(header);
  expected.has_frame_size_error = true;
  auto validator = [&expected, this](
                       const QuicHttpDecodeBuffer& input,
                       QuicHttpDecodeStatus status) -> AssertionResult {
    VERIFY_EQ(status, QuicHttpDecodeStatus::kDecodeError);
    // The decoder detects this error after decoding the header, and without
    // trying to decode the payload.
    VERIFY_EQ(input.Offset(), QuicHttpFrameHeader::EncodedSize());
    VERIFY_AND_RETURN_SUCCESS(VerifyCollected(expected));
  };
  ResetDecodeSpeedCounters();
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(ToStringPiece(kFrameData),
                                                  validator));
  EXPECT_GT(fast_decode_count_, 0u);
  EXPECT_GT(slow_decode_count_, 0u);
}

TEST_F(QuicHttpFrameDecoderTest, PriorityTooLong) {
  const char kFrameData[] = {
      0x00,  0x00, 0x06,        // Length: 5
      0x02,                     //   Type: QUIC_HTTP_PRIORITY
      0x00,                     //  Flags: none
      0x00,  0x00, 0x00, 0x02,  // Stream: 2
      0x80u, 0x00, 0x00, 0x01,  // Parent: 1 (Exclusive)
      0x10,                     // Weight: 17
      0x00,                     // Too much
  };
  QuicHttpFrameHeader header(6, QuicHttpFrameType::QUIC_HTTP_PRIORITY, 0, 2);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, RstStreamTooLong) {
  const char kFrameData[] = {
      0x00, 0x00, 0x05,        // Length: 4
      0x03,                    //   Type: RST_STREAM
      0x00,                    //  Flags: none
      0x00, 0x00, 0x00, 0x01,  // Stream: 1
      0x00, 0x00, 0x00, 0x01,  //  Error: PROTOCOL_ERROR
      0x00,                    // Too much
  };
  QuicHttpFrameHeader header(5, QuicHttpFrameType::RST_STREAM, 0, 1);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, SettingsAckTooLong) {
  const char kFrameData[] = {
      0x00, 0x00, 0x06,        //   Length: 6
      0x04,                    //     Type: SETTINGS
      0x01,                    //    Flags: QUIC_HTTP_ACK
      0x00, 0x00, 0x00, 0x00,  //   Stream: 0
      0x00, 0x00,              //   Extra
      0x00, 0x00, 0x00, 0x00,  //   Extra
  };
  QuicHttpFrameHeader header(6, QuicHttpFrameType::SETTINGS,
                             QuicHttpFrameFlag::QUIC_HTTP_ACK, 0);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, PingAckTooLong) {
  const char kFrameData[] = {
      0x00,  0x00, 0x09,        //   Length: 8
      0x06,                     //     Type: PING
      0xffu,                    //    Flags: QUIC_HTTP_ACK | 0xfe
      0x00,  0x00, 0x00, 0x00,  //   Stream: 0
      's',   'o',  'm',  'e',   // "some"
      'd',   'a',  't',  'a',   // "data"
      0x00,                     // Too much
  };
  QuicHttpFrameHeader header(9, QuicHttpFrameType::PING,
                             QuicHttpFrameFlag::QUIC_HTTP_ACK, 0);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

TEST_F(QuicHttpFrameDecoderTest, WindowUpdateTooLong) {
  const char kFrameData[] = {
      0x00,  0x00, 0x05,        // Length: 5
      0x08,                     //   Type: WINDOW_UPDATE
      0x0f,                     //  Flags: 0xff (no valid flags)
      0x00,  0x00, 0x00, 0x01,  // Stream: 1
      0x80u, 0x00, 0x04, 0x00,  //   Incr: 1024 (plus R bit)
      0x00,                     // Too much
  };
  QuicHttpFrameHeader header(5, QuicHttpFrameType::WINDOW_UPDATE, 0, 1);
  EXPECT_TRUE(DecodePayloadExpectingFrameSizeError(kFrameData, header));
}

}  // namespace
}  // namespace test
}  // namespace net
