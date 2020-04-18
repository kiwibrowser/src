// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_CLIENTS_CLAIM_CALLBACKS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_CLIENTS_CLAIM_CALLBACKS_H_

#include "third_party/blink/public/platform/web_callbacks.h"

namespace blink {

struct WebServiceWorkerError;

using WebServiceWorkerClientsClaimCallbacks =
    WebCallbacks<void, const WebServiceWorkerError&>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_CLIENTS_CLAIM_CALLBACKS_H_
