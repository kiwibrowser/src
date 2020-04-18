// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_provider_struct_traits.h"

namespace mojo {

bool StructTraits<content::mojom::ServiceWorkerProviderHostInfoDataView,
                  content::ServiceWorkerProviderHostInfo>::
    Read(content::mojom::ServiceWorkerProviderHostInfoDataView in,
         content::ServiceWorkerProviderHostInfo* out) {
  if (!in.ReadType(&out->type))
    return false;
  out->provider_id = in.provider_id();
  out->route_id = in.route_id();
  out->is_parent_frame_secure = in.is_parent_frame_secure();
  out->host_request = in.TakeHostRequest<
      content::mojom::ServiceWorkerContainerHostAssociatedRequest>();
  out->client_ptr_info = in.TakeClientPtrInfo<
      content::mojom::ServiceWorkerContainerAssociatedPtrInfo>();
  return true;
}

}  // namespace mojo
