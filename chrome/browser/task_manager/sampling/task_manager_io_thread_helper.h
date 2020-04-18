// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TASK_MANAGER_SAMPLING_TASK_MANAGER_IO_THREAD_HELPER_H_
#define CHROME_BROWSER_TASK_MANAGER_SAMPLING_TASK_MANAGER_IO_THREAD_HELPER_H_

#include <stdint.h>

#include <unordered_map>

#include "base/callback.h"
#include "base/hash.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace net {
class URLRequest;
}  // namespace net

namespace task_manager {

// Identifies the initiator of a network request, by a (child_id,
// route_id) tuple.
// BytesTransferredKey supports hashing and may be used as an unordered_map key.
struct BytesTransferredKey {
  // The unique ID of the host of the child process requester.
  int child_id;

  // The ID of the IPC route for the URLRequest (this identifies the
  // RenderView or like-thing in the renderer that the request gets routed
  // to).
  int route_id;

  struct Hasher {
    size_t operator()(const BytesTransferredKey& key) const;
  };

  bool operator==(const BytesTransferredKey& other) const;
};

// This is the entry of the unordered map that tracks bytes transfered by task.
struct BytesTransferredParam {
  // The number of bytes read.
  int64_t byte_read_count = 0;

  // The number of bytes sent.
  int64_t byte_sent_count = 0;
};

using BytesTransferredMap = std::unordered_map<BytesTransferredKey,
                                               BytesTransferredParam,
                                               BytesTransferredKey::Hasher>;

using BytesTransferredCallback =
    base::RepeatingCallback<void(BytesTransferredMap)>;

// Defines a utility class used to schedule the creation and removal of the
// TaskManagerIoThreadHelper on the IO thread.
class IoThreadHelperManager {
 public:
  // A callback that executes whenever there is activity that registers that
  // bytes have been transferred. It is called from task_manager_impl.cc binding
  // |OnMultipleBytesTranferred|.
  explicit IoThreadHelperManager(BytesTransferredCallback result_callback);
  ~IoThreadHelperManager();

 private:
  DISALLOW_COPY_AND_ASSIGN(IoThreadHelperManager);
};

// Defines a class used by the task manager to receive notifications of the
// network bytes transferred by the various tasks.
// This object lives entirely only on the IO thread.
class TaskManagerIoThreadHelper {
 public:
  // Create and delete the instance of this class. They must be called on the IO
  // thread.
  static void CreateInstance(BytesTransferredCallback result_callback);
  static void DeleteInstance();

  // This is used to forward the call to update the network bytes with
  // transferred bytes from the TaskManagerInterface.
  static void OnRawBytesTransferred(BytesTransferredKey key,
                                    int64_t bytes_read,
                                    int64_t bytes_sent);

 private:
  explicit TaskManagerIoThreadHelper(BytesTransferredCallback result_callback);
  ~TaskManagerIoThreadHelper();

  // We gather multiple notifications on the IO thread in one second before a
  // call is made to the following function to start the processing for
  // transferred bytes.
  void OnMultipleBytesTransferredIO();

  // This will update the task manager with the network bytes read.
  void OnNetworkBytesTransferred(BytesTransferredKey key,
                                 int64_t bytes_read,
                                 int64_t bytes_sent);

  // This unordered_map will be filled on IO thread with information about the
  // number of bytes transferred from URLRequests.
  BytesTransferredMap bytes_transferred_unordered_map_;

  BytesTransferredCallback result_callback_;

  base::WeakPtrFactory<TaskManagerIoThreadHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TaskManagerIoThreadHelper);
};

}  // namespace task_manager

#endif  // CHROME_BROWSER_TASK_MANAGER_SAMPLING_TASK_MANAGER_IO_THREAD_HELPER_H_
