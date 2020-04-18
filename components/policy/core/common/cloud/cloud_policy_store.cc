// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/cloud_policy_store.h"

#include "base/logging.h"
#include "components/policy/core/common/cloud/cloud_external_data_manager.h"

namespace policy {

CloudPolicyStore::Observer::~Observer() {}

CloudPolicyStore::CloudPolicyStore()
    : status_(STATUS_OK),
      validation_status_(CloudPolicyValidatorBase::VALIDATION_OK),
      invalidation_version_(0),
      is_initialized_(false) {}

CloudPolicyStore::~CloudPolicyStore() {
  DCHECK(!external_data_manager_);
}

void CloudPolicyStore::Store(
    const enterprise_management::PolicyFetchResponse& policy,
    int64_t invalidation_version) {
  invalidation_version_ = invalidation_version;
  Store(policy);
}

void CloudPolicyStore::AddObserver(CloudPolicyStore::Observer* observer) {
  observers_.AddObserver(observer);
}

void CloudPolicyStore::RemoveObserver(CloudPolicyStore::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CloudPolicyStore::NotifyStoreLoaded() {
  is_initialized_ = true;
  // The |external_data_manager_| must be notified first so that when other
  // observers are informed about the changed policies and try to fetch external
  // data referenced by these, the |external_data_manager_| has the required
  // metadata already.
  if (external_data_manager_)
    external_data_manager_->OnPolicyStoreLoaded();
  for (auto& observer : observers_)
    observer.OnStoreLoaded(this);
}

void CloudPolicyStore::NotifyStoreError() {
  is_initialized_ = true;
  for (auto& observer : observers_)
    observer.OnStoreError(this);
}

void CloudPolicyStore::SetExternalDataManager(
    base::WeakPtr<CloudExternalDataManager> external_data_manager) {
  DCHECK(!external_data_manager_);
  external_data_manager_ = external_data_manager;
  if (is_initialized_)
    external_data_manager_->OnPolicyStoreLoaded();
}

void CloudPolicyStore::SetPolicyMapForTesting(const PolicyMap& policy_map) {
  policy_map_.CopyFrom(policy_map);
  NotifyStoreLoaded();
}

}  // namespace policy
