// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_GCM_NETWORK_CHANNEL_DELEGATE_H_
#define COMPONENTS_INVALIDATION_IMPL_GCM_NETWORK_CHANNEL_DELEGATE_H_

#include <string>

#include "base/callback.h"
#include "components/gcm_driver/gcm_client.h"

class GoogleServiceAuthError;

namespace syncer {

// Delegate for GCMNetworkChannel.
// GCMNetworkChannel needs Register to register with GCM client and obtain gcm
// registration id. This id is used for building URL to cache invalidation
// endpoint.
// It needs RequestToken and InvalidateToken to get access token to include it
// in HTTP message to server.
// GCMNetworkChannel lives on IO thread therefore calls will be made on IO
// thread and callbacks should be invoked there as well.
class GCMNetworkChannelDelegate {
 public:
  typedef base::Callback<void(const GoogleServiceAuthError& error,
                              const std::string& token)> RequestTokenCallback;
  typedef base::Callback<void(const std::string& registration_id,
                              gcm::GCMClient::Result result)> RegisterCallback;
  typedef base::Callback<void(const std::string& message,
                              const std::string& echo_token)> MessageCallback;
  typedef base::Callback<void(bool online)> ConnectionStateCallback;

  virtual ~GCMNetworkChannelDelegate() {}

  virtual void Initialize(ConnectionStateCallback connection_state_callback,
                          base::Closure store_reset_callback) = 0;
  // Request access token. Callback should be called either with access token or
  // error code.
  virtual void RequestToken(RequestTokenCallback callback) = 0;
  // Invalidate access token that was rejected by server.
  virtual void InvalidateToken(const std::string& token) = 0;

  // Request registration_id from GCMService. Callback should be called with
  // either registration id or error code.
  virtual void Register(RegisterCallback callback) = 0;
  // Provide callback for incoming messages from GCM.
  virtual void SetMessageReceiver(MessageCallback callback) = 0;
};
}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_GCM_NETWORK_CHANNEL_DELEGATE_H_
