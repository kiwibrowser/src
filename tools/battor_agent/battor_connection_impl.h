// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_BATTOR_CONNECTION_IMPL_H_
#define TOOLS_BATTOR_AGENT_BATTOR_CONNECTION_IMPL_H_

#include <fstream>
#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/time/tick_clock.h"
#include "services/device/public/mojom/serial.mojom.h"
#include "tools/battor_agent/battor_connection.h"
#include "tools/battor_agent/battor_error.h"
#include "tools/battor_agent/battor_protocol_types.h"

namespace device {
class SerialIoHandler;
}
namespace net {
class IOBuffer;
}

namespace battor {

// A BattOrConnectionImpl is a concrete implementation of a BattOrConnection.
class BattOrConnectionImpl
    : public BattOrConnection,
      public base::SupportsWeakPtr<BattOrConnectionImpl> {
 public:
  BattOrConnectionImpl(
      const std::string& path,
      BattOrConnection::Listener* listener,
      scoped_refptr<base::SingleThreadTaskRunner> ui_thread_task_runner);
  ~BattOrConnectionImpl() override;

  void Open() override;
  void Close() override;
  bool IsOpen() override;
  void SendBytes(BattOrMessageType type,
                 const void* buffer,
                 size_t bytes_to_send) override;
  void ReadMessage(BattOrMessageType type) override;
  void CancelReadMessage() override;
  void LogSerial(const std::string& str) override;

  // Flushes the serial connection to the BattOr, reading and throwing away
  // bytes from the serial connection until the connection is quiet for a
  // sufficiently long time. This also discards any trailing bytes from past
  // successful reads.
  void Flush() override;

 protected:
  // Overridden by the test to use a fake serial connection.
  virtual scoped_refptr<device::SerialIoHandler> CreateIoHandler();

  // IO handler capable of reading and writing from the serial connection.
  scoped_refptr<device::SerialIoHandler> io_handler_;

  const base::TickClock* tick_clock_;

 private:
  void OnOpened(bool success);

  // Reads the specified number of additional bytes and adds them to the pending
  // read buffer.
  void BeginReadBytesForMessage(size_t bytes_to_read);

  // Internal callback for when bytes are read. This method may trigger
  // additional reads if any newly read bytes are escape bytes.
  void OnBytesReadForMessage(int bytes_read,
                             device::mojom::SerialReceiveError error);

  void EndReadBytesForMessage(bool success,
                              BattOrMessageType type,
                              std::unique_ptr<std::vector<char>> data);

  void BeginReadBytesForFlush();
  void OnBytesReadForFlush(int bytes_read,
                           device::mojom::SerialReceiveError error);
  void SetTimeout(base::TimeDelta timeout);

  // Pulls off the next complete message from already_read_buffer_, returning
  // its type and contents through out parameters and any error that occurred
  // through the return value.
  enum ParseMessageError {
    NONE = 0,
    NOT_ENOUGH_BYTES = 1,
    MISSING_START_BYTE = 2,
    INVALID_MESSAGE_TYPE = 3,
    TOO_MANY_START_BYTES = 4
  };

  ParseMessageError ParseMessage(BattOrMessageType* type,
                                 std::vector<char>* data);

  // Internal callback for when bytes are sent.
  void OnBytesSent(int bytes_sent, device::mojom::SerialSendError error);

  // The path of the BattOr.
  std::string path_;

  // Indicates whether the connection is currently open.
  bool is_open_;

  // All bytes that have already been read from the serial stream, but have not
  // yet been given to the listener as a complete message.
  std::vector<char> already_read_buffer_;
  // The bytes that were read in the pending read.
  scoped_refptr<net::IOBuffer> pending_read_buffer_;
  // The type of message we're looking for in the pending read.
  BattOrMessageType pending_read_message_type_;

  // The total number of bytes that we're expecting to send.
  size_t pending_write_length_;

  // The start of the period over which no bytes must be read from the serial
  // connection in order for Flush() to be considered complete.
  base::TimeTicks flush_quiet_period_start_;

  // The timeout for the current action.
  base::CancelableClosure timeout_callback_;

  // Threads needed for serial communication.
  scoped_refptr<base::SingleThreadTaskRunner> ui_thread_task_runner_;

  std::fstream serial_log_;

  DISALLOW_COPY_AND_ASSIGN(BattOrConnectionImpl);
};

}  // namespace battor

#endif  // TOOLS_BATTOR_AGENT_BATTOR_CONNECTION_IMPL_H_
