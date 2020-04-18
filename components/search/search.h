// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_SEARCH_H_
#define COMPONENTS_SEARCH_SEARCH_H_

namespace search {

// Returns whether the Instant Extended API is enabled. This is always true on
// desktop and false on mobile.
bool IsInstantExtendedAPIEnabled();

}  // namespace search

#endif  // COMPONENTS_SEARCH_SEARCH_H_
