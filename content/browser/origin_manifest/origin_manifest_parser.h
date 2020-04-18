// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_

#include <string>

#include "base/values.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/common/origin_manifest/origin_manifest.h"

namespace base {
class Value;
}

namespace content {
namespace origin_manifest_parser {

// Tries to parse an Origin Manifest from string. In case it fails the pointer
// is set to nullptr.
std::unique_ptr<blink::OriginManifest> CONTENT_EXPORT Parse(std::string json);

enum class DirectiveType {
  kContentSecurityPolicy,
  kUnknown,
};

// Tries to parse the |DirectiveType| from string. Valid strings are all
// top-level keywords from the Origin Manifest JSON format, e.g.
// 'content-security-policy'. Returns kUnknown on failure.
DirectiveType CONTENT_EXPORT GetDirectiveType(const std::string& str);

// Reads out the values from |value| to add a ContentSecurityPolicy to the given
// OriginManifest using its interface. In case invalid values are set they are
// simply ignored. In case values are missing the respective default values are
// set.
void CONTENT_EXPORT ParseContentSecurityPolicy(blink::OriginManifest* const om,
                                               base::Value value);

// Tries to parse the |ContentSecurityPolicyType| from string. Valid values are
// "enforce" and "report-only". Returns kUnknown on failure.
blink::OriginManifest::ContentSecurityPolicyType CONTENT_EXPORT
GetCSPDisposition(const std::string& json);

// If |value| is true returns kBaseline, kFallback otherwise.
blink::OriginManifest::ActivationType CONTENT_EXPORT
GetCSPActivationType(const bool value);

}  // namespace origin_manifest_parser
}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
