// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/utility/aw_content_utility_client.h"

#include "components/services/heap_profiling/heap_profiling_service.h"
#include "components/services/heap_profiling/public/mojom/constants.mojom.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace android_webview {

AwContentUtilityClient::AwContentUtilityClient() = default;
AwContentUtilityClient::~AwContentUtilityClient() = default;

void AwContentUtilityClient::UtilityThreadStarted() {
  content::ServiceManagerConnection* connection =
      content::ChildThread::Get()->GetServiceManagerConnection();
  DCHECK(connection);

  auto registry = std::make_unique<service_manager::BinderRegistry>();
  connection->AddConnectionFilter(
      std::make_unique<content::SimpleConnectionFilter>(std::move(registry)));
}

void AwContentUtilityClient::RegisterServices(
    AwContentUtilityClient::StaticServiceMap* services) {
  service_manager::EmbeddedServiceInfo profiling_info;
  profiling_info.task_runner = content::ChildThread::Get()->GetIOTaskRunner();
  profiling_info.factory =
      base::BindRepeating(&heap_profiling::HeapProfilingService::CreateService);
  services->emplace(heap_profiling::mojom::kServiceName, profiling_info);
}

}  // namespace android_webview
