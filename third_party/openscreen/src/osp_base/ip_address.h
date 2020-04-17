// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_BASE_IP_ADDRESS_H_
#define OSP_BASE_IP_ADDRESS_H_

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>

#include "osp_base/error.h"

namespace openscreen {

class IPAddress {
 public:
  enum class Version {
    kV4,
    kV6,
  };

  static constexpr size_t kV4Size = 4;
  static constexpr size_t kV6Size = 16;

  // Parses a text representation of an IPv4 address (e.g. "192.168.0.1") or an
  // IPv6 address (e.g. "abcd::1234") and puts the result into |address|.
  static ErrorOr<IPAddress> Parse(const std::string& s);

  IPAddress();
  explicit IPAddress(const std::array<uint8_t, 4>& bytes);
  explicit IPAddress(const uint8_t (&b)[4]);
  explicit IPAddress(Version version, const uint8_t* b);
  IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

  explicit IPAddress(const std::array<uint8_t, 16>& bytes);
  explicit IPAddress(const uint8_t (&b)[16]);
  IPAddress(uint8_t b1,
            uint8_t b2,
            uint8_t b3,
            uint8_t b4,
            uint8_t b5,
            uint8_t b6,
            uint8_t b7,
            uint8_t b8,
            uint8_t b9,
            uint8_t b10,
            uint8_t b11,
            uint8_t b12,
            uint8_t b13,
            uint8_t b14,
            uint8_t b15,
            uint8_t b16);
  IPAddress(const IPAddress& o) noexcept;
  ~IPAddress() = default;

  IPAddress& operator=(const IPAddress& o) noexcept;

  bool operator==(const IPAddress& o) const;
  bool operator!=(const IPAddress& o) const;
  explicit operator bool() const;

  Version version() const { return version_; }
  bool IsV4() const { return version_ == Version::kV4; }
  bool IsV6() const { return version_ == Version::kV6; }

  // These methods assume |x| is the appropriate size, but due to various
  // callers' casting needs we can't check them like the constructors above.
  // Callers should instead make any necessary checks themselves.
  void CopyToV4(uint8_t* x) const;
  void CopyToV6(uint8_t* x) const;

 private:
  static ErrorOr<IPAddress> ParseV4(const std::string& s);
  static ErrorOr<IPAddress> ParseV6(const std::string& s);

  friend class IPEndpointComparator;

  Version version_;
  std::array<uint8_t, 16> bytes_;
};

struct IPEndpoint {
 public:
  IPAddress address;
  uint16_t port;
};

bool operator==(const IPEndpoint& a, const IPEndpoint& b);
bool operator!=(const IPEndpoint& a, const IPEndpoint& b);

class IPEndpointComparator {
 public:
  bool operator()(const IPEndpoint& a, const IPEndpoint& b) const;
};

// Outputs a string of the form:
//      123.234.34.56
//   or fe80:0000:0000:0000:1234:5678:9abc:def0
std::ostream& operator<<(std::ostream& out, const IPAddress& address);

// Outputs a string of the form:
//      123.234.34.56:443
//   or [fe80:0000:0000:0000:1234:5678:9abc:def0]:8080
std::ostream& operator<<(std::ostream& out, const IPEndpoint& endpoint);

}  // namespace openscreen

#endif  // OSP_BASE_IP_ADDRESS_H_
