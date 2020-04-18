// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/synthetic_trial_registry.h"

namespace variations {

SyntheticTrialRegistry::SyntheticTrialRegistry() = default;
SyntheticTrialRegistry::~SyntheticTrialRegistry() = default;

void SyntheticTrialRegistry::AddSyntheticTrialObserver(
    SyntheticTrialObserver* observer) {
  synthetic_trial_observer_list_.AddObserver(observer);
  if (!synthetic_trial_groups_.empty())
    observer->OnSyntheticTrialsChanged(synthetic_trial_groups_);
}

void SyntheticTrialRegistry::RemoveSyntheticTrialObserver(
    SyntheticTrialObserver* observer) {
  synthetic_trial_observer_list_.RemoveObserver(observer);
}

void SyntheticTrialRegistry::RegisterSyntheticFieldTrial(
    const SyntheticTrialGroup& trial) {
  for (size_t i = 0; i < synthetic_trial_groups_.size(); ++i) {
    if (synthetic_trial_groups_[i].id.name == trial.id.name) {
      if (synthetic_trial_groups_[i].id.group != trial.id.group) {
        synthetic_trial_groups_[i].id.group = trial.id.group;
        synthetic_trial_groups_[i].start_time = base::TimeTicks::Now();
        NotifySyntheticTrialObservers();
      }
      return;
    }
  }

  SyntheticTrialGroup trial_group = trial;
  trial_group.start_time = base::TimeTicks::Now();
  synthetic_trial_groups_.push_back(trial_group);
  NotifySyntheticTrialObservers();
}

void SyntheticTrialRegistry::RegisterSyntheticMultiGroupFieldTrial(
    uint32_t trial_name_hash,
    const std::vector<uint32_t>& group_name_hashes) {
  auto has_same_trial_name = [trial_name_hash](const SyntheticTrialGroup& x) {
    return x.id.name == trial_name_hash;
  };
  synthetic_trial_groups_.erase(
      std::remove_if(synthetic_trial_groups_.begin(),
                     synthetic_trial_groups_.end(), has_same_trial_name),
      synthetic_trial_groups_.end());

  if (group_name_hashes.empty())
    return;

  SyntheticTrialGroup trial_group(trial_name_hash, group_name_hashes[0]);
  trial_group.start_time = base::TimeTicks::Now();
  for (uint32_t group_name_hash : group_name_hashes) {
    // Note: Adding the trial group will copy it, so this re-uses the same
    // |trial_group| struct for convenience (e.g. so start_time's all match).
    trial_group.id.group = group_name_hash;
    synthetic_trial_groups_.push_back(trial_group);
  }
  NotifySyntheticTrialObservers();
}

void SyntheticTrialRegistry::NotifySyntheticTrialObservers() {
  for (SyntheticTrialObserver& observer : synthetic_trial_observer_list_) {
    observer.OnSyntheticTrialsChanged(synthetic_trial_groups_);
  }
}

void SyntheticTrialRegistry::GetSyntheticFieldTrialsOlderThan(
    base::TimeTicks time,
    std::vector<ActiveGroupId>* synthetic_trials) {
  DCHECK(synthetic_trials);
  synthetic_trials->clear();
  for (size_t i = 0; i < synthetic_trial_groups_.size(); ++i) {
    if (synthetic_trial_groups_[i].start_time <= time)
      synthetic_trials->push_back(synthetic_trial_groups_[i].id);
  }
}

}  // namespace variations
