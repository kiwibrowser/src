// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_CABLE_DEVICE_H_
#define DEVICE_FIDO_FIDO_CABLE_DEVICE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "crypto/aead.h"
#include "device/fido/fido_ble_device.h"

namespace device {

class FidoBleFrame;
class FidoBleConnection;

class COMPONENT_EXPORT(DEVICE_FIDO) FidoCableDevice : public FidoBleDevice {
 public:
  // Encapsulates state FidoCableDevice maintains to encrypt and decrypt
  // data within FidoBleFrame.
  struct COMPONENT_EXPORT(DEVICE_FIDO) EncryptionData {
    EncryptionData() = delete;
    EncryptionData(std::string session_key,
                   const std::array<uint8_t, 8>& nonce);
    EncryptionData(EncryptionData&& data);
    EncryptionData& operator=(EncryptionData&& other);
    ~EncryptionData();

    std::string encryption_key;
    std::array<uint8_t, 8> nonce;
    crypto::Aead aes_key{crypto::Aead::AES_256_GCM};
    uint32_t write_sequence_num = 0;
    uint32_t read_sequence_num = 0;

    DISALLOW_COPY_AND_ASSIGN(EncryptionData);
  };

  using FrameCallback = FidoBleTransaction::FrameCallback;

  FidoCableDevice(std::string address,
                  std::string session_key,
                  const std::array<uint8_t, 8>& nonce);
  // Constructor used for testing purposes.
  FidoCableDevice(std::unique_ptr<FidoBleConnection> connection,
                  std::string session_key,
                  const std::array<uint8_t, 8>& nonce);
  ~FidoCableDevice() override;

  // FidoBleDevice:
  void DeviceTransact(std::vector<uint8_t> command,
                      DeviceCallback callback) override;
  void OnResponseFrame(FrameCallback callback,
                       base::Optional<FidoBleFrame> frame) override;
  base::WeakPtr<FidoDevice> GetWeakPtr() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(FidoCableDeviceTest,
                           TestCableDeviceSendMultipleRequests);
  FRIEND_TEST_ALL_PREFIXES(FidoCableDeviceTest,
                           TestCableDeviceErrorOnMaxCounter);

  EncryptionData encryption_data_;
  base::WeakPtrFactory<FidoCableDevice> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FidoCableDevice);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_CABLE_DEVICE_H_
