// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_GCM_INVALIDATION_BRIDGE_H_
#define COMPONENTS_INVALIDATION_IMPL_GCM_INVALIDATION_BRIDGE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/gcm_driver/common/gcm_messages.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/gcm_driver/gcm_connection_observer.h"
#include "components/invalidation/impl/gcm_network_channel_delegate.h"
#include "google_apis/gaia/oauth2_token_service.h"

class IdentityProvider;

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace gcm {
class GCMDriver;
}  // namespace gcm

namespace invalidation {

// GCMInvalidationBridge and GCMInvalidationBridge::Core implement functions
// needed for GCMNetworkChannel. GCMInvalidationBridge lives on UI thread while
// Core lives on IO thread. Core implements GCMNetworkChannelDelegate and posts
// all function calls to GCMInvalidationBridge which does actual work to perform
// them.
class GCMInvalidationBridge : public gcm::GCMAppHandler,
                              public gcm::GCMConnectionObserver,
                              public OAuth2TokenService::Consumer {
 public:
  class Core;

  GCMInvalidationBridge(gcm::GCMDriver* gcm_driver,
                        IdentityProvider* identity_provider);
  ~GCMInvalidationBridge() override;

  // OAuth2TokenService::Consumer implementation.
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // gcm::GCMAppHandler implementation.
  void ShutdownHandler() override;
  void OnStoreReset() override;
  void OnMessage(const std::string& app_id,
                 const gcm::IncomingMessage& message) override;
  void OnMessagesDeleted(const std::string& app_id) override;
  void OnSendError(
      const std::string& app_id,
      const gcm::GCMClient::SendErrorDetails& send_error_details) override;
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override;

  // gcm::GCMConnectionObserver implementation.
  void OnConnected(const net::IPEndPoint& ip_endpoint) override;
  void OnDisconnected() override;

  std::unique_ptr<syncer::GCMNetworkChannelDelegate> CreateDelegate();

  void CoreInitializationDone(
      base::WeakPtr<Core> core,
      scoped_refptr<base::SingleThreadTaskRunner> core_thread_task_runner);

  // Functions reflecting GCMNetworkChannelDelegate interface. These are called
  // on UI thread to perform actual work.
  void RequestToken(
      syncer::GCMNetworkChannelDelegate::RequestTokenCallback callback);
  void InvalidateToken(const std::string& token);

  void Register(syncer::GCMNetworkChannelDelegate::RegisterCallback callback);

  void SubscribeForIncomingMessages();

  void RegisterFinished(
      syncer::GCMNetworkChannelDelegate::RegisterCallback callback,
      const std::string& registration_id,
      gcm::GCMClient::Result result);

  void Unregister();

 private:
  gcm::GCMDriver* const gcm_driver_;
  IdentityProvider* const identity_provider_;

  base::WeakPtr<Core> core_;
  scoped_refptr<base::SingleThreadTaskRunner> core_thread_task_runner_;

  // Fields related to RequestToken function.
  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;
  syncer::GCMNetworkChannelDelegate::RequestTokenCallback
      request_token_callback_;
  bool subscribed_for_incoming_messages_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<GCMInvalidationBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GCMInvalidationBridge);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_GCM_INVALIDATION_BRIDGE_H_
