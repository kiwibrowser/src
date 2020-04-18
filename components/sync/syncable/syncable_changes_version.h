// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_CHANGES_VERSION_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_CHANGES_VERSION_H_

namespace syncer {
namespace syncable {

// For the most part, the sync engine treats version numbers as opaque values.
// However, there are parts of our code base that break this abstraction, and
// depend on the following two invariants:
//
//  1.  CHANGES_VERSION is less than 0.
//  2.  The server only issues positive version numbers.
//
// Breaking these abstractions makes some operations 10 times
// faster.  If either of these invariants change, then those queries
// must be revisited.

enum { CHANGES_VERSION = -1 };

#define CHANGES_VERSION_STRING "-1"

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_CHANGES_VERSION_H_
