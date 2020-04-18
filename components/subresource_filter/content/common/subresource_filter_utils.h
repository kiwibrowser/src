// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_COMMON_SUBRESOURCE_FILTER_UTILS_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_COMMON_SUBRESOURCE_FILTER_UTILS_H_

class GURL;

namespace subresource_filter {

// Subframe navigations matching these URLs/schemes will not trigger
// ReadyToCommitNavigation in the browser process, so they must be treated
// specially to maintain activation. Should only be invoked for subframes.
bool ShouldUseParentActivation(const GURL& url);

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_COMMON_SUBRESOURCE_FILTER_UTILS_H_
