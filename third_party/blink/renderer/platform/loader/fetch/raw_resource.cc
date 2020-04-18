/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/loader/fetch/raw_resource.h"

#include <memory>
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-shared.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_thread.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_client_walker.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/source_keyed_cached_metadata_handler.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"

namespace blink {

RawResource* RawResource::FetchSynchronously(FetchParameters& params,
                                             ResourceFetcher* fetcher) {
  params.MakeSynchronous();
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kRaw), nullptr));
}

RawResource* RawResource::FetchImport(FetchParameters& params,
                                      ResourceFetcher* fetcher,
                                      RawResourceClient* client) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  params.SetRequestContext(WebURLRequest::kRequestContextImport);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kImportResource), client));
}

RawResource* RawResource::Fetch(FetchParameters& params,
                                ResourceFetcher* fetcher,
                                RawResourceClient* client) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  DCHECK_NE(params.GetResourceRequest().GetRequestContext(),
            WebURLRequest::kRequestContextUnspecified);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kRaw), client));
}

RawResource* RawResource::FetchMainResource(
    FetchParameters& params,
    ResourceFetcher* fetcher,
    RawResourceClient* client,
    const SubstituteData& substitute_data) {
  DCHECK_NE(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  DCHECK(params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextForm ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextFrame ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextHyperlink ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextIframe ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextInternal ||
         params.GetResourceRequest().GetRequestContext() ==
             WebURLRequest::kRequestContextLocation);

  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kMainResource), client,
      substitute_data));
}

RawResource* RawResource::FetchMedia(FetchParameters& params,
                                     ResourceFetcher* fetcher,
                                     RawResourceClient* client) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  auto context = params.GetResourceRequest().GetRequestContext();
  DCHECK(context == WebURLRequest::kRequestContextAudio ||
         context == WebURLRequest::kRequestContextVideo);
  Resource::Type type = (context == WebURLRequest::kRequestContextAudio)
                            ? Resource::kAudio
                            : Resource::kVideo;
  return ToRawResource(
      fetcher->RequestResource(params, RawResourceFactory(type), client));
}

RawResource* RawResource::FetchTextTrack(FetchParameters& params,
                                         ResourceFetcher* fetcher,
                                         RawResourceClient* client) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  params.SetRequestContext(WebURLRequest::kRequestContextTrack);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kTextTrack), client));
}

RawResource* RawResource::FetchManifest(FetchParameters& params,
                                        ResourceFetcher* fetcher,
                                        RawResourceClient* client) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  DCHECK_EQ(params.GetResourceRequest().GetRequestContext(),
            WebURLRequest::kRequestContextManifest);
  return ToRawResource(fetcher->RequestResource(
      params, RawResourceFactory(Resource::kManifest), client));
}

RawResource::RawResource(const ResourceRequest& resource_request,
                         Type type,
                         const ResourceLoaderOptions& options)
    : Resource(resource_request, type, options) {}

void RawResource::AppendData(const char* data, size_t length) {
  if (data_pipe_writer_) {
    DCHECK_EQ(kDoNotBufferData, GetDataBufferingPolicy());
    data_pipe_writer_->Write(data, length);
  } else {
    Resource::AppendData(data, length);
  }
}

void RawResource::DidAddClient(ResourceClient* c) {
  // CHECK()/RevalidationStartForbiddenScope are for
  // https://crbug.com/640960#c24.
  CHECK(!IsCacheValidator());
  if (!HasClient(c))
    return;
  DCHECK(RawResourceClient::IsExpectedType(c));
  RevalidationStartForbiddenScope revalidation_start_forbidden_scope(this);
  RawResourceClient* client = static_cast<RawResourceClient*>(c);
  for (const auto& redirect : RedirectChain()) {
    ResourceRequest request(redirect.request_);
    client->RedirectReceived(this, request, redirect.redirect_response_);
    if (!HasClient(c))
      return;
  }

  if (!GetResponse().IsNull()) {
    client->ResponseReceived(this, GetResponse(),
                             std::move(data_consumer_handle_));
  }
  if (!HasClient(c))
    return;
  Resource::DidAddClient(client);
}

bool RawResource::WillFollowRedirect(
    const ResourceRequest& new_request,
    const ResourceResponse& redirect_response) {
  bool follow = Resource::WillFollowRedirect(new_request, redirect_response);
  // The base class method takes a const reference of a ResourceRequest and
  // returns bool just for allowing RawResource to reject redirect. It must
  // always return true.
  DCHECK(follow);

  DCHECK(!redirect_response.IsNull());
  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next()) {
    if (!c->RedirectReceived(this, new_request, redirect_response))
      follow = false;
  }

  return follow;
}

void RawResource::WillNotFollowRedirect() {
  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next())
    c->RedirectBlocked();
}

SourceKeyedCachedMetadataHandler* RawResource::CacheHandler() {
  return static_cast<SourceKeyedCachedMetadataHandler*>(
      Resource::CacheHandler());
}

