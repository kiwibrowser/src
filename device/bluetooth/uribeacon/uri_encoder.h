// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_URIBEACON_URI_ENCODER_H_
#define DEVICE_BLUETOOTH_URIBEACON_URI_ENCODER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/strings/string_piece.h"

namespace device {

// The following functions EncodeUriBeaconUri() and DecodeUriBeaconUri() helps
// encoding/decoding URI with the UriBeacon encoding.
//
// Example usage:
//
//   std::vector<uint8_t> encoded;
//   EncodeUriBeaconUri("http://web.mit.edu/", encoded)
//   // encoded -> {2, 'w', 'e', 'b', '.', 'm', 'i', 't', 2}
//
//   const char encodedUri[] = {0, 'u', 'r', 'i', 'b', 'e', 'a', 'c', 'o', 'n',
//       8};
//   const std::vector<uint8_t> kEncodedUri(encodedUri, encodedUri +
//   sizeof(encodedUri));
//   std::string decoded;
//   DecodeUriBeaconUri(kEncodedUri, decoded)
//   // decoded -> "http://uribeacon.org"

// Encodes the input string using URI encoding described in UriBeacon
// specifications. |input| must be ASCII characters.
void EncodeUriBeaconUri(const std::string& input, std::vector<uint8_t>& output);

// Decodes the input string using URI encoding described in UriBeacon
// specifications.
void DecodeUriBeaconUri(const std::vector<uint8_t>& input, std::string& output);

}  // namespace device

#endif
