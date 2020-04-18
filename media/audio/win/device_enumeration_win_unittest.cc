// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/win/device_enumeration_win.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(DeviceEnumerationWin, GetUsbVidPidSuffix) {
  // Some real-world USB devices
  EXPECT_EQ(
      GetUsbVidPidSuffixWin("USB\\VID_046D&PID_09A6&MI_02\\6&318d810e&1&0002"),
      " (046d:09a6)");
  EXPECT_EQ(GetUsbVidPidSuffixWin("USB\\VID_8087&PID_07DC&REV_0001"),
            " (8087:07dc)");
  EXPECT_EQ(GetUsbVidPidSuffixWin("USB\\VID_0403&PID_6010"), " (0403:6010)");

  // Some real-world non-USB devices
  EXPECT_TRUE(
      GetUsbVidPidSuffixWin("BTHHFENUM\\BthHFPAudio\\8&39e29755&0&97").empty());
  EXPECT_TRUE(GetUsbVidPidSuffixWin("BTHENUM\\{0000110b-0000-1000-8000-"
                                    "00805f9b34fb}_LOCALMFG&0002\\7&25f92e87&0&"
                                    "70886B900BB0_C00000000")
                  .empty());
  EXPECT_TRUE(
      GetUsbVidPidSuffixWin("INTELAUDIO\\FUNC_01&VEN_8086&DEV_280B&SUBSYS_"
                            "80860101&REV_1000\\4&c083774&0&0201")
          .empty());
  EXPECT_TRUE(
      GetUsbVidPidSuffixWin("INTELAUDIO\\FUNC_01&VEN_10EC&DEV_0298&SUBSYS_"
                            "102807BF&REV_1001\\4&c083774&0&0001")
          .empty());
  EXPECT_TRUE(GetUsbVidPidSuffixWin(
                  "PCI\\VEN_1000&DEV_0001&SUBSYS_00000000&REV_02\\1&08")
                  .empty());

  // Other input strings.
  EXPECT_TRUE(GetUsbVidPidSuffixWin(std::string()).empty());
  EXPECT_TRUE(GetUsbVidPidSuffixWin("            ").empty());
  EXPECT_TRUE(GetUsbVidPidSuffixWin("USBVID_1234&PID1234").empty());
}

}  // namespace media
