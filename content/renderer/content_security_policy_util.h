// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CONTENT_SECURITY_POLICY_UTIL_H_
#define CONTENT_RENDERER_CONTENT_SECURITY_POLICY_UTIL_H_

#include "content/common/content_security_policy/content_security_policy.h"
#include "content/common/content_security_policy/csp_context.h"
#include "third_party/blink/public/platform/web_content_security_policy_struct.h"

namespace content {

// Convert a WebContentSecurityPolicy into a ContentSecurityPolicy. These two
// classes represent the exact same thing, but one is in content, the other is
// in blink.
ContentSecurityPolicy BuildContentSecurityPolicy(
    const blink::WebContentSecurityPolicy&);

// Convert a WebContentSecurityPolicyList into a list of ContentSecurityPolicy.
std::vector<ContentSecurityPolicy> BuildContentSecurityPolicyList(
    const blink::WebContentSecurityPolicyList&);

CSPSource BuildCSPSource(
    const blink::WebContentSecurityPolicySourceExpression&);

// Convert a CSPViolationParams into a WebContentSecurityPolicyViolation. These
// two classes represent the exact same thing, but one is in content, the other
// is in blink.
blink::WebContentSecurityPolicyViolation BuildWebContentSecurityPolicyViolation(
    const content::CSPViolationParams& violation_params);

}  // namespace content

#endif /* CONTENT_RENDERER_CONTENT_SECURITY_POLICY_UTIL_H_ */
