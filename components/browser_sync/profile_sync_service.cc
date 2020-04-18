// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_sync/profile_sync_service.h"

#include <cstddef>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/browser_sync/sync_auth_manager.h"
#include "components/invalidation/impl/invalidation_prefs.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/reading_list/features/reading_list_buildflags.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/sync/base/bind_to_task_runner.h"
#include "components/sync/base/cryptographer.h"
#include "components/sync/base/passphrase_type.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/base/stop_source.h"
#include "components/sync/base/system_encryptor.h"
#include "components/sync/device_info/device_info.h"
#include "components/sync/device_info/device_info_sync_bridge.h"
#include "components/sync/device_info/device_info_tracker.h"
#include "components/sync/driver/backend_migrator.h"
#include "components/sync/driver/directory_data_type_controller.h"
#include "components/sync/driver/signin_manager_wrapper.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/driver/sync_error_controller.h"
#include "components/sync/driver/sync_type_preference_provider.h"
#include "components/sync/driver/sync_util.h"
#include "components/sync/driver/user_selectable_sync_type.h"
#include "components/sync/engine/configure_reason.h"
#include "components/sync/engine/cycle/type_debug_info_observer.h"
#include "components/sync/engine/engine_components_factory_impl.h"
#include "components/sync/engine/net/http_bridge_network_resources.h"
#include "components/sync/engine/net/network_resources.h"
#include "components/sync/engine/polling_constants.h"
#include "components/sync/engine/sync_encryption_handler.h"
#include "components/sync/engine/sync_string_conversions.h"
#include "components/sync/js/js_event_details.h"
#include "components/sync/model/change_processor.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/model_impl/client_tag_based_model_type_processor.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/syncable_read_transaction.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_sessions/favicon_cache.h"
#include "components/sync_sessions/session_data_type_controller.h"
#include "components/sync_sessions/session_sync_bridge.h"
#include "components/sync_sessions/sessions_sync_manager.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/version_info/version_info_values.h"
#include "net/url_request/url_request_context_getter.h"

using syncer::BackendMigrator;
using syncer::ClientTagBasedModelTypeProcessor;
using syncer::DataTypeController;
using syncer::DataTypeManager;
using syncer::DataTypeStatusTable;
using syncer::DeviceInfoSyncBridge;
using syncer::EngineComponentsFactory;
using syncer::EngineComponentsFactoryImpl;
using syncer::JsBackend;
using syncer::JsController;
using syncer::JsEventHandler;
using syncer::ModelSafeRoutingInfo;
using syncer::ModelType;
using syncer::ModelTypeSet;
using syncer::ModelTypeStore;
using syncer::ProtocolEventObserver;
using syncer::SyncBackendRegistrar;
using syncer::SyncClient;
using syncer::SyncCredentials;
using syncer::SyncEngine;
using syncer::SyncManagerFactory;
using syncer::SyncProtocolError;
using syncer::WeakHandle;

namespace browser_sync {

namespace {

constexpr char kSyncUnrecoverableErrorHistogram[] = "Sync.UnrecoverableErrors";

constexpr base::FilePath::CharType kSyncDataFolderName[] =
    FILE_PATH_LITERAL("Sync Data");

constexpr base::FilePath::CharType kLevelDBFolderName[] =
    FILE_PATH_LITERAL("LevelDB");

base::FilePath FormatSyncDataPath(const base::FilePath& base_directory) {
  return base_directory.Append(base::FilePath(kSyncDataFolderName));
}

base::FilePath FormatSharedModelTypeStorePath(
    const base::FilePath& base_directory) {
  return FormatSyncDataPath(base_directory)
      .Append(base::FilePath(kLevelDBFolderName));
}

EngineComponentsFactory::Switches EngineSwitchesFromCommandLine() {
  EngineComponentsFactory::Switches factory_switches = {
      EngineComponentsFactory::ENCRYPTION_KEYSTORE,
      EngineComponentsFactory::BACKOFF_NORMAL};

  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  if (cl->HasSwitch(switches::kSyncShortInitialRetryOverride)) {
    factory_switches.backoff_override =
        EngineComponentsFactory::BACKOFF_SHORT_INITIAL_RETRY_OVERRIDE;
  }
  if (cl->HasSwitch(switches::kSyncEnableGetUpdateAvoidance)) {
    factory_switches.pre_commit_updates_policy =
        EngineComponentsFactory::FORCE_ENABLE_PRE_COMMIT_UPDATE_AVOIDANCE;
  }
  if (cl->HasSwitch(switches::kSyncShortNudgeDelayForTest)) {
    factory_switches.nudge_delay =
        EngineComponentsFactory::NudgeDelay::SHORT_NUDGE_DELAY;
  }
  return factory_switches;
}

}  // namespace

ProfileSyncService::InitParams::InitParams() = default;
ProfileSyncService::InitParams::InitParams(InitParams&& other) = default;
ProfileSyncService::InitParams::~InitParams() = default;

ProfileSyncService::ProfileSyncService(InitParams init_params)
    : sync_client_(std::move(init_params.sync_client)),
      sync_prefs_(sync_client_->GetPrefService()),
      signin_(std::move(init_params.signin_wrapper)),
      auth_manager_(std::make_unique<SyncAuthManager>(
          this,
          &sync_prefs_,
          signin_ ? signin_->GetIdentityManager() : nullptr,
          init_params.oauth2_token_service)),
      channel_(init_params.channel),
      base_directory_(init_params.base_directory),
      debug_identifier_(init_params.debug_identifier),
      sync_service_url_(
          syncer::GetSyncServiceURL(*base::CommandLine::ForCurrentProcess(),
                                    init_params.channel)),
      signin_scoped_device_id_callback_(
          init_params.signin_scoped_device_id_callback),
      network_time_update_callback_(
          std::move(init_params.network_time_update_callback)),
      url_request_context_(init_params.url_request_context),
      is_first_time_sync_configure_(false),
      engine_initialized_(false),
      sync_disabled_by_admin_(false),
      unrecoverable_error_reason_(ERROR_REASON_UNSET),
      expect_sync_configuration_aborted_(false),
      gaia_cookie_manager_service_(init_params.gaia_cookie_manager_service),
      network_resources_(
          std::make_unique<syncer::HttpBridgeNetworkResources>()),
      start_behavior_(init_params.start_behavior),
      passphrase_prompt_triggered_by_version_(false),
      sync_enabled_weak_factory_(this),
      weak_factory_(this) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(signin_scoped_device_id_callback_);
  DCHECK(sync_client_);

  ResetCryptoState();

  std::string last_version = sync_prefs_.GetLastRunVersion();
  std::string current_version = PRODUCT_VERSION;
  sync_prefs_.SetLastRunVersion(current_version);

  // Check for a major version change. Note that the versions have format
  // MAJOR.MINOR.BUILD.PATCH.
  if (last_version.substr(0, last_version.find('.')) !=
      current_version.substr(0, current_version.find('.'))) {
    passphrase_prompt_triggered_by_version_ = true;
  }

  if (init_params.model_type_store_factory.is_null()) {
    model_type_store_factory_ = GetModelTypeStoreFactory(base_directory_);
  } else {
    model_type_store_factory_ = init_params.model_type_store_factory;
  }
}

ProfileSyncService::~ProfileSyncService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (gaia_cookie_manager_service_)
    gaia_cookie_manager_service_->RemoveObserver(this);
  sync_prefs_.RemoveSyncPrefObserver(this);
  // Shutdown() should have been called before destruction.
  DCHECK(!engine_initialized_);
}

bool ProfileSyncService::CanSyncStart() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return (IsSyncAllowed() && IsSyncRequested() &&
          (IsLocalSyncEnabled() || IsSignedIn()));
}

void ProfileSyncService::Initialize() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sync_client_->Initialize();

  // We don't pass StartupController an Unretained reference to future-proof
  // against the controller impl changing to post tasks.
  startup_controller_ = std::make_unique<syncer::StartupController>(
      &sync_prefs_,
      base::BindRepeating(&ProfileSyncService::CanEngineStart,
                          base::Unretained(this)),
      base::BindRepeating(&ProfileSyncService::StartUpSlowEngineComponents,
                          weak_factory_.GetWeakPtr()));
  local_device_ = sync_client_->GetSyncApiComponentFactory()
                      ->CreateLocalDeviceInfoProvider();
  sync_stopped_reporter_ = std::make_unique<syncer::SyncStoppedReporter>(
      sync_service_url_, local_device_->GetSyncUserAgent(),
      url_request_context_, syncer::SyncStoppedReporter::ResultCallback());

  if (base::FeatureList::IsEnabled(switches::kSyncUSSSessions)) {
    sessions_sync_manager_ = std::make_unique<sync_sessions::SessionSyncBridge>(
        sync_client_->GetSyncSessionsClient(), &sync_prefs_,
        local_device_.get(), model_type_store_factory_,
        base::BindRepeating(&ProfileSyncService::NotifyForeignSessionUpdated,
                            sync_enabled_weak_factory_.GetWeakPtr()),
        std::make_unique<ClientTagBasedModelTypeProcessor>(
            syncer::SESSIONS,
            base::BindRepeating(&syncer::ReportUnrecoverableError, channel_)));
  } else {
    sessions_sync_manager_ =
        std::make_unique<sync_sessions::SessionsSyncManager>(
            sync_client_->GetSyncSessionsClient(), &sync_prefs_,
            local_device_.get(),
            base::BindRepeating(
                &ProfileSyncService::NotifyForeignSessionUpdated,
                sync_enabled_weak_factory_.GetWeakPtr()));
  }

  device_info_sync_bridge_ = std::make_unique<DeviceInfoSyncBridge>(
      local_device_.get(), model_type_store_factory_,
      std::make_unique<ClientTagBasedModelTypeProcessor>(
          syncer::DEVICE_INFO,
          /*dump_stack=*/base::BindRepeating(&syncer::ReportUnrecoverableError,
                                             channel_)));

  syncer::SyncApiComponentFactory::RegisterDataTypesMethod
      register_platform_types_callback =
          sync_client_->GetRegisterPlatformTypesCallback();
  sync_client_->GetSyncApiComponentFactory()->RegisterDataTypes(
      this, register_platform_types_callback);

  if (gaia_cookie_manager_service_)
    gaia_cookie_manager_service_->AddObserver(this);

  // We clear this here (vs Shutdown) because we want to remember that an error
  // happened on shutdown so we can display details (message, location) about it
  // in about:sync.
  ClearStaleErrors();

  sync_prefs_.AddSyncPrefObserver(this);

  SyncInitialState sync_state = CAN_START;
  if (!IsLocalSyncEnabled() && !IsSignedIn()) {
    sync_state = NOT_SIGNED_IN;
  } else if (IsManaged()) {
    sync_state = IS_MANAGED;
  } else if (!IsSyncAllowedByPlatform()) {
    // This case should currently never be hit, as Android's master sync isn't
    // plumbed into PSS until after this function. See http://crbug.com/568771.
    sync_state = NOT_ALLOWED_BY_PLATFORM;
  } else if (!IsSyncRequested()) {
    if (IsFirstSetupComplete()) {
      sync_state = NOT_REQUESTED;
    } else {
      sync_state = NOT_REQUESTED_NOT_SETUP;
    }
  } else if (!IsFirstSetupComplete()) {
    sync_state = NEEDS_CONFIRMATION;
  }
  UMA_HISTOGRAM_ENUMERATION("Sync.InitialState", sync_state,
                            SYNC_INITIAL_STATE_LIMIT);

  // If sync isn't allowed, the only thing to do is to turn it off.
  if (!IsSyncAllowed()) {
    // Only clear data if disallowed by policy.
    RequestStop(IsManaged() ? CLEAR_DATA : KEEP_DATA);
    return;
  }

  if (!IsLocalSyncEnabled()) {
    auth_manager_->RegisterForAuthNotifications();

    if (!IsSignedIn()) {
      // Clean up in case of previous crash during signout.
      StopImpl(CLEAR_DATA);
    }
  }

