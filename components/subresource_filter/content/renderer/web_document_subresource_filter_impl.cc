// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/renderer/web_document_subresource_filter_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop_current.h"
#include "components/subresource_filter/core/common/activation_state.h"
#include "components/subresource_filter/core/common/load_policy.h"
#include "components/subresource_filter/core/common/memory_mapped_ruleset.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace subresource_filter {

namespace proto = url_pattern_index::proto;

namespace {

using WebLoadPolicy = blink::WebDocumentSubresourceFilter::LoadPolicy;

proto::ElementType ToElementType(
    blink::WebURLRequest::RequestContext request_context) {
  switch (request_context) {
    case blink::WebURLRequest::kRequestContextAudio:
    case blink::WebURLRequest::kRequestContextVideo:
    case blink::WebURLRequest::kRequestContextTrack:
      return proto::ELEMENT_TYPE_MEDIA;
    case blink::WebURLRequest::kRequestContextBeacon:
    case blink::WebURLRequest::kRequestContextPing:
      return proto::ELEMENT_TYPE_PING;
    case blink::WebURLRequest::kRequestContextEmbed:
    case blink::WebURLRequest::kRequestContextObject:
    case blink::WebURLRequest::kRequestContextPlugin:
      return proto::ELEMENT_TYPE_OBJECT;
    case blink::WebURLRequest::kRequestContextEventSource:
    case blink::WebURLRequest::kRequestContextFetch:
    case blink::WebURLRequest::kRequestContextXMLHttpRequest:
      return proto::ELEMENT_TYPE_XMLHTTPREQUEST;
    case blink::WebURLRequest::kRequestContextFavicon:
    case blink::WebURLRequest::kRequestContextImage:
    case blink::WebURLRequest::kRequestContextImageSet:
      return proto::ELEMENT_TYPE_IMAGE;
    case blink::WebURLRequest::kRequestContextFont:
      return proto::ELEMENT_TYPE_FONT;
    case blink::WebURLRequest::kRequestContextFrame:
    case blink::WebURLRequest::kRequestContextForm:
    case blink::WebURLRequest::kRequestContextHyperlink:
    case blink::WebURLRequest::kRequestContextIframe:
    case blink::WebURLRequest::kRequestContextInternal:
    case blink::WebURLRequest::kRequestContextLocation:
      return proto::ELEMENT_TYPE_SUBDOCUMENT;
    case blink::WebURLRequest::kRequestContextScript:
    case blink::WebURLRequest::kRequestContextServiceWorker:
    case blink::WebURLRequest::kRequestContextSharedWorker:
      return proto::ELEMENT_TYPE_SCRIPT;
    case blink::WebURLRequest::kRequestContextStyle:
    case blink::WebURLRequest::kRequestContextXSLT:
      return proto::ELEMENT_TYPE_STYLESHEET;

    case blink::WebURLRequest::kRequestContextPrefetch:
    case blink::WebURLRequest::kRequestContextSubresource:
      return proto::ELEMENT_TYPE_OTHER;

    case blink::WebURLRequest::kRequestContextCSPReport:
    case blink::WebURLRequest::kRequestContextDownload:
    case blink::WebURLRequest::kRequestContextImport:
    case blink::WebURLRequest::kRequestContextManifest:
    case blink::WebURLRequest::kRequestContextUnspecified:
    default:
      return proto::ELEMENT_TYPE_UNSPECIFIED;
  }
}

WebLoadPolicy ToWebLoadPolicy(LoadPolicy load_policy) {
  switch (load_policy) {
    case LoadPolicy::ALLOW:
      return WebLoadPolicy::kAllow;
    case LoadPolicy::DISALLOW:
      return WebLoadPolicy::kDisallow;
    case LoadPolicy::WOULD_DISALLOW:
      return WebLoadPolicy::kWouldDisallow;
    default:
      NOTREACHED();
      return WebLoadPolicy::kAllow;
  }
}

void ProxyToTaskRunner(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                       base::OnceClosure callback) {
  task_runner->PostTask(FROM_HERE, std::move(callback));
}

}  // namespace

