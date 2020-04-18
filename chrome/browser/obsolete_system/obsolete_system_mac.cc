// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/obsolete_system/obsolete_system.h"

#include "base/mac/mac_util.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"

// static
bool ObsoleteSystem::IsObsoleteNowOrSoon() {
  return base::mac::IsOS10_9() &&
         base::FeatureList::IsEnabled(features::kShow10_9ObsoleteInfobar);
}

// static
base::string16 ObsoleteSystem::LocalizedObsoleteString() {
  return l10n_util::GetStringUTF16(IDS_MAC_10_9_OBSOLETE_NOW);
}

// static
bool ObsoleteSystem::IsEndOfTheLine() {
  return true;
}

// static
const char* ObsoleteSystem::GetLinkURL() {
  return chrome::kMac10_9_ObsoleteURL;
}
