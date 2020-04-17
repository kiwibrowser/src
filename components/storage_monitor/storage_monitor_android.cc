// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/storage_monitor/storage_monitor.h"

namespace storage_monitor {
// TODO(achaulk): Remove this code when APIs are trimmed.
StorageMonitor* StorageMonitor::CreateInternal() {
  return nullptr;
}
}  // namespace storage_monitor
