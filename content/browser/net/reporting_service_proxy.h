// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NET_REPORTING_SERVICE_PROXY_H_
#define CONTENT_BROWSER_NET_REPORTING_SERVICE_PROXY_H_

#include "third_party/blink/public/platform/reporting.mojom.h"

namespace content {

class StoragePartition;

void CreateReportingServiceProxy(
    StoragePartition* storage_partition,
    blink::mojom::ReportingServiceProxyRequest request);

}  // namespace content

#endif  // CONTENT_BROWSER_NET_REPORTING_SERVICE_PROXY_H_
