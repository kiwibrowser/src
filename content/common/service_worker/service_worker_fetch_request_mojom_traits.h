// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_FETCH_REQUEST_MOJOM_TRAITS_H_
#define CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_FETCH_REQUEST_MOJOM_TRAITS_H_

#include "base/numerics/safe_conversions.h"
#include "content/public/common/referrer.h"
#include "services/network/public/mojom/request_context_frame_type.mojom.h"
#include "storage/common/blob_storage/blob_handle.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom.h"

namespace mojo {

template <>
struct EnumTraits<blink::mojom::RequestContextType,
                  content::RequestContextType> {
  static blink::mojom::RequestContextType ToMojom(
      content::RequestContextType input);

  static bool FromMojom(blink::mojom::RequestContextType input,
                        content::RequestContextType* out);
};

template <>
struct StructTraits<blink::mojom::FetchAPIRequestDataView,
                    content::ServiceWorkerFetchRequest> {
  static network::mojom::FetchRequestMode mode(
      const content::ServiceWorkerFetchRequest& request) {
    return request.mode;
  }

  static bool is_main_resource_load(
      const content::ServiceWorkerFetchRequest& request) {
    return request.is_main_resource_load;
  }

  static content::RequestContextType request_context_type(
      const content::ServiceWorkerFetchRequest& request) {
    return request.request_context_type;
  }

  static network::mojom::RequestContextFrameType frame_type(
      const content::ServiceWorkerFetchRequest& request) {
    return request.frame_type;
  }

  static const GURL& url(const content::ServiceWorkerFetchRequest& request) {
    return request.url;
  }

  static const std::string& method(
      const content::ServiceWorkerFetchRequest& request) {
    return request.method;
  }

  static std::map<std::string,
                  std::string,
                  content::ServiceWorkerCaseInsensitiveCompare>
  headers(const content::ServiceWorkerFetchRequest& request) {
    return request.headers;
  }

  // content::ServiceWorkerFetchRequest does not support the request body.
  static blink::mojom::SerializedBlobPtr blob(
      const content::ServiceWorkerFetchRequest& request) {
    return nullptr;
  }

  static const content::Referrer& referrer(
      const content::ServiceWorkerFetchRequest& request) {
    return request.referrer;
  }

  static network::mojom::FetchCredentialsMode credentials_mode(
      const content::ServiceWorkerFetchRequest& request) {
    return request.credentials_mode;
  }

  static blink::mojom::FetchCacheMode cache_mode(
      const content::ServiceWorkerFetchRequest& request) {
    return request.cache_mode;
  }

  static network::mojom::FetchRedirectMode redirect_mode(
      const content::ServiceWorkerFetchRequest& request) {
    return request.redirect_mode;
  }

  static const std::string& integrity(
      const content::ServiceWorkerFetchRequest& request) {
    return request.integrity;
  }

  static bool keepalive(const content::ServiceWorkerFetchRequest& request) {
    return request.keepalive;
  }

  static const std::string& client_id(
      const content::ServiceWorkerFetchRequest& request) {
    return request.client_id;
  }

  static bool is_reload(const content::ServiceWorkerFetchRequest& request) {
    return request.is_reload;
  }

  static bool Read(blink::mojom::FetchAPIRequestDataView data,
                   content::ServiceWorkerFetchRequest* out);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_FETCH_REQUEST_MOJOM_TRAITS_H_
