// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/perf/perf_provider_chromeos.h"

#include <stddef.h>
#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/sys_info.h"
#include "chrome/browser/metrics/perf/windowed_incognito_observer.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/channel_info.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/variations/variations_associated_data.h"
#include "components/version_info/channel.h"
#include "content/public/browser/browser_thread.h"

namespace metrics {

namespace {

const char kCWPFieldTrialName[] = "ChromeOSWideProfilingCollection";

// Limit the total size of protobufs that can be cached, so they don't take up
// too much memory. If the size of cached protobufs exceeds this value, stop
// collecting further perf data. The current value is 4 MB.
const size_t kCachedPerfDataProtobufSizeThreshold = 4 * 1024 * 1024;

// This is used to space out session restore collections in the face of several
// notifications in a short period of time. There should be no less than this
// much time between collections.
const int kMinIntervalBetweenSessionRestoreCollectionsInSec = 30;

// Enumeration representing success and various failure modes for collecting and
// sending perf data.
enum GetPerfDataOutcome {
  SUCCESS,
  NOT_READY_TO_UPLOAD,
  NOT_READY_TO_COLLECT,
  INCOGNITO_ACTIVE,
  INCOGNITO_LAUNCHED,
  PROTOBUF_NOT_PARSED,
  ILLEGAL_DATA_RETURNED,
  ALREADY_COLLECTING,
  NUM_OUTCOMES
};

// Name of the histogram that represents the success and various failure modes
// for collecting and sending perf data.
const char kGetPerfDataOutcomeHistogram[] = "UMA.Perf.GetData";

void AddToPerfHistogram(GetPerfDataOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION(kGetPerfDataOutcomeHistogram,
                            outcome,
                            NUM_OUTCOMES);
}

// Returns true if a normal user is logged in. Returns false otherwise (e.g. if
// logged in as a guest or as a kiosk app).
bool IsNormalUserLoggedIn() {
  return chromeos::LoginState::Get()->IsUserAuthenticated();
}

// Returns a random TimeDelta between zero and |max|.
base::TimeDelta RandomTimeDelta(base::TimeDelta max) {
  return base::TimeDelta::FromMicroseconds(
      base::RandGenerator(max.InMicroseconds()));
}

// Returns a TimeDelta profile duration based on the current chrome channel.
base::TimeDelta ProfileDuration() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
    case version_info::Channel::DEV:
    case version_info::Channel::BETA:
      return base::TimeDelta::FromSeconds(4);
    case version_info::Channel::STABLE:
    case version_info::Channel::UNKNOWN:
    default:
      return base::TimeDelta::FromSeconds(2);
  }
}

// Returns a TimeDelta interval duration for periodic collection based on the
// current chrome channel.
base::TimeDelta PeriodicCollectionInterval() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
    case version_info::Channel::DEV:
    case version_info::Channel::BETA:
      return base::TimeDelta::FromMinutes(90);
    case version_info::Channel::STABLE:
    case version_info::Channel::UNKNOWN:
    default:
      return base::TimeDelta::FromMinutes(180);
  }
}

// Gets parameter named by |key| from the map. If it is present and is an
// integer, stores the result in |out| and return true. Otherwise return false.
bool GetInt64Param(const std::map<std::string, std::string>& params,
                   const std::string& key,
                   int64_t* out) {
  auto it = params.find(key);
  if (it == params.end())
    return false;
  int64_t value;
  // NB: StringToInt64 will set value even if the conversion fails.
  if (!base::StringToInt64(it->second, &value))
    return false;
  *out = value;
  return true;
}

