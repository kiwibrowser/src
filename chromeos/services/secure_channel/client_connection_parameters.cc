// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/client_connection_parameters.h"

namespace chromeos {

namespace secure_channel {

ClientConnectionParameters::ClientConnectionParameters(
    const std::string& feature,
    mojom::ConnectionDelegatePtr connection_delegate_ptr)
    : feature_(feature),
      connection_delegate_ptr_(std::move(connection_delegate_ptr)),
      id_(base::UnguessableToken::Create()) {
  DCHECK(!feature_.empty());
  DCHECK(connection_delegate_ptr_);
}

ClientConnectionParameters::ClientConnectionParameters(
    ClientConnectionParameters&& other)
    : feature_(other.feature_),
      connection_delegate_ptr_(std::move(other.connection_delegate_ptr_)),
      id_(other.id_) {}

ClientConnectionParameters& ClientConnectionParameters::operator=(
    ClientConnectionParameters&& other) {
  feature_ = other.feature_;
  connection_delegate_ptr_ = std::move(other.connection_delegate_ptr_);
  id_ = other.id_;
  return *this;
}

ClientConnectionParameters::~ClientConnectionParameters() = default;

bool ClientConnectionParameters::operator==(
    const ClientConnectionParameters& other) const {
  return feature() == other.feature() && id() == other.id() &&
         connection_delegate_ptr_.get() == other.connection_delegate_ptr_.get();
}

bool ClientConnectionParameters::operator<(
    const ClientConnectionParameters& other) const {
  if (feature() != other.feature())
    return feature() < other.feature();

  if (id() != other.id())
    return id() < other.id();

  // Use pointer comparison of proxies.
  return connection_delegate_ptr_.get() < other.connection_delegate_ptr_.get();
}

std::ostream& operator<<(std::ostream& stream,
                         const ClientConnectionParameters& details) {
  stream << "{feature: \"" << details.feature() << "\", "
         << "id: \"" << details.id().ToString() << "\"}";
  return stream;
}

}  // namespace secure_channel

}  // namespace chromeos
