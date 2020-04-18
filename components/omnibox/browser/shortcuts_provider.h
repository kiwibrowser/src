// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_SHORTCUTS_PROVIDER_H_
#define COMPONENTS_OMNIBOX_BROWSER_SHORTCUTS_PROVIDER_H_

#include <map>
#include <set>
#include <string>

#include "base/gtest_prod_util.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/shortcuts_backend.h"

class AutocompleteProviderClient;
class ShortcutsProviderTest;

// Provider of recently autocompleted links. Provides autocomplete suggestions
// from previously selected suggestions. The more often a user selects a
// suggestion for a given search term the higher will be that suggestion's
// ranking for future uses of that search term.
class ShortcutsProvider : public AutocompleteProvider,
                          public ShortcutsBackend::ShortcutsBackendObserver {
 public:
  explicit ShortcutsProvider(AutocompleteProviderClient* client);

  // Performs the autocompletion synchronously. Since no asynch completion is
  // performed |minimal_changes| is ignored.
  void Start(const AutocompleteInput& input, bool minimal_changes) override;

  void DeleteMatch(const AutocompleteMatch& match) override;

 private:
  friend class ClassifyTest;
  friend class ShortcutsProviderExtensionTest;
  friend class ShortcutsProviderTest;
  FRIEND_TEST_ALL_PREFIXES(ShortcutsProviderTest, CalculateScore);

  typedef std::multimap<base::char16, base::string16> WordMap;

  ~ShortcutsProvider() override;

  // Returns a map mapping characters to groups of words from |text| that start
  // with those characters, ordered lexicographically descending so that longer
  // words appear before their prefixes (if any) within a particular
  // equal_range().
  static WordMap CreateWordMapForString(const base::string16& text);

  // Finds all instances of the words from |find_words| within |text|, adds
  // classifications to |original_class| according to the logic described below,
  // and returns the result.
  //
  //   - if |text_is_search_query| is false, the function adds
  //   ACMatchClassification::MATCH markers for all such instances.
  //
  //   For example, given the |text|
  //   "Sports and News at sports.somesite.com - visit us!" and |original_class|
  //   {{0, NONE}, {18, URL}, {37, NONE}} (marking "sports.somesite.com" as a
  //   URL), calling with |find_text| set to "sp ew" would return
  //   {{0, MATCH}, {2, NONE}, {12, MATCH}, {14, NONE}, {18, URL|MATCH},
  //   {20, URL}, {37, NONE}}.
  //
  //
  //   - if |text_is_search_query| is true, applies the same logic, but uses
  //   NONE for the matching text and MATCH for the non-matching text. This is
  //   done to mimic the behavior of SearchProvider which decorates matches
  //   according to the approach used by Google Suggest.
  //
  //   For example, given that |text| corresponds to a search query "panama
  //   canal" and |original class| is {{0, NONE}}, calling with |find_text| set
  //   to "canal" would return {{0,MATCH}, {7, NONE}}.
  //
  // |find_text| is provided as the original string used to create
  // |find_words|.  This is supplied because it's common for this to be a prefix
  // of |text|, so we can quickly check for that and mark that entire substring
  // as a match before proceeding with the more generic algorithm.
  //
  // |find_words| should be as constructed by CreateWordMapForString(find_text).
  //
  // |find_text| (and thus |find_words|) are expected to be lowercase.  |text|
  // will be lowercased in this function.
  static ACMatchClassifications ClassifyAllMatchesInString(
      const base::string16& find_text,
      const WordMap& find_words,
      const base::string16& text,
      const bool text_is_search_query,
      const ACMatchClassifications& original_class);

  // ShortcutsBackendObserver:
  void OnShortcutsLoaded() override;

  // Performs the autocomplete matching and scoring.
  void GetMatches(const AutocompleteInput& input);

  // Returns an AutocompleteMatch corresponding to |shortcut|. Assigns it
  // |relevance| score in the process, and highlights the description and
  // contents against |input|, which should be the lower-cased version of
  // the user's input. |input| and |fixed_up_input_text| are used to decide
  // what can be inlined.
  AutocompleteMatch ShortcutToACMatch(
      const ShortcutsDatabase::Shortcut& shortcut,
      int relevance,
      const AutocompleteInput& input,
      const base::string16& fixed_up_input_text,
      const base::string16 term_string,
      const WordMap& terms_map);

  // Returns iterator to first item in |shortcuts_map_| matching |keyword|.
  // Returns shortcuts_map_.end() if there are no matches.
  ShortcutsBackend::ShortcutMap::const_iterator FindFirstMatch(
      const base::string16& keyword,
      ShortcutsBackend* backend);

  int CalculateScore(const base::string16& terms,
                     const ShortcutsDatabase::Shortcut& shortcut,
                     int max_relevance);

  // The default max relevance unless overridden by a field trial.
  static const int kShortcutsProviderDefaultMaxRelevance;

  AutocompleteProviderClient* client_;
  bool initialized_;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_SHORTCUTS_PROVIDER_H_
