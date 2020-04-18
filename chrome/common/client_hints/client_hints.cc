// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/client_hints/client_hints.h"

#include "content/public/common/origin_util.h"
#include "third_party/blink/public/platform/web_client_hints_type.h"
#include "url/gurl.h"

namespace client_hints {

void GetAllowedClientHintsFromSource(
    const GURL& url,
    const ContentSettingsForOneType& client_hints_rules,
    blink::WebEnabledClientHints* client_hints) {
  if (client_hints_rules.empty())
    return;

  if (!content::IsOriginSecure(url))
    return;

  const GURL& origin = url.GetOrigin();

  for (const auto& rule : client_hints_rules) {
    // Look for an exact match since persisted client hints are disabled by
    // default, and enabled only on per-host basis.
    if (rule.primary_pattern == ContentSettingsPattern::Wildcard() ||
        !rule.primary_pattern.Matches(origin)) {
      continue;
    }

    // Found an exact match.
    DCHECK(ContentSettingsPattern::Wildcard() == rule.secondary_pattern);
    DCHECK(rule.setting_value.is_dict());
    const base::Value* expiration_time =
        rule.setting_value.FindKey("expiration_time");
    DCHECK(expiration_time->is_double());

    if (base::Time::Now().ToDoubleT() > expiration_time->GetDouble()) {
      // The client hint is expired.
      return;
    }

    const base::Value* list_value = rule.setting_value.FindKey("client_hints");
    DCHECK(list_value->is_list());
    const base::Value::ListStorage& client_hints_list = list_value->GetList();
    for (const auto& client_hint : client_hints_list) {
      DCHECK(client_hint.is_int());
      client_hints->SetIsEnabled(
          static_cast<blink::mojom::WebClientHintsType>(client_hint.GetInt()),
          true);
    }
    // Match found for |url| and client hints have been set.
    return;
  }
}

}  // namespace client_hints