WebDocumentSubresourceFilterImpl::~WebDocumentSubresourceFilterImpl() = default;

WebDocumentSubresourceFilterImpl::WebDocumentSubresourceFilterImpl(
    url::Origin document_origin,
    ActivationState activation_state,
    scoped_refptr<const MemoryMappedRuleset> ruleset,
    base::OnceClosure first_disallowed_load_callback,
    bool is_associated_with_ad_subframe)
    : activation_state_(activation_state),
      filter_(std::move(document_origin), activation_state, std::move(ruleset)),
      first_disallowed_load_callback_(
          std::move(first_disallowed_load_callback)),
      is_associated_with_ad_subframe_(is_associated_with_ad_subframe) {}

WebLoadPolicy WebDocumentSubresourceFilterImpl::GetLoadPolicy(
    const blink::WebURL& resourceUrl,
    blink::WebURLRequest::RequestContext request_context) {
  return getLoadPolicyImpl(resourceUrl, ToElementType(request_context));
}

WebLoadPolicy
WebDocumentSubresourceFilterImpl::GetLoadPolicyForWebSocketConnect(
    const blink::WebURL& url) {
  DCHECK(url.ProtocolIs("ws") || url.ProtocolIs("wss"));
  return getLoadPolicyImpl(url, proto::ELEMENT_TYPE_WEBSOCKET);
}

void WebDocumentSubresourceFilterImpl::ReportDisallowedLoad() {
  if (!first_disallowed_load_callback_.is_null())
    std::move(first_disallowed_load_callback_).Run();
}

bool WebDocumentSubresourceFilterImpl::ShouldLogToConsole() {
  return activation_state().enable_logging;
}

bool WebDocumentSubresourceFilterImpl::GetIsAssociatedWithAdSubframe() const {
  return is_associated_with_ad_subframe_;
}

WebLoadPolicy WebDocumentSubresourceFilterImpl::getLoadPolicyImpl(
    const blink::WebURL& url,
    proto::ElementType element_type) {
  if (filter_.activation_state().filtering_disabled_for_document ||
      url.ProtocolIs(url::kDataScheme)) {
    ++filter_.statistics().num_loads_total;
    return WebLoadPolicy::kAllow;
  }

  // TODO(pkalinnikov): Would be good to avoid converting to GURL.
  return ToWebLoadPolicy(filter_.GetLoadPolicy(GURL(url), element_type));
}

WebDocumentSubresourceFilterImpl::BuilderImpl::BuilderImpl(
    url::Origin document_origin,
    ActivationState activation_state,
    base::File ruleset_file,
    base::OnceClosure first_disallowed_load_callback,
    bool is_associated_with_ad_subframe)
    : document_origin_(std::move(document_origin)),
      activation_state_(std::move(activation_state)),
      ruleset_file_(std::move(ruleset_file)),
      first_disallowed_load_callback_(
          std::move(first_disallowed_load_callback)),
      main_task_runner_(base::MessageLoopCurrent::Get()->task_runner()),
      is_associated_with_ad_subframe_(is_associated_with_ad_subframe) {}

WebDocumentSubresourceFilterImpl::BuilderImpl::~BuilderImpl() {}

std::unique_ptr<blink::WebDocumentSubresourceFilter>
WebDocumentSubresourceFilterImpl::BuilderImpl::Build() {
  DCHECK(ruleset_file_.IsValid());
  DCHECK(!main_task_runner_->BelongsToCurrentThread());
  scoped_refptr<MemoryMappedRuleset> ruleset =
      MemoryMappedRuleset::CreateAndInitialize(std::move(ruleset_file_));
  if (!ruleset)
    return nullptr;
  return std::make_unique<WebDocumentSubresourceFilterImpl>(
      document_origin_, activation_state_, std::move(ruleset),
      base::BindOnce(&ProxyToTaskRunner, main_task_runner_,
                     std::move(first_disallowed_load_callback_)),
      is_associated_with_ad_subframe_);
}

}  // namespace subresource_filter
