/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/document_loader.h"

#include <memory>
#include "base/auto_reset.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_network_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_history_commit_type.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_parser.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"
#include "third_party/blink/renderer/core/dom/weak_identifier_map.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/parser/css_preload_scanner.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/loader/appcache/application_cache_host.h"
#include "third_party/blink/renderer/core/loader/frame_fetch_context.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/idleness_detector.h"
#include "third_party/blink/renderer/core/loader/interactive_detector.h"
#include "third_party/blink/renderer/core/loader/link_loader.h"
#include "third_party/blink/renderer/core/loader/network_hints_interface.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/core/loader/resource/css_style_sheet_resource.h"
#include "third_party/blink/renderer/core/loader/resource/font_resource.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource.h"
#include "third_party/blink/renderer/core/loader/resource/script_resource.h"
#include "third_party/blink/renderer/core/loader/subresource_filter.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/page/frame_tree.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_info.h"
#include "third_party/blink/renderer/platform/mhtml/archive_resource.h"
#include "third_party/blink/renderer/platform/mhtml/mhtml_archive.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_response_headers.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/plugins/plugin_data.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// The MHTML mime type should be same as the one we check in the browser
// process's IsDownload (navigation_url_loader_network_service.cc).
static bool IsArchiveMIMEType(const String& mime_type) {
  return DeprecatedEqualIgnoringCase("multipart/related", mime_type) ||
         DeprecatedEqualIgnoringCase("message/rfc822", mime_type);
}

DocumentLoader::DocumentLoader(
    LocalFrame* frame,
    const ResourceRequest& req,
    const SubstituteData& substitute_data,
    ClientRedirectPolicy client_redirect_policy,
    const base::UnguessableToken& devtools_navigation_token)
    : frame_(frame),
      fetcher_(FrameFetchContext::CreateFetcherFromDocumentLoader(this)),
      original_request_(req),
      substitute_data_(substitute_data),
      request_(req),
      load_type_(kFrameLoadTypeStandard),
      is_client_redirect_(client_redirect_policy ==
                          ClientRedirectPolicy::kClientRedirect),
      replaces_current_history_item_(false),
      data_received_(false),
      navigation_type_(kNavigationTypeOther),
      document_load_timing_(*this),
      application_cache_host_(ApplicationCacheHost::Create(this)),
      was_blocked_after_csp_(false),
      state_(kNotStarted),
      committed_data_buffer_(nullptr),
      in_data_received_(false),
      data_buffer_(SharedBuffer::Create()),
      devtools_navigation_token_(devtools_navigation_token),
      user_activated_(false) {
  DCHECK(frame_);

  // The document URL needs to be added to the head of the list as that is
  // where the redirects originated.
  if (is_client_redirect_)
    AppendRedirect(frame_->GetDocument()->Url());
}

FrameLoader& DocumentLoader::GetFrameLoader() const {
  DCHECK(frame_);
  return frame_->Loader();
}

LocalFrameClient& DocumentLoader::GetLocalFrameClient() const {
  DCHECK(frame_);
  LocalFrameClient* client = frame_->Client();
  // LocalFrame clears its |m_client| only after detaching all DocumentLoaders
  // (i.e. calls detachFromFrame() which clears |frame_|) owned by the
  // LocalFrame's FrameLoader. So, if |frame_| is non nullptr, |client| is
  // also non nullptr.
  DCHECK(client);
  return *client;
}

DocumentLoader::~DocumentLoader() {
  DCHECK(!frame_);
  DCHECK(!GetResource());
  DCHECK(!application_cache_host_);
  DCHECK_EQ(state_, kSentDidFinishLoad);
}

void DocumentLoader::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(fetcher_);
  visitor->Trace(history_item_);
  visitor->Trace(parser_);
  visitor->Trace(subresource_filter_);
  visitor->Trace(document_load_timing_);
  visitor->Trace(application_cache_host_);
  visitor->Trace(content_security_policy_);
  RawResourceClient::Trace(visitor);
}

unsigned long DocumentLoader::MainResourceIdentifier() const {
  return GetResource() ? GetResource()->Identifier() : 0;
}

ResourceTimingInfo* DocumentLoader::GetNavigationTimingInfo() const {
  DCHECK(Fetcher());
  return Fetcher()->GetNavigationTimingInfo();
}

const ResourceRequest& DocumentLoader::OriginalRequest() const {
  return original_request_;
}

const ResourceRequest& DocumentLoader::GetRequest() const {
  return request_;
}

void DocumentLoader::SetSubresourceFilter(
    SubresourceFilter* subresource_filter) {
  subresource_filter_ = subresource_filter;
}

const KURL& DocumentLoader::Url() const {
  return request_.Url();
}

Resource* DocumentLoader::StartPreload(Resource::Type type,
                                       FetchParameters& params,
                                       CSSPreloaderResourceClient* client) {
  Resource* resource = nullptr;
  DCHECK(!client || type == Resource::kCSSStyleSheet);
  switch (type) {
    case Resource::kImage:
      if (frame_)
        frame_->MaybeAllowImagePlaceholder(params);
      resource = ImageResource::Fetch(params, Fetcher());
      break;
    case Resource::kScript:
      params.SetRequestContext(WebURLRequest::kRequestContextScript);
      resource = ScriptResource::Fetch(params, Fetcher(), nullptr);
      break;
    case Resource::kCSSStyleSheet:
      resource = CSSStyleSheetResource::Fetch(params, Fetcher(), client);
      break;
    case Resource::kFont:
      resource = FontResource::Fetch(params, Fetcher(), nullptr);
      break;
    case Resource::kAudio:
    case Resource::kVideo:
      resource = RawResource::FetchMedia(params, Fetcher(), nullptr);
      break;
    case Resource::kTextTrack:
      resource = RawResource::FetchTextTrack(params, Fetcher(), nullptr);
      break;
    case Resource::kImportResource:
      resource = RawResource::FetchImport(params, Fetcher(), nullptr);
      break;
    case Resource::kRaw:
      resource = RawResource::Fetch(params, Fetcher(), nullptr);
      break;
    default:
      NOTREACHED();
  }

  return resource;
}

