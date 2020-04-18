// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/perf/perf_provider_chromeos.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/metrics/perf/windowed_incognito_observer.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/login/login_state.h"
#include "components/variations/variations_associated_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/sampled_profile.pb.h"
#include "third_party/protobuf/src/google/protobuf/io/coded_stream.h"
#include "third_party/protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "third_party/protobuf/src/google/protobuf/wire_format_lite_inl.h"

namespace metrics {

namespace {

const char kPerfRecordCyclesCmd[] =
  "perf record -a -e cycles -c 1000003";
const char kPerfRecordCallgraphCmd[] =
  "perf record -a -e cycles -g -c 4000037";
const char kPerfRecordLBRCmd[] =
  "perf record -a -e r20c4 -b -c 200011";
const char kPerfRecordCacheMissesCmd[] =
    "perf record -a -e cache-misses -c 10007";
const char kPerfStatMemoryBandwidthCmd[] =
  "perf stat -a -e cycles -e instructions "
  "-e uncore_imc/data_reads/ -e uncore_imc/data_writes/ "
  "-e cpu/event=0xD0,umask=0x11,name=MEM_UOPS_RETIRED-STLB_MISS_LOADS/ "
  "-e cpu/event=0xD0,umask=0x12,name=MEM_UOPS_RETIRED-STLB_MISS_STORES/";

// Converts a protobuf to serialized format as a byte vector.
std::vector<uint8_t> SerializeMessageToVector(
    const google::protobuf::MessageLite& message) {
  std::vector<uint8_t> result(message.ByteSize());
  message.SerializeToArray(result.data(), result.size());
  return result;
}

// Returns an example PerfDataProto. The contents don't have to make sense. They
// just need to constitute a semantically valid protobuf.
// |proto| is an output parameter that will contain the created protobuf.
PerfDataProto GetExamplePerfDataProto() {
  PerfDataProto proto;
  proto.set_timestamp_sec(1435604013);  // Time since epoch in seconds->

  PerfDataProto_PerfFileAttr* file_attr = proto.add_file_attrs();
  file_attr->add_ids(61);
  file_attr->add_ids(62);
  file_attr->add_ids(63);

  PerfDataProto_PerfEventAttr* attr = file_attr->mutable_attr();
  attr->set_type(1);
  attr->set_size(2);
  attr->set_config(3);
  attr->set_sample_period(4);
  attr->set_sample_freq(5);

  PerfDataProto_PerfEventStats* stats = proto.mutable_stats();
  stats->set_num_events_read(100);
  stats->set_num_sample_events(200);
  stats->set_num_mmap_events(300);
  stats->set_num_fork_events(400);
  stats->set_num_exit_events(500);

  return proto;
}

// Returns an example PerfStatProto. The contents don't have to make sense. They
// just need to constitute a semantically valid protobuf.
// |result| is an output parameter that will contain the created protobuf.
PerfStatProto GetExamplePerfStatProto() {
  PerfStatProto proto;
  proto.set_command_line(
      "perf stat -a -e cycles -e instructions -e branches -- sleep 2");

  PerfStatProto_PerfStatLine* line1 = proto.add_line();
  line1->set_time_ms(1000);
  line1->set_count(2000);
  line1->set_event_name("cycles");

  PerfStatProto_PerfStatLine* line2 = proto.add_line();
  line2->set_time_ms(2000);
  line2->set_count(5678);
  line2->set_event_name("instructions");

  PerfStatProto_PerfStatLine* line3 = proto.add_line();
  line3->set_time_ms(3000);
  line3->set_count(9999);
  line3->set_event_name("branches");

  return proto;
}

// Creates a serialized data stream containing a string with a field tag number.
std::string SerializeStringFieldWithTag(int field, const std::string& value) {
  std::string result;
  google::protobuf::io::StringOutputStream string_stream(&result);
  google::protobuf::io::CodedOutputStream output(&string_stream);

  using google::protobuf::internal::WireFormatLite;
  WireFormatLite::WriteTag(field, WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
                           &output);
  output.WriteVarint32(value.size());
  output.WriteString(value);

  return result;
}

// Allows testing of PerfProvider behavior when an incognito window is opened.
class TestIncognitoObserver : public WindowedIncognitoObserver {
 public:
  // Factory function to create a TestIncognitoObserver object contained in a
  // std::unique_ptr<WindowedIncognitoObserver> object. |incognito_launched|
  // simulates the presence of an open incognito window, or the lack thereof.
  // Used for passing observers to ParseOutputProtoIfValid().
  static std::unique_ptr<WindowedIncognitoObserver> CreateWithIncognitoLaunched(
      bool incognito_launched) {
    std::unique_ptr<TestIncognitoObserver> observer(new TestIncognitoObserver);
    observer->set_incognito_launched(incognito_launched);
    return std::move(observer);
  }

