// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/mutator.h"

#include <algorithm>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "port/gtest.h"
#include "src/binary_format.h"
#include "src/mutator_test_proto2.pb.h"
#include "src/mutator_test_proto3.pb.h"
#include "src/text_format.h"

namespace protobuf_mutator {

using protobuf::util::MessageDifferencer;
using testing::TestWithParam;
using testing::ValuesIn;

const char kMessages[] = R"(
  required_msg {}
  optional_msg {}
  repeated_msg {}
  repeated_msg {required_sint32: 56}
  repeated_msg {}
  repeated_msg {
    required_msg {}
    optional_msg {}
    repeated_msg {}
    repeated_msg { required_int32: 67 }
    repeated_msg {}
  }
)";

const char kMessagesProto3[] = R"(
  optional_msg {}
  repeated_msg {}
  repeated_msg {optional_sint32: 56}
  repeated_msg {}
  repeated_msg {
    optional_msg {}
    repeated_msg {}
    repeated_msg { optional_int32: 67 }
    repeated_msg {}
  }
)";

const char kRequiredFields[] = R"(
  required_double: 1.26685288449177e-313
  required_float: 5.9808638e-39
  required_int32: 67
  required_int64: 5285068
  required_uint32: 14486213
  required_uint64: 520229415
  required_sint32: 56
  required_sint64: -6057486163525532641
  required_fixed32: 8812173
  required_fixed64: 273731277756
  required_sfixed32: 43142
  required_sfixed64: 132
  required_bool: false
  required_string: "qwert"
  required_bytes: "asdf"
)";

const char kOptionalFields[] = R"(
  optional_double: 1.93177850152856e-314
  optional_float: 4.7397519e-41
  optional_int32: 40020
  optional_int64: 10
  optional_uint32: 40
  optional_uint64: 159
  optional_sint32: 44015
  optional_sint64: 17493625000076
  optional_fixed32: 193
  optional_fixed64: 8542688694448488723
  optional_sfixed32: 4926
  optional_sfixed64: 60
  optional_bool: true
  optional_string: "QWERT"
  optional_bytes: "ASDF"
  optional_enum: ENUM_5
)";

const char kRepeatedFields[] = R"(
  repeated_double: 1.93177850152856e-314
  repeated_double: 1.26685288449177e-313
  repeated_float: 4.7397519e-41
  repeated_float: 5.9808638e-39
  repeated_int32: 40020
  repeated_int32: 67
  repeated_int64: 10
  repeated_int64: 5285068
  repeated_uint32: 40
  repeated_uint32: 14486213
  repeated_uint64: 159
  repeated_uint64: 520229415
  repeated_sint32: 44015
  repeated_sint32: 56
  repeated_sint64: 17493625000076
  repeated_sint64: -6057486163525532641
  repeated_fixed32: 193
  repeated_fixed32: 8812173
  repeated_fixed64: 8542688694448488723
  repeated_fixed64: 273731277756
  repeated_sfixed32: 4926
  repeated_sfixed32: 43142
  repeated_sfixed64: 60
  repeated_sfixed64: 132
  repeated_bool: false
  repeated_bool: true
  repeated_string: "QWERT"
  repeated_string: "qwert"
  repeated_bytes: "ASDF"
  repeated_bytes: "asdf"
  repeated_enum: ENUM_5
  repeated_enum: ENUM_4
)";

const char kRequiredNestedFields[] = R"(
  required_int32: 123
  optional_msg {
    required_double: 1.26685288449177e-313
    required_float: 5.9808638e-39
    required_int32: 67
    required_int64: 5285068
    required_uint32: 14486213
    required_uint64: 520229415
    required_sint32: 56
    required_sint64: -6057486163525532641
    required_fixed32: 8812173
    required_fixed64: 273731277756
    required_sfixed32: 43142
    required_sfixed64: 132
    required_bool: false
    required_string: "qwert"
    required_bytes: "asdf"
  }
)";

const char kOptionalNestedFields[] = R"(
  optional_int32: 123
  optional_msg {
    optional_double: 1.93177850152856e-314
    optional_float: 4.7397519e-41
    optional_int32: 40020
    optional_int64: 10
    optional_uint32: 40
    optional_uint64: 159
    optional_sint32: 44015
    optional_sint64: 17493625000076
    optional_fixed32: 193
    optional_fixed64: 8542688694448488723
    optional_sfixed32: 4926
    optional_sfixed64: 60
    optional_bool: true
    optional_string: "QWERT"
    optional_bytes: "ASDF"
    optional_enum: ENUM_5
  }
)";

