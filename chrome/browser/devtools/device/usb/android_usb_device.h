// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVICE_USB_ANDROID_USB_DEVICE_H_
#define CHROME_BROWSER_DEVTOOLS_DEVICE_USB_ANDROID_USB_DEVICE_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <vector>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "device/usb/usb_device_handle.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace crypto {
class RSAPrivateKey;
}

namespace net {
class StreamSocket;
}

class AndroidUsbSocket;

class AdbMessage {
 public:
  enum Command {
    kCommandSYNC = 0x434e5953,
    kCommandCNXN = 0x4e584e43,
    kCommandOPEN = 0x4e45504f,
    kCommandOKAY = 0x59414b4f,
    kCommandCLSE = 0x45534c43,
    kCommandWRTE = 0x45545257,
    kCommandAUTH = 0x48545541
  };

  enum Auth {
    kAuthToken = 1,
    kAuthSignature = 2,
    kAuthRSAPublicKey = 3
  };

  AdbMessage(uint32_t command,
             uint32_t arg0,
             uint32_t arg1,
             const std::string& body);
  ~AdbMessage();

  uint32_t command;
  uint32_t arg0;
  uint32_t arg1;
  std::string body;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdbMessage);
};

class AndroidUsbDevice;
typedef std::vector<scoped_refptr<AndroidUsbDevice> > AndroidUsbDevices;
typedef base::Callback<void(const AndroidUsbDevices&)>
    AndroidUsbDevicesCallback;

class AndroidUsbDevice : public base::RefCountedThreadSafe<AndroidUsbDevice> {
 public:
  static void CountDevices(const base::Callback<void(int)>& callback);
  static void Enumerate(crypto::RSAPrivateKey* rsa_key,
                        const AndroidUsbDevicesCallback& callback);

  AndroidUsbDevice(crypto::RSAPrivateKey* rsa_key,
                   scoped_refptr<device::UsbDeviceHandle> device,
                   const std::string& serial,
                   int inbound_address,
                   int outbound_address,
                   int zero_mask,
                   int interface_id);

  void InitOnCallerThread();

  net::StreamSocket* CreateSocket(const std::string& command);

  void Send(uint32_t command,
            uint32_t arg0,
            uint32_t arg1,
            const std::string& body);

  scoped_refptr<device::UsbDeviceHandle> usb_device() { return usb_handle_; }

  std::string serial() { return serial_; }

  bool is_connected() { return is_connected_; }

 private:
  friend class base::RefCountedThreadSafe<AndroidUsbDevice>;
  virtual ~AndroidUsbDevice();

  void Queue(std::unique_ptr<AdbMessage> message);
  void ProcessOutgoing();
  void OutgoingMessageSent(device::UsbTransferStatus status,
                           scoped_refptr<base::RefCountedBytes> buffer,
                           size_t result);

  void ReadHeader();
  void ParseHeader(device::UsbTransferStatus status,
                   scoped_refptr<base::RefCountedBytes> buffer,
                   size_t result);

  void ReadBody(std::unique_ptr<AdbMessage> message,
                uint32_t data_length,
                uint32_t data_check);
  void ParseBody(std::unique_ptr<AdbMessage> message,
                 uint32_t data_length,
                 uint32_t data_check,
                 device::UsbTransferStatus status,
                 scoped_refptr<base::RefCountedBytes> buffer,
                 size_t result);

  void HandleIncoming(std::unique_ptr<AdbMessage> message);

  void TransferError(device::UsbTransferStatus status);

  void TerminateIfReleased(scoped_refptr<device::UsbDeviceHandle> usb_handle);
  void Terminate();

  void SocketDeleted(uint32_t socket_id);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  std::unique_ptr<crypto::RSAPrivateKey> rsa_key_;

  // Device info
  scoped_refptr<device::UsbDeviceHandle> usb_handle_;
  std::string serial_;
  int inbound_address_;
  int outbound_address_;
  int zero_mask_;
  int interface_id_;

  bool is_connected_;
  bool signature_sent_;

  // Created sockets info
  uint32_t last_socket_id_;
  using AndroidUsbSockets = std::map<uint32_t, AndroidUsbSocket*>;
  AndroidUsbSockets sockets_;

  // Outgoing bulk queue
  using BulkMessage = scoped_refptr<base::RefCountedBytes>;
  base::queue<BulkMessage> outgoing_queue_;

  // Outgoing messages pending connect
  using PendingMessages = std::vector<std::unique_ptr<AdbMessage>>;
  PendingMessages pending_messages_;

  base::WeakPtrFactory<AndroidUsbDevice> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AndroidUsbDevice);
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVICE_USB_ANDROID_USB_DEVICE_H_
