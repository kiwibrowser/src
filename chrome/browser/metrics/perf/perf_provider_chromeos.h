// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_PERF_PERF_PROVIDER_CHROMEOS_H_
#define CHROME_BROWSER_METRICS_PERF_PERF_PROVIDER_CHROMEOS_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/metrics/perf/cpu_identity.h"
#include "chrome/browser/metrics/perf/perf_output.h"
#include "chrome/browser/metrics/perf/random_selector.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/login/login_state.h"
#include "third_party/metrics_proto/sampled_profile.pb.h"

namespace metrics {

class WindowedIncognitoObserver;

// Provides access to ChromeOS perf data. perf aka "perf events" is a
// performance profiling infrastructure built into the linux kernel. For more
// information, see: https://perf.wiki.kernel.org/index.php/Main_Page.
class PerfProvider : public chromeos::PowerManagerClient::Observer {
 public:
  PerfProvider();
  ~PerfProvider() override;

  void Init();

  // Stores collected perf data protobufs in |sampled_profiles|. Clears all the
  // stored profile data. Returns true if it wrote to |sampled_profiles|.
  bool GetSampledProfiles(std::vector<SampledProfile>* sampled_profiles);

 protected:
  enum PerfSubcommand {
    PERF_COMMAND_RECORD,
    PERF_COMMAND_STAT,
    PERF_COMMAND_MEM,
    PERF_COMMAND_UNSUPPORTED,
  };

  typedef int64_t TimeDeltaInternalType;

  class CollectionParams {
   public:
    class TriggerParams {
     public:
      TriggerParams(int64_t sampling_factor,
                    base::TimeDelta max_collection_delay);

      int64_t sampling_factor() const { return sampling_factor_; }
      void set_sampling_factor(int64_t factor) { sampling_factor_ = factor; }

      base::TimeDelta max_collection_delay() const {
        return base::TimeDelta::FromInternalValue(max_collection_delay_);
      }
      void set_max_collection_delay(base::TimeDelta delay) {
        max_collection_delay_ = delay.ToInternalValue();
      }

     private:
      TriggerParams() = delete;

      // Limit the number of profiles collected.
      int64_t sampling_factor_;

      // Add a random delay before collecting after the trigger.
      // The delay should be randomly selected between 0 and this value.
      TimeDeltaInternalType max_collection_delay_;
    };

    CollectionParams();

    CollectionParams(base::TimeDelta collection_duration,
                     base::TimeDelta periodic_interval,
                     TriggerParams resume_from_suspend,
                     TriggerParams restore_session);

    base::TimeDelta collection_duration() const {
      return base::TimeDelta::FromInternalValue(collection_duration_);
    }
    void set_collection_duration(base::TimeDelta duration) {
      collection_duration_ = duration.ToInternalValue();
    }

    base::TimeDelta periodic_interval() const {
      return base::TimeDelta::FromInternalValue(periodic_interval_);
    }
    void set_periodic_interval(base::TimeDelta interval) {
      periodic_interval_ = interval.ToInternalValue();
    }

    const TriggerParams& resume_from_suspend() const {
      return resume_from_suspend_;
    }
    TriggerParams* mutable_resume_from_suspend() {
      return &resume_from_suspend_;
    }
    const TriggerParams& restore_session() const {
      return restore_session_;
    }
    TriggerParams* mutable_restore_session() {
      return &restore_session_;
    }

   private:
    // Time perf is run for.
    TimeDeltaInternalType collection_duration_;

    // For PERIODIC_COLLECTION, partition time since login into successive
    // intervals of this duration. In each interval, a random time is picked to
    // collect a profile.
    TimeDeltaInternalType periodic_interval_;

    // Parameters for RESUME_FROM_SUSPEND and RESTORE_SESSION collections:
    TriggerParams resume_from_suspend_;
    TriggerParams restore_session_;

    DISALLOW_COPY_AND_ASSIGN(CollectionParams);
  };

  // Returns one of the above enums given an vector of perf arguments, starting
  // with "perf" itself in |args[0]|.
  static PerfSubcommand GetPerfSubcommandType(
      const std::vector<std::string>& args);

  // Parses a PerfDataProto or PerfStatProto from serialized data |perf_stdout|,
  // if non-empty. Which proto to use depends on |subcommand|. If |perf_stdout|
  // is empty, it is counted as an error. |incognito_observer| indicates
  // whether an incognito window had been opened during the profile collection
  // period. If there was an incognito window, discard the incoming data.
  void ParseOutputProtoIfValid(
      std::unique_ptr<WindowedIncognitoObserver> incognito_observer,
      std::unique_ptr<SampledProfile> sampled_profile,
      PerfSubcommand subcommand,
      const std::string& perf_stdout);

  // Called when a session restore has finished.
  void OnSessionRestoreDone(int num_tabs_restored);