const char kRepeatedNestedFields[] = R"(
  optional_int32: 123
  optional_msg {
    repeated_double: 1.93177850152856e-314
    repeated_double: 1.26685288449177e-313
    repeated_float: 4.7397519e-41
    repeated_float: 5.9808638e-39
    repeated_int32: 40020
    repeated_int32: 67
    repeated_int64: 10
    repeated_int64: 5285068
    repeated_uint32: 40
    repeated_uint32: 14486213
    repeated_uint64: 159
    repeated_uint64: 520229415
    repeated_sint32: 44015
    repeated_sint32: 56
    repeated_sint64: 17493625000076
    repeated_sint64: -6057486163525532641
    repeated_fixed32: 193
    repeated_fixed32: 8812173
    repeated_fixed64: 8542688694448488723
    repeated_fixed64: 273731277756
    repeated_sfixed32: 4926
    repeated_sfixed32: 43142
    repeated_sfixed64: 60
    repeated_sfixed64: 132
    repeated_bool: false
    repeated_bool: true
    repeated_string: "QWERT"
    repeated_string: "qwert"
    repeated_bytes: "ASDF"
    repeated_bytes: "asdf"
    repeated_enum: ENUM_5
    repeated_enum: ENUM_4
  }
)";

class TestMutator : public Mutator {
 public:
  explicit TestMutator(bool keep_initialized) : Mutator(&random_), random_(17) {
    keep_initialized_ = keep_initialized;
  }

  // Avoids dedup logic for some tests.
  void NoDeDupCrossOver(const protobuf::Message& message1,
                        protobuf::Message* message2) {
    CrossOverImpl(message1, message2);
  }

 private:
  RandomEngine random_;
};

class ReducedTestMutator : public TestMutator {
 public:
  ReducedTestMutator() : TestMutator(false) {
    for (float i = 1000; i > 0.1; i /= 7) {
      values_.push_back(i);
      values_.push_back(-i);
    }
    values_.push_back(-1.0);
    values_.push_back(0.0);
    values_.push_back(1.0);
  }

 protected:
  int32_t MutateInt32(int32_t value) override { return GetRandomValue(); }
  int64_t MutateInt64(int64_t value) override { return GetRandomValue(); }
  uint32_t MutateUInt32(uint32_t value) override {
    return fabs(GetRandomValue());
  }
  uint64_t MutateUInt64(uint64_t value) override {
    return fabs(GetRandomValue());
  }
  float MutateFloat(float value) override { return GetRandomValue(); }
  double MutateDouble(double value) override { return GetRandomValue(); }
  std::string MutateString(const std::string& value,
                           size_t size_increase_hint) override {
    return strings_[std::uniform_int_distribution<uint8_t>(
        0, strings_.size() - 1)(*random())];
  }

 private:
  float GetRandomValue() {
    return values_[std::uniform_int_distribution<uint8_t>(
        0, values_.size() - 1)(*random())];
  }

  std::vector<float> values_;
  std::vector<std::string> strings_ = {
      "", "\001", "\000", "a", "b", "ab",
  };
};

std::vector<std::string> Split(const std::string& str) {
  std::istringstream iss(str);
  std::vector<std::string> result;
  for (std::string line; std::getline(iss, line, '\n');) result.push_back(line);
  return result;
}

using TestParams = std::tuple<const protobuf::Message*, const char*, size_t>;

template <class T>
std::vector<TestParams> GetFieldTestParams(
    const std::vector<const char*>& tests) {
  std::vector<TestParams> results;
  for (auto t : tests) {
    auto lines = Split(t);
    for (size_t i = 0; i != lines.size(); ++i) {
      if (lines[i].find(':') != std::string::npos)
        results.push_back(std::make_tuple(&T::default_instance(), t, i));
    }
  }
  return results;
}

template <class T>
std::vector<TestParams> GetMessageTestParams(
    const std::vector<const char*>& tests) {
  std::vector<TestParams> results;
  for (auto t : tests) {
    auto lines = Split(t);
    for (size_t i = 0; i != lines.size(); ++i) {
      if (lines[i].find("{}") != std::string::npos)
        results.push_back(std::make_tuple(&T::default_instance(), t, i));
    }
  }
  return results;
}

