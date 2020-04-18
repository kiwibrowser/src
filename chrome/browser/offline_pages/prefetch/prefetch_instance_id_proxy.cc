// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/prefetch_instance_id_proxy.h"

#include <map>

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/gcm/instance_id/instance_id_profile_service_factory.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"
#include "components/gcm_driver/instance_id/instance_id_profile_service.h"
#include "components/offline_pages/core/offline_page_feature.h"

using instance_id::InstanceID;
using instance_id::InstanceIDProfileService;
using instance_id::InstanceIDProfileServiceFactory;

namespace {

const char kScopeGCM[] = "GCM";
const char kProdSenderId[] = "864229763856";

}  // namespace

namespace offline_pages {

PrefetchInstanceIDProxy::PrefetchInstanceIDProxy(
    const std::string& app_id,
    content::BrowserContext* context)
    : app_id_(app_id), context_(context), weak_factory_(this) {
}

PrefetchInstanceIDProxy::~PrefetchInstanceIDProxy() = default;

void PrefetchInstanceIDProxy::GetGCMToken(
    InstanceID::GetTokenCallback callback) {
  DCHECK(IsPrefetchingOfflinePagesEnabled());
  if (!token_.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&PrefetchInstanceIDProxy::GotGCMToken,
                              weak_factory_.GetWeakPtr(), callback, token_,
                              InstanceID::SUCCESS));
    return;
  }

  InstanceIDProfileService* service =
      InstanceIDProfileServiceFactory::GetForProfile(context_);
  DCHECK(service);

  InstanceID* instance_id = service->driver()->GetInstanceID(app_id_);
  DCHECK(instance_id);

  instance_id->GetToken(kProdSenderId, kScopeGCM,
                        std::map<std::string, std::string>(),
                        base::Bind(&PrefetchInstanceIDProxy::GotGCMToken,
                                   weak_factory_.GetWeakPtr(), callback));
}

void PrefetchInstanceIDProxy::GotGCMToken(InstanceID::GetTokenCallback callback,
                                          const std::string& token,
                                          InstanceID::Result result) {
  DVLOG(1) << "Got an Instance ID token for GCM: " << token
           << " with result: " << result;
  callback.Run(token, result);
}

}  // namespace offline_pages
