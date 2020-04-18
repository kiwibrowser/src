// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_TEST_BROWSER_STATE_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_TEST_BROWSER_STATE_H_

#include "base/memory/ref_counted.h"
#include "ios/web/public/browser_state.h"

namespace web {
class TestBrowserState : public BrowserState {
 public:
  TestBrowserState();
  ~TestBrowserState() override;

  // BrowserState:
  bool IsOffTheRecord() const override;
  base::FilePath GetStatePath() const override;
  net::URLRequestContextGetter* GetRequestContext() override;

  // Makes |IsOffTheRecord| return the given flag value.
  void SetOffTheRecord(bool flag);

 private:
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  bool is_off_the_record_;
};
}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_TEST_BROWSER_STATE_H_
