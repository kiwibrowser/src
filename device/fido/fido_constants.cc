// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_constants.h"

namespace device {

const std::array<uint8_t, 32> kBogusAppParam = {
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41};

const std::array<uint8_t, 32> kBogusChallenge = {
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42};

const char kResidentKeyMapKey[] = "rk";
const char kUserVerificationMapKey[] = "uv";
const char kUserPresenceMapKey[] = "up";
const char kClientPinMapKey[] = "client_pin";
const char kPlatformDeviceMapKey[] = "plat";

const size_t kHidPacketSize = 64;
const uint32_t kHidBroadcastChannel = 0xffffffff;
const size_t kHidInitPacketHeaderSize = 7;
const size_t kHidContinuationPacketHeader = 5;
const size_t kHidMaxPacketSize = 64;
const size_t kHidInitPacketDataSize =
    kHidMaxPacketSize - kHidInitPacketHeaderSize;
const size_t kHidContinuationPacketDataSize =
    kHidMaxPacketSize - kHidContinuationPacketHeader;
const uint8_t kHidMaxLockSeconds = 10;
const size_t kHidMaxMessageSize = 7609;

const size_t kU2fMaxResponseSize = 65536;
const uint8_t kP1TupRequired = 0x01;
const uint8_t kP1TupConsumed = 0x02;
const uint8_t kP1TupRequiredConsumed = kP1TupRequired | kP1TupConsumed;
const uint8_t kP1CheckOnly = 0x07;
const uint8_t kP1IndividualAttestation = 0x80;
const size_t kMaxKeyHandleLength = 255;
const size_t kU2fParameterLength = 32;

const base::TimeDelta kDeviceTimeout = base::TimeDelta::FromSeconds(3);
const base::TimeDelta kHidKeepAliveDelay =
    base::TimeDelta::FromMilliseconds(100);

const char kFormatKey[] = "fmt";
const char kAttestationStatementKey[] = "attStmt";
const char kAuthDataKey[] = "authData";
const char kNoneAttestationValue[] = "none";

const char kPublicKey[] = "public-key";

const char* CredentialTypeToString(CredentialType type) {
  switch (type) {
    case CredentialType::kPublicKey:
      return kPublicKey;
  }
  NOTREACHED();
  return kPublicKey;
}

}  // namespace device
