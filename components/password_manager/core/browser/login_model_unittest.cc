// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/login_model.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using autofill::PasswordForm;
using base::ASCIIToUTF16;
using testing::_;

namespace password_manager {

namespace {

// Mocking the class under test is fine, because the mocked methods and the
// tested methods are disjoint. In particular, the virtual methods of
// LoginModelObserver are all pure, so what can be tested are only non-virtual
// (and hence non-mockable) methods.
class MockLoginModelObserver : public LoginModelObserver {
 public:
  MOCK_METHOD2(OnAutofillDataAvailableInternal,
               void(const base::string16& username,
                    const base::string16& password));

 private:
  void OnLoginModelDestroying() override {}
};

PasswordForm CreateFormWithRealm(const std::string& realm) {
  PasswordForm form;
  form.origin = GURL("http://accounts.google.com/a/LoginAuth");
  form.action = GURL("http://accounts.google.com/a/Login");
  form.username_element = ASCIIToUTF16("Email");
  form.password_element = ASCIIToUTF16("Passwd");
  form.username_value = ASCIIToUTF16("expected username");
  form.password_value = ASCIIToUTF16("expected password");
  form.signon_realm = realm;
  return form;
}

}  // namespace

TEST(LoginModelObserverTest, FilterNotMatchingRealm) {
  MockLoginModelObserver observer;
  observer.set_signon_realm("http://example.com/");
  EXPECT_CALL(observer, OnAutofillDataAvailableInternal(_, _)).Times(0);
  PasswordForm form = CreateFormWithRealm("http://test.org/");
  observer.OnAutofillDataAvailable(form);
}

TEST(LoginModelObserverTest, NotFilterMatchingRealm) {
  MockLoginModelObserver observer;
  observer.set_signon_realm("http://example.com/");
  EXPECT_CALL(observer, OnAutofillDataAvailableInternal(
                            ASCIIToUTF16("expected username"),
                            ASCIIToUTF16("expected password")));
  PasswordForm form = CreateFormWithRealm("http://example.com/");
  observer.OnAutofillDataAvailable(form);
}

}  // namespace password_manager
