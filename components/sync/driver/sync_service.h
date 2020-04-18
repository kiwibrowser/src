// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_SERVICE_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_SERVICE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/data_type_encryption_handler.h"
#include "components/sync/driver/sync_service_observer.h"

struct AccountInfo;
class GoogleServiceAuthError;
class GURL;

namespace sync_sessions {
class OpenTabsUIDelegate;
}  // namespace sync_sessions

namespace syncer {

class BaseTransaction;
class DataTypeController;
class JsController;
class LocalDeviceInfoProvider;
class GlobalIdMapper;
class ProtocolEventObserver;
class SyncClient;
class SyncCycleSnapshot;
struct SyncTokenStatus;
class TypeDebugInfoObserver;
struct SyncStatus;
struct UserShare;

// Events in ClearServerData flow to be recorded in histogram. Existing
// constants should not be deleted or reordered. New ones shold be added at the
// end, before CLEAR_SERVER_DATA_MAX.
enum ClearServerDataEvents {
  // ClearServerData started after user switched to custom passphrase.
  CLEAR_SERVER_DATA_STARTED,
  // DataTypeManager reported that catchup configuration failed.
  CLEAR_SERVER_DATA_CATCHUP_FAILED,
  // ClearServerData flow restarted after browser restart.
  CLEAR_SERVER_DATA_RETRIED,
  // Success.
  CLEAR_SERVER_DATA_SUCCEEDED,
  // Client received RECET_LOCAL_SYNC_DATA after custom passphrase was enabled
  // on different client.
  CLEAR_SERVER_DATA_RESET_LOCAL_DATA_RECEIVED,
  CLEAR_SERVER_DATA_MAX
};

// UIs that need to prevent Sync startup should hold an instance of this class
// until the user has finished modifying sync settings. This is not an inner
// class of SyncService to enable forward declarations.
class SyncSetupInProgressHandle {
 public:
  // UIs should not construct this directly, but instead call
  // SyncService::GetSetupInProgress().
  explicit SyncSetupInProgressHandle(base::Closure on_destroy);

  ~SyncSetupInProgressHandle();

 private:
  base::Closure on_destroy_;
};

class SyncService : public DataTypeEncryptionHandler, public KeyedService {
 public:
  // Used to specify the kind of passphrase with which sync data is encrypted.
  enum PassphraseType {
    IMPLICIT,  // The user did not provide a custom passphrase for encryption.
               // We implicitly use the GAIA password in such cases.
    EXPLICIT,  // The user selected the "use custom passphrase" radio button
               // during sync setup and provided a passphrase.
  };

  // Passed as an argument to RequestStop to control whether or not the sync
  // engine should clear its data directory when it shuts down. See
  // RequestStop for more information.
  enum SyncStopDataFate {
    KEEP_DATA,
    CLEAR_DATA,
  };

  ~SyncService() override {}

  // Whether sync is enabled by user or not. This does not necessarily mean
  // that sync is currently running (due to delayed startup, unrecoverable
  // errors, or shutdown). See IsSyncActive below for checking whether sync
  // is actually running.
  virtual bool IsFirstSetupComplete() const = 0;

  // Whether sync is allowed to start. Command line flags, platform-level
  // overrides, and account-level overrides are examples of reasons this
  // might be false.
  virtual bool IsSyncAllowed() const = 0;

  // Returns true if sync is fully initialized and active. This implies that
  // an initial configuration has successfully completed, although there may
  // be datatype specific, auth, or other transient errors. To see which
  // datatypes are actually syncing, see GetActiveDataTypes() below.
  virtual bool IsSyncActive() const = 0;

  // Returns true if the local sync backend server has been enabled through a
  // command line flag or policy. In this case sync is considered active but any
  // implied consent for further related services e.g. Suggestions, Web History
  // etc. is considered not granted.
  virtual bool IsLocalSyncEnabled() const = 0;

  // Triggers a GetUpdates call for the specified |types|, pulling any new data
  // from the sync server.
  virtual void TriggerRefresh(const ModelTypeSet& types) = 0;

  // Get the set of current active data types (those chosen or configured by
  // the user which have not also encountered a runtime error).
  // Note that if the Sync engine is in the middle of a configuration, this
  // will the the empty set. Once the configuration completes the set will
  // be updated.
  virtual ModelTypeSet GetActiveDataTypes() const = 0;

  // Returns the SyncClient instance associated with this service.
  virtual SyncClient* GetSyncClient() const = 0;

  // Adds/removes an observer. SyncService does not take ownership of the
  // observer.
  virtual void AddObserver(SyncServiceObserver* observer) = 0;
  virtual void RemoveObserver(SyncServiceObserver* observer) = 0;