bool Mutate(const protobuf::Message& from, const protobuf::Message& to) {
  EXPECT_FALSE(MessageDifferencer::Equals(from, to));
  ReducedTestMutator mutator;
  std::unique_ptr<protobuf::Message> message(from.New());
  for (int j = 0; j < 1000000; ++j) {
    message->CopyFrom(from);
    mutator.Mutate(message.get(), 1000);
    if (MessageDifferencer::Equals(*message, to)) return true;
  }

  ADD_FAILURE() << "Failed to get from:\n"
                << SaveMessageAsText(from) << "\nto:\n"
                << SaveMessageAsText(to);
  return false;
}

class MutatorTest : public TestWithParam<TestParams> {
 protected:
  void SetUp() override {
    m1_.reset(std::get<0>(GetParam())->New());
    m2_.reset(std::get<0>(GetParam())->New());
    text_ = std::get<1>(GetParam());
    line_ = std::get<2>(GetParam());
  }

  void LoadMessage(protobuf::Message* message) {
    EXPECT_TRUE(ParseTextMessage(text_, message));
  }

  bool LoadWithoutLine(protobuf::Message* message) {
    std::ostringstream oss;
    auto lines = Split(text_);
    for (size_t i = 0; i != lines.size(); ++i) {
      if (i != line_) oss << lines[i] << '\n';
    }
    return ParseTextMessage(oss.str(), message);
  }

  bool LoadWithChangedLine(protobuf::Message* message, int value) {
    auto lines = Split(text_);
    std::ostringstream oss;
    for (size_t i = 0; i != lines.size(); ++i) {
      if (i != line_) {
        oss << lines[i] << '\n';
      } else {
        std::string s = lines[i];
        s.resize(s.find(':') + 2);

        if (lines[i].back() == '\"') {
          // strings
          s += value ? "\"\\" + std::to_string(value) + "\"" : "\"\"";
        } else if (lines[i].back() == 'e') {
          // bools
          s += value ? "true" : "false";
        } else {
          s += std::to_string(value);
        }
        oss << s << '\n';
      }
    }
    return ParseTextMessage(oss.str(), message);
  }

  std::string text_;
  size_t line_;
  std::unique_ptr<protobuf::Message> m1_;
  std::unique_ptr<protobuf::Message> m2_;
};

// These tests are irrelevant for Proto3 as it has no required fields and
// insertion/deletion.

class MutatorFieldInsDelTest : public MutatorTest {};
INSTANTIATE_TEST_CASE_P(Proto2, MutatorFieldInsDelTest,
                        ValuesIn(GetFieldTestParams<Msg>(
                            {kRequiredFields, kOptionalFields, kRepeatedFields,
                             kRequiredNestedFields, kOptionalNestedFields,
                             kRepeatedNestedFields})));

TEST_P(MutatorFieldInsDelTest, DeleteField) {
  LoadMessage(m1_.get());
  LoadWithoutLine(m2_.get());
  EXPECT_TRUE(Mutate(*m1_, *m2_));
}

TEST_P(MutatorFieldInsDelTest, InsertField) {
  LoadWithoutLine(m1_.get());
  LoadWithChangedLine(m2_.get(), 0);
  EXPECT_TRUE(Mutate(*m1_, *m2_));
}

class MutatorFieldTest : public MutatorTest {
 public:
  template <class Msg>
  void TestCopyField();
};
INSTANTIATE_TEST_CASE_P(Proto2, MutatorFieldTest,
                        ValuesIn(GetFieldTestParams<Msg>(
                            {kRequiredFields, kOptionalFields, kRepeatedFields,
                             kRequiredNestedFields, kOptionalNestedFields,
                             kRepeatedNestedFields})));
INSTANTIATE_TEST_CASE_P(Proto3, MutatorFieldTest,
                        ValuesIn(GetFieldTestParams<Msg3>(
                            {kOptionalFields, kRepeatedFields,
                             kOptionalNestedFields, kRepeatedNestedFields})));

TEST_P(MutatorFieldTest, Initialized) {
  LoadWithoutLine(m1_.get());
  TestMutator mutator(true);
  mutator.Mutate(m1_.get(), 1000);
  EXPECT_TRUE(m1_->IsInitialized());
}

