// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/cookie_store/cookie_store_host.h"

#include <utility>

#include "content/browser/cookie_store/cookie_store_manager.h"
#include "url/origin.h"

namespace content {

CookieStoreHost::CookieStoreHost(CookieStoreManager* manager,
                                 const url::Origin& origin)
    : manager_(manager), origin_(origin) {}

CookieStoreHost::~CookieStoreHost() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void CookieStoreHost::AppendSubscriptions(
    int64_t service_worker_registration_id,
    std::vector<blink::mojom::CookieChangeSubscriptionPtr> subscriptions,
    AppendSubscriptionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  manager_->AppendSubscriptions(service_worker_registration_id, origin_,
                                std::move(subscriptions), std::move(callback));
}

void CookieStoreHost::GetSubscriptions(int64_t service_worker_registration_id,
                                       GetSubscriptionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  manager_->GetSubscriptions(service_worker_registration_id, origin_,
                             std::move(callback));
}

}  // namespace content