// Parses the key. e.g.: "PerfCommand::arm::0" returns "arm"
bool ExtractPerfCommandCpuSpecifier(const std::string& key,
                                    std::string* cpu_specifier) {
  std::vector<std::string> tokens = base::SplitStringUsingSubstr(
      key, "::", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (tokens.size() != 3)
    return false;
  if (tokens[0] != "PerfCommand")
    return false;
  *cpu_specifier = tokens[1];
  // tokens[2] is just a unique string (usually an index).
  return true;
}

// Parses the components of a version string, e.g. major.minor.bugfix
void ExtractVersionNumbers(const std::string& version,
                           int32_t* major_version,
                           int32_t* minor_version,
                           int32_t* bugfix_version) {
  *major_version = *minor_version = *bugfix_version = 0;
  // Parse out the version numbers from the string.
  sscanf(version.c_str(), "%d.%d.%d", major_version, minor_version,
         bugfix_version);
}

// Returns if a micro-architecture supports LBR callgraph profiling.
bool MicroarchitectureHasLBRCallgraph(const std::string& uarch) {
  return uarch == "Haswell" || uarch == "Broadwell" || uarch == "Skylake" ||
         uarch == "Kabylake";
}

// Returns if a kernel release supports LBR callgraph profiling.
bool KernelReleaseHasLBRCallgraph(const std::string& release) {
  int32_t major, minor, bugfix;
  ExtractVersionNumbers(release, &major, &minor, &bugfix);
  return major > 4 || (major == 4 && minor >= 4) || (major == 3 && minor == 18);
}

// Hopefully we never need a space in a command argument.
const char kPerfCommandDelimiter[] = " ";

const char kPerfRecordCyclesCmd[] =
  "perf record -a -e cycles -c 1000003";

const char kPerfRecordFPCallgraphCmd[] =
  "perf record -a -e cycles -g -c 4000037";

const char kPerfRecordLBRCallgraphCmd[] =
  "perf record -a -e cycles -c 4000037 --call-graph lbr";

const char kPerfRecordLBRCmd[] =
  "perf record -a -e r20c4 -b -c 200011";

// Silvermont, Airmont, Goldmont don't have a branches taken event. Therefore,
// we sample on the branches retired event.
const char kPerfRecordLBRCmdAtom[] =
  "perf record -a -e rc4 -b -c 300001";

const char kPerfRecordInstructionTLBMissesCmd[] =
  "perf record -a -e iTLB-misses -c 2003";

const char kPerfRecordDataTLBMissesCmd[] =
  "perf record -a -e dTLB-misses -c 2003";

const char kPerfRecordCacheMissesCmd[] =
    "perf record -a -e cache-misses -c 10007";

const char kPerfStatMemoryBandwidthCmd[] =
  "perf stat -a -e cycles -e instructions "
  "-e uncore_imc/data_reads/ -e uncore_imc/data_writes/ "
  "-e cpu/event=0xD0,umask=0x11,name=MEM_UOPS_RETIRED-STLB_MISS_LOADS/ "
  "-e cpu/event=0xD0,umask=0x12,name=MEM_UOPS_RETIRED-STLB_MISS_STORES/";

const std::vector<RandomSelector::WeightAndValue> GetDefaultCommands_x86_64(
    const CPUIdentity& cpuid) {
  using WeightAndValue = RandomSelector::WeightAndValue;
  std::vector<WeightAndValue> cmds;
  DCHECK_EQ(cpuid.arch, "x86_64");
  const std::string cpu_uarch = GetCpuUarch(cpuid);
  // Haswell and newer big Intel cores support LBR callstack profiling. This
  // requires kernel support, which was added in kernel 4.4, and it was
  // backported to kernel 3.18. Prefer LBR callstack profiling where supported
  // instead of FP callchains, because the former works with binaries compiled
  // with frame pointers disabled, such as the ARC runtime.
  const char *callgraph_cmd = kPerfRecordFPCallgraphCmd;
  if (MicroarchitectureHasLBRCallgraph(cpu_uarch) &&
      KernelReleaseHasLBRCallgraph(cpuid.release)) {
    callgraph_cmd = kPerfRecordLBRCallgraphCmd;
  }

  if (cpu_uarch == "IvyBridge" || cpu_uarch == "Haswell" ||
      cpu_uarch == "Broadwell") {
    cmds.push_back(WeightAndValue(45.0, kPerfRecordCyclesCmd));
    cmds.push_back(WeightAndValue(20.0, callgraph_cmd));
    cmds.push_back(WeightAndValue(15.0, kPerfRecordLBRCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordInstructionTLBMissesCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordDataTLBMissesCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfStatMemoryBandwidthCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordCacheMissesCmd));
    return cmds;
  }
  if (cpu_uarch == "SandyBridge" || cpu_uarch == "Skylake" ||
      cpu_uarch == "Kabylake") {
    cmds.push_back(WeightAndValue(50.0, kPerfRecordCyclesCmd));
    cmds.push_back(WeightAndValue(20.0, callgraph_cmd));
    cmds.push_back(WeightAndValue(15.0, kPerfRecordLBRCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordInstructionTLBMissesCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordDataTLBMissesCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordCacheMissesCmd));
    return cmds;
  }
  if (cpu_uarch == "Silvermont" || cpu_uarch == "Airmont" ||
      cpu_uarch == "Goldmont") {
    cmds.push_back(WeightAndValue(50.0, kPerfRecordCyclesCmd));
    cmds.push_back(WeightAndValue(20.0, callgraph_cmd));
    cmds.push_back(WeightAndValue(15.0, kPerfRecordLBRCmdAtom));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordInstructionTLBMissesCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordDataTLBMissesCmd));
    cmds.push_back(WeightAndValue(5.0, kPerfRecordCacheMissesCmd));
    return cmds;
  }
  // Other 64-bit x86
  cmds.push_back(WeightAndValue(65.0, kPerfRecordCyclesCmd));
  cmds.push_back(WeightAndValue(20.0, callgraph_cmd));
  cmds.push_back(WeightAndValue(5.0, kPerfRecordInstructionTLBMissesCmd));
  cmds.push_back(WeightAndValue(5.0, kPerfRecordDataTLBMissesCmd));
  cmds.push_back(WeightAndValue(5.0, kPerfRecordCacheMissesCmd));
  return cmds;
}