TEST_P(MutatorFieldTest, ChangeField) {
  LoadWithChangedLine(m1_.get(), 0);
  LoadWithChangedLine(m2_.get(), 1);
  EXPECT_TRUE(Mutate(*m1_, *m2_));
  EXPECT_TRUE(Mutate(*m2_, *m1_));
}

template <class Msg>
void MutatorFieldTest::TestCopyField() {
  LoadWithChangedLine(m1_.get(), 7);
  LoadWithChangedLine(m2_.get(), 0);

  Msg from;
  from.add_repeated_msg()->CopyFrom(*m1_);
  from.add_repeated_msg()->CopyFrom(*m2_);

  Msg to;
  to.add_repeated_msg()->CopyFrom(*m1_);
  to.add_repeated_msg()->CopyFrom(*m1_);
  EXPECT_TRUE(Mutate(from, to));

  to.Clear();
  to.add_repeated_msg()->CopyFrom(*m2_);
  to.add_repeated_msg()->CopyFrom(*m2_);
  EXPECT_TRUE(Mutate(from, to));
}

TEST_P(MutatorFieldTest, CopyField) {
  if (m1_->GetDescriptor() == Msg::descriptor())
    TestCopyField<Msg>();
  else
    TestCopyField<Msg3>();
}

class MutatorSingleFieldTest : public MutatorTest {};
INSTANTIATE_TEST_CASE_P(Proto2, MutatorSingleFieldTest,
                        ValuesIn(GetFieldTestParams<Msg>({
                            kRequiredFields, kOptionalFields,
                            kRequiredNestedFields, kOptionalNestedFields,
                        })));
INSTANTIATE_TEST_CASE_P(Proto3, MutatorSingleFieldTest,
                        ValuesIn(GetFieldTestParams<Msg3>({
                            kOptionalFields, kOptionalNestedFields,
                        })));

TEST_P(MutatorSingleFieldTest, CrossOver) {
  LoadWithoutLine(m1_.get());
  LoadMessage(m2_.get());

  EXPECT_FALSE(MessageDifferencer::Equals(*m1_, *m2_));
  TestMutator mutator(false);

  int match_m1_ = 0;
  int match_m2_ = 0;
  int iterations = 1000;
  std::unique_ptr<protobuf::Message> message(m1_->New());
  for (int j = 0; j < iterations; ++j) {
    message->CopyFrom(*m1_);
    mutator.NoDeDupCrossOver(*m2_, message.get());
    if (MessageDifferencer::Equals(*message, *m2_)) ++match_m2_;
    if (MessageDifferencer::Equals(*message, *m1_)) ++match_m1_;
  }

  EXPECT_LT(iterations * .4, match_m1_);
  EXPECT_GE(iterations * .6, match_m1_);
  EXPECT_LT(iterations * .4, match_m2_);
  EXPECT_GE(iterations * .6, match_m2_);
}

template <typename T>
class MutatorTypedTest : public ::testing::Test {
 public:
  using Message = T;
};

using MutatorTypedTestTypes = testing::Types<Msg, Msg3>;
TYPED_TEST_CASE(MutatorTypedTest, MutatorTypedTestTypes);

TYPED_TEST(MutatorTypedTest, CrossOverRepeated) {
  typename TestFixture::Message m1;
  m1.add_repeated_int32(1);
  m1.add_repeated_int32(2);
  m1.add_repeated_int32(3);

  typename TestFixture::Message m2;
  m2.add_repeated_int32(4);
  m2.add_repeated_int32(5);
  m2.add_repeated_int32(6);

  int iterations = 10000;
  std::set<std::set<int>> sets;
  TestMutator mutator(false);
  for (int j = 0; j < iterations; ++j) {
    typename TestFixture::Message message;
    message.CopyFrom(m1);
    mutator.NoDeDupCrossOver(m2, &message);
    sets.insert(
        {message.repeated_int32().begin(), message.repeated_int32().end()});
  }

  EXPECT_EQ(1 << 6, sets.size());
}