#if defined(OS_CHROMEOS)
  std::string bootstrap_token = sync_prefs_.GetEncryptionBootstrapToken();
  if (bootstrap_token.empty()) {
    sync_prefs_.SetEncryptionBootstrapToken(
        sync_prefs_.GetSpareBootstrapToken());
  }
#endif

#if !defined(OS_ANDROID)
  DCHECK(sync_error_controller_ == nullptr)
      << "Initialize() called more than once.";
  sync_error_controller_ = std::make_unique<syncer::SyncErrorController>(this);
  AddObserver(sync_error_controller_.get());
#endif

  memory_pressure_listener_ = std::make_unique<base::MemoryPressureListener>(
      base::BindRepeating(&ProfileSyncService::OnMemoryPressure,
                          sync_enabled_weak_factory_.GetWeakPtr()));
  startup_controller_->Reset(GetRegisteredDataTypes());

  // Auto-start means the first time the profile starts up, sync should start up
  // immediately.
  if (start_behavior_ == AUTO_START && IsSyncRequested() &&
      !IsFirstSetupComplete()) {
    startup_controller_->TryStartImmediately();
  } else {
    startup_controller_->TryStart();
  }
}

void ProfileSyncService::StartSyncingWithServer() {
  if (base::FeatureList::IsEnabled(
          switches::kSyncClearDataOnPassphraseEncryption) &&
      sync_prefs_.GetPassphraseEncryptionTransitionInProgress()) {
    // We are restarting catchup configuration after browser restart.
    UMA_HISTOGRAM_ENUMERATION("Sync.ClearServerDataEvents",
                              syncer::CLEAR_SERVER_DATA_RETRIED,
                              syncer::CLEAR_SERVER_DATA_MAX);

    crypto_->BeginConfigureCatchUpBeforeClear();
    return;
  }

  if (engine_)
    engine_->StartSyncingWithServer();
}

void ProfileSyncService::RegisterDataTypeController(
    std::unique_ptr<DataTypeController> data_type_controller) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(data_type_controllers_.count(data_type_controller->type()), 0U);
  data_type_controllers_[data_type_controller->type()] =
      std::move(data_type_controller);
}

bool ProfileSyncService::IsDataTypeControllerRunning(
    syncer::ModelType type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DataTypeController::TypeMap::const_iterator iter =
      data_type_controllers_.find(type);
  if (iter == data_type_controllers_.end()) {
    return false;
  }
  return iter->second->state() == DataTypeController::RUNNING;
}

sync_sessions::OpenTabsUIDelegate* ProfileSyncService::GetOpenTabsUIDelegate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Although the backing data actually is of type |SESSIONS|, the desire to use
  // open tabs functionality is tracked by the state of the |PROXY_TABS| type.
  if (!IsDataTypeControllerRunning(syncer::PROXY_TABS)) {
    return nullptr;
  }

  DCHECK(sessions_sync_manager_);
  return sessions_sync_manager_->GetOpenTabsUIDelegate();
}

sync_sessions::FaviconCache* ProfileSyncService::GetFaviconCache() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sessions_sync_manager_->GetFaviconCache();
}

syncer::DeviceInfoTracker* ProfileSyncService::GetDeviceInfoTracker() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return device_info_sync_bridge_.get();
}

syncer::LocalDeviceInfoProvider*
ProfileSyncService::GetLocalDeviceInfoProvider() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return local_device_.get();
}

void ProfileSyncService::OnSessionRestoreComplete() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DataTypeController::TypeMap::const_iterator iter =
      data_type_controllers_.find(syncer::SESSIONS);
  if (iter == data_type_controllers_.end()) {
    return;
  }
  DCHECK(iter->second);

  if (base::FeatureList::IsEnabled(switches::kSyncUSSSessions)) {
    sessions_sync_manager_->OnSessionRestoreComplete();
  } else {
    static_cast<sync_sessions::SessionDataTypeController*>(iter->second.get())
        ->OnSessionRestoreComplete();
  }
}

syncer::WeakHandle<syncer::JsEventHandler>
ProfileSyncService::GetJsEventHandler() {
  return syncer::MakeWeakHandle(sync_js_controller_.AsWeakPtr());
}

syncer::SyncEngine::HttpPostProviderFactoryGetter
ProfileSyncService::MakeHttpPostProviderFactoryGetter() {
  return base::BindRepeating(
      &syncer::NetworkResources::GetHttpPostProviderFactory,
      base::Unretained(network_resources_.get()), url_request_context_,
      network_time_update_callback_);
}

syncer::WeakHandle<syncer::UnrecoverableErrorHandler>
ProfileSyncService::GetUnrecoverableErrorHandler() {
  return syncer::MakeWeakHandle(sync_enabled_weak_factory_.GetWeakPtr());
}

void ProfileSyncService::ResetCryptoState() {
  crypto_ = std::make_unique<syncer::SyncServiceCrypto>(
      base::BindRepeating(&ProfileSyncService::NotifyObservers,
                          base::Unretained(this)),
      base::BindRepeating(&ProfileSyncService::GetPreferredDataTypes,
                          base::Unretained(this)),
      &sync_prefs_);
}

bool ProfileSyncService::IsEncryptedDatatypeEnabled() const {
  if (encryption_pending())
    return true;
  const syncer::ModelTypeSet preferred_types = GetPreferredDataTypes();
  const syncer::ModelTypeSet encrypted_types = GetEncryptedDataTypes();
  DCHECK(encrypted_types.Has(syncer::PASSWORDS));
  return !Intersection(preferred_types, encrypted_types).Empty();
}

void ProfileSyncService::OnProtocolEvent(const syncer::ProtocolEvent& event) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : protocol_event_observers_)
    observer.OnProtocolEvent(event);
}

void ProfileSyncService::OnDirectoryTypeCommitCounterUpdated(
    syncer::ModelType type,
    const syncer::CommitCounters& counters) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : type_debug_info_observers_)
    observer.OnCommitCountersUpdated(type, counters);
}

void ProfileSyncService::OnDirectoryTypeUpdateCounterUpdated(
    syncer::ModelType type,
    const syncer::UpdateCounters& counters) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : type_debug_info_observers_)
    observer.OnUpdateCountersUpdated(type, counters);
}

void ProfileSyncService::OnDatatypeStatusCounterUpdated(
    syncer::ModelType type,
    const syncer::StatusCounters& counters) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : type_debug_info_observers_)
    observer.OnStatusCountersUpdated(type, counters);
}

void ProfileSyncService::OnDataTypeRequestsSyncStartup(syncer::ModelType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(syncer::UserTypes().Has(type));

  if (!GetPreferredDataTypes().Has(type)) {
    // We can get here as datatype SyncableServices are typically wired up
    // to the native datatype even if sync isn't enabled.
    DVLOG(1) << "Dropping sync startup request because type "
             << syncer::ModelTypeToString(type) << "not enabled.";
    return;
  }

  // If this is a data type change after a major version update, reset the
  // passphrase prompted state and notify observers.
  if (IsPassphraseRequired() && passphrase_prompt_triggered_by_version_) {
    // The major version has changed and a local syncable change was made.
    // Reset the passphrase prompt state.
    passphrase_prompt_triggered_by_version_ = false;
    sync_prefs_.SetPassphrasePrompted(false);
    NotifyObservers();
  }

  if (engine_) {
    DVLOG(1) << "A data type requested sync startup, but it looks like "
                "something else beat it to the punch.";
    return;
  }

  startup_controller_->OnDataTypeRequestsSyncStartup(type);
}