 private:
  TestIncognitoObserver() {}

  DISALLOW_COPY_AND_ASSIGN(TestIncognitoObserver);
};

// Allows access to some private methods for testing.
class TestPerfProvider : public PerfProvider {
 public:
  TestPerfProvider() {}

  using PerfProvider::PerfSubcommand;
  using PerfProvider::ParseOutputProtoIfValid;
  using PerfProvider::OnSessionRestoreDone;
  using PerfProvider::Deactivate;
  using PerfProvider::collection_params;
  using PerfProvider::command_selector;
  using PerfProvider::timer;

 private:
  std::vector<SampledProfile> stored_profiles_;

  DISALLOW_COPY_AND_ASSIGN(TestPerfProvider);
};

}  // namespace

class PerfProviderTest : public testing::Test {
 public:
  PerfProviderTest() : task_runner_(new base::TestSimpleTaskRunner),
                       task_runner_handle_(task_runner_),
                       perf_data_proto_(GetExamplePerfDataProto()),
                       perf_stat_proto_(GetExamplePerfStatProto()) {}

  void SetUp() override {
    // PerfProvider requires chromeos::LoginState and
    // chromeos::DBusThreadManagerto be initialized.
    chromeos::LoginState::Initialize();
    chromeos::DBusThreadManager::Initialize();

    perf_provider_.reset(new TestPerfProvider);
    perf_provider_->Init();

    // PerfProvider requires the user to be logged in.
    chromeos::LoginState::Get()->SetLoggedInState(
        chromeos::LoginState::LOGGED_IN_ACTIVE,
        chromeos::LoginState::LOGGED_IN_USER_REGULAR);
  }

  void TearDown() override {
    perf_provider_.reset();
    chromeos::DBusThreadManager::Shutdown();
    chromeos::LoginState::Shutdown();
  }

 protected:
  std::unique_ptr<TestPerfProvider> perf_provider_;

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;

  // These store example perf data/stat protobufs for testing.
  PerfDataProto perf_data_proto_;
  PerfStatProto perf_stat_proto_;

  DISALLOW_COPY_AND_ASSIGN(PerfProviderTest);
};

TEST_F(PerfProviderTest, CheckSetup) {
  EXPECT_GT(perf_data_proto_.ByteSize(), 0);
  EXPECT_GT(perf_stat_proto_.ByteSize(), 0);

  std::vector<SampledProfile> stored_profiles;
  EXPECT_FALSE(perf_provider_->GetSampledProfiles(&stored_profiles));
  EXPECT_TRUE(stored_profiles.empty());

  EXPECT_FALSE(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false)->
          incognito_launched());
  EXPECT_TRUE(
      TestIncognitoObserver::CreateWithIncognitoLaunched(true)->
          incognito_launched());
}

// If quipper fails, or the DBus call fails, no data will be returned.
TEST_F(PerfProviderTest, NoPerfData) {
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      std::string());

  std::vector<SampledProfile> stored_profiles;
  EXPECT_FALSE(perf_provider_->GetSampledProfiles(&stored_profiles));
}

TEST_F(PerfProviderTest, PerfDataProto) {
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles));
  ASSERT_EQ(1U, stored_profiles.size());

  const SampledProfile& profile = stored_profiles[0];
  EXPECT_EQ(SampledProfile::PERIODIC_COLLECTION, profile.trigger_event());
  EXPECT_TRUE(profile.has_ms_after_login());

  ASSERT_TRUE(profile.has_perf_data());
  EXPECT_FALSE(profile.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_data_proto_),
            SerializeMessageToVector(profile.perf_data()));
}

