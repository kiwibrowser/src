// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/qr_scanner/camera_controller.h"

#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using CameraControllerTest = PlatformTest;

TEST_F(CameraControllerTest, TestAVCaptureDeviceValue) {
  // Although Apple documentation claims that AVCaptureDeviceDiscoverySession
  // etc. is available on iOS 10+, they are not really available on an app
  // whose deployment target is iOS 10.0 (iOS 10.1+ are okay) and Chrome will
  // fail at dynamic link time and instantly crash. To make this work, the
  // actual value for the global variable
  // |AVCaptureDeviceTypeBuiltInWideAngleCamera| is used literally in
  // camera_controller.mm. This unit test detects if a newer iOS version
  // changes this value. If this unit test fails, check the implementation
  // of -loadCaptureSession: in camera_controller.mm.
  //
  // The following use of AVCaptureDeviceTypeBuiltInWideAngleCamera
  // will cause unit tests to be not runnable on iOS 10.0 devices due to
  // failure at dynamic link (dyld) time.
  // TODO(crbug.com/826011): Remove this when iOS 10.0 support is deprecated.
  EXPECT_NSEQ(AVCaptureDeviceTypeBuiltInWideAngleCamera,
              @"AVCaptureDeviceTypeBuiltInWideAngleCamera");
}