void DocumentLoader::SetServiceWorkerNetworkProvider(
    std::unique_ptr<WebServiceWorkerNetworkProvider> provider) {
  service_worker_network_provider_ = std::move(provider);
}

void DocumentLoader::SetSourceLocation(
    std::unique_ptr<SourceLocation> source_location) {
  source_location_ = std::move(source_location);
}

std::unique_ptr<SourceLocation> DocumentLoader::CopySourceLocation() const {
  return source_location_ ? source_location_->Clone() : nullptr;
}

void DocumentLoader::DispatchLinkHeaderPreloads(
    ViewportDescriptionWrapper* viewport,
    LinkLoader::MediaPreloadPolicy media_policy) {
  DCHECK_GE(state_, kCommitted);
  LinkLoader::LoadLinksFromHeader(
      GetResponse().HttpHeaderField(HTTPNames::Link), GetResponse().Url(),
      *frame_, frame_->GetDocument(), NetworkHintsInterfaceImpl(),
      LinkLoader::kOnlyLoadResources, media_policy, viewport);
}

void DocumentLoader::DidChangePerformanceTiming() {
  if (frame_ && state_ >= kCommitted) {
    GetLocalFrameClient().DidChangePerformanceTiming();
  }
}

void DocumentLoader::DidObserveLoadingBehavior(
    WebLoadingBehaviorFlag behavior) {
  if (frame_) {
    DCHECK_GE(state_, kCommitted);
    GetLocalFrameClient().DidObserveLoadingBehavior(behavior);
  }
}

void DocumentLoader::MarkAsCommitted() {
  DCHECK_LT(state_, kCommitted);
  state_ = kCommitted;
}

static HistoryCommitType LoadTypeToCommitType(FrameLoadType type) {
  switch (type) {
    case kFrameLoadTypeStandard:
      return kStandardCommit;
    case kFrameLoadTypeInitialInChildFrame:
    case kFrameLoadTypeInitialHistoryLoad:
      return kInitialCommitInChildFrame;
    case kFrameLoadTypeBackForward:
      return kBackForwardCommit;
    default:
      break;
  }
  return kHistoryInertCommit;
}

void DocumentLoader::UpdateForSameDocumentNavigation(
    const KURL& new_url,
    SameDocumentNavigationSource same_document_navigation_source,
    scoped_refptr<SerializedScriptValue> data,
    HistoryScrollRestorationType scroll_restoration_type,
    FrameLoadType type,
    Document* initiating_document) {
  if (type == kFrameLoadTypeStandard && initiating_document &&
      !initiating_document->CanCreateHistoryEntry()) {
    type = kFrameLoadTypeReplaceCurrentItem;
  }

  KURL old_url = request_.Url();
  original_request_.SetURL(new_url);
  request_.SetURL(new_url);
  SetReplacesCurrentHistoryItem(type != kFrameLoadTypeStandard);
  if (same_document_navigation_source == kSameDocumentNavigationHistoryApi) {
    request_.SetHTTPMethod(HTTPNames::GET);
    request_.SetHTTPBody(nullptr);
  }
  ClearRedirectChain();
  if (is_client_redirect_)
    AppendRedirect(old_url);
  AppendRedirect(new_url);

  SetHistoryItemStateForCommit(
      history_item_.Get(), type,
      same_document_navigation_source == kSameDocumentNavigationHistoryApi
          ? HistoryNavigationType::kHistoryApi
          : HistoryNavigationType::kFragment);
  history_item_->SetDocumentState(frame_->GetDocument()->FormElementsState());
  if (same_document_navigation_source == kSameDocumentNavigationHistoryApi) {
    history_item_->SetStateObject(std::move(data));
    history_item_->SetScrollRestorationType(scroll_restoration_type);
  }
  HistoryCommitType commit_type = LoadTypeToCommitType(type);
  frame_->GetFrameScheduler()->DidCommitProvisionalLoad(
      commit_type == kHistoryInertCommit, type == kFrameLoadTypeReload,
      frame_->IsLocalRoot());
  GetLocalFrameClient().DidFinishSameDocumentNavigation(
      history_item_.Get(), commit_type, initiating_document);
  probe::didNavigateWithinDocument(frame_);
}

const KURL& DocumentLoader::UrlForHistory() const {
  return UnreachableURL().IsEmpty() ? Url() : UnreachableURL();
}

