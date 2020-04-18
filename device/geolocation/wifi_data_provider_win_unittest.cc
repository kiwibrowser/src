// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Most logic for the platform wifi provider is now factored into
// WifiDataProviderCommon and covered by it's unit tests.

#include "base/message_loop/message_loop.h"
#include "device/geolocation/wifi_data_provider_win.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(GeolocationWifiDataProviderWinTest, CreateDestroy) {
  // WifiDataProvider requires a task runner to be present.
  base::MessageLoopForUI message_loop_;
  scoped_refptr<WifiDataProviderWin> instance(new WifiDataProviderWin);
  instance = NULL;
  SUCCEED();
  // Can't actually call start provider on the WifiDataProviderWin without
  // it accessing hardware and so risking making the test flaky.
}

}  // namespace device