TEST_F(PerfProviderTest, PerfDataProto_UnknownFieldsDiscarded) {
  // First add some unknown fields to MMapEvent, CommEvent, PerfBuildID, and
  // StringAndMd5sumPrefix. The known field values don't have to make sense for
  // perf data. They are just padding to avoid having an otherwise empty proto.
  // The unknown field string contents don't have to make sense as serialized
  // data as the test is to discard them.

  // MMapEvent
  PerfDataProto_PerfEvent* event1 = perf_data_proto_.add_events();
  event1->mutable_header()->set_type(1);
  event1->mutable_mmap_event()->set_pid(1234);
  event1->mutable_mmap_event()->set_filename_md5_prefix(0xdeadbeef);
  // Missing field |MMapEvent::filename| has tag=6.
  *event1->mutable_mmap_event()->mutable_unknown_fields() =
      SerializeStringFieldWithTag(6, "/opt/google/chrome/chrome");

  // CommEvent
  PerfDataProto_PerfEvent* event2 = perf_data_proto_.add_events();
  event2->mutable_header()->set_type(2);
  event2->mutable_comm_event()->set_pid(5678);
  event2->mutable_comm_event()->set_comm_md5_prefix(0x900df00d);
  // Missing field |CommEvent::comm| has tag=3.
  *event2->mutable_comm_event()->mutable_unknown_fields() =
      SerializeStringFieldWithTag(3, "chrome");

  // PerfBuildID
  PerfDataProto_PerfBuildID* build_id = perf_data_proto_.add_build_ids();
  build_id->set_misc(3);
  build_id->set_pid(1337);
  build_id->set_filename_md5_prefix(0x9876543210);
  // Missing field |PerfBuildID::filename| has tag=4.
  *build_id->mutable_unknown_fields() =
      SerializeStringFieldWithTag(4, "/opt/google/chrome/chrome");

  // StringAndMd5sumPrefix
  PerfDataProto_StringMetadata* metadata =
      perf_data_proto_.mutable_string_metadata();
  metadata->mutable_perf_command_line_whole()->set_value_md5_prefix(
      0x123456789);
  // Missing field |StringAndMd5sumPrefix::value| has tag=1.
  *metadata->mutable_perf_command_line_whole()->mutable_unknown_fields() =
      SerializeStringFieldWithTag(1, "perf record -a -- sleep 1");

  // Serialize to string and make sure it can be deserialized.
  std::string perf_data_string = perf_data_proto_.SerializeAsString();
  PerfDataProto temp_proto;
  EXPECT_TRUE(temp_proto.ParseFromString(perf_data_string));

  // Now pass it to |perf_provider_|.
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_string);

  std::vector<SampledProfile> stored_profiles;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles));
  ASSERT_EQ(1U, stored_profiles.size());

  const SampledProfile& profile = stored_profiles[0];
  EXPECT_EQ(SampledProfile::PERIODIC_COLLECTION, profile.trigger_event());
  EXPECT_TRUE(profile.has_perf_data());

  // The serialized form should be different because the unknown fields have
  // have been removed.
  EXPECT_NE(perf_data_string, profile.perf_data().SerializeAsString());

  // Check contents of stored protobuf.
  const PerfDataProto& stored_proto = profile.perf_data();
  ASSERT_EQ(2, stored_proto.events_size());

  // MMapEvent
  const PerfDataProto_PerfEvent& stored_event1 = stored_proto.events(0);
  EXPECT_EQ(1U, stored_event1.header().type());
  EXPECT_EQ(1234U, stored_event1.mmap_event().pid());
  EXPECT_EQ(0xdeadbeef, stored_event1.mmap_event().filename_md5_prefix());
  EXPECT_EQ(0U, stored_event1.mmap_event().unknown_fields().size());

  // CommEvent
  const PerfDataProto_PerfEvent& stored_event2 = stored_proto.events(1);
  EXPECT_EQ(2U, stored_event2.header().type());
  EXPECT_EQ(5678U, stored_event2.comm_event().pid());
  EXPECT_EQ(0x900df00d, stored_event2.comm_event().comm_md5_prefix());
  EXPECT_EQ(0U, stored_event2.comm_event().unknown_fields().size());

  // PerfBuildID
  ASSERT_EQ(1, stored_proto.build_ids_size());
  const PerfDataProto_PerfBuildID& stored_build_id = stored_proto.build_ids(0);
  EXPECT_EQ(3U, stored_build_id.misc());
  EXPECT_EQ(1337U, stored_build_id.pid());
  EXPECT_EQ(0x9876543210U, stored_build_id.filename_md5_prefix());
  EXPECT_EQ(0U, stored_build_id.unknown_fields().size());

  // StringAndMd5sumPrefix
  const PerfDataProto_StringMetadata& stored_metadata =
      stored_proto.string_metadata();
  EXPECT_EQ(0x123456789U,
            stored_metadata.perf_command_line_whole().value_md5_prefix());
  EXPECT_EQ(0U,
            stored_metadata.perf_command_line_whole().unknown_fields().size());
}

TEST_F(PerfProviderTest, PerfStatProto) {
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_STAT,
      perf_stat_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles));
  ASSERT_EQ(1U, stored_profiles.size());

  const SampledProfile& profile = stored_profiles[0];
  EXPECT_EQ(SampledProfile::PERIODIC_COLLECTION, profile.trigger_event());
  EXPECT_TRUE(profile.has_ms_after_login());

  EXPECT_FALSE(profile.has_perf_data());
  ASSERT_TRUE(profile.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_stat_proto_),
            SerializeMessageToVector(profile.perf_stat()));
}

