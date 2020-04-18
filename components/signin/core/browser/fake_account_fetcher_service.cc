// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/fake_account_fetcher_service.h"

#include "base/values.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "ui/gfx/image/image_unittest_util.h"

FakeAccountFetcherService::FakeAccountFetcherService() {}

void FakeAccountFetcherService::FakeUserInfoFetchSuccess(
    const std::string& account_id,
    const std::string& email,
    const std::string& gaia,
    const std::string& hosted_domain,
    const std::string& full_name,
    const std::string& given_name,
    const std::string& locale,
    const std::string& picture_url) {
  base::DictionaryValue user_info;
  user_info.SetString("id", gaia);
  user_info.SetString("email", email);
  user_info.SetString("hd", hosted_domain);
  user_info.SetString("name", full_name);
  user_info.SetString("given_name", given_name);
  user_info.SetString("locale", locale);
  user_info.SetString("picture", picture_url);
  account_tracker_service()->SetAccountStateFromUserInfo(account_id,
                                                         &user_info);
}

void FakeAccountFetcherService::FakeSetIsChildAccount(
    const std::string& account_id,
    const bool& is_child_account) {
  SetIsChildAccount(account_id, is_child_account);
}

void FakeAccountFetcherService::StartFetchingUserInfo(
    const std::string& account_id) {
  // In tests, don't do actual network fetch.
}

void FakeAccountFetcherService::StartFetchingChildInfo(
    const std::string& account_id) {
  // In tests, don't do actual network fetch.
}

TestImageDecoder::TestImageDecoder() = default;

TestImageDecoder::~TestImageDecoder() = default;

void TestImageDecoder::DecodeImage(
    const std::string& image_data,
    const gfx::Size& desired_image_frame_size,
    const image_fetcher::ImageDecodedCallback& callback) {
  callback.Run(image_data.empty() ? gfx::Image()
                                  : gfx::test::CreateImage(64, 64));
}
