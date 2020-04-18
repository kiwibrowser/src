// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_METAHANDLE_SET_H_
#define COMPONENTS_SYNC_SYNCABLE_METAHANDLE_SET_H_

#include <stdint.h>

#include <set>

namespace syncer {
namespace syncable {

using MetahandleSet = std::set<int64_t>;

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_METAHANDLE_SET_H_