// Change |sampled_profile| between calls to ParseOutputProtoIfValid().
TEST_F(PerfProviderTest, MultipleCalls) {
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_proto_.SerializeAsString());

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESTORE_SESSION);
  sampled_profile->set_ms_after_restore(3000);
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_STAT,
      perf_stat_proto_.SerializeAsString());

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESUME_FROM_SUSPEND);
  sampled_profile->set_suspend_duration_ms(60000);
  sampled_profile->set_ms_after_resume(1500);
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_proto_.SerializeAsString());

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_STAT,
      perf_stat_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles));
  ASSERT_EQ(4U, stored_profiles.size());

  const SampledProfile& profile1 = stored_profiles[0];
  EXPECT_EQ(SampledProfile::PERIODIC_COLLECTION, profile1.trigger_event());
  EXPECT_TRUE(profile1.has_ms_after_login());
  ASSERT_TRUE(profile1.has_perf_data());
  EXPECT_FALSE(profile1.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_data_proto_),
            SerializeMessageToVector(profile1.perf_data()));

  const SampledProfile& profile2 = stored_profiles[1];
  EXPECT_EQ(SampledProfile::RESTORE_SESSION, profile2.trigger_event());
  EXPECT_TRUE(profile2.has_ms_after_login());
  EXPECT_EQ(3000, profile2.ms_after_restore());
  EXPECT_FALSE(profile2.has_perf_data());
  ASSERT_TRUE(profile2.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_stat_proto_),
            SerializeMessageToVector(profile2.perf_stat()));

  const SampledProfile& profile3 = stored_profiles[2];
  EXPECT_EQ(SampledProfile::RESUME_FROM_SUSPEND, profile3.trigger_event());
  EXPECT_TRUE(profile3.has_ms_after_login());
  EXPECT_EQ(60000, profile3.suspend_duration_ms());
  EXPECT_EQ(1500, profile3.ms_after_resume());
  ASSERT_TRUE(profile3.has_perf_data());
  EXPECT_FALSE(profile3.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_data_proto_),
            SerializeMessageToVector(profile3.perf_data()));

  const SampledProfile& profile4 = stored_profiles[3];
  EXPECT_EQ(SampledProfile::PERIODIC_COLLECTION, profile4.trigger_event());
  EXPECT_TRUE(profile4.has_ms_after_login());
  EXPECT_FALSE(profile4.has_perf_data());
  ASSERT_TRUE(profile4.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_stat_proto_),
            SerializeMessageToVector(profile4.perf_stat()));
}

// Simulate opening and closing of incognito window in between calls to
// ParseOutputProtoIfValid().
TEST_F(PerfProviderTest, IncognitoWindowOpened) {
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles1;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles1));
  ASSERT_EQ(1U, stored_profiles1.size());

  const SampledProfile& profile1 = stored_profiles1[0];
  EXPECT_EQ(SampledProfile::PERIODIC_COLLECTION, profile1.trigger_event());
  EXPECT_TRUE(profile1.has_ms_after_login());
  ASSERT_TRUE(profile1.has_perf_data());
  EXPECT_FALSE(profile1.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_data_proto_),
            SerializeMessageToVector(profile1.perf_data()));

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESTORE_SESSION);
  sampled_profile->set_ms_after_restore(3000);
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_STAT,
      perf_stat_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles2;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles2));
  ASSERT_EQ(1U, stored_profiles2.size());

  const SampledProfile& profile2 = stored_profiles2[0];
  EXPECT_EQ(SampledProfile::RESTORE_SESSION, profile2.trigger_event());
  EXPECT_TRUE(profile2.has_ms_after_login());
  EXPECT_EQ(3000, profile2.ms_after_restore());
  EXPECT_FALSE(profile2.has_perf_data());
  ASSERT_TRUE(profile2.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_stat_proto_),
            SerializeMessageToVector(profile2.perf_stat()));

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESUME_FROM_SUSPEND);
  // An incognito window opens.
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(true),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles_empty;
  EXPECT_FALSE(perf_provider_->GetSampledProfiles(&stored_profiles_empty));

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);
  // Incognito window is still open.
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(true),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_STAT,
      perf_stat_proto_.SerializeAsString());

  EXPECT_FALSE(perf_provider_->GetSampledProfiles(&stored_profiles_empty));

  sampled_profile.reset(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESUME_FROM_SUSPEND);
  sampled_profile->set_suspend_duration_ms(60000);
  sampled_profile->set_ms_after_resume(1500);
  // Incognito window closes.
  perf_provider_->ParseOutputProtoIfValid(
      TestIncognitoObserver::CreateWithIncognitoLaunched(false),
      std::move(sampled_profile),
      TestPerfProvider::PerfSubcommand::PERF_COMMAND_RECORD,
      perf_data_proto_.SerializeAsString());

  std::vector<SampledProfile> stored_profiles3;
  EXPECT_TRUE(perf_provider_->GetSampledProfiles(&stored_profiles3));
  ASSERT_EQ(1U, stored_profiles3.size());

  const SampledProfile& profile3 = stored_profiles3[0];
  EXPECT_EQ(SampledProfile::RESUME_FROM_SUSPEND, profile3.trigger_event());
  EXPECT_TRUE(profile3.has_ms_after_login());
  EXPECT_EQ(60000, profile3.suspend_duration_ms());
  EXPECT_EQ(1500, profile3.ms_after_resume());
  ASSERT_TRUE(profile3.has_perf_data());
  EXPECT_FALSE(profile3.has_perf_stat());
  EXPECT_EQ(SerializeMessageToVector(perf_data_proto_),
            SerializeMessageToVector(profile3.perf_data()));
}