  // Returns true if |observer| has already been added as an observer.
  virtual bool HasObserver(const SyncServiceObserver* observer) const = 0;

  // ---------------------------------------------------------------------------
  // TODO(sync): The methods below were pulled from ProfileSyncService, and
  // should be evaluated to see if they should stay.

  // Called when a datatype (SyncableService) has a need for sync to start
  // ASAP, presumably because a local change event has occurred but we're
  // still in deferred start mode, meaning the SyncableService hasn't been
  // told to MergeDataAndStartSyncing yet.
  virtual void OnDataTypeRequestsSyncStartup(ModelType type) = 0;

  // Returns true if sync is allowed, requested, and the user is logged in.
  // (being logged in does not mean that tokens are available - tokens may
  // be missing because they have not loaded yet, or because they were deleted
  // due to http://crbug.com/121755).
  virtual bool CanSyncStart() const = 0;

  // Stops sync at the user's request. |data_fate| controls whether the sync
  // engine should clear its data directory when it shuts down. Generally
  // KEEP_DATA is used when the user just stops sync, and CLEAR_DATA is used
  // when they sign out of the profile entirely.
  virtual void RequestStop(SyncStopDataFate data_fate) = 0;

  // The user requests that sync start. This only actually starts sync if
  // IsSyncAllowed is true and the user is signed in. Once sync starts,
  // other things such as IsFirstSetupComplete being false can still prevent
  // it from moving into the "active" state.
  virtual void RequestStart() = 0;

  // Returns the set of types which are preferred for enabling. This is a
  // superset of the active types (see GetActiveDataTypes()).
  virtual ModelTypeSet GetPreferredDataTypes() const = 0;

  // Called when a user chooses which data types to sync. |sync_everything|
  // represents whether they chose the "keep everything synced" option; if
  // true, |chosen_types| will be ignored and all data types will be synced.
  // |sync_everything| means "sync all current and future data types."
  // |chosen_types| must be a subset of UserSelectableTypes().
  virtual void OnUserChoseDatatypes(bool sync_everything,
                                    ModelTypeSet chosen_types) = 0;

  // Called whe Sync has been setup by the user and can be started.
  virtual void SetFirstSetupComplete() = 0;

  // Returns true if initial sync setup is in progress (does not return true
  // if the user is customizing sync after already completing setup once).
  // SyncService uses this to determine if it's OK to start syncing, or if the
  // user is still setting up the initial sync configuration.
  virtual bool IsFirstSetupInProgress() const = 0;

  // Called by the UI to notify the SyncService that UI is visible so it will
  // not start syncing. This tells sync whether it's safe to start downloading
  // data types yet (we don't start syncing until after sync setup is complete).
  // The UI calls this and holds onto the instance for as long as any part of
  // the signin wizard is displayed (even just the login UI).
  // When the last outstanding handle is deleted, this kicks off the sync engine
  // to ensure that data download starts. In this case,
  // |ReconfigureDatatypeManager| will get triggered.
  virtual std::unique_ptr<SyncSetupInProgressHandle>
  GetSetupInProgressHandle() = 0;

  // Used by tests.
  virtual bool IsSetupInProgress() const = 0;

  // Whether the data types active for the current mode have finished
  // configuration.
  virtual bool ConfigurationDone() const = 0;

  virtual const GoogleServiceAuthError& GetAuthError() const = 0;
  virtual bool HasUnrecoverableError() const = 0;

  // Returns true if the SyncEngine has told us it's ready to accept changes.
  virtual bool IsEngineInitialized() const = 0;

  // Return the active OpenTabsUIDelegate. If open/proxy tabs is not enabled or
  // not currently syncing, returns nullptr.
  virtual sync_sessions::OpenTabsUIDelegate* GetOpenTabsUIDelegate() = 0;

  // Returns true if OnPassphraseRequired has been called for decryption and
  // we have an encrypted data type enabled.
  virtual bool IsPassphraseRequiredForDecryption() const = 0;

  // Returns the time the current explicit passphrase (if any), was set.
  // If no secondary passphrase is in use, or no time is available, returns an
  // unset base::Time.
  virtual base::Time GetExplicitPassphraseTime() const = 0;

  // Returns true if a secondary (explicit) passphrase is being used. It is not
  // legal to call this method before the engine is initialized.
  virtual bool IsUsingSecondaryPassphrase() const = 0;

  // Turns on encryption for all data. Callers must call OnUserChoseDatatypes()
  // after calling this to force the encryption to occur.
  virtual void EnableEncryptEverything() = 0;

  // Returns true if we are currently set to encrypt all the sync data.
  virtual bool IsEncryptEverythingEnabled() const = 0;

