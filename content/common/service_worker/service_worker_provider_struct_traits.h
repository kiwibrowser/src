// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_PROVIDER_STRUCT_TRAITS_H_
#define CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_PROVIDER_STRUCT_TRAITS_H_

#include "content/common/service_worker/service_worker_provider.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_provider_type.mojom.h"

namespace mojo {

template <>
struct StructTraits<content::mojom::ServiceWorkerProviderHostInfoDataView,
                    content::ServiceWorkerProviderHostInfo> {
  static int32_t provider_id(
      const content::ServiceWorkerProviderHostInfo& info) {
    return info.provider_id;
  }

  static int32_t route_id(const content::ServiceWorkerProviderHostInfo& info) {
    return info.route_id;
  }

  static blink::mojom::ServiceWorkerProviderType type(
      const content::ServiceWorkerProviderHostInfo& info) {
    return info.type;
  }

  static bool is_parent_frame_secure(
      const content::ServiceWorkerProviderHostInfo& info) {
    return info.is_parent_frame_secure;
  }

  static content::mojom::ServiceWorkerContainerHostAssociatedRequest&
  host_request(content::ServiceWorkerProviderHostInfo& info) {
    return info.host_request;
  }

  static content::mojom::ServiceWorkerContainerAssociatedPtrInfo&
  client_ptr_info(content::ServiceWorkerProviderHostInfo& info) {
    return info.client_ptr_info;
  }

  static bool Read(content::mojom::ServiceWorkerProviderHostInfoDataView in,
                   content::ServiceWorkerProviderHostInfo* out);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_PROVIDER_STRUCT_TRAITS_H_
