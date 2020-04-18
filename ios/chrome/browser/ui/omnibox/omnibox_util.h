// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_UTIL_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_UTIL_H_

#include "components/omnibox/browser/autocomplete_match_type.h"
#include "components/security_state/core/security_state.h"

// Converts |type| to a resource identifier for the appropriate icon for this
// type to show in the omnibox.
int GetIconForAutocompleteMatchType(AutocompleteMatchType::Type type,
                                    bool is_starred,
                                    bool is_incognito);

// Converts |security_level| to a resource identifier for the appropriate icon
// for this security level in the omnibox.
int GetIconForSecurityState(security_state::SecurityLevel security_level);

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_UTIL_H_
