// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_provider.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/url_formatter/url_fixer.h"
#include "url/gurl.h"

// static
const size_t AutocompleteProvider::kMaxMatches = 3;

AutocompleteProvider::AutocompleteProvider(Type type)
    : done_(true),
      type_(type) {
}

// static
const char* AutocompleteProvider::TypeToString(Type type) {
  switch (type) {
    case TYPE_BOOKMARK:
      return "Bookmark";
    case TYPE_BUILTIN:
      return "Builtin";
    case TYPE_HISTORY_QUICK:
      return "HistoryQuick";
    case TYPE_HISTORY_URL:
      return "HistoryURL";
    case TYPE_KEYWORD:
      return "Keyword";
    case TYPE_SEARCH:
      return "Search";
    case TYPE_SHORTCUTS:
      return "Shortcuts";
    case TYPE_ZERO_SUGGEST:
      return "ZeroSuggest";
    case TYPE_CLIPBOARD_URL:
      return "ClipboardURL";
    default:
      NOTREACHED() << "Unhandled AutocompleteProvider::Type " << type;
      return "Unknown";
  }
}

void AutocompleteProvider::Stop(bool clear_cached_results,
                                bool due_to_user_inactivity) {
  done_ = true;
}

const char* AutocompleteProvider::GetName() const {
  return TypeToString(type_);
}

metrics::OmniboxEventProto_ProviderType AutocompleteProvider::
    AsOmniboxEventProviderType() const {
  switch (type_) {
    case TYPE_BOOKMARK:
      return metrics::OmniboxEventProto::BOOKMARK;
    case TYPE_BUILTIN:
      return metrics::OmniboxEventProto::BUILTIN;
    case TYPE_HISTORY_QUICK:
      return metrics::OmniboxEventProto::HISTORY_QUICK;
    case TYPE_HISTORY_URL:
      return metrics::OmniboxEventProto::HISTORY_URL;
    case TYPE_KEYWORD:
      return metrics::OmniboxEventProto::KEYWORD;
    case TYPE_SEARCH:
      return metrics::OmniboxEventProto::SEARCH;
    case TYPE_SHORTCUTS:
      return metrics::OmniboxEventProto::SHORTCUTS;
    case TYPE_ZERO_SUGGEST:
      return metrics::OmniboxEventProto::ZERO_SUGGEST;
    case TYPE_CLIPBOARD_URL:
      return metrics::OmniboxEventProto::CLIPBOARD_URL;
    default:
      NOTREACHED() << "Unhandled AutocompleteProvider::Type " << type_;
      return metrics::OmniboxEventProto::UNKNOWN_PROVIDER;
  }
}

void AutocompleteProvider::DeleteMatch(const AutocompleteMatch& match) {
  DLOG(WARNING) << "The AutocompleteProvider '" << GetName()
                << "' has not implemented DeleteMatch.";
}

void AutocompleteProvider::AddProviderInfo(ProvidersInfo* provider_info) const {
}

void AutocompleteProvider::ResetSession() {
}

size_t AutocompleteProvider::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(matches_);
}

AutocompleteProvider::~AutocompleteProvider() {
  Stop(false, false);
}

// static
AutocompleteProvider::FixupReturn AutocompleteProvider::FixupUserInput(
    const AutocompleteInput& input) {
  const base::string16& input_text = input.text();
  const FixupReturn failed(false, input_text);

  // Fixup and canonicalize user input.
  const GURL canonical_gurl(
      url_formatter::FixupURL(base::UTF16ToUTF8(input_text), std::string()));
  std::string canonical_gurl_str(canonical_gurl.possibly_invalid_spec());
  if (canonical_gurl_str.empty()) {
    // This probably won't happen, but there are no guarantees.
    return failed;
  }

  // If the user types a number, GURL will convert it to a dotted quad.
  // However, if the parser did not mark this as a URL, then the user probably
  // didn't intend this interpretation.  Since this can break history matching
  // for hostname beginning with numbers (e.g. input of "17173" will be matched
  // against "0.0.67.21" instead of the original "17173", failing to find
  // "17173.com"), swap the original hostname in for the fixed-up one.
  if ((input.type() != metrics::OmniboxInputType::URL) &&
      canonical_gurl.HostIsIPAddress()) {
    std::string original_hostname =
        base::UTF16ToUTF8(input_text.substr(input.parts().host.begin,
                                            input.parts().host.len));
    const url::Parsed& parts =
        canonical_gurl.parsed_for_possibly_invalid_spec();
    // parts.host must not be empty when HostIsIPAddress() is true.
    DCHECK(parts.host.is_nonempty());
    canonical_gurl_str.replace(parts.host.begin, parts.host.len,
                               original_hostname);
  }
  base::string16 output(base::UTF8ToUTF16(canonical_gurl_str));
  // Don't prepend a scheme when the user didn't have one.  Since the fixer
  // upper only prepends the "http" scheme, that's all we need to check for.
  if (!AutocompleteInput::HasHTTPScheme(input_text))
    TrimHttpPrefix(&output);

  // Make the number of trailing slashes on the output exactly match the input.
  // Examples of why not doing this would matter:
  // * The user types "a" and has this fixed up to "a/".  Now no other sites
  //   beginning with "a" will match.
  // * The user types "file:" and has this fixed up to "file://".  Now inline
  //   autocomplete will append too few slashes, resulting in e.g. "file:/b..."
  //   instead of "file:///b..."
  // * The user types "http:/" and has this fixed up to "http:".  Now inline
  //   autocomplete will append too many slashes, resulting in e.g.
  //   "http:///c..." instead of "http://c...".
  // NOTE: We do this after calling TrimHttpPrefix() since that can strip
  // trailing slashes (if the scheme is the only thing in the input).  It's not
  // clear that the result of fixup really matters in this case, but there's no
  // harm in making sure.
  const size_t last_input_nonslash =
      input_text.find_last_not_of(base::ASCIIToUTF16("/\\"));
  const size_t num_input_slashes =
      (last_input_nonslash == base::string16::npos) ?
      input_text.length() : (input_text.length() - 1 - last_input_nonslash);
  const size_t last_output_nonslash =
      output.find_last_not_of(base::ASCIIToUTF16("/\\"));
  const size_t num_output_slashes =
      (last_output_nonslash == base::string16::npos) ?
      output.length() : (output.length() - 1 - last_output_nonslash);
  if (num_output_slashes < num_input_slashes)
    output.append(num_input_slashes - num_output_slashes, '/');
  else if (num_output_slashes > num_input_slashes)
    output.erase(output.length() - num_output_slashes + num_input_slashes);
  if (output.empty())
    return failed;

  return FixupReturn(true, output);
}

// static
size_t AutocompleteProvider::TrimHttpPrefix(base::string16* url) {
  // Find any "http:".
  if (!AutocompleteInput::HasHTTPScheme(*url))
    return 0;
  size_t scheme_pos =
      url->find(base::ASCIIToUTF16(url::kHttpScheme) + base::char16(':'));
  DCHECK_NE(base::string16::npos, scheme_pos);

  // Erase scheme plus up to two slashes.
  size_t prefix_end = scheme_pos + strlen(url::kHttpScheme) + 1;
  const size_t after_slashes = std::min(url->length(), prefix_end + 2);
  while ((prefix_end < after_slashes) && ((*url)[prefix_end] == '/'))
    ++prefix_end;
  url->erase(scheme_pos, prefix_end - scheme_pos);
  return (scheme_pos == 0) ? prefix_end : 0;
}
