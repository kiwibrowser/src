// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ASSETS_COMPONENT_UPDATE_STATUS_H_
#define CHROME_BROWSER_VR_ASSETS_COMPONENT_UPDATE_STATUS_H_

namespace vr {

// Status of the assets component after an update.
//
// Keep this enum aligned with VRAssetsComponentUpdateStatus in
// //tools/metrics/histograms/enums.xml.
// If you rename this file update the
// reference in
// //tools/metrics/histograms/histograms.xml.
enum class AssetsComponentUpdateStatus : int {
  kSuccess = 0,       // Component was installed successfully.
  kInvalid = 1,       // Failed to verify component.
  kIncompatible = 2,  // Component version is not compatible with Chrome.
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ASSETS_COMPONENT_UPDATE_STATUS_H_
