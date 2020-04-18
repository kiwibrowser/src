// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/call_stack_profile_metrics_provider.h"

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "base/macros.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "components/metrics/call_stack_profile_params.h"
#include "components/variations/entropy_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"

using base::StackSamplingProfiler;
using Frame = StackSamplingProfiler::Frame;
using Module = StackSamplingProfiler::Module;
using Profile = StackSamplingProfiler::CallStackProfile;
using Profiles = StackSamplingProfiler::CallStackProfiles;
using Sample = StackSamplingProfiler::Sample;

namespace {

struct ExpectedProtoModule {
  const char* build_id;
  uint64_t name_md5;
  uint64_t base_address;
};

struct ExpectedProtoEntry {
  int32_t module_index;
  uint64_t address;
};

struct ExpectedProtoSample {
  uint32_t process_milestones;  // Bit-field of expected milestones.
  const ExpectedProtoEntry* entries;
  int entry_count;
  int64_t entry_repeats;
};

struct ExpectedProtoProfile {
  int32_t duration_ms;
  int32_t period_ms;
  const ExpectedProtoModule* modules;
  int module_count;
  const ExpectedProtoSample* samples;
  int sample_count;
};

class ProfilesFactory {
 public:
  ProfilesFactory() {}
  ~ProfilesFactory() {}

  ProfilesFactory& AddMilestone(int milestone);
  ProfilesFactory& NewProfile(int duration_ms, int interval_ms);
  ProfilesFactory& NewSample();
  ProfilesFactory& AddFrame(size_t module, uintptr_t offset);
  ProfilesFactory& DefineModule(const char* name,
                                const base::FilePath& path,
                                uintptr_t base);

  Profiles Build();

 private:
  Profiles profiles_;
  uint32_t process_milestones_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ProfilesFactory);
};

ProfilesFactory& ProfilesFactory::AddMilestone(int milestone) {
  process_milestones_ |= 1 << milestone;
  return *this;
}

ProfilesFactory& ProfilesFactory::NewProfile(int duration_ms, int interval_ms) {
  profiles_.push_back(Profile());
  Profile* profile = &profiles_.back();
  profile->profile_duration = base::TimeDelta::FromMilliseconds(duration_ms);
  profile->sampling_period = base::TimeDelta::FromMilliseconds(interval_ms);
  return *this;
}

ProfilesFactory& ProfilesFactory::NewSample() {
  profiles_.back().samples.push_back(Sample());
  profiles_.back().samples.back().process_milestones = process_milestones_;
  return *this;
}

ProfilesFactory& ProfilesFactory::AddFrame(size_t module, uintptr_t offset) {
  profiles_.back().samples.back().frames.push_back(Frame(offset, module));
  return *this;
}

ProfilesFactory& ProfilesFactory::DefineModule(const char* name,
                                               const base::FilePath& path,
                                               uintptr_t base) {
  profiles_.back().modules.push_back(Module(base, name, path));
  return *this;
}

Profiles ProfilesFactory::Build() {
  return std::move(profiles_);
}

}  // namespace

namespace metrics {

// This test fixture enables the feature that
// CallStackProfileMetricsProvider depends on to report profiles.
class CallStackProfileMetricsProviderTest : public testing::Test {
 public:
  CallStackProfileMetricsProviderTest() {
    scoped_feature_list_.InitAndEnableFeature(TestState::kEnableReporting);
    TestState::ResetStaticStateForTesting();
  }

  ~CallStackProfileMetricsProviderTest() override {}

  // Utility function to append profiles to the metrics provider.
  void AppendProfiles(const CallStackProfileParams& params, Profiles profiles) {
    CallStackProfileMetricsProvider::GetProfilerCallbackForBrowserProcess(
        params)
        .Run(std::move(profiles));
  }

  void VerifyProfileProto(const ExpectedProtoProfile& expected,
                          const SampledProfile& proto);

