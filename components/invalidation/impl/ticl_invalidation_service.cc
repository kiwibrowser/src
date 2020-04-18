// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/ticl_invalidation_service.h"

#include <stddef.h>
#include <utility>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/invalidation/impl/gcm_invalidation_bridge.h"
#include "components/invalidation/impl/invalidation_service_util.h"
#include "components/invalidation/impl/invalidator.h"
#include "components/invalidation/impl/non_blocking_invalidator.h"
#include "components/invalidation/public/invalidation_util.h"
#include "components/invalidation/public/invalidator_state.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/url_request/url_request_context_getter.h"

static const char* kOAuth2Scopes[] = {
  GaiaConstants::kGoogleTalkOAuth2Scope
};

static const net::BackoffEntry::Policy kRequestAccessTokenBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential back-off in ms.
  2000,

  // Factor by which the waiting time will be multiplied.
  2,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0.2, // 20%

  // Maximum amount of time we are willing to delay our request in ms.
  // TODO(pavely): crbug.com/246686 ProfileSyncService should retry
  // RequestAccessToken on connection state change after backoff
  1000 * 3600 * 4, // 4 hours.

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

namespace invalidation {

TiclInvalidationService::TiclInvalidationService(
    const std::string& user_agent,
    std::unique_ptr<IdentityProvider> identity_provider,
    std::unique_ptr<TiclSettingsProvider> settings_provider,
    gcm::GCMDriver* gcm_driver,
    const scoped_refptr<net::URLRequestContextGetter>& request_context)
    : OAuth2TokenService::Consumer("ticl_invalidation"),
      user_agent_(user_agent),
      identity_provider_(std::move(identity_provider)),
      settings_provider_(std::move(settings_provider)),
      invalidator_registrar_(new syncer::InvalidatorRegistrar()),
      request_access_token_backoff_(&kRequestAccessTokenBackoffPolicy),
      network_channel_type_(GCM_NETWORK_CHANNEL),
      gcm_driver_(gcm_driver),
      request_context_(request_context) {}

TiclInvalidationService::~TiclInvalidationService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  invalidator_registrar_->UpdateInvalidatorState(
      syncer::INVALIDATOR_SHUTTING_DOWN);
  settings_provider_->RemoveObserver(this);
  identity_provider_->RemoveActiveAccountRefreshTokenObserver(this);
  identity_provider_->RemoveObserver(this);
  if (IsStarted()) {
    StopInvalidator();
  }
}

void TiclInvalidationService::Init(
    std::unique_ptr<syncer::InvalidationStateTracker>
        invalidation_state_tracker) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  invalidation_state_tracker_ = std::move(invalidation_state_tracker);

  if (invalidation_state_tracker_->GetInvalidatorClientId().empty()) {
    invalidation_state_tracker_->ClearAndSetNewClientId(
        GenerateInvalidatorClientId());
  }

  UpdateInvalidationNetworkChannel();
  if (IsReadyToStart()) {
    StartInvalidator(network_channel_type_);
  }

  identity_provider_->AddObserver(this);
  identity_provider_->AddActiveAccountRefreshTokenObserver(this);
  settings_provider_->AddObserver(this);
}

void TiclInvalidationService::InitForTest(
    std::unique_ptr<syncer::InvalidationStateTracker>
        invalidation_state_tracker,
    syncer::Invalidator* invalidator) {
  // Here we perform the equivalent of Init() and StartInvalidator(), but with
  // some minor changes to account for the fact that we're injecting the
  // invalidator.
  invalidation_state_tracker_ = std::move(invalidation_state_tracker);
  invalidator_.reset(invalidator);

  invalidator_->RegisterHandler(this);
  CHECK(invalidator_->UpdateRegisteredIds(
      this, invalidator_registrar_->GetAllRegisteredIds()));
}

void TiclInvalidationService::RegisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "Registering an invalidation handler";
  invalidator_registrar_->RegisterHandler(handler);
  logger_.OnRegistration(handler->GetOwnerName());
}

bool TiclInvalidationService::UpdateRegisteredInvalidationIds(
    syncer::InvalidationHandler* handler,
    const syncer::ObjectIdSet& ids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "Registering ids: " << ids.size();
  if (!invalidator_registrar_->UpdateRegisteredIds(handler, ids))
    return false;
  if (invalidator_) {
    CHECK(invalidator_->UpdateRegisteredIds(
        this, invalidator_registrar_->GetAllRegisteredIds()));
  }
  logger_.OnUpdateIds(invalidator_registrar_->GetSanitizedHandlersIdsMap());
  return true;
}

void TiclInvalidationService::UnregisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "Unregistering";
  invalidator_registrar_->UnregisterHandler(handler);
  if (invalidator_) {
    CHECK(invalidator_->UpdateRegisteredIds(
        this, invalidator_registrar_->GetAllRegisteredIds()));
  }
  logger_.OnUnregistration(handler->GetOwnerName());
}