TEST_F(PerfProviderTest, DefaultCommandsBasedOnUarch_IvyBridge) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 0x06;
  cpuid.model = 0x3a;  // IvyBridge
  cpuid.model_name = "";
  std::vector<RandomSelector::WeightAndValue> cmds =
      internal::GetDefaultCommandsForCpu(cpuid);
  ASSERT_GE(cmds.size(), 2UL);
  EXPECT_EQ(cmds[0].value, kPerfRecordCyclesCmd);
  EXPECT_EQ(cmds[1].value, kPerfRecordCallgraphCmd);
  auto found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfStatMemoryBandwidthCmd;
      });
  EXPECT_NE(cmds.end(), found);
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordLBRCmd;
      });
  EXPECT_NE(cmds.end(), found);
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordCacheMissesCmd;
      });
  EXPECT_NE(cmds.end(), found);
}

TEST_F(PerfProviderTest, DefaultCommandsBasedOnUarch_SandyBridge) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 0x06;
  cpuid.model = 0x2a;  // SandyBridge
  cpuid.model_name = "";
  std::vector<RandomSelector::WeightAndValue> cmds =
      internal::GetDefaultCommandsForCpu(cpuid);
  ASSERT_GE(cmds.size(), 2UL);
  EXPECT_EQ(cmds[0].value, kPerfRecordCyclesCmd);
  EXPECT_EQ(cmds[1].value, kPerfRecordCallgraphCmd);
  auto found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfStatMemoryBandwidthCmd;
      });
  EXPECT_EQ(cmds.end(), found) << "SandyBridge does not support this command";
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordLBRCmd;
      });
  EXPECT_NE(cmds.end(), found);
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordCacheMissesCmd;
      });
  EXPECT_NE(cmds.end(), found);
}

TEST_F(PerfProviderTest, DefaultCommandsBasedOnArch_Arm) {
  CPUIdentity cpuid;
  cpuid.arch = "armv7l";
  cpuid.vendor = "";
  cpuid.family = 0;
  cpuid.model = 0;
  cpuid.model_name = "";
  std::vector<RandomSelector::WeightAndValue> cmds =
      internal::GetDefaultCommandsForCpu(cpuid);
  ASSERT_GE(cmds.size(), 2UL);
  EXPECT_EQ(cmds[0].value, kPerfRecordCyclesCmd);
  EXPECT_EQ(cmds[1].value, kPerfRecordCallgraphCmd);
  auto found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordLBRCmd;
      });
  EXPECT_EQ(cmds.end(), found) << "ARM does not support this command";
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordCacheMissesCmd;
      });
  EXPECT_EQ(cmds.end(), found) << "ARM does not support this command";
}

