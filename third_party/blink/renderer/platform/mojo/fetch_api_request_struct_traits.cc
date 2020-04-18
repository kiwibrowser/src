// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/mojo/fetch_api_request_struct_traits.h"

#include "mojo/public/cpp/bindings/map_traits_wtf_hash_map.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/public/platform/web_referrer_policy.h"
#include "third_party/blink/renderer/platform/blob/serialized_blob_struct_traits.h"
#include "third_party/blink/renderer/platform/mojo/kurl_struct_traits.h"
#include "third_party/blink/renderer/platform/mojo/referrer_struct_traits.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"

namespace mojo {

using blink::mojom::RequestContextType;

RequestContextType
EnumTraits<RequestContextType, blink::WebURLRequest::RequestContext>::ToMojom(
    blink::WebURLRequest::RequestContext input) {
  switch (input) {
    case blink::WebURLRequest::kRequestContextUnspecified:
      return RequestContextType::UNSPECIFIED;
    case blink::WebURLRequest::kRequestContextAudio:
      return RequestContextType::AUDIO;
    case blink::WebURLRequest::kRequestContextBeacon:
      return RequestContextType::BEACON;
    case blink::WebURLRequest::kRequestContextCSPReport:
      return RequestContextType::CSP_REPORT;
    case blink::WebURLRequest::kRequestContextDownload:
      return RequestContextType::DOWNLOAD;
    case blink::WebURLRequest::kRequestContextEmbed:
      return RequestContextType::EMBED;
    case blink::WebURLRequest::kRequestContextEventSource:
      return RequestContextType::EVENT_SOURCE;
    case blink::WebURLRequest::kRequestContextFavicon:
      return RequestContextType::FAVICON;
    case blink::WebURLRequest::kRequestContextFetch:
      return RequestContextType::FETCH;
    case blink::WebURLRequest::kRequestContextFont:
      return RequestContextType::FONT;
    case blink::WebURLRequest::kRequestContextForm:
      return RequestContextType::FORM;
    case blink::WebURLRequest::kRequestContextFrame:
      return RequestContextType::FRAME;
    case blink::WebURLRequest::kRequestContextHyperlink:
      return RequestContextType::HYPERLINK;
    case blink::WebURLRequest::kRequestContextIframe:
      return RequestContextType::IFRAME;
    case blink::WebURLRequest::kRequestContextImage:
      return RequestContextType::IMAGE;
    case blink::WebURLRequest::kRequestContextImageSet:
      return RequestContextType::IMAGE_SET;
    case blink::WebURLRequest::kRequestContextImport:
      return RequestContextType::IMPORT;
    case blink::WebURLRequest::kRequestContextInternal:
      return RequestContextType::INTERNAL;
    case blink::WebURLRequest::kRequestContextLocation:
      return RequestContextType::LOCATION;
    case blink::WebURLRequest::kRequestContextManifest:
      return RequestContextType::MANIFEST;
    case blink::WebURLRequest::kRequestContextObject:
      return RequestContextType::OBJECT;
    case blink::WebURLRequest::kRequestContextPing:
      return RequestContextType::PING;
    case blink::WebURLRequest::kRequestContextPlugin:
      return RequestContextType::PLUGIN;
    case blink::WebURLRequest::kRequestContextPrefetch:
      return RequestContextType::PREFETCH;
    case blink::WebURLRequest::kRequestContextScript:
      return RequestContextType::SCRIPT;
    case blink::WebURLRequest::kRequestContextServiceWorker:
      return RequestContextType::SERVICE_WORKER;
    case blink::WebURLRequest::kRequestContextSharedWorker:
      return RequestContextType::SHARED_WORKER;
    case blink::WebURLRequest::kRequestContextSubresource:
      return RequestContextType::SUBRESOURCE;
    case blink::WebURLRequest::kRequestContextStyle:
      return RequestContextType::STYLE;
    case blink::WebURLRequest::kRequestContextTrack:
      return RequestContextType::TRACK;
    case blink::WebURLRequest::kRequestContextVideo:
      return RequestContextType::VIDEO;
    case blink::WebURLRequest::kRequestContextWorker:
      return RequestContextType::WORKER;
    case blink::WebURLRequest::kRequestContextXMLHttpRequest:
      return RequestContextType::XML_HTTP_REQUEST;
    case blink::WebURLRequest::kRequestContextXSLT:
      return RequestContextType::XSLT;
  }

  NOTREACHED();
  return RequestContextType::UNSPECIFIED;
}

bool EnumTraits<RequestContextType, blink::WebURLRequest::RequestContext>::
    FromMojom(RequestContextType input,
              blink::WebURLRequest::RequestContext* out) {
  switch (input) {
    case RequestContextType::UNSPECIFIED:
      *out = blink::WebURLRequest::kRequestContextUnspecified;
      return true;
    case RequestContextType::AUDIO:
      *out = blink::WebURLRequest::kRequestContextAudio;
      return true;
    case RequestContextType::BEACON:
      *out = blink::WebURLRequest::kRequestContextBeacon;
      return true;
    case RequestContextType::CSP_REPORT:
      *out = blink::WebURLRequest::kRequestContextCSPReport;
      return true;
    case RequestContextType::DOWNLOAD:
      *out = blink::WebURLRequest::kRequestContextDownload;
      return true;
    case RequestContextType::EMBED:
      *out = blink::WebURLRequest::kRequestContextEmbed;
      return true;
    case RequestContextType::EVENT_SOURCE:
      *out = blink::WebURLRequest::kRequestContextEventSource;
      return true;
    case RequestContextType::FAVICON:
      *out = blink::WebURLRequest::kRequestContextFavicon;
      return true;
    case RequestContextType::FETCH:
      *out = blink::WebURLRequest::kRequestContextFetch;
      return true;
    case RequestContextType::FONT:
      *out = blink::WebURLRequest::kRequestContextFont;
      return true;
    case RequestContextType::FORM:
      *out = blink::WebURLRequest::kRequestContextForm;
      return true;
    case RequestContextType::FRAME:
      *out = blink::WebURLRequest::kRequestContextFrame;
      return true;
    case RequestContextType::HYPERLINK:
      *out = blink::WebURLRequest::kRequestContextHyperlink;
      return true;
    case RequestContextType::IFRAME:
      *out = blink::WebURLRequest::kRequestContextIframe;
      return true;
    case RequestContextType::IMAGE:
      *out = blink::WebURLRequest::kRequestContextImage;
      return true;
    case RequestContextType::IMAGE_SET:
      *out = blink::WebURLRequest::kRequestContextImageSet;
      return true;
    case RequestContextType::IMPORT:
      *out = blink::WebURLRequest::kRequestContextImport;
      return true;
    case RequestContextType::INTERNAL:
      *out = blink::WebURLRequest::kRequestContextInternal;
      return true;
    case RequestContextType::LOCATION:
      *out = blink::WebURLRequest::kRequestContextLocation;
      return true;
    case RequestContextType::MANIFEST:
      *out = blink::WebURLRequest::kRequestContextManifest;
      return true;
    case RequestContextType::OBJECT:
      *out = blink::WebURLRequest::kRequestContextObject;
      return true;
    case RequestContextType::PING:
      *out = blink::WebURLRequest::kRequestContextPing;
      return true;
    case RequestContextType::PLUGIN:
      *out = blink::WebURLRequest::kRequestContextPlugin;
      return true;
    case RequestContextType::PREFETCH:
      *out = blink::WebURLRequest::kRequestContextPrefetch;
      return true;
    case RequestContextType::SCRIPT:
      *out = blink::WebURLRequest::kRequestContextScript;
      return true;
    case RequestContextType::SERVICE_WORKER:
      *out = blink::WebURLRequest::kRequestContextServiceWorker;
      return true;
    case RequestContextType::SHARED_WORKER:
      *out = blink::WebURLRequest::kRequestContextSharedWorker;
      return true;
    case RequestContextType::SUBRESOURCE:
      *out = blink::WebURLRequest::kRequestContextSubresource;
      return true;
    case RequestContextType::STYLE:
      *out = blink::WebURLRequest::kRequestContextStyle;
      return true;
    case RequestContextType::TRACK:
      *out = blink::WebURLRequest::kRequestContextTrack;
      return true;
    case RequestContextType::VIDEO:
      *out = blink::WebURLRequest::kRequestContextVideo;
      return true;
    case RequestContextType::WORKER:
      *out = blink::WebURLRequest::kRequestContextWorker;
      return true;
    case RequestContextType::XML_HTTP_REQUEST:
      *out = blink::WebURLRequest::kRequestContextXMLHttpRequest;
      return true;
    case RequestContextType::XSLT:
      *out = blink::WebURLRequest::kRequestContextXSLT;
      return true;
  }

  return false;
}

// static
blink::KURL StructTraits<blink::mojom::FetchAPIRequestDataView,
                         blink::WebServiceWorkerRequest>::
    url(const blink::WebServiceWorkerRequest& request) {
  return request.Url();
}

// static
WTF::String StructTraits<blink::mojom::FetchAPIRequestDataView,
                         blink::WebServiceWorkerRequest>::
    method(const blink::WebServiceWorkerRequest& request) {
  return request.Method();
}

// static
WTF::HashMap<WTF::String, WTF::String>
StructTraits<blink::mojom::FetchAPIRequestDataView,
             blink::WebServiceWorkerRequest>::
    headers(const blink::WebServiceWorkerRequest& request) {
  WTF::HashMap<WTF::String, WTF::String> header_map;
  for (const auto& pair : request.Headers())
    header_map.insert(pair.key, pair.value);
  return header_map;
}

// static
const blink::Referrer& StructTraits<blink::mojom::FetchAPIRequestDataView,
                                    blink::WebServiceWorkerRequest>::
    referrer(const blink::WebServiceWorkerRequest& request) {
  return request.GetReferrer();
}

// static
scoped_refptr<blink::BlobDataHandle> StructTraits<
    blink::mojom::FetchAPIRequestDataView,
    blink::WebServiceWorkerRequest>::blob(const blink::WebServiceWorkerRequest&
                                              request) {
  return request.GetBlobDataHandle();
}

// static
WTF::String StructTraits<blink::mojom::FetchAPIRequestDataView,
                         blink::WebServiceWorkerRequest>::
    integrity(const blink::WebServiceWorkerRequest& request) {
  return request.Integrity();
}

// static
WTF::String StructTraits<blink::mojom::FetchAPIRequestDataView,
                         blink::WebServiceWorkerRequest>::
    client_id(const blink::WebServiceWorkerRequest& request) {
  return request.ClientId();
}

// static
bool StructTraits<blink::mojom::FetchAPIRequestDataView,
                  blink::WebServiceWorkerRequest>::
    Read(blink::mojom::FetchAPIRequestDataView data,
         blink::WebServiceWorkerRequest* out) {
  network::mojom::FetchRequestMode mode;
  blink::WebURLRequest::RequestContext requestContext;
  network::mojom::RequestContextFrameType frameType;
  blink::KURL url;
  WTF::String method;
  WTF::HashMap<WTF::String, WTF::String> headers;
  scoped_refptr<blink::BlobDataHandle> blob;
  blink::Referrer referrer;
  network::mojom::FetchCredentialsMode credentialsMode;
  network::mojom::FetchRedirectMode redirectMode;
  WTF::String integrity;
  WTF::String clientId;

  if (!data.ReadMode(&mode) || !data.ReadRequestContextType(&requestContext) ||
      !data.ReadFrameType(&frameType) || !data.ReadUrl(&url) ||
      !data.ReadMethod(&method) || !data.ReadHeaders(&headers) ||
      !data.ReadBlob(&blob) || !data.ReadReferrer(&referrer) ||
      !data.ReadCredentialsMode(&credentialsMode) ||
      !data.ReadRedirectMode(&redirectMode) || !data.ReadClientId(&clientId) ||
      !data.ReadIntegrity(&integrity)) {
    return false;
  }

  out->SetMode(mode);
  out->SetIsMainResourceLoad(data.is_main_resource_load());
  out->SetRequestContext(requestContext);
  out->SetFrameType(frameType);
  out->SetURL(url);
  out->SetMethod(method);
  for (const auto& pair : headers)
    out->SetHeader(pair.key, pair.value);
  out->SetBlobDataHandle(blob);
  out->SetReferrer(referrer.referrer, static_cast<blink::WebReferrerPolicy>(
                                          referrer.referrer_policy));
  out->SetCredentialsMode(credentialsMode);
  out->SetCacheMode(data.cache_mode());
  out->SetRedirectMode(redirectMode);
  out->SetIntegrity(integrity);
  out->SetKeepalive(data.keepalive());
  out->SetClientId(clientId);
  out->SetIsReload(data.is_reload());
  return true;
}

}  // namespace mojo
