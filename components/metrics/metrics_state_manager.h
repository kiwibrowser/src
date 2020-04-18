// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_METRICS_STATE_MANAGER_H_
#define COMPONENTS_METRICS_METRICS_STATE_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string16.h"
#include "components/metrics/clean_exit_beacon.h"
#include "components/metrics/client_info.h"

class PrefService;
class PrefRegistrySimple;

namespace metrics {

class ClonedInstallDetector;
class EnabledStateProvider;
class MetricsProvider;

// Responsible for managing MetricsService state prefs, specifically the UMA
// client id and low entropy source. Code outside the metrics directory should
// not be instantiating or using this class directly.
class MetricsStateManager final {
 public:
  // A callback that can be invoked to store client info to persistent storage.
  // Storing an empty client_id will resulted in the backup being voided.
  typedef base::Callback<void(const ClientInfo& client_info)>
      StoreClientInfoCallback;

  // A callback that can be invoked to load client info stored through the
  // StoreClientInfoCallback.
  typedef base::Callback<std::unique_ptr<ClientInfo>(void)>
      LoadClientInfoCallback;

  ~MetricsStateManager();

  std::unique_ptr<MetricsProvider> GetProvider();

  // Returns true if the user has consented to sending metric reports, and there
  // is no other reason to disable reporting. One such reason is client
  // sampling, and this client isn't in the sample.
  bool IsMetricsReportingEnabled();

  // Returns the install date of the application, in seconds since the epoch.
  int64_t GetInstallDate() const;

  // Returns the client ID for this client, or the empty string if the user is
  // not opted in to metrics reporting.
  const std::string& client_id() const { return client_id_; }

  // The CleanExitBeacon, used to determine whether the previous Chrome browser
  // session terminated gracefully.
  CleanExitBeacon* clean_exit_beacon() { return &clean_exit_beacon_; }
  const CleanExitBeacon* clean_exit_beacon() const {
    return &clean_exit_beacon_;
  }

  // Forces the client ID to be generated. This is useful in case it's needed
  // before recording.
  void ForceClientIdCreation();

  // Checks if this install was cloned or imaged from another machine. If a
  // clone is detected, resets the client id and low entropy source. This
  // should not be called more than once.
  void CheckForClonedInstall();

  // Returns the preferred entropy provider used to seed persistent activities
  // based on whether or not metrics reporting is permitted on this client.
  //
  // If there's consent to report metrics, this method returns an entropy
  // provider that has a high source of entropy, partially based on the client
  // ID. Otherwise, it returns an entropy provider that is based on a low
  // entropy source.
  std::unique_ptr<const base::FieldTrial::EntropyProvider>
  CreateDefaultEntropyProvider();

  // Returns an entropy provider that is based on a low entropy source. This
  // provider is the same type of provider returned by
  // CreateDefaultEntropyProvider when there's no consent to report metrics, but
  // will be a new instance.
  std::unique_ptr<const base::FieldTrial::EntropyProvider>
  CreateLowEntropyProvider();

  // Creates the MetricsStateManager, enforcing that only a single instance
  // of the class exists at a time. Returns NULL if an instance exists already.
  // On Windows, |backup_registry_key| is used to store a backup of the clean
  // exit beacon. It is ignored on other platforms.
  static std::unique_ptr<MetricsStateManager> Create(
      PrefService* local_state,
      EnabledStateProvider* enabled_state_provider,
      const base::string16& backup_registry_key,
      const StoreClientInfoCallback& store_client_info,
      const LoadClientInfoCallback& load_client_info);

