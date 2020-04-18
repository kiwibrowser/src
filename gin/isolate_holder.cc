// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/public/isolate_holder.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/message_loop/message_loop_current.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "gin/debug_impl.h"
#include "gin/function_template.h"
#include "gin/per_isolate_data.h"
#include "gin/run_microtasks_observer.h"
#include "gin/v8_initializer.h"
#include "gin/v8_isolate_memory_dump_provider.h"

namespace gin {

namespace {
v8::ArrayBuffer::Allocator* g_array_buffer_allocator = nullptr;
const intptr_t* g_reference_table = nullptr;
}  // namespace

IsolateHolder::IsolateHolder(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : IsolateHolder(std::move(task_runner), AccessMode::kSingleThread) {}

IsolateHolder::IsolateHolder(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    AccessMode access_mode)
    : IsolateHolder(std::move(task_runner),
                    access_mode,
                    kAllowAtomicsWait,
                    IsolateCreationMode::kNormal) {}

IsolateHolder::IsolateHolder(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    AccessMode access_mode,
    AllowAtomicsWaitMode atomics_wait_mode,
    IsolateCreationMode isolate_creation_mode)
    : access_mode_(access_mode) {
  DCHECK(task_runner);
  DCHECK(task_runner->BelongsToCurrentThread());

  v8::ArrayBuffer::Allocator* allocator = g_array_buffer_allocator;
  CHECK(allocator) << "You need to invoke gin::IsolateHolder::Initialize first";

  isolate_ = v8::Isolate::Allocate();
  isolate_data_.reset(
      new PerIsolateData(isolate_, allocator, access_mode_, task_runner));
  if (isolate_creation_mode == IsolateCreationMode::kCreateSnapshot) {
    // This branch is called when creating a V8 snapshot for Blink.
    // Note SnapshotCreator calls isolate->Enter() in its construction.
    snapshot_creator_.reset(
        new v8::SnapshotCreator(isolate_, g_reference_table));
    DCHECK_EQ(isolate_, snapshot_creator_->GetIsolate());
  } else {
    v8::Isolate::CreateParams params;
    params.entry_hook = DebugImpl::GetFunctionEntryHook();
    params.code_event_handler = DebugImpl::GetJitCodeEventHandler();
    params.constraints.ConfigureDefaults(
        base::SysInfo::AmountOfPhysicalMemory(),
        base::SysInfo::AmountOfVirtualMemory());
    params.array_buffer_allocator = allocator;
    params.allow_atomics_wait =
        atomics_wait_mode == AllowAtomicsWaitMode::kAllowAtomicsWait;
    params.external_references = g_reference_table;
    params.only_terminate_in_safe_scope = true;

    v8::Isolate::Initialize(isolate_, params);
  }

  isolate_memory_dump_provider_.reset(
      new V8IsolateMemoryDumpProvider(this, task_runner));
#if defined(OS_WIN)
  {
    void* code_range;
    size_t size;
    isolate_->GetCodeRange(&code_range, &size);
    Debug::CodeRangeCreatedCallback callback =
        DebugImpl::GetCodeRangeCreatedCallback();
    if (code_range && size && callback)
      callback(code_range, size);
  }
#endif
}

IsolateHolder::~IsolateHolder() {
  if (task_observer_.get())
    base::MessageLoopCurrent::Get()->RemoveTaskObserver(task_observer_.get());
#if defined(OS_WIN)
  {
    void* code_range;
    size_t size;
    isolate_->GetCodeRange(&code_range, &size);
    Debug::CodeRangeDeletedCallback callback =
        DebugImpl::GetCodeRangeDeletedCallback();
    if (code_range && callback)
      callback(code_range);
  }
#endif
  isolate_memory_dump_provider_.reset();
  isolate_data_.reset();
  isolate_->Dispose();
  isolate_ = nullptr;
}

// static
void IsolateHolder::Initialize(ScriptMode mode,
                               V8ExtrasMode v8_extras_mode,
                               v8::ArrayBuffer::Allocator* allocator,
                               const intptr_t* reference_table) {
  CHECK(allocator);
  V8Initializer::Initialize(mode, v8_extras_mode);
  g_array_buffer_allocator = allocator;
  g_reference_table = reference_table;
}

void IsolateHolder::AddRunMicrotasksObserver() {
  DCHECK(!task_observer_.get());
  task_observer_.reset(new RunMicrotasksObserver(isolate_));
  base::MessageLoopCurrent::Get()->AddTaskObserver(task_observer_.get());
}

void IsolateHolder::RemoveRunMicrotasksObserver() {
  DCHECK(task_observer_.get());
  base::MessageLoopCurrent::Get()->RemoveTaskObserver(task_observer_.get());
  task_observer_.reset();
}

void IsolateHolder::EnableIdleTasks(
    std::unique_ptr<V8IdleTaskRunner> idle_task_runner) {
  DCHECK(isolate_data_.get());
  isolate_data_->EnableIdleTasks(std::move(idle_task_runner));
}

}  // namespace gin
