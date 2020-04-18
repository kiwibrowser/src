// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_CHILD_ACCOUNT_INFO_FETCHER_ANDROID_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_CHILD_ACCOUNT_INFO_FETCHER_ANDROID_H_

#include <jni.h>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "components/signin/core/browser/child_account_info_fetcher.h"

class AccountFetcherService;

class ChildAccountInfoFetcherAndroid : public ChildAccountInfoFetcher {
 public:
  static std::unique_ptr<ChildAccountInfoFetcher> Create(
      AccountFetcherService* service,
      const std::string& account_id);

  static void InitializeForTests();

 private:
  ChildAccountInfoFetcherAndroid(AccountFetcherService* service,
                                 const std::string& account_id,
                                 const std::string& account_name);
  ~ChildAccountInfoFetcherAndroid() override;

 private:
  base::android::ScopedJavaGlobalRef<jobject> j_child_account_info_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(ChildAccountInfoFetcherAndroid);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_CHILD_ACCOUNT_INFO_FETCHER_ANDROID_H_