void DocumentLoader::SetHistoryItemStateForCommit(
    HistoryItem* old_item,
    FrameLoadType load_type,
    HistoryNavigationType navigation_type) {
  if (!history_item_ || !IsBackForwardLoadType(load_type))
    history_item_ = HistoryItem::Create();

  history_item_->SetURL(UrlForHistory());
  history_item_->SetReferrer(SecurityPolicy::GenerateReferrer(
      request_.GetReferrerPolicy(), history_item_->Url(),
      request_.HttpReferrer()));
  history_item_->SetFormInfoFromRequest(request_);

  // Don't propagate state from the old item to the new item if there isn't an
  // old item (obviously), or if this is a back/forward navigation, since we
  // explicitly want to restore the state we just committed.
  if (!old_item || IsBackForwardLoadType(load_type))
    return;
  // Don't propagate state from the old item if this is a different-document
  // navigation, unless the before and after pages are logically related. This
  // means they have the same url (ignoring fragment) and the new item was
  // loaded via reload or client redirect.
  HistoryCommitType history_commit_type = LoadTypeToCommitType(load_type);
  if (navigation_type == HistoryNavigationType::kDifferentDocument &&
      (history_commit_type != kHistoryInertCommit ||
       !EqualIgnoringFragmentIdentifier(old_item->Url(), history_item_->Url())))
    return;
  history_item_->SetDocumentSequenceNumber(old_item->DocumentSequenceNumber());

  history_item_->CopyViewStateFrom(old_item);
  history_item_->SetScrollRestorationType(old_item->ScrollRestorationType());

  // The item sequence number determines whether items are "the same", such
  // back/forward navigation between items with the same item sequence number is
  // a no-op. Only treat this as identical if the navigation did not create a
  // back/forward entry and the url is identical or it was loaded via
  // history.replaceState().
  if (history_commit_type == kHistoryInertCommit &&
      (navigation_type == HistoryNavigationType::kHistoryApi ||
       old_item->Url() == history_item_->Url())) {
    history_item_->SetStateObject(old_item->StateObject());
    history_item_->SetItemSequenceNumber(old_item->ItemSequenceNumber());
  }
}

void DocumentLoader::NotifyFinished(Resource* resource) {
  DCHECK_EQ(GetResource(), resource);
  DCHECK(GetResource());

  if (!resource->ErrorOccurred() && !resource->WasCanceled()) {
    FinishedLoading(resource->LoadFinishTime());
    return;
  }

  if (application_cache_host_)
    application_cache_host_->FailedLoadingMainResource();

  if (resource->GetResourceError().WasBlockedByResponse()) {
    probe::CanceledAfterReceivedResourceResponse(
        frame_, this, MainResourceIdentifier(), resource->GetResponse(),
        resource);
  }

  LoadFailed(resource->GetResourceError());
  ClearResource();
}

void DocumentLoader::LoadFailed(const ResourceError& error) {
  if (!error.IsCancellation() && frame_->Owner())
    frame_->Owner()->RenderFallbackContent();
  fetcher_->ClearResourcesFromPreviousFetcher();

  HistoryCommitType history_commit_type = LoadTypeToCommitType(load_type_);
  switch (state_) {
    case kNotStarted:
      probe::frameClearedScheduledClientNavigation(frame_);
      FALLTHROUGH;
    case kProvisional:
      state_ = kSentDidFinishLoad;
      GetLocalFrameClient().DispatchDidFailProvisionalLoad(error,
                                                           history_commit_type);
      if (frame_)
        GetFrameLoader().DetachProvisionalDocumentLoader(this);
      break;
    case kCommitted:
      if (frame_->GetDocument()->Parser())
        frame_->GetDocument()->Parser()->StopParsing();
      state_ = kSentDidFinishLoad;
      GetLocalFrameClient().DispatchDidFailLoad(error, history_commit_type);
      GetFrameLoader().DidFinishNavigation();
      break;
    case kSentDidFinishLoad:
      NOTREACHED();
      break;
  }
  DCHECK_EQ(kSentDidFinishLoad, state_);
}

void DocumentLoader::SetUserActivated() {
  user_activated_ = true;
}

const AtomicString& DocumentLoader::RequiredCSP() {
  return GetFrameLoader().RequiredCSP();
}

void DocumentLoader::FinishedLoading(TimeTicks finish_time) {
  DCHECK(frame_->Loader().StateMachine()->CreatingInitialEmptyDocument() ||
         !frame_->GetPage()->Paused() ||
         MainThreadDebugger::Instance()->IsPaused());

  TimeTicks response_end_time = finish_time;
  if (response_end_time.is_null())
    response_end_time = time_of_last_data_received_;
  if (response_end_time.is_null())
    response_end_time = CurrentTimeTicks();
  GetTiming().SetResponseEnd(response_end_time);
  if (!MaybeCreateArchive()) {
    // If this is an empty document, it will not have actually been created yet.
    // Force a commit so that the Document actually gets created.
    if (state_ == kProvisional)
      CommitData(nullptr, 0);
  }

  if (!frame_)
    return;

  application_cache_host_->FinishedLoadingMainResource();
  if (parser_) {
    if (parser_blocked_count_) {
      finished_loading_ = true;
    } else {
      parser_->Finish();
      parser_.Clear();
    }
  }
  ClearResource();
}