 private:
  // Exposes the feature from the CallStackProfileMetricsProvider.
  class TestState : public CallStackProfileMetricsProvider {
   public:
    using CallStackProfileMetricsProvider::kEnableReporting;
    using CallStackProfileMetricsProvider::ResetStaticStateForTesting;
  };

  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(CallStackProfileMetricsProviderTest);
};

void CallStackProfileMetricsProviderTest::VerifyProfileProto(
    const ExpectedProtoProfile& expected,
    const SampledProfile& proto) {
  ASSERT_TRUE(proto.has_call_stack_profile());
  const CallStackProfile& stack = proto.call_stack_profile();

  ASSERT_TRUE(stack.has_profile_duration_ms());
  EXPECT_EQ(expected.duration_ms, stack.profile_duration_ms());
  ASSERT_TRUE(stack.has_sampling_period_ms());
  EXPECT_EQ(expected.period_ms, stack.sampling_period_ms());

  ASSERT_EQ(expected.module_count, stack.module_id().size());
  for (int m = 0; m < expected.module_count; ++m) {
    SCOPED_TRACE("module " + base::IntToString(m));
    const CallStackProfile::ModuleIdentifier& module_id =
        stack.module_id().Get(m);
    ASSERT_TRUE(module_id.has_build_id());
    EXPECT_EQ(expected.modules[m].build_id, module_id.build_id());
    ASSERT_TRUE(module_id.has_name_md5_prefix());
    EXPECT_EQ(expected.modules[m].name_md5, module_id.name_md5_prefix());
  }

  ASSERT_EQ(expected.sample_count, stack.sample().size());
  for (int s = 0; s < expected.sample_count; ++s) {
    SCOPED_TRACE("sample " + base::IntToString(s));
    const CallStackProfile::Sample& proto_sample = stack.sample().Get(s);

    uint32_t process_milestones = 0;
    for (int i = 0; i < proto_sample.process_phase().size(); ++i)
      process_milestones |= 1U << proto_sample.process_phase().Get(i);
    EXPECT_EQ(expected.samples[s].process_milestones, process_milestones);

    ASSERT_EQ(expected.samples[s].entry_count, proto_sample.entry().size());
    ASSERT_TRUE(proto_sample.has_count());
    EXPECT_EQ(expected.samples[s].entry_repeats, proto_sample.count());
    for (int e = 0; e < expected.samples[s].entry_count; ++e) {
      SCOPED_TRACE("entry " + base::NumberToString(e));
      const CallStackProfile::Entry& entry = proto_sample.entry().Get(e);
      if (expected.samples[s].entries[e].module_index >= 0) {
        ASSERT_TRUE(entry.has_module_id_index());
        EXPECT_EQ(expected.samples[s].entries[e].module_index,
                  entry.module_id_index());
        ASSERT_TRUE(entry.has_address());
        EXPECT_EQ(expected.samples[s].entries[e].address, entry.address());
      } else {
        EXPECT_FALSE(entry.has_module_id_index());
        EXPECT_FALSE(entry.has_address());
      }
    }
  }
}

// Checks that all properties from multiple profiles are filled as expected.
TEST_F(CallStackProfileMetricsProviderTest, MultipleProfiles) {
  const uintptr_t moduleA_base_address = 0x1000;
  const uintptr_t moduleB_base_address = 0x2000;
  const uintptr_t moduleC_base_address = 0x3000;
  const char* moduleA_name = "ABCD";
  const char* moduleB_name = "EFGH";
  const char* moduleC_name = "MNOP";

  // Values for Windows generated with:
  // perl -MDigest::MD5=md5 -MEncode=encode
  //     -e 'for(@ARGV){printf "%x\n", unpack "Q>", md5 encode "UTF-16LE", $_}'
  //     chrome.exe third_party.dll third_party2.dll
  //
  // Values for Linux generated with:
  // perl -MDigest::MD5=md5
  //     -e 'for(@ARGV){printf "%x\n", unpack "Q>", md5 $_}'
  //     chrome third_party.so third_party2.so
#if defined(OS_WIN)
  uint64_t moduleA_md5 = 0x46C3E4166659AC02ULL;
  uint64_t moduleB_md5 = 0x7E2B8BFDDEAE1ABAULL;
  uint64_t moduleC_md5 = 0x87B66F4573A4D5CAULL;
  base::FilePath moduleA_path(L"c:\\some\\path\\to\\chrome.exe");
  base::FilePath moduleB_path(L"c:\\some\\path\\to\\third_party.dll");
  base::FilePath moduleC_path(L"c:\\some\\path\\to\\third_party2.dll");
#else
  uint64_t moduleA_md5 = 0x554838A8451AC36CULL;
  uint64_t moduleB_md5 = 0x843661148659C9F8ULL;
  uint64_t moduleC_md5 = 0xB4647E539FA6EC9EULL;
  base::FilePath moduleA_path("/some/path/to/chrome");
  base::FilePath moduleB_path("/some/path/to/third_party.so");
  base::FilePath moduleC_path("/some/path/to/third_party2.so");
#endif

  // Represents two stack samples for each of two profiles, where each stack
  // contains three frames. Each frame contains an instruction pointer and a
  // module index corresponding to the module for the profile in
  // profile_modules.
  //
  // So, the first stack sample below has its top frame in module 0 at an offset
  // of 0x10 from the module's base address, the next-to-top frame in module 1
  // at an offset of 0x20 from the module's base address, and the bottom frame
  // in module 0 at an offset of 0x30 from the module's base address
  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .DefineModule(moduleA_name, moduleA_path, moduleA_base_address)
      .DefineModule(moduleB_name, moduleB_path, moduleB_base_address)

      .NewSample()
      .AddFrame(0, moduleA_base_address + 0x10)
      .AddFrame(1, moduleB_base_address + 0x20)
      .AddFrame(0, moduleA_base_address + 0x30)
      .NewSample()
      .AddFrame(1, moduleB_base_address + 0x10)
      .AddFrame(0, moduleA_base_address + 0x20)
      .AddFrame(1, moduleB_base_address + 0x30)

      .NewProfile(200, 20)
      .DefineModule(moduleC_name, moduleC_path, moduleC_base_address)
      .DefineModule(moduleA_name, moduleA_path, moduleA_base_address)

      .NewSample()
      .AddFrame(0, moduleC_base_address + 0x10)
      .AddFrame(1, moduleA_base_address + 0x20)
      .AddFrame(0, moduleC_base_address + 0x30)
      .NewSample()
      .AddFrame(1, moduleA_base_address + 0x10)
      .AddFrame(0, moduleC_base_address + 0x20)
      .AddFrame(1, moduleA_base_address + 0x30)

      .Build();

  const ExpectedProtoModule expected_proto_modules1[] = {
      { moduleA_name, moduleA_md5, moduleA_base_address },
      { moduleB_name, moduleB_md5, moduleB_base_address }
  };

  const ExpectedProtoEntry expected_proto_entries11[] = {
      { 0, 0x10 },
      { 1, 0x20 },
      { 0, 0x30 },
  };
  const ExpectedProtoEntry expected_proto_entries12[] = {
      { 1, 0x10 },
      { 0, 0x20 },
      { 1, 0x30 },
  };
  const ExpectedProtoSample expected_proto_samples1[] = {
      {
          0, expected_proto_entries11, arraysize(expected_proto_entries11), 1,
      },
      {
          0, expected_proto_entries12, arraysize(expected_proto_entries12), 1,
      },
  };

  const ExpectedProtoModule expected_proto_modules2[] = {
      { moduleC_name, moduleC_md5, moduleC_base_address },
      { moduleA_name, moduleA_md5, moduleA_base_address }
  };

  const ExpectedProtoEntry expected_proto_entries21[] = {
      { 0, 0x10 },
      { 1, 0x20 },
      { 0, 0x30 },
  };
  const ExpectedProtoEntry expected_proto_entries22[] = {
      { 1, 0x10 },
      { 0, 0x20 },
      { 1, 0x30 },
  };
  const ExpectedProtoSample expected_proto_samples2[] = {
      {
          0, expected_proto_entries11, arraysize(expected_proto_entries21), 1,
      },
      {
          0, expected_proto_entries12, arraysize(expected_proto_entries22), 1,
      },
  };

  const ExpectedProtoProfile expected_proto_profiles[] = {
      {
          100, 10,
          expected_proto_modules1, arraysize(expected_proto_modules1),
          expected_proto_samples1, arraysize(expected_proto_samples1),
      },
      {
          200, 20,
          expected_proto_modules2, arraysize(expected_proto_modules2),
          expected_proto_samples2, arraysize(expected_proto_samples2),
      },
  };

  ASSERT_EQ(arraysize(expected_proto_profiles), profiles.size());

  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  AppendProfiles(params, std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  ASSERT_EQ(static_cast<int>(arraysize(expected_proto_profiles)),
            uma_proto.sampled_profile().size());
  for (size_t p = 0; p < arraysize(expected_proto_profiles); ++p) {
    SCOPED_TRACE("profile " + base::NumberToString(p));
    VerifyProfileProto(expected_proto_profiles[p],
                       uma_proto.sampled_profile().Get(p));
  }
}

// Checks that all duplicate samples are collapsed with
// preserve_sample_ordering = false.
TEST_F(CallStackProfileMetricsProviderTest, RepeatedStacksUnordered) {
  const uintptr_t module_base_address = 0x1000;
  const char* module_name = "ABCD";

#if defined(OS_WIN)
  uint64_t module_md5 = 0x46C3E4166659AC02ULL;
  base::FilePath module_path(L"c:\\some\\path\\to\\chrome.exe");
#else
  uint64_t module_md5 = 0x554838A8451AC36CULL;
  base::FilePath module_path("/some/path/to/chrome");
#endif

  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .DefineModule(module_name, module_path, module_base_address)

      .AddMilestone(0)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x20)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)