syncer::InvalidatorState TiclInvalidationService::GetInvalidatorState() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (invalidator_) {
    DVLOG(2) << "GetInvalidatorState returning "
        << invalidator_->GetInvalidatorState();
    return invalidator_->GetInvalidatorState();
  }
  DVLOG(2) << "Invalidator currently stopped";
  return syncer::TRANSIENT_INVALIDATION_ERROR;
}

std::string TiclInvalidationService::GetInvalidatorClientId() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return invalidation_state_tracker_->GetInvalidatorClientId();
}

InvalidationLogger* TiclInvalidationService::GetInvalidationLogger() {
  return &logger_;
}

void TiclInvalidationService::RequestDetailedStatus(
    base::Callback<void(const base::DictionaryValue&)> return_callback) const {
  if (IsStarted()) {
    return_callback.Run(network_channel_options_);
    invalidator_->RequestDetailedStatus(return_callback);
  }
}

void TiclInvalidationService::RequestAccessToken() {
  // Only one active request at a time.
  if (access_token_request_ != nullptr)
    return;
  request_access_token_retry_timer_.Stop();
  OAuth2TokenService::ScopeSet oauth2_scopes;
  for (size_t i = 0; i < arraysize(kOAuth2Scopes); i++)
    oauth2_scopes.insert(kOAuth2Scopes[i]);
  // Invalidate previous token, otherwise token service will return the same
  // token again.
  const std::string& account_id = identity_provider_->GetActiveAccountId();
  OAuth2TokenService* token_service = identity_provider_->GetTokenService();
  token_service->InvalidateAccessToken(account_id, oauth2_scopes,
                                       access_token_);
  access_token_.clear();
  access_token_request_ =
      token_service->StartRequest(account_id, oauth2_scopes, this);
}

void TiclInvalidationService::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_EQ(access_token_request_.get(), request);
  access_token_request_.reset();
  // Reset backoff time after successful response.
  request_access_token_backoff_.Reset();
  access_token_ = access_token;
  if (!IsStarted() && IsReadyToStart()) {
    StartInvalidator(network_channel_type_);
  } else {
    UpdateInvalidatorCredentials();
  }
}

void TiclInvalidationService::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(access_token_request_.get(), request);
  DCHECK_NE(error.state(), GoogleServiceAuthError::NONE);
  access_token_request_.reset();
  switch (error.state()) {
    case GoogleServiceAuthError::CONNECTION_FAILED:
    case GoogleServiceAuthError::SERVICE_UNAVAILABLE: {
      // Transient error. Retry after some time.
      request_access_token_backoff_.InformOfRequest(false);
      request_access_token_retry_timer_.Start(
            FROM_HERE,
            request_access_token_backoff_.GetTimeUntilRelease(),
            base::Bind(&TiclInvalidationService::RequestAccessToken,
                       base::Unretained(this)));
      break;
    }
    case GoogleServiceAuthError::SERVICE_ERROR:
    case GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS: {
      invalidator_registrar_->UpdateInvalidatorState(
          syncer::INVALIDATION_CREDENTIALS_REJECTED);
      break;
    }
    default: {
      // We have no way to notify the user of this.  Do nothing.
    }
  }
}

void TiclInvalidationService::OnActiveAccountLogin() {
  if (!IsStarted() && IsReadyToStart())
    StartInvalidator(network_channel_type_);
}

void TiclInvalidationService::OnRefreshTokenAvailable(
    const std::string& account_id) {
  if (!IsStarted() && IsReadyToStart())
    StartInvalidator(network_channel_type_);
}

void TiclInvalidationService::OnRefreshTokenRevoked(
    const std::string& account_id) {
  access_token_.clear();
  if (IsStarted())
    UpdateInvalidatorCredentials();
}

void TiclInvalidationService::OnActiveAccountLogout() {
  access_token_request_.reset();
  request_access_token_retry_timer_.Stop();

  if (gcm_invalidation_bridge_)
    gcm_invalidation_bridge_->Unregister();

  if (IsStarted()) {
    StopInvalidator();
  }

  // This service always expects to have a valid invalidation state. Thus, we
  // must generate a new client ID to replace the existing one. Setting a new
  // client ID also clears all other state.
  invalidation_state_tracker_->
      ClearAndSetNewClientId(GenerateInvalidatorClientId());
}

void TiclInvalidationService::OnUseGCMChannelChanged() {
  UpdateInvalidationNetworkChannel();
}

void TiclInvalidationService::OnInvalidatorStateChange(
    syncer::InvalidatorState state) {
  if (state == syncer::INVALIDATION_CREDENTIALS_REJECTED) {
    // This may be due to normal OAuth access token expiration.  If so, we must
    // fetch a new one using our refresh token.  Resetting the invalidator's
    // access token will not reset the invalidator's exponential backoff, so
    // it's safe to try to update the token every time we receive this signal.
    //
    // We won't be receiving any invalidations while the refresh is in progress,
    // we set our state to TRANSIENT_INVALIDATION_ERROR.  If the credentials
    // really are invalid, the refresh request should fail and
    // OnGetTokenFailure() will put us into a INVALIDATION_CREDENTIALS_REJECTED
    // state.
    invalidator_registrar_->UpdateInvalidatorState(
        syncer::TRANSIENT_INVALIDATION_ERROR);
    RequestAccessToken();
  } else {
    invalidator_registrar_->UpdateInvalidatorState(state);
  }
  logger_.OnStateChange(state);
}

