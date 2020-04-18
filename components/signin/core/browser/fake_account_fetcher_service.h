// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_FAKE_ACCOUNT_FETCHER_SERVICE_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_FAKE_ACCOUNT_FETCHER_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/signin/core/browser/account_fetcher_service.h"

class KeyedService;

// AccountTrackerService is a KeyedService that retrieves and caches GAIA
// information about Google Accounts.  This fake class can be used in tests
// to prevent AccountTrackerService from sending network requests.
class FakeAccountFetcherService : public AccountFetcherService {
 public:
  void FakeUserInfoFetchSuccess(const std::string& email,
                                const std::string& gaia,
                                const std::string& hosted_domain,
                                const std::string& full_name,
                                const std::string& given_name,
                                const std::string& locale,
                                const std::string& picture_url);
  void FakeUserInfoFetchSuccess(const std::string& account_id,
                                const std::string& email,
                                const std::string& gaia,
                                const std::string& hosted_domain,
                                const std::string& full_name,
                                const std::string& given_name,
                                const std::string& locale,
                                const std::string& picture_url);
  void FakeSetIsChildAccount(const std::string& account_id,
                             const bool& is_child_account);

  FakeAccountFetcherService();

 private:
  void StartFetchingUserInfo(const std::string& account_id) override;
  void StartFetchingChildInfo(const std::string& account_id) override;

  DISALLOW_COPY_AND_ASSIGN(FakeAccountFetcherService);
};

// This dummy class implements |image_fetcher::ImageDecoder|, and is passed
// as an argument to |AccountFetcherService::Initialize|.
class TestImageDecoder : public image_fetcher::ImageDecoder {
 public:
  TestImageDecoder();
  ~TestImageDecoder() override;

  // image_fetcher::Decoder implementation:

  // If |image_data| is non-empty, a blank 64x64 image is passed to callback.
  // Otherwise an empty image is passed.
  void DecodeImage(
      const std::string& image_data,
      const gfx::Size& desired_image_frame_size,
      const image_fetcher::ImageDecodedCallback& callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestImageDecoder);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_FAKE_ACCOUNT_FETCHER_SERVICE_H_