      .AddMilestone(1)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x20)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)

      .Build();

  const ExpectedProtoModule expected_proto_modules[] = {
      { module_name, module_md5, module_base_address },
  };

  const ExpectedProtoEntry expected_proto_entries[] = {
      { 0, 0x10 },
      { 0, 0x20 },
  };
  const ExpectedProtoSample expected_proto_samples[] = {
      { 1, &expected_proto_entries[0], 1, 3 },
      { 0, &expected_proto_entries[1], 1, 1 },
      { 2, &expected_proto_entries[0], 1, 3 },
      { 0, &expected_proto_entries[1], 1, 1 },
  };

  const ExpectedProtoProfile expected_proto_profiles[] = {
      {
          100, 10,
          expected_proto_modules, arraysize(expected_proto_modules),
          expected_proto_samples, arraysize(expected_proto_samples),
      },
  };

  ASSERT_EQ(arraysize(expected_proto_profiles), profiles.size());

  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  AppendProfiles(params, std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  ASSERT_EQ(static_cast<int>(arraysize(expected_proto_profiles)),
            uma_proto.sampled_profile().size());
  for (size_t p = 0; p < arraysize(expected_proto_profiles); ++p) {
    SCOPED_TRACE("profile " + base::NumberToString(p));
    VerifyProfileProto(expected_proto_profiles[p],
                       uma_proto.sampled_profile().Get(p));
  }
}

