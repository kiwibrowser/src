// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_BATTOR_CONNECTION_H_
#define TOOLS_BATTOR_AGENT_BATTOR_CONNECTION_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "tools/battor_agent/battor_protocol_types.h"

namespace battor {

// A BattOrConnection is a wrapper around the serial connection to the BattOr
// that handles conversion of a message to and from the byte-level BattOr
// protocol.
//
// At a high-level, all BattOr messages consist of:
//
//   0x00               (1 byte start marker)
//   uint8_t            (1 byte header indicating the message type)
//   data               (message data, with 0x00s and 0x01s escaped with 0x02)
//   0x01               (1 byte end marker)
//
// For a more in-depth description of the protocol, see http://bit.ly/1NvNVc3.
class BattOrConnection {
 public:
  // The listener interface that must be implemented in order to interact with
  // the BattOrConnection.
  class Listener {
   public:
    virtual void OnConnectionOpened(bool success) = 0;
    virtual void OnConnectionFlushed(bool success) = 0;
    virtual void OnBytesSent(bool success) = 0;
    virtual void OnMessageRead(bool success,
                               BattOrMessageType type,
                               std::unique_ptr<std::vector<char>> bytes) = 0;
  };

  BattOrConnection(Listener* listener);
  virtual ~BattOrConnection() = 0;

  // Opens and initializes the serial connection to the BattOr and calls the
  // listener's OnConnectionOpened() when complete. As part of this
  // initialization, the serial connection is flushed by reading and throwing
  // away bytes until the serial connection remains quiet for a sufficiently
  // long time. This function must be called before using the
  // BattOrConnection. If the connection is already open, calling this method
  // reflushes the connection and then calls the listener's OnConnectionOpened
  // method.
  virtual void Open() = 0;
  // Closes the serial connection and releases any handles being held.
  virtual void Close() = 0;
  // Returns true if the connection is currently open.
  virtual bool IsOpen() = 0;
  // Flushes the serial connection by reading and throwing away bytes until the
  // serial connection remains quiet for a sufficiently long time.
  virtual void Flush() = 0;

  // Sends the specified buffer over the serial connection and calls the
  // listener's OnBytesSent() when complete. Note that bytes_to_send should not
  // include the start, end, type, or escape bytes required by the BattOr
  // protocol.
  virtual void SendBytes(BattOrMessageType type,
                         const void* buffer,
                         size_t bytes_to_send) = 0;

  // Gets the next message available from the serial connection, reading the
  // correct number of bytes based on the specified message type, and calls the
  // listener's OnMessageRead() when complete.
  virtual void ReadMessage(BattOrMessageType type) = 0;

  // Cancels the current message read operation.
  virtual void CancelReadMessage() = 0;

  // Appends |str| to the serial log file if it exists.
  virtual void LogSerial(const std::string& str) = 0;

 protected:
  // The listener receiving the results of the commands being executed.
  Listener* listener_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BattOrConnection);
};

}  // namespace battor

#endif  // TOOLS_BATTOR_AGENT_BATTOR_CONNECTION_H_
