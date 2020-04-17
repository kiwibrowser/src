// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/ip_address.h"

#include <cstring>
#include <iomanip>

#include "absl/types/optional.h"
#include "platform/api/logging.h"

namespace openscreen {

// static
ErrorOr<IPAddress> IPAddress::Parse(const std::string& s) {
  ErrorOr<IPAddress> v4 = ParseV4(s);

  return v4 ? std::move(v4) : ParseV6(s);
}  // namespace openscreen

IPAddress::IPAddress() : version_(Version::kV4), bytes_({}) {}
IPAddress::IPAddress(const std::array<uint8_t, 4>& bytes)
    : version_(Version::kV4),
      bytes_{{bytes[0], bytes[1], bytes[2], bytes[3]}} {}
IPAddress::IPAddress(const uint8_t (&b)[4])
    : version_(Version::kV4), bytes_{{b[0], b[1], b[2], b[3]}} {}
IPAddress::IPAddress(Version version, const uint8_t* b) : version_(version) {
  if (version_ == Version::kV4) {
    bytes_ = {{b[0], b[1], b[2], b[3]}};
  } else {
    bytes_ = {{b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9],
               b[10], b[11], b[12], b[13], b[14], b[15]}};
  }
}
IPAddress::IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
    : version_(Version::kV4), bytes_{{b1, b2, b3, b4}} {}
IPAddress::IPAddress(const std::array<uint8_t, 16>& bytes)
    : version_(Version::kV6), bytes_(bytes) {}
IPAddress::IPAddress(const uint8_t (&b)[16])
    : version_(Version::kV6),
      bytes_{{b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10],
              b[11], b[12], b[13], b[14], b[15]}} {}
IPAddress::IPAddress(uint8_t b1,
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
                     uint8_t b16)
    : version_(Version::kV6),
      bytes_{{b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15,
              b16}} {}
IPAddress::IPAddress(const IPAddress& o) noexcept = default;

IPAddress& IPAddress::operator=(const IPAddress& o) noexcept = default;

bool IPAddress::operator==(const IPAddress& o) const {
  if (version_ != o.version_)
    return false;

  if (version_ == Version::kV4) {
    return bytes_[0] == o.bytes_[0] && bytes_[1] == o.bytes_[1] &&
           bytes_[2] == o.bytes_[2] && bytes_[3] == o.bytes_[3];
  }
  return bytes_ == o.bytes_;
}

bool IPAddress::operator!=(const IPAddress& o) const {
  return !(*this == o);
}

IPAddress::operator bool() const {
  if (version_ == Version::kV4)
    return bytes_[0] | bytes_[1] | bytes_[2] | bytes_[3];

  for (const auto& byte : bytes_)
    if (byte)
      return true;

  return false;
}

void IPAddress::CopyToV4(uint8_t x[4]) const {
  OSP_DCHECK(version_ == Version::kV4);
  std::memcpy(x, bytes_.data(), 4);
}

void IPAddress::CopyToV6(uint8_t x[16]) const {
  OSP_DCHECK(version_ == Version::kV6);
  std::memcpy(x, bytes_.data(), 16);
}

// static
ErrorOr<IPAddress> IPAddress::ParseV4(const std::string& s) {
  if (s.size() > 0 && s[0] == '.')
    return Error::Code::kInvalidIPV4Address;

  IPAddress address;
  uint16_t next_octet = 0;
  int i = 0;
  bool previous_dot = false;
  for (auto c : s) {
    if (c == '.') {
      if (previous_dot) {
        return Error::Code::kInvalidIPV4Address;
      }
      address.bytes_[i++] = static_cast<uint8_t>(next_octet);
      next_octet = 0;
      previous_dot = true;
      if (i > 3)
        return Error::Code::kInvalidIPV4Address;

      continue;
    }
    previous_dot = false;
    if (!std::isdigit(c))
      return Error::Code::kInvalidIPV4Address;

    next_octet = next_octet * 10 + (c - '0');
    if (next_octet > 255)
      return Error::Code::kInvalidIPV4Address;
  }
  if (previous_dot)
    return Error::Code::kInvalidIPV4Address;

  if (i != 3)
    return Error::Code::kInvalidIPV4Address;

  address.bytes_[i] = static_cast<uint8_t>(next_octet);
  address.version_ = Version::kV4;
  return address;
}

