// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_GENERIC_CHANGE_PROCESSOR_FACTORY_H_
#define COMPONENTS_SYNC_DRIVER_GENERIC_CHANGE_PROCESSOR_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/sync/base/model_type.h"
#include "components/sync/model/data_type_error_handler.h"

namespace syncer {

class GenericChangeProcessor;
class SyncClient;
class SyncMergeResult;
class SyncableService;
struct UserShare;

// Because GenericChangeProcessors are created and used only from the model
// thread, their lifetime is strictly shorter than other components like
// DataTypeController, which live before / after communication with model
// threads begins and ends.
// The GCP is created "on the fly" at just the right time, on just the right
// thread. Given that, we use a factory to instantiate GenericChangeProcessors
// so that tests can choose to use a fake processor (i.e instead of injection).
class GenericChangeProcessorFactory {
 public:
  GenericChangeProcessorFactory();
  virtual ~GenericChangeProcessorFactory();
  virtual std::unique_ptr<GenericChangeProcessor> CreateGenericChangeProcessor(
      ModelType type,
      UserShare* user_share,
      std::unique_ptr<DataTypeErrorHandler> error_handler,
      const base::WeakPtr<SyncableService>& local_service,
      const base::WeakPtr<SyncMergeResult>& merge_result,
      SyncClient* sync_client);

 private:
  DISALLOW_COPY_AND_ASSIGN(GenericChangeProcessorFactory);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_GENERIC_CHANGE_PROCESSOR_FACTORY_H_
