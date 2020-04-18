// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_CACHE_STORAGE_CACHE_STORAGE_MOJOM_TRAITS_H_
#define CONTENT_COMMON_CACHE_STORAGE_CACHE_STORAGE_MOJOM_TRAITS_H_

#include <map>
#include <string>
#include <vector>

#include "base/optional.h"
#include "base/strings/string16.h"
#include "content/common/service_worker/service_worker_fetch_request_mojom_traits.h"
#include "content/common/service_worker/service_worker_fetch_response_mojom_traits.h"
#include "content/common/service_worker/service_worker_types.h"
#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "third_party/blink/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "url/gurl.h"

namespace mojo {

template <>
struct EnumTraits<blink::mojom::OperationType,
                  content::CacheStorageCacheOperationType> {
  static blink::mojom::OperationType ToMojom(
      content::CacheStorageCacheOperationType input);
  static bool FromMojom(blink::mojom::OperationType input,
                        content::CacheStorageCacheOperationType* out);
};

template <>
struct StructTraits<blink::mojom::QueryParamsDataView,
                    content::CacheStorageCacheQueryParams> {
  static bool ignore_search(
      const content::CacheStorageCacheQueryParams& query_params) {
    return query_params.ignore_search;
  }
  static bool ignore_method(
      const content::CacheStorageCacheQueryParams& query_params) {
    return query_params.ignore_method;
  }
  static bool ignore_vary(
      const content::CacheStorageCacheQueryParams& query_params) {
    return query_params.ignore_vary;
  }
  static const base::Optional<base::string16>& cache_name(
      const content::CacheStorageCacheQueryParams& query_params) {
    return query_params.cache_name.as_optional_string16();
  }
  static bool Read(blink::mojom::QueryParamsDataView,
                   content::CacheStorageCacheQueryParams* output);
};

template <>
struct StructTraits<blink::mojom::BatchOperationDataView,
                    content::CacheStorageBatchOperation> {
  static content::CacheStorageCacheOperationType operation_type(
      const content::CacheStorageBatchOperation& batch_operation) {
    return batch_operation.operation_type;
  }
  static content::ServiceWorkerFetchRequest request(
      const content::CacheStorageBatchOperation& batch_operation) {
    return batch_operation.request;
  }
  static content::ServiceWorkerResponse response(
      const content::CacheStorageBatchOperation& batch_operation) {
    return batch_operation.response;
  }
  static content::CacheStorageCacheQueryParams match_params(
      const content::CacheStorageBatchOperation& batch_operation) {
    return batch_operation.match_params;
  }
  static bool Read(blink::mojom::BatchOperationDataView,
                   content::CacheStorageBatchOperation* output);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_CACHE_STORAGE_CACHE_STORAGE_MOJOM_TRAITS_H_
