/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/loader/frame_fetch_context.h"

#include <algorithm>
#include <memory>

#include "base/feature_list.h"
#include "base/optional.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/public/common/client_hints/client_hints.h"
#include "third_party/blink/public/common/device_memory/approximated_device_memory.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_network_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_application_cache_host.h"
#include "third_party/blink/public/platform/web_effective_connection_type.h"
#include "third_party/blink/public/platform/web_insecure_request_policy.h"
#include "third_party/blink/public/platform/websocket_handshake_throttle.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/fileapi/public_url_manager.h"
#include "third_party/blink/renderer/core/frame/content_settings_client.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/imports/html_imports_controller.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/loader/appcache/application_cache_host.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/idleness_detector.h"
#include "third_party/blink/renderer/core/loader/interactive_detector.h"
#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"
#include "third_party/blink/renderer/core/loader/network_hints_interface.h"
#include "third_party/blink/renderer/core/loader/ping_loader.h"
#include "third_party/blink/renderer/core/loader/private/frame_client_hints_preferences_context.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/core/loader/subresource_filter.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/first_meaningful_paint_detector.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image_chrome_client.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/performance.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_activity_logger.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_request.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/loader/fetch/client_hints_preferences.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_priority.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loading_log.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_info.h"
#include "third_party/blink/renderer/platform/loader/fetch/unique_identifier.h"
#include "third_party/blink/renderer/platform/mhtml/mhtml_archive.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/network_state_notifier.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace {

// If kAllowClientHintsToThirdParty is enabled, then device-memory,
// resource-width and viewport-width client hints can be sent to third-party
// origins if the first-party has opted in to receiving client hints.
const base::Feature kAllowClientHintsToThirdParty{
    "AllowClientHintsToThirdParty", base::FEATURE_DISABLED_BY_DEFAULT};

enum class RequestMethod { kIsPost, kIsNotPost };
enum class RequestType { kIsConditional, kIsNotConditional };
enum class ResourceType { kIsMainResource, kIsNotMainResource };

void MaybeRecordCTPolicyComplianceUseCounter(
    LocalFrame* frame,
    Resource::Type resource_type,
    ResourceResponse::CTPolicyCompliance compliance) {
  if (compliance != ResourceResponse::kCTPolicyDoesNotComply)
    return;
  // Exclude main-frame navigation requests; those are tracked elsewhere.
  if (!frame->Tree().Parent() && resource_type == Resource::kMainResource)
    return;
  UseCounter::Count(
      frame,
      frame->Tree().Parent()
          ? WebFeature::kCertificateTransparencyNonCompliantResourceInSubframe
          : WebFeature::
                kCertificateTransparencyNonCompliantSubresourceInMainFrame);
}

// Determines FetchCacheMode for a main resource, or FetchCacheMode that is
// corresponding to FrameLoadType.
// TODO(toyoshim): Probably, we should split FrameLoadType to FetchCacheMode
// conversion logic into a separate function.
mojom::FetchCacheMode DetermineCacheMode(RequestMethod method,
                                         RequestType request_type,
                                         ResourceType resource_type,
                                         FrameLoadType load_type) {
  switch (load_type) {
    case kFrameLoadTypeStandard:
    case kFrameLoadTypeReplaceCurrentItem:
    case kFrameLoadTypeInitialInChildFrame:
      return (request_type == RequestType::kIsConditional ||
              method == RequestMethod::kIsPost)
                 ? mojom::FetchCacheMode::kValidateCache
                 : mojom::FetchCacheMode::kDefault;
    case kFrameLoadTypeBackForward:
    case kFrameLoadTypeInitialHistoryLoad:
      // Mutates the policy for POST requests to avoid form resubmission.
      return method == RequestMethod::kIsPost
                 ? mojom::FetchCacheMode::kOnlyIfCached
                 : mojom::FetchCacheMode::kForceCache;
    case kFrameLoadTypeReload:
      return resource_type == ResourceType::kIsMainResource
                 ? mojom::FetchCacheMode::kValidateCache
                 : mojom::FetchCacheMode::kDefault;
    case kFrameLoadTypeReloadBypassingCache:
      return mojom::FetchCacheMode::kBypassCache;
  }
  NOTREACHED();
  return mojom::FetchCacheMode::kDefault;
}

// Determines FetchCacheMode for |frame|. This FetchCacheMode should be a base
// policy to consider one of each resource belonging to the frame, and should
// not count resource specific conditions in.
// TODO(toyoshim): Remove |resourceType| to realize the design described above.
// See also comments in resourceRequestCachePolicy().
mojom::FetchCacheMode DetermineFrameCacheMode(Frame* frame,
                                              ResourceType resource_type) {
  if (!frame)
    return mojom::FetchCacheMode::kDefault;
  if (!frame->IsLocalFrame())
    return DetermineFrameCacheMode(frame->Tree().Parent(), resource_type);

  // Does not propagate cache policy for subresources after the load event.
  // TODO(toyoshim): We should be able to remove following parents' policy check
  // if each frame has a relevant FrameLoadType for reload and history
  // navigations.
  if (resource_type == ResourceType::kIsNotMainResource &&
      ToLocalFrame(frame)->GetDocument()->LoadEventFinished()) {
    return mojom::FetchCacheMode::kDefault;
  }

  // Respects BypassingCache rather than parent's policy.
  FrameLoadType load_type =
      ToLocalFrame(frame)->Loader().GetDocumentLoader()->LoadType();
  if (load_type == kFrameLoadTypeReloadBypassingCache)
    return mojom::FetchCacheMode::kBypassCache;

  // Respects parent's policy if it has a special one.
  mojom::FetchCacheMode parent_cache_mode =
      DetermineFrameCacheMode(frame->Tree().Parent(), resource_type);
  if (parent_cache_mode != mojom::FetchCacheMode::kDefault)
    return parent_cache_mode;

  // Otherwise, follows FrameLoadType. Use kIsNotPost, kIsNotConditional, and
  // kIsNotMainResource to obtain a representative policy for the frame.
  return DetermineCacheMode(RequestMethod::kIsNotPost,
                            RequestType::kIsNotConditional,
                            ResourceType::kIsNotMainResource, load_type);
}

}  // namespace

