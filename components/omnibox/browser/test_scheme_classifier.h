// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_TEST_SCHEME_CLASSIFIER_H_
#define COMPONENTS_OMNIBOX_BROWSER_TEST_SCHEME_CLASSIFIER_H_

#include <string>

#include "base/macros.h"
#include "components/omnibox/browser/autocomplete_scheme_classifier.h"

// The subclass of AutocompleteSchemeClassifier for testing.
class TestSchemeClassifier : public AutocompleteSchemeClassifier {
 public:
  TestSchemeClassifier();
  ~TestSchemeClassifier() override;

  // Overridden from AutocompleteInputSchemeChecker:
  metrics::OmniboxInputType GetInputTypeForScheme(
      const std::string& scheme) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestSchemeClassifier);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_TEST_SCHEME_CLASSIFIER_H_
