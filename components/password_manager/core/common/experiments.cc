// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/common/experiments.h"

#include "components/password_manager/core/common/password_manager_features.h"

namespace password_manager {

bool ForceSavingExperimentEnabled() {
  return base::FeatureList::IsEnabled(
      password_manager::features::kPasswordForceSaving);
}

}  // namespace password_manager
