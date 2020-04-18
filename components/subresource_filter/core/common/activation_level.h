// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_LEVEL_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_LEVEL_H_

#include <iosfwd>

namespace subresource_filter {

// Degrees to which the subresource filtering mechanism can be activated.
enum class ActivationLevel {
  DISABLED,
  // Subresource loads are matched against filtering rules, but all loads are
  // allowed to proceed regardless. Used for stability and performance testing.
  DRYRUN,
  ENABLED,
  LAST = ENABLED,
};

// For logging use only.
std::ostream& operator<<(std::ostream& os, const ActivationLevel& level);

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_LEVEL_H_