// Checks that only contiguous duplicate samples are collapsed with
// preserve_sample_ordering = true.
TEST_F(CallStackProfileMetricsProviderTest, RepeatedStacksOrdered) {
  const uintptr_t module_base_address = 0x1000;
  const char* module_name = "ABCD";

#if defined(OS_WIN)
  uint64_t module_md5 = 0x46C3E4166659AC02ULL;
  base::FilePath module_path(L"c:\\some\\path\\to\\chrome.exe");
#else
  uint64_t module_md5 = 0x554838A8451AC36CULL;
  base::FilePath module_path("/some/path/to/chrome");
#endif

  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .DefineModule(module_name, module_path, module_base_address)

      .AddMilestone(0)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x20)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)

      .AddMilestone(1)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x20)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)
      .NewSample()
      .AddFrame(0, module_base_address + 0x10)

      .Build();

  const ExpectedProtoModule expected_proto_modules[] = {
      { module_name, module_md5, module_base_address },
  };

  const ExpectedProtoEntry expected_proto_entries[] = {
      { 0, 0x10 },
      { 0, 0x20 },
  };
  const ExpectedProtoSample expected_proto_samples[] = {
      { 1, &expected_proto_entries[0], 1, 1 },
      { 0, &expected_proto_entries[1], 1, 1 },
      { 0, &expected_proto_entries[0], 1, 2 },
      { 2, &expected_proto_entries[0], 1, 1 },
      { 0, &expected_proto_entries[1], 1, 1 },
      { 0, &expected_proto_entries[0], 1, 2 },
  };

  const ExpectedProtoProfile expected_proto_profiles[] = {
      {
          100, 10,
          expected_proto_modules, arraysize(expected_proto_modules),
          expected_proto_samples, arraysize(expected_proto_samples),
      },
  };

  ASSERT_EQ(arraysize(expected_proto_profiles), profiles.size());

  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::PRESERVE_ORDER);
  AppendProfiles(params, std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  ASSERT_EQ(static_cast<int>(arraysize(expected_proto_profiles)),
            uma_proto.sampled_profile().size());
  for (size_t p = 0; p < arraysize(expected_proto_profiles); ++p) {
    SCOPED_TRACE("profile " + base::NumberToString(p));
    VerifyProfileProto(expected_proto_profiles[p],
                       uma_proto.sampled_profile().Get(p));
  }
}

