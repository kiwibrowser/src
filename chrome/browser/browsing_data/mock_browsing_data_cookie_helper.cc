// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/mock_browsing_data_cookie_helper.h"

#include <memory>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "net/cookies/cookie_options.h"
#include "testing/gtest/include/gtest/gtest.h"

MockBrowsingDataCookieHelper::MockBrowsingDataCookieHelper(
    net::URLRequestContextGetter* request_context_getter)
    : BrowsingDataCookieHelper(request_context_getter) {
}

MockBrowsingDataCookieHelper::~MockBrowsingDataCookieHelper() {
}

void MockBrowsingDataCookieHelper::StartFetching(
    const FetchCallback& callback) {
  ASSERT_FALSE(callback.is_null());
  ASSERT_TRUE(callback_.is_null());
  callback_ = callback;
}

void MockBrowsingDataCookieHelper::DeleteCookie(
    const net::CanonicalCookie& cookie) {
  ASSERT_FALSE(callback_.is_null());
  std::string key = cookie.Name() + "=" + cookie.Value();
  ASSERT_TRUE(base::ContainsKey(cookies_, key));
  cookies_[key] = false;
}

void MockBrowsingDataCookieHelper::AddCookieSamples(
    const GURL& url, const std::string& cookie_line) {
  std::unique_ptr<net::CanonicalCookie> cc(net::CanonicalCookie::Create(
      url, cookie_line, base::Time::Now(), net::CookieOptions()));

  if (cc.get()) {
    for (const auto& cookie : cookie_list_) {
      if (cookie.Name() == cc->Name() && cookie.Domain() == cc->Domain() &&
          cookie.Path() == cc->Path()) {
        return;
      }
    }
    cookie_list_.push_back(*cc);
    cookies_[cookie_line] = true;
  }
}

void MockBrowsingDataCookieHelper::Notify() {
  if (!callback_.is_null())
    callback_.Run(cookie_list_);
}

void MockBrowsingDataCookieHelper::Reset() {
  for (auto& pair : cookies_)
    pair.second = true;
}

bool MockBrowsingDataCookieHelper::AllDeleted() {
  for (const auto& pair : cookies_) {
    if (pair.second)
      return false;
  }
  return true;
}
