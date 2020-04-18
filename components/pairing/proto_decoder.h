// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_PROTO_DECODER_H_
#define COMPONENTS_PAIRING_PROTO_DECODER_H_

#include <stdint.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/pairing/message_buffer.h"

namespace net {
class IOBuffer;
}

namespace pairing_api {
class AddNetwork;
class CompleteSetup;
class ConfigureHost;
class Error;
class HostStatus;
class PairDevices;
class Reboot;
}  // namespace pairing_api

namespace pairing_chromeos {

// A ProtoDecoder collects data from a series of IOBuffers and decodes Proto
// buffers from the data.  The decoded messages are then forwarded to an
// observer.
class ProtoDecoder {
 public:
  typedef scoped_refptr<net::IOBuffer> IOBufferRefPtr;
  class Observer {
   public:
    virtual ~Observer() {}

    virtual void OnHostStatusMessage(
        const pairing_api::HostStatus& message) = 0;
    virtual void OnConfigureHostMessage(
        const pairing_api::ConfigureHost& message) = 0;
    virtual void OnPairDevicesMessage(
        const pairing_api::PairDevices& message) = 0;
    virtual void OnCompleteSetupMessage(
        const pairing_api::CompleteSetup& message) = 0;
    virtual void OnErrorMessage(
        const pairing_api::Error& message) = 0;
    virtual void OnAddNetworkMessage(
        const pairing_api::AddNetwork& message) = 0;
    virtual void OnRebootMessage(const pairing_api::Reboot& message) = 0;

   protected:
    Observer() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  explicit ProtoDecoder(Observer* observer);
  ~ProtoDecoder();

  // Decodes the data from an io_buffer, and sends out events for any complete
  // messages.
  bool DecodeIOBuffer(int size, IOBufferRefPtr io_buffer);

  // Convenience functions for serializing messages into an IOBuffer.
  static IOBufferRefPtr SendHostStatus(const pairing_api::HostStatus& message,
                                       int* size);
  static IOBufferRefPtr SendHostNetwork(const pairing_api::AddNetwork& message,
                                        int* size);
  static IOBufferRefPtr SendConfigureHost(
      const pairing_api::ConfigureHost& message, int* size);
  static IOBufferRefPtr SendPairDevices(const pairing_api::PairDevices& message,
                                        int* size);
  static IOBufferRefPtr SendCompleteSetup(
      const pairing_api::CompleteSetup& message, int* size);
  static IOBufferRefPtr SendError(const pairing_api::Error& message, int* size);
  static IOBufferRefPtr SendRebootHost(const pairing_api::Reboot& message,
                                       int* size);

 private:
  static IOBufferRefPtr SendMessage(uint8_t message_type,
                                    const std::string& message,
                                    int* size);

  Observer* observer_;
  MessageBuffer message_buffer_;
  int next_message_type_;
  int next_message_size_;

  DISALLOW_COPY_AND_ASSIGN(ProtoDecoder);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_PROTO_DECODER_H_
