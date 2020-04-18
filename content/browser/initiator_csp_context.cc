// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/initiator_csp_context.h"

namespace content {

InitiatorCSPContext::InitiatorCSPContext(
    const std::vector<ContentSecurityPolicy>& policies,
    base::Optional<CSPSource>& self_source)
    : reporting_render_frame_host_impl_(nullptr) {
  for (const auto& policy : policies)
    AddContentSecurityPolicy(policy);

  if (self_source.has_value())
    SetSelf(self_source.value());
}

void InitiatorCSPContext::SetReportingRenderFrameHost(
    RenderFrameHostImpl* rfh) {
  reporting_render_frame_host_impl_ = rfh;
}

void InitiatorCSPContext::ReportContentSecurityPolicyViolation(
    const CSPViolationParams& violation_params) {
  if (reporting_render_frame_host_impl_) {
    reporting_render_frame_host_impl_->ReportContentSecurityPolicyViolation(
        violation_params);
  }
}

bool InitiatorCSPContext::SchemeShouldBypassCSP(
    const base::StringPiece& scheme) {
  // TODO(andypaicu): RenderFrameHostImpl::SchemeShouldBypassCSP could be
  // static except for the fact that it's virtual. It's weird to use
  // the reporting RFH to do this check but overall harmless.
  if (reporting_render_frame_host_impl_)
    return reporting_render_frame_host_impl_->SchemeShouldBypassCSP(scheme);

  return false;
}

void InitiatorCSPContext::SanitizeDataForUseInCspViolation(
    bool is_redirect,
    CSPDirective::Name directive,
    GURL* blocked_url,
    SourceLocation* source_location) const {
  if (reporting_render_frame_host_impl_) {
    reporting_render_frame_host_impl_->SanitizeDataForUseInCspViolation(
        is_redirect, directive, blocked_url, source_location);
  }
}

}  // namespace content
