// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_MESSENGER_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_MESSENGER_H_

namespace cryptauth {
class Connection;
class SecureContext;
}  // namespace cryptauth

namespace proximity_auth {

class MessengerObserver;

// A messenger handling the Easy Unlock protocol, capable of parsing events from
// the remote device and sending events for the local device.
class Messenger {
 public:
  virtual ~Messenger() {}

  // Adds or removes an observer for Messenger events.
  virtual void AddObserver(MessengerObserver* observer) = 0;
  virtual void RemoveObserver(MessengerObserver* observer) = 0;

  // Returns true iff the remote device supports the v3.1 sign-in protocol.
  virtual bool SupportsSignIn() const = 0;

  // Sends an unlock event message to the remote device.
  virtual void DispatchUnlockEvent() = 0;

  // Sends a serialized SecureMessage to the remote device to decrypt the
  // |challenge|. OnDecryptResponse will be called for each observer when the
  // decrypted response is received.
  // TODO(isherman): Add params for the RSA private key and crypto delegate.
  virtual void RequestDecryption(const std::string& challenge) = 0;

  // Sends a simple request to the remote device to unlock the screen.
  // OnUnlockResponse is called for each observer when the response is returned.
  virtual void RequestUnlock() = 0;

  // Returns the SecureContext instance used by the messenger. Ownership of the
  // SecureContext is not passed.
  virtual cryptauth::SecureContext* GetSecureContext() const = 0;

  // Returns the underlying raw connection. Note that you should use
  // |GetSecureContext()| instead if you want to send and receive messages
  // securely.
  virtual cryptauth::Connection* GetConnection() const = 0;
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_MESSENGER_H_
