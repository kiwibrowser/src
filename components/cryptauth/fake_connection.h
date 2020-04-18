// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_CONNECTION_H_
#define COMPONENTS_CRYPTAUTH_FAKE_CONNECTION_H_

#include "base/macros.h"
#include "components/cryptauth/connection.h"

namespace cryptauth {

class ConnectionObserver;

// A fake implementation of Connection to use in tests.
class FakeConnection : public Connection {
 public:
  FakeConnection(RemoteDeviceRef remote_device);
  FakeConnection(RemoteDeviceRef remote_device, bool should_auto_connect);
  ~FakeConnection() override;

  // Connection:
  void Connect() override;
  void Disconnect() override;
  std::string GetDeviceAddress() override;
  void AddObserver(ConnectionObserver* observer) override;
  void RemoveObserver(ConnectionObserver* observer) override;

  // Completes a connection attempt which was originally started via a call to
  // |Connect()|. If |success| is true, the connection's status shifts to
  // |CONNECTED|; otherwise, the status shifts to |DISCONNECTED|. Note that this
  // function should only be called when |should_auto_connect| is false.
  void CompleteInProgressConnection(bool success);

  // Completes the current send operation with success |success|.
  void FinishSendingMessageWithSuccess(bool success);

  // Simulates receiving a wire message with the given |payload|, bypassing the
  // container WireMessage format.
  void ReceiveMessage(const std::string& feature, const std::string& payload);

  // Notifies observers that GATT characteristics are unavailable.
  void NotifyGattCharacteristicsNotAvailable();

  // Returns the current message in progress of being sent.
  WireMessage* current_message() { return current_message_.get(); }

  std::vector<ConnectionObserver*>& observers() {
    return observers_;
  }

  using Connection::SetStatus;

 private:
  // Connection:
  void SendMessageImpl(std::unique_ptr<WireMessage> message) override;
  std::unique_ptr<WireMessage> DeserializeWireMessage(
      bool* is_incomplete_message) override;

  // The message currently being sent. Only set between a call to
  // SendMessageImpl() and FinishSendingMessageWithSuccess().
  std::unique_ptr<WireMessage> current_message_;

  // The feature and payload that should be returned when
  // DeserializeWireMessage() is called.
  std::string pending_feature_;
  std::string pending_payload_;

  std::vector<ConnectionObserver*> observers_;

  const bool should_auto_connect_;

  DISALLOW_COPY_AND_ASSIGN(FakeConnection);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_CONNECTION_H_
