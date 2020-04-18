// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/security_style_explanations.h"

namespace content {

SecurityStyleExplanations::SecurityStyleExplanations()
    : ran_mixed_content(false),
      displayed_mixed_content(false),
      contained_mixed_form(false),
      ran_content_with_cert_errors(false),
      displayed_content_with_cert_errors(false),
      ran_insecure_content_style(blink::kWebSecurityStyleUnknown),
      displayed_insecure_content_style(blink::kWebSecurityStyleUnknown),
      scheme_is_cryptographic(false),
      pkp_bypassed(false) {}

SecurityStyleExplanations::SecurityStyleExplanations(
    const SecurityStyleExplanations& other) = default;

SecurityStyleExplanations::~SecurityStyleExplanations() {
}

}  // namespace content
