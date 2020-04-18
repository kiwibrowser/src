// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_settings_payload_decoder.h"

#include <stddef.h>

#include <cstdint>
#include <vector>

#include "base/logging.h"
#include "net/third_party/quic/http/decoder/payload_decoders/quic_http_payload_decoder_base_test_util.h"
#include "net/third_party/quic/http/decoder/quic_http_frame_decoder_listener.h"
#include "net/third_party/quic/http/quic_http_constants.h"
#include "net/third_party/quic/http/quic_http_constants_test_util.h"
#include "net/third_party/quic/http/quic_http_structures_test_util.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts.h"
#include "net/third_party/quic/http/test_tools/quic_http_frame_parts_collector.h"
#include "net/third_party/quic/http/tools/quic_http_frame_builder.h"
#include "net/third_party/quic/http/tools/quic_http_random_decoder_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

class QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderPeer {
 public:
  static constexpr QuicHttpFrameType FrameType() {
    return QuicHttpFrameType::SETTINGS;
  }

  // Returns the mask of flags that affect the decoding of the payload (i.e.
  // flags that that indicate the presence of certain fields or padding).
  static constexpr uint8_t FlagsAffectingPayloadDecoding() {
    return QuicHttpFrameFlag::QUIC_HTTP_ACK;
  }

  static void Randomize(QuicHttpQuicHttpSettingsQuicHttpPayloadDecoder* p,
                        QuicTestRandomBase* rng) {
    VLOG(1) << "QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderPeer::Randomize";
    test::Randomize(&p->setting_fields_, rng);
  }
};

namespace {

struct Listener : public QuicHttpFramePartsCollector {
  void OnSettingsStart(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnSettingsStart: " << header;
    EXPECT_EQ(QuicHttpFrameType::SETTINGS, header.type) << header;
    EXPECT_EQ(QuicHttpFrameFlag(), header.flags) << header;
    StartFrame(header)->OnSettingsStart(header);
  }

  void OnSetting(const QuicHttpSettingFields& setting_fields) override {
    VLOG(1) << "QuicHttpSettingFields: setting_fields=" << setting_fields;
    CurrentFrame()->OnSetting(setting_fields);
  }

  void OnSettingsEnd() override {
    VLOG(1) << "OnSettingsEnd";
    EndFrame()->OnSettingsEnd();
  }

  void OnSettingsAck(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnSettingsAck: " << header;
    StartAndEndFrame(header)->OnSettingsAck(header);
  }

  void OnFrameSizeError(const QuicHttpFrameHeader& header) override {
    VLOG(1) << "OnFrameSizeError: " << header;
    FrameError(header)->OnFrameSizeError(header);
  }
};

class QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderTest
    : public AbstractQuicHttpPayloadDecoderTest<
          QuicHttpQuicHttpSettingsQuicHttpPayloadDecoder,
          QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderPeer,
          Listener> {
 protected:
  QuicHttpSettingFields RandSettingsFields() {
    QuicHttpSettingFields fields;
    test::Randomize(&fields, RandomPtr());
    return fields;
  }
};

// Confirm we get an error if the SETTINGS payload is not the correct size
// to hold exactly zero or more whole QuicHttpSettingFields.
TEST_F(QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderTest, SettingsWrongSize) {
  auto approve_size = [](size_t size) {
    // Should get an error if size is not an integral multiple of the size
    // of one setting.
    return 0 != (size % QuicHttpSettingFields::EncodedSize());
  };
  QuicHttpFrameBuilder fb;
  fb.Append(RandSettingsFields());
  fb.Append(RandSettingsFields());
  fb.Append(RandSettingsFields());
  EXPECT_TRUE(VerifyDetectsFrameSizeError(0, fb.buffer(), approve_size));
}

// Confirm we get an error if the SETTINGS QUIC_HTTP_ACK payload is not empty.
TEST_F(QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderTest,
       SettingsAkcWrongSize) {
  auto approve_size = [](size_t size) { return size != 0; };
  QuicHttpFrameBuilder fb;
  fb.Append(RandSettingsFields());
  fb.Append(RandSettingsFields());
  fb.Append(RandSettingsFields());
  EXPECT_TRUE(VerifyDetectsFrameSizeError(QuicHttpFrameFlag::QUIC_HTTP_ACK,
                                          fb.buffer(), approve_size));
}

// SETTINGS must have stream_id==0, but the payload decoder doesn't check that.
TEST_F(QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderTest, SettingsAck) {
  for (int stream_id = 0; stream_id < 3; ++stream_id) {
    QuicHttpFrameHeader header(0, QuicHttpFrameType::SETTINGS,
                               RandFlags() | QuicHttpFrameFlag::QUIC_HTTP_ACK,
                               stream_id);
    set_frame_header(header);
    QuicHttpFrameParts expected(header);
    EXPECT_TRUE(DecodePayloadAndValidateSeveralWays("", expected));
  }
}

// Try several values of each known SETTINGS parameter.
TEST_F(QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderTest, OneRealSetting) {
  std::vector<uint32_t> values = {0, 1, 0xffffffff, Random().Rand32()};
  for (auto param : AllQuicHttpSettingsParameters()) {
    for (uint32_t value : values) {
      QuicHttpSettingFields fields(param, value);
      QuicHttpFrameBuilder fb;
      fb.Append(fields);
      QuicHttpFrameHeader header(fb.size(), QuicHttpFrameType::SETTINGS,
                                 RandFlags(), RandStreamId());
      set_frame_header(header);
      QuicHttpFrameParts expected(header);
      expected.settings.push_back(fields);
      EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
    }
  }
}

// Decode a SETTINGS frame with lots of fields.
TEST_F(QuicHttpQuicHttpSettingsQuicHttpPayloadDecoderTest, ManySettings) {
  const size_t num_settings = 100;
  const size_t size = QuicHttpSettingFields::EncodedSize() * num_settings;
  QuicHttpFrameHeader header(
      size, QuicHttpFrameType::SETTINGS,
      RandFlags(),  // & ~QuicHttpFrameFlag::QUIC_HTTP_ACK,
      RandStreamId());
  set_frame_header(header);
  QuicHttpFrameParts expected(header);
  QuicHttpFrameBuilder fb;
  for (size_t n = 0; n < num_settings; ++n) {
    QuicHttpSettingFields fields(static_cast<QuicHttpSettingsParameter>(n),
                                 Random().Rand32());
    fb.Append(fields);
    expected.settings.push_back(fields);
  }
  ASSERT_EQ(size, fb.size());
  EXPECT_TRUE(DecodePayloadAndValidateSeveralWays(fb.buffer(), expected));
}

}  // namespace
}  // namespace test
}  // namespace net
