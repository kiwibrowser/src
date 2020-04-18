// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/omnibox_provider.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/search/omnibox_result.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "url/gurl.h"

namespace app_list {

OmniboxProvider::OmniboxProvider(Profile* profile,
                                 AppListControllerDelegate* list_controller)
    : profile_(profile),
      list_controller_(list_controller),
      controller_(new AutocompleteController(
          base::WrapUnique(new ChromeAutocompleteProviderClient(profile)),
          this,
          AutocompleteClassifier::DefaultOmniboxProviders() &
              ~AutocompleteProvider::TYPE_ZERO_SUGGEST)) {}

OmniboxProvider::~OmniboxProvider() {}

void OmniboxProvider::Start(const base::string16& query) {
  controller_->Stop(false);
  AutocompleteInput input =
      AutocompleteInput(query, metrics::OmniboxEventProto::INVALID_SPEC,
                        ChromeAutocompleteSchemeClassifier(profile_));
  controller_->Start(input);
}

void OmniboxProvider::PopulateFromACResult(const AutocompleteResult& result) {
  SearchProvider::Results new_results;
  new_results.reserve(result.size());
  for (const AutocompleteMatch& match : result) {
    if (!match.destination_url.is_valid())
      continue;

    new_results.emplace_back(std::make_unique<OmniboxResult>(
        profile_, list_controller_, controller_.get(), match));
  }
  SwapResults(&new_results);
}

void OmniboxProvider::OnResultChanged(bool default_match_changed) {
  PopulateFromACResult(controller_->result());
}

}  // namespace app_list
