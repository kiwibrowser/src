// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/clipboard_url_provider.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/verbatim_match.h"
#include "components/open_from_clipboard/clipboard_recent_content.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "ui/base/l10n/l10n_util.h"

ClipboardURLProvider::ClipboardURLProvider(
    AutocompleteProviderClient* client,
    HistoryURLProvider* history_url_provider,
    ClipboardRecentContent* clipboard_content)
    : AutocompleteProvider(AutocompleteProvider::TYPE_CLIPBOARD_URL),
      client_(client),
      clipboard_content_(clipboard_content),
      history_url_provider_(history_url_provider),
      current_url_suggested_times_(0) {
  DCHECK(clipboard_content_);
}

ClipboardURLProvider::~ClipboardURLProvider() {}

void ClipboardURLProvider::Start(const AutocompleteInput& input,
                                 bool minimal_changes) {
  matches_.clear();

  // If the user started typing, do not offer clipboard based match.
  if (!input.from_omnibox_focus())
    return;

  GURL url;
  // The clipboard does not contain a URL worth suggesting.
  if (!clipboard_content_->GetRecentURLFromClipboard(&url)) {
    current_url_suggested_ = GURL();
    current_url_suggested_times_ = 0;
    return;
  }
  // The URL on the page is the same as the URL in the clipboard.  Don't
  // bother suggesting it.
  if (url == input.current_url())
    return;

  DCHECK(url.is_valid());
  // If the omnibox is not empty, add a default match.
  // This match will be opened when the user presses "Enter".
  if (!input.text().empty()) {
    const base::string16 description =
        (base::FeatureList::IsEnabled(omnibox::kDisplayTitleForCurrentUrl))
            ? input.current_title()
            : base::string16();
    AutocompleteMatch verbatim_match =
        VerbatimMatchForURL(client_, input, input.current_url(), description,
                            history_url_provider_, -1);
    matches_.push_back(verbatim_match);
  }
  UMA_HISTOGRAM_BOOLEAN("Omnibox.ClipboardSuggestionShownWithCurrentURL",
                        !matches_.empty());
  UMA_HISTOGRAM_LONG_TIMES_100("Omnibox.ClipboardSuggestionShownAge",
                               clipboard_content_->GetClipboardContentAge());
  // Record the number of times the currently-offered URL has been suggested.
  // This only works over this run of Chrome; if the URL was in the clipboard
  // on a previous run, those offerings will not be counted.
  if (url == current_url_suggested_) {
    current_url_suggested_times_++;
  } else {
    current_url_suggested_ = url;
    current_url_suggested_times_ = 1;
  }
  base::UmaHistogramSparse(
      "Omnibox.ClipboardSuggestionShownNumTimes",
      std::min(current_url_suggested_times_, static_cast<size_t>(20)));

  // Add the clipboard match. The relevance is 800 to beat ZeroSuggest results.
  AutocompleteMatch match(this, 800, false, AutocompleteMatchType::CLIPBOARD);
  match.destination_url = url;
  // Because the user did not type a related input to get this clipboard
  // suggestion, preserve the path and subdomain so the user has extra context.
  auto format_types = AutocompleteMatch::GetFormatTypes(false, true, true);
  match.contents.assign(url_formatter::FormatUrl(
      url, format_types, net::UnescapeRule::SPACES, nullptr, nullptr, nullptr));
  AutocompleteMatch::ClassifyLocationInString(
      base::string16::npos, 0, match.contents.length(),
      ACMatchClassification::URL, &match.contents_class);

  match.description.assign(l10n_util::GetStringUTF16(IDS_LINK_FROM_CLIPBOARD));
  AutocompleteMatch::ClassifyLocationInString(
      base::string16::npos, 0, match.description.length(),
      ACMatchClassification::NONE, &match.description_class);

  matches_.push_back(match);
}

void ClipboardURLProvider::AddProviderInfo(ProvidersInfo* provider_info) const {
  // If a URL wasn't suggested on this most recent focus event, don't bother
  // setting |times_returned_results_in_session|, as in effect this URL has
  // never been suggested during the current session.  (For the purpose of
  // this provider, we define a session as intervals between when a URL
  // clipboard suggestion changes.)
  if (current_url_suggested_times_ == 0)
    return;
  provider_info->push_back(metrics::OmniboxEventProto_ProviderInfo());
  metrics::OmniboxEventProto_ProviderInfo& new_entry = provider_info->back();
  new_entry.set_provider(AsOmniboxEventProviderType());
  new_entry.set_times_returned_results_in_session(current_url_suggested_times_);
}
