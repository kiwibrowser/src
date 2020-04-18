// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_CLIENT_CONNECTION_PARAMETERS_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_CLIENT_CONNECTION_PARAMETERS_H_

#include <ostream>
#include <string>

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "chromeos/services/secure_channel/public/mojom/secure_channel.mojom.h"

namespace chromeos {

namespace secure_channel {

// Parameters associated with a client request, which should be tightly-coupled
// to the associated communication channel.
class ClientConnectionParameters {
 public:
  ClientConnectionParameters(
      const std::string& feature,
      mojom::ConnectionDelegatePtr connection_delegate_ptr);
  ClientConnectionParameters(ClientConnectionParameters&& other);
  ClientConnectionParameters& operator=(ClientConnectionParameters&& other);
  virtual ~ClientConnectionParameters();

  const base::UnguessableToken& id() const { return id_; }

  const std::string& feature() const { return feature_; }

  mojom::ConnectionDelegatePtr& connection_delegate_ptr() {
    return connection_delegate_ptr_;
  }

  bool operator==(const ClientConnectionParameters& other) const;
  bool operator<(const ClientConnectionParameters& other) const;

 private:
  std::string feature_;
  mojom::ConnectionDelegatePtr connection_delegate_ptr_;
  base::UnguessableToken id_;

  DISALLOW_COPY_AND_ASSIGN(ClientConnectionParameters);
};

std::ostream& operator<<(std::ostream& stream,
                         const ClientConnectionParameters& details);

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_CLIENT_CONNECTION_PARAMETERS_H_
