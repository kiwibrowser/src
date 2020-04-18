// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/modulescript/document_module_script_fetcher.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/subresource_integrity_helper.h"
#include "third_party/blink/renderer/core/script/layered_api.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_utils.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

namespace {

bool WasModuleLoadSuccessful(
    Resource* resource,
    HeapVector<Member<ConsoleMessage>>* error_messages) {
  // Implements conditions in Step 7 of
  // https://html.spec.whatwg.org/#fetch-a-single-module-script

  DCHECK(error_messages);

  if (resource) {
    SubresourceIntegrityHelper::GetConsoleMessages(
        resource->IntegrityReportInfo(), error_messages);
  }

  // - response's type is "error"
  if (!resource || resource->ErrorOccurred() ||
      resource->IntegrityDisposition() !=
          ResourceIntegrityDisposition::kPassed) {
    return false;
  }

  const auto& response = resource->GetResponse();
  // - response's status is not an ok status
  if (response.IsHTTP() && !CORS::IsOkStatus(response.HttpStatusCode())) {
    return false;
  }

  // The result of extracting a MIME type from response's header list
  // (ignoring parameters) is not a JavaScript MIME type
  // Note: For historical reasons, fetching a classic script does not include
  // MIME type checking. In contrast, module scripts will fail to load if they
  // are not of a correct MIME type.
  // We use ResourceResponse::HttpContentType() instead of MimeType(), as
  // MimeType() may be rewritten by mime sniffer.
  if (!MIMETypeRegistry::IsSupportedJavaScriptMIMEType(
          response.HttpContentType())) {
    String message =
        "Failed to load module script: The server responded with a "
        "non-JavaScript MIME type of \"" +
        response.HttpContentType() +
        "\". Strict MIME type checking is enforced for module scripts per "
        "HTML spec.";
    error_messages->push_back(ConsoleMessage::CreateForRequest(
        kJSMessageSource, kErrorMessageLevel, message,
        response.Url().GetString(), nullptr, resource->Identifier()));
    return false;
  }

  return true;
}

}  // namespace

DocumentModuleScriptFetcher::DocumentModuleScriptFetcher(
    ResourceFetcher* fetcher)
    : fetcher_(fetcher) {
  DCHECK(fetcher_);
}

void DocumentModuleScriptFetcher::Fetch(FetchParameters& fetch_params,
                                        ModuleScriptFetcher::Client* client) {
  SetClient(client);

  if (FetchIfLayeredAPI(fetch_params))
    return;

  ScriptResource::Fetch(fetch_params, fetcher_, this);
}

bool DocumentModuleScriptFetcher::FetchIfLayeredAPI(
    FetchParameters& fetch_params) {
  if (!RuntimeEnabledFeatures::LayeredAPIEnabled())
    return false;

  KURL layered_api_url = blink::layered_api::GetInternalURL(fetch_params.Url());

  if (layered_api_url.IsNull())
    return false;

  const String source_text = blink::layered_api::GetSourceText(layered_api_url);

  if (source_text.IsNull()) {
    HeapVector<Member<ConsoleMessage>> error_messages;
    error_messages.push_back(ConsoleMessage::CreateForRequest(
        kJSMessageSource, kErrorMessageLevel, "Unexpected data error",
        fetch_params.Url().GetString(), nullptr, 0));
    Finalize(base::nullopt, error_messages);
    return true;
  }

  ModuleScriptCreationParams params(
      layered_api_url, source_text,
      fetch_params.GetResourceRequest().GetFetchCredentialsMode(),
      kSharableCrossOrigin);
  Finalize(params, HeapVector<Member<ConsoleMessage>>());
  return true;
}

void DocumentModuleScriptFetcher::NotifyFinished(Resource* resource) {
  ClearResource();

  ScriptResource* script_resource = ToScriptResource(resource);

  HeapVector<Member<ConsoleMessage>> error_messages;
  if (!WasModuleLoadSuccessful(script_resource, &error_messages)) {
    Finalize(base::nullopt, error_messages);
    return;
  }

  ModuleScriptCreationParams params(
      script_resource->GetResponse().Url(), script_resource->SourceText(),
      script_resource->GetResourceRequest().GetFetchCredentialsMode(),
      script_resource->CalculateAccessControlStatus(
          fetcher_->Context().GetSecurityOrigin()));
  Finalize(params, error_messages);
}

void DocumentModuleScriptFetcher::Finalize(
    const base::Optional<ModuleScriptCreationParams>& params,
    const HeapVector<Member<ConsoleMessage>>& error_messages) {
  NotifyFetchFinished(params, error_messages);
}

void DocumentModuleScriptFetcher::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetcher_);
  ResourceClient::Trace(visitor);
  ModuleScriptFetcher::Trace(visitor);
}

}  // namespace blink
