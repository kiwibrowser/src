// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SHARED_CHANGE_PROCESSOR_REF_H_
#define COMPONENTS_SYNC_DRIVER_SHARED_CHANGE_PROCESSOR_REF_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "components/sync/driver/shared_change_processor.h"
#include "components/sync/model/sync_change_processor.h"
#include "components/sync/model/sync_error_factory.h"

namespace syncer {

// A SyncChangeProcessor stub for interacting with a refcounted
// SharedChangeProcessor.
class SharedChangeProcessorRef : public SyncChangeProcessor,
                                 public SyncErrorFactory {
 public:
  SharedChangeProcessorRef(
      const scoped_refptr<SharedChangeProcessor>& change_processor);
  ~SharedChangeProcessorRef() override;

  // SyncChangeProcessor implementation.
  SyncError ProcessSyncChanges(const base::Location& from_here,
                               const SyncChangeList& change_list) override;
  SyncDataList GetAllSyncData(ModelType type) const override;
  SyncError UpdateDataTypeContext(
      ModelType type,
      SyncChangeProcessor::ContextRefreshStatus refresh_status,
      const std::string& context) override;
  void AddLocalChangeObserver(LocalChangeObserver* observer) override;
  void RemoveLocalChangeObserver(LocalChangeObserver* observer) override;

  // SyncErrorFactory implementation.
  SyncError CreateAndUploadError(const base::Location& from_here,
                                 const std::string& message) override;

  // Default copy and assign welcome (and safe due to refcounted-ness).

 private:
  scoped_refptr<SharedChangeProcessor> change_processor_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SHARED_CHANGE_PROCESSOR_REF_H_
