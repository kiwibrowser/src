// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/call_stack_profile_metrics_provider.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/metrics_hashes.h"
#include "base/process/process_info.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"

using base::StackSamplingProfiler;

namespace metrics {

namespace {

// Provide a mapping from the C++ "enum" definition of various process mile-
// stones to the equivalent protobuf "enum" definition. This table-lookup
// conversion allows for the implementation to evolve and still be compatible
// with the protobuf -- even if there are ever more than 32 defined proto
// values, though never more than 32 could be in-use in a given C++ version
// of the code.
const ProcessPhase
    kProtoPhases[CallStackProfileMetricsProvider::MILESTONES_MAX_VALUE] = {
        ProcessPhase::MAIN_LOOP_START,
        ProcessPhase::MAIN_NAVIGATION_START,
        ProcessPhase::MAIN_NAVIGATION_FINISHED,
        ProcessPhase::FIRST_NONEMPTY_PAINT,

        ProcessPhase::SHUTDOWN_START,
};

// ProfilesState --------------------------------------------------------------

// A set of profiles and the CallStackProfileMetricsProvider state associated
// with them.
struct ProfilesState {
  ProfilesState(const CallStackProfileParams& params,
                base::TimeTicks start_timestamp,
                StackSamplingProfiler::CallStackProfiles profiles);
  ProfilesState(ProfilesState&&);
  ProfilesState& operator=(ProfilesState&&);

  // The metrics-related parameters provided to
  // CallStackProfileMetricsProvider::GetProfilerCallback().
  CallStackProfileParams params;

  // The time at which the profile collection was started.
  base::TimeTicks start_timestamp;

  // The call stack profiles collected by the profiler.
  StackSamplingProfiler::CallStackProfiles profiles;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfilesState);
};

ProfilesState::ProfilesState(const CallStackProfileParams& params,
                             base::TimeTicks start_timestamp,
                             StackSamplingProfiler::CallStackProfiles profiles)
    : params(params),
      start_timestamp(start_timestamp),
      profiles(std::move(profiles)) {}

ProfilesState::ProfilesState(ProfilesState&&) = default;

// Some versions of GCC need this for push_back to work with std::move.
ProfilesState& ProfilesState::operator=(ProfilesState&&) = default;

// PendingProfiles ------------------------------------------------------------

// Singleton class responsible for retaining profiles received via the callback
// created by GetProfilerCallback(). These are then sent to UMA on the
// invocation of CallStackProfileMetricsProvider::ProvideCurrentSessionData().
// We need to store the profiles outside of a CallStackProfileMetricsProvider
// instance since callers may start profiling before the
// CallStackProfileMetricsProvider is created.
//
// Member functions on this class may be called on any thread.
class PendingProfiles {
 public:
  static PendingProfiles* GetInstance();

  void Swap(std::vector<ProfilesState>* profiles);

  // Enables the collection of profiles by CollectProfilesIfCollectionEnabled if
  // |enabled| is true. Otherwise, clears current profiles and ignores profiles
  // provided to future invocations of CollectProfilesIfCollectionEnabled.
  void SetCollectionEnabled(bool enabled);

  // True if profiles are being collected.
  bool IsCollectionEnabled() const;

  // Adds |profiles| to the list of profiles if collection is enabled; it is
  // not const& because it must be passed with std::move.
  void CollectProfilesIfCollectionEnabled(ProfilesState profiles);

  // Allows testing against the initial state multiple times.
  void ResetToDefaultStateForTesting();

 private:
  friend struct base::DefaultSingletonTraits<PendingProfiles>;

  PendingProfiles();
  ~PendingProfiles();

  mutable base::Lock lock_;

  // If true, profiles provided to CollectProfilesIfCollectionEnabled should be
  // collected. Otherwise they will be ignored.
  bool collection_enabled_;

  // The last time collection was disabled. Used to determine if collection was
  // disabled at any point since a profile was started.
  base::TimeTicks last_collection_disable_time_;

  // The set of completed profiles that should be reported.
  std::vector<ProfilesState> profiles_;

