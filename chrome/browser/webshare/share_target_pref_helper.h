// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBSHARE_SHARE_TARGET_PREF_HELPER_H_
#define CHROME_BROWSER_WEBSHARE_SHARE_TARGET_PREF_HELPER_H_

#include "base/optional.h"
#include "base/strings/string_piece.h"
#include "third_party/blink/public/common/manifest/manifest.h"

class GURL;
class PrefService;

// Adds the Web Share target defined by |manifest_url| to |pref_service| under
// kWebShareVisitedTargets. It maps the key |manifest_url| to a dictionary that
// contains a dictionary of the attributes of the share_target field, as well as
// the name field in |manifest|. If the |manifest| doesn't contain a
// share_target field, or it does but there is no url_template field, this will
// remove |manifest_url| from kWebShareVisitedTargets, if it is there.
void UpdateShareTargetInPrefs(const GURL& manifest_url,
                              const blink::Manifest& manifest,
                              PrefService* pref_service);

#endif // CHROME_BROWSER_WEBSHARE_SHARE_TARGET_PREF_HELPER_H_