void ProfileSyncService::StartUpSlowEngineComponents() {
  invalidation::InvalidationService* invalidator =
      sync_client_->GetInvalidationService();

  engine_ = sync_client_->GetSyncApiComponentFactory()->CreateSyncEngine(
      debug_identifier_, invalidator, sync_prefs_.AsWeakPtr(),
      FormatSyncDataPath(base_directory_));

  // Clear any old errors the first time sync starts.
  if (!IsFirstSetupComplete())
    ClearStaleErrors();

  InitializeEngine();

  UpdateFirstSyncTimePref();

  ReportPreviousSessionMemoryWarningCount();
}

void ProfileSyncService::InitializeEngine() {
  DCHECK(engine_);

  if (!sync_thread_) {
    sync_thread_ = std::make_unique<base::Thread>("Chrome_SyncThread");
    base::Thread::Options options;
    options.timer_slack = base::TIMER_SLACK_MAXIMUM;
    bool success = sync_thread_->StartWithOptions(options);
    DCHECK(success);
  }

  SyncEngine::InitParams params;
  params.sync_task_runner = sync_thread_->task_runner();
  params.host = this;
  params.registrar = std::make_unique<SyncBackendRegistrar>(
      debug_identifier_,
      base::BindRepeating(&SyncClient::CreateModelWorkerForGroup,
                          base::Unretained(sync_client_.get())));
  params.encryption_observer_proxy = crypto_->GetEncryptionObserverProxy();
  params.extensions_activity = sync_client_->GetExtensionsActivity();
  params.event_handler = GetJsEventHandler();
  params.service_url = sync_service_url();
  params.sync_user_agent = GetLocalDeviceInfoProvider()->GetSyncUserAgent();
  params.http_factory_getter = MakeHttpPostProviderFactoryGetter();
  params.credentials = auth_manager_->GetCredentials();
  DCHECK(!params.credentials.account_id.empty() || IsLocalSyncEnabled());
  invalidation::InvalidationService* invalidator =
      sync_client_->GetInvalidationService();
  params.invalidator_client_id =
      invalidator ? invalidator->GetInvalidatorClientId() : "",
  params.sync_manager_factory = std::make_unique<SyncManagerFactory>();
  // The first time we start up the engine we want to ensure we have a clean
  // directory, so delete any old one that might be there.
  params.delete_sync_data_folder = !IsFirstSetupComplete();
  params.enable_local_sync_backend = sync_prefs_.IsLocalSyncEnabled();
  params.local_sync_backend_folder = sync_client_->GetLocalSyncBackendFolder();
  params.restored_key_for_bootstrapping =
      sync_prefs_.GetEncryptionBootstrapToken();
  params.restored_keystore_key_for_bootstrapping =
      sync_prefs_.GetKeystoreEncryptionBootstrapToken();
  params.engine_components_factory =
      std::make_unique<EngineComponentsFactoryImpl>(
          EngineSwitchesFromCommandLine());
  params.unrecoverable_error_handler = GetUnrecoverableErrorHandler();
  params.report_unrecoverable_error_function =
      base::BindRepeating(syncer::ReportUnrecoverableError, channel_);
  params.saved_nigori_state = crypto_->TakeSavedNigoriState();
  sync_prefs_.GetInvalidationVersions(&params.invalidation_versions);
  params.short_poll_interval = sync_prefs_.GetShortPollInterval();
  if (params.short_poll_interval.is_zero()) {
    params.short_poll_interval =
        base::TimeDelta::FromSeconds(syncer::kDefaultShortPollIntervalSeconds);
  }
  params.long_poll_interval = sync_prefs_.GetLongPollInterval();
  if (params.long_poll_interval.is_zero()) {
    params.long_poll_interval =
        base::TimeDelta::FromSeconds(syncer::kDefaultLongPollIntervalSeconds);
  }

  engine_->Initialize(std::move(params));
}

void ProfileSyncService::AccessTokenFetched(
    const GoogleServiceAuthError& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (error.state() == GoogleServiceAuthError::NONE) {
    if (HasSyncingEngine()) {
      engine_->UpdateCredentials(auth_manager_->GetCredentials());
    } else {
      startup_controller_->TryStart();
    }
  }
  // Else: Some error happened. SyncAuthManager takes care of that (retry if
  // appropriate etc), so there's nothing for us to do here.

  NotifyObservers();
}

void ProfileSyncService::OnRefreshTokenAvailable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (HasSyncingEngine()) {
    auth_manager_->RequestAccessToken();
  } else {
    startup_controller_->TryStart();
  }
}

void ProfileSyncService::OnRefreshTokenRevoked() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (HasSyncingEngine())
    engine_->InvalidateCredentials();

  NotifyObservers();
}

void ProfileSyncService::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  ShutdownImpl(syncer::BROWSER_SHUTDOWN);
  NotifyShutdown();
  // TODO(treib): DCHECK that all observers are gone now. All KeyedServices
  // should have unregistered their observers already before, in their own
  // Shutdown(), and all others should have done it now when they got the
  // shutdown notification.
  if (sync_error_controller_) {
    // Destroy the SyncErrorController when the service shuts down for good.
    RemoveObserver(sync_error_controller_.get());
    sync_error_controller_.reset();
  }

  auth_manager_.reset();

  signin_scoped_device_id_callback_.Reset();

  if (sync_thread_)
    sync_thread_->Stop();
}

void ProfileSyncService::ShutdownImpl(syncer::ShutdownReason reason) {
  if (!engine_) {
    if (reason == syncer::ShutdownReason::DISABLE_SYNC && sync_thread_) {
      // If the engine is already shut down when a DISABLE_SYNC happens,
      // the data directory needs to be cleaned up here.
      sync_thread_->task_runner()->PostTask(
          FROM_HERE,
          base::BindOnce(&syncer::syncable::Directory::DeleteDirectoryFiles,
                         FormatSyncDataPath(base_directory_)));
    }
    return;
  }

  if (reason == syncer::ShutdownReason::STOP_SYNC ||
      reason == syncer::ShutdownReason::DISABLE_SYNC) {
    RemoveClientFromServer();
  }

  // First, we spin down the engine to stop change processing as soon as
  // possible.
  base::Time shutdown_start_time = base::Time::Now();
  engine_->StopSyncingForShutdown();

  // Stop all data type controllers, if needed. Note that until Stop completes,
  // it is possible in theory to have a ChangeProcessor apply a change from a
  // native model. In that case, it will get applied to the sync database (which
  // doesn't get destroyed until we destroy the engine below) as an unsynced
  // change. That will be persisted, and committed on restart.
  if (data_type_manager_) {
    if (data_type_manager_->state() != DataTypeManager::STOPPED) {
      // When aborting as part of shutdown, we should expect an aborted sync
      // configure result, else we'll dcheck when we try to read the sync error.
      expect_sync_configuration_aborted_ = true;
      data_type_manager_->Stop();
    }
    data_type_manager_.reset();
  }

  // Shutdown the migrator before the engine to ensure it doesn't pull a null
  // snapshot.
  migrator_.reset();
  sync_js_controller_.AttachJsBackend(WeakHandle<syncer::JsBackend>());

  engine_->Shutdown(reason);
  engine_.reset();

  base::TimeDelta shutdown_time = base::Time::Now() - shutdown_start_time;
  UMA_HISTOGRAM_TIMES("Sync.Shutdown.BackendDestroyedTime", shutdown_time);

  sync_enabled_weak_factory_.InvalidateWeakPtrs();

  startup_controller_->Reset(GetRegisteredDataTypes());

  // If the sync DB is getting destroyed, the local DeviceInfo is no longer
  // valid and should be cleared from the cache.
  if (reason == syncer::ShutdownReason::DISABLE_SYNC) {
    local_device_->Clear();
  }

  // Clear various state.
  ResetCryptoState();
  expect_sync_configuration_aborted_ = false;
  engine_initialized_ = false;
  last_snapshot_ = syncer::SyncCycleSnapshot();
  auth_manager_->Clear();

  NotifyObservers();

  // Mark this as a clean shutdown(without crash).
  sync_prefs_.SetCleanShutdown(true);
}

void ProfileSyncService::StopImpl(SyncStopDataFate data_fate) {
  switch (data_fate) {
    case KEEP_DATA:
      ShutdownImpl(syncer::STOP_SYNC);
      break;
    case CLEAR_DATA:
      // Clear prefs (including SyncSetupHasCompleted) before shutting down so
      // PSS clients don't think we're set up while we're shutting down.
      sync_prefs_.ClearPreferences();
      ClearUnrecoverableError();
      ShutdownImpl(syncer::DISABLE_SYNC);
      break;
  }
}

bool ProfileSyncService::IsFirstSetupComplete() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_prefs_.IsFirstSetupComplete();
}

void ProfileSyncService::SetFirstSetupComplete() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sync_prefs_.SetFirstSetupComplete();
  if (IsEngineInitialized()) {
    ReconfigureDatatypeManager();
  }
}

bool ProfileSyncService::IsSyncConfirmationNeeded() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return (!IsLocalSyncEnabled() && IsSignedIn()) && !IsFirstSetupInProgress() &&
         !IsFirstSetupComplete() && IsSyncRequested();
}

void ProfileSyncService::UpdateLastSyncedTime() {
  sync_prefs_.SetLastSyncedTime(base::Time::Now());
}

void ProfileSyncService::NotifyObservers() {
  for (auto& observer : observers_) {
    observer.OnStateChanged(this);
  }
}

void ProfileSyncService::NotifySyncCycleCompleted() {
  for (auto& observer : observers_)
    observer.OnSyncCycleCompleted(this);
}

void ProfileSyncService::NotifyForeignSessionUpdated() {
  for (auto& observer : observers_)
    observer.OnForeignSessionUpdated(this);
}

void ProfileSyncService::NotifyShutdown() {
  for (auto& observer : observers_)
    observer.OnSyncShutdown(this);
}

void ProfileSyncService::ClearStaleErrors() {
  ClearUnrecoverableError();
  last_actionable_error_ = SyncProtocolError();
  // Clear the data type errors as well.
  if (data_type_manager_)
    data_type_manager_->ResetDataTypeErrors();
}