struct FrameFetchContext::FrozenState final
    : GarbageCollectedFinalized<FrozenState> {
  FrozenState(ReferrerPolicy referrer_policy,
              const String& outgoing_referrer,
              const KURL& url,
              scoped_refptr<const SecurityOrigin> security_origin,
              scoped_refptr<const SecurityOrigin> parent_security_origin,
              const base::Optional<mojom::IPAddressSpace>& address_space,
              const ContentSecurityPolicy* content_security_policy,
              KURL site_for_cookies,
              scoped_refptr<const SecurityOrigin> requestor_origin,
              const ClientHintsPreferences& client_hints_preferences,
              float device_pixel_ratio,
              const String& user_agent,
              bool is_main_frame,
              bool is_svg_image_chrome_client)
      : referrer_policy(referrer_policy),
        outgoing_referrer(outgoing_referrer),
        url(url),
        security_origin(std::move(security_origin)),
        parent_security_origin(std::move(parent_security_origin)),
        address_space(address_space),
        content_security_policy(content_security_policy),
        site_for_cookies(site_for_cookies),
        requestor_origin(requestor_origin),
        client_hints_preferences(client_hints_preferences),
        device_pixel_ratio(device_pixel_ratio),
        user_agent(user_agent),
        is_main_frame(is_main_frame),
        is_svg_image_chrome_client(is_svg_image_chrome_client) {}

  const ReferrerPolicy referrer_policy;
  const String outgoing_referrer;
  const KURL url;
  const scoped_refptr<const SecurityOrigin> security_origin;
  const scoped_refptr<const SecurityOrigin> parent_security_origin;
  const base::Optional<mojom::IPAddressSpace> address_space;
  const Member<const ContentSecurityPolicy> content_security_policy;
  const KURL site_for_cookies;
  const scoped_refptr<const SecurityOrigin> requestor_origin;
  const ClientHintsPreferences client_hints_preferences;
  const float device_pixel_ratio;
  const String user_agent;
  const bool is_main_frame;
  const bool is_svg_image_chrome_client;

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(content_security_policy);
  }
};

ResourceFetcher* FrameFetchContext::CreateFetcher(DocumentLoader* loader,
                                                  Document* document) {
  FrameFetchContext* context = new FrameFetchContext(loader, document);
  ResourceFetcher* fetcher = ResourceFetcher::Create(context);

  if (loader && context->GetSettings()->GetSavePreviousDocumentResources() !=
                    SavePreviousDocumentResources::kNever) {
    if (Document* previous_document = context->GetFrame()->GetDocument()) {
      if (previous_document->IsSecureTransitionTo(loader->Url())) {
        fetcher->HoldResourcesFromPreviousFetcher(
            previous_document->Loader()->Fetcher());
      }
    }
  }

  return fetcher;
}

FrameFetchContext::FrameFetchContext(DocumentLoader* loader, Document* document)
    : document_loader_(loader),
      document_(document),
      save_data_enabled_(GetNetworkStateNotifier().SaveDataEnabled() &&
                         !GetSettings()->GetDataSaverHoldbackWebApi()) {
  DCHECK(GetFrame());
}

void FrameFetchContext::ProvideDocumentToContext(FetchContext& context,
                                                 Document* document) {
  DCHECK(document);
  CHECK(context.IsFrameFetchContext());
  static_cast<FrameFetchContext&>(context).document_ = document;
}

FrameFetchContext::~FrameFetchContext() {
  document_loader_ = nullptr;
}

LocalFrame* FrameFetchContext::FrameOfImportsController() const {
  DCHECK(document_);
  DCHECK(!IsDetached());

  // It's guaranteed that imports_controller is not nullptr since:
  // - only ClearImportsController() clears it
  // - ClearImportsController() also calls ClearContext() on this
  //   FrameFetchContext() making IsDetached() return false
  HTMLImportsController* imports_controller = document_->ImportsController();
  DCHECK(imports_controller);

  // It's guaranteed that Master() is not yet Shutdown()-ed since when Master()
  // is Shutdown()-ed:
  // - Master()'s HTMLImportsController is disposed.
  // - All the HTMLImportLoader instances of the HTMLImportsController are
  //   disposed.
  // - ClearImportsController() is called on the Document of the
  //   HTMLImportLoader to detach this context which makes IsDetached() return
  //   true.
  // HTMLImportsController is created only when the master Document's
  // GetFrame() doesn't return nullptr, this is guaranteed to be not nullptr
  // here.
  LocalFrame* frame = imports_controller->Master()->GetFrame();
  DCHECK(frame);
  return frame;
}

scoped_refptr<base::SingleThreadTaskRunner>
FrameFetchContext::GetLoadingTaskRunner() {
  if (IsDetached())
    return FetchContext::GetLoadingTaskRunner();
  return GetFrame()->GetTaskRunner(TaskType::kNetworking);
}

FrameScheduler* FrameFetchContext::GetFrameScheduler() const {
  if (IsDetached())
    return nullptr;
  return GetFrame()->GetFrameScheduler();
}

KURL FrameFetchContext::GetSiteForCookies() const {
  if (IsDetached())
    return frozen_state_->site_for_cookies;

  // Use document_ for subresource or nested frame cases,
  // GetFrame()->GetDocument() otherwise.
  Document* document = document_ ? document_.Get() : GetFrame()->GetDocument();
  return document->SiteForCookies();
}

SubresourceFilter* FrameFetchContext::GetSubresourceFilter() const {
  if (IsDetached())
    return nullptr;
  DocumentLoader* document_loader = MasterDocumentLoader();
  return document_loader ? document_loader->GetSubresourceFilter() : nullptr;
}

LocalFrame* FrameFetchContext::GetFrame() const {
  DCHECK(!IsDetached());

  if (!document_loader_)
    return FrameOfImportsController();

  LocalFrame* frame = document_loader_->GetFrame();
  DCHECK(frame);
  return frame;
}

LocalFrameClient* FrameFetchContext::GetLocalFrameClient() const {
  return GetFrame()->Client();
}

void FrameFetchContext::AddAdditionalRequestHeaders(ResourceRequest& request,
                                                    FetchResourceType type) {
  BaseFetchContext::AddAdditionalRequestHeaders(request, type);

  // The remaining modifications are only necessary for HTTP and HTTPS.
  if (!request.Url().IsEmpty() && !request.Url().ProtocolIsInHTTPFamily())
    return;

  if (IsDetached())
    return;

  // Reload should reflect the current data saver setting.
  if (IsReloadLoadType(MasterDocumentLoader()->LoadType()))
    request.ClearHTTPHeaderField(HTTPNames::Save_Data);

  if (save_data_enabled_)
    request.SetHTTPHeaderField(HTTPNames::Save_Data, "on");

  if (GetLocalFrameClient()->GetPreviewsStateForFrame() &
      WebURLRequest::kNoScriptOn) {
    request.AddHTTPHeaderField(
        "Intervention",
        "<https://www.chromestatus.com/features/4775088607985664>; "
        "level=\"warning\"");
  }

  if (GetLocalFrameClient()->GetPreviewsStateForFrame() &
      WebURLRequest::kClientLoFiOn) {
    request.AddHTTPHeaderField(
        "Intervention",
        "<https://www.chromestatus.com/features/6072546726248448>; "
        "level=\"warning\"");
  }
}