// static
ErrorOr<IPAddress> IPAddress::ParseV6(const std::string& s) {
  if (s.size() > 1 && s[0] == ':' && s[1] != ':')
    return Error::Code::kInvalidIPV6Address;

  uint16_t next_value = 0;
  uint8_t values[16];
  int i = 0;
  int num_previous_colons = 0;
  absl::optional<int> double_colon_index = absl::nullopt;
  for (auto c : s) {
    if (c == ':') {
      ++num_previous_colons;
      if (num_previous_colons == 2) {
        if (double_colon_index) {
          return Error::Code::kInvalidIPV6Address;
        }
        double_colon_index = i;
      } else if (i >= 15 || num_previous_colons > 2) {
        return Error::Code::kInvalidIPV6Address;
      } else {
        values[i++] = static_cast<uint8_t>(next_value >> 8);
        values[i++] = static_cast<uint8_t>(next_value & 0xff);
        next_value = 0;
      }
    } else {
      num_previous_colons = 0;
      uint8_t x = 0;
      if (c >= '0' && c <= '9') {
        x = c - '0';
      } else if (c >= 'a' && c <= 'f') {
        x = c - 'a' + 10;
      } else if (c >= 'A' && c <= 'F') {
        x = c - 'A' + 10;
      } else {
        return Error::Code::kInvalidIPV6Address;
      }
      if (next_value & 0xf000) {
        return Error::Code::kInvalidIPV6Address;
      } else {
        next_value = static_cast<uint16_t>(next_value * 16 + x);
      }
    }
  }
  if (num_previous_colons == 1)
    return Error::Code::kInvalidIPV6Address;

  if (i >= 15)
    return Error::Code::kInvalidIPV6Address;

  values[i++] = static_cast<uint8_t>(next_value >> 8);
  values[i] = static_cast<uint8_t>(next_value & 0xff);
  if (!((i == 15 && !double_colon_index) || (i < 14 && double_colon_index))) {
    return Error::Code::kInvalidIPV6Address;
  }

  IPAddress address;
  for (int j = 15; j >= 0;) {
    if (double_colon_index && (i == double_colon_index)) {
      address.bytes_[j--] = values[i--];
      while (j > i)
        address.bytes_[j--] = 0;
    } else {
      address.bytes_[j--] = values[i--];
    }
  }
  address.version_ = Version::kV6;
  return address;
}

bool operator==(const IPEndpoint& a, const IPEndpoint& b) {
  return (a.address == b.address) && (a.port == b.port);
}

bool operator!=(const IPEndpoint& a, const IPEndpoint& b) {
  return !(a == b);
}

bool IPEndpointComparator::operator()(const IPEndpoint& a,
                                      const IPEndpoint& b) const {
  if (a.address.version() != b.address.version())
    return a.address.version() < b.address.version();
  if (a.address.IsV4()) {
    int ret = memcmp(a.address.bytes_.data(), b.address.bytes_.data(), 4);
    if (ret != 0)
      return ret < 0;
  } else {
    int ret = memcmp(a.address.bytes_.data(), b.address.bytes_.data(), 16);
    if (ret != 0)
      return ret < 0;
  }
  return a.port < b.port;
}

std::ostream& operator<<(std::ostream& out, const IPAddress& address) {
  uint8_t values[16];
  size_t len = 0;
  char separator;
  size_t values_per_separator;
  if (address.IsV4()) {
    out << std::dec;
    address.CopyToV4(values);
    len = 4;
    separator = '.';
    values_per_separator = 1;
  } else if (address.IsV6()) {
    out << std::hex;
    address.CopyToV6(values);
    len = 16;
    separator = ':';
    values_per_separator = 2;
  }
  out << std::setfill('0') << std::right;
  for (size_t i = 0; i < len; ++i) {
    if (i > 0 && (i % values_per_separator == 0)) {
      out << separator;
    }
    out << std::setw(2) << static_cast<int>(values[i]);
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const IPEndpoint& endpoint) {
  if (endpoint.address.IsV6()) {
    out << '[';
  }
  out << endpoint.address;
  if (endpoint.address.IsV6()) {
    out << ']';
  }
  return out << ':' << std::dec << static_cast<int>(endpoint.port);
}

}  // namespace openscreen
