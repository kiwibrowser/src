// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_DEVICE_CLASSIFIER_UTIL_H_
#define COMPONENTS_CRYPTAUTH_DEVICE_CLASSIFIER_UTIL_H_

#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

namespace device_classifier_util {

const cryptauth::DeviceClassifier& GetDeviceClassifier();

}  // namespace device_classifier_util

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_DEVICE_CLASSIFIER_UTIL_H_