TEST_F(PerfProviderTest, DefaultCommandsBasedOnArch_x86_32) {
  CPUIdentity cpuid;
  cpuid.arch = "x86";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 0x06;
  cpuid.model = 0x2f;  // Westmere
  cpuid.model_name = "";
  std::vector<RandomSelector::WeightAndValue> cmds =
      internal::GetDefaultCommandsForCpu(cpuid);
  ASSERT_GE(cmds.size(), 2UL);
  EXPECT_EQ(cmds[0].value, kPerfRecordCyclesCmd);
  EXPECT_EQ(cmds[1].value, kPerfRecordCallgraphCmd);
  auto found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfStatMemoryBandwidthCmd;
      });
  EXPECT_EQ(cmds.end(), found) << "x86_32 does not support this command";
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordLBRCmd;
      });
  EXPECT_EQ(cmds.end(), found) << "x86_32 does not support this command";
  found = std::find_if(
      cmds.begin(), cmds.end(),
      [](const RandomSelector::WeightAndValue& cmd) -> bool {
        return cmd.value == kPerfRecordCacheMissesCmd;
      });
  EXPECT_EQ(cmds.end(), found) << "x86_32 does not support this command";
}

TEST_F(PerfProviderTest, DefaultCommandsBasedOnArch_Unknown) {
  CPUIdentity cpuid;
  cpuid.arch = "nonsense";
  cpuid.vendor = "";
  cpuid.family = 0;
  cpuid.model = 0;
  cpuid.model_name = "";
  std::vector<RandomSelector::WeightAndValue> cmds =
      internal::GetDefaultCommandsForCpu(cpuid);
  EXPECT_EQ(1UL, cmds.size());
  EXPECT_EQ(cmds[0].value, kPerfRecordCyclesCmd);
}

TEST_F(PerfProviderTest, CommandMatching_Empty) {
  CPUIdentity cpuid = {};
  std::map<std::string, std::string> params;
  EXPECT_EQ("", internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_NoPerfCommands) {
  CPUIdentity cpuid = {};
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("NotEvenClose", ""));
  params.insert(std::make_pair("NotAPerfCommand", ""));
  params.insert(std::make_pair("NotAPerfCommand::Really", ""));
  params.insert(std::make_pair("NotAPerfCommand::Nope::0", ""));
  params.insert(std::make_pair("PerfCommands::SoClose::0", ""));
  EXPECT_EQ("", internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_NoMatch) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 6;
  cpuid.model = 0x3a;  // IvyBridge
  cpuid.model_name = "Xeon or somesuch";
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("PerfCommand::armv7l::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86::1", "perf command"));
  params.insert(std::make_pair("PerfCommand::Broadwell::0", "perf command"));

  EXPECT_EQ("", internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_default) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 6;
  cpuid.model = 0x3a;  // IvyBridge
  cpuid.model_name = "Xeon or somesuch";
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("PerfCommand::default::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::armv7l::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86::1", "perf command"));
  params.insert(std::make_pair("PerfCommand::Broadwell::0", "perf command"));

  EXPECT_EQ("default", internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_SystemArch) {
  CPUIdentity cpuid;
  cpuid.arch = "nothing_in_particular";
  cpuid.vendor = "";
  cpuid.family = 0;
  cpuid.model = 0;
  cpuid.model_name = "";
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("PerfCommand::default::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::armv7l::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86::1", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86_64::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86_64::xyz#$%", "perf command"));
  params.insert(std::make_pair("PerfCommand::Broadwell::0", "perf command"));

  EXPECT_EQ("default", internal::FindBestCpuSpecifierFromParams(params, cpuid));

  cpuid.arch = "armv7l";
  EXPECT_EQ("armv7l", internal::FindBestCpuSpecifierFromParams(params, cpuid));

  cpuid.arch = "x86";
  EXPECT_EQ("x86", internal::FindBestCpuSpecifierFromParams(params, cpuid));

  cpuid.arch = "x86_64";
  EXPECT_EQ("x86_64", internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_Microarchitecture) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 6;
  cpuid.model = 0x3D;  // Broadwell
  cpuid.model_name = "Wrong Model CPU @ 0 Hz";
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("PerfCommand::default::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86_64::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::Broadwell::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::interesting-model-500x::0",
                               "perf command"));

  EXPECT_EQ("Broadwell",
            internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_SpecificModel) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 6;
  cpuid.model = 0x3D;  // Broadwell
  cpuid.model_name = "An Interesting(R) Model(R) 500x CPU @ 1.2GHz";
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("PerfCommand::default::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86_64::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::Broadwell::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::interesting-model-500x::0",
                               "perf command"));

  EXPECT_EQ("interesting-model-500x",
            internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

TEST_F(PerfProviderTest, CommandMatching_SpecificModel_LongestMatch) {
  CPUIdentity cpuid;
  cpuid.arch = "x86_64";
  cpuid.vendor = "GenuineIntel";
  cpuid.family = 6;
  cpuid.model = 0x3D;  // Broadwell
  cpuid.model_name = "An Interesting(R) Model(R) 500x CPU @ 1.2GHz";
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("PerfCommand::default::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::x86_64::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::Broadwell::0", "perf command"));
  params.insert(std::make_pair("PerfCommand::model-500x::0",
                               "perf command"));
  params.insert(std::make_pair("PerfCommand::interesting-model-500x::0",
                               "perf command"));
  params.insert(std::make_pair("PerfCommand::interesting-model::0",
                               "perf command"));

  EXPECT_EQ("interesting-model-500x",
            internal::FindBestCpuSpecifierFromParams(params, cpuid));
}