// TODO(toyoshim, arthursonzogni): PlzNavigate doesn't use this function to set
// the ResourceRequest's cache policy. The cache policy determination needs to
// be factored out from FrameFetchContext and moved to the FrameLoader for
// instance.
mojom::FetchCacheMode FrameFetchContext::ResourceRequestCachePolicy(
    const ResourceRequest& request,
    Resource::Type type,
    FetchParameters::DeferOption defer) const {
  if (IsDetached())
    return mojom::FetchCacheMode::kDefault;

  DCHECK(GetFrame());
  if (type == Resource::kMainResource) {
    const auto cache_mode = DetermineCacheMode(
        request.HttpMethod() == HTTPNames::POST ? RequestMethod::kIsPost
                                                : RequestMethod::kIsNotPost,
        request.IsConditional() ? RequestType::kIsConditional
                                : RequestType::kIsNotConditional,
        ResourceType::kIsMainResource, MasterDocumentLoader()->LoadType());
    // Follows the parent frame's policy.
    // TODO(toyoshim): Probably, FrameLoadType for each frame should have a
    // right type for reload or history navigations, and should not need to
    // check parent's frame policy here. Once it has a right FrameLoadType,
    // we can remove Resource::Type argument from determineFrameCacheMode.
    // See also crbug.com/332602.
    if (cache_mode != mojom::FetchCacheMode::kDefault)
      return cache_mode;
    return DetermineFrameCacheMode(GetFrame()->Tree().Parent(),
                                   ResourceType::kIsMainResource);
  }

  const auto cache_mode =
      DetermineFrameCacheMode(GetFrame(), ResourceType::kIsNotMainResource);

  // TODO(toyoshim): Revisit to consider if this clause can be merged to
  // determineWebCachePolicy or determineFrameCacheMode.
  if (cache_mode == mojom::FetchCacheMode::kDefault &&
      request.IsConditional()) {
    return mojom::FetchCacheMode::kValidateCache;
  }
  return cache_mode;
}

inline DocumentLoader* FrameFetchContext::MasterDocumentLoader() const {
  DCHECK(!IsDetached());

  if (document_loader_)
    return document_loader_.Get();

  // GetDocumentLoader() here always returns a non-nullptr value that is the
  // DocumentLoader for |document_| because:
  // - A Document is created with a LocalFrame only after the
  //   DocumentLoader is committed
  // - When another DocumentLoader is committed, the FrameLoader
  //   Shutdown()-s |document_| making IsDetached() return false
  return FrameOfImportsController()->Loader().GetDocumentLoader();
}

void FrameFetchContext::DispatchDidChangeResourcePriority(
    unsigned long identifier,
    ResourceLoadPriority load_priority,
    int intra_priority_value) {
  if (IsDetached())
    return;
  TRACE_EVENT1("devtools.timeline", "ResourceChangePriority", "data",
               InspectorChangeResourcePriorityEvent::Data(
                   MasterDocumentLoader(), identifier, load_priority));
  probe::didChangeResourcePriority(GetFrame(), MasterDocumentLoader(),
                                   identifier, load_priority);
}

void FrameFetchContext::PrepareRequest(ResourceRequest& request,
                                       RedirectType redirect_type) {
  SetFirstPartyCookieAndRequestorOrigin(request);

  String user_agent = GetUserAgent();
  request.SetHTTPUserAgent(AtomicString(user_agent));

  if (IsDetached())
    return;
  GetLocalFrameClient()->DispatchWillSendRequest(request);

  // ServiceWorker hook ups.
  if (MasterDocumentLoader()->GetServiceWorkerNetworkProvider()) {
    WrappedResourceRequest webreq(request);
    MasterDocumentLoader()->GetServiceWorkerNetworkProvider()->WillSendRequest(
        webreq);
  }

  // If it's not for redirect, hook up ApplicationCache here too.
  if (redirect_type == FetchContext::RedirectType::kNotForRedirect &&
      document_loader_ && !document_loader_->Fetcher()->Archive() &&
      request.Url().IsValid()) {
    document_loader_->GetApplicationCacheHost()->WillStartLoading(request);
  }
}

void FrameFetchContext::DispatchWillSendRequest(
    unsigned long identifier,
    ResourceRequest& request,
    const ResourceResponse& redirect_response,
    Resource::Type resource_type,
    const FetchInitiatorInfo& initiator_info) {
  if (IsDetached())
    return;

  if (redirect_response.IsNull()) {
    // Progress doesn't care about redirects, only notify it when an
    // initial request is sent.
    GetFrame()->Loader().Progress().WillStartLoading(identifier,
                                                     request.Priority());
  }
  probe::willSendRequest(GetFrame()->GetDocument(), identifier,
                         MasterDocumentLoader(), request, redirect_response,
                         initiator_info, resource_type);
  if (IdlenessDetector* idleness_detector = GetFrame()->GetIdlenessDetector())
    idleness_detector->OnWillSendRequest(MasterDocumentLoader()->Fetcher());
  if (document_) {
    InteractiveDetector* interactive_detector(
        InteractiveDetector::From(*document_));
    if (interactive_detector) {
      interactive_detector->OnResourceLoadBegin(base::nullopt);
    }
  }
}

void FrameFetchContext::DispatchDidReceiveResponse(
    unsigned long identifier,
    const ResourceResponse& response,
    network::mojom::RequestContextFrameType frame_type,
    WebURLRequest::RequestContext request_context,
    Resource* resource,
    ResourceResponseType response_type) {
  if (IsDetached())
    return;

  MaybeRecordCTPolicyComplianceUseCounter(GetFrame(), resource->GetType(),
                                          response.GetCTPolicyCompliance());

  if (response_type == ResourceResponseType::kFromMemoryCache) {
    // Note: probe::willSendRequest needs to precede before this probe method.
    probe::markResourceAsCached(GetFrame(), MasterDocumentLoader(), identifier);
    if (response.IsNull())
      return;
  }

  MixedContentChecker::CheckMixedPrivatePublic(GetFrame(),
                                               response.RemoteIPAddress());
  LinkLoader::CanLoadResources resource_loading_policy =
      response_type == ResourceResponseType::kFromMemoryCache
          ? LinkLoader::kDoNotLoadResources
          : LinkLoader::kLoadResourcesAndPreconnect;
  if (document_loader_ &&
      document_loader_ == document_loader_->GetFrame()
                              ->Loader()
                              .GetProvisionalDocumentLoader()) {
    FrameClientHintsPreferencesContext hints_context(GetFrame());
    document_loader_->GetClientHintsPreferences()
        .UpdateFromAcceptClientHintsHeader(
            response.HttpHeaderField(HTTPNames::Accept_CH), response.Url(),
            &hints_context);

    // When response is received with a provisional docloader, the resource
    // haven't committed yet, and we cannot load resources, only preconnect.
    resource_loading_policy = LinkLoader::kDoNotLoadResources;
  }
  // Client hints preferences should be persisted only from responses that were
  // served by the same host as the host of the document-level origin.
  KURL frame_url = Url();
  if (frame_url == NullURL())
    frame_url = document_loader_->Url();

  // Check if |response| belongs to a resource in the main frame, and if belongs
  // to the same origin as frame top request.
  if (SecurityOrigin::AreSameSchemeHostPort(response.Url(), frame_url) &&
      GetFrame()->IsMainFrame()) {
    ParseAndPersistClientHints(response);
  }

  LinkLoader::LoadLinksFromHeader(
      response.HttpHeaderField(HTTPNames::Link), response.Url(), *GetFrame(),
      document_, NetworkHintsInterfaceImpl(), resource_loading_policy,
      LinkLoader::kLoadAll, nullptr);

  if (response.HasMajorCertificateErrors()) {
    MixedContentChecker::HandleCertificateError(GetFrame(), response,
                                                frame_type, request_context);
  }

  if (response.IsLegacySymantecCert()) {
    GetLocalFrameClient()->ReportLegacySymantecCert(response.Url(),
                                                    false /* did_fail */);
  }

  GetFrame()->Loader().Progress().IncrementProgress(identifier, response);
  GetLocalFrameClient()->DispatchDidReceiveResponse(response);
  DocumentLoader* document_loader = MasterDocumentLoader();
  probe::didReceiveResourceResponse(GetFrame()->GetDocument(), identifier,
                                    document_loader, response, resource);
  // It is essential that inspector gets resource response BEFORE console.
  GetFrame()->Console().ReportResourceResponseReceived(document_loader,
                                                       identifier, response);
}