TYPED_TEST(MutatorTypedTest, CrossOverRepeatedMessages) {
  typename TestFixture::Message m1;
  auto* rm1 = m1.add_repeated_msg();
  rm1->add_repeated_int32(1);
  rm1->add_repeated_int32(2);

  typename TestFixture::Message m2;
  auto* rm2 = m2.add_repeated_msg();
  rm2->add_repeated_int32(3);
  rm2->add_repeated_int32(4);
  rm2->add_repeated_int32(5);
  rm2->add_repeated_int32(6);

  int iterations = 10000;
  std::set<std::set<int>> sets;
  TestMutator mutator(false);
  for (int j = 0; j < iterations; ++j) {
    typename TestFixture::Message message;
    message.CopyFrom(m1);
    mutator.NoDeDupCrossOver(m2, &message);
    for (const auto& msg : message.repeated_msg())
      sets.insert({msg.repeated_int32().begin(), msg.repeated_int32().end()});
  }

  EXPECT_EQ(1 << 6, sets.size());
}

TYPED_TEST(MutatorTypedTest, FailedMutations) {
  TestMutator mutator(false);
  size_t crossovers = 0;
  for (int i = 0; i < 10000; ++i) {
    typename TestFixture::Message messages[2];
    typename TestFixture::Message tmp;
    for (int j = 0; j < 20; ++j) {
      for (auto& m : messages) {
        tmp.CopyFrom(m);
        mutator.Mutate(&m, 1000);
        // Mutate must not produce the same result.
        EXPECT_FALSE(MessageDifferencer::Equals(m, tmp));
      }
    }

    tmp.CopyFrom(messages[1]);
    mutator.CrossOver(messages[0], &tmp);
    if (MessageDifferencer::Equals(tmp, messages[1]) ||
        MessageDifferencer::Equals(tmp, messages[0]))
      ++crossovers;
  }

  // CrossOver may fail but very rare.
  EXPECT_LT(crossovers, 100);
}

TYPED_TEST(MutatorTypedTest, Serialization) {
  TestMutator mutator(false);
  for (int i = 0; i < 10000; ++i) {
    typename TestFixture::Message message;
    for (int j = 0; j < 5; ++j) {
      mutator.Mutate(&message, 1000);
      typename TestFixture::Message parsed;

      EXPECT_TRUE(ParseTextMessage(SaveMessageAsText(message), &parsed));
      EXPECT_TRUE(MessageDifferencer::Equals(parsed, message));

      EXPECT_TRUE(ParseBinaryMessage(SaveMessageAsBinary(message), &parsed));
      EXPECT_TRUE(MessageDifferencer::Equals(parsed, message));
    }
  }
}

class MutatorMessagesTest : public MutatorTest {};
INSTANTIATE_TEST_CASE_P(Proto2, MutatorMessagesTest,
                        ValuesIn(GetMessageTestParams<Msg>({kMessages})));
INSTANTIATE_TEST_CASE_P(
    Proto3, MutatorMessagesTest,
    ValuesIn(GetMessageTestParams<Msg3>({kMessagesProto3})));

TEST_P(MutatorMessagesTest, DeletedMessage) {
  LoadMessage(m1_.get());
  LoadWithoutLine(m2_.get());
  EXPECT_TRUE(Mutate(*m1_, *m2_));
}

TEST_P(MutatorMessagesTest, InsertMessage) {
  LoadWithoutLine(m1_.get());
  LoadMessage(m2_.get());
  EXPECT_TRUE(Mutate(*m1_, *m2_));
}

// TODO(vitalybuka): Special tests for oneof.

TEST(MutatorMessagesTest, UsageExample) {
  SmallMessage message;
  TestMutator mutator(false);

  // Test that we can generate all variation of the message.
  std::set<std::string> mutations;
  for (int j = 0; j < 1000; ++j) {
    mutator.Mutate(&message, 1000);
    std::string str = SaveMessageAsText(message);
    mutations.insert(str);
  }

  // 3 states for boolean and 5 for enum, including missing fields.
  EXPECT_EQ(3u * 5u, mutations.size());
}

TEST(MutatorMessagesTest, EmptyMessage) {
  EmptyMessage message;
  TestMutator mutator(false);
  for (int j = 0; j < 10000; ++j) mutator.Mutate(&message, 1000);
}


TEST(MutatorMessagesTest, Regressions) {
  RegressionMessage message;
  TestMutator mutator(false);
  for (int j = 0; j < 10000; ++j) mutator.Mutate(&message, 1000);
}

}  // namespace protobuf_mutator