class PerfProviderCollectionParamsTest : public testing::Test {
 public:
  PerfProviderCollectionParamsTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        task_runner_handle_(task_runner_),
        field_trial_list_(nullptr) {}

  void SetUp() override {
    // PerfProvider requires chromeos::LoginState and
    // chromeos::DBusThreadManagerto be initialized.
    chromeos::LoginState::Initialize();
    chromeos::DBusThreadManager::Initialize();

    // PerfProvider requires the user to be logged in.
    chromeos::LoginState::Get()->SetLoggedInState(
        chromeos::LoginState::LOGGED_IN_ACTIVE,
        chromeos::LoginState::LOGGED_IN_USER_REGULAR);
  }

  void TearDown() override {
    chromeos::DBusThreadManager::Shutdown();
    chromeos::LoginState::Shutdown();
    variations::testing::ClearAllVariationParams();
  }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  base::FieldTrialList field_trial_list_;

  DISALLOW_COPY_AND_ASSIGN(PerfProviderCollectionParamsTest);
};

TEST_F(PerfProviderCollectionParamsTest, Commands_InitializedAfterVariations) {
  TestPerfProvider perf_provider;
  EXPECT_TRUE(perf_provider.command_selector().odds().empty());
  // Init would be called after VariationsService is initialized.
  perf_provider.Init();
  EXPECT_FALSE(perf_provider.command_selector().odds().empty());
}

TEST_F(PerfProviderCollectionParamsTest, Commands_EmptyExperiment) {
  std::vector<RandomSelector::WeightAndValue> default_cmds =
      internal::GetDefaultCommandsForCpu(GetCPUIdentity());
  std::map<std::string, std::string> params;
  ASSERT_TRUE(variations::AssociateVariationParams(
      "ChromeOSWideProfilingCollection", "group_name", params));
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "ChromeOSWideProfilingCollection", "group_name"));

  TestPerfProvider perf_provider;
  EXPECT_TRUE(perf_provider.command_selector().odds().empty());
  perf_provider.Init();
  EXPECT_EQ(default_cmds, perf_provider.command_selector().odds());
}

TEST_F(PerfProviderCollectionParamsTest, Commands_InvalidValues) {
  std::vector<RandomSelector::WeightAndValue> default_cmds =
      internal::GetDefaultCommandsForCpu(GetCPUIdentity());
  std::map<std::string, std::string> params;
  // Use the "default" cpu specifier since we don't want to predict what CPU
  // this test is running on. (CPU detection is tested above.)
  params.insert(std::make_pair("PerfCommand::default::0", ""));
  params.insert(std::make_pair("PerfCommand::default::1", " "));
  params.insert(std::make_pair("PerfCommand::default::2", " leading space"));
  params.insert(std::make_pair("PerfCommand::default::3",
                               "no-spaces-or-numbers"));
  params.insert(std::make_pair("PerfCommand::default::4",
                               "NaN-trailing-space "));
  params.insert(std::make_pair("PerfCommand::default::5", "NaN x"));
  params.insert(std::make_pair("PerfCommand::default::6", "perf command"));
  ASSERT_TRUE(variations::AssociateVariationParams(
      "ChromeOSWideProfilingCollection", "group_name", params));
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "ChromeOSWideProfilingCollection", "group_name"));

  TestPerfProvider perf_provider;
  EXPECT_TRUE(perf_provider.command_selector().odds().empty());
  perf_provider.Init();
  EXPECT_EQ(default_cmds, perf_provider.command_selector().odds());
}

