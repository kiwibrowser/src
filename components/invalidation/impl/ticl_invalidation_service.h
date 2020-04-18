// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_TICL_INVALIDATION_SERVICE_H_
#define COMPONENTS_INVALIDATION_IMPL_TICL_INVALIDATION_SERVICE_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "components/invalidation/impl/invalidation_logger.h"
#include "components/invalidation/impl/invalidator_registrar.h"
#include "components/invalidation/impl/ticl_settings_provider.h"
#include "components/invalidation/public/invalidation_handler.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "google_apis/gaia/identity_provider.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/base/backoff_entry.h"

namespace gcm {
class GCMDriver;
}

namespace net {
class URLRequestContextGetter;
}

namespace syncer {
class InvalidationStateTracker;
class Invalidator;
}

namespace invalidation {
class GCMInvalidationBridge;

// This InvalidationService wraps the C++ Invalidation Client (TICL) library.
// It provides invalidations for desktop platforms (Win, Mac, Linux).
class TiclInvalidationService : public InvalidationService,
                                public OAuth2TokenService::Consumer,
                                public OAuth2TokenService::Observer,
                                public IdentityProvider::Observer,
                                public TiclSettingsProvider::Observer,
                                public syncer::InvalidationHandler {
 public:
  enum InvalidationNetworkChannel {
    PUSH_CLIENT_CHANNEL = 0,
    GCM_NETWORK_CHANNEL = 1,

    // This enum is used in UMA_HISTOGRAM_ENUMERATION. Insert new values above
    // this line.
    NETWORK_CHANNELS_COUNT = 2
  };

  TiclInvalidationService(
      const std::string& user_agent,
      std::unique_ptr<IdentityProvider> identity_provider,
      std::unique_ptr<TiclSettingsProvider> settings_provider,
      gcm::GCMDriver* gcm_driver,
      const scoped_refptr<net::URLRequestContextGetter>& request_context);
  ~TiclInvalidationService() override;

  void Init(std::unique_ptr<syncer::InvalidationStateTracker>
                invalidation_state_tracker);

  // InvalidationService implementation.
  // It is an error to have registered handlers when the service is destroyed.
  void RegisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  bool UpdateRegisteredInvalidationIds(syncer::InvalidationHandler* handler,
                                       const syncer::ObjectIdSet& ids) override;
  void UnregisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  syncer::InvalidatorState GetInvalidatorState() const override;
  std::string GetInvalidatorClientId() const override;
  InvalidationLogger* GetInvalidationLogger() override;
  void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> caller) const override;

  void RequestAccessToken();

  // OAuth2TokenService::Consumer implementation
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // OAuth2TokenService::Observer implementation
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokenRevoked(const std::string& account_id) override;

  // IdentityProvider::Observer implementation.
  void OnActiveAccountLogin() override;
  void OnActiveAccountLogout() override;

  // TiclSettingsProvider::Observer implementation.
  void OnUseGCMChannelChanged() override;

  // syncer::InvalidationHandler implementation.
  void OnInvalidatorStateChange(syncer::InvalidatorState state) override;
  void OnIncomingInvalidation(
      const syncer::ObjectIdInvalidationMap& invalidation_map) override;
  std::string GetOwnerName() const override;

 protected:
  // Initializes with an injected invalidator.
  void InitForTest(std::unique_ptr<syncer::InvalidationStateTracker>
                       invalidation_state_tracker,
                   syncer::Invalidator* invalidator);

 private:
  friend class TiclInvalidationServiceTestDelegate;
  friend class TiclProfileSettingsProviderTest;

  bool IsReadyToStart();
  bool IsStarted() const;

  void StartInvalidator(InvalidationNetworkChannel network_channel);
  void UpdateInvalidationNetworkChannel();
  void UpdateInvalidatorCredentials();
  void StopInvalidator();

  const std::string user_agent_;

  std::unique_ptr<IdentityProvider> identity_provider_;
  std::unique_ptr<TiclSettingsProvider> settings_provider_;

  std::unique_ptr<syncer::InvalidatorRegistrar> invalidator_registrar_;
  std::unique_ptr<syncer::InvalidationStateTracker> invalidation_state_tracker_;
  std::unique_ptr<syncer::Invalidator> invalidator_;

  // TiclInvalidationService needs to remember access token in order to
  // invalidate it with OAuth2TokenService.
  std::string access_token_;

  // TiclInvalidationService needs to hold reference to access_token_request_
  // for the duration of request in order to receive callbacks.
  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;
  base::OneShotTimer request_access_token_retry_timer_;
  net::BackoffEntry request_access_token_backoff_;

  InvalidationNetworkChannel network_channel_type_;
  gcm::GCMDriver* gcm_driver_;
  std::unique_ptr<GCMInvalidationBridge> gcm_invalidation_bridge_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // The invalidation logger object we use to record state changes
  // and invalidations.
  InvalidationLogger logger_;

  // Keep a copy of the important parameters used in network channel creation
  // for debugging.
  base::DictionaryValue network_channel_options_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TiclInvalidationService);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_TICL_INVALIDATION_SERVICE_H_