  // Turns off perf collection. Does not delete any data that was already
  // collected and stored in |cached_perf_data_|.
  void Deactivate();

  const CollectionParams& collection_params() const {
    return collection_params_;
  }
  const RandomSelector& command_selector() const {
    return command_selector_;
  }

  const base::OneShotTimer& timer() const {
    return timer_;
  }

 private:
  // Class that listens for changes to the login state. When a normal user logs
  // in, it updates PerfProvider to start collecting data.
  class LoginObserver : public chromeos::LoginState::Observer {
   public:
    explicit LoginObserver(PerfProvider* perf_provider);

    // Called when either the login state or the logged in user type changes.
    // Activates |perf_provider_| to start collecting.
    void LoggedInStateChanged() override;

   private:
    // Points to a PerfProvider instance that can be turned on or off based on
    // the login state.
    PerfProvider* perf_provider_;
  };

  // Change the values in |collection_params_| and the commands in
  // |command_selector_| for any keys that are present in |params|.
  void SetCollectionParamsFromVariationParams(
      const std::map<std::string, std::string> &params);

  // Called when a suspend finishes. This is either a successful suspend
  // followed by a resume, or a suspend that was canceled. Inherited from
  // PowerManagerClient::Observer.
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // Turns on perf collection. Resets the timer that's used to schedule
  // collections.
  void OnUserLoggedIn();

  // Selects a random time in the upcoming profiling interval that begins at
  // |next_profiling_interval_start_|. Schedules |timer_| to invoke
  // DoPeriodicCollection() when that time comes.
  void ScheduleIntervalCollection();

  // Collects perf data for a given |trigger_event|. Calls perf via the ChromeOS
  // debug daemon's dbus interface.
  void CollectIfNecessary(std::unique_ptr<SampledProfile> sampled_profile);

  // Collects perf data on a repeating basis by calling CollectIfNecessary() and
  // reschedules it to be collected again.
  void DoPeriodicCollection();

  // Collects perf data after a resume. |sleep_duration| is the duration the
  // system was suspended before resuming. |time_after_resume_ms| is how long
  // ago the system resumed.
  void CollectPerfDataAfterResume(const base::TimeDelta& sleep_duration,
                                  const base::TimeDelta& time_after_resume);

  // Collects perf data after a session restore. |time_after_restore| is how
  // long ago the session restore started. |num_tabs_restored| is the total
  // number of tabs being restored.
  void CollectPerfDataAfterSessionRestore(
      const base::TimeDelta& time_after_restore,
      int num_tabs_restored);

  // Parameters controlling how profiles are collected.
  CollectionParams collection_params_;

  // Set of commands to choose from.
  RandomSelector command_selector_;

  // An active call to perf/quipper, if set.
  std::unique_ptr<PerfOutputCall> perf_output_call_;

  // Vector of SampledProfile protobufs containing perf profiles.
  std::vector<SampledProfile> cached_perf_data_;

  // For scheduling collection of perf data.
  base::OneShotTimer timer_;

  // For detecting when changes to the login state.
  LoginObserver login_observer_;

  // Record of the last login time.
  base::TimeTicks login_time_;

  // Record of the start of the upcoming profiling interval.
  base::TimeTicks next_profiling_interval_start_;

  // Tracks the last time a session restore was collected.
  base::TimeTicks last_session_restore_collection_time_;

  // Points to the on-session-restored callback that was registered with
  // SessionRestore's callback list. When objects of this class are destroyed,
  // the subscription object's destructor will automatically unregister the
  // callback in SessionRestore, so that the callback list does not contain any
  // obsolete callbacks.
  SessionRestore::CallbackSubscription
      on_session_restored_callback_subscription_;

  SEQUENCE_CHECKER(sequence_checker_);

  // To pass around the "this" pointer across threads safely.
  base::WeakPtrFactory<PerfProvider> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PerfProvider);
};

// Exposed for unit testing.
namespace internal {

// Return the default set of perf commands and their odds of selection given
// the identity of the CPU in |cpuid|.
std::vector<RandomSelector::WeightAndValue> GetDefaultCommandsForCpu(
    const CPUIdentity& cpuid);

// For the "PerfCommand::"-prefixed keys in |params|, return the cpu specifier
// that is the narrowest match for the CPU identified by |cpuid|.
// Valid CPU specifiers, in increasing order of specificity, are:
// "default", a system architecture (e.g. "x86_64"), a CPU microarchitecture
// (currently only some Intel and AMD uarchs supported), or a CPU model name
// substring.
std::string FindBestCpuSpecifierFromParams(
    const std::map<std::string, std::string>& params,
    const CPUIdentity& cpuid);

}  // namespace internal

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_PERF_PERF_PROVIDER_CHROMEOS_H_
