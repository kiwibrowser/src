// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SEARCH_INSTANT_TEST_BASE_H_
#define CHROME_BROWSER_UI_SEARCH_INSTANT_TEST_BASE_H_

#include <string>

#include "base/macros.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

class Browser;

// This utility class is meant to be used in a "mix-in" fashion, giving the
// derived test class additional Instant-related functionality.
class InstantTestBase {
 protected:
  InstantTestBase();
  virtual ~InstantTestBase();

 protected:
  Browser* instant_browser() { return browser_; }

  void SetupInstant(Browser* browser);
  void Init(const GURL& base_url,
            const GURL& ntp_url,
            bool init_suggestions_url);

  const GURL& base_url() const { return base_url_; }

  const GURL& ntp_url() const { return ntp_url_; }

  net::EmbeddedTestServer& https_test_server() { return https_test_server_; }

 private:
  GURL base_url_;
  GURL ntp_url_;

  Browser* browser_;

  // HTTPS Testing server, started on demand.
  net::EmbeddedTestServer https_test_server_;

  // Set to true to initialize suggestions URL in default search provider.
  bool init_suggestions_url_;

  DISALLOW_COPY_AND_ASSIGN(InstantTestBase);
};

#endif  // CHROME_BROWSER_UI_SEARCH_INSTANT_TEST_BASE_H_
