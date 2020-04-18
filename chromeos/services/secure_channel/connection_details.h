// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_DETAILS_H_
#define CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_DETAILS_H_

#include <functional>
#include <ostream>
#include <string>
#include <type_traits>

#include "base/hash.h"
#include "chromeos/services/secure_channel/connection_medium.h"

namespace chromeos {

namespace secure_channel {

// Fields describing a connection. At any given time, at most one connection
// with a given set of ConnectionDetails should exist.
class ConnectionDetails {
 public:
  ConnectionDetails(const std::string& device_id,
                    ConnectionMedium connection_medium);
  ~ConnectionDetails();

  const std::string& device_id() const { return device_id_; }
  ConnectionMedium connection_medium() const { return connection_medium_; }

  bool operator==(const ConnectionDetails& other) const;
  bool operator<(const ConnectionDetails& other) const;

 private:
  friend struct ConnectionDetailsHash;

  std::string device_id_;
  ConnectionMedium connection_medium_;
};

// For use in std::unordered_map.
struct ConnectionDetailsHash {
  size_t operator()(const ConnectionDetails& details) const {
    static std::hash<std::string> string_hash;
    static std::hash<std::underlying_type<ConnectionMedium>::type> medium_hash;
    return base::HashInts64(
        string_hash(details.device_id_),
        medium_hash(static_cast<std::underlying_type<ConnectionMedium>::type>(
            details.connection_medium_)));
  }
};

std::ostream& operator<<(std::ostream& stream,
                         const ConnectionDetails& details);

}  // namespace secure_channel

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_SECURE_CHANNEL_CONNECTION_DETAILS_H_
