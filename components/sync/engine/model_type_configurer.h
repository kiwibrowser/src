// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_MODEL_TYPE_CONFIGURER_H_
#define COMPONENTS_SYNC_ENGINE_MODEL_TYPE_CONFIGURER_H_

#include <map>
#include <memory>

#include "base/callback.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine/configure_reason.h"
#include "components/sync/engine/model_safe_worker.h"

namespace syncer {

class ChangeProcessor;
struct ActivationContext;

// The DataTypeConfigurer interface abstracts out the action of
// configuring a set of new data types and cleaning up after a set of
// removed data types.
class ModelTypeConfigurer {
 public:
  // Utility struct for holding ConfigureDataTypes options.
  struct ConfigureParams {
    ConfigureParams();
    ConfigureParams(ConfigureParams&& other);
    ~ConfigureParams();
    ConfigureParams& operator=(ConfigureParams&& other);

    ConfigureReason reason;
    ModelTypeSet enabled_types;
    ModelTypeSet disabled_types;
    ModelTypeSet to_download;
    ModelTypeSet to_purge;
    ModelTypeSet to_journal;
    ModelTypeSet to_unapply;
    // Run when configuration is done with the set of all types that failed
    // configuration (if its argument isn't empty, an error was encountered).
    // TODO(akalin): Use a Delegate class with OnConfigureSuccess,
    // OnConfigureFailure, and OnConfigureRetry instead of a pair of callbacks.
    // The awkward part is handling when SyncEngine calls ConfigureDataTypes on
    // itself to configure Nigori.
    base::Callback<void(ModelTypeSet, ModelTypeSet)> ready_task;
    base::Closure retry_callback;

   private:
    DISALLOW_COPY_AND_ASSIGN(ConfigureParams);
  };

  ModelTypeConfigurer();
  virtual ~ModelTypeConfigurer();

  // Changes the set of data types that are currently being synced.
  virtual void ConfigureDataTypes(ConfigureParams params) = 0;

  // Registers directory type with sync engine. This function creates update
  // handler for the type and thus needs to be called before ConfigureDataType
  // that includes the type in |to_download| type set.
  virtual void RegisterDirectoryDataType(ModelType type,
                                         ModelSafeGroup group) = 0;

  // Unregisters directory type from sync engine. After this call updates and
  // local change will not be synced with server.
  virtual void UnregisterDirectoryDataType(ModelType type) = 0;

  // Activates change processing for the given directory data type.  This must
  // be called synchronously with the data type's model association so
  // no changes are dropped between model association and change
  // processor activation.
  virtual void ActivateDirectoryDataType(ModelType type,
                                         ModelSafeGroup group,
                                         ChangeProcessor* change_processor) = 0;

  // Deactivates change processing for the given data type.
  virtual void DeactivateDirectoryDataType(ModelType type) = 0;

  // Activates change processing for the given non-blocking data type.
  // This must be called before initial sync for data type.
  virtual void ActivateNonBlockingDataType(
      ModelType type,
      std::unique_ptr<ActivationContext> activation_context) = 0;

  // Deactivates change processing for the given non-blocking data type.
  virtual void DeactivateNonBlockingDataType(ModelType type) = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_MODEL_TYPE_CONFIGURER_H_