  // Asynchronously sets the passphrase to |passphrase| for encryption. |type|
  // specifies whether the passphrase is a custom passphrase or the GAIA
  // password being reused as a passphrase.
  // TODO(atwilson): Change this so external callers can only set an EXPLICIT
  // passphrase with this API.
  virtual void SetEncryptionPassphrase(const std::string& passphrase,
                                       PassphraseType type) = 0;

  // Asynchronously decrypts pending keys using |passphrase|. Returns false
  // immediately if the passphrase could not be used to decrypt a locally cached
  // copy of encrypted keys; returns true otherwise.
  virtual bool SetDecryptionPassphrase(const std::string& passphrase)
      WARN_UNUSED_RESULT = 0;

  // Checks whether the Cryptographer is ready to encrypt and decrypt updates
  // for sensitive data types. Caller must be holding a
  // syncapi::BaseTransaction to ensure thread safety.
  virtual bool IsCryptographerReady(const BaseTransaction* trans) const = 0;

  // TODO(akalin): This is called mostly by ModelAssociators and
  // tests.  Figure out how to pass the handle to the ModelAssociators
  // directly, figure out how to expose this to tests, and remove this
  // function.
  virtual UserShare* GetUserShare() const = 0;

  // Returns DeviceInfo provider for the local device.
  virtual LocalDeviceInfoProvider* GetLocalDeviceInfoProvider() const = 0;

  // Registers a data type controller with the sync service.  This
  // makes the data type controller available for use, it does not
  // enable or activate the synchronization of the data type (see
  // ActivateDataType).  Takes ownership of the pointer.
  virtual void RegisterDataTypeController(
      std::unique_ptr<DataTypeController> data_type_controller) = 0;

  // Called to re-enable a type disabled by DisableDatatype(..). Note, this does
  // not change the preferred state of a datatype, and is not persisted across
  // restarts.
  virtual void ReenableDatatype(ModelType type) = 0;

  // Return sync token status.
  virtual SyncTokenStatus GetSyncTokenStatus() const = 0;

  // Get a description of the sync status for displaying in the user interface.
  virtual std::string QuerySyncStatusSummaryString() = 0;

  // Initializes a struct of status indicators with data from the engine.
  // Returns false if the engine was not available for querying; in that case
  // the struct will be filled with default data.
  virtual bool QueryDetailedSyncStatus(SyncStatus* result) = 0;

  // Returns the last synced time.
  virtual base::Time GetLastSyncedTime() const = 0;

  // Returns a human readable string describing engine initialization state.
  virtual std::string GetEngineInitializationStateString() const = 0;

  virtual SyncCycleSnapshot GetLastCycleSnapshot() const = 0;

  // Returns a ListValue indicating the status of all registered types.
  //
  // The format is:
  // [ {"name": <name>, "value": <value>, "status": <status> }, ... ]
  // where <name> is a type's name, <value> is a string providing details for
  // the type's status, and <status> is one of "error", "warning" or "ok"
  // depending on the type's current status.
  //
  // This function is used by about_sync_util.cc to help populate the about:sync
  // page.  It returns a ListValue rather than a DictionaryValue in part to make
  // it easier to iterate over its elements when constructing that page.
  virtual std::unique_ptr<base::Value> GetTypeStatusMap() = 0;

  virtual const GURL& sync_service_url() const = 0;

  virtual std::string unrecoverable_error_message() const = 0;
  virtual base::Location unrecoverable_error_location() const = 0;

  virtual void AddProtocolEventObserver(ProtocolEventObserver* observer) = 0;
  virtual void RemoveProtocolEventObserver(ProtocolEventObserver* observer) = 0;

  virtual void AddTypeDebugInfoObserver(TypeDebugInfoObserver* observer) = 0;
  virtual void RemoveTypeDebugInfoObserver(TypeDebugInfoObserver* observer) = 0;

  // Returns a weak pointer to the service's JsController.
  virtual base::WeakPtr<JsController> GetJsController() = 0;

  // Asynchronously fetches base::Value representations of all sync nodes and
  // returns them to the specified callback on this thread.
  //
  // These requests can live a long time and return when you least expect it.
  // For safety, the callback should be bound to some sort of WeakPtr<> or
  // scoped_refptr<>.
  virtual void GetAllNodes(
      const base::Callback<void(std::unique_ptr<base::ListValue>)>&
          callback) = 0;

  // Information about the currently signed in user.
  virtual AccountInfo GetAuthenticatedAccountInfo() const = 0;

  virtual GlobalIdMapper* GetGlobalIdMapper() const = 0;

 protected:
  SyncService() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncService);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_SERVICE_H_
