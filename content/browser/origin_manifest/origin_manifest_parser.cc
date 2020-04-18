// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_parser.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"

namespace content {
namespace origin_manifest_parser {

std::unique_ptr<blink::OriginManifest> Parse(std::string json) {
  // TODO throw some meaningful errors when parsing does not go well
  std::unique_ptr<base::DictionaryValue> directives_dict =
      base::DictionaryValue::From(
          base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));

  std::unique_ptr<blink::OriginManifest> origin_manifest;

  // TODO(dhausknecht) that's playing safe. But we should do something to tell
  // things went wrong here.
  if (!directives_dict)
    return origin_manifest;

  origin_manifest.reset(new blink::OriginManifest());

  for (base::detail::dict_iterator_proxy::iterator it =
           directives_dict->DictItems().begin();
       it != directives_dict->DictItems().end(); ++it) {
    switch (GetDirectiveType(it->first)) {
      // content-security-policy
      case DirectiveType::kContentSecurityPolicy:
        ParseContentSecurityPolicy(origin_manifest.get(),
                                   std::move(it->second));
        break;
      case DirectiveType::kUnknown:
        // Ignore unknown directives for forward-compatibility
        break;
    }
  }

  return origin_manifest;
}

DirectiveType GetDirectiveType(const std::string& name) {
  if (name == "content-security-policy") {
    return DirectiveType::kContentSecurityPolicy;
  }

  return DirectiveType::kUnknown;
}

void ParseContentSecurityPolicy(blink::OriginManifest* const om,
                                base::Value value) {
  // TODO give respective parsing errors
  if (!value.is_list())
    return;

  for (auto& elem : value.GetList()) {
    if (!elem.is_dict())
      continue;

    std::string policy = "";
    blink::OriginManifest::ContentSecurityPolicyType disposition =
        blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
    blink::OriginManifest::ActivationType activation_type =
        blink::OriginManifest::ActivationType::kFallback;

    const base::Value* v = elem.FindKey("policy");
    if (v && v->is_string())
      policy = v->GetString();

    v = elem.FindKey("disposition");
    if (v && v->is_string())
      disposition = GetCSPDisposition(v->GetString());

    v = elem.FindKey("allow-override");
    if (v && v->is_bool()) {
      activation_type = GetCSPActivationType(v->GetBool());
    }

    om->AddContentSecurityPolicy(policy, disposition, activation_type);
  }
}

blink::OriginManifest::ContentSecurityPolicyType GetCSPDisposition(
    const std::string& name) {
  if (name == "enforce")
    return blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
  if (name == "report-only")
    return blink::OriginManifest::ContentSecurityPolicyType::kReport;

  return blink::OriginManifest::ContentSecurityPolicyType::kEnforce;
}

blink::OriginManifest::ActivationType GetCSPActivationType(const bool value) {
  if (value)
    return blink::OriginManifest::ActivationType::kBaseline;

  return blink::OriginManifest::ActivationType::kFallback;
}

}  // namespace origin_manifest_parser
}  // namespace content
