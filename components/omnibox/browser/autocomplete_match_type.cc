// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_match_type.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

// static
std::string AutocompleteMatchType::ToString(AutocompleteMatchType::Type type) {
  // clang-format off
  const char* strings[] = {
    "url-what-you-typed",
    "history-url",
    "history-title",
    "history-body",
    "history-keyword",
    "navsuggest",
    "search-what-you-typed",
    "search-history",
    "search-suggest",
    "search-suggest-entity",
    "search-suggest-infinite",
    "search-suggest-personalized",
    "search-suggest-profile",
    "search-other-engine",
    "extension-app",
    "contact",
    "bookmark-title",
    "navsuggest-personalized",
    "search-calculator-answer",
    "url-from-clipboard",
    "voice-suggest",
    "physical-web",
    "physical-web-overflow",
    "tab-search",
  };
  // clang-format on
  static_assert(arraysize(strings) == AutocompleteMatchType::NUM_TYPES,
                "strings array must have NUM_TYPES elements");
  return strings[type];
}

static const wchar_t kAccessibilityLabelPrefixEndSentinal[] =
    L"\uFFFC";  // Embedded object character.

static int AccessibilityLabelPrefixLength(base::string16 accessibility_label) {
  const base::string16 sentinal =
      base::WideToUTF16(kAccessibilityLabelPrefixEndSentinal);
  auto length = accessibility_label.find(sentinal);
  return length == base::string16::npos ? 0 : static_cast<int>(length);
}

// static
base::string16 AutocompleteMatchType::ToAccessibilityLabel(
    const AutocompleteMatch& match,
    const base::string16& match_text,
    int* label_prefix_length) {
  // Types with a message ID of zero get |text| returned as-is.
  static constexpr int message_ids[] = {
      0,                             // URL_WHAT_YOU_TYPED
      IDS_ACC_AUTOCOMPLETE_HISTORY,  // HISTORY_URL
      IDS_ACC_AUTOCOMPLETE_HISTORY,  // HISTORY_TITLE
      IDS_ACC_AUTOCOMPLETE_HISTORY,  // HISTORY_BODY

      // HISTORY_KEYWORD is a custom search engine with no %s in its string - so
      // more or less a regular URL.
      0,                                      // HISTORY_KEYWORD
      0,                                      // NAVSUGGEST
      IDS_ACC_AUTOCOMPLETE_SEARCH,            // SEARCH_WHAT_YOU_TYPED
      IDS_ACC_AUTOCOMPLETE_SEARCH_HISTORY,    // SEARCH_HISTORY
      IDS_ACC_AUTOCOMPLETE_SUGGESTED_SEARCH,  // SEARCH_SUGGEST
      IDS_ACC_AUTOCOMPLETE_SUGGESTED_SEARCH,  // SEARCH_SUGGEST_ENTITY
      IDS_ACC_AUTOCOMPLETE_SUGGESTED_SEARCH,  // SEARCH_SUGGEST_TAIL

      // SEARCH_SUGGEST_PERSONALIZED are searches from history elsewhere, maybe
      // on other machines via Sync, or when signed in to Google.
      IDS_ACC_AUTOCOMPLETE_HISTORY,           // SEARCH_SUGGEST_PERSONALIZED
      IDS_ACC_AUTOCOMPLETE_SUGGESTED_SEARCH,  // SEARCH_SUGGEST_PROFILE
      IDS_ACC_AUTOCOMPLETE_SEARCH,            // SEARCH_OTHER_ENGINE
      0,                                      // EXTENSION_APP (deprecated)
      0,                                      // CONTACT_DEPRECATED
      IDS_ACC_AUTOCOMPLETE_BOOKMARK,          // BOOKMARK_TITLE

      // NAVSUGGEST_PERSONALIZED is like SEARCH_SUGGEST_PERSONALIZED, but it's a
      // URL instead of a search query.
      IDS_ACC_AUTOCOMPLETE_HISTORY,    // NAVSUGGEST_PERSONALIZED
      0,                               // CALCULATOR
      IDS_ACC_AUTOCOMPLETE_CLIPBOARD,  // CLIPBOARD
      0,                               // VOICE_SUGGEST
      0,                               // PHYSICAL_WEB_DEPRECATED
      0,                               // PHYSICAL_WEB_OVERFLOW_DEPRECATED
      IDS_ACC_AUTOCOMPLETE_HISTORY,    // TAB_SEARCH_DEPRECATED
  };
  static_assert(arraysize(message_ids) == AutocompleteMatchType::NUM_TYPES,
                "message_ids must have NUM_TYPES elements");

  if (label_prefix_length)
    *label_prefix_length = 0;

  int message = message_ids[match.type];
  if (!message)
    return match_text;

  const base::string16 sentinal =
      base::WideToUTF16(kAccessibilityLabelPrefixEndSentinal);
  base::string16 description;
  bool has_description = false;
  switch (message) {
    case IDS_ACC_AUTOCOMPLETE_SEARCH_HISTORY:
    case IDS_ACC_AUTOCOMPLETE_SEARCH:
    case IDS_ACC_AUTOCOMPLETE_SUGGESTED_SEARCH:
      // Search match.
      // If additional descriptive text exists with a search, treat as search
      // with immediate answer, such as Weather in Boston: 53 degrees.
      if (match.answer) {
        description = match.answer->second_line().AccessibleText();
        has_description = true;
        message = IDS_ACC_AUTOCOMPLETE_QUICK_ANSWER;
      }
      break;

    case IDS_ACC_AUTOCOMPLETE_HISTORY:
    case IDS_ACC_AUTOCOMPLETE_BOOKMARK:
    case IDS_ACC_AUTOCOMPLETE_CLIPBOARD:
      // History match.
      // May have descriptive text for the title of the page.
      description = match.description;
      has_description = true;
      break;
    default:
      NOTREACHED();
      break;
  }

  // Get the length of friendly text inserted before the actual suggested match.
  if (label_prefix_length) {
    *label_prefix_length =
        has_description
            ? AccessibilityLabelPrefixLength(
                  l10n_util::GetStringFUTF16(message, sentinal, description))
            : AccessibilityLabelPrefixLength(
                  l10n_util::GetStringFUTF16(message, sentinal));
  }

  return has_description
             ? l10n_util::GetStringFUTF16(message, match_text, description)
             : l10n_util::GetStringFUTF16(message, match_text);
}

// static
base::string16 AutocompleteMatchType::ToAccessibilityLabel(
    const AutocompleteMatch& match,
    const base::string16& match_text,
    size_t match_index,
    size_t total_matches,
    int* label_prefix_length) {
  base::string16 result =
      ToAccessibilityLabel(match, match_text, label_prefix_length);

  return l10n_util::GetStringFUTF16(IDS_ACC_AUTOCOMPLETE_N_OF_M, result,
                                    base::IntToString16(match_index + 1),
                                    base::IntToString16(total_matches));
}
