// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_CYCLE_STATUS_COUNTERS_H_
#define COMPONENTS_SYNC_ENGINE_CYCLE_STATUS_COUNTERS_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/values.h"

namespace syncer {

// A class to maintain counts related to the current status of a sync type.
struct StatusCounters {
  StatusCounters();
  ~StatusCounters();

  std::unique_ptr<base::DictionaryValue> ToValue() const;
  std::string ToString() const;

  size_t num_entries;
  size_t num_entries_and_tombstones;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_CYCLE_STATUS_COUNTERS_H_
