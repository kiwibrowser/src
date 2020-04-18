// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/win/evaluate_d3d.h"

#include <D3DCommon.h>

#include <iostream>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "remoting/host/evaluate_capability.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/switches.h"
#include "third_party/webrtc/modules/desktop_capture/win/dxgi_duplicator_controller.h"
#include "third_party/webrtc/modules/desktop_capture/win/screen_capturer_win_directx.h"

namespace remoting {

namespace {

constexpr char kNoDirectXCapturer[] = "No-DirectX-Capturer";

}  // namespace

int EvaluateD3D() {
  // Creates a capturer instance to avoid the DxgiDuplicatorController to be
  // initialized and deinitialized for each static call to
  // webrtc::ScreenCapturerWinDirectx below.
  webrtc::ScreenCapturerWinDirectx capturer;

  if (webrtc::ScreenCapturerWinDirectx::IsSupported()) {
    // Guaranteed to work.
    // This string is also hard-coded in host_attributes_unittests.cc.
    std::cout << "DirectX-Capturer" << std::endl;
  } else if (webrtc::ScreenCapturerWinDirectx::IsCurrentSessionSupported()) {
    // If we are in a supported session, but DirectX capturer is not able to be
    // initialized. Something must be wrong, we should actively disable it.
    std::cout << kNoDirectXCapturer << std::endl;
  }

  webrtc::DxgiDuplicatorController::D3dInfo info;
  webrtc::ScreenCapturerWinDirectx::RetrieveD3dInfo(&info);
  if (info.min_feature_level < D3D_FEATURE_LEVEL_10_0) {
    std::cout << "MinD3DLT10" << std::endl;
  } else {
    std::cout << "MinD3DGE10" << std::endl;
  }
  if (info.min_feature_level >= D3D_FEATURE_LEVEL_11_0) {
    std::cout << "MinD3DGE11" << std::endl;
  }
  if (info.min_feature_level >= D3D_FEATURE_LEVEL_12_0) {
    std::cout << "MinD3DGE12" << std::endl;
  }

  return kSuccessExitCode;
}

bool GetD3DCapability(std::vector<std::string>* result /* = nullptr */) {
  std::string d3d_info;
  if (EvaluateCapability(kEvaluateD3D, &d3d_info) != kSuccessExitCode) {
    if (result) {
      result->push_back(kNoDirectXCapturer);
    }
    return false;
  }

  if (result) {
    auto capabilities = base::SplitString(
        d3d_info,
        base::kWhitespaceASCII,
        base::TRIM_WHITESPACE,
        base::SPLIT_WANT_NONEMPTY);
    for (const auto& capability : capabilities) {
      result->push_back(capability);
    }
  }
  return true;
}

}  // namespace remoting
