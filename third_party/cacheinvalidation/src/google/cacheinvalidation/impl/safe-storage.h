// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// An implementation of the Storage resource that schedules the callbacks on the
// given scheduler thread.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_SAFE_STORAGE_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_SAFE_STORAGE_H_

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/include/types.h"

namespace invalidation {

// An implementation of the Storage resource that schedules the callbacks on the
// given scheduler thread.
class SafeStorage : public Storage {
 public:
  /* Creates a new instance. Storage for |delegate| is owned by caller. */
  explicit SafeStorage(Storage* delegate) : delegate_(delegate) {
  }

  virtual ~SafeStorage() {}

  // All public methods below are methods of the Storage interface.
  virtual void SetSystemResources(SystemResources* resources);

  virtual void WriteKey(const string& key, const string& value,
                        WriteKeyCallback* done);

  virtual void ReadKey(const string& key, ReadKeyCallback* done);

  virtual void DeleteKey(const string& key, DeleteKeyCallback* done);

  virtual void ReadAllKeys(ReadAllKeysCallback* key_callback);

 private:
  /* Callback invoked when WriteKey finishes. */
  void WriteCallback(WriteKeyCallback* done, Status status);

  /* Callback invoked when ReadKey finishes. */
  void ReadCallback(ReadKeyCallback* done, StatusStringPair read_result);

  /* Callback invoked when DeleteKey finishes. */
  void DeleteCallback(DeleteKeyCallback* done, bool result);

  /* Callback invoked when ReadAllKeys finishes. */
  void ReadAllCallback(ReadAllKeysCallback* key_callback,
                       StatusStringPair result);

  /* The delegate to which the calls are forwarded. */
  Storage* delegate_;

  /* The scheduler on which the callbacks are scheduled. */
  Scheduler* scheduler_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_SAFE_STORAGE_H_