// PerfDataProto is defined elsewhere with more fields than the definition in
// Chromium's copy of perf_data.proto. During deserialization, the protobuf
// data could contain fields that are defined elsewhere but not in
// perf_data.proto, resulting in some data in |unknown_fields| for the message
// types within PerfDataProto.
//
// This function deletes those dangling unknown fields if they are in messages
// containing strings. See comments in perf_data.proto describing the fields
// that have been intentionally left out. Note that all unknown fields will be
// removed from those messages, not just unknown string fields.
void RemoveUnknownFieldsFromMessagesWithStrings(PerfDataProto* proto) {
  // Clean up PerfEvent::MMapEvent and PerfEvent::CommEvent.
  for (PerfDataProto::PerfEvent& event : *proto->mutable_events()) {
    if (event.has_comm_event())
      event.mutable_comm_event()->mutable_unknown_fields()->clear();
    if (event.has_mmap_event())
      event.mutable_mmap_event()->mutable_unknown_fields()->clear();
  }
  // Clean up PerfBuildID.
  for (PerfDataProto::PerfBuildID& build_id : *proto->mutable_build_ids()) {
    build_id.mutable_unknown_fields()->clear();
  }
  // Clean up StringMetadata and StringMetadata::StringAndMd5sumPrefix.
  if (proto->has_string_metadata()) {
    proto->mutable_string_metadata()->mutable_unknown_fields()->clear();
    if (proto->string_metadata().has_perf_command_line_whole()) {
      proto->mutable_string_metadata()->mutable_perf_command_line_whole()->
          mutable_unknown_fields()->clear();
    }
  }
}

}  // namespace

namespace internal {

std::vector<RandomSelector::WeightAndValue> GetDefaultCommandsForCpu(
    const CPUIdentity& cpuid) {
  using WeightAndValue = RandomSelector::WeightAndValue;

  if (cpuid.arch == "x86_64")  // 64-bit x86
    return GetDefaultCommands_x86_64(cpuid);

  std::vector<WeightAndValue> cmds;
  if (cpuid.arch == "x86" ||     // 32-bit x86, or...
      cpuid.arch == "armv7l") {  // ARM
    cmds.push_back(WeightAndValue(80.0, kPerfRecordCyclesCmd));
    cmds.push_back(WeightAndValue(20.0, kPerfRecordFPCallgraphCmd));
    return cmds;
  }

  // Unknown CPUs
  cmds.push_back(WeightAndValue(1.0, kPerfRecordCyclesCmd));
  return cmds;
}

}  // namespace internal

PerfProvider::CollectionParams::CollectionParams()
    : CollectionParams(ProfileDuration() /* collection_duration */,
                       PeriodicCollectionInterval() /* periodic_interval */,
                       PerfProvider::CollectionParams::
                           TriggerParams(/* resume_from_suspend */
                                         10 /* sampling_factor */,
                                         base::TimeDelta::FromSeconds(
                                             5)) /* max_collection_delay */,
                       PerfProvider::CollectionParams::
                           TriggerParams(/* restore_session */
                                         10 /* sampling_factor */,
                                         base::TimeDelta::FromSeconds(
                                             10)) /* max_collection_delay */) {}

