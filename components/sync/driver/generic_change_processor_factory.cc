// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/generic_change_processor_factory.h"

#include <utility>

#include "components/sync/driver/generic_change_processor.h"
#include "components/sync/model/syncable_service.h"

namespace syncer {

GenericChangeProcessorFactory::GenericChangeProcessorFactory() {}

GenericChangeProcessorFactory::~GenericChangeProcessorFactory() {}

std::unique_ptr<GenericChangeProcessor>
GenericChangeProcessorFactory::CreateGenericChangeProcessor(
    ModelType type,
    UserShare* user_share,
    std::unique_ptr<DataTypeErrorHandler> error_handler,
    const base::WeakPtr<SyncableService>& local_service,
    const base::WeakPtr<SyncMergeResult>& merge_result,
    SyncClient* sync_client) {
  DCHECK(user_share);
  return std::make_unique<GenericChangeProcessor>(
      type, std::move(error_handler), local_service, merge_result, user_share,
      sync_client);
}

}  // namespace syncer
