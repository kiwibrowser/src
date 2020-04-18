// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_SYNC_PROFILE_SYNC_COMPONENTS_FACTORY_IMPL_H__
#define COMPONENTS_BROWSER_SYNC_PROFILE_SYNC_COMPONENTS_FACTORY_IMPL_H__

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/version_info/version_info.h"

namespace autofill {
class AutofillWebDataService;
}

namespace password_manager {
class PasswordStore;
}

namespace browser_sync {

class ProfileSyncComponentsFactoryImpl
    : public syncer::SyncApiComponentFactory {
 public:
  ProfileSyncComponentsFactoryImpl(
      syncer::SyncClient* sync_client,
      version_info::Channel channel,
      const std::string& version,
      bool is_tablet,
      const base::CommandLine& command_line,
      const char* history_disabled_pref,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_thread,
      const scoped_refptr<base::SingleThreadTaskRunner>& db_thread,
      const scoped_refptr<autofill::AutofillWebDataService>& web_data_service,
      const scoped_refptr<password_manager::PasswordStore>& password_store);
  ~ProfileSyncComponentsFactoryImpl() override;

  // SyncApiComponentFactory implementation:
  void RegisterDataTypes(
      syncer::SyncService* sync_service,
      const RegisterDataTypesMethod& register_platform_types_method) override;
  std::unique_ptr<syncer::DataTypeManager> CreateDataTypeManager(
      syncer::ModelTypeSet initial_types,
      const syncer::WeakHandle<syncer::DataTypeDebugInfoListener>&
          debug_info_listener,
      const syncer::DataTypeController::TypeMap* controllers,
      const syncer::DataTypeEncryptionHandler* encryption_handler,
      syncer::ModelTypeConfigurer* configurer,
      syncer::DataTypeManagerObserver* observer) override;
  std::unique_ptr<syncer::SyncEngine> CreateSyncEngine(
      const std::string& name,
      invalidation::InvalidationService* invalidator,
      const base::WeakPtr<syncer::SyncPrefs>& sync_prefs,
      const base::FilePath& sync_data_folder) override;
  std::unique_ptr<syncer::LocalDeviceInfoProvider>
  CreateLocalDeviceInfoProvider() override;
  syncer::SyncApiComponentFactory::SyncComponents CreateBookmarkSyncComponents(
      syncer::SyncService* sync_service,
      std::unique_ptr<syncer::DataTypeErrorHandler> error_handler) override;

  // Sets a bit that determines whether PREFERENCES should be registered with a
  // ModelTypeController for testing purposes.
  static void OverridePrefsForUssTest(bool use_uss);

 private:
  // Register data types which are enabled on both desktop and mobile.
  // |disabled_types| corresponds only to those types being explicitly disabled
  // by the command line.
  void RegisterCommonDataTypes(syncer::SyncService* sync_service,
                               syncer::ModelTypeSet disabled_types);

  // Client/platform specific members.
  syncer::SyncClient* const sync_client_;
  const version_info::Channel channel_;
  const std::string version_;
  const bool is_tablet_;
  const base::CommandLine command_line_;
  const char* history_disabled_pref_;
  const scoped_refptr<base::SingleThreadTaskRunner> ui_thread_;
  const scoped_refptr<base::SingleThreadTaskRunner> db_thread_;
  const scoped_refptr<autofill::AutofillWebDataService> web_data_service_;
  const scoped_refptr<password_manager::PasswordStore> password_store_;

  // Whether to override PREFERENCES to use USS.
  static bool override_prefs_controller_to_uss_for_test_;

  DISALLOW_COPY_AND_ASSIGN(ProfileSyncComponentsFactoryImpl);
};

}  // namespace browser_sync

#endif  // COMPONENTS_BROWSER_SYNC_PROFILE_SYNC_COMPONENTS_FACTORY_IMPL_H__
