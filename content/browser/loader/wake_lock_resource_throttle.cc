// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/wake_lock_resource_throttle.h"

#include "content/browser/service_manager/service_manager_context.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {

const int kWakeLockDelaySeconds = 30;

}  // namespace

WakeLockResourceThrottle::WakeLockResourceThrottle(const std::string& host)
    : host_(host) {}

WakeLockResourceThrottle::~WakeLockResourceThrottle() {}

void WakeLockResourceThrottle::WillStartRequest(bool* defer) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Delay wake lock request to dismiss small requests.
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(kWakeLockDelaySeconds),
               this, &WakeLockResourceThrottle::RequestWakeLock);
}

void WakeLockResourceThrottle::WillProcessResponse(bool* defer) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Cancel wake lock after request finishes.
  if (wake_lock_)
    wake_lock_->CancelWakeLock();

  timer_.Stop();
}

const char* WakeLockResourceThrottle::GetNameForLogging() const {
  return "WakeLockResourceThrottle";
}

void WakeLockResourceThrottle::RequestWakeLock() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!wake_lock_);

  service_manager::Connector* connector =
      ServiceManagerContext::GetConnectorForIOThread();
  // |connector| might be nullptr in some testing contexts, in which the
  // service manager connection isn't initialized.
  if (connector) {
    device::mojom::WakeLockProviderPtr wake_lock_provider;
    connector->BindInterface(device::mojom::kServiceName,
                             mojo::MakeRequest(&wake_lock_provider));
    wake_lock_provider->GetWakeLockWithoutContext(
        device::mojom::WakeLockType::kPreventAppSuspension,
        device::mojom::WakeLockReason::kOther, "Uploading data to " + host_,
        mojo::MakeRequest(&wake_lock_));

    wake_lock_->RequestWakeLock();
  }
}

}  // namespace content
