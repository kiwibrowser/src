// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_fetch_request_mojom_traits.h"

#include "base/logging.h"
#include "content/public/common/referrer_struct_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

using blink::mojom::RequestContextType;
using network::mojom::FetchRequestMode;

RequestContextType
EnumTraits<RequestContextType, content::RequestContextType>::ToMojom(
    content::RequestContextType input) {
  switch (input) {
    case content::REQUEST_CONTEXT_TYPE_UNSPECIFIED:
      return RequestContextType::UNSPECIFIED;
    case content::REQUEST_CONTEXT_TYPE_AUDIO:
      return RequestContextType::AUDIO;
    case content::REQUEST_CONTEXT_TYPE_BEACON:
      return RequestContextType::BEACON;
    case content::REQUEST_CONTEXT_TYPE_CSP_REPORT:
      return RequestContextType::CSP_REPORT;
    case content::REQUEST_CONTEXT_TYPE_DOWNLOAD:
      return RequestContextType::DOWNLOAD;
    case content::REQUEST_CONTEXT_TYPE_EMBED:
      return RequestContextType::EMBED;
    case content::REQUEST_CONTEXT_TYPE_EVENT_SOURCE:
      return RequestContextType::EVENT_SOURCE;
    case content::REQUEST_CONTEXT_TYPE_FAVICON:
      return RequestContextType::FAVICON;
    case content::REQUEST_CONTEXT_TYPE_FETCH:
      return RequestContextType::FETCH;
    case content::REQUEST_CONTEXT_TYPE_FONT:
      return RequestContextType::FONT;
    case content::REQUEST_CONTEXT_TYPE_FORM:
      return RequestContextType::FORM;
    case content::REQUEST_CONTEXT_TYPE_FRAME:
      return RequestContextType::FRAME;
    case content::REQUEST_CONTEXT_TYPE_HYPERLINK:
      return RequestContextType::HYPERLINK;
    case content::REQUEST_CONTEXT_TYPE_IFRAME:
      return RequestContextType::IFRAME;
    case content::REQUEST_CONTEXT_TYPE_IMAGE:
      return RequestContextType::IMAGE;
    case content::REQUEST_CONTEXT_TYPE_IMAGE_SET:
      return RequestContextType::IMAGE_SET;
    case content::REQUEST_CONTEXT_TYPE_IMPORT:
      return RequestContextType::IMPORT;
    case content::REQUEST_CONTEXT_TYPE_INTERNAL:
      return RequestContextType::INTERNAL;
    case content::REQUEST_CONTEXT_TYPE_LOCATION:
      return RequestContextType::LOCATION;
    case content::REQUEST_CONTEXT_TYPE_MANIFEST:
      return RequestContextType::MANIFEST;
    case content::REQUEST_CONTEXT_TYPE_OBJECT:
      return RequestContextType::OBJECT;
    case content::REQUEST_CONTEXT_TYPE_PING:
      return RequestContextType::PING;
    case content::REQUEST_CONTEXT_TYPE_PLUGIN:
      return RequestContextType::PLUGIN;
    case content::REQUEST_CONTEXT_TYPE_PREFETCH:
      return RequestContextType::PREFETCH;
    case content::REQUEST_CONTEXT_TYPE_SCRIPT:
      return RequestContextType::SCRIPT;
    case content::REQUEST_CONTEXT_TYPE_SERVICE_WORKER:
      return RequestContextType::SERVICE_WORKER;
    case content::REQUEST_CONTEXT_TYPE_SHARED_WORKER:
      return RequestContextType::SHARED_WORKER;
    case content::REQUEST_CONTEXT_TYPE_SUBRESOURCE:
      return RequestContextType::SUBRESOURCE;
    case content::REQUEST_CONTEXT_TYPE_STYLE:
      return RequestContextType::STYLE;
    case content::REQUEST_CONTEXT_TYPE_TRACK:
      return RequestContextType::TRACK;
    case content::REQUEST_CONTEXT_TYPE_VIDEO:
      return RequestContextType::VIDEO;
    case content::REQUEST_CONTEXT_TYPE_WORKER:
      return RequestContextType::WORKER;
    case content::REQUEST_CONTEXT_TYPE_XML_HTTP_REQUEST:
      return RequestContextType::XML_HTTP_REQUEST;
    case content::REQUEST_CONTEXT_TYPE_XSLT:
      return RequestContextType::XSLT;
  }

  NOTREACHED();
  return RequestContextType::UNSPECIFIED;
}

