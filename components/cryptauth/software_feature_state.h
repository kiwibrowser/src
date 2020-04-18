// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_STATE_H_
#define COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_STATE_H_

namespace cryptauth {

enum class SoftwareFeatureState {
  kNotSupported = 0,
  kSupported = 1,
  kEnabled = 2
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_STATE_H_