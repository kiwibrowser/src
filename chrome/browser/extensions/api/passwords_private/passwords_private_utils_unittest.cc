// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/passwords_private/passwords_private_utils.h"

#include "components/autofill/core/common/password_form.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

TEST(CreateUrlCollectionFromFormTest, UrlsFromHtmlForm) {
  autofill::PasswordForm html_form;
  html_form.origin = GURL("http://example.com/LoginAuth");
  html_form.signon_realm = html_form.origin.GetOrigin().spec();

  api::passwords_private::UrlCollection html_urls =
      CreateUrlCollectionFromForm(html_form);
  EXPECT_EQ(html_urls.origin, "http://example.com/");
  EXPECT_EQ(html_urls.shown, "example.com");
  EXPECT_EQ(html_urls.link, "http://example.com/LoginAuth");
}

TEST(CreateUrlCollectionFromFormTest, UrlsFromFederatedForm) {
  autofill::PasswordForm federated_form;
  federated_form.signon_realm = "federation://example.com/google.com";
  federated_form.origin = GURL("https://example.com/");
  federated_form.federation_origin =
      url::Origin::Create(GURL("https://google.com/"));

  api::passwords_private::UrlCollection federated_urls =
      CreateUrlCollectionFromForm(federated_form);
  EXPECT_EQ(federated_urls.origin, "federation://example.com/google.com");
  EXPECT_EQ(federated_urls.shown, "example.com");
  EXPECT_EQ(federated_urls.link, "https://example.com/");
}

TEST(CreateUrlCollectionFromFormTest, UrlsFromAndroidFormWithoutDisplayName) {
  autofill::PasswordForm android_form;
  android_form.signon_realm = "android://example@com.example.android";
  android_form.app_display_name.clear();

  api::passwords_private::UrlCollection android_urls =
      CreateUrlCollectionFromForm(android_form);
  EXPECT_EQ("android://example@com.example.android", android_urls.origin);
  EXPECT_EQ("android.example.com", android_urls.shown);
  EXPECT_EQ("https://play.google.com/store/apps/details?id=com.example.android",
            android_urls.link);
}

TEST(CreateUrlCollectionFromFormTest, UrlsFromAndroidFormWithAppName) {
  autofill::PasswordForm android_form;
  android_form.signon_realm = "android://hash@com.example.android";
  android_form.app_display_name = "Example Android App";

  api::passwords_private::UrlCollection android_urls =
      CreateUrlCollectionFromForm(android_form);
  EXPECT_EQ(android_urls.origin, "android://hash@com.example.android");
  EXPECT_EQ("Example Android App", android_urls.shown);
  EXPECT_EQ("https://play.google.com/store/apps/details?id=com.example.android",
            android_urls.link);
}

}  // namespace extensions