TEST_F(PerfProviderCollectionParamsTest, Commands_Override) {
  using WeightAndValue = RandomSelector::WeightAndValue;
  std::vector<RandomSelector::WeightAndValue> default_cmds =
      internal::GetDefaultCommandsForCpu(GetCPUIdentity());
  std::map<std::string, std::string> params;
  // Use the "default" cpu specifier since we don't want to predict what CPU
  // this test is running on. (CPU detection is tested above.)
  params.insert(std::make_pair("PerfCommand::default::0",
                               "50 perf record foo"));
  params.insert(std::make_pair("PerfCommand::default::1",
                               "25 perf record bar"));
  params.insert(std::make_pair("PerfCommand::default::2",
                               "25 perf record baz"));
  params.insert(std::make_pair("PerfCommand::another-cpu::0",
                               "7 perf record bar"));
  ASSERT_TRUE(variations::AssociateVariationParams(
      "ChromeOSWideProfilingCollection", "group_name", params));
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "ChromeOSWideProfilingCollection", "group_name"));

  TestPerfProvider perf_provider;
  EXPECT_TRUE(perf_provider.command_selector().odds().empty());
  perf_provider.Init();

  std::vector<WeightAndValue> expected_cmds;
  expected_cmds.push_back(WeightAndValue(50.0, "perf record foo"));
  expected_cmds.push_back(WeightAndValue(25.0, "perf record bar"));
  expected_cmds.push_back(WeightAndValue(25.0, "perf record baz"));

  EXPECT_EQ(expected_cmds, perf_provider.command_selector().odds());
}

TEST_F(PerfProviderCollectionParamsTest, Parameters_Override) {
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("ProfileCollectionDurationSec", "15"));
  params.insert(std::make_pair("PeriodicProfilingIntervalMs", "3600000"));
  params.insert(std::make_pair("ResumeFromSuspend::SamplingFactor", "1"));
  params.insert(std::make_pair("ResumeFromSuspend::MaxDelaySec", "10"));
  params.insert(std::make_pair("RestoreSession::SamplingFactor", "2"));
  params.insert(std::make_pair("RestoreSession::MaxDelaySec", "20"));
  ASSERT_TRUE(variations::AssociateVariationParams(
      "ChromeOSWideProfilingCollection", "group_name", params));
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "ChromeOSWideProfilingCollection", "group_name"));

  TestPerfProvider perf_provider;
  const auto& parsed_params = perf_provider.collection_params();

  // Not initialized yet:
  EXPECT_NE(base::TimeDelta::FromSeconds(15),
            parsed_params.collection_duration());
  EXPECT_NE(base::TimeDelta::FromHours(1),
            parsed_params.periodic_interval());
  EXPECT_NE(1, parsed_params.resume_from_suspend().sampling_factor());
  EXPECT_NE(base::TimeDelta::FromSeconds(10),
            parsed_params.resume_from_suspend().max_collection_delay());
  EXPECT_NE(2, parsed_params.restore_session().sampling_factor());
  EXPECT_NE(base::TimeDelta::FromSeconds(20),
            parsed_params.restore_session().max_collection_delay());

  perf_provider.Init();

  EXPECT_EQ(base::TimeDelta::FromSeconds(15),
            parsed_params.collection_duration());
  EXPECT_EQ(base::TimeDelta::FromHours(1),
            parsed_params.periodic_interval());
  EXPECT_EQ(1, parsed_params.resume_from_suspend().sampling_factor());
  EXPECT_EQ(base::TimeDelta::FromSeconds(10),
            parsed_params.resume_from_suspend().max_collection_delay());
  EXPECT_EQ(2, parsed_params.restore_session().sampling_factor());
  EXPECT_EQ(base::TimeDelta::FromSeconds(20),
            parsed_params.restore_session().max_collection_delay());
}

// Setting "::SamplingFactor" to zero should disable the trigger.
// Otherwise, it could cause a div-by-zero crash.
TEST_F(PerfProviderCollectionParamsTest, ZeroSamplingFactorDisablesTrigger) {
  std::map<std::string, std::string> params;
  params.insert(std::make_pair("ResumeFromSuspend::SamplingFactor", "0"));
  params.insert(std::make_pair("RestoreSession::SamplingFactor", "0"));
  ASSERT_TRUE(variations::AssociateVariationParams(
      "ChromeOSWideProfilingCollection", "group_name", params));
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "ChromeOSWideProfilingCollection", "group_name"));

  TestPerfProvider perf_provider;
  chromeos::PowerManagerClient::Observer& pm_observer = perf_provider;
  perf_provider.Init();

  // Cancel the background collection.
  perf_provider.Deactivate();
  EXPECT_FALSE(perf_provider.timer().IsRunning())
      << "Sanity: timer should not be running.";

  // Calling SuspendDone or OnSessionRestoreDone should not start the timer
  // that triggers collection.

  pm_observer.SuspendDone(base::TimeDelta::FromMinutes(10));
  EXPECT_FALSE(perf_provider.timer().IsRunning());

  perf_provider.OnSessionRestoreDone(100);
  EXPECT_FALSE(perf_provider.timer().IsRunning());
}

}  // namespace metrics
