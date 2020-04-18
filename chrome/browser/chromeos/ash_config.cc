// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ash_config.h"

#include "ui/base/ui_base_features.h"

namespace chromeos {

namespace {

ash::Config ComputeAshConfig() {
  return base::FeatureList::IsEnabled(features::kMash) ? ash::Config::MASH
                                                       : ash::Config::CLASSIC;
}

}  // namespace

ash::Config GetAshConfig() {
  static const ash::Config config = ComputeAshConfig();
  return config;
}

}  // namespace chromeos