bool DocumentLoader::RedirectReceived(
    Resource* resource,
    const ResourceRequest& request,
    const ResourceResponse& redirect_response) {
  DCHECK(frame_);
  DCHECK_EQ(resource, GetResource());
  DCHECK(!redirect_response.IsNull());
  request_ = request;

  // If the redirecting url is not allowed to display content from the target
  // origin, then block the redirect.
  const KURL& request_url = request_.Url();
  scoped_refptr<const SecurityOrigin> redirecting_origin =
      SecurityOrigin::Create(redirect_response.Url());
  if (!redirecting_origin->CanDisplay(request_url)) {
    frame_->Console().AddMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Not allowed to load local resource: " + request_url.GetString()));
    fetcher_->StopFetching();
    return false;
  }
  if (GetFrameLoader().ShouldContinueForRedirectNavigationPolicy(
          request_, SubstituteData(), this, kCheckContentSecurityPolicy,
          navigation_type_, kNavigationPolicyCurrentTab, load_type_,
          IsClientRedirect(), nullptr) != kNavigationPolicyCurrentTab) {
    fetcher_->StopFetching();
    return false;
  }

  DCHECK(!GetTiming().FetchStart().is_null());
  AppendRedirect(request_url);
  GetTiming().AddRedirect(redirect_response.Url(), request_url);

  // If a redirection happens during a back/forward navigation, don't restore
  // any state from the old HistoryItem. There is a provisional history item for
  // back/forward navigation only. In the other case, clearing it is a no-op.
  history_item_.Clear();

  GetLocalFrameClient().DispatchDidReceiveServerRedirectForProvisionalLoad();

  return true;
}

static bool CanShowMIMEType(const String& mime_type, LocalFrame* frame) {
  if (MIMETypeRegistry::IsSupportedMIMEType(mime_type))
    return true;
  PluginData* plugin_data = frame->GetPluginData();
  return !mime_type.IsEmpty() && plugin_data &&
         plugin_data->SupportsMimeType(mime_type);
}

bool DocumentLoader::ShouldContinueForResponse() const {
  if (substitute_data_.IsValid())
    return true;

  int status_code = response_.HttpStatusCode();
  if (status_code == 204 || status_code == 205) {
    // The server does not want us to replace the page contents.
    return false;
  }

  if (IsContentDispositionAttachment(
          response_.HttpHeaderField(HTTPNames::Content_Disposition))) {
    // The server wants us to download instead of replacing the page contents.
    // Downloading is handled by the embedder, but we still get the initial
    // response so that we can ignore it and clean up properly.
    return false;
  }

  if (!CanShowMIMEType(response_.MimeType(), frame_))
    return false;
  return true;
}

void DocumentLoader::CancelLoadAfterCSPDenied(
    const ResourceResponse& response) {
  probe::CanceledAfterReceivedResourceResponse(
      frame_, this, MainResourceIdentifier(), response, GetResource());

  SetWasBlockedAfterCSP();

  // Pretend that this was an empty HTTP 200 response.  Don't reuse the original
  // URL for the empty page (https://crbug.com/622385).
  //
  // TODO(mkwst):  Remove this once XFO moves to the browser.
  // https://crbug.com/555418.
  ClearResource();
  content_security_policy_.Clear();
  KURL blocked_url = SecurityOrigin::UrlWithUniqueOpaqueOrigin();
  original_request_.SetURL(blocked_url);
  request_.SetURL(blocked_url);
  redirect_chain_.pop_back();
  AppendRedirect(blocked_url);
  response_ = ResourceResponse(blocked_url, "text/html");
  FinishedLoading(CurrentTimeTicks());

  return;
}

void DocumentLoader::ResponseReceived(
    Resource* resource,
    const ResourceResponse& response,
    std::unique_ptr<WebDataConsumerHandle> handle) {
  DCHECK_EQ(GetResource(), resource);
  DCHECK(!handle);
  DCHECK(frame_);

  application_cache_host_->DidReceiveResponseForMainResource(response);

  // The memory cache doesn't understand the application cache or its caching
  // rules. So if a main resource is served from the application cache, ensure
  // we don't save the result for future use. All responses loaded from appcache
  // will have a non-zero appCacheID().
  if (response.AppCacheID())
    GetMemoryCache()->Remove(resource);

  content_security_policy_ = ContentSecurityPolicy::Create();
  content_security_policy_->SetOverrideURLForSelf(response.Url());
  if (!frame_->GetSettings()->BypassCSP()) {
    content_security_policy_->DidReceiveHeaders(
        ContentSecurityPolicyResponseHeaders(response));
  }
  if (!content_security_policy_->AllowAncestors(frame_, response.Url())) {
    CancelLoadAfterCSPDenied(response);
    return;
  }

  if (!frame_->GetSettings()->BypassCSP() &&
      RuntimeEnabledFeatures::EmbedderCSPEnforcementEnabled() &&
      !GetFrameLoader().RequiredCSP().IsEmpty()) {
    const SecurityOrigin* parent_security_origin =
        frame_->Tree().Parent()->GetSecurityContext()->GetSecurityOrigin();
    if (ContentSecurityPolicy::ShouldEnforceEmbeddersPolicy(
            response, parent_security_origin)) {
      content_security_policy_->AddPolicyFromHeaderValue(
          GetFrameLoader().RequiredCSP(),
          kContentSecurityPolicyHeaderTypeEnforce,
          kContentSecurityPolicyHeaderSourceHTTP);
    } else {
      ContentSecurityPolicy* required_csp = ContentSecurityPolicy::Create();
      required_csp->AddPolicyFromHeaderValue(
          GetFrameLoader().RequiredCSP(),
          kContentSecurityPolicyHeaderTypeEnforce,
          kContentSecurityPolicyHeaderSourceHTTP);
      if (!required_csp->Subsumes(*content_security_policy_)) {
        String message = "Refused to display '" +
                         response.Url().ElidedString() +
                         "' because it has not opted-into the following policy "
                         "required by its embedder: '" +
                         GetFrameLoader().RequiredCSP() + "'.";
        ConsoleMessage* console_message = ConsoleMessage::CreateForRequest(
            kSecurityMessageSource, kErrorMessageLevel, message, response.Url(),
            this, MainResourceIdentifier());
        frame_->GetDocument()->AddConsoleMessage(console_message);
        CancelLoadAfterCSPDenied(response);
        return;
      }
    }
  }

  DCHECK(!frame_->GetPage()->Paused());

  if (response.DidServiceWorkerNavigationPreload())
    UseCounter::Count(frame_, WebFeature::kServiceWorkerNavigationPreload);
  response_ = response;

  if (IsArchiveMIMEType(response_.MimeType()) &&
      resource->GetDataBufferingPolicy() != kBufferData)
    resource->SetDataBufferingPolicy(kBufferData);

  if (!ShouldContinueForResponse()) {
    probe::ContinueWithPolicyIgnore(frame_, this, resource->Identifier(),
                                    response_, resource);
    fetcher_->StopFetching();
    return;
  }

  if (frame_->Owner() && response_.IsHTTP() &&
      !CORS::IsOkStatus(response_.HttpStatusCode()))
    frame_->Owner()->RenderFallbackContent();
}