void FrameFetchContext::DispatchDidReceiveData(unsigned long identifier,
                                               const char* data,
                                               int data_length) {
  if (IsDetached())
    return;

  GetFrame()->Loader().Progress().IncrementProgress(identifier, data_length);
  probe::didReceiveData(GetFrame()->GetDocument(), identifier,
                        MasterDocumentLoader(), data, data_length);
}

void FrameFetchContext::DispatchDidReceiveEncodedData(unsigned long identifier,
                                                      int encoded_data_length) {
  if (IsDetached())
    return;
  probe::didReceiveEncodedDataLength(GetFrame()->GetDocument(),
                                     MasterDocumentLoader(), identifier,
                                     encoded_data_length);
}

void FrameFetchContext::DispatchDidDownloadData(unsigned long identifier,
                                                int data_length,
                                                int encoded_data_length) {
  if (IsDetached())
    return;

  GetFrame()->Loader().Progress().IncrementProgress(identifier, data_length);
  probe::didReceiveData(GetFrame()->GetDocument(), identifier,
                        MasterDocumentLoader(), nullptr, data_length);
  probe::didReceiveEncodedDataLength(GetFrame()->GetDocument(),
                                     MasterDocumentLoader(), identifier,
                                     encoded_data_length);
}

void FrameFetchContext::DispatchDidDownloadToBlob(unsigned long identifier,
                                                  BlobDataHandle* blob) {
  if (IsDetached() || !blob)
    return;

  probe::didReceiveBlob(GetFrame()->GetDocument(), identifier,
                        MasterDocumentLoader(), blob);
}

void FrameFetchContext::DispatchDidFinishLoading(
    unsigned long identifier,
    TimeTicks finish_time,
    int64_t encoded_data_length,
    int64_t decoded_body_length,
    bool blocked_cross_site_document) {
  if (IsDetached())
    return;

  GetFrame()->Loader().Progress().CompleteProgress(identifier);
  probe::didFinishLoading(GetFrame()->GetDocument(), identifier,
                          MasterDocumentLoader(), finish_time,
                          encoded_data_length, decoded_body_length,
                          blocked_cross_site_document);
  if (document_) {
    InteractiveDetector* interactive_detector(
        InteractiveDetector::From(*document_));
    if (interactive_detector) {
      interactive_detector->OnResourceLoadEnd(finish_time);
    }
  }
}

void FrameFetchContext::DispatchDidFail(const KURL& url,
                                        unsigned long identifier,
                                        const ResourceError& error,
                                        int64_t encoded_data_length,
                                        bool is_internal_request) {
  if (IsDetached())
    return;

  if (NetworkUtils::IsCertificateTransparencyRequiredError(error.ErrorCode())) {
    UseCounter::Count(
        GetFrame()->GetDocument(),
        WebFeature::kCertificateTransparencyRequiredErrorOnResourceLoad);
  }

  if (NetworkUtils::IsLegacySymantecCertError(error.ErrorCode())) {
    UseCounter::Count(GetFrame()->GetDocument(),
                      WebFeature::kDistrustedLegacySymantecSubresource);
    GetLocalFrameClient()->ReportLegacySymantecCert(url, true /* did_fail */);
  }

  GetFrame()->Loader().Progress().CompleteProgress(identifier);
  probe::didFailLoading(GetFrame()->GetDocument(), identifier,
                        MasterDocumentLoader(), error);
  if (document_) {
    InteractiveDetector* interactive_detector(
        InteractiveDetector::From(*document_));
    if (interactive_detector) {
      // We have not yet recorded load_finish_time. Pass nullopt here; we will
      // call CurrentTimeTicksInSeconds lazily when we need it.
      interactive_detector->OnResourceLoadEnd(base::nullopt);
    }
  }
  // Notification to FrameConsole should come AFTER InspectorInstrumentation
  // call, DevTools front-end relies on this.
  if (!is_internal_request) {
    GetFrame()->Console().DidFailLoading(MasterDocumentLoader(), identifier,
                                         error);
  }
}

void FrameFetchContext::DispatchDidLoadResourceFromMemoryCache(
    unsigned long identifier,
    const ResourceRequest& resource_request,
    const ResourceResponse& resource_response) {
  if (IsDetached())
    return;

  GetLocalFrameClient()->DispatchDidLoadResourceFromMemoryCache(
      resource_request, resource_response);
}

bool FrameFetchContext::ShouldLoadNewResource(Resource::Type type) const {
  if (!document_loader_)
    return true;

  if (IsDetached())
    return false;

  FrameLoader& loader = document_loader_->GetFrame()->Loader();
  if (type == Resource::kMainResource)
    return document_loader_ == loader.GetProvisionalDocumentLoader();
  return document_loader_ == loader.GetDocumentLoader();
}

void FrameFetchContext::RecordLoadingActivity(
    const ResourceRequest& request,
    Resource::Type type,
    const AtomicString& fetch_initiator_name) {
  if (!document_loader_ || document_loader_->Fetcher()->Archive() ||
      !request.Url().IsValid())
    return;
  V8DOMActivityLogger* activity_logger = nullptr;
  if (fetch_initiator_name == FetchInitiatorTypeNames::xmlhttprequest) {
    activity_logger = V8DOMActivityLogger::CurrentActivityLogger();
  } else {
    activity_logger =
        V8DOMActivityLogger::CurrentActivityLoggerIfIsolatedWorld();
  }

  if (activity_logger) {
    Vector<String> argv;
    argv.push_back(Resource::ResourceTypeToString(type, fetch_initiator_name));
    argv.push_back(request.Url());
    activity_logger->LogEvent("blinkRequestResource", argv.size(), argv.data());
  }
}

