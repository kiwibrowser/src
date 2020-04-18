// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_CLIENT_QUERY_OPTIONS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_CLIENT_QUERY_OPTIONS_H_

#include "third_party/blink/public/mojom/service_worker/service_worker_client.mojom-shared.h"

namespace blink {

struct WebServiceWorkerClientQueryOptions {
  WebServiceWorkerClientQueryOptions()
      : client_type(mojom::ServiceWorkerClientType::kWindow),
        include_uncontrolled(false) {}

  mojom::ServiceWorkerClientType client_type;
  bool include_uncontrolled;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_CLIENT_QUERY_OPTIONS_H_
