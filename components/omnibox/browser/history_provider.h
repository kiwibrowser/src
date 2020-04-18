// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_HISTORY_PROVIDER_H_
#define COMPONENTS_OMNIBOX_BROWSER_HISTORY_PROVIDER_H_

#include <stddef.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/in_memory_url_index_types.h"

class AutocompleteInput;
struct AutocompleteMatch;

// This class is a base class for the history autocomplete providers and
// provides functions useful to all derived classes.
class HistoryProvider : public AutocompleteProvider {
 public:
  void DeleteMatch(const AutocompleteMatch& match) override;

  // Returns true if inline autocompletion should be prevented for URL-like
  // input.  This method returns true if input.prevent_inline_autocomplete()
  // is true or the input text contains trailing whitespace.
  static bool PreventInlineAutocomplete(const AutocompleteInput& input);

 protected:
  HistoryProvider(AutocompleteProvider::Type type,
                  AutocompleteProviderClient* client);

  ~HistoryProvider() override;

  // Finds and removes the match from the current collection of matches and
  // backing data.
  void DeleteMatchFromMatches(const AutocompleteMatch& match);

  // Fill and return an ACMatchClassifications structure given the |matches|
  // to highlight.
  static ACMatchClassifications SpansFromTermMatch(const TermMatches& matches,
                                                   size_t text_length,
                                                   bool is_url);

  AutocompleteProviderClient* client() { return client_; }

 private:
  AutocompleteProviderClient* client_;

  DISALLOW_COPY_AND_ASSIGN(HistoryProvider);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_HISTORY_PROVIDER_H_
