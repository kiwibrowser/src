/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/inspector/inspector_page_agent.h"

#include <memory>

#include "build/build_config.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/script_regexp.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_timing.h"
#include "third_party/blink/renderer/core/dom/dom_implementation.h"
#include "third_party/blink/renderer/core/dom/user_gesture_indicator.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/imports/html_import_loader.h"
#include "third_party/blink/renderer/core/html/imports/html_imports_controller.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/inspector/inspector_css_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_resource_content_loader.h"
#include "third_party/blink/renderer/core/inspector/v8_inspector_string.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/idleness_detector.h"
#include "third_party/blink/renderer/core/loader/resource/css_style_sheet_resource.h"
#include "third_party/blink/renderer/core/loader/resource/script_resource.h"
#include "third_party/blink/renderer/core/loader/scheduled_navigation.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/text_resource_decoder_options.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "v8/include/v8-inspector.h"

namespace blink {

using protocol::Response;

namespace PageAgentState {
static const char kPageAgentEnabled[] = "pageAgentEnabled";
static const char kPageAgentScriptsToEvaluateOnLoad[] =
    "pageAgentScriptsToEvaluateOnLoad";
static const char kScreencastEnabled[] = "screencastEnabled";
static const char kLifecycleEventsEnabled[] = "lifecycleEventsEnabled";
static const char kBypassCSPEnabled[] = "bypassCSPEnabled";
}  // namespace PageAgentState

namespace {

String ScheduledNavigationReasonToProtocol(ScheduledNavigation::Reason reason) {
  using ReasonEnum =
      protocol::Page::FrameScheduledNavigationNotification::ReasonEnum;
  switch (reason) {
    case ScheduledNavigation::Reason::kFormSubmissionGet:
      return ReasonEnum::FormSubmissionGet;
    case ScheduledNavigation::Reason::kFormSubmissionPost:
      return ReasonEnum::FormSubmissionPost;
    case ScheduledNavigation::Reason::kHttpHeaderRefresh:
      return ReasonEnum::HttpHeaderRefresh;
    case ScheduledNavigation::Reason::kFrameNavigation:
      return ReasonEnum::ScriptInitiated;
    case ScheduledNavigation::Reason::kMetaTagRefresh:
      return ReasonEnum::MetaTagRefresh;
    case ScheduledNavigation::Reason::kPageBlock:
      return ReasonEnum::PageBlockInterstitial;
    case ScheduledNavigation::Reason::kReload:
      return ReasonEnum::Reload;
    default:
      NOTREACHED();
  }
  return ReasonEnum::Reload;
}

Resource* CachedResource(LocalFrame* frame,
                         const KURL& url,
                         InspectorResourceContentLoader* loader) {
  Document* document = frame->GetDocument();
  if (!document)
    return nullptr;
  Resource* cached_resource = document->Fetcher()->CachedResource(url);
  if (!cached_resource) {
    HeapVector<Member<Document>> all_imports =
        InspectorPageAgent::ImportsForFrame(frame);
    for (Document* import : all_imports) {
      cached_resource = import->Fetcher()->CachedResource(url);
      if (cached_resource)
        break;
    }
  }
  if (!cached_resource) {
    cached_resource = GetMemoryCache()->ResourceForURL(
        url, document->Fetcher()->GetCacheIdentifier());
  }
  if (!cached_resource)
    cached_resource = loader->ResourceForURL(url);
  return cached_resource;
}

std::unique_ptr<protocol::Array<String>> GetEnabledWindowFeatures(
    const WebWindowFeatures& window_features) {
  std::unique_ptr<protocol::Array<String>> feature_strings =
      protocol::Array<String>::create();
  if (window_features.x_set) {
    feature_strings->addItem(
        String::Format("left=%d", static_cast<int>(window_features.x)));
  }
  if (window_features.y_set) {
    feature_strings->addItem(
        String::Format("top=%d", static_cast<int>(window_features.y)));
  }
  if (window_features.width_set) {
    feature_strings->addItem(
        String::Format("width=%d", static_cast<int>(window_features.width)));
  }
  if (window_features.height_set) {
    feature_strings->addItem(
        String::Format("height=%d", static_cast<int>(window_features.height)));
  }
  if (window_features.menu_bar_visible)
    feature_strings->addItem("menubar");
  if (window_features.tool_bar_visible)
    feature_strings->addItem("toolbar");
  if (window_features.status_bar_visible)
    feature_strings->addItem("status");
  if (window_features.scrollbars_visible)
    feature_strings->addItem("scrollbars");
  if (window_features.resizable)
    feature_strings->addItem("resizable");
  if (window_features.noopener)
    feature_strings->addItem("noopener");
  if (window_features.background)
    feature_strings->addItem("background");
  if (window_features.persistent)
    feature_strings->addItem("persistent");
  return feature_strings;
}

}  // namespace

static bool PrepareResourceBuffer(Resource* cached_resource,
                                  bool* has_zero_size) {
  if (!cached_resource)
    return false;

  if (cached_resource->GetDataBufferingPolicy() == kDoNotBufferData)
    return false;

  // Zero-sized resources don't have data at all -- so fake the empty buffer,
  // instead of indicating error by returning 0.
  if (!cached_resource->EncodedSize()) {
    *has_zero_size = true;
    return true;
  }

  *has_zero_size = false;
  return true;
}

static bool HasTextContent(Resource* cached_resource) {
  Resource::Type type = cached_resource->GetType();
  return type == Resource::kCSSStyleSheet || type == Resource::kXSLStyleSheet ||
         type == Resource::kScript || type == Resource::kRaw ||
         type == Resource::kImportResource || type == Resource::kMainResource;
}

static std::unique_ptr<TextResourceDecoder> CreateResourceTextDecoder(
    const String& mime_type,
    const String& text_encoding_name) {
  if (!text_encoding_name.IsEmpty()) {
    return TextResourceDecoder::Create(TextResourceDecoderOptions(
        TextResourceDecoderOptions::kPlainTextContent,
        WTF::TextEncoding(text_encoding_name)));
  }
  if (DOMImplementation::IsXMLMIMEType(mime_type)) {
    TextResourceDecoderOptions options(TextResourceDecoderOptions::kXMLContent);
    options.SetUseLenientXMLDecoding();
    return TextResourceDecoder::Create(options);
  }
  if (DeprecatedEqualIgnoringCase(mime_type, "text/html")) {
    return TextResourceDecoder::Create(TextResourceDecoderOptions(
        TextResourceDecoderOptions::kHTMLContent, UTF8Encoding()));
  }
  if (MIMETypeRegistry::IsSupportedJavaScriptMIMEType(mime_type) ||
      DOMImplementation::IsJSONMIMEType(mime_type)) {
    return TextResourceDecoder::Create(TextResourceDecoderOptions(
        TextResourceDecoderOptions::kPlainTextContent, UTF8Encoding()));
  }
  if (DOMImplementation::IsTextMIMEType(mime_type)) {
    return TextResourceDecoder::Create(TextResourceDecoderOptions(
        TextResourceDecoderOptions::kPlainTextContent,
        WTF::TextEncoding("ISO-8859-1")));
  }
  return std::unique_ptr<TextResourceDecoder>();
}

static void MaybeEncodeTextContent(const String& text_content,
                                   const char* buffer_data,
                                   size_t buffer_size,
                                   String* result,
                                   bool* base64_encoded) {
  if (!text_content.IsNull() &&
      !text_content.Utf8(WTF::kStrictUTF8Conversion).IsNull()) {
    *result = text_content;
    *base64_encoded = false;
  } else if (buffer_data) {
    *result = Base64Encode(buffer_data, buffer_size);
    *base64_encoded = true;
  } else if (text_content.IsNull()) {
    *result = "";
    *base64_encoded = false;
  } else {
    DCHECK(!text_content.Is8Bit());
    *result = Base64Encode(text_content.Utf8(WTF::kLenientUTF8Conversion));
    *base64_encoded = true;
  }
}

static void MaybeEncodeTextContent(const String& text_content,
                                   scoped_refptr<const SharedBuffer> buffer,
                                   String* result,
                                   bool* base64_encoded) {
  if (!buffer) {
    return MaybeEncodeTextContent(text_content, nullptr, 0, result,
                                  base64_encoded);
  }

  const SharedBuffer::DeprecatedFlatData flat_buffer(std::move(buffer));
  return MaybeEncodeTextContent(text_content, flat_buffer.Data(),
                                flat_buffer.size(), result, base64_encoded);
}

// static
KURL InspectorPageAgent::UrlWithoutFragment(const KURL& url) {
  KURL result = url;
  result.RemoveFragmentIdentifier();
  return result;
}

// static
bool InspectorPageAgent::SharedBufferContent(
    scoped_refptr<const SharedBuffer> buffer,
    const String& mime_type,
    const String& text_encoding_name,
    String* result,
    bool* base64_encoded) {
  if (!buffer)
    return false;

  String text_content;
  std::unique_ptr<TextResourceDecoder> decoder =
      CreateResourceTextDecoder(mime_type, text_encoding_name);
  WTF::TextEncoding encoding(text_encoding_name);

  const SharedBuffer::DeprecatedFlatData flat_buffer(std::move(buffer));
  if (decoder) {
    text_content = decoder->Decode(flat_buffer.Data(), flat_buffer.size());
    text_content = text_content + decoder->Flush();
  } else if (encoding.IsValid()) {
    text_content = encoding.Decode(flat_buffer.Data(), flat_buffer.size());
  }

  MaybeEncodeTextContent(text_content, flat_buffer.Data(), flat_buffer.size(),
                         result, base64_encoded);
  return true;
}

// static
bool InspectorPageAgent::CachedResourceContent(Resource* cached_resource,
                                               String* result,
                                               bool* base64_encoded) {
  bool has_zero_size;
  if (!PrepareResourceBuffer(cached_resource, &has_zero_size))
    return false;

  if (!HasTextContent(cached_resource)) {
    scoped_refptr<const SharedBuffer> buffer =
        has_zero_size ? SharedBuffer::Create()
                      : cached_resource->ResourceBuffer();
    if (!buffer)
      return false;

    const SharedBuffer::DeprecatedFlatData flat_buffer(std::move(buffer));
    *result = Base64Encode(flat_buffer.Data(), flat_buffer.size());
    *base64_encoded = true;
    return true;
  }

  if (has_zero_size) {
    *result = "";
    *base64_encoded = false;
    return true;
  }

  DCHECK(cached_resource);
  switch (cached_resource->GetType()) {
    case Resource::kCSSStyleSheet:
      MaybeEncodeTextContent(
          ToCSSStyleSheetResource(cached_resource)
              ->SheetText(nullptr, CSSStyleSheetResource::MIMETypeCheck::kLax),
          cached_resource->ResourceBuffer(), result, base64_encoded);
      return true;
    case Resource::kScript:
      MaybeEncodeTextContent(
          cached_resource->ResourceBuffer()
              ? ToScriptResource(cached_resource)->DecodedText()
              : ToScriptResource(cached_resource)->SourceText(),
          cached_resource->ResourceBuffer(), result, base64_encoded);
      return true;
    default:
      String text_encoding_name =
          cached_resource->GetResponse().TextEncodingName();
      if (text_encoding_name.IsEmpty() &&
          cached_resource->GetType() != Resource::kRaw)
        text_encoding_name = "WinLatin1";
      return InspectorPageAgent::SharedBufferContent(
          cached_resource->ResourceBuffer(),
          cached_resource->GetResponse().MimeType(), text_encoding_name, result,
          base64_encoded);
  }
}

InspectorPageAgent* InspectorPageAgent::Create(
    InspectedFrames* inspected_frames,
    Client* client,
    InspectorResourceContentLoader* resource_content_loader,
    v8_inspector::V8InspectorSession* v8_session) {
  return new InspectorPageAgent(inspected_frames, client,
                                resource_content_loader, v8_session);
}

String InspectorPageAgent::ResourceTypeJson(
    InspectorPageAgent::ResourceType resource_type) {
  switch (resource_type) {
    case kDocumentResource:
      return protocol::Page::ResourceTypeEnum::Document;
    case kFontResource:
      return protocol::Page::ResourceTypeEnum::Font;
    case kImageResource:
      return protocol::Page::ResourceTypeEnum::Image;
    case kMediaResource:
      return protocol::Page::ResourceTypeEnum::Media;
    case kScriptResource:
      return protocol::Page::ResourceTypeEnum::Script;
    case kStylesheetResource:
      return protocol::Page::ResourceTypeEnum::Stylesheet;
    case kTextTrackResource:
      return protocol::Page::ResourceTypeEnum::TextTrack;
    case kXHRResource:
      return protocol::Page::ResourceTypeEnum::XHR;
    case kFetchResource:
      return protocol::Page::ResourceTypeEnum::Fetch;
    case kEventSourceResource:
      return protocol::Page::ResourceTypeEnum::EventSource;
    case kWebSocketResource:
      return protocol::Page::ResourceTypeEnum::WebSocket;
    case kManifestResource:
      return protocol::Page::ResourceTypeEnum::Manifest;
    case kSignedExchangeResource:
      return protocol::Page::ResourceTypeEnum::SignedExchange;
    case kOtherResource:
      return protocol::Page::ResourceTypeEnum::Other;
  }
  return protocol::Page::ResourceTypeEnum::Other;
}

InspectorPageAgent::ResourceType InspectorPageAgent::ToResourceType(
    const Resource::Type resource_type) {
  switch (resource_type) {
    case Resource::kImage:
      return InspectorPageAgent::kImageResource;
    case Resource::kFont:
      return InspectorPageAgent::kFontResource;
    case Resource::kAudio:
    case Resource::kVideo:
      return InspectorPageAgent::kMediaResource;
    case Resource::kManifest:
      return InspectorPageAgent::kManifestResource;
    case Resource::kTextTrack:
      return InspectorPageAgent::kTextTrackResource;
    case Resource::kCSSStyleSheet:
    // Fall through.
    case Resource::kXSLStyleSheet:
      return InspectorPageAgent::kStylesheetResource;
    case Resource::kScript:
      return InspectorPageAgent::kScriptResource;
    case Resource::kImportResource:
    // Fall through.
    case Resource::kMainResource:
      return InspectorPageAgent::kDocumentResource;
    default:
      break;
  }
  return InspectorPageAgent::kOtherResource;
}

String InspectorPageAgent::CachedResourceTypeJson(
    const Resource& cached_resource) {
  return ResourceTypeJson(ToResourceType(cached_resource.GetType()));
}

InspectorPageAgent::InspectorPageAgent(
    InspectedFrames* inspected_frames,
    Client* client,
    InspectorResourceContentLoader* resource_content_loader,
    v8_inspector::V8InspectorSession* v8_session)
    : inspected_frames_(inspected_frames),
      v8_session_(v8_session),
      client_(client),
      last_script_identifier_(0),
      enabled_(false),
      reloading_(false),
      inspector_resource_content_loader_(resource_content_loader),
      resource_content_loader_client_id_(
          resource_content_loader->CreateClientId()) {}

void InspectorPageAgent::Restore() {
  if (state_->booleanProperty(PageAgentState::kPageAgentEnabled, false))
    enable();
  if (state_->booleanProperty(PageAgentState::kBypassCSPEnabled, false))
    setBypassCSP(true);
}

Response InspectorPageAgent::enable() {
  enabled_ = true;
  state_->setBoolean(PageAgentState::kPageAgentEnabled, true);
  instrumenting_agents_->addInspectorPageAgent(this);
  return Response::OK();
}

Response InspectorPageAgent::disable() {
  enabled_ = false;
  state_->setBoolean(PageAgentState::kPageAgentEnabled, false);
  state_->remove(PageAgentState::kPageAgentScriptsToEvaluateOnLoad);
  script_to_evaluate_on_load_once_ = String();
  pending_script_to_evaluate_on_load_once_ = String();
  instrumenting_agents_->removeInspectorPageAgent(this);
  inspector_resource_content_loader_->Cancel(
      resource_content_loader_client_id_);

  stopScreencast();

  FinishReload();
  return Response::OK();
}

Response InspectorPageAgent::addScriptToEvaluateOnLoad(const String& source,
                                                       String* identifier) {
  protocol::DictionaryValue* scripts =
      state_->getObject(PageAgentState::kPageAgentScriptsToEvaluateOnLoad);
  if (!scripts) {
    std::unique_ptr<protocol::DictionaryValue> new_scripts =
        protocol::DictionaryValue::create();
    scripts = new_scripts.get();
    state_->setObject(PageAgentState::kPageAgentScriptsToEvaluateOnLoad,
                      std::move(new_scripts));
  }
  // Assure we don't override existing ids -- m_lastScriptIdentifier could get
  // out of sync WRT actual scripts once we restored the scripts from the cookie
  // during navigation.
  do {
    *identifier = String::Number(++last_script_identifier_);
  } while (scripts->get(*identifier));
  scripts->setString(*identifier, source);
  return Response::OK();
}

Response InspectorPageAgent::removeScriptToEvaluateOnLoad(
    const String& identifier) {
  protocol::DictionaryValue* scripts =
      state_->getObject(PageAgentState::kPageAgentScriptsToEvaluateOnLoad);
  if (!scripts || !scripts->get(identifier))
    return Response::Error("Script not found");
  scripts->remove(identifier);
  return Response::OK();
}

Response InspectorPageAgent::addScriptToEvaluateOnNewDocument(
    const String& source,
    String* identifier) {
  return addScriptToEvaluateOnLoad(source, identifier);
}

Response InspectorPageAgent::removeScriptToEvaluateOnNewDocument(
    const String& identifier) {
  return removeScriptToEvaluateOnLoad(identifier);
}

Response InspectorPageAgent::setLifecycleEventsEnabled(bool enabled) {
  state_->setBoolean(PageAgentState::kLifecycleEventsEnabled, enabled);
  if (!enabled)
    return Response::OK();

  for (LocalFrame* frame : *inspected_frames_) {
    Document* document = frame->GetDocument();
    DocumentLoader* loader = frame->Loader().GetDocumentLoader();
    if (!document || !loader)
      continue;

    DocumentLoadTiming& timing = loader->GetTiming();
    TimeTicks commit_timestamp = timing.ResponseEnd();
    if (!commit_timestamp.is_null()) {
      LifecycleEvent(frame, loader, "commit",
                     TimeTicksInSeconds(commit_timestamp));
    }

    TimeTicks domcontentloaded_timestamp =
        document->GetTiming().DomContentLoadedEventEnd();
    if (!domcontentloaded_timestamp.is_null()) {
      LifecycleEvent(frame, loader, "DOMContentLoaded",
                     TimeTicksInSeconds(domcontentloaded_timestamp));
    }

    TimeTicks load_timestamp = timing.LoadEventEnd();
    if (!load_timestamp.is_null()) {
      LifecycleEvent(frame, loader, "load", TimeTicksInSeconds(load_timestamp));
    }

    IdlenessDetector* idleness_detector = frame->GetIdlenessDetector();
    TimeTicks network_almost_idle_timestamp =
        idleness_detector->GetNetworkAlmostIdleTime();
    if (!network_almost_idle_timestamp.is_null()) {
      LifecycleEvent(frame, loader, "networkAlmostIdle",
                     TimeTicksInSeconds(network_almost_idle_timestamp));
    }
    TimeTicks network_idle_timestamp = idleness_detector->GetNetworkIdleTime();
    if (!network_idle_timestamp.is_null()) {
      LifecycleEvent(frame, loader, "networkIdle",
                     TimeTicksInSeconds(network_idle_timestamp));
    }
  }

  return Response::OK();
}

Response InspectorPageAgent::setAdBlockingEnabled(bool enable) {
  return Response::OK();
}

Response InspectorPageAgent::reload(
    Maybe<bool> optional_bypass_cache,
    Maybe<String> optional_script_to_evaluate_on_load) {
  pending_script_to_evaluate_on_load_once_ =
      optional_script_to_evaluate_on_load.fromMaybe("");
  v8_session_->setSkipAllPauses(true);
  reloading_ = true;
  inspected_frames_->Root()->Reload(optional_bypass_cache.fromMaybe(false)
                                        ? kFrameLoadTypeReloadBypassingCache
                                        : kFrameLoadTypeReload,
                                    ClientRedirectPolicy::kNotClientRedirect);
  return Response::OK();
}

Response InspectorPageAgent::stopLoading() {
  return Response::OK();
}

static void CachedResourcesForDocument(Document* document,
                                       HeapVector<Member<Resource>>& result,
                                       bool skip_xhrs) {
  const ResourceFetcher::DocumentResourceMap& all_resources =
      document->Fetcher()->AllResources();
  for (const auto& resource : all_resources) {
    Resource* cached_resource = resource.value.Get();
    if (!cached_resource)
      continue;

    // Skip images that were not auto loaded (images disabled in the user
    // agent), fonts that were referenced in CSS but never used/downloaded, etc.
    if (cached_resource->StillNeedsLoad())
      continue;
    if (cached_resource->GetType() == Resource::kRaw && skip_xhrs)
      continue;
    result.push_back(cached_resource);
  }
}

// static
HeapVector<Member<Document>> InspectorPageAgent::ImportsForFrame(
    LocalFrame* frame) {
  HeapVector<Member<Document>> result;
  Document* root_document = frame->GetDocument();

  if (HTMLImportsController* controller = root_document->ImportsController()) {
    for (size_t i = 0; i < controller->LoaderCount(); ++i) {
      if (Document* document = controller->LoaderAt(i)->GetDocument())
        result.push_back(document);
    }
  }

  return result;
}

static HeapVector<Member<Resource>> CachedResourcesForFrame(LocalFrame* frame,
                                                            bool skip_xhrs) {
  HeapVector<Member<Resource>> result;
  Document* root_document = frame->GetDocument();
  HeapVector<Member<Document>> loaders =
      InspectorPageAgent::ImportsForFrame(frame);

  CachedResourcesForDocument(root_document, result, skip_xhrs);
  for (size_t i = 0; i < loaders.size(); ++i)
    CachedResourcesForDocument(loaders[i], result, skip_xhrs);

  return result;
}

Response InspectorPageAgent::getResourceTree(
    std::unique_ptr<protocol::Page::FrameResourceTree>* object) {
  *object = BuildObjectForResourceTree(inspected_frames_->Root());
  return Response::OK();
}

Response InspectorPageAgent::getFrameTree(
    std::unique_ptr<protocol::Page::FrameTree>* object) {
  *object = BuildObjectForFrameTree(inspected_frames_->Root());
  return Response::OK();
}

void InspectorPageAgent::FinishReload() {
  if (!reloading_)
    return;
  reloading_ = false;
  v8_session_->setSkipAllPauses(false);
}

void InspectorPageAgent::GetResourceContentAfterResourcesContentLoaded(
    const String& frame_id,
    const String& url,
    std::unique_ptr<GetResourceContentCallback> callback) {
  LocalFrame* frame =
      IdentifiersFactory::FrameById(inspected_frames_, frame_id);
  if (!frame) {
    callback->sendFailure(Response::Error("No frame for given id found"));
    return;
  }
  String content;
  bool base64_encoded;
  if (InspectorPageAgent::CachedResourceContent(
          CachedResource(frame, KURL(url), inspector_resource_content_loader_),
          &content, &base64_encoded))
    callback->sendSuccess(content, base64_encoded);
  else
    callback->sendFailure(Response::Error("No resource with given URL found"));
}

void InspectorPageAgent::getResourceContent(
    const String& frame_id,
    const String& url,
    std::unique_ptr<GetResourceContentCallback> callback) {
  if (!enabled_) {
    callback->sendFailure(Response::Error("Agent is not enabled."));
    return;
  }
  inspector_resource_content_loader_->EnsureResourcesContentLoaded(
      resource_content_loader_client_id_,
      WTF::Bind(
          &InspectorPageAgent::GetResourceContentAfterResourcesContentLoaded,
          WrapPersistent(this), frame_id, url,
          WTF::Passed(std::move(callback))));
}

void InspectorPageAgent::SearchContentAfterResourcesContentLoaded(
    const String& frame_id,
    const String& url,
    const String& query,
    bool case_sensitive,
    bool is_regex,
    std::unique_ptr<SearchInResourceCallback> callback) {
  LocalFrame* frame =
      IdentifiersFactory::FrameById(inspected_frames_, frame_id);
  if (!frame) {
    callback->sendFailure(Response::Error("No frame for given id found"));
    return;
  }
  String content;
  bool base64_encoded;
  if (!InspectorPageAgent::CachedResourceContent(
          CachedResource(frame, KURL(url), inspector_resource_content_loader_),
          &content, &base64_encoded)) {
    callback->sendFailure(Response::Error("No resource with given URL found"));
    return;
  }

  auto matches = v8_session_->searchInTextByLines(
      ToV8InspectorStringView(content), ToV8InspectorStringView(query),
      case_sensitive, is_regex);
  auto results = protocol::Array<
      v8_inspector::protocol::Debugger::API::SearchMatch>::create();
  for (size_t i = 0; i < matches.size(); ++i)
    results->addItem(std::move(matches[i]));
  callback->sendSuccess(std::move(results));
}

void InspectorPageAgent::searchInResource(
    const String& frame_id,
    const String& url,
    const String& query,
    Maybe<bool> optional_case_sensitive,
    Maybe<bool> optional_is_regex,
    std::unique_ptr<SearchInResourceCallback> callback) {
  if (!enabled_) {
    callback->sendFailure(Response::Error("Agent is not enabled."));
    return;
  }
  inspector_resource_content_loader_->EnsureResourcesContentLoaded(
      resource_content_loader_client_id_,
      WTF::Bind(&InspectorPageAgent::SearchContentAfterResourcesContentLoaded,
                WrapPersistent(this), frame_id, url, query,
                optional_case_sensitive.fromMaybe(false),
                optional_is_regex.fromMaybe(false),
                WTF::Passed(std::move(callback))));
}

Response InspectorPageAgent::setBypassCSP(bool enabled) {
  LocalFrame* frame = inspected_frames_->Root();
  frame->GetSettings()->SetBypassCSP(enabled);
  state_->setBoolean(PageAgentState::kBypassCSPEnabled, enabled);
  return Response::OK();
}

Response InspectorPageAgent::setDocumentContent(const String& frame_id,
                                                const String& html) {
  LocalFrame* frame =
      IdentifiersFactory::FrameById(inspected_frames_, frame_id);
  if (!frame)
    return Response::Error("No frame for given id found");

  Document* document = frame->GetDocument();
  if (!document)
    return Response::Error("No Document instance to set HTML for");
  document->SetContent(html);
  return Response::OK();
}

void InspectorPageAgent::DidNavigateWithinDocument(LocalFrame* frame) {
  Document* document = frame->GetDocument();
  if (document) {
    return GetFrontend()->navigatedWithinDocument(
        IdentifiersFactory::FrameId(frame), document->Url());
  }
}

void InspectorPageAgent::DidClearDocumentOfWindowObject(LocalFrame* frame) {
  if (!GetFrontend())
    return;

  protocol::DictionaryValue* scripts =
      state_->getObject(PageAgentState::kPageAgentScriptsToEvaluateOnLoad);
  if (scripts) {
    for (size_t i = 0; i < scripts->size(); ++i) {
      auto script = scripts->at(i);
      String script_text;
      if (script.second->asString(&script_text))
        frame->GetScriptController().ExecuteScriptInMainWorld(script_text);
    }
  }
  if (!script_to_evaluate_on_load_once_.IsEmpty()) {
    frame->GetScriptController().ExecuteScriptInMainWorld(
        script_to_evaluate_on_load_once_);
  }
}

void InspectorPageAgent::DomContentLoadedEventFired(LocalFrame* frame) {
  double timestamp = CurrentTimeTicksInSeconds();
  if (frame == inspected_frames_->Root())
    GetFrontend()->domContentEventFired(timestamp);
  DocumentLoader* loader = frame->Loader().GetDocumentLoader();
  LifecycleEvent(frame, loader, "DOMContentLoaded", timestamp);
}

void InspectorPageAgent::LoadEventFired(LocalFrame* frame) {
  double timestamp = CurrentTimeTicksInSeconds();
  if (frame == inspected_frames_->Root())
    GetFrontend()->loadEventFired(timestamp);
  DocumentLoader* loader = frame->Loader().GetDocumentLoader();
  LifecycleEvent(frame, loader, "load", timestamp);
}

void InspectorPageAgent::WillCommitLoad(LocalFrame*, DocumentLoader* loader) {
  if (loader->GetFrame() == inspected_frames_->Root()) {
    FinishReload();
    script_to_evaluate_on_load_once_ = pending_script_to_evaluate_on_load_once_;
    pending_script_to_evaluate_on_load_once_ = String();
  }
  GetFrontend()->frameNavigated(BuildObjectForFrame(loader->GetFrame()));
}

void InspectorPageAgent::FrameAttachedToParent(LocalFrame* frame) {
  Frame* parent_frame = frame->Tree().Parent();
  std::unique_ptr<SourceLocation> location =
      SourceLocation::CaptureWithFullStackTrace();
  GetFrontend()->frameAttached(
      IdentifiersFactory::FrameId(frame),
      IdentifiersFactory::FrameId(parent_frame),
      location ? location->BuildInspectorObject() : nullptr);
  // Some network events referencing this frame will be reported from the
  // browser, so make sure to deliver FrameAttached without buffering,
  // so it gets to the front-end first.
  GetFrontend()->flush();
}

void InspectorPageAgent::FrameDetachedFromParent(LocalFrame* frame) {
  GetFrontend()->frameDetached(IdentifiersFactory::FrameId(frame));
}

bool InspectorPageAgent::ScreencastEnabled() {
  return enabled_ &&
         state_->booleanProperty(PageAgentState::kScreencastEnabled, false);
}

void InspectorPageAgent::FrameStartedLoading(LocalFrame* frame, FrameLoadType) {
  GetFrontend()->frameStartedLoading(IdentifiersFactory::FrameId(frame));
}

void InspectorPageAgent::FrameStoppedLoading(LocalFrame* frame) {
  GetFrontend()->frameStoppedLoading(IdentifiersFactory::FrameId(frame));
}

void InspectorPageAgent::FrameScheduledNavigation(
    LocalFrame* frame,
    ScheduledNavigation* scheduled_navigation) {
  GetFrontend()->frameScheduledNavigation(
      IdentifiersFactory::FrameId(frame), scheduled_navigation->Delay(),
      ScheduledNavigationReasonToProtocol(scheduled_navigation->GetReason()),
      scheduled_navigation->Url().GetString());
}

void InspectorPageAgent::FrameClearedScheduledNavigation(LocalFrame* frame) {
  GetFrontend()->frameClearedScheduledNavigation(
      IdentifiersFactory::FrameId(frame));
}

void InspectorPageAgent::WillRunJavaScriptDialog() {
  GetFrontend()->flush();
}

void InspectorPageAgent::DidRunJavaScriptDialog() {
  GetFrontend()->flush();
}

void InspectorPageAgent::DidResizeMainFrame() {
  if (!inspected_frames_->Root()->IsMainFrame())
    return;
#if !defined(OS_ANDROID)
  PageLayoutInvalidated(true);
#endif
  GetFrontend()->frameResized();
}

void InspectorPageAgent::DidChangeViewport() {
  PageLayoutInvalidated(false);
}

void InspectorPageAgent::LifecycleEvent(LocalFrame* frame,
                                        DocumentLoader* loader,
                                        const char* name,
                                        double timestamp) {
  if (!loader ||
      !state_->booleanProperty(PageAgentState::kLifecycleEventsEnabled, false))
    return;
  GetFrontend()->lifecycleEvent(IdentifiersFactory::FrameId(frame),
                                IdentifiersFactory::LoaderId(loader), name,
                                timestamp);
}

void InspectorPageAgent::PaintTiming(Document* document,
                                     const char* name,
                                     double timestamp) {
  LocalFrame* frame = document->GetFrame();
  DocumentLoader* loader = frame->Loader().GetDocumentLoader();
  LifecycleEvent(frame, loader, name, timestamp);
}

void InspectorPageAgent::Will(const probe::UpdateLayout&) {}

void InspectorPageAgent::Did(const probe::UpdateLayout&) {
  PageLayoutInvalidated(false);
}

void InspectorPageAgent::Will(const probe::RecalculateStyle&) {}

void InspectorPageAgent::Did(const probe::RecalculateStyle&) {
  PageLayoutInvalidated(false);
}

void InspectorPageAgent::PageLayoutInvalidated(bool resized) {
  if (enabled_ && client_)
    client_->PageLayoutInvalidated(resized);
}

void InspectorPageAgent::WindowOpen(Document* document,
                                    const String& url,
                                    const AtomicString& window_name,
                                    const WebWindowFeatures& window_features,
                                    bool user_gesture) {
  KURL completed_url = url.IsEmpty() ? BlankURL() : document->CompleteURL(url);
  GetFrontend()->windowOpen(completed_url.GetString(), window_name,
                            GetEnabledWindowFeatures(window_features),
                            user_gesture);
}

std::unique_ptr<protocol::Page::Frame> InspectorPageAgent::BuildObjectForFrame(
    LocalFrame* frame) {
  DocumentLoader* loader = frame->Loader().GetDocumentLoader();
  KURL url = loader->GetRequest().Url();
  std::unique_ptr<protocol::Page::Frame> frame_object =
      protocol::Page::Frame::create()
          .setId(IdentifiersFactory::FrameId(frame))
          .setLoaderId(IdentifiersFactory::LoaderId(loader))
          .setUrl(UrlWithoutFragment(url).GetString())
          .setMimeType(frame->Loader().GetDocumentLoader()->MimeType())
          .setSecurityOrigin(SecurityOrigin::Create(url)->ToRawString())
          .build();
  Frame* parent_frame = frame->Tree().Parent();
  if (parent_frame) {
    frame_object->setParentId(IdentifiersFactory::FrameId(parent_frame));
    AtomicString name = frame->Tree().GetName();
    if (name.IsEmpty() && frame->DeprecatedLocalOwner())
      name = frame->DeprecatedLocalOwner()->getAttribute(HTMLNames::idAttr);
    frame_object->setName(name);
  }
  if (loader && !loader->UnreachableURL().IsEmpty())
    frame_object->setUnreachableUrl(loader->UnreachableURL().GetString());
  return frame_object;
}

std::unique_ptr<protocol::Page::FrameTree>
InspectorPageAgent::BuildObjectForFrameTree(LocalFrame* frame) {
  std::unique_ptr<protocol::Page::FrameTree> result =
      protocol::Page::FrameTree::create()
          .setFrame(BuildObjectForFrame(frame))
          .build();

  std::unique_ptr<protocol::Array<protocol::Page::FrameTree>> children_array;
  for (Frame* child = frame->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    if (!children_array)
      children_array = protocol::Array<protocol::Page::FrameTree>::create();
    children_array->addItem(BuildObjectForFrameTree(ToLocalFrame(child)));
  }
  result->setChildFrames(std::move(children_array));
  return result;
}

std::unique_ptr<protocol::Page::FrameResourceTree>
InspectorPageAgent::BuildObjectForResourceTree(LocalFrame* frame) {
  std::unique_ptr<protocol::Page::Frame> frame_object =
      BuildObjectForFrame(frame);
  std::unique_ptr<protocol::Array<protocol::Page::FrameResource>> subresources =
      protocol::Array<protocol::Page::FrameResource>::create();

  HeapVector<Member<Resource>> all_resources =
      CachedResourcesForFrame(frame, true);
  for (Resource* cached_resource : all_resources) {
    std::unique_ptr<protocol::Page::FrameResource> resource_object =
        protocol::Page::FrameResource::create()
            .setUrl(UrlWithoutFragment(cached_resource->Url()).GetString())
            .setType(CachedResourceTypeJson(*cached_resource))
            .setMimeType(cached_resource->GetResponse().MimeType())
            .setContentSize(cached_resource->GetResponse().DecodedBodyLength())
            .build();
    double last_modified = cached_resource->GetResponse().LastModified();
    if (!std::isnan(last_modified))
      resource_object->setLastModified(last_modified);
    if (cached_resource->WasCanceled())
      resource_object->setCanceled(true);
    else if (cached_resource->GetStatus() == ResourceStatus::kLoadError)
      resource_object->setFailed(true);
    subresources->addItem(std::move(resource_object));
  }

  HeapVector<Member<Document>> all_imports =
      InspectorPageAgent::ImportsForFrame(frame);
  for (Document* import : all_imports) {
    std::unique_ptr<protocol::Page::FrameResource> resource_object =
        protocol::Page::FrameResource::create()
            .setUrl(UrlWithoutFragment(import->Url()).GetString())
            .setType(ResourceTypeJson(InspectorPageAgent::kDocumentResource))
            .setMimeType(import->SuggestedMIMEType())
            .build();
    subresources->addItem(std::move(resource_object));
  }

  std::unique_ptr<protocol::Page::FrameResourceTree> result =
      protocol::Page::FrameResourceTree::create()
          .setFrame(std::move(frame_object))
          .setResources(std::move(subresources))
          .build();

  std::unique_ptr<protocol::Array<protocol::Page::FrameResourceTree>>
      children_array;
  for (Frame* child = frame->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    if (!children_array)
      children_array =
          protocol::Array<protocol::Page::FrameResourceTree>::create();
    children_array->addItem(BuildObjectForResourceTree(ToLocalFrame(child)));
  }
  result->setChildFrames(std::move(children_array));
  return result;
}

Response InspectorPageAgent::startScreencast(Maybe<String> format,
                                             Maybe<int> quality,
                                             Maybe<int> max_width,
                                             Maybe<int> max_height,
                                             Maybe<int> every_nth_frame) {
  state_->setBoolean(PageAgentState::kScreencastEnabled, true);
  return Response::OK();
}

Response InspectorPageAgent::stopScreencast() {
  state_->setBoolean(PageAgentState::kScreencastEnabled, false);
  return Response::OK();
}

Response InspectorPageAgent::getLayoutMetrics(
    std::unique_ptr<protocol::Page::LayoutViewport>* out_layout_viewport,
    std::unique_ptr<protocol::Page::VisualViewport>* out_visual_viewport,
    std::unique_ptr<protocol::DOM::Rect>* out_content_size) {
  LocalFrame* main_frame = inspected_frames_->Root();
  VisualViewport& visual_viewport = main_frame->GetPage()->GetVisualViewport();

  main_frame->GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  IntRect visible_contents =
      main_frame->View()->LayoutViewportScrollableArea()->VisibleContentRect();
  *out_layout_viewport = protocol::Page::LayoutViewport::create()
                             .setPageX(visible_contents.X())
                             .setPageY(visible_contents.Y())
                             .setClientWidth(visible_contents.Width())
                             .setClientHeight(visible_contents.Height())
                             .build();

  LocalFrameView* frame_view = main_frame->View();
  ScrollOffset page_offset = frame_view->GetScrollableArea()->GetScrollOffset();
  float page_zoom = main_frame->PageZoomFactor();
  FloatRect visible_rect = visual_viewport.VisibleRect();
  float scale = visual_viewport.Scale();
  float scrollbar_width =
      frame_view->LayoutViewportScrollableArea()->VerticalScrollbarWidth() /
      scale;
  float scrollbar_height =
      frame_view->LayoutViewportScrollableArea()->HorizontalScrollbarHeight() /
      scale;

  IntSize content_size = frame_view->GetScrollableArea()->ContentsSize();
  *out_content_size = protocol::DOM::Rect::create()
                          .setX(0)
                          .setY(0)
                          .setWidth(content_size.Width())
                          .setHeight(content_size.Height())
                          .build();

  *out_visual_viewport =
      protocol::Page::VisualViewport::create()
          .setOffsetX(
              AdjustForAbsoluteZoom::AdjustScroll(visible_rect.X(), page_zoom))
          .setOffsetY(
              AdjustForAbsoluteZoom::AdjustScroll(visible_rect.Y(), page_zoom))
          .setPageX(AdjustForAbsoluteZoom::AdjustScroll(page_offset.Width(),
                                                        page_zoom))
          .setPageY(AdjustForAbsoluteZoom::AdjustScroll(page_offset.Height(),
                                                        page_zoom))
          .setClientWidth(visible_rect.Width() - scrollbar_width)
          .setClientHeight(visible_rect.Height() - scrollbar_height)
          .setScale(scale)
          .build();
  return Response::OK();
}

protocol::Response InspectorPageAgent::createIsolatedWorld(
    const String& frame_id,
    Maybe<String> world_name,
    Maybe<bool> grant_universal_access,
    int* execution_context_id) {
  LocalFrame* frame =
      IdentifiersFactory::FrameById(inspected_frames_, frame_id);
  if (!frame)
    return Response::Error("No frame for given id found");

  scoped_refptr<DOMWrapperWorld> world =
      frame->GetScriptController().CreateNewInspectorIsolatedWorld(
          world_name.fromMaybe(""));
  if (!world)
    return Response::Error("Could not create isolated world");

  if (grant_universal_access.fromMaybe(false)) {
    scoped_refptr<SecurityOrigin> security_origin =
        frame->GetSecurityContext()->GetSecurityOrigin()->IsolatedCopy();
    security_origin->GrantUniversalAccess();
    DOMWrapperWorld::SetIsolatedWorldSecurityOrigin(world->GetWorldId(),
                                                    security_origin);
  }

  LocalWindowProxy* isolated_world_window_proxy =
      frame->GetScriptController().WindowProxy(*world);
  v8::HandleScope handle_scope(V8PerIsolateData::MainThreadIsolate());
  *execution_context_id = v8_inspector::V8ContextInfo::executionContextId(
      isolated_world_window_proxy->ContextIfInitialized());
  return Response::OK();
}

void InspectorPageAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  visitor->Trace(inspector_resource_content_loader_);
  InspectorBaseAgent::Trace(visitor);
}

}  // namespace blink