void ProfileSyncService::ClearUnrecoverableError() {
  unrecoverable_error_reason_ = ERROR_REASON_UNSET;
  unrecoverable_error_message_.clear();
  unrecoverable_error_location_ = base::Location();
}

// An invariant has been violated.  Transition to an error state where we try
// to do as little work as possible, to avoid further corruption or crashes.
void ProfileSyncService::OnUnrecoverableError(const base::Location& from_here,
                                              const std::string& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Unrecoverable errors that arrive via the syncer::UnrecoverableErrorHandler
  // interface are assumed to originate within the syncer.
  unrecoverable_error_reason_ = ERROR_REASON_SYNCER;
  OnUnrecoverableErrorImpl(from_here, message, true);
}

void ProfileSyncService::OnUnrecoverableErrorImpl(
    const base::Location& from_here,
    const std::string& message,
    bool delete_sync_database) {
  DCHECK(HasUnrecoverableError());
  unrecoverable_error_message_ = message;
  unrecoverable_error_location_ = from_here;

  UMA_HISTOGRAM_ENUMERATION(kSyncUnrecoverableErrorHistogram,
                            unrecoverable_error_reason_, ERROR_REASON_LIMIT);
  LOG(ERROR) << "Unrecoverable error detected at " << from_here.ToString()
             << " -- ProfileSyncService unusable: " << message;

  // Shut all data types down.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&ProfileSyncService::ShutdownImpl,
                                sync_enabled_weak_factory_.GetWeakPtr(),
                                delete_sync_database ? syncer::DISABLE_SYNC
                                                     : syncer::STOP_SYNC));
}

void ProfileSyncService::ReenableDatatype(syncer::ModelType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!engine_initialized_ || !data_type_manager_)
    return;
  data_type_manager_->ReenableType(type);
}

void ProfileSyncService::UpdateEngineInitUMA(bool success) {
  is_first_time_sync_configure_ = !IsFirstSetupComplete();

  if (is_first_time_sync_configure_) {
    UMA_HISTOGRAM_BOOLEAN("Sync.BackendInitializeFirstTimeSuccess", success);
  } else {
    UMA_HISTOGRAM_BOOLEAN("Sync.BackendInitializeRestoreSuccess", success);
  }

  base::Time on_engine_initialized_time = base::Time::Now();
  base::TimeDelta delta =
      on_engine_initialized_time - startup_controller_->start_engine_time();
  if (is_first_time_sync_configure_) {
    UMA_HISTOGRAM_LONG_TIMES("Sync.BackendInitializeFirstTime", delta);
  } else {
    UMA_HISTOGRAM_LONG_TIMES("Sync.BackendInitializeRestoreTime", delta);
  }
}

void ProfileSyncService::OnEngineInitialized(
    ModelTypeSet initial_types,
    const syncer::WeakHandle<syncer::JsBackend>& js_backend,
    const syncer::WeakHandle<syncer::DataTypeDebugInfoListener>&
        debug_info_listener,
    const std::string& cache_guid,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UpdateEngineInitUMA(success);

  if (!success) {
    // Something went unexpectedly wrong.  Play it safe: stop syncing at once
    // and surface error UI to alert the user sync has stopped.
    // Keep the directory around for now so that on restart we will retry
    // again and potentially succeed in presence of transient file IO failures
    // or permissions issues, etc.
    //
    // TODO(rlarocque): Consider making this UnrecoverableError less special.
    // Unlike every other UnrecoverableError, it does not delete our sync data.
    // This exception made sense at the time it was implemented, but our new
    // directory corruption recovery mechanism makes it obsolete.  By the time
    // we get here, we will have already tried and failed to delete the
    // directory.  It would be no big deal if we tried to delete it again.
    OnInternalUnrecoverableError(FROM_HERE, "BackendInitialize failure", false,
                                 ERROR_REASON_ENGINE_INIT_FAILURE);
    return;
  }

  engine_initialized_ = true;

  sync_js_controller_.AttachJsBackend(js_backend);

  // Initialize local device info.
  local_device_->Initialize(cache_guid,
                            signin_scoped_device_id_callback_.Run());

  if (protocol_event_observers_.might_have_observers()) {
    engine_->RequestBufferedProtocolEventsAndEnableForwarding();
  }

  if (type_debug_info_observers_.might_have_observers()) {
    engine_->EnableDirectoryTypeDebugInfoForwarding();
  }

  // The very first time the backend initializes is effectively the first time
  // we can say we successfully "synced".  LastSyncedTime will only be null in
  // this case, because the pref wasn't restored on StartUp.
  if (sync_prefs_.GetLastSyncedTime().is_null()) {
    UpdateLastSyncedTime();
  }

  data_type_manager_ =
      sync_client_->GetSyncApiComponentFactory()->CreateDataTypeManager(
          initial_types, debug_info_listener, &data_type_controllers_, this,
          engine_.get(), this);

  crypto_->SetSyncEngine(engine_.get());
  crypto_->SetDataTypeManager(data_type_manager_.get());

  // Auto-start means IsFirstSetupComplete gets set automatically.
  if (start_behavior_ == AUTO_START && !IsFirstSetupComplete()) {
    // This will trigger a configure if it completes setup.
    SetFirstSetupComplete();
  } else if (CanConfigureDataTypes()) {
    ConfigureDataTypeManager();
  }

  // Check for a cookie jar mismatch.
  std::vector<gaia::ListedAccount> accounts;
  std::vector<gaia::ListedAccount> signed_out_accounts;
  GoogleServiceAuthError error(GoogleServiceAuthError::NONE);
  if (gaia_cookie_manager_service_ &&
      gaia_cookie_manager_service_->ListAccounts(
          &accounts, &signed_out_accounts, "ChromiumProfileSyncService")) {
    OnGaiaAccountsInCookieUpdated(accounts, signed_out_accounts, error);
  }

  NotifyObservers();

  // Nobody will call us to start if no sign in is going to happen.
  if (IsLocalSyncEnabled())
    RequestStart();
}

void ProfileSyncService::OnSyncCycleCompleted(
    const syncer::SyncCycleSnapshot& snapshot) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  last_snapshot_ = snapshot;

  UpdateLastSyncedTime();
  if (!snapshot.poll_finish_time().is_null())
    sync_prefs_.SetLastPollTime(snapshot.poll_finish_time());
  DCHECK(!snapshot.short_poll_interval().is_zero());
  sync_prefs_.SetShortPollInterval(snapshot.short_poll_interval());

  DCHECK(!snapshot.long_poll_interval().is_zero());
  sync_prefs_.SetLongPollInterval(snapshot.long_poll_interval());

  if (IsDataTypeControllerRunning(syncer::SESSIONS) &&
      snapshot.model_neutral_state().get_updates_request_types.Has(
          syncer::SESSIONS) &&
      !syncer::HasSyncerError(snapshot.model_neutral_state())) {
    // Trigger garbage collection of old sessions now that we've downloaded
    // any new session data.
    sessions_sync_manager_->ScheduleGarbageCollection();
  }
  DVLOG(2) << "Notifying observers sync cycle completed";
  NotifySyncCycleCompleted();
}

void ProfileSyncService::OnExperimentsChanged(
    const syncer::Experiments& experiments) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (current_experiments_.Matches(experiments))
    return;

  current_experiments_ = experiments;

  sync_client_->GetPrefService()->SetBoolean(
      invalidation::prefs::kInvalidationServiceUseGCMChannel,
      experiments.gcm_invalidations_enabled);
}

void ProfileSyncService::OnConnectionStatusChange(
    syncer::ConnectionStatus status) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auth_manager_->ConnectionStatusChanged(status);
  NotifyObservers();
}

void ProfileSyncService::OnMigrationNeededForTypes(syncer::ModelTypeSet types) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(engine_initialized_);
  DCHECK(data_type_manager_);

  // Migrator must be valid, because we don't sync until it is created and this
  // callback originates from a sync cycle.
  migrator_->MigrateTypes(types);
}

void ProfileSyncService::OnActionableError(const SyncProtocolError& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  last_actionable_error_ = error;
  DCHECK_NE(last_actionable_error_.action, syncer::UNKNOWN_ACTION);
  switch (error.action) {
    case syncer::UPGRADE_CLIENT:
    case syncer::CLEAR_USER_DATA_AND_RESYNC:
    case syncer::ENABLE_SYNC_ON_ACCOUNT:
    case syncer::STOP_AND_RESTART_SYNC:
      // TODO(lipalani) : if setup in progress we want to display these
      // actions in the popup. The current experience might not be optimal for
      // the user. We just dismiss the dialog.
      if (startup_controller_->IsSetupInProgress()) {
        RequestStop(CLEAR_DATA);
        expect_sync_configuration_aborted_ = true;
      }
      // Trigger an unrecoverable error to stop syncing.
      OnInternalUnrecoverableError(FROM_HERE,
                                   last_actionable_error_.error_description,
                                   true, ERROR_REASON_ACTIONABLE_ERROR);
      break;
    case syncer::DISABLE_SYNC_ON_CLIENT:
      if (error.error_type == syncer::NOT_MY_BIRTHDAY) {
        UMA_HISTOGRAM_ENUMERATION("Sync.StopSource", syncer::BIRTHDAY_ERROR,
                                  syncer::STOP_SOURCE_LIMIT);
      }
      RequestStop(CLEAR_DATA);
#if !defined(OS_CHROMEOS)
      // On every platform except ChromeOS, sign out the user after a dashboard
      // clear.
      if (!IsLocalSyncEnabled()) {
        SigninManager::FromSigninManagerBase(signin_->GetSigninManager())
            ->SignOut(signin_metrics::SERVER_FORCED_DISABLE,
                      signin_metrics::SignoutDelete::IGNORE_METRIC);
      }
#endif
      break;
    case syncer::STOP_SYNC_FOR_DISABLED_ACCOUNT:
      // Sync disabled by domain admin. we should stop syncing until next
      // restart.
      sync_disabled_by_admin_ = true;
      ShutdownImpl(syncer::DISABLE_SYNC);
      break;
    case syncer::RESET_LOCAL_SYNC_DATA:
      ShutdownImpl(syncer::DISABLE_SYNC);
      startup_controller_->TryStart();
      UMA_HISTOGRAM_ENUMERATION(
          "Sync.ClearServerDataEvents",
          syncer::CLEAR_SERVER_DATA_RESET_LOCAL_DATA_RECEIVED,
          syncer::CLEAR_SERVER_DATA_MAX);
      break;
    default:
      NOTREACHED();
  }
  NotifyObservers();
}

