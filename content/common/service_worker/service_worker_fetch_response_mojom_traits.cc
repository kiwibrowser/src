// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_fetch_response_mojom_traits.h"

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/referrer_struct_traits.h"
#include "ipc/ipc_message_utils.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

bool StructTraits<blink::mojom::FetchAPIResponseDataView,
                  content::ServiceWorkerResponse>::
    Read(blink::mojom::FetchAPIResponseDataView data,
         content::ServiceWorkerResponse* out) {
  blink::mojom::SerializedBlobPtr serialized_blob_ptr;
  blink::mojom::SerializedBlobPtr serialized_side_data_blob_ptr;
  if (!data.ReadUrlList(&out->url_list) ||
      !data.ReadStatusText(&out->status_text) ||
      !data.ReadResponseType(&out->response_type) ||
      !data.ReadHeaders(&out->headers) ||
      !data.ReadBlob(&serialized_blob_ptr) || !data.ReadError(&out->error) ||
      !data.ReadResponseTime(&out->response_time) ||
      !data.ReadCacheStorageCacheName(&out->cache_storage_cache_name) ||
      !data.ReadCorsExposedHeaderNames(&out->cors_exposed_header_names) ||
      !data.ReadSideDataBlob(&serialized_side_data_blob_ptr)) {
    return false;
  }

  out->status_code = data.status_code();
  out->is_in_cache_storage = data.is_in_cache_storage();

  if (serialized_blob_ptr) {
    out->blob_uuid = serialized_blob_ptr->uuid;
    out->blob_size = serialized_blob_ptr->size;
    blink::mojom::BlobPtr blob_ptr;
    blob_ptr.Bind(std::move(serialized_blob_ptr->blob));
    out->blob = base::MakeRefCounted<storage::BlobHandle>(std::move(blob_ptr));
  }

  if (serialized_side_data_blob_ptr) {
    out->side_data_blob_uuid = serialized_side_data_blob_ptr->uuid;
    out->side_data_blob_size = serialized_side_data_blob_ptr->size;
    blink::mojom::BlobPtr blob_ptr;
    blob_ptr.Bind(std::move(serialized_side_data_blob_ptr->blob));
    out->side_data_blob =
        base::MakeRefCounted<storage::BlobHandle>(std::move(blob_ptr));
  }

  return true;
}

}  // namespace mojo
