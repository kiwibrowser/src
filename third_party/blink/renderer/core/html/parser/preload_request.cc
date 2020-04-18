// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/preload_request.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/script/document_write_intervention.h"
#include "third_party/blink/renderer/core/script/script_loader.h"
#include "third_party/blink/renderer/platform/cross_origin_attribute_value.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_info.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"

namespace blink {

KURL PreloadRequest::CompleteURL(Document* document) {
  if (!base_url_.IsEmpty())
    return document->CompleteURLWithOverride(resource_url_, base_url_);
  return document->CompleteURL(resource_url_);
}

Resource* PreloadRequest::Start(Document* document,
                                CSSPreloaderResourceClient* client) {
  DCHECK(IsMainThread());

  FetchInitiatorInfo initiator_info;
  initiator_info.name = AtomicString(initiator_name_);
  initiator_info.position = initiator_position_;

  const KURL& url = CompleteURL(document);
  // Data URLs are filtered out in the preload scanner.
  DCHECK(!url.ProtocolIsData());

  ResourceRequest resource_request(url);
  resource_request.SetHTTPReferrer(SecurityPolicy::GenerateReferrer(
      referrer_policy_, url,
      referrer_source_ == kBaseUrlIsReferrer
          ? base_url_.StrippedForUseAsReferrer()
          : document->OutgoingReferrer()));
  resource_request.SetRequestContext(ResourceFetcher::DetermineRequestContext(
      resource_type_, is_image_set_, false));

  ResourceLoaderOptions options;
  options.initiator_info = initiator_info;
  FetchParameters params(resource_request, options);

  if (resource_type_ == Resource::kImportResource) {
    const SecurityOrigin* security_origin =
        document->ContextDocument()->GetSecurityOrigin();
    params.SetCrossOriginAccessControl(security_origin,
                                       kCrossOriginAttributeAnonymous);
  }

  if (script_type_ == ScriptType::kModule) {
    DCHECK_EQ(resource_type_, Resource::kScript);
    params.SetCrossOriginAccessControl(
        document->GetSecurityOrigin(),
        ScriptLoader::ModuleScriptCredentialsMode(cross_origin_));
  } else if (cross_origin_ != kCrossOriginAttributeNotSet) {
    params.SetCrossOriginAccessControl(document->GetSecurityOrigin(),
                                       cross_origin_);
  }

  params.SetDefer(defer_);
  params.SetResourceWidth(resource_width_);
  params.GetClientHintsPreferences().UpdateFrom(client_hints_preferences_);
  params.SetIntegrityMetadata(integrity_metadata_);
  params.SetContentSecurityPolicyNonce(nonce_);
  params.SetParserDisposition(kParserInserted);

  if (request_type_ == kRequestTypeLinkRelPreload)
    params.SetLinkPreload(true);

  if (script_type_ == ScriptType::kModule) {
    DCHECK_EQ(resource_type_, Resource::kScript);
    params.SetDecoderOptions(
        TextResourceDecoderOptions::CreateAlwaysUseUTF8ForText());
  } else if (resource_type_ == Resource::kScript ||
             resource_type_ == Resource::kCSSStyleSheet ||
             resource_type_ == Resource::kImportResource) {
    params.SetCharset(charset_.IsEmpty() ? document->Encoding()
                                         : WTF::TextEncoding(charset_));
  }
  FetchParameters::SpeculativePreloadType speculative_preload_type =
      FetchParameters::SpeculativePreloadType::kInDocument;
  if (from_insertion_scanner_) {
    speculative_preload_type =
        FetchParameters::SpeculativePreloadType::kInserted;
  }
  params.SetSpeculativePreloadType(speculative_preload_type);

  if (resource_type_ == Resource::kScript) {
    MaybeDisallowFetchForDocWrittenScript(params, *document);
    // We intentionally ignore the returned value, because we don't resend
    // the async request to the blocked script here.
  }

  return document->Loader()->StartPreload(resource_type_, params, client);
}

}  // namespace blink
