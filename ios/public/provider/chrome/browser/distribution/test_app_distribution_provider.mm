// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/distribution/test_app_distribution_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TestAppDistributionProvider::TestAppDistributionProvider() {}

TestAppDistributionProvider::~TestAppDistributionProvider() {}

std::string TestAppDistributionProvider::GetDistributionBrandCode() {
  return std::string();
}

void TestAppDistributionProvider::ScheduleDistributionNotifications(
    net::URLRequestContextGetter* context,
    bool is_first_run) {}

void TestAppDistributionProvider::CancelDistributionNotifications() {}
