// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_SETTINGS_API_BUBBLE_HELPERS_H_
#define CHROME_BROWSER_UI_EXTENSIONS_SETTINGS_API_BUBBLE_HELPERS_H_

#include "components/omnibox/browser/autocomplete_match.h"

class Browser;

namespace content {
class WebContents;
}

namespace extensions {

// Sets whether the NTP bubble is enabled for testing purposes.
void SetNtpBubbleEnabledForTesting(bool enabled);

// Shows a bubble notifying the user that the homepage is controlled by an
// extension. This bubble is shown only on the first use of the Home button
// after the controlling extension takes effect.
void MaybeShowExtensionControlledHomeNotification(Browser* browser);

// Shows a bubble notifying the user that the search engine is controlled by an
// extension. This bubble is shown only on the first search after the
// controlling extension takes effect.
void MaybeShowExtensionControlledSearchNotification(
    content::WebContents* web_contents,
    AutocompleteMatch::Type match_type);

// Shows a bubble notifying the user that the new tab page is controlled by an
// extension. This bubble is shown only the first time the new tab page is shown
// after the controlling extension takes effect.
void MaybeShowExtensionControlledNewTabPage(
    Browser* browser,
    content::WebContents* web_contents);

}  // namespace extensions

#endif  // CHROME_BROWSER_UI_EXTENSIONS_SETTINGS_API_BUBBLE_HELPERS_H_
