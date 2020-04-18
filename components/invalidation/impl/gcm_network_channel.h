// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_GCM_NETWORK_CHANNEL_H_
#define COMPONENTS_INVALIDATION_IMPL_GCM_NETWORK_CHANNEL_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "components/invalidation/impl/gcm_network_channel_delegate.h"
#include "components/invalidation/impl/sync_system_resources.h"
#include "components/invalidation/public/invalidation_export.h"
#include "net/base/backoff_entry.h"
#include "net/base/network_change_notifier.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

class GoogleServiceAuthError;

namespace syncer {
class GCMNetworkChannel;

// POD with copy of some statuses for debugging purposes.
struct GCMNetworkChannelDiagnostic {
  explicit GCMNetworkChannelDiagnostic(GCMNetworkChannel* parent);

  // Collect all the internal variables in a single readable dictionary.
  std::unique_ptr<base::DictionaryValue> CollectDebugData() const;

  // TODO(pavely): Move this toString to a more appropiate place in GCMClient.
  std::string GCMClientResultToString(
      const gcm::GCMClient::Result result) const;

  GCMNetworkChannel* parent_;
  bool last_message_empty_echo_token_;
  base::Time last_message_received_time_;
  int last_post_response_code_;
  std::string registration_id_;
  gcm::GCMClient::Result registration_result_;
  int sent_messages_count_;
};

// GCMNetworkChannel is an implementation of SyncNetworkChannel that routes
// messages through GCMService.
class INVALIDATION_EXPORT GCMNetworkChannel
    : public SyncNetworkChannel,
      public net::URLFetcherDelegate,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  GCMNetworkChannel(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      std::unique_ptr<GCMNetworkChannelDelegate> delegate);

  ~GCMNetworkChannel() override;

  // invalidation::NetworkChannel implementation.
  void SendMessage(const std::string& message) override;
  void SetMessageReceiver(
      invalidation::MessageCallback* incoming_receiver) override;

  // SyncNetworkChannel implementation.
  void UpdateCredentials(const std::string& email,
                         const std::string& token) override;
  int GetInvalidationClientType() override;
  void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> callback) override;

  // URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // NetworkChangeObserver implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType connection_type) override;

 protected:
  void ResetRegisterBackoffEntryForTest(
      const net::BackoffEntry::Policy* policy);

  virtual GURL BuildUrl(const std::string& registration_id);

 private:
  friend class GCMNetworkChannelTest;
  void Register();
  void OnRegisterComplete(const std::string& registration_id,
                          gcm::GCMClient::Result result);
  void RequestAccessToken();
  void OnGetTokenComplete(const GoogleServiceAuthError& error,
                          const std::string& token);
  void OnIncomingMessage(const std::string& message,
                         const std::string& echo_token);
  void OnConnectionStateChanged(bool online);
  void OnStoreReset();
  void UpdateGcmChannelState(bool online);
  void UpdateHttpChannelState(bool online);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  std::unique_ptr<GCMNetworkChannelDelegate> delegate_;

  // Message is saved until all conditions are met: there is valid
  // registration_id and access_token.
  std::string cached_message_;

  // Access token is saved because in case of auth failure from server we need
  // to invalidate it.
  std::string access_token_;

  // GCM registration_id is requested one at startup and never refreshed until
  // next restart.
  std::string registration_id_;
  std::unique_ptr<net::BackoffEntry> register_backoff_entry_;

  std::unique_ptr<net::URLFetcher> fetcher_;

  // cacheinvalidation client receives echo_token with incoming message from
  // GCM and shuld include it in headers with outgoing message over http.
  std::string echo_token_;

  // State of gcm and http channels. GCMNetworkChannel will only report
  // INVALIDATIONS_ENABLED if both channels are online.
  bool gcm_channel_online_;
  bool http_channel_online_;

  GCMNetworkChannelDiagnostic diagnostic_info_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<GCMNetworkChannel> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GCMNetworkChannel);
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_GCM_NETWORK_CHANNEL_H_