// Checks that unknown modules produce an empty Entry.
TEST_F(CallStackProfileMetricsProviderTest, UnknownModule) {
  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .NewSample()
      .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
      .Build();

  const ExpectedProtoEntry expected_proto_entries[] = {
      { -1, 0 },
  };
  const ExpectedProtoSample expected_proto_samples[] = {
      { 0, &expected_proto_entries[0], 1, 1 },
  };

  const ExpectedProtoProfile expected_proto_profiles[] = {
      {
          100, 10,
          nullptr, 0,
          expected_proto_samples, arraysize(expected_proto_samples),
      },
  };

  ASSERT_EQ(arraysize(expected_proto_profiles), profiles.size());

  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  AppendProfiles(params, std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  ASSERT_EQ(static_cast<int>(arraysize(expected_proto_profiles)),
            uma_proto.sampled_profile().size());
  for (size_t p = 0; p < arraysize(expected_proto_profiles); ++p) {
    SCOPED_TRACE("profile " + base::NumberToString(p));
    VerifyProfileProto(expected_proto_profiles[p],
                       uma_proto.sampled_profile().Get(p));
  }
}

// Checks that pending profiles are only passed back to
// ProvideCurrentSessionData once.
TEST_F(CallStackProfileMetricsProviderTest, ProfilesProvidedOnlyOnce) {
  CallStackProfileMetricsProvider provider;
  for (int r = 0; r < 2; ++r) {
    Profiles profiles = ProfilesFactory()
        // Use the sampling period to distinguish the two profiles.
        .NewProfile(100, r)
        .NewSample()
        .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
        .Build();

    ASSERT_EQ(1U, profiles.size());

    CallStackProfileMetricsProvider provider;
    provider.OnRecordingEnabled();
    CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                  CallStackProfileParams::MAIN_THREAD,
                                  CallStackProfileParams::PROCESS_STARTUP,
                                  CallStackProfileParams::MAY_SHUFFLE);
    AppendProfiles(params, std::move(profiles));
    ChromeUserMetricsExtension uma_proto;
    provider.ProvideCurrentSessionData(&uma_proto);

    ASSERT_EQ(1, uma_proto.sampled_profile().size());
    const SampledProfile& sampled_profile = uma_proto.sampled_profile().Get(0);
    ASSERT_TRUE(sampled_profile.has_call_stack_profile());
    const CallStackProfile& call_stack_profile =
        sampled_profile.call_stack_profile();
    ASSERT_TRUE(call_stack_profile.has_sampling_period_ms());
    EXPECT_EQ(r, call_stack_profile.sampling_period_ms());
  }
}

