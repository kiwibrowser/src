// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_provider_logos/switches.h"

namespace search_provider_logos {
namespace switches {

// Overrides the URL used to fetch the current Google Doodle.
// Example: https://www.google.com/async/ddljson
const char kGoogleDoodleUrl[] = "google-doodle-url";

// Use a static URL for the logo of the default search engine.
// Example: https://www.google.com/branding/logo.png
const char kSearchProviderLogoURL[] = "search-provider-logo-url";

// Overrides the Doodle URL to use for third-party search engines.
const char kThirdPartyDoodleURL[] = "third-party-doodle-url";

}  // namespace switches
}  // namespace search_provider_logos
