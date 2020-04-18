// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_FETCH_RESPONSE_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_FETCH_RESPONSE_DATA_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-blink.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_response.mojom-blink.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/public/platform/web_cors.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class BodyStreamBuffer;
class FetchHeaderList;
class ScriptState;
class WebServiceWorkerResponse;

class CORE_EXPORT FetchResponseData final
    : public GarbageCollectedFinalized<FetchResponseData> {
 public:
  // "A response can have an associated termination reason which is one of
  // end-user abort, fatal, and timeout."
  enum TerminationReason {
    kEndUserAbortTermination,
    kFatalTermination,
    kTimeoutTermination
  };

  static FetchResponseData* Create();
  static FetchResponseData* CreateNetworkErrorResponse();
  static FetchResponseData* CreateWithBuffer(BodyStreamBuffer*);

  FetchResponseData* CreateBasicFilteredResponse() const;
  FetchResponseData* CreateCORSFilteredResponse(
      const WebHTTPHeaderSet& exposed_headers) const;
  FetchResponseData* CreateOpaqueFilteredResponse() const;
  FetchResponseData* CreateOpaqueRedirectFilteredResponse() const;

  FetchResponseData* InternalResponse() { return internal_response_; }
  const FetchResponseData* InternalResponse() const {
    return internal_response_;
  }

  FetchResponseData* Clone(ScriptState*);

  network::mojom::FetchResponseType GetType() const { return type_; }
  const KURL* Url() const;
  unsigned short Status() const { return status_; }
  AtomicString StatusMessage() const { return status_message_; }
  FetchHeaderList* HeaderList() const { return header_list_.Get(); }
  BodyStreamBuffer* Buffer() const { return buffer_; }
  String MimeType() const;
  // Returns the BodyStreamBuffer of |m_internalResponse| if any. Otherwise,
  // returns |m_buffer|.
  BodyStreamBuffer* InternalBuffer() const;
  String InternalMIMEType() const;
  Time ResponseTime() const { return response_time_; }
  String CacheStorageCacheName() const { return cache_storage_cache_name_; }
  const WebHTTPHeaderSet& CorsExposedHeaderNames() const {
    return cors_exposed_header_names_;
  }

  void SetURLList(const Vector<KURL>&);
  const Vector<KURL>& UrlList() const { return url_list_; }
  const Vector<KURL>& InternalURLList() const;

  void SetStatus(unsigned short status) { status_ = status; }
  void SetStatusMessage(AtomicString status_message) {
    status_message_ = status_message;
  }
  void SetMIMEType(const String& type) { mime_type_ = type; }
  void SetResponseTime(Time response_time) { response_time_ = response_time; }
  void SetCacheStorageCacheName(const String& cache_storage_cache_name) {
    cache_storage_cache_name_ = cache_storage_cache_name;
  }
  void SetCorsExposedHeaderNames(const WebHTTPHeaderSet& header_names) {
    cors_exposed_header_names_ = header_names;
  }

  // If the type is Default, replaces |m_buffer|.
  // If the type is Basic or CORS, replaces |m_buffer| and
  // |m_internalResponse->m_buffer|.
  // If the type is Error or Opaque, does nothing.
  // Call Response::refreshBody after calling this function.
  void ReplaceBodyStreamBuffer(BodyStreamBuffer*);

  // Does not call response.setBlobDataHandle().
  void PopulateWebServiceWorkerResponse(
      WebServiceWorkerResponse& /* response */);
  mojom::blink::FetchAPIResponsePtr PopulateFetchAPIResponse();

  void Trace(blink::Visitor*);

 private:
  FetchResponseData(network::mojom::FetchResponseType,
                    unsigned short,
                    AtomicString);

  network::mojom::FetchResponseType type_;
  std::unique_ptr<TerminationReason> termination_reason_;
  Vector<KURL> url_list_;
  unsigned short status_;
  AtomicString status_message_;
  Member<FetchHeaderList> header_list_;
  Member<FetchResponseData> internal_response_;
  Member<BodyStreamBuffer> buffer_;
  String mime_type_;
  Time response_time_;
  String cache_storage_cache_name_;
  WebHTTPHeaderSet cors_exposed_header_names_;

  DISALLOW_COPY_AND_ASSIGN(FetchResponseData);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_FETCH_RESPONSE_DATA_H_
