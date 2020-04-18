// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_COOKIE_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_COOKIE_HELPER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "net/cookies/canonical_cookie.h"

// Mock for BrowsingDataCookieHelper.
class MockBrowsingDataCookieHelper : public BrowsingDataCookieHelper {
 public:
  explicit MockBrowsingDataCookieHelper(
      net::URLRequestContextGetter* request_context_getter);

  // BrowsingDataCookieHelper methods.
  void StartFetching(const FetchCallback& callback) override;
  void DeleteCookie(const net::CanonicalCookie& cookie) override;

  // Adds some cookie samples.
  void AddCookieSamples(const GURL& url, const std::string& cookie_line);

  // Notifies the callback.
  void Notify();

  // Marks all cookies as existing.
  void Reset();

  // Returns true if all cookies since the last Reset() invocation were
  // deleted.
  bool AllDeleted();

 private:
  ~MockBrowsingDataCookieHelper() override;

  FetchCallback callback_;

  net::CookieList cookie_list_;

  // Stores which cookies exist.
  std::map<const std::string, bool> cookies_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowsingDataCookieHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_COOKIE_HELPER_H_
