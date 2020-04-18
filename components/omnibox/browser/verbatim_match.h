// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_VERBATIM_MATCH_H_
#define COMPONENTS_OMNIBOX_BROWSER_VERBATIM_MATCH_H_

#include "base/strings/string16.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "url/gurl.h"

struct AutocompleteMatch;
class AutocompleteProviderClient;
class HistoryURLProvider;

// Returns a verbatim match for input.text() with a relevance of
// |verbatim_relevance|. If |verbatim_relevance| is negative then a default
// value is used. If the desired |destination_url| is already known and a
// |history_url_provider| is also provided, use |destination_description| as the
// description. Providing |history_url_provider| also may be more efficient (see
// implementation for details) than the default code path.
// input.text() must not be empty.
AutocompleteMatch VerbatimMatchForURL(
    AutocompleteProviderClient* client,
    const AutocompleteInput& input,
    const GURL& destination_url,
    const base::string16& destination_description,
    HistoryURLProvider* history_url_provider,
    int verbatim_relevance);

#endif  // COMPONENTS_OMNIBOX_BROWSER_VERBATIM_MATCH_H_