PerfProvider::CollectionParams::CollectionParams(
    base::TimeDelta collection_duration,
    base::TimeDelta periodic_interval,
    TriggerParams resume_from_suspend,
    TriggerParams restore_session)
    : collection_duration_(collection_duration.ToInternalValue()),
      periodic_interval_(periodic_interval.ToInternalValue()),
      resume_from_suspend_(resume_from_suspend),
      restore_session_(restore_session) {
}

PerfProvider::CollectionParams::TriggerParams::TriggerParams(
    int64_t sampling_factor,
    base::TimeDelta max_collection_delay)
    : sampling_factor_(sampling_factor),
      max_collection_delay_(max_collection_delay.ToInternalValue()) {}

PerfProvider::PerfProvider()
    : login_observer_(this),
      weak_factory_(this) {
}

PerfProvider::~PerfProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  chromeos::LoginState::Get()->RemoveObserver(&login_observer_);
}

void PerfProvider::Init() {
  CHECK(command_selector_.SetOdds(
      internal::GetDefaultCommandsForCpu(GetCPUIdentity())));
  std::map<std::string, std::string> params;
  if (variations::GetVariationParams(kCWPFieldTrialName, &params))
    SetCollectionParamsFromVariationParams(params);

  // Register the login observer with LoginState.
  chromeos::LoginState::Get()->AddObserver(&login_observer_);

  // Register as an observer of power manager events.
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->
      AddObserver(this);

  // Register as an observer of session restore.
  on_session_restored_callback_subscription_ =
      SessionRestore::RegisterOnSessionRestoredCallback(
          base::Bind(&PerfProvider::OnSessionRestoreDone,
                     weak_factory_.GetWeakPtr()));

  // Check the login state. At the time of writing, this class is instantiated
  // before login. A subsequent login would activate the profiling. However,
  // that behavior may change in the future so that the user is already logged
  // when this class is instantiated. By calling LoggedInStateChanged() here,
  // PerfProvider will recognize that the system is already logged in.
  login_observer_.LoggedInStateChanged();
}

namespace internal {

std::string FindBestCpuSpecifierFromParams(
    const std::map<std::string, std::string>& params,
    const CPUIdentity& cpuid) {
  std::string ret;
  // The CPU specified in the variation params could be "default", a system
  // architecture, a CPU microarchitecture, or a CPU model substring. We should
  // prefer to match the most specific.
  enum MatchSpecificity {
    NO_MATCH,
    DEFAULT,
    SYSTEM_ARCH,
    CPU_UARCH,
    CPU_MODEL,
  };
  MatchSpecificity match_level = NO_MATCH;

  const std::string cpu_uarch = GetCpuUarch(cpuid);
  const std::string simplified_cpu_model =
      SimplifyCPUModelName(cpuid.model_name);

  for (const auto& key_val : params) {
    const std::string& key = key_val.first;

    std::string cpu_specifier;
    if (!ExtractPerfCommandCpuSpecifier(key, &cpu_specifier))
      continue;

    if (match_level < DEFAULT && cpu_specifier == "default") {
      match_level = DEFAULT;
      ret = cpu_specifier;
    }
    if (match_level < SYSTEM_ARCH && cpu_specifier == cpuid.arch) {
      match_level = SYSTEM_ARCH;
      ret = cpu_specifier;
    }
    if (match_level < CPU_UARCH && !cpu_uarch.empty() &&
        cpu_specifier == cpu_uarch) {
      match_level = CPU_UARCH;
      ret = cpu_specifier;
    }
    if (match_level < CPU_MODEL &&
        simplified_cpu_model.find(cpu_specifier) != std::string::npos) {
      match_level = CPU_MODEL;
      ret = cpu_specifier;
    }
  }
  return ret;
}

}  // namespace internal

