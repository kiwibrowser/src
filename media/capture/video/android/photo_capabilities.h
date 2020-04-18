// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_ANDROID_PHOTO_CAPABILITIES_H_
#define MEDIA_CAPTURE_VIDEO_ANDROID_PHOTO_CAPABILITIES_H_

#include <jni.h>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"

namespace media {

class PhotoCapabilities {
 public:
  // Metering modes from Java side, equivalent to media.mojom::MeteringMode,
  // except NOT_SET, which is used to signify absence of setting configuration.
  // A Java counterpart will be generated for this enum.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.media
  enum class AndroidMeteringMode {
    NOT_SET,
    NONE,
    FIXED,
    SINGLE_SHOT,
    CONTINUOUS,
  };

  // Fill light modes from Java side, equivalent to media.mojom::FillLightMode,
  // except NOT_SET, which is used to signify absence of setting configuration.
  // A Java counterpart will be generated for this enum.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.media
  enum class AndroidFillLightMode {
    NOT_SET,
    OFF,
    AUTO,
    FLASH,
  };

  explicit PhotoCapabilities(base::android::ScopedJavaLocalRef<jobject> object);
  ~PhotoCapabilities();

  int getMinIso() const;
  int getMaxIso() const;
  int getCurrentIso() const;
  int getStepIso() const;
  int getMinHeight() const;
  int getMaxHeight() const;
  int getCurrentHeight() const;
  int getStepHeight() const;
  int getMinWidth() const;
  int getMaxWidth() const;
  int getCurrentWidth() const;
  int getStepWidth() const;
  double getMinZoom() const;
  double getMaxZoom() const;
  double getCurrentZoom() const;
  double getStepZoom() const;
  AndroidMeteringMode getFocusMode() const;
  std::vector<AndroidMeteringMode> getFocusModes() const;
  AndroidMeteringMode getExposureMode() const;
  std::vector<AndroidMeteringMode> getExposureModes() const;
  double getMinExposureCompensation() const;
  double getMaxExposureCompensation() const;
  double getCurrentExposureCompensation() const;
  double getStepExposureCompensation() const;
  AndroidMeteringMode getWhiteBalanceMode() const;
  std::vector<AndroidMeteringMode> getWhiteBalanceModes() const;
  std::vector<AndroidFillLightMode> getFillLightModes() const;
  bool getSupportsTorch() const;
  bool getTorch() const;
  bool getRedEyeReduction() const;
  int getMinColorTemperature() const;
  int getMaxColorTemperature() const;
  int getCurrentColorTemperature() const;
  int getStepColorTemperature() const;

 private:
  const base::android::ScopedJavaLocalRef<jobject> object_;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_ANDROID_PHOTO_CAPABILITIES_H_