  DISALLOW_COPY_AND_ASSIGN(PendingProfiles);
};

// static
PendingProfiles* PendingProfiles::GetInstance() {
  // Leaky for performance rather than correctness reasons.
  return base::Singleton<PendingProfiles,
                         base::LeakySingletonTraits<PendingProfiles>>::get();
}

void PendingProfiles::Swap(std::vector<ProfilesState>* profiles) {
  base::AutoLock scoped_lock(lock_);
  profiles_.swap(*profiles);
}

void PendingProfiles::SetCollectionEnabled(bool enabled) {
  base::AutoLock scoped_lock(lock_);

  collection_enabled_ = enabled;

  if (!collection_enabled_) {
    profiles_.clear();
    last_collection_disable_time_ = base::TimeTicks::Now();
  }
}

bool PendingProfiles::IsCollectionEnabled() const {
  base::AutoLock scoped_lock(lock_);
  return collection_enabled_;
}

void PendingProfiles::CollectProfilesIfCollectionEnabled(
    ProfilesState profiles) {
  base::AutoLock scoped_lock(lock_);

  // Only collect if collection is not disabled and hasn't been disabled
  // since the start of collection for this profile.
  if (!collection_enabled_ ||
      (!last_collection_disable_time_.is_null() &&
       last_collection_disable_time_ >= profiles.start_timestamp)) {
    return;
  }

  profiles_.push_back(std::move(profiles));
}

void PendingProfiles::ResetToDefaultStateForTesting() {
  base::AutoLock scoped_lock(lock_);

  collection_enabled_ = true;
  last_collection_disable_time_ = base::TimeTicks();
  profiles_.clear();
}

// |collection_enabled_| is initialized to true to collect any profiles that are
// generated prior to creation of the CallStackProfileMetricsProvider. The
// ultimate disposition of these pre-creation collected profiles will be
// determined by the initial recording state provided to
// CallStackProfileMetricsProvider.
PendingProfiles::PendingProfiles() : collection_enabled_(true) {}

PendingProfiles::~PendingProfiles() {}

// Functions to process completed profiles ------------------------------------

// Will be invoked on either the main thread or the profiler's thread. Provides
// the profiles to PendingProfiles to append, if the collecting state allows.
void ReceiveCompletedProfilesImpl(
    const CallStackProfileParams& params,
    base::TimeTicks start_timestamp,
    StackSamplingProfiler::CallStackProfiles profiles) {
  PendingProfiles::GetInstance()->CollectProfilesIfCollectionEnabled(
      ProfilesState(params, start_timestamp, std::move(profiles)));
}

// Invoked on an arbitrary thread. Ignores the provided profiles.
void IgnoreCompletedProfiles(
    StackSamplingProfiler::CallStackProfiles profiles) {}

// Functions to encode protobufs ----------------------------------------------

// The protobuf expects the MD5 checksum prefix of the module name.
uint64_t HashModuleFilename(const base::FilePath& filename) {
  const base::FilePath::StringType basename = filename.BaseName().value();
  // Copy the bytes in basename into a string buffer.
  size_t basename_length_in_bytes =
      basename.size() * sizeof(base::FilePath::CharType);
  std::string name_bytes(basename_length_in_bytes, '\0');
  memcpy(&name_bytes[0], &basename[0], basename_length_in_bytes);
  return base::HashMetricName(name_bytes);
}

// Transcode |sample| into |proto_sample|, using base addresses in |modules| to
// compute module instruction pointer offsets.
void CopySampleToProto(
    const StackSamplingProfiler::Sample& sample,
    const std::vector<StackSamplingProfiler::Module>& modules,
    CallStackProfile::Sample* proto_sample) {
  for (const StackSamplingProfiler::Frame& frame : sample.frames) {
    CallStackProfile::Entry* entry = proto_sample->add_entry();
    // A frame may not have a valid module. If so, we can't compute the
    // instruction pointer offset, and we don't want to send bare pointers, so
    // leave call_stack_entry empty.
    if (frame.module_index == StackSamplingProfiler::Frame::kUnknownModuleIndex)
      continue;
    int64_t module_offset =
        reinterpret_cast<const char*>(frame.instruction_pointer) -
        reinterpret_cast<const char*>(modules[frame.module_index].base_address);
    DCHECK_GE(module_offset, 0);
    entry->set_address(static_cast<uint64_t>(module_offset));
    entry->set_module_id_index(frame.module_index);
  }
}

// Transcode Sample annotations into protobuf fields. The C++ code uses a bit-
// field with each bit corresponding to an entry in an enumeration while the
// protobuf uses a repeated field of individual values. Conversion tables
// allow for arbitrary mapping, though no more than 32 in any given version
// of the code.
void CopyAnnotationsToProto(uint32_t new_milestones,
                            CallStackProfile::Sample* sample_proto) {
  for (size_t bit = 0; new_milestones != 0 && bit < sizeof(new_milestones) * 8;
       ++bit) {
    const uint32_t flag = 1U << bit;
    if (new_milestones & flag) {
      if (bit >= arraysize(kProtoPhases)) {
        NOTREACHED();
        continue;
      }
      sample_proto->add_process_phase(kProtoPhases[bit]);
      new_milestones ^= flag;  // Bit is set so XOR will clear it.
    }
  }
}

// Transcode |profile| into |proto_profile|.
void CopyProfileToProto(
    const StackSamplingProfiler::CallStackProfile& profile,
    CallStackProfileParams::SampleOrderingSpec ordering_spec,
    CallStackProfile* proto_profile) {
  if (profile.samples.empty())
    return;

  const bool preserve_order =
      (ordering_spec == CallStackProfileParams::PRESERVE_ORDER);

  std::map<StackSamplingProfiler::Sample, int> sample_index;
  uint32_t milestones = 0;
  for (auto it = profile.samples.begin(); it != profile.samples.end(); ++it) {
    int existing_sample_index = -1;
    if (preserve_order) {
      // Collapse sample with the previous one if they match. Samples match
      // if the frame and all annotations are the same.
      if (proto_profile->sample_size() > 0 && *it == *(it - 1))
        existing_sample_index = proto_profile->sample_size() - 1;
    } else {
      auto location = sample_index.find(*it);
      if (location != sample_index.end())
        existing_sample_index = location->second;
    }

    if (existing_sample_index != -1) {
      CallStackProfile::Sample* sample_proto =
          proto_profile->mutable_sample()->Mutable(existing_sample_index);
      sample_proto->set_count(sample_proto->count() + 1);
      continue;
    }

    CallStackProfile::Sample* sample_proto = proto_profile->add_sample();
    CopySampleToProto(*it, profile.modules, sample_proto);
    sample_proto->set_count(1);
    CopyAnnotationsToProto(it->process_milestones & ~milestones, sample_proto);
    milestones = it->process_milestones;

    if (!preserve_order) {
      sample_index.insert(std::make_pair(
          *it, static_cast<int>(proto_profile->sample_size()) - 1));
    }
  }

  for (const StackSamplingProfiler::Module& module : profile.modules) {
    CallStackProfile::ModuleIdentifier* module_id =
        proto_profile->add_module_id();
    module_id->set_build_id(module.id);
    module_id->set_name_md5_prefix(HashModuleFilename(module.filename));
  }

  proto_profile->set_profile_duration_ms(
      profile.profile_duration.InMilliseconds());
  proto_profile->set_sampling_period_ms(
      profile.sampling_period.InMilliseconds());
}

// Translates CallStackProfileParams's process to the corresponding
// execution context Process.
Process ToExecutionContextProcess(CallStackProfileParams::Process process) {
  switch (process) {
    case CallStackProfileParams::UNKNOWN_PROCESS:
      return UNKNOWN_PROCESS;
    case CallStackProfileParams::BROWSER_PROCESS:
      return BROWSER_PROCESS;
    case CallStackProfileParams::RENDERER_PROCESS:
      return RENDERER_PROCESS;
    case CallStackProfileParams::GPU_PROCESS:
      return GPU_PROCESS;
    case CallStackProfileParams::UTILITY_PROCESS:
      return UTILITY_PROCESS;
    case CallStackProfileParams::ZYGOTE_PROCESS:
      return ZYGOTE_PROCESS;
    case CallStackProfileParams::SANDBOX_HELPER_PROCESS:
      return SANDBOX_HELPER_PROCESS;
    case CallStackProfileParams::PPAPI_PLUGIN_PROCESS:
      return PPAPI_PLUGIN_PROCESS;
    case CallStackProfileParams::PPAPI_BROKER_PROCESS:
      return PPAPI_BROKER_PROCESS;
  }
  NOTREACHED();
  return UNKNOWN_PROCESS;
}

// Translates CallStackProfileParams's thread to the corresponding
// SampledProfile Thread.
Thread ToExecutionContextThread(CallStackProfileParams::Thread thread) {
  switch (thread) {
    case CallStackProfileParams::UNKNOWN_THREAD:
      return UNKNOWN_THREAD;
    case CallStackProfileParams::MAIN_THREAD:
      return UI_THREAD;
    case CallStackProfileParams::IO_THREAD:
      return IO_THREAD;
    case CallStackProfileParams::COMPOSITOR_THREAD:
      return COMPOSITOR_THREAD;
  }
  NOTREACHED();
  return UNKNOWN_THREAD;
}

// Translates CallStackProfileParams's trigger to the corresponding
// SampledProfile TriggerEvent.
SampledProfile::TriggerEvent ToSampledProfileTriggerEvent(
    CallStackProfileParams::Trigger trigger) {
  switch (trigger) {
    case CallStackProfileParams::UNKNOWN:
      return SampledProfile::UNKNOWN_TRIGGER_EVENT;
    case CallStackProfileParams::PROCESS_STARTUP:
      return SampledProfile::PROCESS_STARTUP;
    case CallStackProfileParams::JANKY_TASK:
      return SampledProfile::JANKY_TASK;
    case CallStackProfileParams::THREAD_HUNG:
      return SampledProfile::THREAD_HUNG;
    case CallStackProfileParams::PERIODIC_COLLECTION:
      return SampledProfile::PERIODIC_COLLECTION;
  }
  NOTREACHED();
  return SampledProfile::UNKNOWN_TRIGGER_EVENT;
}

}  // namespace