void DocumentLoader::CommitNavigation(const AtomicString& mime_type,
                                      const KURL& overriding_url) {
  if (state_ != kProvisional)
    return;

  // Set history state before commitProvisionalLoad() so that we still have
  // access to the previous committed DocumentLoader's HistoryItem, in case we
  // need to copy state from it.
  if (!GetFrameLoader().StateMachine()->CreatingInitialEmptyDocument()) {
    SetHistoryItemStateForCommit(
        GetFrameLoader().GetDocumentLoader()->GetHistoryItem(), load_type_,
        HistoryNavigationType::kDifferentDocument);
  }

  DCHECK_EQ(state_, kProvisional);
  GetFrameLoader().CommitProvisionalLoad();
  if (!frame_)
    return;

  const AtomicString& encoding = GetResponse().TextEncodingName();

  // Prepare a DocumentInit before clearing the frame, because it may need to
  // inherit an aliased security context.
  Document* owner_document = nullptr;
  // TODO(dcheng): This differs from the behavior of both IE and Firefox: the
  // origin is inherited from the document that loaded the URL.
  if (Document::ShouldInheritSecurityOriginFromOwner(Url())) {
    Frame* owner_frame = frame_->Tree().Parent();
    if (!owner_frame)
      owner_frame = frame_->Loader().Opener();
    if (owner_frame && owner_frame->IsLocalFrame())
      owner_document = ToLocalFrame(owner_frame)->GetDocument();
  }
  DCHECK(frame_->GetPage());

  ParserSynchronizationPolicy parsing_policy = kAllowAsynchronousParsing;
  if (!Document::ThreadedParsingEnabledForTesting())
    parsing_policy = kForceSynchronousParsing;

  InstallNewDocument(Url(), owner_document,
                     frame_->ShouldReuseDefaultView(Url())
                         ? WebGlobalObjectReusePolicy::kUseExisting
                         : WebGlobalObjectReusePolicy::kCreateNew,
                     mime_type, encoding, InstallNewDocumentReason::kNavigation,
                     parsing_policy, overriding_url);
  parser_->SetDocumentWasLoadedAsPartOfNavigation();
  if (request_.WasDiscarded())
    frame_->GetDocument()->SetWasDiscarded(true);
  frame_->GetDocument()->MaybeHandleHttpRefresh(
      response_.HttpHeaderField(HTTPNames::Refresh),
      Document::kHttpRefreshFromHeader);
}

void DocumentLoader::CommitData(const char* bytes, size_t length) {
  CommitNavigation(response_.MimeType());
  DCHECK_GE(state_, kCommitted);

  // This can happen if document.close() is called by an event handler while
  // there's still pending incoming data.
  if (!frame_ || !frame_->GetDocument()->Parsing())
    return;

  if (length)
    data_received_ = true;

  if (parser_blocked_count_) {
    if (!committed_data_buffer_)
      committed_data_buffer_ = SharedBuffer::Create();
    committed_data_buffer_->Append(bytes, length);
  } else {
    parser_->AppendBytes(bytes, length);
  }
}

void DocumentLoader::DataReceived(Resource* resource,
                                  const char* data,
                                  size_t length) {
  DCHECK(data);
  DCHECK(length);
  DCHECK_EQ(resource, GetResource());
  DCHECK(!response_.IsNull());
  DCHECK(!frame_->GetPage()->Paused());

  if (in_data_received_) {
    // If this function is reentered, defer processing of the additional data to
    // the top-level invocation. Reentrant calls can occur because of web
    // platform (mis-)features that require running a nested run loop:
    // - alert(), confirm(), prompt()
    // - Detach of plugin elements.
    // - Synchronous XMLHTTPRequest
    data_buffer_->Append(data, length);
    return;
  }

  base::AutoReset<bool> reentrancy_protector(&in_data_received_, true);
  ProcessData(data, length);
  ProcessDataBuffer();
}

void DocumentLoader::ProcessDataBuffer() {
  // Process data received in reentrant invocations. Note that the invocations
  // of processData() may queue more data in reentrant invocations, so iterate
  // until it's empty.
  const char* segment;
  size_t pos = 0;
  while (size_t length = data_buffer_->GetSomeData(segment, pos)) {
    ProcessData(segment, length);
    pos += length;
  }
  // All data has been consumed, so flush the buffer.
  data_buffer_->Clear();
}

