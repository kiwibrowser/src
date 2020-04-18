// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/navigation_state_impl.h"

namespace content {

NavigationStateImpl::~NavigationStateImpl() {
}

NavigationStateImpl* NavigationStateImpl::CreateBrowserInitiated(
    const CommonNavigationParams& common_params,
    const RequestNavigationParams& request_params,
    base::TimeTicks time_commit_requested) {
  return new NavigationStateImpl(common_params, request_params,
                                 time_commit_requested, false);
}

NavigationStateImpl* NavigationStateImpl::CreateContentInitiated() {
  return new NavigationStateImpl(CommonNavigationParams(),
                                 RequestNavigationParams(), base::TimeTicks(),
                                 true);
}

ui::PageTransition NavigationStateImpl::GetTransitionType() {
  return common_params_.transition;
}

bool NavigationStateImpl::WasWithinSameDocument() {
  return was_within_same_document_;
}

bool NavigationStateImpl::IsContentInitiated() {
  return is_content_initiated_;
}

NavigationStateImpl::NavigationStateImpl(
    const CommonNavigationParams& common_params,
    const RequestNavigationParams& request_params,
    base::TimeTicks time_commit_requested,
    bool is_content_initiated)
    : request_committed_(false),
      was_within_same_document_(false),
      is_content_initiated_(is_content_initiated),
      common_params_(common_params),
      request_params_(request_params),
      time_commit_requested_(time_commit_requested) {}

}  // namespace content