void PerfProvider::SetCollectionParamsFromVariationParams(
    const std::map<std::string, std::string>& params) {
  int64_t value;
  if (GetInt64Param(params, "ProfileCollectionDurationSec", &value)) {
    collection_params_.set_collection_duration(
        base::TimeDelta::FromSeconds(value));
  }
  if (GetInt64Param(params, "PeriodicProfilingIntervalMs", &value)) {
    collection_params_.set_periodic_interval(
        base::TimeDelta::FromMilliseconds(value));
  }
  if (GetInt64Param(params, "ResumeFromSuspend::SamplingFactor", &value)) {
    collection_params_.mutable_resume_from_suspend()
        ->set_sampling_factor(value);
  }
  if (GetInt64Param(params, "ResumeFromSuspend::MaxDelaySec", &value)) {
    collection_params_.mutable_resume_from_suspend()->set_max_collection_delay(
        base::TimeDelta::FromSeconds(value));
  }
  if (GetInt64Param(params, "RestoreSession::SamplingFactor", &value)) {
    collection_params_.mutable_restore_session()->set_sampling_factor(value);
  }
  if (GetInt64Param(params, "RestoreSession::MaxDelaySec", &value)) {
    collection_params_.mutable_restore_session()->set_max_collection_delay(
        base::TimeDelta::FromSeconds(value));
  }

  const std::string best_cpu_specifier =
      internal::FindBestCpuSpecifierFromParams(params, GetCPUIdentity());

  if (best_cpu_specifier.empty())  // No matching cpu specifier. Keep defaults.
    return;

  std::vector<RandomSelector::WeightAndValue> commands;
  for (const auto& key_val : params) {
    const std::string& key = key_val.first;
    const std::string& val = key_val.second;

    std::string cpu_specifier;
    if (!ExtractPerfCommandCpuSpecifier(key, &cpu_specifier))
      continue;
    if (cpu_specifier != best_cpu_specifier)
      continue;

    auto split = val.find(" ");
    if (split == std::string::npos)
      continue;  // Just drop invalid commands.
    std::string weight_str = std::string(val.begin(), val.begin() + split);

    double weight;
    if (!(base::StringToDouble(weight_str, &weight) && weight > 0.0))
      continue;  // Just drop invalid commands.
    std::string command(val.begin() + split + 1, val.end());
    commands.push_back(RandomSelector::WeightAndValue(weight, command));
  }
  command_selector_.SetOdds(commands);
}

bool PerfProvider::GetSampledProfiles(
    std::vector<SampledProfile>* sampled_profiles) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (cached_perf_data_.empty()) {
    AddToPerfHistogram(NOT_READY_TO_UPLOAD);
    return false;
  }

  sampled_profiles->swap(cached_perf_data_);
  cached_perf_data_.clear();

  AddToPerfHistogram(SUCCESS);
  return true;
}

// Returns one of the above enums given an vector of perf arguments, starting
// with "perf" itself in |args[0]|.
// static
PerfProvider::PerfSubcommand PerfProvider::GetPerfSubcommandType(
    const std::vector<std::string>& args) {
  if (args.size() > 1 && args[0] == "perf") {
    if (args[1] == "record")
      return PerfSubcommand::PERF_COMMAND_RECORD;
    if (args[1] == "stat")
      return PerfSubcommand::PERF_COMMAND_STAT;
    if (args[1] == "mem")
      return PerfSubcommand::PERF_COMMAND_MEM;
  }

  return PerfSubcommand::PERF_COMMAND_UNSUPPORTED;
}

void PerfProvider::ParseOutputProtoIfValid(
    std::unique_ptr<WindowedIncognitoObserver> incognito_observer,
    std::unique_ptr<SampledProfile> sampled_profile,
    PerfSubcommand subcommand,
    const std::string& perf_stdout) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // |perf_output_call_| called us, and owns |perf_stdout|. We must delete it,
  // but not before parsing |perf_stdout|, and we may return early.
  std::unique_ptr<PerfOutputCall> call_deleter(std::move(perf_output_call_));

  if (incognito_observer->incognito_launched()) {
    AddToPerfHistogram(INCOGNITO_LAUNCHED);
    return;
  }

  if (perf_stdout.empty()) {
    AddToPerfHistogram(ILLEGAL_DATA_RETURNED);
    return;
  }

  switch (subcommand) {
    case PerfSubcommand::PERF_COMMAND_RECORD:
    case PerfSubcommand::PERF_COMMAND_MEM: {
      PerfDataProto perf_data_proto;
      if (!perf_data_proto.ParseFromString(perf_stdout)) {
        AddToPerfHistogram(PROTOBUF_NOT_PARSED);
        return;
      }
      RemoveUnknownFieldsFromMessagesWithStrings(&perf_data_proto);
      sampled_profile->set_ms_after_boot(
          base::SysInfo::Uptime().InMilliseconds());
      sampled_profile->mutable_perf_data()->Swap(&perf_data_proto);
      break;
    }
    case PerfSubcommand::PERF_COMMAND_STAT: {
      PerfStatProto perf_stat_proto;
      if (!perf_stat_proto.ParseFromString(perf_stdout)) {
        AddToPerfHistogram(PROTOBUF_NOT_PARSED);
        return;
      }
      sampled_profile->mutable_perf_stat()->Swap(&perf_stat_proto);
      break;
    }
    case PerfSubcommand::PERF_COMMAND_UNSUPPORTED:
      AddToPerfHistogram(PROTOBUF_NOT_PARSED);
      return;
  }

  DCHECK(!login_time_.is_null());
  sampled_profile->set_ms_after_login(
      (base::TimeTicks::Now() - login_time_).InMilliseconds());

  // Add the collected data to the container of collected SampledProfiles.
  cached_perf_data_.resize(cached_perf_data_.size() + 1);
  cached_perf_data_.back().Swap(sampled_profile.get());
}