bool EnumTraits<RequestContextType, content::RequestContextType>::FromMojom(
    RequestContextType input,
    content::RequestContextType* out) {
  switch (input) {
    case RequestContextType::UNSPECIFIED:
      *out = content::REQUEST_CONTEXT_TYPE_UNSPECIFIED;
      return true;
    case RequestContextType::AUDIO:
      *out = content::REQUEST_CONTEXT_TYPE_AUDIO;
      return true;
    case RequestContextType::BEACON:
      *out = content::REQUEST_CONTEXT_TYPE_BEACON;
      return true;
    case RequestContextType::CSP_REPORT:
      *out = content::REQUEST_CONTEXT_TYPE_CSP_REPORT;
      return true;
    case RequestContextType::DOWNLOAD:
      *out = content::REQUEST_CONTEXT_TYPE_DOWNLOAD;
      return true;
    case RequestContextType::EMBED:
      *out = content::REQUEST_CONTEXT_TYPE_EMBED;
      return true;
    case RequestContextType::EVENT_SOURCE:
      *out = content::REQUEST_CONTEXT_TYPE_EVENT_SOURCE;
      return true;
    case RequestContextType::FAVICON:
      *out = content::REQUEST_CONTEXT_TYPE_FAVICON;
      return true;
    case RequestContextType::FETCH:
      *out = content::REQUEST_CONTEXT_TYPE_FETCH;
      return true;
    case RequestContextType::FONT:
      *out = content::REQUEST_CONTEXT_TYPE_FONT;
      return true;
    case RequestContextType::FORM:
      *out = content::REQUEST_CONTEXT_TYPE_FORM;
      return true;
    case RequestContextType::FRAME:
      *out = content::REQUEST_CONTEXT_TYPE_FRAME;
      return true;
    case RequestContextType::HYPERLINK:
      *out = content::REQUEST_CONTEXT_TYPE_HYPERLINK;
      return true;
    case RequestContextType::IFRAME:
      *out = content::REQUEST_CONTEXT_TYPE_IFRAME;
      return true;
    case RequestContextType::IMAGE:
      *out = content::REQUEST_CONTEXT_TYPE_IMAGE;
      return true;
    case RequestContextType::IMAGE_SET:
      *out = content::REQUEST_CONTEXT_TYPE_IMAGE_SET;
      return true;
    case RequestContextType::IMPORT:
      *out = content::REQUEST_CONTEXT_TYPE_IMPORT;
      return true;
    case RequestContextType::INTERNAL:
      *out = content::REQUEST_CONTEXT_TYPE_INTERNAL;
      return true;
    case RequestContextType::LOCATION:
      *out = content::REQUEST_CONTEXT_TYPE_LOCATION;
      return true;
    case RequestContextType::MANIFEST:
      *out = content::REQUEST_CONTEXT_TYPE_MANIFEST;
      return true;
    case RequestContextType::OBJECT:
      *out = content::REQUEST_CONTEXT_TYPE_OBJECT;
      return true;
    case RequestContextType::PING:
      *out = content::REQUEST_CONTEXT_TYPE_PING;
      return true;
    case RequestContextType::PLUGIN:
      *out = content::REQUEST_CONTEXT_TYPE_PLUGIN;
      return true;
    case RequestContextType::PREFETCH:
      *out = content::REQUEST_CONTEXT_TYPE_PREFETCH;
      return true;
    case RequestContextType::SCRIPT:
      *out = content::REQUEST_CONTEXT_TYPE_SCRIPT;
      return true;
    case RequestContextType::SERVICE_WORKER:
      *out = content::REQUEST_CONTEXT_TYPE_SERVICE_WORKER;
      return true;
    case RequestContextType::SHARED_WORKER:
      *out = content::REQUEST_CONTEXT_TYPE_SHARED_WORKER;
      return true;
    case RequestContextType::SUBRESOURCE:
      *out = content::REQUEST_CONTEXT_TYPE_SUBRESOURCE;
      return true;
    case RequestContextType::STYLE:
      *out = content::REQUEST_CONTEXT_TYPE_STYLE;
      return true;
    case RequestContextType::TRACK:
      *out = content::REQUEST_CONTEXT_TYPE_TRACK;
      return true;
    case RequestContextType::VIDEO:
      *out = content::REQUEST_CONTEXT_TYPE_VIDEO;
      return true;
    case RequestContextType::WORKER:
      *out = content::REQUEST_CONTEXT_TYPE_WORKER;
      return true;
    case RequestContextType::XML_HTTP_REQUEST:
      *out = content::REQUEST_CONTEXT_TYPE_XML_HTTP_REQUEST;
      return true;
    case RequestContextType::XSLT:
      *out = content::REQUEST_CONTEXT_TYPE_XSLT;
      return true;
  }

  return false;
}

bool StructTraits<blink::mojom::FetchAPIRequestDataView,
                  content::ServiceWorkerFetchRequest>::
    Read(blink::mojom::FetchAPIRequestDataView data,
         content::ServiceWorkerFetchRequest* out) {
  std::unordered_map<std::string, std::string> headers;
  blink::mojom::SerializedBlobPtr serialized_blob_ptr;
  if (!data.ReadMode(&out->mode) ||
      !data.ReadRequestContextType(&out->request_context_type) ||
      !data.ReadFrameType(&out->frame_type) || !data.ReadUrl(&out->url) ||
      !data.ReadMethod(&out->method) || !data.ReadHeaders(&headers) ||
      !data.ReadBlob(&serialized_blob_ptr) ||
      !data.ReadReferrer(&out->referrer) ||
      !data.ReadCredentialsMode(&out->credentials_mode) ||
      !data.ReadRedirectMode(&out->redirect_mode) ||
      !data.ReadIntegrity(&out->integrity) ||
      !data.ReadClientId(&out->client_id)) {
    return false;
  }

  // content::ServiceWorkerFetchRequest doesn't support request body.
  if (serialized_blob_ptr)
    return false;

  out->is_main_resource_load = data.is_main_resource_load();
  out->headers.insert(headers.begin(), headers.end());
  out->cache_mode = data.cache_mode();
  out->keepalive = data.keepalive();
  out->is_reload = data.is_reload();
  return true;
}

}  // namespace mojo
