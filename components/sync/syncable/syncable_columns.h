// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_COLUMNS_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_COLUMNS_H_

#include "base/macros.h"
#include "components/sync/syncable/entry_kernel.h"
#include "components/sync/syncable/syncable_changes_version.h"

namespace syncer {
namespace syncable {

struct ColumnSpec {
  const char* name;
  const char* spec;
};

extern const ColumnSpec g_metas_columns[FIELD_COUNT];

static inline const char* ColumnName(int field) {
  DCHECK(field >= 0 && field < BEGIN_TEMPS);
  return g_metas_columns[field].name;
}

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_COLUMNS_H_
