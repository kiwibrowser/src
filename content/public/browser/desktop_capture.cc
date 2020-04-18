// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/desktop_capture.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "content/public/common/content_features.h"

namespace content {
namespace desktop_capture {

webrtc::DesktopCaptureOptions CreateDesktopCaptureOptions() {
  auto options = webrtc::DesktopCaptureOptions::CreateDefault();
  // Leave desktop effects enabled during WebRTC captures.
  options.set_disable_effects(false);
#if defined(OS_WIN)
  static constexpr base::Feature kDirectXCapturer{
      "DirectXCapturer",
      base::FEATURE_ENABLED_BY_DEFAULT};
  if (base::FeatureList::IsEnabled(kDirectXCapturer)) {
    options.set_allow_directx_capturer(true);
    options.set_allow_use_magnification_api(false);
  } else {
    options.set_allow_use_magnification_api(true);
  }
#elif defined(OS_MACOSX)
  if (base::FeatureList::IsEnabled(features::kIOSurfaceCapturer)) {
    options.set_allow_iosurface(true);
  }
#endif
  return options;
}

std::unique_ptr<webrtc::DesktopCapturer> CreateScreenCapturer() {
  return webrtc::DesktopCapturer::CreateScreenCapturer(
      CreateDesktopCaptureOptions());
}

std::unique_ptr<webrtc::DesktopCapturer> CreateWindowCapturer() {
  return webrtc::DesktopCapturer::CreateWindowCapturer(
      CreateDesktopCaptureOptions());
}

}  // namespace desktop_capture
}  // namespace content