PerfProvider::LoginObserver::LoginObserver(PerfProvider* perf_provider)
    : perf_provider_(perf_provider) {}

void PerfProvider::LoginObserver::LoggedInStateChanged() {
  if (IsNormalUserLoggedIn())
    perf_provider_->OnUserLoggedIn();
  else
    perf_provider_->Deactivate();
}

void PerfProvider::SuspendDone(const base::TimeDelta& sleep_duration) {
  // A zero value for the suspend duration indicates that the suspend was
  // canceled. Do not collect anything if that's the case.
  if (sleep_duration.is_zero())
    return;

  // Do not collect a profile unless logged in. The system behavior when closing
  // the lid or idling when not logged in is currently to shut down instead of
  // suspending. But it's good to enforce the rule here in case that changes.
  if (!IsNormalUserLoggedIn())
    return;

  // Collect a profile only 1/|sampling_factor| of the time, to avoid
  // collecting too much data. (0 means disable the trigger)
  const auto& resume_params = collection_params_.resume_from_suspend();
  if (resume_params.sampling_factor() == 0 ||
      base::RandGenerator(resume_params.sampling_factor()) != 0)
    return;

  // Override any existing profiling.
  if (timer_.IsRunning())
    timer_.Stop();

  // Randomly pick a delay before doing the collection.
  base::TimeDelta collection_delay = RandomTimeDelta(
      resume_params.max_collection_delay());
  timer_.Start(FROM_HERE,
               collection_delay,
               base::Bind(&PerfProvider::CollectPerfDataAfterResume,
                          weak_factory_.GetWeakPtr(),
                          sleep_duration,
                          collection_delay));
}

void PerfProvider::OnSessionRestoreDone(int num_tabs_restored) {
  // Do not collect a profile unless logged in as a normal user.
  if (!IsNormalUserLoggedIn())
    return;

  // Collect a profile only 1/|sampling_factor| of the time, to
  // avoid collecting too much data and potentially causing UI latency.
  // (0 means disable the trigger)
  const auto& restore_params = collection_params_.restore_session();
  if (restore_params.sampling_factor() == 0 ||
      base::RandGenerator(restore_params.sampling_factor()) != 0) {
    return;
  }

  const auto min_interval = base::TimeDelta::FromSeconds(
      kMinIntervalBetweenSessionRestoreCollectionsInSec);
  const base::TimeDelta time_since_last_collection =
      (base::TimeTicks::Now() - last_session_restore_collection_time_);
  // Do not collect if there hasn't been enough elapsed time since the last
  // collection.
  if (!last_session_restore_collection_time_.is_null() &&
      time_since_last_collection < min_interval) {
    return;
  }

  // Stop any existing scheduled collection.
  if (timer_.IsRunning())
    timer_.Stop();

  // Randomly pick a delay before doing the collection.
  base::TimeDelta collection_delay = RandomTimeDelta(
      restore_params.max_collection_delay());
  timer_.Start(
      FROM_HERE,
      collection_delay,
      base::Bind(&PerfProvider::CollectPerfDataAfterSessionRestore,
                 weak_factory_.GetWeakPtr(),
                 collection_delay,
                 num_tabs_restored));
}

void PerfProvider::OnUserLoggedIn() {
  const base::TimeTicks now = base::TimeTicks::Now();
  login_time_ = now;
  next_profiling_interval_start_ = now;
  ScheduleIntervalCollection();
}

void PerfProvider::Deactivate() {
  // Stop the timer, but leave |cached_perf_data_| intact.
  timer_.Stop();
}

