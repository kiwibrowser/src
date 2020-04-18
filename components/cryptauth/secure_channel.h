// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_SECURE_CHANNEL_H_
#define COMPONENTS_CRYPTAUTH_SECURE_CHANNEL_H_

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/authenticator.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/connection_observer.h"
#include "components/cryptauth/device_to_device_authenticator.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/secure_context.h"

namespace cryptauth {

class CryptAuthService;

// An authenticated bi-directional channel for exchanging messages with remote
// devices. |SecureChannel| manages a |Connection| by initializing it and
// authenticating it via a security handshake once the connection has occurred.
// Once the channel has been authenticated, messages sent are automatically
// encrypted and messages received are automatically decrypted.
class SecureChannel : public ConnectionObserver {
 public:
  // Enumeration of possible states of connecting to a remote device.
  //   DISCONNECTED: There is no connection to the device, nor is there a
  //       pending connection attempt.
  //   CONNECTING: There is an ongoing connection attempt.
  //   CONNECTED: There is a Bluetooth connection to the device, but the
  //       connection has not yet been authenticated.
  //   AUTHENTICATING: There is an active connection that is currently in the
  //       process of authenticating via a 3-message authentication handshake.
  //   AUTHENTICATED: The connection has been authenticated, and arbitrary
  //       messages can be sent/received to/from the device.
  //   DISCONNECTING: The connection has started disconnecting but has not yet
  //       finished.
  enum class Status {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AUTHENTICATING,
    AUTHENTICATED,
    DISCONNECTING
  };

  static std::string StatusToString(const Status& status);

  class Observer {
   public:
    virtual void OnSecureChannelStatusChanged(
        SecureChannel* secure_channel,
        const Status& old_status,
        const Status& new_status) = 0;

    virtual void OnMessageReceived(
        SecureChannel* secure_channel,
        const std::string& feature,
        const std::string& payload) = 0;

    // Called when a message has been sent successfully; |sequence_number|
    // corresponds to the value returned by an earlier call to SendMessage().
    virtual void OnMessageSent(SecureChannel* secure_channel,
                               int sequence_number) {}

    // Called when GATT characteristics are not available. This observer
    // function is a temporary work-around (see crbug.com/784968).
    // TODO(khorimoto): This observer function is specific to only one
    //     SecureChannel implementation, so it is hacky to include it as part of
    //     the observer for SecureChannel. Remove this work-around when it is no
    //     longer necessary.
    virtual void OnGattCharacteristicsNotAvailable() {}
  };

  class Factory {
   public:
    static std::unique_ptr<SecureChannel> NewInstance(
        std::unique_ptr<Connection> connection,
        CryptAuthService* cryptauth_service);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<SecureChannel> BuildInstance(
        std::unique_ptr<Connection> connection,
        CryptAuthService* cryptauth_service);

   private:
    static Factory* factory_instance_;
  };

  ~SecureChannel() override;

  virtual void Initialize();

  // Sends a message over the connection and returns a sequence number. If the
  // message is successfully sent, observers will be notified that the message
  // has been sent and will be provided this sequence number.
  virtual int SendMessage(const std::string& feature,
                          const std::string& payload);

  virtual void Disconnect();

  virtual void AddObserver(Observer* observer);
  virtual void RemoveObserver(Observer* observer);

  Status status() const {
    return status_;
  }

  // ConnectionObserver:
  void OnConnectionStatusChanged(Connection* connection,
                                 Connection::Status old_status,
                                 Connection::Status new_status) override;
  void OnMessageReceived(const Connection& connection,
                         const WireMessage& wire_message) override;
  void OnSendCompleted(const cryptauth::Connection& connection,
                       const cryptauth::WireMessage& wire_message,
                       bool success) override;
  void OnGattCharacteristicsNotAvailable() override;

 protected:
  SecureChannel(std::unique_ptr<Connection> connection,
                CryptAuthService* cryptauth_service);

  void NotifyGattCharacteristicsNotAvailable();

  Status status_;

 private:
  friend class CryptAuthSecureChannelTest;

  // Message waiting to be sent. Note that this is *not* the message that will
  // end up being sent over the wire; before that can be done, the payload must
  // be encrypted.
  struct PendingMessage {
    PendingMessage(const std::string& feature,
                   const std::string& payload,
                   int sequence_number);
    virtual ~PendingMessage();

    const std::string feature;
    const std::string payload;
    const int sequence_number;
  };

  void TransitionToStatus(const Status& new_status);
  void Authenticate();
  void ProcessMessageQueue();
  void OnMessageEncoded(const std::string& feature,
                        int sequence_number,
                        const std::string& encoded_message);
  void OnMessageDecoded(
      const std::string& feature, const std::string& decoded_message);
  void OnAuthenticationResult(
      Authenticator::Result result,
      std::unique_ptr<SecureContext> secure_context);

  std::unique_ptr<Connection> connection_;
  CryptAuthService* cryptauth_service_;  // Outlives this instance.
  std::unique_ptr<Authenticator> authenticator_;
  std::unique_ptr<SecureContext> secure_context_;
  base::queue<std::unique_ptr<PendingMessage>> queued_messages_;
  std::unique_ptr<PendingMessage> pending_message_;
  int next_sequence_number_ = 0;
  base::ObserverList<Observer> observer_list_;
  base::WeakPtrFactory<SecureChannel> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SecureChannel);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_SECURE_CHANNEL_H_
