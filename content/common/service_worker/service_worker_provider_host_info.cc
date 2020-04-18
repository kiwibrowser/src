// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_provider_host_info.h"

#include "ipc/ipc_message.h"

namespace content {

namespace {

void SetDefaultValues(ServiceWorkerProviderHostInfo* info) {
  info->provider_id = kInvalidServiceWorkerProviderId;
  info->route_id = MSG_ROUTING_NONE;
  info->type = blink::mojom::ServiceWorkerProviderType::kUnknown;
  info->is_parent_frame_secure = false;
}

}  // namespace

ServiceWorkerProviderHostInfo::ServiceWorkerProviderHostInfo()
    : provider_id(kInvalidServiceWorkerProviderId),
      route_id(MSG_ROUTING_NONE),
      type(blink::mojom::ServiceWorkerProviderType::kUnknown),
      is_parent_frame_secure(false) {}

ServiceWorkerProviderHostInfo::ServiceWorkerProviderHostInfo(
    ServiceWorkerProviderHostInfo&& other)
    : provider_id(other.provider_id),
      route_id(other.route_id),
      type(other.type),
      is_parent_frame_secure(other.is_parent_frame_secure),
      host_request(std::move(other.host_request)),
      client_ptr_info(std::move(other.client_ptr_info)) {
  SetDefaultValues(&other);
}

ServiceWorkerProviderHostInfo::ServiceWorkerProviderHostInfo(
    ServiceWorkerProviderHostInfo&& other,
    mojom::ServiceWorkerContainerHostAssociatedRequest host_request,
    mojom::ServiceWorkerContainerAssociatedPtrInfo client_ptr_info)
    : provider_id(other.provider_id),
      route_id(other.route_id),
      type(other.type),
      is_parent_frame_secure(other.is_parent_frame_secure),
      host_request(std::move(host_request)),
      client_ptr_info(std::move(client_ptr_info)) {
  DCHECK(!other.host_request.is_pending());
  DCHECK(!other.client_ptr_info.is_valid());
  SetDefaultValues(&other);
}

ServiceWorkerProviderHostInfo::ServiceWorkerProviderHostInfo(
    int provider_id,
    int route_id,
    blink::mojom::ServiceWorkerProviderType type,
    bool is_parent_frame_secure)
    : provider_id(provider_id),
      route_id(route_id),
      type(type),
      is_parent_frame_secure(is_parent_frame_secure) {}

ServiceWorkerProviderHostInfo::~ServiceWorkerProviderHostInfo() {}

ServiceWorkerProviderHostInfo& ServiceWorkerProviderHostInfo::operator=(
    ServiceWorkerProviderHostInfo&& other) {
  provider_id = other.provider_id;
  route_id = other.route_id;
  type = other.type;
  is_parent_frame_secure = other.is_parent_frame_secure;

  SetDefaultValues(&other);
  return *this;
}

}  // namespace content
