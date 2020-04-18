// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/http_equiv.h"

#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/private/frame_client_hints_preferences_context.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/platform/loader/fetch/client_hints_preferences.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_violation_reporting_policy.h"

namespace blink {

void HttpEquiv::Process(Document& document,
                        const AtomicString& equiv,
                        const AtomicString& content,
                        bool in_document_head_element,
                        Element* element) {
  DCHECK(!equiv.IsNull());
  DCHECK(!content.IsNull());

  if (EqualIgnoringASCIICase(equiv, "default-style")) {
    ProcessHttpEquivDefaultStyle(document, content);
  } else if (EqualIgnoringASCIICase(equiv, "refresh")) {
    ProcessHttpEquivRefresh(document, content, element);
  } else if (EqualIgnoringASCIICase(equiv, "set-cookie")) {
    ProcessHttpEquivSetCookie(document, content, element);
  } else if (EqualIgnoringASCIICase(equiv, "content-language")) {
    document.SetContentLanguage(content);
  } else if (EqualIgnoringASCIICase(equiv, "x-dns-prefetch-control")) {
    document.ParseDNSPrefetchControlHeader(content);
  } else if (EqualIgnoringASCIICase(equiv, "x-frame-options")) {
    document.AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "X-Frame-Options may only be set via an HTTP header sent along with a "
        "document. It may not be set inside <meta>."));
  } else if (EqualIgnoringASCIICase(equiv, "accept-ch")) {
    ProcessHttpEquivAcceptCH(document, content);
  } else if (EqualIgnoringASCIICase(equiv, "content-security-policy") ||
             EqualIgnoringASCIICase(equiv,
                                    "content-security-policy-report-only")) {
    if (in_document_head_element)
      ProcessHttpEquivContentSecurityPolicy(document, equiv, content);
    else
      document.GetContentSecurityPolicy()->ReportMetaOutsideHead(content);
  } else if (EqualIgnoringASCIICase(equiv, HTTPNames::Origin_Trial)) {
    if (in_document_head_element)
      OriginTrialContext::FromOrCreate(&document)->AddToken(content);
  }
}

void HttpEquiv::ProcessHttpEquivContentSecurityPolicy(
    Document& document,
    const AtomicString& equiv,
    const AtomicString& content) {
  if (document.ImportLoader())
    return;
  if (document.GetSettings() && document.GetSettings()->BypassCSP())
    return;
  if (EqualIgnoringASCIICase(equiv, "content-security-policy")) {
    document.GetContentSecurityPolicy()->DidReceiveHeader(
        content, kContentSecurityPolicyHeaderTypeEnforce,
        kContentSecurityPolicyHeaderSourceMeta);
  } else if (EqualIgnoringASCIICase(equiv,
                                    "content-security-policy-report-only")) {
    document.GetContentSecurityPolicy()->DidReceiveHeader(
        content, kContentSecurityPolicyHeaderTypeReport,
        kContentSecurityPolicyHeaderSourceMeta);
  } else {
    NOTREACHED();
  }
}

void HttpEquiv::ProcessHttpEquivAcceptCH(Document& document,
                                         const AtomicString& content) {
  if (!document.GetFrame())
    return;

  UseCounter::Count(document, WebFeature::kClientHintsMetaAcceptCH);
  FrameClientHintsPreferencesContext hints_context(document.GetFrame());
  document.GetClientHintsPreferences().UpdateFromAcceptClientHintsHeader(
      content, document.Url(), &hints_context);
}

void HttpEquiv::ProcessHttpEquivDefaultStyle(Document& document,
                                             const AtomicString& content) {
  document.GetStyleEngine().SetHttpDefaultStyle(content);
}

void HttpEquiv::ProcessHttpEquivRefresh(Document& document,
                                        const AtomicString& content,
                                        Element* element) {
  UseCounter::Count(document, WebFeature::kMetaRefresh);
  if (!document.GetContentSecurityPolicy()->AllowInlineScript(
          element, NullURL(), "", OrdinalNumber(), "",
          ContentSecurityPolicy::InlineType::kBlock,
          SecurityViolationReportingPolicy::kSuppressReporting)) {
    UseCounter::Count(document,
                      WebFeature::kMetaRefreshWhenCSPBlocksInlineScript);
  }

  document.MaybeHandleHttpRefresh(content, Document::kHttpRefreshFromMetaTag);
}

void HttpEquiv::ProcessHttpEquivSetCookie(Document& document,
                                          const AtomicString& content,
                                          Element* element) {
  Deprecation::CountDeprecation(document, WebFeature::kMetaSetCookie);

  if (!document.GetContentSecurityPolicy()->AllowInlineScript(
          element, NullURL(), "", OrdinalNumber(), "",
          ContentSecurityPolicy::InlineType::kBlock,
          SecurityViolationReportingPolicy::kSuppressReporting)) {
    UseCounter::Count(document,
                      WebFeature::kMetaSetCookieWhenCSPBlocksInlineScript);
  }

  if (!RuntimeEnabledFeatures::BlockMetaSetCookieEnabled()) {
    // Exception (for sandboxed documents) ignored.
    document.setCookie(content, IGNORE_EXCEPTION_FOR_TESTING);
    return;
  }

  document.AddConsoleMessage(ConsoleMessage::Create(
      kSecurityMessageSource, kErrorMessageLevel,
      String::Format("Blocked setting the `%s` cookie from a `<meta>` tag.",
                     content.Utf8().data())));
}

}  // namespace blink
