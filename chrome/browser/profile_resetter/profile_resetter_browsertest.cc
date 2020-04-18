// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profile_resetter/profile_resetter.h"

#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/profile_resetter/profile_resetter_test_base.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_utils.h"
#include "net/cookies/canonical_cookie.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace {

const char kCookieName[] = "A";
const char kCookieValue[] = "1";
const char kCookieHostname[] = "host1.com";

using content::BrowserThread;

// RemoveCookieTester provides the user with the ability to set and get a
// cookie for given profile.
class RemoveCookieTester {
 public:
  explicit RemoveCookieTester(Profile* profile);
  ~RemoveCookieTester();

  bool GetCookie(const std::string& host, net::CanonicalCookie* cookie);
  void AddCookie(const std::string& host,
                 const std::string& name,
                 const std::string& value);

 private:
  void GetCookieListCallback(const std::vector<net::CanonicalCookie>& cookies);
  void SetCanonicalCookieCallback(bool result);

  void BlockUntilNotified();
  void Notify();

  std::vector<net::CanonicalCookie> last_cookies_;
  bool waiting_callback_;
  Profile* profile_;
  network::mojom::CookieManagerPtr cookie_manager_;
  scoped_refptr<content::MessageLoopRunner> runner_;

  DISALLOW_COPY_AND_ASSIGN(RemoveCookieTester);
};

RemoveCookieTester::RemoveCookieTester(Profile* profile)
    : waiting_callback_(false),
      profile_(profile) {
  network::mojom::NetworkContext* network_context =
      content::BrowserContext::GetDefaultStoragePartition(profile_)
          ->GetNetworkContext();
  network_context->GetCookieManager(mojo::MakeRequest(&cookie_manager_));
}

RemoveCookieTester::~RemoveCookieTester() {}

// Returns true and sets |*cookie| if the given cookie exists in
// the cookie store.
bool RemoveCookieTester::GetCookie(const std::string& host,
                                   net::CanonicalCookie* cookie) {
  last_cookies_.clear();
  DCHECK(!waiting_callback_);
  waiting_callback_ = true;
  net::CookieOptions cookie_options;
  cookie_manager_->GetCookieList(
      GURL("http://" + host + "/"), cookie_options,
      base::BindOnce(&RemoveCookieTester::GetCookieListCallback,
                     base::Unretained(this)));
  BlockUntilNotified();
  DCHECK_GE(1u, last_cookies_.size());
  if (0u == last_cookies_.size())
    return false;
  *cookie = last_cookies_[0];
  return true;
}

void RemoveCookieTester::AddCookie(const std::string& host,
                                   const std::string& name,
                                   const std::string& value) {
  DCHECK(!waiting_callback_);
  waiting_callback_ = true;
  cookie_manager_->SetCanonicalCookie(
      net::CanonicalCookie(name, value, host, "/", base::Time(), base::Time(),
                           base::Time(), false, false,
                           net::CookieSameSite::NO_RESTRICTION,
                           net::COOKIE_PRIORITY_MEDIUM),
      false /* secure_source */, true /* modify_http_only */,
      base::BindOnce(&RemoveCookieTester::SetCanonicalCookieCallback,
                     base::Unretained(this)));
  BlockUntilNotified();
}

void RemoveCookieTester::GetCookieListCallback(
    const std::vector<net::CanonicalCookie>& cookies) {
  last_cookies_ = cookies;
  Notify();
}

void RemoveCookieTester::SetCanonicalCookieCallback(bool result) {
  ASSERT_TRUE(result);
  Notify();
}

void RemoveCookieTester::BlockUntilNotified() {
  DCHECK(!runner_.get());
  if (waiting_callback_) {
    runner_ = new content::MessageLoopRunner;
    runner_->Run();
    runner_ = NULL;
  }
}

void RemoveCookieTester::Notify() {
  DCHECK(waiting_callback_);
  waiting_callback_ = false;
  if (runner_.get())
    runner_->Quit();
}

class ProfileResetTest : public InProcessBrowserTest,
                         public ProfileResetterTestBase {
 protected:
  void SetUpOnMainThread() override {
    resetter_.reset(new ProfileResetter(browser()->profile()));
  }
};


IN_PROC_BROWSER_TEST_F(ProfileResetTest, ResetCookiesAndSiteData) {
  RemoveCookieTester tester(browser()->profile());
  std::string host_prefix("http://");
  tester.AddCookie(kCookieHostname, kCookieName, kCookieValue);
  net::CanonicalCookie cookie;
  ASSERT_TRUE(tester.GetCookie(kCookieHostname, &cookie));
  EXPECT_EQ(kCookieName, cookie.Name());
  EXPECT_EQ(kCookieValue, cookie.Value());

  ResetAndWait(ProfileResetter::COOKIES_AND_SITE_DATA);

  EXPECT_FALSE(tester.GetCookie(host_prefix + kCookieHostname, &cookie));
}

}  // namespace