void RawResource::ResponseReceived(
    const ResourceResponse& response,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  if (response.WasFallbackRequiredByServiceWorker()) {
    // The ServiceWorker asked us to re-fetch the request. This resource must
    // not be reused.
    // Note: This logic is needed here because DocumentThreadableLoader handles
    // CORS independently from ResourceLoader. Fix it.
    if (IsMainThread())
      GetMemoryCache()->Remove(this);
  }

  Resource::ResponseReceived(response, nullptr);

  DCHECK(!handle || !data_consumer_handle_);
  if (!handle && Clients().size() > 0)
    handle = std::move(data_consumer_handle_);
  ResourceClientWalker<RawResourceClient> w(Clients());
  DCHECK(Clients().size() <= 1 || !handle);
  while (RawResourceClient* c = w.Next()) {
    // |handle| is cleared when passed, but it's not a problem because |handle|
    // is null when there are two or more clients, as asserted.
    c->ResponseReceived(this, this->GetResponse(), std::move(handle));
  }
}

CachedMetadataHandler* RawResource::CreateCachedMetadataHandler(
    std::unique_ptr<CachedMetadataSender> send_callback) {
  return new SourceKeyedCachedMetadataHandler(Encoding(),
                                              std::move(send_callback));
}

void RawResource::SetSerializedCachedMetadata(const char* data, size_t size) {
  Resource::SetSerializedCachedMetadata(data, size);

  SourceKeyedCachedMetadataHandler* cache_handler = CacheHandler();
  if (cache_handler) {
    cache_handler->SetSerializedCachedMetadata(data, size);
  }

  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next())
    c->SetSerializedCachedMetadata(this, data, size);
}

void RawResource::DidSendData(unsigned long long bytes_sent,
                              unsigned long long total_bytes_to_be_sent) {
  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next())
    c->DataSent(this, bytes_sent, total_bytes_to_be_sent);
}

void RawResource::DidDownloadData(int data_length) {
  downloaded_file_length_ =
      (downloaded_file_length_ ? *downloaded_file_length_ : 0) + data_length;
  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next())
    c->DataDownloaded(this, data_length);
}

void RawResource::DidDownloadToBlob(scoped_refptr<BlobDataHandle> blob) {
  downloaded_blob_ = blob;
  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next())
    c->DidDownloadToBlob(this, blob);
}

void RawResource::ReportResourceTimingToClients(
    const ResourceTimingInfo& info) {
  ResourceClientWalker<RawResourceClient> w(Clients());
  while (RawResourceClient* c = w.Next())
    c->DidReceiveResourceTiming(this, info);
}

bool RawResource::MatchPreload(const FetchParameters& params,
                               base::SingleThreadTaskRunner* task_runner) {
  if (!Resource::MatchPreload(params, task_runner))
    return false;

  // This is needed to call Platform::Current() below. Remove this branch
  // when the calls are removed.
  if (!IsMainThread())
    return false;

  if (!params.GetResourceRequest().UseStreamOnResponse())
    return true;

  if (ErrorOccurred())
    return true;

  // A preloaded resource is not for streaming.
  DCHECK(!GetResourceRequest().UseStreamOnResponse());
  DCHECK_EQ(GetDataBufferingPolicy(), kBufferData);

  // Preloading for raw resources are not cached.
  DCHECK(!IsMainThread() || !GetMemoryCache()->Contains(this));

  constexpr auto kCapacity = 32 * 1024;
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = kCapacity;

  MojoResult result = mojo::CreateDataPipe(&options, &producer, &consumer);
  if (result != MOJO_RESULT_OK)
    return false;

  data_consumer_handle_ =
      Platform::Current()->CreateDataConsumerHandle(std::move(consumer));
  data_pipe_writer_ = std::make_unique<BufferingDataPipeWriter>(
      std::move(producer), task_runner);

  if (Data()) {
    Data()->ForEachSegment(
        [this](const char* segment, size_t size, size_t offset) -> bool {
          return data_pipe_writer_->Write(segment, size);
        });
  }
  SetDataBufferingPolicy(kDoNotBufferData);

  if (IsLoaded())
    data_pipe_writer_->Finish();
  return true;
}

void RawResource::NotifyFinished() {
  if (data_pipe_writer_)
    data_pipe_writer_->Finish();
  Resource::NotifyFinished();
}

static bool ShouldIgnoreHeaderForCacheReuse(AtomicString header_name) {
  // FIXME: This list of headers that don't affect cache policy almost certainly
  // isn't complete.
  DEFINE_STATIC_LOCAL(
      HashSet<AtomicString>, headers,
      ({"Cache-Control", "If-Modified-Since", "If-None-Match", "Origin",
        "Pragma", "Purpose", "Referer", "User-Agent",
        HTTPNames::X_DevTools_Emulate_Network_Conditions_Client_Id}));
  return headers.Contains(header_name);
}