void TiclInvalidationService::OnIncomingInvalidation(
    const syncer::ObjectIdInvalidationMap& invalidation_map) {
  invalidator_registrar_->DispatchInvalidationsToHandlers(invalidation_map);

  logger_.OnInvalidation(invalidation_map);
}

std::string TiclInvalidationService::GetOwnerName() const { return "TICL"; }

bool TiclInvalidationService::IsReadyToStart() {
  if (identity_provider_->GetActiveAccountId().empty()) {
    DVLOG(2) << "Not starting TiclInvalidationService: User is not signed in.";
    return false;
  }

  OAuth2TokenService* token_service = identity_provider_->GetTokenService();
  if (!token_service) {
    DVLOG(2)
        << "Not starting TiclInvalidationService: "
        << "OAuth2TokenService unavailable.";
    return false;
  }

  if (!token_service->RefreshTokenIsAvailable(
          identity_provider_->GetActiveAccountId())) {
    DVLOG(2)
        << "Not starting TiclInvalidationServce: Waiting for refresh token.";
    return false;
  }

  return true;
}

bool TiclInvalidationService::IsStarted() const {
  return invalidator_ != nullptr;
}

void TiclInvalidationService::StartInvalidator(
    InvalidationNetworkChannel network_channel) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!invalidator_);
  DCHECK(invalidation_state_tracker_);
  DCHECK(!invalidation_state_tracker_->GetInvalidatorClientId().empty());

  // Request access token for PushClientChannel. GCMNetworkChannel will request
  // access token before sending message to server.
  if (network_channel == PUSH_CLIENT_CHANNEL && access_token_.empty()) {
    DVLOG(1)
        << "TiclInvalidationService: "
        << "Deferring start until we have an access token.";
    RequestAccessToken();
    return;
  }

  syncer::NetworkChannelCreator network_channel_creator;

  switch (network_channel) {
    case PUSH_CLIENT_CHANNEL: {
      notifier::NotifierOptions options =
          ParseNotifierOptions(*base::CommandLine::ForCurrentProcess());
      options.request_context_getter = request_context_;
      options.auth_mechanism = "X-OAUTH2";
      network_channel_options_.SetString("Options.HostPort",
                                         options.xmpp_host_port.ToString());
      network_channel_options_.SetString("Options.AuthMechanism",
                                         options.auth_mechanism);
      DCHECK_EQ(notifier::NOTIFICATION_SERVER, options.notification_method);
      network_channel_creator =
          syncer::NonBlockingInvalidator::MakePushClientChannelCreator(options);
      break;
    }
    case GCM_NETWORK_CHANNEL: {
      gcm_invalidation_bridge_.reset(new GCMInvalidationBridge(
          gcm_driver_, identity_provider_.get()));
      network_channel_creator =
          syncer::NonBlockingInvalidator::MakeGCMNetworkChannelCreator(
              request_context_, gcm_invalidation_bridge_->CreateDelegate());
      break;
    }
    default: {
      NOTREACHED();
      return;
    }
  }

  UMA_HISTOGRAM_ENUMERATION(
      "Invalidations.NetworkChannel", network_channel, NETWORK_CHANNELS_COUNT);
  invalidator_.reset(new syncer::NonBlockingInvalidator(
          network_channel_creator,
          invalidation_state_tracker_->GetInvalidatorClientId(),
          invalidation_state_tracker_->GetSavedInvalidations(),
          invalidation_state_tracker_->GetBootstrapData(),
          invalidation_state_tracker_.get(),
          user_agent_,
          request_context_));

  UpdateInvalidatorCredentials();

  invalidator_->RegisterHandler(this);
  CHECK(invalidator_->UpdateRegisteredIds(
      this, invalidator_registrar_->GetAllRegisteredIds()));
}

void TiclInvalidationService::UpdateInvalidationNetworkChannel() {
  const InvalidationNetworkChannel network_channel_type =
      settings_provider_->UseGCMChannel() ? GCM_NETWORK_CHANNEL
                                          : PUSH_CLIENT_CHANNEL;
  if (network_channel_type_ == network_channel_type)
    return;
  network_channel_type_ = network_channel_type;
  if (IsStarted()) {
    StopInvalidator();
    StartInvalidator(network_channel_type_);
  }
}

void TiclInvalidationService::UpdateInvalidatorCredentials() {
  std::string email = identity_provider_->GetActiveAccountId();

  DCHECK(!email.empty()) << "Expected user to be signed in.";

  DVLOG(2) << "UpdateCredentials: " << email;
  invalidator_->UpdateCredentials(email, access_token_);
}

void TiclInvalidationService::StopInvalidator() {
  DCHECK(invalidator_);
  gcm_invalidation_bridge_.reset();
  invalidator_->UnregisterHandler(this);
  invalidator_.reset();
}

}  // namespace invalidation
