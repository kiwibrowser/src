// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARD_REASON_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARD_REASON_H_

namespace resource_coordinator {

enum class DiscardReason {
  // The discard is requested from outside of TabManager (e.g. by an extension).
  kExternal,
  // The discard is requested proactively by TabManager when the system is in a
  // good state.
  kProactive,
  // The discard is requested urgently by TabManager when the system is in a
  // critical condition.
  kUrgent,
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARD_REASON_H_