void ProfileSyncService::ClearAndRestartSyncForPassphraseEncryption() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  engine_->ClearServerData(
      base::BindRepeating(&ProfileSyncService::OnClearServerDataDone,
                          sync_enabled_weak_factory_.GetWeakPtr()));
}

void ProfileSyncService::OnClearServerDataDone() {
  DCHECK(sync_prefs_.GetPassphraseEncryptionTransitionInProgress());
  sync_prefs_.SetPassphraseEncryptionTransitionInProgress(false);

  // Call to ClearServerData generates new keystore key on the server. This
  // makes keystore bootstrap token invalid. Let's clear it from preferences.
  sync_prefs_.SetKeystoreEncryptionBootstrapToken(std::string());

  // Shutdown sync, delete the Directory, then restart, restoring the cached
  // nigori state.
  ShutdownImpl(syncer::DISABLE_SYNC);
  startup_controller_->TryStart();
  UMA_HISTOGRAM_ENUMERATION("Sync.ClearServerDataEvents",
                            syncer::CLEAR_SERVER_DATA_SUCCEEDED,
                            syncer::CLEAR_SERVER_DATA_MAX);
}

void ProfileSyncService::ClearServerDataForTest(const base::Closure& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Sync has a restriction that the engine must be in configuration mode
  // in order to run clear server data.
  engine_->StartConfiguration();
  engine_->ClearServerData(callback);
}

void ProfileSyncService::OnConfigureDone(
    const DataTypeManager::ConfigureResult& result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  data_type_status_table_ = result.data_type_status_table;

  if (!sync_configure_start_time_.is_null()) {
    if (result.status == DataTypeManager::OK) {
      base::Time sync_configure_stop_time = base::Time::Now();
      base::TimeDelta delta =
          sync_configure_stop_time - sync_configure_start_time_;
      if (is_first_time_sync_configure_) {
        UMA_HISTOGRAM_LONG_TIMES("Sync.ServiceInitialConfigureTime", delta);
      } else {
        UMA_HISTOGRAM_LONG_TIMES("Sync.ServiceSubsequentConfigureTime", delta);
      }
    }
    sync_configure_start_time_ = base::Time();
  }

  // Notify listeners that configuration is done.
  for (auto& observer : observers_)
    observer.OnSyncConfigurationCompleted(this);

  DVLOG(1) << "PSS OnConfigureDone called with status: " << result.status;
  // The possible status values:
  //    ABORT - Configuration was aborted. This is not an error, if
  //            initiated by user.
  //    OK - Some or all types succeeded.
  //    Everything else is an UnrecoverableError. So treat it as such.

  // First handle the abort case.
  if (result.status == DataTypeManager::ABORTED &&
      expect_sync_configuration_aborted_) {
    DVLOG(0) << "ProfileSyncService::Observe Sync Configure aborted";
    expect_sync_configuration_aborted_ = false;
    return;
  }

  // Handle unrecoverable error.
  if (result.status != DataTypeManager::OK) {
    if (result.was_catch_up_configure) {
      // Record catchup configuration failure.
      UMA_HISTOGRAM_ENUMERATION("Sync.ClearServerDataEvents",
                                syncer::CLEAR_SERVER_DATA_CATCHUP_FAILED,
                                syncer::CLEAR_SERVER_DATA_MAX);
    }
    // Something catastrophic had happened. We should only have one
    // error representing it.
    syncer::SyncError error = data_type_status_table_.GetUnrecoverableError();
    DCHECK(error.IsSet());
    std::string message =
        "Sync configuration failed with status " +
        DataTypeManager::ConfigureStatusToString(result.status) +
        " caused by " +
        syncer::ModelTypeSetToString(
            data_type_status_table_.GetUnrecoverableErrorTypes()) +
        ": " + error.message();
    LOG(ERROR) << "ProfileSyncService error: " << message;
    OnInternalUnrecoverableError(error.location(), message, true,
                                 ERROR_REASON_CONFIGURATION_FAILURE);
    return;
  }

  DCHECK_EQ(DataTypeManager::OK, result.status);

  // We should never get in a state where we have no encrypted datatypes
  // enabled, and yet we still think we require a passphrase for decryption.
  DCHECK(
      !(IsPassphraseRequiredForDecryption() && !IsEncryptedDatatypeEnabled()));

  // This must be done before we start syncing with the server to avoid
  // sending unencrypted data up on a first time sync.
  if (crypto_->encryption_pending())
    engine_->EnableEncryptEverything();
  NotifyObservers();

  if (migrator_.get() && migrator_->state() != BackendMigrator::IDLE) {
    // Migration in progress.  Let the migrator know we just finished
    // configuring something.  It will be up to the migrator to call
    // StartSyncingWithServer() if migration is now finished.
    migrator_->OnConfigureDone(result);
    return;
  }

  if (result.was_catch_up_configure) {
    ClearAndRestartSyncForPassphraseEncryption();
    return;
  }

  RecordMemoryUsageHistograms();

  StartSyncingWithServer();
}

void ProfileSyncService::OnConfigureStart() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sync_configure_start_time_ = base::Time::Now();
  engine_->StartConfiguration();
  NotifyObservers();
}

std::string ProfileSyncService::QuerySyncStatusSummaryString() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (HasUnrecoverableError())
    return "Unrecoverable error detected";
  if (!engine_)
    return "Syncing not enabled";
  if (!IsFirstSetupComplete())
    return "First time sync setup incomplete";
  if (data_type_manager_ &&
      data_type_manager_->state() == DataTypeManager::STOPPED) {
    return "Datatypes not fully initialized";
  }
  if (IsSyncActive())
    return "Sync service initialized";

  return "Status unknown: Internal error?";
}

std::string ProfileSyncService::GetEngineInitializationStateString() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return startup_controller_->GetEngineInitializationStateString();
}

bool ProfileSyncService::IsSetupInProgress() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return startup_controller_->IsSetupInProgress();
}

bool ProfileSyncService::QueryDetailedSyncStatus(SyncEngine::Status* result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (engine_ && engine_initialized_) {
    *result = engine_->GetDetailedStatus();
    return true;
  }
  SyncEngine::Status status;
  status.sync_protocol_error = last_actionable_error_;
  *result = status;
  return false;
}

const GoogleServiceAuthError& ProfileSyncService::GetAuthError() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auth_manager_->GetLastAuthError();
}

bool ProfileSyncService::CanConfigureDataTypes() const {
  return IsFirstSetupComplete() && !IsSetupInProgress();
}

bool ProfileSyncService::IsFirstSetupInProgress() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !IsFirstSetupComplete() && startup_controller_->IsSetupInProgress();
}

std::unique_ptr<syncer::SyncSetupInProgressHandle>
ProfileSyncService::GetSetupInProgressHandle() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (++outstanding_setup_in_progress_handles_ == 1) {
    DCHECK(!startup_controller_->IsSetupInProgress());
    startup_controller_->SetSetupInProgress(true);

    NotifyObservers();
  }

  return std::make_unique<syncer::SyncSetupInProgressHandle>(
      base::BindRepeating(&ProfileSyncService::OnSetupInProgressHandleDestroyed,
                          weak_factory_.GetWeakPtr()));
}

bool ProfileSyncService::IsSyncAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return IsSyncAllowedByFlag() && !IsManaged() && IsSyncAllowedByPlatform();
}

bool ProfileSyncService::IsSyncActive() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return engine_initialized_ && data_type_manager_ &&
         data_type_manager_->state() != DataTypeManager::STOPPED;
}

bool ProfileSyncService::IsLocalSyncEnabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_prefs_.IsLocalSyncEnabled();
}

void ProfileSyncService::TriggerRefresh(const syncer::ModelTypeSet& types) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (engine_initialized_)
    engine_->TriggerRefresh(types);
}

bool ProfileSyncService::IsSignedIn() const {
  // Sync is logged in if there is a non-empty account id.
  return !GetAuthenticatedAccountInfo().account_id.empty();
}

bool ProfileSyncService::CanEngineStart() const {
  if (IsLocalSyncEnabled())
    return true;
  return CanSyncStart() && auth_manager_->RefreshTokenIsAvailable();
}

bool ProfileSyncService::IsEngineInitialized() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return engine_initialized_;
}

bool ProfileSyncService::ConfigurationDone() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return data_type_manager_ &&
         data_type_manager_->state() == DataTypeManager::CONFIGURED;
}

bool ProfileSyncService::waiting_for_auth() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auth_manager_->IsAuthInProgress();
}

bool ProfileSyncService::HasUnrecoverableError() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return unrecoverable_error_reason_ != ERROR_REASON_UNSET;
}

bool ProfileSyncService::IsPassphraseRequired() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->passphrase_required_reason() !=
         syncer::REASON_PASSPHRASE_NOT_REQUIRED;
}

bool ProfileSyncService::IsPassphraseRequiredForDecryption() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // If there is an encrypted datatype enabled and we don't have the proper
  // passphrase, we must prompt the user for a passphrase. The only way for the
  // user to avoid entering their passphrase is to disable the encrypted types.
  return IsEncryptedDatatypeEnabled() && IsPassphraseRequired();
}

base::Time ProfileSyncService::GetLastSyncedTime() const {
  return sync_prefs_.GetLastSyncedTime();
}

