// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_LOAD_POLICY_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_LOAD_POLICY_H_

namespace subresource_filter {

// Represents the value returned by the DocumentSubresourceFilter corresponding
// to a resource load.
enum class LoadPolicy {
  ALLOW,
  DISALLOW,
  WOULD_DISALLOW,
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_LOAD_POLICY_H_