void FrameFetchContext::DidLoadResource(Resource* resource) {
  if (!document_)
    return;
  FirstMeaningfulPaintDetector::From(*document_).CheckNetworkStable();
  if (LocalFrame* local_frame = document_->GetFrame()) {
    if (IdlenessDetector* idleness_detector =
            local_frame->GetIdlenessDetector()) {
      idleness_detector->OnDidLoadResource();
    }
  }

  if (resource->IsLoadEventBlockingResourceType())
    document_->CheckCompleted();
}

void FrameFetchContext::AddResourceTiming(const ResourceTimingInfo& info) {
  // Normally, |document_| is cleared on Document shutdown. However, Documents
  // for HTML imports will also not have a LocalFrame set: in that case, also
  // early return, as there is nothing to report the resource timing to.
  if (!document_)
    return;
  LocalFrame* frame = document_->GetFrame();
  if (!frame)
    return;

  if (info.IsMainResource()) {
    DCHECK(frame->Owner());
    // Main resource timing information is reported through the owner to be
    // passed to the parent frame, if appropriate.
    frame->Owner()->AddResourceTiming(info);
    frame->DidSendResourceTimingInfoToParent();
    return;
  }

  // All other resources are reported to the corresponding Document.
  DOMWindowPerformance::performance(*document_->domWindow())
      ->GenerateAndAddResourceTiming(info);
}

bool FrameFetchContext::AllowImage(bool images_enabled, const KURL& url) const {
  if (IsDetached())
    return true;

  return GetContentSettingsClient()->AllowImage(images_enabled, url);
}

bool FrameFetchContext::IsControlledByServiceWorker() const {
  if (IsDetached())
    return false;

  DCHECK(MasterDocumentLoader());

  auto* service_worker_network_provider =
      MasterDocumentLoader()->GetServiceWorkerNetworkProvider();
  return service_worker_network_provider &&
         service_worker_network_provider->HasControllerServiceWorker();
}

int64_t FrameFetchContext::ServiceWorkerID() const {
  DCHECK(IsControlledByServiceWorker());
  DCHECK(MasterDocumentLoader());
  auto* service_worker_network_provider =
      MasterDocumentLoader()->GetServiceWorkerNetworkProvider();
  return service_worker_network_provider
             ? service_worker_network_provider->ControllerServiceWorkerID()
             : -1;
}

int FrameFetchContext::ApplicationCacheHostID() const {
  if (!document_loader_)
    return WebApplicationCacheHost::kAppCacheNoHostId;
  return document_loader_->GetApplicationCacheHost()->GetHostID();
}

bool FrameFetchContext::IsMainFrame() const {
  if (IsDetached())
    return frozen_state_->is_main_frame;
  return GetFrame()->IsMainFrame();
}

bool FrameFetchContext::DefersLoading() const {
  return IsDetached() ? false : GetFrame()->GetPage()->Paused();
}

bool FrameFetchContext::IsLoadComplete() const {
  if (IsDetached())
    return true;

  return document_ && document_->LoadEventFinished();
}

bool FrameFetchContext::UpdateTimingInfoForIFrameNavigation(
    ResourceTimingInfo* info) {
  if (IsDetached())
    return false;

  // <iframe>s should report the initial navigation requested by the parent
  // document, but not subsequent navigations.
  if (!GetFrame()->Owner())
    return false;
  // Note that this can be racy since this information is forwarded over IPC
  // when crossing process boundaries.
  if (!GetFrame()->should_send_resource_timing_info_to_parent())
    return false;
  // Do not report iframe navigation that restored from history, since its
  // location may have been changed after initial navigation.
  if (MasterDocumentLoader()->LoadType() == kFrameLoadTypeInitialHistoryLoad)
    return false;
  return true;
}

const SecurityOrigin* FrameFetchContext::GetSecurityOrigin() const {
  if (IsDetached())
    return frozen_state_->security_origin.get();
  return document_ ? document_->GetSecurityOrigin() : nullptr;
}

void FrameFetchContext::ModifyRequestForCSP(ResourceRequest& resource_request) {
  if (IsDetached())
    return;

  // Record the latest requiredCSP value that will be used when sending this
  // request.
  GetFrame()->Loader().RecordLatestRequiredCSP();
  GetFrame()->Loader().ModifyRequestForCSP(resource_request, document_);
}

