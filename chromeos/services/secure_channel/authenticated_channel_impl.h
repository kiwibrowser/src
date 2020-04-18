// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_AUTHENTICATED_CHANNEL_IMPL_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_AUTHENTICATED_CHANNEL_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/services/secure_channel/authenticated_channel.h"
#include "components/cryptauth/secure_channel.h"

namespace cryptauth {
class SecureChannel;
}  // namespace cryptauth

namespace chromeos {

namespace secure_channel {

// Concrete AuthenticatedChannel implementation, whose send/receive mechanisms
// are implemented via SecureChannel.
class AuthenticatedChannelImpl : public AuthenticatedChannel,
                                 public cryptauth::SecureChannel::Observer {
 public:
  class Factory {
   public:
    static Factory* Get();
    static void SetFactoryForTesting(Factory* test_factory);
    virtual std::unique_ptr<AuthenticatedChannel> BuildInstance(
        const std::vector<mojom::ConnectionCreationDetail>&
            connection_creation_details,
        std::unique_ptr<cryptauth::SecureChannel> secure_channel);

   private:
    static Factory* test_factory_;
  };

  ~AuthenticatedChannelImpl() override;

 private:
  AuthenticatedChannelImpl(
      const std::vector<mojom::ConnectionCreationDetail>&
          connection_creation_details,
      std::unique_ptr<cryptauth::SecureChannel> secure_channel);

  // AuthenticatedChannel:
  const mojom::ConnectionMetadata& GetConnectionMetadata() const override;
  void PerformSendMessage(const std::string& feature,
                          const std::string& payload,
                          base::OnceClosure on_sent_callback) final;
  void PerformDisconnection() override;

  // cryptauth::SecureChannel::Observer:
  void OnSecureChannelStatusChanged(
      cryptauth::SecureChannel* secure_channel,
      const cryptauth::SecureChannel::Status& old_status,
      const cryptauth::SecureChannel::Status& new_status) override;
  void OnMessageReceived(cryptauth::SecureChannel* secure_channel,
                         const std::string& feature,
                         const std::string& payload) override;
  void OnMessageSent(cryptauth::SecureChannel* secure_channel,
                     int sequence_number) override;

  mojom::ConnectionMetadata connection_metadata_;
  std::unique_ptr<cryptauth::SecureChannel> secure_channel_;
  std::unordered_map<int, base::OnceClosure> sequence_number_to_callback_map_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatedChannelImpl);
};

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_AUTHENTICATED_CHANNEL_IMPL_H_
