// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_TEST_OMNIBOX_CLIENT_H_
#define COMPONENTS_OMNIBOX_BROWSER_TEST_OMNIBOX_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/sessions/core/session_id.h"

class AutocompleteSchemeClassifier;

// Fake implementation of OmniboxClient for use in tests.
class TestOmniboxClient : public OmniboxClient {
 public:
  TestOmniboxClient();
  ~TestOmniboxClient() override;

  const AutocompleteMatch& alternate_nav_match() const {
    return alternate_nav_match_;
  }

  // OmniboxClient:
  std::unique_ptr<AutocompleteProviderClient> CreateAutocompleteProviderClient()
      override;
  std::unique_ptr<OmniboxNavigationObserver> CreateOmniboxNavigationObserver(
      const base::string16& text,
      const AutocompleteMatch& match,
      const AutocompleteMatch& alternate_nav_match) override;
  const SessionID& GetSessionID() const override;
  const AutocompleteSchemeClassifier& GetSchemeClassifier() const override;
  AutocompleteClassifier* GetAutocompleteClassifier() override;
  gfx::Image GetSizedIcon(const gfx::VectorIcon& vector_icon_type,
                          SkColor vector_icon_color) const override;

 private:
  AutocompleteMatch alternate_nav_match_;
  SessionID session_id_;
  TestSchemeClassifier scheme_classifier_;
  AutocompleteClassifier autocomplete_classifier_;

  DISALLOW_COPY_AND_ASSIGN(TestOmniboxClient);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_TEST_OMNIBOX_CLIENT_H_
