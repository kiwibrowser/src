/*
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_THREADABLE_LOADER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_THREADABLE_LOADER_H_

#include <memory>
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/raw_resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class Document;
class KURL;
class ResourceRequest;
class SecurityOrigin;
class ThreadableLoaderClient;
class ThreadableLoadingContext;

// TODO(horo): We are using this class not only in documents, but also in
// workers. We should change the name to ThreadableLoaderImpl.
class CORE_EXPORT DocumentThreadableLoader final : public ThreadableLoader,
                                                   private RawResourceClient {
  USING_GARBAGE_COLLECTED_MIXIN(DocumentThreadableLoader);

 public:
  static void LoadResourceSynchronously(ThreadableLoadingContext&,
                                        const ResourceRequest&,
                                        ThreadableLoaderClient&,
                                        const ThreadableLoaderOptions&,
                                        const ResourceLoaderOptions&);

  // Exposed for testing. Code outside this class should not call this function.
  static std::unique_ptr<ResourceRequest>
  CreateAccessControlPreflightRequestForTesting(const ResourceRequest&);

  static DocumentThreadableLoader* Create(ThreadableLoadingContext&,
                                          ThreadableLoaderClient*,
                                          const ThreadableLoaderOptions&,
                                          const ResourceLoaderOptions&);
  ~DocumentThreadableLoader() override;

  void Start(const ResourceRequest&) override;

  void OverrideTimeout(unsigned long timeout) override;

  void Cancel() override;
  void Detach() override;
  void SetDefersLoading(bool);

  // Exposed for thread-correctness DCHECKs in WorkerThreadableLoader.
  ExecutionContext* GetExecutionContext() const;

  void Trace(blink::Visitor*) override;

 private:
  class DetachedClient;
  enum BlockingBehavior { kLoadSynchronously, kLoadAsynchronously };

  static std::unique_ptr<ResourceRequest> CreateAccessControlPreflightRequest(
      const ResourceRequest&,
      const SecurityOrigin*);

  DocumentThreadableLoader(ThreadableLoadingContext&,
                           ThreadableLoaderClient*,
                           BlockingBehavior,
                           const ThreadableLoaderOptions&,
                           const ResourceLoaderOptions&);

  void Clear();

  // ResourceClient
  void NotifyFinished(Resource*) override;

  String DebugName() const override { return "DocumentThreadableLoader"; }

  // RawResourceClient
  void DataSent(Resource*,
                unsigned long long bytes_sent,
                unsigned long long total_bytes_to_be_sent) override;
  void ResponseReceived(Resource*,
                        const ResourceResponse&,
                        std::unique_ptr<WebDataConsumerHandle>) override;
  void SetSerializedCachedMetadata(Resource*, const char*, size_t) override;
  void DataReceived(Resource*, const char* data, size_t data_length) override;
  bool RedirectReceived(Resource*,
                        const ResourceRequest&,
                        const ResourceResponse&) override;
  void RedirectBlocked() override;
  void DataDownloaded(Resource*, int) override;
  void DidReceiveResourceTiming(Resource*, const ResourceTimingInfo&) override;
  void DidDownloadToBlob(Resource*, scoped_refptr<BlobDataHandle>) override;

  // Notify Inspector and log to console about resource response. Use this
  // method if response is not going to be finished normally.
  void ReportResponseReceived(unsigned long identifier,
                              const ResourceResponse&);

  // Methods containing code to handle resource fetch results which are common
  // to both sync and async mode.
  //
  // The FetchCredentialsMode argument must be the request's credentials mode.
  // It's used for CORS check.
  void HandleResponse(unsigned long identifier,
                      network::mojom::FetchRequestMode,
                      network::mojom::FetchCredentialsMode,
                      const ResourceResponse&,
                      std::unique_ptr<WebDataConsumerHandle>);
  void HandleReceivedData(const char* data, size_t data_length);
  void HandleSuccessfulFinish(unsigned long identifier);

  void DidTimeout(TimerBase*);
  // Calls the appropriate loading method according to policy and data about
  // origin. Only for handling the initial load (including fallback after
  // consulting ServiceWorker).
  void DispatchInitialRequest(ResourceRequest&);
  void MakeCrossOriginAccessRequest(const ResourceRequest&);

  // Loads m_fallbackRequestForServiceWorker.
  void LoadFallbackRequestForServiceWorker();
  // Issues a CORS preflight.
  void LoadPreflightRequest(const ResourceRequest&,
                            const ResourceLoaderOptions&);
  // Loads actual_request_.
  void LoadActualRequest();
  // Clears actual_request_ and reports access control check failure to
  // m_client.
  void HandlePreflightFailure(const KURL&, const String& error_description);
  // Investigates the response for the preflight request. If successful,
  // the actual request will be made later in handleSuccessfulFinish().
  void HandlePreflightResponse(const ResourceResponse&);

  void DispatchDidFailAccessControlCheck(const ResourceError&);
  void DispatchDidFail(const ResourceError&);

  void LoadRequestAsync(const ResourceRequest&, ResourceLoaderOptions);
  void LoadRequestSync(const ResourceRequest&, ResourceLoaderOptions);

  void PrepareCrossOriginRequest(ResourceRequest&) const;

  // This method modifies the ResourceRequest by calling
  // SetAllowStoredCredentials() on it based on same-origin-ness and the
  // credentials mode.
  //
  // This method configures the ResourceLoaderOptions so that the underlying
  // ResourceFetcher doesn't perform some part of the CORS logic since this
  // class performs it by itself.
  void LoadRequest(ResourceRequest&, ResourceLoaderOptions);
  bool IsAllowedRedirect(network::mojom::FetchRequestMode, const KURL&) const;

  const SecurityOrigin* GetSecurityOrigin() const;

  // Returns null if the loader is not associated with Document.
  // TODO(kinuko): Remove dependency to document.
  Document* GetDocument() const;

  ThreadableLoaderClient* client_;
  Member<ThreadableLoadingContext> loading_context_;

  const ThreadableLoaderOptions options_;
  // Some items may be overridden by m_forceDoNotAllowStoredCredentials and
  // m_securityOrigin. In such a case, build a ResourceLoaderOptions with
  // up-to-date values from them and this variable, and use it.
  const ResourceLoaderOptions resource_loader_options_;

  // True when feature OutOfBlinkCORS is enabled (https://crbug.com/736308).
  bool out_of_blink_cors_;

  // Corresponds to the CORS flag in the Fetch spec.
  bool cors_flag_;
  scoped_refptr<const SecurityOrigin> security_origin_;

  // Set to true when the response data is given to a data consumer handle.
  bool is_using_data_consumer_handle_;

  const bool async_;

  // Holds the original request context (used for sanity checks).
  WebURLRequest::RequestContext request_context_;

  // Saved so that we can use the original value for the modes in
  // ResponseReceived() where |resource| might be a reused one (e.g. preloaded
  // resource) which can have different modes.
  network::mojom::FetchRequestMode fetch_request_mode_;
  network::mojom::FetchCredentialsMode fetch_credentials_mode_;

  // Holds the original request for fallback in case the Service Worker
  // does not respond.
  ResourceRequest fallback_request_for_service_worker_;

  // Holds the original request and options for it during preflight request
  // handling phase.
  ResourceRequest actual_request_;
  ResourceLoaderOptions actual_options_;

  // stores request headers in case of a cross-origin redirect.
  HTTPHeaderMap request_headers_;

  TaskRunnerTimer<DocumentThreadableLoader> timeout_timer_;
  double request_started_seconds_;  // Time an asynchronous fetch request is
                                    // started

  // Max number of times that this DocumentThreadableLoader can follow
  // cross-origin redirects. This is used to limit the number of redirects. But
  // this value is not the max number of total redirects allowed, because
  // same-origin redirects are not counted here.
  int cors_redirect_limit_;

  network::mojom::FetchRedirectMode redirect_mode_;

  // Holds the referrer after a redirect response was received. This referrer is
  // used to populate the HTTP Referer header when following the redirect.
  bool override_referrer_;
  Referrer referrer_after_redirect_;

  RawResourceClientStateChecker checker_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_DOCUMENT_THREADABLE_LOADER_H_
