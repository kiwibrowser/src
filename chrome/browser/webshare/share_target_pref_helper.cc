// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webshare/share_target_pref_helper.h"

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

void UpdateShareTargetInPrefs(const GURL& manifest_url,
                              const blink::Manifest& manifest,
                              PrefService* pref_service) {
  DictionaryPrefUpdate update(pref_service, prefs::kWebShareVisitedTargets);
  base::DictionaryValue* share_target_dict = update.Get();

  // Manifest does not contain a share_target field, or it does but the
  // url_template is invalid.
  if (!manifest.share_target.has_value() ||
      !manifest.share_target.value().url_template.is_valid()) {
    share_target_dict->RemoveWithoutPathExpansion(manifest_url.spec(), nullptr);
    return;
  }

  // TODO(mgiuca): This DCHECK is known to fail due to https://crbug.com/762388.
  // Currently, this can only happen if flags are turned on. These cases should
  // be fixed before this feature is rolled out.
  DCHECK(manifest_url.is_valid());

  constexpr char kNameKey[] = "name";
  constexpr char kUrlTemplateKey[] = "url_template";

  std::unique_ptr<base::DictionaryValue> origin_dict(new base::DictionaryValue);

  if (!manifest.name.is_null()) {
    origin_dict->SetKey(kNameKey, base::Value(manifest.name.string()));
  }
  origin_dict->SetKey(kUrlTemplateKey,
                      base::Value(manifest.share_target->url_template.spec()));

  share_target_dict->SetWithoutPathExpansion(manifest_url.spec(),
                                             std::move(origin_dict));
}
