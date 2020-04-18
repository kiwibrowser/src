// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/history/history_utils.h"

#include "components/dom_distiller/core/url_constants.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ios {

// Returns true if this looks like the type of URL that should be added to the
// history. This filters out URLs such a JavaScript.
bool CanAddURLToHistory(const GURL& url) {
  if (!url.is_valid())
    return false;

  // TODO: We should allow ChromeUIScheme URLs if they have been explicitly
  // typed.  Right now, however, these are marked as typed even when triggered
  // by a shortcut or menu action.
  if (url.SchemeIs(url::kJavaScriptScheme) ||
      url.SchemeIs(dom_distiller::kDomDistillerScheme) ||
      url.SchemeIs(kChromeUIScheme))
    return false;

  // Allow all about: and chrome: URLs except about:blank, since the user may
  // like to see "chrome://version", etc. in their history and autocomplete.
  if (url == url::kAboutBlankURL)
    return false;

  return true;
}

}  // namespace ios