void FrameFetchContext::AddClientHintsIfNecessary(
    const ClientHintsPreferences& hints_preferences,
    const FetchParameters::ResourceWidth& resource_width,
    ResourceRequest& request) {
  WebEnabledClientHints enabled_hints;

  bool is_1p_origin = false;

  if (blink::RuntimeEnabledFeatures::ClientHintsPersistentEnabled()) {
    // If the feature is enabled, then client hints are allowed only on secure
    // URLs.
    if (!ClientHintsPreferences::IsClientHintsAllowed(request.Url()))
      return;

    // Check if |url| is allowed to run JavaScript. If not, client hints are not
    // attached to the requests that initiate on the render side.
    if (!AllowScriptFromSourceWithoutNotifying(request.Url())) {
      return;
    }

    if (IsDetached())
      return;

    is_1p_origin =
        GetFrame()
            ->Tree()
            .Top()
            .GetSecurityContext()
            ->GetSecurityOrigin()
            ->IsSameSchemeHostPort(SecurityOrigin::Create(request.Url()).get());

    if (!base::FeatureList::IsEnabled(kAllowClientHintsToThirdParty) &&
        !is_1p_origin) {
      // No client hints for 3p origins.
      return;
    }
    // Persisted client hints preferences should be read for only the first
    // party origins.
    if (is_1p_origin && GetContentSettingsClient()) {
      GetContentSettingsClient()->GetAllowedClientHintsFromSource(
          request.Url(), &enabled_hints);
    }
  }

  if (ShouldSendClientHint(mojom::WebClientHintsType::kDeviceMemory,
                           hints_preferences, enabled_hints)) {
    request.AddHTTPHeaderField(
        "Device-Memory",
        AtomicString(String::Number(
            ApproximatedDeviceMemory::GetApproximatedDeviceMemory())));
  }

  float dpr = GetDevicePixelRatio();
  if (ShouldSendClientHint(mojom::WebClientHintsType::kDpr, hints_preferences,
                           enabled_hints)) {
    request.AddHTTPHeaderField("DPR", AtomicString(String::Number(dpr)));
  }

  if (ShouldSendClientHint(mojom::WebClientHintsType::kResourceWidth,
                           hints_preferences, enabled_hints)) {
    if (resource_width.is_set) {
      float physical_width = resource_width.width * dpr;
      request.AddHTTPHeaderField(
          "Width", AtomicString(String::Number(ceil(physical_width))));
    }
  }

  if (ShouldSendClientHint(mojom::WebClientHintsType::kViewportWidth,
                           hints_preferences, enabled_hints) &&
      !IsDetached() && GetFrame()->View()) {
    request.AddHTTPHeaderField(
        "Viewport-Width",
        AtomicString(String::Number(GetFrame()->View()->ViewportWidth())));
  }

  if (!is_1p_origin) {
    // No network quality client hints for 3p origins. Only DPR, resource width
    // and viewport width client hints are allowed for 1p origins.
    return;
  }

  if (ShouldSendClientHint(mojom::WebClientHintsType::kRtt, hints_preferences,
                           enabled_hints)) {
    unsigned long rtt = GetNetworkStateNotifier().RoundRtt(
        request.Url().Host(), GetNetworkStateNotifier().HttpRtt());
    request.AddHTTPHeaderField(
        blink::kClientHintsHeaderMapping[static_cast<size_t>(
            mojom::WebClientHintsType::kRtt)],
        AtomicString(String::Number(rtt)));
  }

  if (ShouldSendClientHint(mojom::WebClientHintsType::kDownlink,
                           hints_preferences, enabled_hints)) {
    double mbps = GetNetworkStateNotifier().RoundMbps(
        request.Url().Host(),
        GetNetworkStateNotifier().DownlinkThroughputMbps());
    request.AddHTTPHeaderField(
        blink::kClientHintsHeaderMapping[static_cast<size_t>(
            mojom::WebClientHintsType::kDownlink)],
        AtomicString(String::Number(mbps)));
  }

  if (ShouldSendClientHint(mojom::WebClientHintsType::kEct, hints_preferences,
                           enabled_hints)) {
    request.AddHTTPHeaderField(
        blink::kClientHintsHeaderMapping[static_cast<size_t>(
            mojom::WebClientHintsType::kEct)],
        AtomicString(NetworkStateNotifier::EffectiveConnectionTypeToString(
            GetNetworkStateNotifier().EffectiveType())));
  }
}

void FrameFetchContext::PopulateResourceRequest(
    Resource::Type type,
    const ClientHintsPreferences& hints_preferences,
    const FetchParameters::ResourceWidth& resource_width,
    ResourceRequest& request) {
  ModifyRequestForCSP(request);
  AddClientHintsIfNecessary(hints_preferences, resource_width, request);
  AddCSPHeaderIfNecessary(type, request);
}

void FrameFetchContext::SetFirstPartyCookieAndRequestorOrigin(
    ResourceRequest& request) {
  // Set the first party for cookies url if it has not been set yet (new
  // requests). This value will be updated during redirects, consistent with
  // https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-00#section-2.1.1?
  if (request.SiteForCookies().IsNull()) {
    if (request.GetFrameType() ==
        network::mojom::RequestContextFrameType::kTopLevel) {
      request.SetSiteForCookies(request.Url());
    } else {
      request.SetSiteForCookies(GetSiteForCookies());
    }
  }

  // * For subresources, the initiator origin is the origin of the
  //   |document_| that contains them.
  // * For loading a new document in the frame, the initiator is not set here.
  //   In most of the cases, it is set in the FrameLoadRequest constructor.
  //   Otherwise, it must be set immediately after the request has been created.
  //   See the calls to ResourceRequest::SetRequestorOrigin().
  if (request.GetFrameType() ==
      network::mojom::RequestContextFrameType::kNone) {
    if (!request.RequestorOrigin())
      request.SetRequestorOrigin(GetRequestorOrigin());
  }
}

MHTMLArchive* FrameFetchContext::Archive() const {
  DCHECK(!IsMainFrame());
  // TODO(nasko): How should this work with OOPIF?
  // The MHTMLArchive is parsed as a whole, but can be constructed from frames
  // in multiple processes. In that case, which process should parse it and how
  // should the output be spread back across multiple processes?
  if (IsDetached() || !GetFrame()->Tree().Parent()->IsLocalFrame())
    return nullptr;
  return ToLocalFrame(GetFrame()->Tree().Parent())
      ->Loader()
      .GetDocumentLoader()
      ->Fetcher()
      ->Archive();
}

bool FrameFetchContext::AllowScriptFromSource(const KURL& url) const {
  if (AllowScriptFromSourceWithoutNotifying(url))
    return true;
  ContentSettingsClient* settings_client = GetContentSettingsClient();
  if (settings_client)
    settings_client->DidNotAllowScript();
  return false;
}

bool FrameFetchContext::AllowScriptFromSourceWithoutNotifying(
    const KURL& url) const {
  ContentSettingsClient* settings_client = GetContentSettingsClient();
  Settings* settings = GetSettings();
  if (settings_client && !settings_client->AllowScriptFromSource(
                             !settings || settings->GetScriptEnabled(), url)) {
    return false;
  }
  return true;
}

bool FrameFetchContext::ShouldBlockRequestByInspector(const KURL& url) const {
  if (IsDetached())
    return false;
  bool should_block_request = false;
  probe::shouldBlockRequest(GetFrame()->GetDocument(), url,
                            &should_block_request);
  return should_block_request;
}

void FrameFetchContext::DispatchDidBlockRequest(
    const ResourceRequest& resource_request,
    const FetchInitiatorInfo& fetch_initiator_info,
    ResourceRequestBlockedReason blocked_reason,
    Resource::Type resource_type) const {
  if (IsDetached())
    return;
  probe::didBlockRequest(GetFrame()->GetDocument(), resource_request,
                         MasterDocumentLoader(), fetch_initiator_info,
                         blocked_reason, resource_type);
}

bool FrameFetchContext::ShouldBypassMainWorldCSP() const {
  if (IsDetached())
    return false;

  return GetFrame()->GetScriptController().ShouldBypassMainWorldCSP();
}

bool FrameFetchContext::IsSVGImageChromeClient() const {
  if (IsDetached())
    return frozen_state_->is_svg_image_chrome_client;

  return GetFrame()->GetChromeClient().IsSVGImageChromeClient();
}

void FrameFetchContext::CountUsage(WebFeature feature) const {
  if (IsDetached())
    return;
  UseCounter::Count(GetFrame(), feature);
}

void FrameFetchContext::CountDeprecation(WebFeature feature) const {
  if (IsDetached())
    return;
  Deprecation::CountDeprecation(GetFrame(), feature);
}

bool FrameFetchContext::ShouldBlockWebSocketByMixedContentCheck(
    const KURL& url) const {
  if (IsDetached()) {
    // TODO(yhirano): Implement the detached case.
    return false;
  }
  return !MixedContentChecker::IsWebSocketAllowed(GetFrame(), url);
}

