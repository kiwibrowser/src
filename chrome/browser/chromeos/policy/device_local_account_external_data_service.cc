// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_local_account_external_data_service.h"

#include <set>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "chromeos/chromeos_paths.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/policy_constants.h"

namespace policy {

DeviceLocalAccountExternalDataService::DeviceLocalAccountExternalDataService(
    DeviceLocalAccountPolicyService* parent,
    scoped_refptr<base::SequencedTaskRunner> backend_task_runner,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : parent_(parent),
      backend_task_runner_(backend_task_runner),
      io_task_runner_(io_task_runner) {
  base::FilePath cache_dir;
  CHECK(base::PathService::Get(chromeos::DIR_DEVICE_LOCAL_ACCOUNT_EXTERNAL_DATA,
                               &cache_dir));
  resource_cache_.reset(new ResourceCache(cache_dir, backend_task_runner));
  parent_->AddObserver(this);
}

DeviceLocalAccountExternalDataService::
    ~DeviceLocalAccountExternalDataService() {
  parent_->RemoveObserver(this);
#if !defined(NDEBUG)
  for (ExternalDataManagerMap::const_iterator it =
           external_data_managers_.begin();
       it != external_data_managers_.end(); ++it) {
    DCHECK(it->second->HasOneRef());
  }
#endif  // !defined(NDEBUG)
  backend_task_runner_->DeleteSoon(FROM_HERE, resource_cache_.release());
}

void DeviceLocalAccountExternalDataService::OnPolicyUpdated(
    const std::string& user_id) {
}

void DeviceLocalAccountExternalDataService::OnDeviceLocalAccountsChanged() {
  std::set<std::string> account_ids;
  for (ExternalDataManagerMap::iterator it = external_data_managers_.begin();
       it != external_data_managers_.end(); ) {
    if (it->second->HasOneRef()) {
      external_data_managers_.erase(it++);
    } else {
      account_ids.insert(it->first);
      ++it;
    }
  }
  backend_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ResourceCache::PurgeOtherKeys,
                     base::Unretained(resource_cache_.get()), account_ids));
}

scoped_refptr<DeviceLocalAccountExternalDataManager>
    DeviceLocalAccountExternalDataService::GetExternalDataManager(
        const std::string& account_id,
        CloudPolicyStore* policy_store) {
  scoped_refptr<DeviceLocalAccountExternalDataManager>& external_data_manager =
      external_data_managers_[account_id];
  if (!external_data_manager.get()) {
    external_data_manager = new DeviceLocalAccountExternalDataManager(
        account_id,
        base::Bind(&GetChromePolicyDetails),
        backend_task_runner_,
        io_task_runner_,
        resource_cache_.get());
  }
  external_data_manager->SetPolicyStore(policy_store);
  return external_data_manager;
}

}  // namespace policy
