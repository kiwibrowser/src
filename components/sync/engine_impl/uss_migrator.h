// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_USS_MIGRATOR_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_USS_MIGRATOR_H_

#include "base/callback.h"
#include "components/sync/base/model_type.h"

namespace syncer {

class ModelTypeWorker;
struct UserShare;

using UssMigrator =
    base::Callback<bool(ModelType, UserShare*, ModelTypeWorker*)>;

// Pulls all the data for |type| out of the directory and sends it to |worker|
// as the result of an initial GetUpdates. Returns whether migration succeeded.
bool MigrateDirectoryData(ModelType type,
                          UserShare* user_share,
                          ModelTypeWorker* worker);

// A version of the above with |batch_size| as a parameter so it can be lowered
// for unit testing.
bool MigrateDirectoryDataWithBatchSize(ModelType type,
                                       UserShare* user_share,
                                       ModelTypeWorker* worker,
                                       int batch_size);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_USS_MIGRATOR_H_