// CallStackProfileMetricsProvider --------------------------------------------

const base::Feature CallStackProfileMetricsProvider::kEnableReporting = {
    "SamplingProfilerReporting", base::FEATURE_DISABLED_BY_DEFAULT};

CallStackProfileMetricsProvider::CallStackProfileMetricsProvider() {
}

CallStackProfileMetricsProvider::~CallStackProfileMetricsProvider() {
}

StackSamplingProfiler::CompletedCallback
CallStackProfileMetricsProvider::GetProfilerCallbackForBrowserProcess(
    const CallStackProfileParams& params) {
  // Ignore the profiles if the collection is disabled. If the collection state
  // changes while collecting, this will be detected by the callback and
  // profiles will be ignored at that point.
  if (!PendingProfiles::GetInstance()->IsCollectionEnabled())
    return base::Bind(&IgnoreCompletedProfiles);

  return base::Bind(&ReceiveCompletedProfilesImpl, params,
                    base::TimeTicks::Now());
}

// static
void CallStackProfileMetricsProvider::ReceiveCompletedProfiles(
    const CallStackProfileParams& params,
    base::TimeTicks profile_start_time,
    base::StackSamplingProfiler::CallStackProfiles profiles) {
  ReceiveCompletedProfilesImpl(params, profile_start_time, std::move(profiles));
}

