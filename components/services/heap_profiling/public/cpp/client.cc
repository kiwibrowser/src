// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/public/cpp/client.h"

#include "base/allocator/allocator_interception_mac.h"
#include "base/files/platform_file.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/trace_event/malloc_dump_provider.h"
#include "build/build_config.h"
#include "components/services/heap_profiling/public/cpp/allocator_shim.h"
#include "components/services/heap_profiling/public/cpp/sender_pipe.h"
#include "components/services/heap_profiling/public/cpp/stream.h"
#include "mojo/public/cpp/system/platform_handle.h"

#if defined(OS_ANDROID) && BUILDFLAG(CAN_UNWIND_WITH_CFI_TABLE) && \
    defined(OFFICIAL_BUILD)
#include "base/trace_event/cfi_backtrace_android.h"
#endif

namespace heap_profiling {

namespace {
const int kTimeoutDurationMs = 10000;

#if defined(OS_ANDROID) && BUILDFLAG(CAN_UNWIND_WITH_CFI_TABLE) && \
    defined(OFFICIAL_BUILD)
void EnsureCFIInitializedOnBackgroundThread(
    scoped_refptr<base::SingleThreadTaskRunner> callback_task_runner,
    base::OnceClosure callback) {
  bool can_unwind =
      base::trace_event::CFIBacktraceAndroid::GetInitializedInstance()
          ->can_unwind_stack_frames();
  DCHECK(can_unwind);
  callback_task_runner->PostTask(FROM_HERE, std::move(callback));
}
#endif

}  // namespace

Client::Client() : started_profiling_(false), weak_factory_(this) {}

Client::~Client() {
  if (started_profiling_) {
    StopAllocatorShimDangerous();

    base::trace_event::MallocDumpProvider::GetInstance()->EnableMetrics();

    // The allocator shim cannot be synchronously, consistently stopped. We leak
    // the sender_pipe_, with the idea that very few future messages will
    // be sent to it. This happens at shutdown, so resources will be reclaimed
    // by the OS after the process is terminated.
    sender_pipe_.release();
  }
}

void Client::BindToInterface(mojom::ProfilingClientRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void Client::StartProfiling(mojom::ProfilingParamsPtr params) {
  if (started_profiling_)
    return;
  started_profiling_ = true;

  base::PlatformFile platform_file;
  CHECK_EQ(MOJO_RESULT_OK, mojo::UnwrapPlatformFile(
                               std::move(params->sender_pipe), &platform_file));

  base::ScopedPlatformFile scoped_platform_file(platform_file);
  sender_pipe_.reset(new SenderPipe(std::move(scoped_platform_file)));

  StreamHeader header;
  header.signature = kStreamSignature;
  SenderPipe::Result result =
      sender_pipe_->Send(&header, sizeof(header), kTimeoutDurationMs);
  if (result != SenderPipe::Result::kSuccess) {
    sender_pipe_->Close();
    return;
  }

  base::trace_event::MallocDumpProvider::GetInstance()->DisableMetrics();

#if defined(OS_MACOSX)
  // On macOS, this call is necessary to shim malloc zones that were created
  // after startup. This cannot be done during shim initialization because the
  // task scheduler has not yet been initialized.
  base::allocator::PeriodicallyShimNewMallocZones();
#endif

#if defined(OS_ANDROID) && BUILDFLAG(CAN_UNWIND_WITH_CFI_TABLE) && \
    defined(OFFICIAL_BUILD)
  // On Android the unwinder initialization requires file reading before
  // initializing shim. So, post task on background thread.
  auto init_callback =
      base::BindOnce(&Client::InitAllocatorShimOnUIThread,
                     weak_factory_.GetWeakPtr(), std::move(params));

  auto background_task = base::BindOnce(&EnsureCFIInitializedOnBackgroundThread,
                                        base::ThreadTaskRunnerHandle::Get(),
                                        std::move(init_callback));
  base::PostTaskWithTraits(FROM_HERE,
                           {base::TaskPriority::BACKGROUND, base::MayBlock(),
                            base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
                           std::move(background_task));
#else
  InitAllocatorShim(sender_pipe_.get(), std::move(params));
#endif
}

void Client::FlushMemlogPipe(uint32_t barrier_id) {
  AllocatorShimFlushPipe(barrier_id);
}

void Client::InitAllocatorShimOnUIThread(mojom::ProfilingParamsPtr params) {
  InitAllocatorShim(sender_pipe_.get(), std::move(params));
}

}  // namespace heap_profiling
