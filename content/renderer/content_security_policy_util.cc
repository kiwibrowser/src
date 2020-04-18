// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/content_security_policy_util.h"

namespace content {

CSPSource BuildCSPSource(
    const blink::WebContentSecurityPolicySourceExpression& source) {
  return CSPSource(
      source.scheme.Utf8(),  // scheme
      source.host.Utf8(),    // host
      source.is_host_wildcard == blink::kWebWildcardDispositionHasWildcard,
      source.port == 0 ? url::PORT_UNSPECIFIED : source.port,  // port
      source.is_port_wildcard == blink::kWebWildcardDispositionHasWildcard,
      source.path.Utf8());  // path
}

CSPSourceList BuildCSPSourceList(
    const blink::WebContentSecurityPolicySourceList& source_list) {
  std::vector<CSPSource> sources;
  for (const auto& source : source_list.sources) {
    sources.push_back(BuildCSPSource(source));
  }

  return CSPSourceList(source_list.allow_self,       // allow_self
                       source_list.allow_star,       // allow_star
                       source_list.allow_redirects,  // allow_redirects
                       sources);                     // source_list
}

CSPDirective BuildCSPDirective(
    const blink::WebContentSecurityPolicyDirective& directive) {
  return CSPDirective(
      CSPDirective::StringToName(directive.name.Utf8()),  // name
      BuildCSPSourceList(directive.source_list));         // source_list
}

ContentSecurityPolicy BuildContentSecurityPolicy(
    const blink::WebContentSecurityPolicy& policy) {
  std::vector<CSPDirective> directives;
  for (const auto& directive : policy.directives)
    directives.push_back(BuildCSPDirective(directive));

  std::vector<std::string> report_endpoints;
  for (const blink::WebString& endpoint : policy.report_endpoints)
    report_endpoints.push_back(endpoint.Utf8());

  return ContentSecurityPolicy(
      ContentSecurityPolicyHeader(policy.header.Utf8(), policy.disposition,
                                  policy.source),
      directives, report_endpoints, policy.use_reporting_api);
}

std::vector<ContentSecurityPolicy> BuildContentSecurityPolicyList(
    const blink::WebContentSecurityPolicyList& policies) {
  std::vector<ContentSecurityPolicy> list;

  for (const auto& policy : policies.policies)
    list.push_back(BuildContentSecurityPolicy(policy));

  return list;
}

blink::WebContentSecurityPolicyViolation BuildWebContentSecurityPolicyViolation(
    const content::CSPViolationParams& violation_params) {
  blink::WebContentSecurityPolicyViolation violation;
  violation.directive = blink::WebString::FromASCII(violation_params.directive);
  violation.effective_directive =
      blink::WebString::FromASCII(violation_params.effective_directive);
  violation.console_message =
      blink::WebString::FromASCII(violation_params.console_message);
  violation.blocked_url = violation_params.blocked_url;
  violation.report_endpoints = blink::WebVector<blink::WebString>(
      violation_params.report_endpoints.size());
  for (size_t i = 0; i < violation_params.report_endpoints.size(); ++i) {
    violation.report_endpoints[i] =
        blink::WebString::FromASCII(violation_params.report_endpoints[i]);
  }
  violation.use_reporting_api = violation_params.use_reporting_api;
  violation.header = blink::WebString::FromASCII(violation_params.header);
  violation.disposition = violation_params.disposition;
  violation.after_redirect = violation_params.after_redirect;
  violation.source_location.url =
      blink::WebString::FromLatin1(violation_params.source_location.url);
  violation.source_location.line_number =
      violation_params.source_location.line_number;
  violation.source_location.column_number =
      violation_params.source_location.column_number;
  return violation;
}

}  // namespace content
