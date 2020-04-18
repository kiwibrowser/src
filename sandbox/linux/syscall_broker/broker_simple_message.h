// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SYSCALL_BROKER_BROKER_SIMPLE_MESSAGE_H_
#define SANDBOX_LINUX_SYSCALL_BROKER_BROKER_SIMPLE_MESSAGE_H_

#include <stdint.h>
#include <sys/types.h>

#include "base/files/scoped_file.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace syscall_broker {

// This class is meant to provide a very simple messaging mechanism that is
// signal-safe for the broker to utilize. This addresses many of the issues
// outlined in https://crbug.com/255063. In short, the use of the standard
// base::UnixDomainSockets is not possible because it uses base::Pickle and
// std::vector, which are not signal-safe.
//
// In implementation, much of the code for sending and receiving is taken from
// base::UnixDomainSockets and re-used below. Thus, ultimately, it might be
// worthwhile making a first-class base-supported signal-safe set of mechanisms
// that reduces the code duplication.
class SANDBOX_EXPORT BrokerSimpleMessage {
 public:
  BrokerSimpleMessage();

  // Signal-safe
  ssize_t SendRecvMsgWithFlags(int fd,
                               int recvmsg_flags,
                               int* send_fd,
                               BrokerSimpleMessage* reply);

  // Use sendmsg to write the given msg and the file descriptor |send_fd|.
  // Returns true if successful. Signal-safe.
  bool SendMsg(int fd, int send_fd);

  // Similar to RecvMsg, but allows to specify |flags| for recvmsg(2).
  // Guaranteed to return either 1 or 0 fds. Signal-safe.
  ssize_t RecvMsgWithFlags(int fd, int flags, base::ScopedFD* return_fd);

  // Adds a NUL-terminated C-style string to the message as a raw buffer.
  // Returns true if the internal message buffer has room for the data, and the
  // data is successfully appended.
  bool AddStringToMessage(const char* string);

  // Adds a raw data buffer to the message. If the raw data is actually a
  // string, be sure to have length explicitly include the '\0' terminating
  // character. Returns true if the internal message buffer has room for the
  // data, and the data is successfully appended.
  bool AddDataToMessage(const char* buffer, size_t length);

  // Adds an int to the message. Returns true if the internal message buffer has
  // room for the int and the int is successfully added.
  bool AddIntToMessage(int int_to_add);

  // This returns a pointer to the next available data buffer in |data|. The
  // pointer is owned by |this| class. The resulting buffer is a string and
  // terminated with a '\0' character.
  bool ReadString(const char** string);

  // This returns a pointer to the next available data buffer in the message
  // in |data|, and the length of the buffer in |length|. The buffer is owned by
  // |this| class.
  bool ReadData(const char** data, size_t* length);

  // This reads the next available int from the message and stores it in
  // |result|.
  bool ReadInt(int* result);

  // The maximum length of a message in the fixed size buffer.
  static constexpr size_t kMaxMessageLength = 4096;

 private:
  friend class BrokerSimpleMessageTestHelper;

  enum class EntryType : uint32_t { DATA = 0xBDBDBD80, INT = 0xBDBDBD81 };

  // Returns whether or not the next available entry matches the expected entry
  // type.
  bool ValidateType(EntryType expected_type);

  // Set to true once a message is read from, it may never be written to.
  bool read_only_;
  // Set to true once a message is written to, it may never be read from.
  bool write_only_;
  // Set when an operation fails, so that all subsequed operations fail,
  // including any attempt to send the broken message.
  bool broken_;
  // The current length of the contents in the |message_| buffer.
  size_t length_;
  // The pointer to the next location in the |message_| buffer to read from.
  uint8_t* read_next_;
  // The pointer to the next location in the |message_| buffer to write from.
  uint8_t* write_next_;
  // The statically allocated buffer of size |kMaxMessageLength|.
  uint8_t message_[kMaxMessageLength];
};

}  // namespace syscall_broker
}  // namespace sandbox

#endif  // SANDBOX_LINUX_SYSCALL_BROKER_BROKER_SIMPLE_MESSAGE_H_
