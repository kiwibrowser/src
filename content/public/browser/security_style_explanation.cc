// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/security_style_explanation.h"

namespace content {

SecurityStyleExplanation::SecurityStyleExplanation() {}

SecurityStyleExplanation::SecurityStyleExplanation(
    const std::string& summary,
    const std::string& description)
    : SecurityStyleExplanation(std::string(), summary, description) {}

SecurityStyleExplanation::SecurityStyleExplanation(
    const std::string& title,
    const std::string& summary,
    const std::string& description)
    : title(title),
      summary(summary),
      description(description),
      mixed_content_type(blink::WebMixedContentContextType::kNotMixedContent) {}

SecurityStyleExplanation::SecurityStyleExplanation(
    const std::string& title,
    const std::string& summary,
    const std::string& description,
    scoped_refptr<net::X509Certificate> certificate,
    blink::WebMixedContentContextType mixed_content_type)
    : title(title),
      summary(summary),
      description(description),
      certificate(certificate),
      mixed_content_type(mixed_content_type) {}

SecurityStyleExplanation::SecurityStyleExplanation(
    const SecurityStyleExplanation& other) = default;

SecurityStyleExplanation& SecurityStyleExplanation::operator=(
    const SecurityStyleExplanation& other) = default;

SecurityStyleExplanation::~SecurityStyleExplanation() {}

}  // namespace content