  // Registers local state prefs used by this class.
  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest, CheckProviderResetIds);
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest, EntropySourceUsed_Low);
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest, EntropySourceUsed_High);
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest, LowEntropySource0NotReset);
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest,
                           PermutedEntropyCacheClearedWhenLowEntropyReset);
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest, ResetBackup);
  FRIEND_TEST_ALL_PREFIXES(MetricsStateManagerTest, ResetMetricsIDs);

  // Designates which entropy source was returned from this class.
  // This is used for testing to validate that we return the correct source
  // depending on the state of the service.
  enum EntropySourceType {
    ENTROPY_SOURCE_NONE,
    ENTROPY_SOURCE_LOW,
    ENTROPY_SOURCE_HIGH,
    ENTROPY_SOURCE_ENUM_SIZE,
  };

  // Creates the MetricsStateManager with the given |local_state|. Uses
  // |enabled_state_provider| to query whether there is consent for metrics
  // reporting, and if it is enabled. Clients should instead use Create(), which
  // enforces that a single instance of this class be alive at any given time.
  // |store_client_info| should back up client info to persistent storage such
  // that it is later retrievable by |load_client_info|.
  MetricsStateManager(PrefService* local_state,
                      EnabledStateProvider* enabled_state_provider,
                      const base::string16& backup_registry_key,
                      const StoreClientInfoCallback& store_client_info,
                      const LoadClientInfoCallback& load_client_info);

  // Backs up the current client info via |store_client_info_|.
  void BackUpCurrentClientInfo();

  // Loads the client info via |load_client_info_|.
  std::unique_ptr<ClientInfo> LoadClientInfo();

  // Returns the low entropy source for this client. This is a random value
  // that is non-identifying amongst browser clients. This method will
  // generate the entropy source value if it has not been called before.
  int GetLowEntropySource();

  // Generates the low entropy source value for this client if it is not
  // already set.
  void UpdateLowEntropySource();

  // Updates |entropy_source_returned_| with |type| iff the current value is
  // ENTROPY_SOURCE_NONE and logs the new value in a histogram.
  void UpdateEntropySourceReturnedValue(EntropySourceType type);

  // Returns the first entropy source that was returned by this service since
  // start up, or NONE if neither was returned yet. This is exposed for testing
  // only.
  EntropySourceType entropy_source_returned() const {
    return entropy_source_returned_;
  }

  // Reset the client id and low entropy source if the kMetricsResetMetricIDs
  // pref is true.
  void ResetMetricsIDsIfNecessary();

  // Whether an instance of this class exists. Used to enforce that there aren't
  // multiple instances of this class at a given time.
  static bool instance_exists_;

  // Weak pointer to the local state prefs store.
  PrefService* const local_state_;

  // Weak pointer to an enabled state provider. Used to know whether the user
  // has consented to reporting, and if reporting should be done.
  EnabledStateProvider* enabled_state_provider_;

  // A callback run during client id creation so this MetricsStateManager can
  // store a backup of the newly generated ID.
  const StoreClientInfoCallback store_client_info_;

  // A callback run if this MetricsStateManager can't get the client id from
  // its typical location and wants to attempt loading it from this backup.
  const LoadClientInfoCallback load_client_info_;

  // A beacon used to determine whether the previous Chrome browser session
  // terminated gracefully.
  CleanExitBeacon clean_exit_beacon_;

  // The identifier that's sent to the server with the log reports.
  std::string client_id_;

  // The non-identifying low entropy source value.
  int low_entropy_source_;

  // The last entropy source returned by this service, used for testing.
  EntropySourceType entropy_source_returned_;

  // The value of prefs::kMetricsResetIds seen upon startup, i.e., the value
  // that was appropriate in the previous session. Used when reporting previous
  // session (stability) data.
  bool metrics_ids_were_reset_;

  // The value of the metrics id before reseting. Only possibly valid if the
  // metrics id was reset. May be blank if the metrics id was reset but Chrome
  // has no record of what the previous metrics id was.
  std::string previous_client_id_;

  std::unique_ptr<ClonedInstallDetector> cloned_install_detector_;

  DISALLOW_COPY_AND_ASSIGN(MetricsStateManager);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_METRICS_STATE_MANAGER_H_
