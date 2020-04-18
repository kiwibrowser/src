// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/third_party/quic/http/quic_http_structures.h"

// Tests are focused on QuicHttpFrameHeader because it has by far the most
// methods of any of the structures.
// Note that EXPECT.*DEATH tests are slow (a fork is probably involved).

// And in case you're wondering, yes, these are ridiculously thorough tests,
// but believe it or not, I've found stupid bugs this way.

#include <memory>
#include <ostream>  // IWYU pragma: keep  // for stringstream
#include <type_traits>
#include <vector>

#include "net/third_party/quic/http/quic_http_structures_test_util.h"
#include "net/third_party/quic/platform/api/quic_str_cat.h"
#include "net/third_party/quic/platform/api/quic_string_utils.h"
#include "net/third_party/quic/platform/api/quic_test_random.h"
#include "net/third_party/quic/platform/api/quic_text_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::AssertionResult;
using ::testing::Combine;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Values;
using ::testing::ValuesIn;

namespace net {
namespace test {
namespace {

template <typename E>
E IncrementEnum(E e) {
  typedef typename std::underlying_type<E>::type I;
  return static_cast<E>(1 + static_cast<I>(e));
}

#if GTEST_HAS_DEATH_TEST && !defined(NDEBUG)
std::vector<QuicHttpFrameType> ValidFrameTypes() {
  std::vector<QuicHttpFrameType> valid_types{QuicHttpFrameType::DATA};
  while (valid_types.back() != QuicHttpFrameType::ALTSVC) {
    valid_types.push_back(IncrementEnum(valid_types.back()));
  }
  return valid_types;
}
#endif  // GTEST_HAS_DEATH_TEST && !defined(NDEBUG)

TEST(QuicHttpFrameHeaderTest, Constructor) {
  QuicTestRandom random;
  uint8_t frame_type = 0;
  do {
    // Only the payload length is DCHECK'd in the constructor, so we need to
    // make sure it is a "uint24".
    uint32_t payload_length = random.Rand32() & 0xffffff;
    QuicHttpFrameType type = static_cast<QuicHttpFrameType>(frame_type);
    uint8_t flags = random.Rand8();
    uint32_t stream_id = random.Rand32();

    QuicHttpFrameHeader v(payload_length, type, flags, stream_id);

    EXPECT_EQ(payload_length, v.payload_length);
    EXPECT_EQ(type, v.type);
    EXPECT_EQ(flags, v.flags);
    EXPECT_EQ(stream_id, v.stream_id);
  } while (frame_type++ == 255);

#if GTEST_HAS_DEATH_TEST && !defined(NDEBUG)
  EXPECT_DEBUG_DEATH(
      QuicHttpFrameHeader(0x01000000, QuicHttpFrameType::DATA, 0, 1),
      "payload_length");
#endif  // GTEST_HAS_DEATH_TEST && !defined(NDEBUG)
}

TEST(QuicHttpFrameHeaderTest, Eq) {
  QuicTestRandom random;
  uint32_t payload_length = random.Rand32() & 0xffffff;
  QuicHttpFrameType type = static_cast<QuicHttpFrameType>(random.Rand8());

  uint8_t flags = random.Rand8();
  uint32_t stream_id = random.Rand32();

  QuicHttpFrameHeader v(payload_length, type, flags, stream_id);

  EXPECT_EQ(payload_length, v.payload_length);
  EXPECT_EQ(type, v.type);
  EXPECT_EQ(flags, v.flags);
  EXPECT_EQ(stream_id, v.stream_id);

  QuicHttpFrameHeader u(0, type, ~flags, stream_id);

  EXPECT_NE(u, v);
  EXPECT_NE(v, u);
  EXPECT_FALSE(u == v);
  EXPECT_FALSE(v == u);
  EXPECT_TRUE(u != v);
  EXPECT_TRUE(v != u);

  u = v;

  EXPECT_EQ(u, v);
  EXPECT_EQ(v, u);
  EXPECT_TRUE(u == v);
  EXPECT_TRUE(v == u);
  EXPECT_FALSE(u != v);
  EXPECT_FALSE(v != u);
}

#if GTEST_HAS_DEATH_TEST && !defined(NDEBUG)
// The tests of the valid frame types include EXPECT_DEBUG_DEATH, which is
// quite slow, so using value parameterized tests in order to allow sharding.
class QuicHttpFrameHeaderTypeAndFlagTest
    : public ::testing::TestWithParam<
          std::tuple<QuicHttpFrameType, QuicHttpFrameFlag>> {
 protected:
  QuicHttpFrameHeaderTypeAndFlagTest()
      : type_(std::get<0>(GetParam())), flags_(std::get<1>(GetParam())) {
    LOG(INFO) << "Frame type: " << type_;
    LOG(INFO) << "Frame flags: " << QuicHttpFrameFlagsToString(type_, flags_);
  }

  const QuicHttpFrameType type_;
  const QuicHttpFrameFlag flags_;
};

class QuicHttpIsEndStreamTest : public QuicHttpFrameHeaderTypeAndFlagTest {};
INSTANTIATE_TEST_CASE_P(IsEndStream,
                        QuicHttpIsEndStreamTest,
                        Combine(ValuesIn(ValidFrameTypes()),
                                Values(~QuicHttpFrameFlag::QUIC_HTTP_END_STREAM,
                                       0xff)));
TEST_P(QuicHttpIsEndStreamTest, IsEndStream) {
  const bool is_set = (flags_ & QuicHttpFrameFlag::QUIC_HTTP_END_STREAM) ==
                      QuicHttpFrameFlag::QUIC_HTTP_END_STREAM;
  QuicString flags_string;
  QuicHttpFrameHeader v(0, type_, flags_, 0);
  switch (type_) {
    case QuicHttpFrameType::DATA:
    case QuicHttpFrameType::HEADERS:
      EXPECT_EQ(is_set, v.IsEndStream()) << v;
      flags_string = v.FlagsToString();
      if (is_set) {
        EXPECT_THAT(QuicTextUtils::Split(flags_string, '|'),
                    Contains("QUIC_HTTP_END_STREAM"));
      } else {
        EXPECT_THAT(flags_string, Not(HasSubstr("QUIC_HTTP_END_STREAM")));
      }
      v.RetainFlags(QuicHttpFrameFlag::QUIC_HTTP_END_STREAM);
      EXPECT_EQ(is_set, v.IsEndStream()) << v;
      {
        std::stringstream s;
        s << v;
        EXPECT_EQ(v.ToString(), s.str());
        if (is_set) {
          EXPECT_THAT(s.str(), HasSubstr("flags=QUIC_HTTP_END_STREAM,"));
        } else {
          EXPECT_THAT(s.str(), HasSubstr("flags=,"));
        }
      }
      break;
    default:
      EXPECT_DEBUG_DEATH(v.IsEndStream(), "DATA.*HEADERS") << v;
  }
}

class IsQUIC_HTTP_ACKTest : public QuicHttpFrameHeaderTypeAndFlagTest {};
INSTANTIATE_TEST_CASE_P(IsAck,
                        IsQUIC_HTTP_ACKTest,
                        Combine(ValuesIn(ValidFrameTypes()),
                                Values(~QuicHttpFrameFlag::QUIC_HTTP_ACK,
                                       0xff)));
TEST_P(IsQUIC_HTTP_ACKTest, IsAck) {
  const bool is_set = (flags_ & QuicHttpFrameFlag::QUIC_HTTP_ACK) ==
                      QuicHttpFrameFlag::QUIC_HTTP_ACK;
  QuicString flags_string;
  QuicHttpFrameHeader v(0, type_, flags_, 0);
  switch (type_) {
    case QuicHttpFrameType::SETTINGS:
    case QuicHttpFrameType::PING:
      EXPECT_EQ(is_set, v.IsAck()) << v;
      flags_string = v.FlagsToString();
      if (is_set) {
        EXPECT_THAT(QuicTextUtils::Split(flags_string, '|'),
                    Contains("QUIC_HTTP_ACK"));
      } else {
        EXPECT_THAT(flags_string, Not(HasSubstr("QUIC_HTTP_ACK")));
      }
      v.RetainFlags(QuicHttpFrameFlag::QUIC_HTTP_ACK);
      EXPECT_EQ(is_set, v.IsAck()) << v;
      {
        std::stringstream s;
        s << v;
        EXPECT_EQ(v.ToString(), s.str());
        if (is_set) {
          EXPECT_THAT(s.str(), HasSubstr("flags=QUIC_HTTP_ACK,"));
        } else {
          EXPECT_THAT(s.str(), HasSubstr("flags=,"));
        }
      }
      break;
    default:
      EXPECT_DEBUG_DEATH(v.IsAck(), "SETTINGS.*PING") << v;
  }
}

class QuicHttpIsEndHeadersTest : public QuicHttpFrameHeaderTypeAndFlagTest {};
INSTANTIATE_TEST_CASE_P(
    IsEndHeaders,
    QuicHttpIsEndHeadersTest,
    Combine(ValuesIn(ValidFrameTypes()),
            Values(~QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS, 0xff)));
TEST_P(QuicHttpIsEndHeadersTest, IsEndHeaders) {
  const bool is_set = (flags_ & QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS) ==
                      QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS;
  QuicString flags_string;
  QuicHttpFrameHeader v(0, type_, flags_, 0);
  switch (type_) {
    case QuicHttpFrameType::HEADERS:
    case QuicHttpFrameType::PUSH_PROMISE:
    case QuicHttpFrameType::CONTINUATION:
      EXPECT_EQ(is_set, v.IsEndHeaders()) << v;
      flags_string = v.FlagsToString();
      if (is_set) {
        EXPECT_THAT(QuicTextUtils::Split(flags_string, '|'),
                    Contains("QUIC_HTTP_END_HEADERS"));
      } else {
        EXPECT_THAT(flags_string, Not(HasSubstr("QUIC_HTTP_END_HEADERS")));
      }
      v.RetainFlags(QuicHttpFrameFlag::QUIC_HTTP_END_HEADERS);
      EXPECT_EQ(is_set, v.IsEndHeaders()) << v;
      {
        std::stringstream s;
        s << v;
        EXPECT_EQ(v.ToString(), s.str());
        if (is_set) {
          EXPECT_THAT(s.str(), HasSubstr("flags=QUIC_HTTP_END_HEADERS,"));
        } else {
          EXPECT_THAT(s.str(), HasSubstr("flags=,"));
        }
      }
      break;
    default:
      EXPECT_DEBUG_DEATH(v.IsEndHeaders(),
                         "HEADERS.*PUSH_PROMISE.*CONTINUATION")
          << v;
  }
}

class QuicHttpIsPaddedTest : public QuicHttpFrameHeaderTypeAndFlagTest {};
INSTANTIATE_TEST_CASE_P(IsPadded,
                        QuicHttpIsPaddedTest,
                        Combine(ValuesIn(ValidFrameTypes()),
                                Values(~QuicHttpFrameFlag::QUIC_HTTP_PADDED,
                                       0xff)));
TEST_P(QuicHttpIsPaddedTest, IsPadded) {
  const bool is_set = (flags_ & QuicHttpFrameFlag::QUIC_HTTP_PADDED) ==
                      QuicHttpFrameFlag::QUIC_HTTP_PADDED;
  QuicString flags_string;
  QuicHttpFrameHeader v(0, type_, flags_, 0);
  switch (type_) {
    case QuicHttpFrameType::DATA:
    case QuicHttpFrameType::HEADERS:
    case QuicHttpFrameType::PUSH_PROMISE:
      EXPECT_EQ(is_set, v.IsPadded()) << v;
      flags_string = v.FlagsToString();
      if (is_set) {
        EXPECT_THAT(QuicTextUtils::Split(flags_string, '|'),
                    Contains("QUIC_HTTP_PADDED"));
      } else {
        EXPECT_THAT(flags_string, Not(HasSubstr("QUIC_HTTP_PADDED")));
      }
      v.RetainFlags(QuicHttpFrameFlag::QUIC_HTTP_PADDED);
      EXPECT_EQ(is_set, v.IsPadded()) << v;
      {
        std::stringstream s;
        s << v;
        EXPECT_EQ(v.ToString(), s.str());
        if (is_set) {
          EXPECT_THAT(s.str(), HasSubstr("flags=QUIC_HTTP_PADDED,"));
        } else {
          EXPECT_THAT(s.str(), HasSubstr("flags=,"));
        }
      }
      break;
    default:
      EXPECT_DEBUG_DEATH(v.IsPadded(), "DATA.*HEADERS.*PUSH_PROMISE") << v;
  }
}

class QuicHttpHasPriorityTest : public QuicHttpFrameHeaderTypeAndFlagTest {};
INSTANTIATE_TEST_CASE_P(HasPriority,
                        QuicHttpHasPriorityTest,
                        Combine(ValuesIn(ValidFrameTypes()),
                                Values(~QuicHttpFrameFlag::QUIC_HTTP_PRIORITY,
                                       0xff)));
TEST_P(QuicHttpHasPriorityTest, HasPriority) {
  const bool is_set = (flags_ & QuicHttpFrameFlag::QUIC_HTTP_PRIORITY) ==
                      QuicHttpFrameFlag::QUIC_HTTP_PRIORITY;
  QuicString flags_string;
  QuicHttpFrameHeader v(0, type_, flags_, 0);
  switch (type_) {
    case QuicHttpFrameType::HEADERS:
      EXPECT_EQ(is_set, v.HasPriority()) << v;
      flags_string = v.FlagsToString();
      if (is_set) {
        EXPECT_THAT(QuicTextUtils::Split(flags_string, '|'),
                    Contains("QUIC_HTTP_PRIORITY"));
      } else {
        EXPECT_THAT(flags_string, Not(HasSubstr("QUIC_HTTP_PRIORITY")));
      }
      v.RetainFlags(QuicHttpFrameFlag::QUIC_HTTP_PRIORITY);
      EXPECT_EQ(is_set, v.HasPriority()) << v;
      {
        std::stringstream s;
        s << v;
        EXPECT_EQ(v.ToString(), s.str());
        if (is_set) {
          EXPECT_THAT(s.str(), HasSubstr("flags=QUIC_HTTP_PRIORITY,"));
        } else {
          EXPECT_THAT(s.str(), HasSubstr("flags=,"));
        }
      }
      break;
    default:
      EXPECT_DEBUG_DEATH(v.HasPriority(), "HEADERS") << v;
  }
}

TEST(QuicHttpPriorityFieldsTest, Constructor) {
  QuicTestRandom random;
  uint32_t stream_dependency = random.Rand32() & QuicHttpStreamIdMask();
  uint32_t weight = 1 + random.Rand8();
  bool is_exclusive = random.OneIn(2);

  QuicHttpPriorityFields v(stream_dependency, weight, is_exclusive);

  EXPECT_EQ(stream_dependency, v.stream_dependency);
  EXPECT_EQ(weight, v.weight);
  EXPECT_EQ(is_exclusive, v.is_exclusive);

  // The high-bit must not be set on the stream id.
  EXPECT_DEBUG_DEATH(QuicHttpPriorityFields(stream_dependency | 0x80000000,
                                            weight, is_exclusive),
                     "31-bit");

  // The weight must be in the range 1-256.
  EXPECT_DEBUG_DEATH(QuicHttpPriorityFields(stream_dependency, 0, is_exclusive),
                     "too small");
  EXPECT_DEBUG_DEATH(
      QuicHttpPriorityFields(stream_dependency, weight + 256, is_exclusive),
      "too large");
}
#endif  // GTEST_HAS_DEATH_TEST && !defined(NDEBUG)

TEST(QuicHttpRstStreamFieldsTest, IsSupported) {
  QuicHttpRstStreamFields v{QuicHttpErrorCode::HTTP2_NO_ERROR};
  EXPECT_TRUE(v.IsSupportedErrorCode()) << v;

  QuicHttpRstStreamFields u{static_cast<QuicHttpErrorCode>(~0)};
  EXPECT_FALSE(u.IsSupportedErrorCode()) << v;
}

TEST(QuicHttpSettingFieldsTest, Misc) {
  QuicTestRandom random;
  QuicHttpSettingsParameter parameter =
      static_cast<QuicHttpSettingsParameter>(random.Rand16());
  uint32_t value = random.Rand32();

  QuicHttpSettingFields v(parameter, value);

  EXPECT_EQ(v, v);
  EXPECT_EQ(parameter, v.parameter);
  EXPECT_EQ(value, v.value);

  if (static_cast<int>(parameter) < 7) {
    EXPECT_TRUE(v.IsSupportedParameter()) << v;
  } else {
    EXPECT_FALSE(v.IsSupportedParameter()) << v;
  }

  QuicHttpSettingFields u(parameter, ~value);
  EXPECT_NE(v, u);
  EXPECT_EQ(v.parameter, u.parameter);
  EXPECT_NE(v.value, u.value);

  QuicHttpSettingFields w(IncrementEnum(parameter), value);
  EXPECT_NE(v, w);
  EXPECT_NE(v.parameter, w.parameter);
  EXPECT_EQ(v.value, w.value);

  QuicHttpSettingFields x(QuicHttpSettingsParameter::MAX_FRAME_SIZE, 123);
  std::stringstream s;
  s << x;
  EXPECT_EQ("parameter=MAX_FRAME_SIZE, value=123", s.str());
}

TEST(QuicHttpPushPromiseTest, Misc) {
  QuicTestRandom random;
  uint32_t promised_stream_id = random.Rand32() & QuicHttpStreamIdMask();

  QuicHttpPushPromiseFields v{promised_stream_id};
  EXPECT_EQ(promised_stream_id, v.promised_stream_id);
  EXPECT_EQ(v, v);

  std::stringstream s;
  s << v;
  EXPECT_EQ(QuicStrCat("promised_stream_id=", promised_stream_id), s.str());

  // High-bit is reserved, but not used, so we can set it.
  promised_stream_id |= 0x80000000;
  QuicHttpPushPromiseFields w{promised_stream_id};
  EXPECT_EQ(w, w);
  EXPECT_NE(v, w);

  v.promised_stream_id = promised_stream_id;
  EXPECT_EQ(v, w);
}

TEST(QuicHttpPingFieldsTest, Misc) {
  QuicHttpPingFields v{{'8', ' ', 'b', 'y', 't', 'e', 's', '\0'}};
  std::stringstream s;
  s << v;
  EXPECT_EQ("opaque_bytes=0x3820627974657300", s.str());
}

// Started becoming very flaky on all platforms after around crrev.com/519987.
TEST(QuicHttpGoAwayFieldsTest, DISABLED_Misc) {
  QuicTestRandom random;
  uint32_t last_stream_id = random.Rand32() & QuicHttpStreamIdMask();
  QuicHttpErrorCode error_code =
      static_cast<QuicHttpErrorCode>(random.Rand32());

  QuicHttpGoAwayFields v(last_stream_id, error_code);
  EXPECT_EQ(v, v);
  EXPECT_EQ(last_stream_id, v.last_stream_id);
  EXPECT_EQ(error_code, v.error_code);

  if (static_cast<int>(error_code) < 14) {
    EXPECT_TRUE(v.IsSupportedErrorCode()) << v;
  } else {
    EXPECT_FALSE(v.IsSupportedErrorCode()) << v;
  }

  QuicHttpGoAwayFields u(~last_stream_id, error_code);
  EXPECT_NE(v, u);
  EXPECT_NE(v.last_stream_id, u.last_stream_id);
  EXPECT_EQ(v.error_code, u.error_code);
}

TEST(QuicHttpWindowUpdateTest, Misc) {
  QuicTestRandom random;
  uint32_t window_size_increment = random.Rand32() & QuicHttpUint31Mask();

  QuicHttpWindowUpdateFields v{window_size_increment};
  EXPECT_EQ(window_size_increment, v.window_size_increment);
  EXPECT_EQ(v, v);

  std::stringstream s;
  s << v;
  EXPECT_EQ(QuicStrCat("window_size_increment=", window_size_increment),
            s.str());

  // High-bit is reserved, but not used, so we can set it.
  window_size_increment |= 0x80000000;
  QuicHttpWindowUpdateFields w{window_size_increment};
  EXPECT_EQ(w, w);
  EXPECT_NE(v, w);

  v.window_size_increment = window_size_increment;
  EXPECT_EQ(v, w);
}

TEST(QuicHttpAltSvcTest, Misc) {
  QuicTestRandom random;
  uint16_t origin_length = random.Rand16();

  QuicHttpAltSvcFields v{origin_length};
  EXPECT_EQ(origin_length, v.origin_length);
  EXPECT_EQ(v, v);

  std::stringstream s;
  s << v;
  EXPECT_EQ(QuicStrCat("origin_length=", origin_length), s.str());

  QuicHttpAltSvcFields w{++origin_length};
  EXPECT_EQ(w, w);
  EXPECT_NE(v, w);

  v.origin_length = w.origin_length;
  EXPECT_EQ(v, w);
}

}  // namespace
}  // namespace test
}  // namespace net