static bool IsCacheableHTTPMethod(const AtomicString& method) {
  // Per http://www.w3.org/Protocols/rfc2616/rfc2616-sec13.html#sec13.10,
  // these methods always invalidate the cache entry.
  return method != HTTPNames::POST && method != HTTPNames::PUT &&
         method != "DELETE";
}

bool RawResource::CanReuse(
    const FetchParameters& new_fetch_parameters,
    scoped_refptr<const SecurityOrigin> new_source_origin) const {
  const ResourceRequest& new_request =
      new_fetch_parameters.GetResourceRequest();

  if (GetDataBufferingPolicy() == kDoNotBufferData)
    return false;

  if (!IsCacheableHTTPMethod(GetResourceRequest().HttpMethod()))
    return false;
  if (GetResourceRequest().HttpMethod() != new_request.HttpMethod())
    return false;

  if (GetResourceRequest().HttpBody() != new_request.HttpBody())
    return false;

  if (GetResourceRequest().AllowStoredCredentials() !=
      new_request.AllowStoredCredentials())
    return false;

  // Ensure most headers match the existing headers before continuing. Note that
  // the list of ignored headers includes some headers explicitly related to
  // caching. A more detailed check of caching policy will be performed later,
  // this is simply a list of headers that we might permit to be different and
  // still reuse the existing Resource.
  const HTTPHeaderMap& new_headers = new_request.HttpHeaderFields();
  const HTTPHeaderMap& old_headers = GetResourceRequest().HttpHeaderFields();

  for (const auto& header : new_headers) {
    AtomicString header_name = header.key;
    if (!ShouldIgnoreHeaderForCacheReuse(header_name) &&
        header.value != old_headers.Get(header_name))
      return false;
  }

  for (const auto& header : old_headers) {
    AtomicString header_name = header.key;
    if (!ShouldIgnoreHeaderForCacheReuse(header_name) &&
        header.value != new_headers.Get(header_name))
      return false;
  }

  return Resource::CanReuse(new_fetch_parameters, std::move(new_source_origin));
}

RawResourceClientStateChecker::RawResourceClientStateChecker()
    : state_(kNotAddedAsClient) {}

RawResourceClientStateChecker::~RawResourceClientStateChecker() = default;

NEVER_INLINE void RawResourceClientStateChecker::WillAddClient() {
  SECURITY_CHECK(state_ == kNotAddedAsClient);
  state_ = kStarted;
}

NEVER_INLINE void RawResourceClientStateChecker::WillRemoveClient() {
  SECURITY_CHECK(state_ != kNotAddedAsClient);
  state_ = kNotAddedAsClient;
}

NEVER_INLINE void RawResourceClientStateChecker::RedirectReceived() {
  SECURITY_CHECK(state_ == kStarted);
}

NEVER_INLINE void RawResourceClientStateChecker::RedirectBlocked() {
  SECURITY_CHECK(state_ == kStarted);
  state_ = kRedirectBlocked;
}

NEVER_INLINE void RawResourceClientStateChecker::DataSent() {
  SECURITY_CHECK(state_ == kStarted);
}

NEVER_INLINE void RawResourceClientStateChecker::ResponseReceived() {
  SECURITY_CHECK(state_ == kStarted);
  state_ = kResponseReceived;
}

NEVER_INLINE void RawResourceClientStateChecker::SetSerializedCachedMetadata() {
  SECURITY_CHECK(state_ == kResponseReceived);
  state_ = kSetSerializedCachedMetadata;
}

NEVER_INLINE void RawResourceClientStateChecker::DataReceived() {
  SECURITY_CHECK(state_ == kResponseReceived ||
                 state_ == kSetSerializedCachedMetadata ||
                 state_ == kDataReceived);
  state_ = kDataReceived;
}

NEVER_INLINE void RawResourceClientStateChecker::DataDownloaded() {
  SECURITY_CHECK(state_ == kResponseReceived ||
                 state_ == kSetSerializedCachedMetadata ||
                 state_ == kDataDownloaded);
  state_ = kDataDownloaded;
}

NEVER_INLINE void RawResourceClientStateChecker::DidDownloadToBlob() {
  SECURITY_CHECK(state_ == kResponseReceived ||
                 state_ == kSetSerializedCachedMetadata ||
                 state_ == kDataDownloaded);
  state_ = kDidDownloadToBlob;
}

NEVER_INLINE void RawResourceClientStateChecker::NotifyFinished(
    Resource* resource) {
  SECURITY_CHECK(state_ != kNotAddedAsClient);
  SECURITY_CHECK(state_ != kNotifyFinished);
  SECURITY_CHECK(resource->ErrorOccurred() ||
                 (state_ == kResponseReceived ||
                  state_ == kSetSerializedCachedMetadata ||
                  state_ == kDataReceived || state_ == kDataDownloaded ||
                  state_ == kDidDownloadToBlob));
  state_ = kNotifyFinished;
}

}  // namespace blink
