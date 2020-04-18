// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_DISTRIBUTION_TEST_APP_DISTRIBUTION_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_DISTRIBUTION_TEST_APP_DISTRIBUTION_PROVIDER_H_

#include "base/macros.h"
#import "ios/public/provider/chrome/browser/distribution/app_distribution_provider.h"

class TestAppDistributionProvider : public AppDistributionProvider {
 public:
  TestAppDistributionProvider();
  ~TestAppDistributionProvider() override;

  // AppDistributionProvider.
  std::string GetDistributionBrandCode() override;
  void ScheduleDistributionNotifications(net::URLRequestContextGetter* context,
                                         bool is_first_run) override;
  void CancelDistributionNotifications() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAppDistributionProvider);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_DISTRIBUTION_TEST_APP_DISTRIBUTION_PROVIDER_H_