void ProfileSyncService::UpdateSelectedTypesHistogram(
    bool sync_everything,
    const syncer::ModelTypeSet chosen_types) const {
  if (!IsFirstSetupComplete() ||
      sync_everything != sync_prefs_.HasKeepEverythingSynced()) {
    UMA_HISTOGRAM_BOOLEAN("Sync.SyncEverything", sync_everything);
  }

  // Only log the data types that are shown in the sync settings ui.
  // Note: the order of these types must match the ordering of
  // the respective types in ModelType
  const syncer::user_selectable_type::UserSelectableSyncType
      user_selectable_types[] = {
        syncer::user_selectable_type::BOOKMARKS,
        syncer::user_selectable_type::PREFERENCES,
        syncer::user_selectable_type::PASSWORDS,
        syncer::user_selectable_type::AUTOFILL,
        syncer::user_selectable_type::THEMES,
        syncer::user_selectable_type::TYPED_URLS,
        syncer::user_selectable_type::EXTENSIONS,
        syncer::user_selectable_type::APPS,
#if BUILDFLAG(ENABLE_READING_LIST)
        syncer::user_selectable_type::READING_LIST,
#endif
        syncer::user_selectable_type::PROXY_TABS,
      };

  static_assert(41 == syncer::MODEL_TYPE_COUNT,
                "If adding a user selectable type, update "
                "UserSelectableSyncType in user_selectable_sync_type.h and "
                "histograms.xml.");

  if (!sync_everything) {
    const syncer::ModelTypeSet current_types = GetPreferredDataTypes();

    syncer::ModelTypeSet type_set = syncer::UserSelectableTypes();
    syncer::ModelTypeSet::Iterator it = type_set.First();

    DCHECK_EQ(arraysize(user_selectable_types), type_set.Size());

    for (size_t i = 0; i < arraysize(user_selectable_types) && it.Good();
         ++i, it.Inc()) {
      const syncer::ModelType type = it.Get();
      if (chosen_types.Has(type) &&
          (!IsFirstSetupComplete() || !current_types.Has(type))) {
        // Selected type has changed - log it.
        UMA_HISTOGRAM_ENUMERATION(
            "Sync.CustomSync", user_selectable_types[i],
            syncer::user_selectable_type::SELECTABLE_DATATYPE_COUNT + 1);
      }
    }
  }
}

void ProfileSyncService::OnUserChoseDatatypes(
    bool sync_everything,
    syncer::ModelTypeSet chosen_types) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(syncer::UserSelectableTypes().HasAll(chosen_types));

  if (!engine_ && !HasUnrecoverableError()) {
    NOTREACHED();
    return;
  }

  UpdateSelectedTypesHistogram(sync_everything, chosen_types);
  sync_prefs_.SetKeepEverythingSynced(sync_everything);

  if (data_type_manager_)
    data_type_manager_->ResetDataTypeErrors();
  ChangePreferredDataTypes(chosen_types);
}

void ProfileSyncService::ChangePreferredDataTypes(
    syncer::ModelTypeSet preferred_types) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "ChangePreferredDataTypes invoked";
  const syncer::ModelTypeSet registered_types = GetRegisteredDataTypes();
  // Will only enable those types that are registered and preferred.
  sync_prefs_.SetPreferredDataTypes(registered_types, preferred_types);

  // Now reconfigure the DTM.
  ReconfigureDatatypeManager();
}

syncer::ModelTypeSet ProfileSyncService::GetActiveDataTypes() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!data_type_manager_)
    return syncer::ModelTypeSet();
  return data_type_manager_->GetActiveDataTypes();
}

syncer::SyncClient* ProfileSyncService::GetSyncClient() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_client_.get();
}

void ProfileSyncService::AddObserver(syncer::SyncServiceObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void ProfileSyncService::RemoveObserver(syncer::SyncServiceObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

bool ProfileSyncService::HasObserver(
    const syncer::SyncServiceObserver* observer) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return observers_.HasObserver(observer);
}

syncer::ModelTypeSet ProfileSyncService::GetPreferredDataTypes() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const syncer::ModelTypeSet registered_types = GetRegisteredDataTypes();
  const syncer::ModelTypeSet preferred_types =
      Union(sync_prefs_.GetPreferredDataTypes(registered_types),
            syncer::ControlTypes());
  const syncer::ModelTypeSet enforced_types =
      Intersection(GetDataTypesFromPreferenceProviders(), registered_types);
  return Union(preferred_types, enforced_types);
}

syncer::ModelTypeSet ProfileSyncService::GetForcedDataTypes() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(treib,zea): When SyncPrefs also implements SyncTypePreferenceProvider,
  // we'll need another way to distinguish user-choosable types from
  // programmatically-enabled types.
  return GetDataTypesFromPreferenceProviders();
}

syncer::ModelTypeSet ProfileSyncService::GetRegisteredDataTypes() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  syncer::ModelTypeSet registered_types;
  // The data_type_controllers_ are determined by command-line flags;
  // that's effectively what controls the values returned here.
  for (const std::pair<const syncer::ModelType,
                       std::unique_ptr<DataTypeController>>&
           type_and_controller : data_type_controllers_) {
    registered_types.Put(type_and_controller.first);
  }
  return registered_types;
}

bool ProfileSyncService::IsUsingSecondaryPassphrase() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->IsUsingSecondaryPassphrase();
}

std::string ProfileSyncService::GetCustomPassphraseKey() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  syncer::SystemEncryptor encryptor;
  syncer::Cryptographer cryptographer(&encryptor);
  cryptographer.Bootstrap(sync_prefs_.GetEncryptionBootstrapToken());
  return cryptographer.GetDefaultNigoriKeyData();
}

syncer::PassphraseType ProfileSyncService::GetPassphraseType() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->GetPassphraseType();
}

base::Time ProfileSyncService::GetExplicitPassphraseTime() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->GetExplicitPassphraseTime();
}

bool ProfileSyncService::IsCryptographerReady(
    const syncer::BaseTransaction* trans) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return engine_ && engine_->IsCryptographerReady(trans);
}

void ProfileSyncService::SetPlatformSyncAllowedProvider(
    const PlatformSyncAllowedProvider& platform_sync_allowed_provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  platform_sync_allowed_provider_ = platform_sync_allowed_provider;
}

// static
syncer::RepeatingModelTypeStoreFactory
ProfileSyncService::GetModelTypeStoreFactory(const base::FilePath& base_path) {
  // TODO(skym): Verify using AsUTF8Unsafe is okay here. Should work as long
  // as the Local State file is guaranteed to be UTF-8.
  const std::string path =
      FormatSharedModelTypeStorePath(base_path).AsUTF8Unsafe();
  return base::BindRepeating(&ModelTypeStore::CreateStore, path);
}

void ProfileSyncService::ConfigureDataTypeManager() {
  // Don't configure datatypes if the setup UI is still on the screen - this
  // is to help multi-screen setting UIs (like iOS) where they don't want to
  // start syncing data until the user is done configuring encryption options,
  // etc. ReconfigureDatatypeManager() will get called again once the UI calls
  // SetSetupInProgress(false).
  if (!CanConfigureDataTypes()) {
    // If we can't configure the data type manager yet, we should still notify
    // observers. This is to support multiple setup UIs being open at once.
    NotifyObservers();
    return;
  }

  bool restart = false;
  if (!migrator_) {
    restart = true;

    // We create the migrator at the same time.
    migrator_ = std::make_unique<BackendMigrator>(
        debug_identifier_, GetUserShare(), this, data_type_manager_.get(),
        base::BindRepeating(&ProfileSyncService::StartSyncingWithServer,
                            base::Unretained(this)));
  }

  syncer::ModelTypeSet types;
  syncer::ConfigureReason reason = syncer::CONFIGURE_REASON_UNKNOWN;
  types = GetPreferredDataTypes();
  if (restart) {
    // Datatype downloads on restart are generally due to newly supported
    // datatypes (although it's also possible we're picking up where a failed
    // previous configuration left off).
    // TODO(sync): consider detecting configuration recovery and setting
    // the reason here appropriately.
    reason = syncer::CONFIGURE_REASON_NEWLY_ENABLED_DATA_TYPE;
  } else {
    // The user initiated a reconfiguration (either to add or remove types).
    reason = syncer::CONFIGURE_REASON_RECONFIGURATION;
  }

  data_type_manager_->Configure(types, reason);
}

syncer::UserShare* ProfileSyncService::GetUserShare() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (engine_ && engine_initialized_) {
    return engine_->GetUserShare();
  }
  NOTREACHED();
  return nullptr;
}

syncer::SyncCycleSnapshot ProfileSyncService::GetLastCycleSnapshot() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return last_snapshot_;
}

void ProfileSyncService::HasUnsyncedItemsForTest(
    base::OnceCallback<void(bool)> cb) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(HasSyncingEngine());
  DCHECK(engine_initialized_);
  engine_->HasUnsyncedItemsForTest(std::move(cb));
}

BackendMigrator* ProfileSyncService::GetBackendMigratorForTest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return migrator_.get();
}