std::unique_ptr<WebSocketHandshakeThrottle>
FrameFetchContext::CreateWebSocketHandshakeThrottle() {
  if (IsDetached()) {
    // TODO(yhirano): Implement the detached case.
    return nullptr;
  }
  if (!GetFrame())
    return nullptr;
  return WebFrame::FromFrame(GetFrame())
      ->ToWebLocalFrame()
      ->Client()
      ->CreateWebSocketHandshakeThrottle();
}

bool FrameFetchContext::ShouldBlockFetchByMixedContentCheck(
    WebURLRequest::RequestContext request_context,
    network::mojom::RequestContextFrameType frame_type,
    ResourceRequest::RedirectStatus redirect_status,
    const KURL& url,
    SecurityViolationReportingPolicy reporting_policy) const {
  if (IsDetached()) {
    // TODO(yhirano): Implement the detached case.
    return false;
  }
  return MixedContentChecker::ShouldBlockFetch(GetFrame(), request_context,
                                               frame_type, redirect_status, url,
                                               reporting_policy);
}

bool FrameFetchContext::ShouldBlockFetchAsCredentialedSubresource(
    const ResourceRequest& resource_request,
    const KURL& url) const {
  // BlockCredentialedSubresources has already been checked on the
  // browser-side. It should not be checked a second time here because the
  // renderer-side implementation suffers from https://crbug.com/756846.
  if (!resource_request.CheckForBrowserSideNavigation())
    return false;

  // URLs with no embedded credentials should load correctly.
  if (url.User().IsEmpty() && url.Pass().IsEmpty())
    return false;

  if (resource_request.GetRequestContext() ==
      WebURLRequest::kRequestContextXMLHttpRequest) {
    return false;
  }

  // Relative URLs on top-level pages that were loaded with embedded credentials
  // should load correctly.
  // TODO(mkwst): This doesn't work when the subresource is an iframe.
  // See https://crbug.com/756846.
  if (Url().User() == url.User() && Url().Pass() == url.Pass() &&
      SecurityOrigin::Create(url)->IsSameSchemeHostPort(GetSecurityOrigin())) {
    return false;
  }

  CountDeprecation(WebFeature::kRequestedSubresourceWithEmbeddedCredentials);

  // TODO(mkwst): Remove the runtime check one way or the other once we're
  // sure it's going to stick (or that it's not).
  return RuntimeEnabledFeatures::BlockCredentialedSubresourcesEnabled();
}

ReferrerPolicy FrameFetchContext::GetReferrerPolicy() const {
  if (IsDetached())
    return frozen_state_->referrer_policy;
  return document_->GetReferrerPolicy();
}

String FrameFetchContext::GetOutgoingReferrer() const {
  if (IsDetached())
    return frozen_state_->outgoing_referrer;
  return document_->OutgoingReferrer();
}

const KURL& FrameFetchContext::Url() const {
  if (IsDetached())
    return frozen_state_->url;
  if (!document_)
    return NullURL();
  return document_->Url();
}

const SecurityOrigin* FrameFetchContext::GetParentSecurityOrigin() const {
  if (IsDetached())
    return frozen_state_->parent_security_origin.get();
  Frame* parent = GetFrame()->Tree().Parent();
  if (!parent)
    return nullptr;
  return parent->GetSecurityContext()->GetSecurityOrigin();
}

base::Optional<mojom::IPAddressSpace> FrameFetchContext::GetAddressSpace()
    const {
  if (IsDetached())
    return frozen_state_->address_space;
  if (!document_)
    return base::nullopt;
  ExecutionContext* context = document_;
  return base::make_optional(context->GetSecurityContext().AddressSpace());
}

const ContentSecurityPolicy* FrameFetchContext::GetContentSecurityPolicy()
    const {
  if (IsDetached())
    return frozen_state_->content_security_policy;
  return document_ ? document_->GetContentSecurityPolicy() : nullptr;
}

void FrameFetchContext::AddConsoleMessage(ConsoleMessage* message) const {
  if (IsDetached())
    return;

  // Route the console message through Document if it's attached, so
  // that script line numbers can be included. Otherwise, route directly to the
  // FrameConsole, to ensure we never drop a message.
  if (document_ && document_->GetFrame())
    document_->AddConsoleMessage(message);
  else
    GetFrame()->Console().AddMessage(message);
}

ContentSettingsClient* FrameFetchContext::GetContentSettingsClient() const {
  if (IsDetached())
    return nullptr;
  return GetFrame()->GetContentSettingsClient();
}

Settings* FrameFetchContext::GetSettings() const {
  if (IsDetached())
    return nullptr;
  DCHECK(GetFrame());
  return GetFrame()->GetSettings();
}

String FrameFetchContext::GetUserAgent() const {
  if (IsDetached())
    return frozen_state_->user_agent;
  return GetFrame()->Loader().UserAgent();
}

scoped_refptr<const SecurityOrigin> FrameFetchContext::GetRequestorOrigin() {
  if (IsDetached())
    return frozen_state_->requestor_origin;

  // Should not be called during the document load, and |document_| should be
  // always set beforehand for a subresource loading.
  DCHECK(document_);

  // If sandbox is enabled and allow-same-origin is not set in the attribute,
  // |document|'s SecurityOrigin is set to the unique opaque origin, and
  // FrameFetchContext::GetSecurityOrigin also respects the unique origin.
  // But, we still need to set the unveiled document origin to the requestor
  // origin. See also sandbox's spec;
  // https://html.spec.whatwg.org/multipage/iframe-embed-object.html#attr-iframe-sandbox.
  if (document_->IsSandboxed(kSandboxOrigin))
    return SecurityOrigin::Create(document_->Url());

  return GetSecurityOrigin();
}

ClientHintsPreferences FrameFetchContext::GetClientHintsPreferences() const {
  if (IsDetached())
    return frozen_state_->client_hints_preferences;

  if (!document_)
    return ClientHintsPreferences();

  return document_->GetClientHintsPreferences();
}

float FrameFetchContext::GetDevicePixelRatio() const {
  if (IsDetached())
    return frozen_state_->device_pixel_ratio;

  if (!document_) {
    // Note that this value is not used because the preferences object returned
    // by GetClientHintsPreferences() doesn't allow to use it.
    return 1.0;
  }

  return document_->DevicePixelRatio();
}

bool FrameFetchContext::ShouldSendClientHint(
    mojom::WebClientHintsType type,
    const ClientHintsPreferences& hints_preferences,
    const WebEnabledClientHints& enabled_hints) const {
  return GetClientHintsPreferences().ShouldSend(type) ||
         hints_preferences.ShouldSend(type) || enabled_hints.IsEnabled(type);
}