void PerfProvider::ScheduleIntervalCollection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (timer_.IsRunning())
    return;

  const base::TimeTicks now = base::TimeTicks::Now();

  base::TimeTicks interval_end =
      next_profiling_interval_start_ + collection_params_.periodic_interval();
  if (now > interval_end) {
    // We somehow missed at least one window. Start over.
    next_profiling_interval_start_ = now;
    interval_end = now + collection_params_.periodic_interval();
  }

  // Pick a random time in the current interval.
  base::TimeTicks scheduled_time =
      next_profiling_interval_start_ +
      RandomTimeDelta(collection_params_.periodic_interval());
  // If the scheduled time has already passed in the time it took to make the
  // above calculations, trigger the collection event immediately.
  if (scheduled_time < now)
    scheduled_time = now;

  timer_.Start(FROM_HERE, scheduled_time - now, this,
               &PerfProvider::DoPeriodicCollection);

  // Update the profiling interval tracker to the start of the next interval.
  next_profiling_interval_start_ = interval_end;
}

void PerfProvider::CollectIfNecessary(
    std::unique_ptr<SampledProfile> sampled_profile) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Schedule another interval collection. This call makes sense regardless of
  // whether or not the current collection was interval-triggered. If it had
  // been another type of trigger event, the interval timer would have been
  // halted, so it makes sense to reschedule a new interval collection.
  ScheduleIntervalCollection();

  // Only allow one active collection.
  if (perf_output_call_) {
    AddToPerfHistogram(ALREADY_COLLECTING);
    return;
  }

  // Do not collect further data if we've already collected a substantial amount
  // of data, as indicated by |kCachedPerfDataProtobufSizeThreshold|.
  size_t cached_perf_data_size = 0;
  for (size_t i = 0; i < cached_perf_data_.size(); ++i) {
    cached_perf_data_size += cached_perf_data_[i].ByteSize();
  }
  if (cached_perf_data_size >= kCachedPerfDataProtobufSizeThreshold) {
    AddToPerfHistogram(NOT_READY_TO_COLLECT);
    return;
  }

  // For privacy reasons, Chrome should only collect perf data if there is no
  // incognito session active (or gets spawned during the collection).
  if (BrowserList::IsIncognitoSessionActive()) {
    AddToPerfHistogram(INCOGNITO_ACTIVE);
    return;
  }

  std::unique_ptr<WindowedIncognitoObserver> incognito_observer(
      new WindowedIncognitoObserver);

  std::vector<std::string> command = base::SplitString(
      command_selector_.Select(), kPerfCommandDelimiter,
      base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  PerfSubcommand subcommand = GetPerfSubcommandType(command);

  perf_output_call_.reset(new PerfOutputCall(
      collection_params_.collection_duration(), command,
      base::Bind(&PerfProvider::ParseOutputProtoIfValid,
                 weak_factory_.GetWeakPtr(), base::Passed(&incognito_observer),
                 base::Passed(&sampled_profile), subcommand)));
}

void PerfProvider::DoPeriodicCollection() {
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::PERIODIC_COLLECTION);

  CollectIfNecessary(std::move(sampled_profile));
}

void PerfProvider::CollectPerfDataAfterResume(
    const base::TimeDelta& sleep_duration,
    const base::TimeDelta& time_after_resume) {
  // Fill out a SampledProfile protobuf that will contain the collected data.
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESUME_FROM_SUSPEND);
  sampled_profile->set_suspend_duration_ms(sleep_duration.InMilliseconds());
  sampled_profile->set_ms_after_resume(time_after_resume.InMilliseconds());

  CollectIfNecessary(std::move(sampled_profile));
}

void PerfProvider::CollectPerfDataAfterSessionRestore(
    const base::TimeDelta& time_after_restore,
    int num_tabs_restored) {
  // Fill out a SampledProfile protobuf that will contain the collected data.
  std::unique_ptr<SampledProfile> sampled_profile(new SampledProfile);
  sampled_profile->set_trigger_event(SampledProfile::RESTORE_SESSION);
  sampled_profile->set_ms_after_restore(time_after_restore.InMilliseconds());
  sampled_profile->set_num_tabs_restored(num_tabs_restored);

  CollectIfNecessary(std::move(sampled_profile));
  last_session_restore_collection_time_ = base::TimeTicks::Now();
}

}  // namespace metrics
