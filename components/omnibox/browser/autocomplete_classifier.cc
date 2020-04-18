// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_classifier.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/feature_list.h"
#include "build/build_config.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "url/gurl.h"

AutocompleteClassifier::AutocompleteClassifier(
    std::unique_ptr<AutocompleteController> controller,
    std::unique_ptr<AutocompleteSchemeClassifier> scheme_classifier)
    : controller_(std::move(controller)),
      scheme_classifier_(std::move(scheme_classifier)),
      inside_classify_(false) {}

AutocompleteClassifier::~AutocompleteClassifier() {
  // We should only reach here after Shutdown() has been called.
  DCHECK(!controller_);
}

void AutocompleteClassifier::Shutdown() {
  controller_.reset();
}

// static
int AutocompleteClassifier::DefaultOmniboxProviders() {
  return
#if !defined(OS_ANDROID) && !defined(OS_IOS)
      // Custom search engines cannot be used on mobile.
      AutocompleteProvider::TYPE_KEYWORD |
#endif
      AutocompleteProvider::TYPE_ZERO_SUGGEST |
      (base::FeatureList::IsEnabled(omnibox::kEnableClipboardProvider)
           ? AutocompleteProvider::TYPE_CLIPBOARD_URL
           : 0) |
      AutocompleteProvider::TYPE_BOOKMARK | AutocompleteProvider::TYPE_BUILTIN |
      AutocompleteProvider::TYPE_HISTORY_QUICK |
      AutocompleteProvider::TYPE_HISTORY_URL |
      AutocompleteProvider::TYPE_SEARCH | AutocompleteProvider::TYPE_SHORTCUTS;
}

void AutocompleteClassifier::Classify(
    const base::string16& text,
    bool prefer_keyword,
    bool allow_exact_keyword_match,
    metrics::OmniboxEventProto::PageClassification page_classification,
    AutocompleteMatch* match,
    GURL* alternate_nav_url) {
  DCHECK(!inside_classify_);
  base::AutoReset<bool> reset(&inside_classify_, true);
  AutocompleteInput input(text, page_classification, *scheme_classifier_);
  input.set_prevent_inline_autocomplete(true);
  input.set_prefer_keyword(prefer_keyword);
  input.set_allow_exact_keyword_match(allow_exact_keyword_match);
  input.set_want_asynchronous_matches(false);
  controller_->Start(input);
  DCHECK(controller_->done());
  const AutocompleteResult& result = controller_->result();
  if (result.empty()) {
    if (alternate_nav_url)
      *alternate_nav_url = GURL();
    return;
  }

  DCHECK(result.default_match() != result.end());
  *match = *result.default_match();
  if (alternate_nav_url)
    *alternate_nav_url = result.alternate_nav_url();
}