void DocumentLoader::ProcessData(const char* data, size_t length) {
  application_cache_host_->MainResourceDataReceived(data, length);
  time_of_last_data_received_ = CurrentTimeTicks();

  if (IsArchiveMIMEType(GetResponse().MimeType()))
    return;
  CommitData(data, length);

  // If we are sending data to MediaDocument, we should stop here and cancel the
  // request.
  if (frame_ && frame_->GetDocument()->IsMediaDocument())
    fetcher_->StopFetching();
}

void DocumentLoader::ClearRedirectChain() {
  redirect_chain_.clear();
}

void DocumentLoader::AppendRedirect(const KURL& url) {
  redirect_chain_.push_back(url);
}

void DocumentLoader::StopLoading() {
  fetcher_->StopFetching();
  if (frame_ && !SentDidFinishLoad())
    LoadFailed(ResourceError::CancelledError(Url()));
}

void DocumentLoader::DetachFromFrame() {
  DCHECK(frame_);
  StopLoading();
  fetcher_->ClearContext();

  // If that load cancellation triggered another detach, leave.
  // (fast/frames/detach-frame-nested-no-crash.html is an example of this.)
  if (!frame_)
    return;

  application_cache_host_->DetachFromDocumentLoader();
  application_cache_host_.Clear();
  service_worker_network_provider_ = nullptr;
  WeakIdentifierMap<DocumentLoader>::NotifyObjectDestroyed(this);
  ClearResource();
  frame_ = nullptr;
}

bool DocumentLoader::MaybeCreateArchive() {
  // Give the archive machinery a crack at this document. If the MIME type is
  // not an archive type, it will return 0.
  if (!IsArchiveMIMEType(response_.MimeType()))
    return false;

  DCHECK(GetResource());
  ArchiveResource* main_resource = fetcher_->CreateArchive(GetResource());
  if (!main_resource)
    return false;
  // The origin is the MHTML file, we need to set the base URL to the document
  // encoded in the MHTML so relative URLs are resolved properly.
  CommitNavigation(main_resource->MimeType(), main_resource->Url());
  if (!frame_)
    return false;

  scoped_refptr<SharedBuffer> data(main_resource->Data());
  data->ForEachSegment(
      [this](const char* segment, size_t segment_size, size_t segment_offset) {
        CommitData(segment, segment_size);
        return true;
      });
  return true;
}

const KURL& DocumentLoader::UnreachableURL() const {
  return substitute_data_.FailingURL();
}

bool DocumentLoader::MaybeLoadEmpty() {
  bool should_load_empty = !substitute_data_.IsValid() &&
                           (request_.Url().IsEmpty() ||
                            SchemeRegistry::ShouldLoadURLSchemeAsEmptyDocument(
                                request_.Url().Protocol()));
  if (!should_load_empty)
    return false;

  if (request_.Url().IsEmpty() &&
      !GetFrameLoader().StateMachine()->CreatingInitialEmptyDocument())
    request_.SetURL(BlankURL());
  response_ = ResourceResponse(request_.Url(), "text/html");
  FinishedLoading(CurrentTimeTicks());
  return true;
}

void DocumentLoader::StartLoading() {
  GetTiming().MarkNavigationStart();
  DCHECK(!GetResource());
  DCHECK_EQ(state_, kNotStarted);
  state_ = kProvisional;

  if (MaybeLoadEmpty())
    return;

  DCHECK(!GetTiming().NavigationStart().is_null());
  // The fetch has already started in the browser,
  // so we don't MarkFetchStart here.

  ResourceLoaderOptions options;
  options.data_buffering_policy = kDoNotBufferData;
  options.initiator_info.name = FetchInitiatorTypeNames::document;
  FetchParameters fetch_params(request_, options);
  RawResource::FetchMainResource(fetch_params, Fetcher(), this,
                                 substitute_data_);
  // A bunch of headers are set when the underlying resource load begins, and
  // request_ needs to include those. Even when using a cached resource, we may
  // make some modification to the request, e.g. adding the referer header.
  request_ = GetResource()->IsLoading() ? GetResource()->GetResourceRequest()
                                        : fetch_params.GetResourceRequest();
}

void DocumentLoader::DidInstallNewDocument(Document* document) {
  document->SetReadyState(Document::kLoading);
  if (content_security_policy_) {
    document->InitContentSecurityPolicy(content_security_policy_.Release());
  }

  if (history_item_ && IsBackForwardLoadType(load_type_))
    document->SetStateForNewFormElements(history_item_->GetDocumentState());

  document->GetClientHintsPreferences().UpdateFrom(client_hints_preferences_);

  // TODO(japhet): There's no reason to wait until commit to set these bits.
  Settings* settings = document->GetSettings();
  fetcher_->SetImagesEnabled(settings->GetImagesEnabled());
  fetcher_->SetAutoLoadImages(settings->GetLoadsImagesAutomatically());

  const AtomicString& dns_prefetch_control =
      response_.HttpHeaderField(HTTPNames::X_DNS_Prefetch_Control);
  if (!dns_prefetch_control.IsEmpty())
    document->ParseDNSPrefetchControlHeader(dns_prefetch_control);

  String header_content_language =
      response_.HttpHeaderField(HTTPNames::Content_Language);
  if (!header_content_language.IsEmpty()) {
    size_t comma_index = header_content_language.find(',');
    // kNotFound == -1 == don't truncate
    header_content_language.Truncate(comma_index);
    header_content_language =
        header_content_language.StripWhiteSpace(IsHTMLSpace<UChar>);
    if (!header_content_language.IsEmpty())
      document->SetContentLanguage(AtomicString(header_content_language));
  }

  String referrer_policy_header =
      response_.HttpHeaderField(HTTPNames::Referrer_Policy);
  if (!referrer_policy_header.IsNull()) {
    UseCounter::Count(*document, WebFeature::kReferrerPolicyHeader);
    document->ParseAndSetReferrerPolicy(referrer_policy_header);
  }

  GetLocalFrameClient().DidCreateNewDocument();
}

