// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_ANDROID_VIDEO_CAPTURE_DEVICE_FACTORY_ANDROID_H_
#define MEDIA_CAPTURE_VIDEO_ANDROID_VIDEO_CAPTURE_DEVICE_FACTORY_ANDROID_H_

#include "media/capture/video/video_capture_device_factory.h"

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "media/capture/video/video_capture_device.h"

namespace media {

// VideoCaptureDeviceFactory on Android. This class implements the static
// VideoCapture methods and the factory of VideoCaptureAndroid.
class CAPTURE_EXPORT VideoCaptureDeviceFactoryAndroid
    : public VideoCaptureDeviceFactory {
 public:
  static base::android::ScopedJavaLocalRef<jobject> createVideoCaptureAndroid(
      int id,
      jlong nativeVideoCaptureDeviceAndroid);

  VideoCaptureDeviceFactoryAndroid() : test_mode_(false) {}
  ~VideoCaptureDeviceFactoryAndroid() override {}

  std::unique_ptr<VideoCaptureDevice> CreateDevice(
      const VideoCaptureDeviceDescriptor& device_descriptor) override;
  void GetDeviceDescriptors(
      VideoCaptureDeviceDescriptors* device_descriptors) override;
  void GetSupportedFormats(const VideoCaptureDeviceDescriptor& device,
                           VideoCaptureFormats* supported_formats) override;

  static bool IsLegacyOrDeprecatedDevice(const std::string& device_id);

  // Configures all subsequent CreateDevice()s in test mode.
  void ConfigureForTesting() { test_mode_ = true; }

 private:
  // Switch to indicate that all created Java capturers will be in test mode.
  bool test_mode_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureDeviceFactoryAndroid);
};
}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_ANDROID_VIDEO_CAPTURE_DEVICE_FACTORY_ANDROID_H_
