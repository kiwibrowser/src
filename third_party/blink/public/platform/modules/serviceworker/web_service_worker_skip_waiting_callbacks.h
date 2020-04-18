// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_SKIP_WAITING_CALLBACKS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_SKIP_WAITING_CALLBACKS_H_

#include "third_party/blink/public/platform/web_callbacks.h"

namespace blink {

using WebServiceWorkerSkipWaitingCallbacks = WebCallbacks<void, void>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_SKIP_WAITING_CALLBACKS_H_