void DocumentLoader::WillCommitNavigation() {
  if (GetFrameLoader().StateMachine()->CreatingInitialEmptyDocument())
    return;
  probe::willCommitLoad(frame_, this);
  frame_->GetIdlenessDetector()->WillCommitLoad();
}

void DocumentLoader::DidCommitNavigation(
    WebGlobalObjectReusePolicy global_object_reuse_policy) {
  if (GetFrameLoader().StateMachine()->CreatingInitialEmptyDocument())
    return;

  if (!frame_->Loader().StateMachine()->CommittedMultipleRealLoads() &&
      load_type_ == kFrameLoadTypeStandard) {
    frame_->Loader().StateMachine()->AdvanceTo(
        FrameLoaderStateMachine::kCommittedMultipleRealLoads);
  }

  HistoryCommitType commit_type = LoadTypeToCommitType(load_type_);
  frame_->GetFrameScheduler()->DidCommitProvisionalLoad(
      commit_type == kHistoryInertCommit, load_type_ == kFrameLoadTypeReload,
      frame_->IsLocalRoot());
  GetLocalFrameClient().DispatchDidCommitLoad(history_item_.Get(), commit_type,
                                              global_object_reuse_policy);

  // When the embedder gets notified (above) that the new navigation has
  // committed, the embedder will drop the old Content Security Policy and
  // therefore now is a good time to report to the embedder the Content
  // Security Policies that have accumulated so far for the new navigation.
  frame_->GetSecurityContext()
      ->GetContentSecurityPolicy()
      ->ReportAccumulatedHeaders(&GetLocalFrameClient());

  // didObserveLoadingBehavior() must be called after dispatchDidCommitLoad() is
  // called for the metrics tracking logic to handle it properly.
  if (service_worker_network_provider_ &&
      service_worker_network_provider_->HasControllerServiceWorker()) {
    GetLocalFrameClient().DidObserveLoadingBehavior(
        kWebLoadingBehaviorServiceWorkerControlled);
  }

  // Links with media values need more information (like viewport information).
  // This happens after the first chunk is parsed in HTMLDocumentParser.
  DispatchLinkHeaderPreloads(nullptr, LinkLoader::kOnlyLoadNonMedia);

  Document* document = frame_->GetDocument();
  InteractiveDetector* interactive_detector =
      InteractiveDetector::From(*document);
  if (interactive_detector)
    interactive_detector->SetNavigationStartTime(GetTiming().NavigationStart());

  TRACE_EVENT1("devtools.timeline", "CommitLoad", "data",
               InspectorCommitLoadEvent::Data(frame_));
  probe::didCommitLoad(frame_, this);
  frame_->GetPage()->DidCommitLoad(frame_);

  // Report legacy Symantec certificates after Page::DidCommitLoad, because the
  // latter clears the console.
  if (response_.IsLegacySymantecCert()) {
    GetLocalFrameClient().ReportLegacySymantecCert(response_.Url(),
                                                   false /* did_fail */);
  }
}

// static
bool DocumentLoader::ShouldClearWindowName(
    const LocalFrame& frame,
    const SecurityOrigin* previous_security_origin,
    const Document& new_document) {
  if (!previous_security_origin)
    return false;
  if (!frame.IsMainFrame())
    return false;
  if (frame.Loader().Opener())
    return false;

  return !new_document.GetSecurityOrigin()->IsSameSchemeHostPort(
      previous_security_origin);
}