void FrameFetchContext::ParseAndPersistClientHints(
    const ResourceResponse& response) {
  ClientHintsPreferences hints_preferences;
  WebEnabledClientHints enabled_client_hints;
  TimeDelta persist_duration;
  FrameClientHintsPreferencesContext hints_context(GetFrame());
  hints_preferences.UpdatePersistentHintsFromHeaders(
      response, &hints_context, enabled_client_hints, &persist_duration);

  if (persist_duration.InSeconds() <= 0)
    return;

  if (!AllowScriptFromSourceWithoutNotifying(response.Url())) {
    // Do not persist client hint preferences if the JavaScript is disabled.
    return;
  }

  GetContentSettingsClient()->PersistClientHints(
      enabled_client_hints, persist_duration, response.Url());
}

std::unique_ptr<WebURLLoader> FrameFetchContext::CreateURLLoader(
    const ResourceRequest& request,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const ResourceLoaderOptions& options) {
  DCHECK(!IsDetached());
  WrappedResourceRequest webreq(request);

  network::mojom::blink::URLLoaderFactoryPtr url_loader_factory;
  if (options.url_loader_factory) {
    options.url_loader_factory->data->Clone(MakeRequest(&url_loader_factory));
  }
  // Resolve any blob: URLs that haven't been resolved yet. The XHR and fetch()
  // API implementations resolve blob URLs earlier because there can be
  // arbitrarily long delays between creating requests with those APIs and
  // actually creating the URL loader here. Other subresource loading will
  // immediately create the URL loader so resolving those blob URLs here is
  // simplest.
  if (document_ && request.Url().ProtocolIs("blob") &&
      RuntimeEnabledFeatures::MojoBlobURLsEnabled() && !url_loader_factory) {
    document_->GetPublicURLManager().Resolve(request.Url(),
                                             MakeRequest(&url_loader_factory));
  }
  if (url_loader_factory) {
    return Platform::Current()
        ->WrapURLLoaderFactory(url_loader_factory.PassInterface().PassHandle())
        ->CreateURLLoader(webreq, task_runner);
  }

  if (MasterDocumentLoader()->GetServiceWorkerNetworkProvider()) {
    auto loader = MasterDocumentLoader()
                      ->GetServiceWorkerNetworkProvider()
                      ->CreateURLLoader(webreq, task_runner);
    if (loader)
      return loader;
  }
  return GetFrame()->GetURLLoaderFactory()->CreateURLLoader(webreq,
                                                            task_runner);
}

FetchContext* FrameFetchContext::Detach() {
  if (IsDetached())
    return this;

  if (document_) {
    frozen_state_ = new FrozenState(
        GetReferrerPolicy(), GetOutgoingReferrer(), Url(), GetSecurityOrigin(),
        GetParentSecurityOrigin(), GetAddressSpace(),
        GetContentSecurityPolicy(), GetSiteForCookies(), GetRequestorOrigin(),
        GetClientHintsPreferences(), GetDevicePixelRatio(), GetUserAgent(),
        IsMainFrame(), IsSVGImageChromeClient());
  } else {
    // Some getters are unavailable in this case.
    frozen_state_ = new FrozenState(
        kReferrerPolicyDefault, String(), NullURL(), GetSecurityOrigin(),
        GetParentSecurityOrigin(), GetAddressSpace(),
        GetContentSecurityPolicy(), GetSiteForCookies(),
        SecurityOrigin::CreateUnique(), GetClientHintsPreferences(),
        GetDevicePixelRatio(), GetUserAgent(), IsMainFrame(),
        IsSVGImageChromeClient());
  }

  // This is needed to break a reference cycle in which off-heap
  // ComputedStyle is involved. See https://crbug.com/383860 for details.
  document_ = nullptr;

  return this;
}

void FrameFetchContext::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_loader_);
  visitor->Trace(document_);
  visitor->Trace(frozen_state_);
  BaseFetchContext::Trace(visitor);
}

void FrameFetchContext::RecordDataUriWithOctothorpe() {
  CountDeprecation(WebFeature::kDataUriHasOctothorpe);
}

ResourceLoadPriority FrameFetchContext::ModifyPriorityForExperiments(
    ResourceLoadPriority priority) const {
  if (!GetSettings())
    return priority;

  WebEffectiveConnectionType max_effective_connection_type_threshold =
      GetSettings()->GetLowPriorityIframesThreshold();

  if (max_effective_connection_type_threshold <=
      WebEffectiveConnectionType::kTypeOffline) {
    return priority;
  }

  WebEffectiveConnectionType effective_connection_type =
      GetNetworkStateNotifier().EffectiveType();

  if (effective_connection_type <= WebEffectiveConnectionType::kTypeOffline) {
    return priority;
  }

  if (effective_connection_type > max_effective_connection_type_threshold) {
    // Network is not slow enough.
    return priority;
  }

  if (GetFrame()->IsMainFrame()) {
    DEFINE_STATIC_LOCAL(EnumerationHistogram, main_frame_priority_histogram,
                        ("LowPriorityIframes.MainFrameRequestPriority",
                         static_cast<int>(ResourceLoadPriority::kHighest) + 1));
    main_frame_priority_histogram.Count(static_cast<int>(priority));
    return priority;
  }

  DEFINE_STATIC_LOCAL(EnumerationHistogram, iframe_priority_histogram,
                      ("LowPriorityIframes.IframeRequestPriority",
                       static_cast<int>(ResourceLoadPriority::kHighest) + 1));
  iframe_priority_histogram.Count(static_cast<int>(priority));
  // When enabled, the priority of all resources in subframe is dropped.
  // Non-delayable resources are assigned a priority of kLow, and the rest of
  // them are assigned a priority of kLowest. This ensures that if the webpage
  // fetches most of its primary content using iframes, then high priority
  // requests within the iframe go on the network first.
  if (priority >= ResourceLoadPriority::kHigh)
    return ResourceLoadPriority::kLow;
  return ResourceLoadPriority::kLowest;
}

base::Optional<ResourceRequestBlockedReason> FrameFetchContext::CanRequest(
    Resource::Type type,
    const ResourceRequest& resource_request,
    const KURL& url,
    const ResourceLoaderOptions& options,
    SecurityViolationReportingPolicy reporting_policy,
    FetchParameters::OriginRestriction origin_restriction,
    ResourceRequest::RedirectStatus redirect_status) const {
  if (document_ && document_->IsFreezingInProgress() &&
      !resource_request.GetKeepalive()) {
    AddErrorConsoleMessage(
        "Only fetch keepalive is allowed during onfreeze: " + url.GetString(),
        kJSSource);
    return ResourceRequestBlockedReason::kOther;
  }
  return BaseFetchContext::CanRequest(type, resource_request, url, options,
                                      reporting_policy, origin_restriction,
                                      redirect_status);
}

}  // namespace blink
