// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_OMAHA_TEST_OMAHA_SERVICE_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_OMAHA_TEST_OMAHA_SERVICE_PROVIDER_H_

#include "ios/public/provider/chrome/browser/omaha/omaha_service_provider.h"

class TestOmahaServiceProvider : public OmahaServiceProvider {
 public:
  TestOmahaServiceProvider();
  ~TestOmahaServiceProvider() override;

  // OmahaServiceProvider.
  void Initialize() override;
  GURL GetUpdateServerURL() const override;
  std::string GetApplicationID() const override;
  std::string GetBrandCode() const override;
  void AppendExtraAttributes(const std::string& tag,
                             OmahaXmlWriter* writer) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestOmahaServiceProvider);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_OMAHA_TEST_OMAHA_SERVICE_PROVIDER_H_