void DocumentLoader::InstallNewDocument(
    const KURL& url,
    Document* owner_document,
    WebGlobalObjectReusePolicy global_object_reuse_policy,
    const AtomicString& mime_type,
    const AtomicString& encoding,
    InstallNewDocumentReason reason,
    ParserSynchronizationPolicy parsing_policy,
    const KURL& overriding_url) {
  DCHECK(!frame_->GetDocument() || !frame_->GetDocument()->IsActive());
  DCHECK_EQ(frame_->Tree().ChildCount(), 0u);
  if (GetFrameLoader().StateMachine()->IsDisplayingInitialEmptyDocument()) {
    GetFrameLoader().StateMachine()->AdvanceTo(
        FrameLoaderStateMachine::kCommittedFirstRealLoad);
  }

  const SecurityOrigin* previous_security_origin = nullptr;
  if (frame_->GetDocument())
    previous_security_origin = frame_->GetDocument()->GetSecurityOrigin();

  // In some rare cases, we'll re-use a LocalDOMWindow for a new Document. For
  // example, when a script calls window.open("..."), the browser gives
  // JavaScript a window synchronously but kicks off the load in the window
  // asynchronously. Web sites expect that modifications that they make to the
  // window object synchronously won't be blown away when the network load
  // commits. To make that happen, we "securely transition" the existing
  // LocalDOMWindow to the Document that results from the network load. See also
  // Document::IsSecureTransitionTo.
  if (global_object_reuse_policy != WebGlobalObjectReusePolicy::kUseExisting)
    frame_->SetDOMWindow(LocalDOMWindow::Create(*frame_));

  if (reason == InstallNewDocumentReason::kNavigation)
    WillCommitNavigation();

  Document* document = frame_->DomWindow()->InstallNewDocument(
      mime_type,
      DocumentInit::Create()
          .WithFrame(frame_)
          .WithURL(url)
          .WithOwnerDocument(owner_document)
          .WithNewRegistrationContext(),
      false);

  // Clear the user activation state.
  // TODO(crbug.com/736415): Clear this bit unconditionally for all frames.
  if (frame_->IsMainFrame())
    frame_->ClearActivation();

  // The DocumentLoader was flagged as activated if it needs to notify the frame
  // that it was activated before navigation. Update the frame state based on
  // the new value.
  if (frame_->HasReceivedUserGestureBeforeNavigation() != user_activated_) {
    frame_->SetDocumentHasReceivedUserGestureBeforeNavigation(user_activated_);
    GetLocalFrameClient().SetHasReceivedUserGestureBeforeNavigation(
        user_activated_);
  }

  if (ShouldClearWindowName(*frame_, previous_security_origin, *document)) {
    // TODO(andypaicu): experimentalSetNullName will just record the fact
    // that the name would be nulled and if the name is accessed after we will
    // fire a UseCounter. If we decide to move forward with this change, we'd
    // actually clean the name here.
    // frame_->tree().setName(g_null_atom);
    frame_->Tree().ExperimentalSetNulledName();
  }

  if (!overriding_url.IsEmpty())
    document->SetBaseURLOverride(overriding_url);
  DidInstallNewDocument(document);

  // This must be called before the document is opened, otherwise HTML parser
  // will use stale values from HTMLParserOption.
  if (reason == InstallNewDocumentReason::kNavigation)
    DidCommitNavigation(global_object_reuse_policy);

  // Initializing origin trials might force window proxy initialization,
  // which later triggers CHECK when swapping in via WebFrame::Swap().
  // We can safely omit installing original trials on initial empty document
  // and wait for the real load.
  if (GetFrameLoader().StateMachine()->CommittedFirstRealDocumentLoad()) {
    if (document->GetSettings()
            ->GetForceTouchEventFeatureDetectionForInspector()) {
      OriginTrialContext::FromOrCreate(document)->AddFeature(
          "ForceTouchEventFeatureDetectionForInspector");
    }
    OriginTrialContext::AddTokensFromHeader(
        document, response_.HttpHeaderField(HTTPNames::Origin_Trial));
  }

  parser_ = document->OpenForNavigation(parsing_policy, mime_type, encoding);

  // If this is a scriptable parser and there is a resource, register the
  // resource's cache handler with the parser.
  ScriptableDocumentParser* scriptable_parser =
      parser_->AsScriptableDocumentParser();
  if (scriptable_parser && GetResource()) {
    scriptable_parser->SetInlineScriptCacheHandler(
        ToRawResource(GetResource())->CacheHandler());
  }

  // FeaturePolicy is reset in the browser process on commit, so this needs to
  // be initialized and replicated to the browser process after commit messages
  // are sent in didCommitNavigation().
  document->ApplyFeaturePolicyFromHeader(
      response_.HttpHeaderField(HTTPNames::Feature_Policy));

  GetFrameLoader().DispatchDidClearDocumentOfWindowObject();
}

const AtomicString& DocumentLoader::MimeType() const {
  if (fetcher_->Archive())
    return fetcher_->Archive()->MainResource()->MimeType();
  return response_.MimeType();
}

// This is only called by
// FrameLoader::ReplaceDocumentWhileExecutingJavaScriptURL()
void DocumentLoader::ReplaceDocumentWhileExecutingJavaScriptURL(
    const KURL& url,
    Document* owner_document,
    WebGlobalObjectReusePolicy global_object_reuse_policy,
    const String& source) {
  InstallNewDocument(url, owner_document, global_object_reuse_policy,
                     MimeType(), response_.TextEncodingName(),
                     InstallNewDocumentReason::kJavascriptURL,
                     kForceSynchronousParsing, NullURL());

  if (!source.IsNull()) {
    frame_->GetDocument()->SetCompatibilityMode(Document::kNoQuirksMode);
    parser_->Append(source);
  }

  // Append() might lead to a detach.
  if (parser_)
    parser_->Finish();
}

void DocumentLoader::BlockParser() {
  parser_blocked_count_++;
}

void DocumentLoader::ResumeParser() {
  parser_blocked_count_--;
  DCHECK_GE(parser_blocked_count_, 0);

  if (parser_blocked_count_ != 0)
    return;

  if (committed_data_buffer_ && !committed_data_buffer_->IsEmpty()) {
    // Don't recursively process data.
    base::AutoReset<bool> reentrancy_protector(&in_data_received_, true);

    // Append data to the parser that may have been received while the parser
    // was blocked.
    const char* segment;
    size_t pos = 0;
    while (size_t length = committed_data_buffer_->GetSomeData(segment, pos)) {
      parser_->AppendBytes(segment, length);
      pos += length;
    }
    committed_data_buffer_->Clear();

    // DataReceived may be called in a nested message loop.
    ProcessDataBuffer();
  }

  if (finished_loading_) {
    finished_loading_ = false;
    parser_->Finish();
    parser_.Clear();
  }
}

DEFINE_WEAK_IDENTIFIER_MAP(DocumentLoader);

STATIC_ASSERT_ENUM(kWebStandardCommit, kStandardCommit);
STATIC_ASSERT_ENUM(kWebBackForwardCommit, kBackForwardCommit);
STATIC_ASSERT_ENUM(kWebInitialCommitInChildFrame, kInitialCommitInChildFrame);
STATIC_ASSERT_ENUM(kWebHistoryInertCommit, kHistoryInertCommit);

}  // namespace blink
