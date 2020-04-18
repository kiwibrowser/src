// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "net/url_request/url_request.h"
#include "third_party/metrics_proto/omnibox_input_type.pb.h"
#include "url/url_constants.h"

TestSchemeClassifier::TestSchemeClassifier() {}

TestSchemeClassifier::~TestSchemeClassifier() {}

metrics::OmniboxInputType TestSchemeClassifier::GetInputTypeForScheme(
    const std::string& scheme) const {
  // This doesn't check the preference but check some chrome-ish schemes.
  const char* kKnownURLSchemes[] = {
    url::kFileScheme, url::kAboutScheme, url::kFtpScheme, url::kBlobScheme,
    url::kFileSystemScheme, "view-source", "javascript", "chrome", "chrome-ui",
  };
  for (size_t i = 0; i < arraysize(kKnownURLSchemes); ++i) {
    if (scheme == kKnownURLSchemes[i])
      return metrics::OmniboxInputType::URL;
  }
  if (net::URLRequest::IsHandledProtocol(scheme))
    return metrics::OmniboxInputType::URL;

  return metrics::OmniboxInputType::INVALID;
}