// Checks that pending profiles are provided to ProvideCurrentSessionData
// when collected before CallStackProfileMetricsProvider is instantiated.
TEST_F(CallStackProfileMetricsProviderTest,
       ProfilesProvidedWhenCollectedBeforeInstantiation) {
  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .NewSample()
      .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
      .Build();

  ASSERT_EQ(1U, profiles.size());

  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  AppendProfiles(params, std::move(profiles));

  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  EXPECT_EQ(1, uma_proto.sampled_profile_size());
}

// Checks that pending profiles are not provided to ProvideCurrentSessionData
// while recording is disabled.
TEST_F(CallStackProfileMetricsProviderTest, ProfilesNotProvidedWhileDisabled) {
  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .NewSample()
      .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
      .Build();

  ASSERT_EQ(1U, profiles.size());

  CallStackProfileMetricsProvider provider;
  provider.OnRecordingDisabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  AppendProfiles(params, std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  EXPECT_EQ(0, uma_proto.sampled_profile_size());
}

// Checks that pending profiles are not provided to ProvideCurrentSessionData
// if recording is disabled while profiling.
TEST_F(CallStackProfileMetricsProviderTest,
       ProfilesNotProvidedAfterChangeToDisabled) {
  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  base::StackSamplingProfiler::CompletedCallback callback =
      CallStackProfileMetricsProvider::GetProfilerCallbackForBrowserProcess(
          params);
  provider.OnRecordingDisabled();

  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .NewSample()
      .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
      .Build();
  callback.Run(std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  EXPECT_EQ(0, uma_proto.sampled_profile_size());
}

// Checks that pending profiles are not provided to ProvideCurrentSessionData if
// recording is enabled, but then disabled and reenabled while profiling.
TEST_F(CallStackProfileMetricsProviderTest,
       ProfilesNotProvidedAfterChangeToDisabledThenEnabled) {
  CallStackProfileMetricsProvider provider;
  provider.OnRecordingEnabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  base::StackSamplingProfiler::CompletedCallback callback =
      CallStackProfileMetricsProvider::GetProfilerCallbackForBrowserProcess(
          params);
  provider.OnRecordingDisabled();
  provider.OnRecordingEnabled();

  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .NewSample()
      .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
      .Build();
  callback.Run(std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  EXPECT_EQ(0, uma_proto.sampled_profile_size());
}

// Checks that pending profiles are not provided to ProvideCurrentSessionData
// if recording is disabled, but then enabled while profiling.
TEST_F(CallStackProfileMetricsProviderTest,
       ProfilesNotProvidedAfterChangeFromDisabled) {
  CallStackProfileMetricsProvider provider;
  provider.OnRecordingDisabled();
  CallStackProfileParams params(CallStackProfileParams::BROWSER_PROCESS,
                                CallStackProfileParams::MAIN_THREAD,
                                CallStackProfileParams::PROCESS_STARTUP,
                                CallStackProfileParams::MAY_SHUFFLE);
  base::StackSamplingProfiler::CompletedCallback callback =
      CallStackProfileMetricsProvider::GetProfilerCallbackForBrowserProcess(
          params);
  provider.OnRecordingEnabled();

  Profiles profiles = ProfilesFactory()
      .NewProfile(100, 10)
      .NewSample()
      .AddFrame(Frame::kUnknownModuleIndex, 0x1234)
      .Build();
  callback.Run(std::move(profiles));
  ChromeUserMetricsExtension uma_proto;
  provider.ProvideCurrentSessionData(&uma_proto);

  EXPECT_EQ(0, uma_proto.sampled_profile_size());
}

}  // namespace metrics