std::unique_ptr<base::Value> ProfileSyncService::GetTypeStatusMap() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result = std::make_unique<base::ListValue>();

  if (!engine_ || !engine_initialized_) {
    return std::move(result);
  }

  SyncEngine::Status detailed_status = engine_->GetDetailedStatus();
  const ModelTypeSet& throttled_types(detailed_status.throttled_types);
  const ModelTypeSet& backed_off_types(detailed_status.backed_off_types);

  std::unique_ptr<base::DictionaryValue> type_status_header(
      new base::DictionaryValue());
  type_status_header->SetString("status", "header");
  type_status_header->SetString("name", "Model Type");
  type_status_header->SetString("num_entries", "Total Entries");
  type_status_header->SetString("num_live", "Live Entries");
  type_status_header->SetString("message", "Message");
  type_status_header->SetString("state", "State");
  type_status_header->SetString("group_type", "Group Type");
  result->Append(std::move(type_status_header));

  const DataTypeStatusTable::TypeErrorMap error_map =
      data_type_status_table_.GetAllErrors();
  ModelSafeRoutingInfo routing_info;
  engine_->GetModelSafeRoutingInfo(&routing_info);
  const ModelTypeSet registered = GetRegisteredDataTypes();
  for (ModelTypeSet::Iterator it = registered.First(); it.Good(); it.Inc()) {
    ModelType type = it.Get();

    auto type_status = std::make_unique<base::DictionaryValue>();
    type_status->SetString("name", ModelTypeToString(type));
    type_status->SetString("group_type",
                           ModelSafeGroupToString(routing_info[type]));

    if (error_map.find(type) != error_map.end()) {
      const syncer::SyncError& error = error_map.find(type)->second;
      DCHECK(error.IsSet());
      switch (error.GetSeverity()) {
        case syncer::SyncError::SYNC_ERROR_SEVERITY_ERROR:
          type_status->SetString("status", "error");
          type_status->SetString(
              "message", "Error: " + error.location().ToString() + ", " +
                             error.GetMessagePrefix() + error.message());
          break;
        case syncer::SyncError::SYNC_ERROR_SEVERITY_INFO:
          type_status->SetString("status", "disabled");
          type_status->SetString("message", error.message());
          break;
      }
    } else if (throttled_types.Has(type)) {
      type_status->SetString("status", "warning");
      type_status->SetString("message", " Throttled");
    } else if (backed_off_types.Has(type)) {
      type_status->SetString("status", "warning");
      type_status->SetString("message", "Backed off");
    } else if (routing_info.find(type) != routing_info.end()) {
      type_status->SetString("status", "ok");
      type_status->SetString("message", "");
    } else {
      type_status->SetString("status", "warning");
      type_status->SetString("message", "Disabled by User");
    }

    const auto& dtc_iter = data_type_controllers_.find(type);
    if (dtc_iter != data_type_controllers_.end()) {
      // OnDatatypeStatusCounterUpdated that posts back to the UI thread so that
      // real results can't get overwritten by the empty counters set at the end
      // of this method.
      dtc_iter->second->GetStatusCounters(
          BindToCurrentSequence(base::BindRepeating(
              &ProfileSyncService::OnDatatypeStatusCounterUpdated,
              base::Unretained(this))));
      type_status->SetString("state", DataTypeController::StateToString(
                                          dtc_iter->second->state()));
    }

    result->Append(std::move(type_status));
  }
  return std::move(result);
}

void ProfileSyncService::SetEncryptionPassphrase(const std::string& passphrase,
                                                 PassphraseType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  crypto_->SetEncryptionPassphrase(passphrase, type == EXPLICIT);
}

bool ProfileSyncService::SetDecryptionPassphrase(
    const std::string& passphrase) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (IsPassphraseRequired()) {
    DVLOG(1) << "Setting passphrase for decryption.";
    bool result = crypto_->SetDecryptionPassphrase(passphrase);
    UMA_HISTOGRAM_BOOLEAN("Sync.PassphraseDecryptionSucceeded", result);
    return result;
  }
  NOTREACHED() << "SetDecryptionPassphrase must not be called when "
                  "IsPassphraseRequired() is false.";
  return false;
}

bool ProfileSyncService::IsEncryptEverythingAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->IsEncryptEverythingAllowed();
}

void ProfileSyncService::SetEncryptEverythingAllowed(bool allowed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  crypto_->SetEncryptEverythingAllowed(allowed);
}

void ProfileSyncService::EnableEncryptEverything() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  crypto_->EnableEncryptEverything();
}

bool ProfileSyncService::encryption_pending() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // We may be called during the setup process before we're
  // initialized (via IsEncryptedDatatypeEnabled and
  // IsPassphraseRequiredForDecryption).
  return crypto_->encryption_pending();
}

bool ProfileSyncService::IsEncryptEverythingEnabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->IsEncryptEverythingEnabled();
}

syncer::ModelTypeSet ProfileSyncService::GetEncryptedDataTypes() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return crypto_->GetEncryptedDataTypes();
}

void ProfileSyncService::OnSyncManagedPrefChange(bool is_sync_managed) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (is_sync_managed) {
    StopImpl(CLEAR_DATA);
  } else {
    // Sync is no longer disabled by policy. Try starting it up if appropriate.
    startup_controller_->TryStart();
  }
}

void ProfileSyncService::OnPrimaryAccountSet() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!engine_);
}

void ProfileSyncService::OnPrimaryAccountCleared() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sync_disabled_by_admin_ = false;
  RequestStop(CLEAR_DATA);
  DCHECK(!engine_);
}

void ProfileSyncService::OnGaiaAccountsInCookieUpdated(
    const std::vector<gaia::ListedAccount>& accounts,
    const std::vector<gaia::ListedAccount>& signed_out_accounts,
    const GoogleServiceAuthError& error) {
  OnGaiaAccountsInCookieUpdatedWithCallback(accounts, base::Closure());
}

void ProfileSyncService::OnGaiaAccountsInCookieUpdatedWithCallback(
    const std::vector<gaia::ListedAccount>& accounts,
    const base::Closure& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsEngineInitialized())
    return;

  bool cookie_jar_mismatch = HasCookieJarMismatch(accounts);
  bool cookie_jar_empty = accounts.size() == 0;

  DVLOG(1) << "Cookie jar mismatch: " << cookie_jar_mismatch;
  DVLOG(1) << "Cookie jar empty: " << cookie_jar_empty;
  engine_->OnCookieJarChanged(cookie_jar_mismatch, cookie_jar_empty, callback);
}

bool ProfileSyncService::HasCookieJarMismatch(
    const std::vector<gaia::ListedAccount>& cookie_jar_accounts) {
  std::string account_id = GetAuthenticatedAccountInfo().account_id;
  // Iterate through list of accounts, looking for current sync account.
  for (const auto& account : cookie_jar_accounts) {
    if (account.id == account_id)
      return false;
  }
  return true;
}

void ProfileSyncService::AddProtocolEventObserver(
    ProtocolEventObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  protocol_event_observers_.AddObserver(observer);
  if (HasSyncingEngine()) {
    engine_->RequestBufferedProtocolEventsAndEnableForwarding();
  }
}

void ProfileSyncService::RemoveProtocolEventObserver(
    ProtocolEventObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  protocol_event_observers_.RemoveObserver(observer);
  if (HasSyncingEngine() && !protocol_event_observers_.might_have_observers()) {
    engine_->DisableProtocolEventForwarding();
  }
}

void ProfileSyncService::AddTypeDebugInfoObserver(
    syncer::TypeDebugInfoObserver* type_debug_info_observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  type_debug_info_observers_.AddObserver(type_debug_info_observer);
  if (type_debug_info_observers_.might_have_observers() &&
      engine_initialized_) {
    engine_->EnableDirectoryTypeDebugInfoForwarding();
  }
}

void ProfileSyncService::RemoveTypeDebugInfoObserver(
    syncer::TypeDebugInfoObserver* type_debug_info_observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  type_debug_info_observers_.RemoveObserver(type_debug_info_observer);
  if (!type_debug_info_observers_.might_have_observers() &&
      engine_initialized_) {
    engine_->DisableDirectoryTypeDebugInfoForwarding();
  }
}

void ProfileSyncService::AddPreferenceProvider(
    syncer::SyncTypePreferenceProvider* provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!HasPreferenceProvider(provider))
      << "Providers may only be added once!";
  preference_providers_.insert(provider);
}

void ProfileSyncService::RemovePreferenceProvider(
    syncer::SyncTypePreferenceProvider* provider) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(HasPreferenceProvider(provider))
      << "Only providers that have been added before can be removed!";
  preference_providers_.erase(provider);
}

bool ProfileSyncService::HasPreferenceProvider(
    syncer::SyncTypePreferenceProvider* provider) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return preference_providers_.count(provider) > 0;
}

namespace {

class GetAllNodesRequestHelper
    : public base::RefCountedThreadSafe<GetAllNodesRequestHelper> {
 public:
  GetAllNodesRequestHelper(
      syncer::ModelTypeSet requested_types,
      base::OnceCallback<void(std::unique_ptr<base::ListValue>)> callback);

  void OnReceivedNodesForType(const syncer::ModelType type,
                              std::unique_ptr<base::ListValue> node_list);

 private:
  friend class base::RefCountedThreadSafe<GetAllNodesRequestHelper>;
  virtual ~GetAllNodesRequestHelper();

  std::unique_ptr<base::ListValue> result_accumulator_;
  syncer::ModelTypeSet awaiting_types_;
  base::OnceCallback<void(std::unique_ptr<base::ListValue>)> callback_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(GetAllNodesRequestHelper);
};

GetAllNodesRequestHelper::GetAllNodesRequestHelper(
    syncer::ModelTypeSet requested_types,
    base::OnceCallback<void(std::unique_ptr<base::ListValue>)> callback)
    : result_accumulator_(std::make_unique<base::ListValue>()),
      awaiting_types_(requested_types),
      callback_(std::move(callback)) {}

GetAllNodesRequestHelper::~GetAllNodesRequestHelper() {
  if (!awaiting_types_.Empty()) {
    DLOG(WARNING)
        << "GetAllNodesRequest deleted before request was fulfilled.  "
        << "Missing types are: " << ModelTypeSetToString(awaiting_types_);
  }
}

// Called when the set of nodes for a type has been returned.
// Only return one type of nodes each time.
void GetAllNodesRequestHelper::OnReceivedNodesForType(
    const syncer::ModelType type,
    std::unique_ptr<base::ListValue> node_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Add these results to our list.
  base::DictionaryValue type_dict;
  type_dict.SetKey("type", base::Value(ModelTypeToString(type)));
  type_dict.SetKey("nodes",
                   base::Value::FromUniquePtrValue(std::move(node_list)));
  result_accumulator_->GetList().push_back(std::move(type_dict));

  // Remember that this part of the request is satisfied.
  awaiting_types_.Remove(type);

  if (awaiting_types_.Empty()) {
    std::move(callback_).Run(std::move(result_accumulator_));
  }
}

}  // namespace

