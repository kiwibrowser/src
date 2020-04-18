// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_SERVICE_VARIATIONS_FIELD_TRIAL_CREATOR_H_
#define COMPONENTS_VARIATIONS_SERVICE_VARIATIONS_FIELD_TRIAL_CREATOR_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "components/variations/client_filterable_state.h"
#include "components/variations/seed_response.h"
#include "components/variations/service/ui_string_overrider.h"
#include "components/variations/variations_seed_store.h"

namespace variations {

class PlatformFieldTrials;
class SafeSeedManager;
class VariationsServiceClient;

// Used to setup field trials based on stored variations seed data.
class VariationsFieldTrialCreator {
 public:
  // Caller is responsible for ensuring that objects passed to the constructor
  // stay valid for the lifetime of this object.
  VariationsFieldTrialCreator(PrefService* local_state,
                              VariationsServiceClient* client,
                              const UIStringOverrider& ui_string_overrider);
  // |initial_seed| may be null. If not null, then it will be stored in the
  // contained seed store.
  VariationsFieldTrialCreator(PrefService* local_state,
                              VariationsServiceClient* client,
                              const UIStringOverrider& ui_string_overrider,
                              std::unique_ptr<SeedResponse> initial_seed);
  virtual ~VariationsFieldTrialCreator();

  // Returns what variations will consider to be the latest country. Returns
  // empty if it is not available.
  std::string GetLatestCountry() const;

  VariationsSeedStore* seed_store() { return &seed_store_; }

  // Sets up field trials based on stored variations seed data. Returns whether
  // setup completed successfully.
  // |kEnableGpuBenchmarking|, |kEnableFeatures|, |kDisableFeatures| are
  // feature controlling flags not directly accesible from variations.
  // |unforcable_field_trials| contains the list of trials that can not be
  // overridden.
  // |variation_ids| allows for forcing ids selected in chrome://flags and/or
  // specified using the command-line flag.
  // |low_entropy_provider| allows for field trial randomization.
  // |feature_list| contains the list of all active features for this client.
  // |platform_field_trials| provides the platform specific field trial set up
  // for Chrome.
  // |safe_seed_manager| should be notified of the combined server and client
  // state that was activated to create the field trials (only when the return
  // value is true).
  bool SetupFieldTrials(const char* kEnableGpuBenchmarking,
                        const char* kEnableFeatures,
                        const char* kDisableFeatures,
                        const std::set<std::string>& unforceable_field_trials,
                        const std::vector<std::string>& variation_ids,
                        std::unique_ptr<const base::FieldTrial::EntropyProvider>
                            low_entropy_provider,
                        std::unique_ptr<base::FeatureList> feature_list,
                        PlatformFieldTrials* platform_field_trials,
                        SafeSeedManager* safe_seed_manager);

  // Returns all of the client state used for filtering studies.
  // As a side-effect, may update the stored permanent consistency country.
  std::unique_ptr<ClientFilterableState> GetClientFilterableStateForVersion(
      const base::Version& version);

  // Loads the country code to use for filtering permanent consistency studies,
  // updating the stored country code if the stored value was for a different
  // Chrome version. The country used for permanent consistency studies is kept
  // consistent between Chrome upgrades in order to avoid annoying the user due
  // to experiment churn while traveling.
  std::string LoadPermanentConsistencyCountry(
      const base::Version& version,
      const std::string& latest_country);

  // Sets the stored permanent country pref for this client.
  void StorePermanentCountry(const base::Version& version,
                             const std::string& country);

  // Records the time of the most recent successful fetch.
  void RecordLastFetchTime();

  // Allow the platform that is used to filter the set of active trials to be
  // overridden.
  void OverrideVariationsPlatform(Study::Platform platform_override);

  // Returns the short hardware class value used to evaluate variations hardware
  // class filters. Only implemented on CrOS - returns empty string on other
  // platforms.
  static std::string GetShortHardwareClass();

 private:
  // Loads the seed from the variations store into |seed|, and records metrics
  // about the loaded seed. Returns true on success, in which case |seed| will
  // contain the loaded data, and |seed_data| and |base64_signature| will
  // contain the raw pref values.
  bool LoadSeed(VariationsSeed* seed,
                std::string* seed_data,
                std::string* base64_signature) WARN_UNUSED_RESULT;

  // Loads the safe seed from the variations store into |seed| and updates any
  // relevant fields in |client_state|. If the load succeeds, records metrics
  // about the loaded seed. Returns whether the load succeeded.
  bool LoadSafeSeed(VariationsSeed* seed,
                    ClientFilterableState* client_state) WARN_UNUSED_RESULT;

  // Creates field trials based on the variations seed loaded from local state.
  // If there is a problem loading the seed data, all trials specified by the
  // seed may not be created. Some field trials are configured to override or
  // associate with (for reporting) specific features. These associations are
  // registered with |feature_list|. Returns true if trials were created
  // successfully; and if so, stores the loaded variations state into the
  // |safe_seed_manager|.
  bool CreateTrialsFromSeed(
      std::unique_ptr<const base::FieldTrial::EntropyProvider>
          low_entropy_provider,
      base::FeatureList* feature_list,
      SafeSeedManager* safe_seed_manager);

  // Returns the seed store. Virtual for testing.
  virtual VariationsSeedStore* GetSeedStore();

  PrefService* local_state() { return seed_store_.local_state(); }
  const PrefService* local_state() const { return seed_store_.local_state(); }

  VariationsServiceClient* client_;

  UIStringOverrider ui_string_overrider_;

  VariationsSeedStore seed_store_;

  // Tracks whether |CreateTrialsFromSeed| has been called, to ensure that it is
  // called at most once.
  bool create_trials_from_seed_called_;

  // Indiciate if OverrideVariationsPlatform has been used to set
  // |platform_override_|.
  bool has_platform_override_;

  // Platform to be used for variations filtering, overridding the current
  // platform.
  Study::Platform platform_override_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(VariationsFieldTrialCreator);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_SERVICE_VARIATIONS_FIELD_TRIAL_CREATOR_H_