void CallStackProfileMetricsProvider::OnRecordingEnabled() {
  PendingProfiles::GetInstance()->SetCollectionEnabled(
      base::FeatureList::IsEnabled(kEnableReporting));
}

void CallStackProfileMetricsProvider::OnRecordingDisabled() {
  PendingProfiles::GetInstance()->SetCollectionEnabled(false);
}

void CallStackProfileMetricsProvider::ProvideCurrentSessionData(
    ChromeUserMetricsExtension* uma_proto) {
  std::vector<ProfilesState> pending_profiles;
  PendingProfiles::GetInstance()->Swap(&pending_profiles);

  DCHECK(base::FeatureList::IsEnabled(kEnableReporting) ||
         pending_profiles.empty());

  for (const ProfilesState& profiles_state : pending_profiles) {
    for (const StackSamplingProfiler::CallStackProfile& profile :
         profiles_state.profiles) {
      SampledProfile* sampled_profile = uma_proto->add_sampled_profile();
      sampled_profile->set_process(
          ToExecutionContextProcess(profiles_state.params.process));
      sampled_profile->set_thread(
          ToExecutionContextThread(profiles_state.params.thread));
      sampled_profile->set_trigger_event(
          ToSampledProfileTriggerEvent(profiles_state.params.trigger));
      CopyProfileToProto(profile, profiles_state.params.ordering_spec,
                         sampled_profile->mutable_call_stack_profile());
    }
  }
}

// static
void CallStackProfileMetricsProvider::ResetStaticStateForTesting() {
  PendingProfiles::GetInstance()->ResetToDefaultStateForTesting();
}

}  // namespace metrics
