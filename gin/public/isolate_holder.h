// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_PUBLIC_ISOLATE_HOLDER_H_
#define GIN_PUBLIC_ISOLATE_HOLDER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "gin/gin_export.h"
#include "gin/public/v8_idle_task_runner.h"
#include "v8/include/v8.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace gin {

class PerIsolateData;
class RunMicrotasksObserver;
class V8IsolateMemoryDumpProvider;

// To embed Gin, first initialize gin using IsolateHolder::Initialize and then
// create an instance of IsolateHolder to hold the v8::Isolate in which you
// will execute JavaScript. You might wish to subclass IsolateHolder if you
// want to tie more state to the lifetime of the isolate.
class GIN_EXPORT IsolateHolder {
 public:
  // Controls whether or not V8 should only accept strict mode scripts.
  enum ScriptMode {
    kNonStrictMode,
    kStrictMode
  };

  // Stores whether the client uses v8::Locker to access the isolate.
  enum AccessMode {
    kSingleThread,
    kUseLocker
  };

  // Whether Atomics.wait can be called on this isolate.
  enum AllowAtomicsWaitMode {
    kDisallowAtomicsWait,
    kAllowAtomicsWait
  };

  // Indicates whether V8 works with stable or experimental v8 extras.
  enum V8ExtrasMode {
    kStableV8Extras,
    kStableAndExperimentalV8Extras,
  };

  // Indicates how the Isolate instance will be created.
  enum class IsolateCreationMode {
    kNormal,
    kCreateSnapshot,
  };

  explicit IsolateHolder(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  IsolateHolder(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                AccessMode access_mode);
  IsolateHolder(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      AccessMode access_mode,
      AllowAtomicsWaitMode atomics_wait_mode,
      IsolateCreationMode isolate_creation_mode = IsolateCreationMode::kNormal);
  ~IsolateHolder();

  // Should be invoked once before creating IsolateHolder instances to
  // initialize V8 and Gin. In case V8_USE_EXTERNAL_STARTUP_DATA is
  // defined, V8's initial natives should be loaded (by calling
  // V8Initializer::LoadV8NativesFromFD or
  // V8Initializer::LoadV8Natives) before calling this method.  If the
  // snapshot file is available, it should also be loaded (by calling
  // V8Initializer::LoadV8SnapshotFromFD or
  // V8Initializer::LoadV8Snapshot) before calling this method.
  // If the snapshot file contains customised contexts which have static
  // external references, |reference_table| needs to point an array of those
  // reference pointers. Otherwise, it can be nullptr.
  static void Initialize(ScriptMode mode,
                         V8ExtrasMode v8_extras_mode,
                         v8::ArrayBuffer::Allocator* allocator,
                         const intptr_t* reference_table = nullptr);

  v8::Isolate* isolate() { return isolate_; }

  // The implementations of Object.observe() and Promise enqueue v8 Microtasks
  // that should be executed just before control is returned to the message
  // loop. This method adds a MessageLoop TaskObserver which runs any pending
  // Microtasks each time a Task is completed. This method should be called
  // once, when a MessageLoop is created and it should be called on the
  // MessageLoop's thread.
  void AddRunMicrotasksObserver();

  // This method should also only be called once, and on the MessageLoop's
  // thread.
  void RemoveRunMicrotasksObserver();

  // This method returns if v8::Locker is needed to access isolate.
  AccessMode access_mode() const { return access_mode_; }

  v8::SnapshotCreator* snapshot_creator() const {
    return snapshot_creator_.get();
  }

  void EnableIdleTasks(std::unique_ptr<V8IdleTaskRunner> idle_task_runner);

  // This method returns V8IsolateMemoryDumpProvider of this isolate, used for
  // testing.
  V8IsolateMemoryDumpProvider* isolate_memory_dump_provider_for_testing()
      const {
    return isolate_memory_dump_provider_.get();
  }

 private:
  void SetUp(scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  std::unique_ptr<v8::SnapshotCreator> snapshot_creator_;
  v8::Isolate* isolate_;
  std::unique_ptr<PerIsolateData> isolate_data_;
  std::unique_ptr<RunMicrotasksObserver> task_observer_;
  std::unique_ptr<V8IsolateMemoryDumpProvider> isolate_memory_dump_provider_;
  AccessMode access_mode_;

  DISALLOW_COPY_AND_ASSIGN(IsolateHolder);
};

}  // namespace gin

#endif  // GIN_PUBLIC_ISOLATE_HOLDER_H_
