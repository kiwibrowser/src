// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_CONNECTION_OBSERVER_H_
#define COMPONENTS_CRYPTAUTH_CONNECTION_OBSERVER_H_

#include "components/cryptauth/connection.h"

namespace cryptauth {

class WireMessage;

// An interface for observing events that happen on a Connection.
class ConnectionObserver {
 public:
  virtual ~ConnectionObserver() {}

  // Called when the |connection|'s status changes from |old_status| to
  // |new_status|. The |connectoin| is guaranteed to be non-null.
  virtual void OnConnectionStatusChanged(Connection* connection,
                                         Connection::Status old_status,
                                         Connection::Status new_status) {}

  // Called when a |message| is received from a remote device over the
  // |connection|.
  virtual void OnMessageReceived(const Connection& connection,
                                 const WireMessage& message) {}

  // Called after a |message| is sent to the remote device over the
  // |connection|. |success| is |true| iff the message is sent successfully.
  virtual void OnSendCompleted(const Connection& connection,
                               const WireMessage& message,
                               bool success) {}

  // Called when GATT characteristics are not available. This observer function
  // is a temporary work-around (see crbug.com/784968).
  // TODO(khorimoto): This observer function is specific to only one Connection
  //     implementation, so it is hacky to include it as part of the observer
  //     for Connection. Remove this work-around when it is no longer necessary.
  virtual void OnGattCharacteristicsNotAvailable() {}
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_CONNECTION_OBSERVER_H_
