// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_DATA_CONSUMER_HANDLE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_DATA_CONSUMER_HANDLE_H_

#include <stddef.h>
#include <memory>

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_common.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace blink {

// WebDataConsumerHandle represents the "consumer" side of a data pipe. A user
// can read data from it.
//
// A WebDataConsumerHandle is a thread-safe object. A user can call
// |ObtainReader| or destruct the object on any thread.
// A WebDataConsumerHandle having a reader is called "locked". A
// WebDataConsumerHandle or its reader are called "waiting" when reading from
// the handle or reader returns kShouldWait.
//
// WebDataConsumerHandle can be created / used / destructed only on
// Oilpan-enabled threads.
// TODO(yhirano): Remove this restriction.
class BLINK_PLATFORM_EXPORT WebDataConsumerHandle {
 public:
  using Flags = unsigned;
  static const Flags kFlagNone = 0;

  enum Result {
    kOk,
    kDone,
    kBusy,
    kShouldWait,
    kResourceExhausted,
    kUnexpectedError,
  };

  // Client gets notification from the pipe.
  class BLINK_PLATFORM_EXPORT Client {
   public:
    virtual ~Client() = default;
    // The associated handle gets readable. This function will be called
    // when the associated reader was waiting but is not waiting any more.
    // This means this function can be called when handle gets errored or
    // closed. This also means that this function will not be called even
    // when some data arrives if the handle already has non-empty readable
    // data.
    // It is not guaranteed that the handle is not waiting when this
    // function is called, i.e. it can be called more than needed.
    // One can use / destruct the associated reader in this function.
    virtual void DidGetReadable() = 0;
  };

  // This class provides a means to read data from the associated handle. A
  // Reader object is bound to the thread on which |ObtainReader| is called.
  // Any functions including the destructor should be called on the thread.
  // A reader can outlive the associated handle. In such a case, the handle
  // destruction will not affect the reader functionality.
  // Reading functions may success (i.e. return kOk) or fail (otherwise), and
  // the behavior which is not specified is unspecified.
  class BLINK_PLATFORM_EXPORT Reader {
   public:
    // Destructing a reader means it is released and a user can get another
    // Reader by calling |ObtainReader| on any thread again.
    virtual ~Reader() = default;

    // Reads data into |data| up to |size| bytes. The actual read size will
    // be stored in |*read_size|. This function cannot be called when a
    // two-phase read is in progress.
    // Returns kDone when it reaches to the end of the data.
    // Returns kShouldWait when the handle does not have data to read but
    // it is not closed or errored.
    // The default implementation uses BeginRead and EndRead.
    virtual Result Read(void* data,
                        size_t /* size */,
                        Flags,
                        size_t* read_size);

    // Begins a two-phase read. On success, the function stores a buffer
    // that contains the read data of length |*available| into |*buffer|.
    // Returns kDone when it reaches to the end of the data.
    // Returns kShouldWait when the handle does not have data to read but
    // it is not closed or errored.
    // On fail, you don't have to (and should not) call EndRead, because the
    // read session implicitly ends in that case.
    virtual Result BeginRead(const void** buffer, Flags, size_t* available) = 0;

    // Ends a two-phase read.
    // |read_size| indicates the actual read size.
    virtual Result EndRead(size_t read_size) = 0;
  };

  WebDataConsumerHandle();
  virtual ~WebDataConsumerHandle();

  // Returns a non-null reader. This function can be called only when this
  // handle is not locked. |client| can be null. Otherwise, |*client| must be
  // valid as long as the reader is valid. The returned reader is bound to
  // the calling thread and client notification will be called on the thread
  // if |client| is not null.
  // If |client| is not null and the handle is not waiting, client
  // notification is called asynchronously.
  // |task_runner| cannot be null.
  virtual std::unique_ptr<Reader> ObtainReader(
      Client*,
      scoped_refptr<base::SingleThreadTaskRunner>) = 0;

  // Returns a string literal (e.g. class name) for debugging only.
  virtual const char* DebugName() const = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_DATA_CONSUMER_HANDLE_H_