void ProfileSyncService::GetAllNodes(
    const base::Callback<void(std::unique_ptr<base::ListValue>)>& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If the engine isn't initialized yet, then there are no nodes to return.
  if (!engine_initialized_) {
    callback.Run(std::make_unique<base::ListValue>());
    return;
  }

  ModelTypeSet all_types = GetActiveDataTypes();
  all_types.PutAll(syncer::ControlTypes());
  scoped_refptr<GetAllNodesRequestHelper> helper =
      new GetAllNodesRequestHelper(all_types, callback);

  for (ModelTypeSet::Iterator it = all_types.First(); it.Good(); it.Inc()) {
    ModelType type = it.Get();
    const auto dtc_iter = data_type_controllers_.find(type);
    if (dtc_iter != data_type_controllers_.end()) {
      dtc_iter->second->GetAllNodes(base::BindRepeating(
          &GetAllNodesRequestHelper::OnReceivedNodesForType, helper));
    } else {
      // Control Types.
      helper->OnReceivedNodesForType(
          type,
          syncer::DirectoryDataTypeController::GetAllNodesForTypeFromDirectory(
              type, GetUserShare()->directory.get()));
    }
  }
}

AccountInfo ProfileSyncService::GetAuthenticatedAccountInfo() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auth_manager_->GetAuthenticatedAccountInfo();
}

syncer::GlobalIdMapper* ProfileSyncService::GetGlobalIdMapper() const {
  return sessions_sync_manager_->GetGlobalIdMapper();
}

base::WeakPtr<syncer::JsController> ProfileSyncService::GetJsController() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_js_controller_.AsWeakPtr();
}

// static
void ProfileSyncService::SyncEvent(SyncEventCodes code) {
  UMA_HISTOGRAM_ENUMERATION("Sync.EventCodes", code, MAX_SYNC_EVENT_CODE);
}

// static
bool ProfileSyncService::IsSyncAllowedByFlag() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableSync);
}

bool ProfileSyncService::IsSyncAllowedByPlatform() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return platform_sync_allowed_provider_.is_null() ||
         platform_sync_allowed_provider_.Run();
}

bool ProfileSyncService::IsManaged() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_prefs_.IsManaged() || sync_disabled_by_admin_;
}

void ProfileSyncService::RequestStop(SyncStopDataFate data_fate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sync_prefs_.SetSyncRequested(false);
  StopImpl(data_fate);
}

bool ProfileSyncService::IsSyncRequested() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // When local sync is on sync should be considered requsted or otherwise it
  // will not resume after the policy or the flag has been removed.
  return sync_prefs_.IsSyncRequested() || sync_prefs_.IsLocalSyncEnabled();
}

void ProfileSyncService::RequestStart() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsSyncAllowed()) {
    // Sync cannot be requested if it's not allowed.
    return;
  }
  DCHECK(sync_client_);
  if (!IsSyncRequested()) {
    sync_prefs_.SetSyncRequested(true);
    NotifyObservers();
  }
  startup_controller_->TryStartImmediately();
}

void ProfileSyncService::ReconfigureDatatypeManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // If we haven't initialized yet, don't configure the DTM as it could cause
  // association to start before a Directory has even been created.
  if (engine_initialized_) {
    DCHECK(engine_);
    ConfigureDataTypeManager();
  } else if (HasUnrecoverableError()) {
    // There is nothing more to configure. So inform the listeners,
    NotifyObservers();

    DVLOG(1) << "ConfigureDataTypeManager not invoked because of an "
             << "Unrecoverable error.";
  } else {
    DVLOG(0) << "ConfigureDataTypeManager not invoked because engine is not "
             << "initialized";
  }
}

syncer::ModelTypeSet ProfileSyncService::GetDataTypesFromPreferenceProviders()
    const {
  syncer::ModelTypeSet types;
  for (const syncer::SyncTypePreferenceProvider* provider :
       preference_providers_) {
    types.PutAll(provider->GetPreferredDataTypes());
  }
  return types;
}

const DataTypeStatusTable& ProfileSyncService::data_type_status_table() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return data_type_status_table_;
}

void ProfileSyncService::OnInternalUnrecoverableError(
    const base::Location& from_here,
    const std::string& message,
    bool delete_sync_database,
    UnrecoverableErrorReason reason) {
  DCHECK(!HasUnrecoverableError());
  unrecoverable_error_reason_ = reason;
  OnUnrecoverableErrorImpl(from_here, message, delete_sync_database);
}

bool ProfileSyncService::IsRetryingAccessTokenFetchForTest() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auth_manager_->IsRetryingAccessTokenFetchForTest();
}

std::string ProfileSyncService::GetAccessTokenForTest() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auth_manager_->access_token();
}

syncer::SyncableService* ProfileSyncService::GetSessionsSyncableService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!sessions_sync_manager_)
    return nullptr;
  return sessions_sync_manager_->GetSyncableService();
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
ProfileSyncService::GetSessionSyncControllerDelegateOnUIThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!sessions_sync_manager_)
    return nullptr;
  return sessions_sync_manager_->GetModelTypeSyncBridge()
      ->change_processor()
      ->GetControllerDelegateOnUIThread();
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
ProfileSyncService::GetDeviceInfoSyncControllerDelegateOnUIThread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return device_info_sync_bridge_->change_processor()
      ->GetControllerDelegateOnUIThread();
}

syncer::SyncTokenStatus ProfileSyncService::GetSyncTokenStatus() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return auth_manager_->GetSyncTokenStatus();
}

void ProfileSyncService::OverrideNetworkResourcesForTest(
    std::unique_ptr<syncer::NetworkResources> network_resources) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_resources_ = std::move(network_resources);
}

bool ProfileSyncService::HasSyncingEngine() const {
  return engine_ != nullptr;
}

void ProfileSyncService::UpdateFirstSyncTimePref() {
  if (!IsLocalSyncEnabled() && !IsSignedIn()) {
    sync_prefs_.ClearFirstSyncTime();
  } else if (sync_prefs_.GetFirstSyncTime().is_null()) {
    // Set if not set before and it's syncing now.
    sync_prefs_.SetFirstSyncTime(base::Time::Now());
  }
}

void ProfileSyncService::FlushDirectory() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // engine_initialized_ implies engine_ isn't null and the manager exists.
  // If sync is not initialized yet, we fail silently.
  if (engine_initialized_)
    engine_->FlushDirectory();
}

base::FilePath ProfileSyncService::GetDirectoryPathForTest() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return FormatSyncDataPath(base_directory_);
}

base::MessageLoop* ProfileSyncService::GetSyncLoopForTest() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_thread_ ? sync_thread_->message_loop() : nullptr;
}

syncer::SyncEncryptionHandler::Observer*
ProfileSyncService::GetEncryptionObserverForTest() const {
  return crypto_.get();
}

void ProfileSyncService::RemoveClientFromServer() const {
  if (!engine_initialized_)
    return;
  const std::string cache_guid = local_device_->GetLocalSyncCacheGUID();
  std::string birthday;
  syncer::UserShare* user_share = GetUserShare();
  if (user_share && user_share->directory.get()) {
    birthday = user_share->directory->store_birthday();
  }
  const std::string& access_token = auth_manager_->access_token();
  if (!access_token.empty() && !cache_guid.empty() && !birthday.empty()) {
    sync_stopped_reporter_->ReportSyncStopped(access_token, cache_guid,
                                              birthday);
  }
}

void ProfileSyncService::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  if (memory_pressure_level ==
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL) {
    sync_prefs_.SetMemoryPressureWarningCount(
        sync_prefs_.GetMemoryPressureWarningCount() + 1);
  }
}

void ProfileSyncService::ReportPreviousSessionMemoryWarningCount() {
  int warning_received = sync_prefs_.GetMemoryPressureWarningCount();

  if (-1 != warning_received) {
    // -1 means it is new client.
    if (!sync_prefs_.DidSyncShutdownCleanly()) {
      UMA_HISTOGRAM_COUNTS("Sync.MemoryPressureWarningBeforeUncleanShutdown",
                           warning_received);
    } else {
      UMA_HISTOGRAM_COUNTS("Sync.MemoryPressureWarningBeforeCleanShutdown",
                           warning_received);
    }
  }
  sync_prefs_.SetMemoryPressureWarningCount(0);
  // Will set to true during a clean shutdown, so crash or something else will
  // remain this as false.
  sync_prefs_.SetCleanShutdown(false);
}

void ProfileSyncService::RecordMemoryUsageHistograms() {
  ModelTypeSet active_types = GetActiveDataTypes();
  for (ModelTypeSet::Iterator type_it = active_types.First(); type_it.Good();
       type_it.Inc()) {
    auto dtc_it = data_type_controllers_.find(type_it.Get());
    if (dtc_it != data_type_controllers_.end())
      dtc_it->second->RecordMemoryUsageHistogram();
  }
}

const GURL& ProfileSyncService::sync_service_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return sync_service_url_;
}

std::string ProfileSyncService::unrecoverable_error_message() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return unrecoverable_error_message_;
}

base::Location ProfileSyncService::unrecoverable_error_location() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return unrecoverable_error_location_;
}

void ProfileSyncService::OnSetupInProgressHandleDestroyed() {
  DCHECK_GT(outstanding_setup_in_progress_handles_, 0);

  // Don't re-start Sync until all outstanding handles are destroyed.
  if (--outstanding_setup_in_progress_handles_ != 0)
    return;

  DCHECK(startup_controller_->IsSetupInProgress());
  startup_controller_->SetSetupInProgress(false);

  if (IsEngineInitialized())
    ReconfigureDatatypeManager();
  NotifyObservers();
}

}  // namespace browser_sync
