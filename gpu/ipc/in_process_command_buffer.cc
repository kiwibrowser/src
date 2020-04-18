// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/in_process_command_buffer.h"

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <utility>

#include "base/atomic_sequence_num.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/containers/queue.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "gpu/command_buffer/client/gpu_control_client.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/command_buffer/common/swap_buffers_complete_params.h"
#include "gpu/command_buffer/common/swap_buffers_flags.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/gl_context_virtual.h"
#include "gpu/command_buffer/service/gl_state_restorer_impl.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/gpu_fence_manager.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/command_buffer/service/gpu_tracer.h"
#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/command_buffer/service/mailbox_manager_factory.h"
#include "gpu/command_buffer/service/memory_program_cache.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/query_manager.h"
#include "gpu/command_buffer/service/raster_decoder.h"
#include "gpu/command_buffer/service/service_utils.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/command_buffer/service/transfer_buffer_manager.h"
#include "gpu/config/gpu_crash_keys.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_switches.h"
#include "gpu/ipc/gpu_in_process_thread_service.h"
#include "gpu/ipc/host/gpu_memory_buffer_support.h"
#include "gpu/ipc/service/gpu_channel_manager_delegate.h"
#include "gpu/ipc/service/image_transport_surface.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image.h"
#include "ui/gl/gl_image_shared_memory.h"
#include "ui/gl/gl_share_group.h"
#include "ui/gl/init/gl_factory.h"

#if defined(OS_WIN)
#include <windows.h>
#include "base/process/process_handle.h"
#endif

namespace gpu {

namespace {

base::AtomicSequenceNumber g_next_command_buffer_id;
base::AtomicSequenceNumber g_next_image_id;

template <typename T>
base::OnceClosure WrapTaskWithResult(base::OnceCallback<T(void)> task,
                                     T* result,
                                     base::WaitableEvent* completion) {
  auto wrapper = [](base::OnceCallback<T(void)> task, T* result,
                    base::WaitableEvent* completion) {
    *result = std::move(task).Run();
    completion->Signal();
  };
  return base::BindOnce(wrapper, std::move(task), result, completion);
}

class GpuInProcessThreadHolder : public base::Thread {
 public:
  GpuInProcessThreadHolder()
      : base::Thread("GpuThread"), sync_point_manager_(new SyncPointManager()) {
    Start();
  }

  ~GpuInProcessThreadHolder() override { Stop(); }

  void SetGpuFeatureInfo(const GpuFeatureInfo& gpu_feature_info) {
    DCHECK(!gpu_thread_service_.get());
    gpu_feature_info_ = gpu_feature_info;
  }

  const scoped_refptr<InProcessCommandBuffer::Service>& GetGpuThreadService() {
    if (!gpu_thread_service_) {
      DCHECK(base::CommandLine::InitializedForCurrentProcess());
      const base::CommandLine* command_line =
          base::CommandLine::ForCurrentProcess();
      GpuPreferences gpu_preferences = gles2::ParseGpuPreferences(command_line);
      gpu_preferences.texture_target_exception_list =
          CreateBufferUsageAndFormatExceptionList();
      gpu_thread_service_ = base::MakeRefCounted<GpuInProcessThreadService>(
          task_runner(), sync_point_manager_.get(), nullptr, nullptr,
          gpu_feature_info_, gpu_preferences);
    }
    return gpu_thread_service_;
  }

 private:
  std::unique_ptr<SyncPointManager> sync_point_manager_;
  scoped_refptr<InProcessCommandBuffer::Service> gpu_thread_service_;
  GpuFeatureInfo gpu_feature_info_;
};

base::LazyInstance<GpuInProcessThreadHolder>::DestructorAtExit
    g_default_service = LAZY_INSTANCE_INITIALIZER;

class ScopedEvent {
 public:
  explicit ScopedEvent(base::WaitableEvent* event) : event_(event) {}
  ~ScopedEvent() { event_->Signal(); }

 private:
  base::WaitableEvent* event_;
};

scoped_refptr<InProcessCommandBuffer::Service> GetInitialService(
    const scoped_refptr<InProcessCommandBuffer::Service>& service) {
  if (service)
    return service;

  // Call base::ThreadTaskRunnerHandle::IsSet() to ensure that it is
  // instantiated before we create the GPU thread, otherwise shutdown order will
  // delete the ThreadTaskRunnerHandle before the GPU thread's message loop,
  // and when the message loop is shutdown, it will recreate
  // ThreadTaskRunnerHandle, which will re-add a new task to the, AtExitManager,
  // which causes a deadlock because it's already locked.
  base::ThreadTaskRunnerHandle::IsSet();
  return g_default_service.Get().GetGpuThreadService();
}

}  // anonyous namespace

const int InProcessCommandBuffer::kGpuMemoryBufferClientId = 1;

InProcessCommandBuffer::Service::Service(
    const GpuPreferences& gpu_preferences,
    MailboxManager* mailbox_manager,
    scoped_refptr<gl::GLShareGroup> share_group,
    const GpuFeatureInfo& gpu_feature_info)
    : gpu_preferences_(gpu_preferences),
      gpu_feature_info_(gpu_feature_info),
      mailbox_manager_(mailbox_manager),
      share_group_(share_group),
      shader_translator_cache_(gpu_preferences_) {
  if (!mailbox_manager_) {
    // TODO(piman): have embedders own the mailbox manager.
    owned_mailbox_manager_ = gles2::CreateMailboxManager(gpu_preferences_);
    mailbox_manager_ = owned_mailbox_manager_.get();
  }
}

InProcessCommandBuffer::Service::~Service() = default;

const GpuPreferences& InProcessCommandBuffer::Service::gpu_preferences() {
  return gpu_preferences_;
}

scoped_refptr<gl::GLShareGroup> InProcessCommandBuffer::Service::share_group() {
  if (!share_group_.get())
    share_group_ = new gl::GLShareGroup();
  return share_group_;
}

gles2::Outputter* InProcessCommandBuffer::Service::outputter() {
  if (!outputter_)
    outputter_.reset(new gles2::TraceOutputter("InProcessCommandBuffer Trace"));
  return outputter_.get();
}

gles2::ProgramCache* InProcessCommandBuffer::Service::program_cache() {
  if (!program_cache_.get() &&
      (gl::g_current_gl_driver->ext.b_GL_ARB_get_program_binary ||
       gl::g_current_gl_driver->ext.b_GL_OES_get_program_binary) &&
      !gpu_preferences().disable_gpu_program_cache) {
    bool disable_disk_cache =
        gpu_preferences_.disable_gpu_shader_disk_cache ||
        gpu_feature_info_.IsWorkaroundEnabled(gpu::DISABLE_PROGRAM_DISK_CACHE);
    program_cache_.reset(new gles2::MemoryProgramCache(
        gpu_preferences_.gpu_program_cache_size, disable_disk_cache,
        gpu_feature_info_.IsWorkaroundEnabled(
            gpu::DISABLE_PROGRAM_CACHING_FOR_TRANSFORM_FEEDBACK),
        &activity_flags_));
  }
  return program_cache_.get();
}

InProcessCommandBuffer::InProcessCommandBuffer(
    const scoped_refptr<Service>& service)
    : command_buffer_id_(CommandBufferId::FromUnsafeValue(
          g_next_command_buffer_id.GetNext() + 1)),
      flush_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                   base::WaitableEvent::InitialState::NOT_SIGNALED),
      service_(GetInitialService(service)),
      fence_sync_wait_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                             base::WaitableEvent::InitialState::NOT_SIGNALED),
      client_thread_weak_ptr_factory_(this),
      gpu_thread_weak_ptr_factory_(this) {
  DCHECK(service_.get());
}

InProcessCommandBuffer::~InProcessCommandBuffer() {
  Destroy();
}

// static
void InProcessCommandBuffer::InitializeDefaultServiceForTesting(
    const GpuFeatureInfo& gpu_feature_info) {
  // Call base::ThreadTaskRunnerHandle::IsSet() to ensure that it is
  // instantiated before we create the GPU thread, otherwise shutdown order will
  // delete the ThreadTaskRunnerHandle before the GPU thread's message loop,
  // and when the message loop is shutdown, it will recreate
  // ThreadTaskRunnerHandle, which will re-add a new task to the, AtExitManager,
  // which causes a deadlock because it's already locked.
  base::ThreadTaskRunnerHandle::IsSet();
  g_default_service.Get().SetGpuFeatureInfo(gpu_feature_info);
}

gpu::ServiceTransferCache* InProcessCommandBuffer::GetTransferCacheForTest()
    const {
  return static_cast<raster::RasterDecoder*>(decoder_.get())
      ->GetTransferCacheForTest();
}

bool InProcessCommandBuffer::MakeCurrent() {
  CheckSequencedThread();
  command_buffer_lock_.AssertAcquired();

  if (error::IsError(command_buffer_->GetState().error)) {
    DLOG(ERROR) << "MakeCurrent failed because context lost.";
    return false;
  }
  if (!decoder_->MakeCurrent()) {
    DLOG(ERROR) << "Context lost because MakeCurrent failed.";
    command_buffer_->SetParseError(error::kLostContext);
    return false;
  }
  return true;
}

gpu::ContextResult InProcessCommandBuffer::Initialize(
    scoped_refptr<gl::GLSurface> surface,
    bool is_offscreen,
    SurfaceHandle window,
    const ContextCreationAttribs& attribs,
    InProcessCommandBuffer* share_group,
    GpuMemoryBufferManager* gpu_memory_buffer_manager,
    ImageFactory* image_factory,
    GpuChannelManagerDelegate* gpu_channel_manager_delegate,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK(!share_group || service_.get() == share_group->service_.get());

  gpu_memory_buffer_manager_ = gpu_memory_buffer_manager;
  gpu_channel_manager_delegate_ = gpu_channel_manager_delegate;

  if (surface) {
    // If a surface is provided, we are running in a webview and should not have
    // a task runner.
    DCHECK(!task_runner);

    // GPU thread must be the same as client thread due to GLSurface not being
    // thread safe.
    sequence_checker_.reset(new base::SequenceChecker);
    surface_ = surface;
  } else {
    DCHECK(task_runner);
    origin_task_runner_ = std::move(task_runner);
    client_thread_weak_ptr_ = client_thread_weak_ptr_factory_.GetWeakPtr();
  }

  Capabilities capabilities;
  InitializeOnGpuThreadParams params(is_offscreen, window, attribs,
                                     &capabilities, share_group, image_factory);

  base::OnceCallback<gpu::ContextResult(void)> init_task =
      base::BindOnce(&InProcessCommandBuffer::InitializeOnGpuThread,
                     base::Unretained(this), params);

  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  gpu::ContextResult result = gpu::ContextResult::kSuccess;
  QueueOnceTask(true,
                WrapTaskWithResult(std::move(init_task), &result, &completion));
  completion.Wait();

  if (result == gpu::ContextResult::kSuccess)
    capabilities_ = capabilities;

  return result;
}

gpu::ContextResult InProcessCommandBuffer::InitializeOnGpuThread(
    const InitializeOnGpuThreadParams& params) {
  CheckSequencedThread();
  gpu_thread_weak_ptr_ = gpu_thread_weak_ptr_factory_.GetWeakPtr();

  // TODO(crbug.com/832243): This could use the TransferBufferManager owned by
  // |context_group_| instead.
  transfer_buffer_manager_ = std::make_unique<TransferBufferManager>(nullptr);

  if (params.share_command_buffer) {
    context_group_ = params.share_command_buffer->context_group_;
  } else {
    GpuDriverBugWorkarounds workarounds(
        service_->gpu_feature_info().enabled_gpu_driver_bug_workarounds);
    auto feature_info = base::MakeRefCounted<gles2::FeatureInfo>(workarounds);

    context_group_ = base::MakeRefCounted<gles2::ContextGroup>(
        service_->gpu_preferences(),
        gles2::PassthroughCommandDecoderSupported(),
        service_->mailbox_manager(), nullptr /* memory_tracker */,
        service_->shader_translator_cache(),
        service_->framebuffer_completeness_cache(), feature_info,
        params.attribs.bind_generates_resource, service_->image_manager(),
        params.image_factory, nullptr /* progress_reporter */,
        service_->gpu_feature_info(), service_->discardable_manager());
  }

#if defined(OS_MACOSX)
  // Virtualize PreferIntegratedGpu contexts by default on OS X to prevent
  // performance regressions when enabling FCM. https://crbug.com/180463
  use_virtualized_gl_context_ |=
      (params.attribs.gpu_preference == gl::PreferIntegratedGpu);
#endif

  use_virtualized_gl_context_ |= service_->ForceVirtualizedGLContexts();

  // MailboxManagerSync synchronization correctness currently depends on having
  // only a single context. See https://crbug.com/510243 for details.
  use_virtualized_gl_context_ |= service_->mailbox_manager()->UsesSync();

  use_virtualized_gl_context_ |=
      context_group_->feature_info()->workarounds().use_virtualized_gl_contexts;

  command_buffer_ = std::make_unique<CommandBufferService>(
      this, transfer_buffer_manager_.get());

  // Check gpu_preferences() as well as attribs.enable_oop_rasterization to
  // prevent compromised renderer from unilaterally enabling RasterDecoder until
  // we have fuzzed it (https://crbug.com/829469).
  if (service_->gpu_preferences().enable_oop_rasterization &&
      params.attribs.enable_oop_rasterization &&
      params.attribs.enable_raster_interface &&
      !params.attribs.enable_gles2_interface) {
    decoder_.reset(raster::RasterDecoder::Create(this, command_buffer_.get(),
                                                 service_->outputter(),
                                                 context_group_.get()));
  } else {
    decoder_.reset(gles2::GLES2Decoder::Create(this, command_buffer_.get(),
                                               service_->outputter(),
                                               context_group_.get()));
  }

  if (!surface_) {
    if (params.is_offscreen) {
      // TODO(crbug.com/832243): GLES2CommandBufferStub has additional logic for
      // offscreen surfaces that might be needed here.
      surface_ = gl::init::CreateOffscreenGLSurface(gfx::Size());
      if (!surface_.get()) {
        DestroyOnGpuThread();
        LOG(ERROR) << "ContextResult::kFatalFailure: Failed to create surface.";
        return gpu::ContextResult::kFatalFailure;
      }
    } else {
      gl::GLSurfaceFormat surface_format;
      switch (params.attribs.color_space) {
        case COLOR_SPACE_UNSPECIFIED:
          surface_format.SetColorSpace(
              gl::GLSurfaceFormat::COLOR_SPACE_UNSPECIFIED);
          break;
        case COLOR_SPACE_SRGB:
          surface_format.SetColorSpace(gl::GLSurfaceFormat::COLOR_SPACE_SRGB);
          break;
        case COLOR_SPACE_DISPLAY_P3:
          surface_format.SetColorSpace(
              gl::GLSurfaceFormat::COLOR_SPACE_DISPLAY_P3);
          break;
      }
      surface_ = ImageTransportSurface::CreateNativeSurface(
          gpu_thread_weak_ptr_factory_.GetWeakPtr(), params.window,
          surface_format);
      if (!surface_ || !surface_->Initialize(surface_format)) {
        DestroyOnGpuThread();
        LOG(ERROR) << "ContextResult::kFatalFailure: Failed to create surface.";
        return gpu::ContextResult::kFatalFailure;
      }
      if (params.attribs.enable_swap_timestamps_if_supported &&
          surface_->SupportsSwapTimestamps())
        surface_->SetEnableSwapTimestamps();
    }
  }

  // TODO(crbug.com/832243): InProcessCommandBuffer should support using the GPU
  // scheduler for non-WebView cases.
  sync_point_order_data_ =
      service_->sync_point_manager()->CreateSyncPointOrderData();
  sync_point_client_state_ =
      service_->sync_point_manager()->CreateSyncPointClientState(
          GetNamespaceID(), GetCommandBufferID(),
          sync_point_order_data_->sequence_id());

  if (context_group_->use_passthrough_cmd_decoder()) {
    // When using the passthrough command decoder, only share with other
    // contexts in the explicitly requested share group.
    if (params.share_command_buffer) {
      gl_share_group_ = params.share_command_buffer->gl_share_group_;
    } else {
      gl_share_group_ = new gl::GLShareGroup();
    }
  } else {
    // When using the validating command decoder, always use the global share
    // group.
    gl_share_group_ = service_->share_group();
  }

  // TODO(sunnyps): Should this use ScopedCrashKey instead?
  crash_keys::gpu_gl_context_is_virtual.Set(use_virtualized_gl_context_ ? "1"
                                                                        : "0");

  if (use_virtualized_gl_context_) {
    DCHECK(gl_share_group_);
    scoped_refptr<gl::GLContext> real_context =
        gl_share_group_->GetSharedContext(surface_.get());
    if (!real_context.get()) {
      real_context = gl::init::CreateGLContext(
          gl_share_group_.get(), surface_.get(),
          GenerateGLContextAttribs(params.attribs, context_group_.get()));
      if (!real_context) {
        // TODO(piman): This might not be fatal, we could recurse into
        // CreateGLContext to get more info, tho it should be exceedingly
        // rare and may not be recoverable anyway.
        DestroyOnGpuThread();
        LOG(ERROR) << "ContextResult::kFatalFailure: "
                      "Failed to create shared context for virtualization.";
        return gpu::ContextResult::kFatalFailure;
      }
      // Ensure that context creation did not lose track of the intended share
      // group.
      DCHECK(real_context->share_group() == gl_share_group_.get());
      gl_share_group_->SetSharedContext(surface_.get(), real_context.get());

      service_->gpu_feature_info().ApplyToGLContext(real_context.get());
    }

    context_ = base::MakeRefCounted<GLContextVirtual>(
        gl_share_group_.get(), real_context.get(), decoder_->AsWeakPtr());
    if (!context_->Initialize(
            surface_.get(),
            GenerateGLContextAttribs(params.attribs, context_group_.get()))) {
      // TODO(piman): This might not be fatal, we could recurse into
      // CreateGLContext to get more info, tho it should be exceedingly
      // rare and may not be recoverable anyway.
      DestroyOnGpuThread();
      LOG(ERROR) << "ContextResult::kFatalFailure: "
                    "Failed to initialize virtual GL context.";
      return gpu::ContextResult::kFatalFailure;
    }
  } else {
    context_ = gl::init::CreateGLContext(
        gl_share_group_.get(), surface_.get(),
        GenerateGLContextAttribs(params.attribs, context_group_.get()));
    if (!context_) {
      DestroyOnGpuThread();
      LOG(ERROR) << "ContextResult::kFatalFailure: Failed to create context.";
      return gpu::ContextResult::kFatalFailure;
    }
    service_->gpu_feature_info().ApplyToGLContext(context_.get());
  }

  if (!context_->MakeCurrent(surface_.get())) {
    DestroyOnGpuThread();
    // The caller should retry making a context, but this one won't work.
    LOG(ERROR) << "ContextResult::kTransientFailure: "
                  "Could not make context current.";
    return gpu::ContextResult::kTransientFailure;
  }

  if (!context_->GetGLStateRestorer()) {
    context_->SetGLStateRestorer(
        new GLStateRestorerImpl(decoder_->AsWeakPtr()));
  }

  if (!context_group_->has_program_cache() &&
      !context_group_->feature_info()->workarounds().disable_program_cache) {
    context_group_->set_program_cache(service_->program_cache());
  }

  gles2::DisallowedFeatures disallowed_features;
  disallowed_features.gpu_memory_manager = true;
  auto result = decoder_->Initialize(surface_, context_, params.is_offscreen,
                                     disallowed_features, params.attribs);
  if (result != gpu::ContextResult::kSuccess) {
    DestroyOnGpuThread();
    DLOG(ERROR) << "Failed to initialize decoder.";
    return result;
  }

  if (service_->gpu_preferences().enable_gpu_service_logging)
    decoder_->SetLogCommands(true);

  if (use_virtualized_gl_context_) {
    // If virtualized GL contexts are in use, then real GL context state
    // is in an indeterminate state, since the GLStateRestorer was not
    // initialized at the time the GLContextVirtual was made current. In
    // the case that this command decoder is the next one to be
    // processed, force a "full virtual" MakeCurrent to be performed.
    context_->ForceReleaseVirtuallyCurrent();
    if (!context_->MakeCurrent(surface_.get())) {
      DestroyOnGpuThread();
      LOG(ERROR) << "ContextResult::kTransientFailure: "
                    "Failed to make context current after initialization.";
      return gpu::ContextResult::kTransientFailure;
    }
  }

  *params.capabilities = decoder_->GetCapabilities();

  image_factory_ = params.image_factory;

  return gpu::ContextResult::kSuccess;
}

void InProcessCommandBuffer::Destroy() {
  CheckSequencedThread();
  client_thread_weak_ptr_factory_.InvalidateWeakPtrs();
  gpu_control_client_ = nullptr;
  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  bool result = false;
  base::OnceCallback<bool(void)> destroy_task = base::BindOnce(
      &InProcessCommandBuffer::DestroyOnGpuThread, base::Unretained(this));
  QueueOnceTask(
      true, WrapTaskWithResult(std::move(destroy_task), &result, &completion));
  completion.Wait();
}

bool InProcessCommandBuffer::DestroyOnGpuThread() {
  CheckSequencedThread();
  // TODO(sunnyps): Should this use ScopedCrashKey instead?
  crash_keys::gpu_gl_context_is_virtual.Set(use_virtualized_gl_context_ ? "1"
                                                                        : "0");
  gpu_thread_weak_ptr_factory_.InvalidateWeakPtrs();
  // Clean up GL resources if possible.
  bool have_context = context_.get() && context_->MakeCurrent(surface_.get());

  // Prepare to destroy the surface while the context is still current, because
  // some surface destructors make GL calls.
  if (surface_)
    surface_->PrepareToDestroy(have_context);

  if (decoder_) {
    decoder_->Destroy(have_context);
    decoder_.reset();
  }
  command_buffer_.reset();
  surface_ = nullptr;

  context_ = nullptr;
  if (sync_point_order_data_) {
    sync_point_order_data_->Destroy();
    sync_point_order_data_ = nullptr;
  }
  if (sync_point_client_state_) {
    sync_point_client_state_->Destroy();
    sync_point_client_state_ = nullptr;
  }
  gl_share_group_ = nullptr;
  context_group_ = nullptr;

  base::AutoLock lock(task_queue_lock_);
  base::queue<std::unique_ptr<GpuTask>> empty;
  task_queue_.swap(empty);

  return true;
}

void InProcessCommandBuffer::CheckSequencedThread() {
  DCHECK(!sequence_checker_ || sequence_checker_->CalledOnValidSequence());
}

CommandBufferServiceClient::CommandBatchProcessedResult
InProcessCommandBuffer::OnCommandBatchProcessed() {
  return kContinueExecution;
}

void InProcessCommandBuffer::OnParseError() {
  if (!origin_task_runner_)
    return OnContextLost();  // Just kidding, we're on the client thread.
  origin_task_runner_->PostTask(
      FROM_HERE, base::Bind(&InProcessCommandBuffer::OnContextLost,
                            client_thread_weak_ptr_));
}

void InProcessCommandBuffer::OnContextLost() {
  CheckSequencedThread();

#if DCHECK_IS_ON()
  // This method shouldn't be called more than once.
  DCHECK(!context_lost_);
  context_lost_ = true;
#endif

  if (gpu_control_client_)
    gpu_control_client_->OnGpuControlLostContext();
}

void InProcessCommandBuffer::QueueOnceTask(bool out_of_order,
                                           base::OnceClosure task) {
  if (out_of_order) {
    service_->ScheduleTask(std::move(task));
    return;
  }
  // Release the |task_queue_lock_| before calling ScheduleTask because
  // the callback may get called immediately and attempt to acquire the lock.
  uint32_t order_num = sync_point_order_data_->GenerateUnprocessedOrderNumber();
  {
    base::AutoLock lock(task_queue_lock_);
    std::unique_ptr<GpuTask> gpu_task =
        std::make_unique<GpuTask>(std::move(task), order_num);
    task_queue_.push(std::move(gpu_task));
  }
  service_->ScheduleTask(base::BindOnce(
      &InProcessCommandBuffer::ProcessTasksOnGpuThread, gpu_thread_weak_ptr_));
}

void InProcessCommandBuffer::QueueRepeatableTask(base::RepeatingClosure task) {
  // Release the |task_queue_lock_| before calling ScheduleTask because
  // the callback may get called immediately and attempt to acquire the lock.
  uint32_t order_num = sync_point_order_data_->GenerateUnprocessedOrderNumber();
  {
    base::AutoLock lock(task_queue_lock_);
    std::unique_ptr<GpuTask> gpu_task =
        std::make_unique<GpuTask>(std::move(task), order_num);
    task_queue_.push(std::move(gpu_task));
  }
  service_->ScheduleTask(base::Bind(
      &InProcessCommandBuffer::ProcessTasksOnGpuThread, gpu_thread_weak_ptr_));
}

void InProcessCommandBuffer::ProcessTasksOnGpuThread() {
  // TODO(sunnyps): Should this use ScopedCrashKey instead?
  crash_keys::gpu_gl_context_is_virtual.Set(use_virtualized_gl_context_ ? "1"
                                                                        : "0");
  while (command_buffer_->scheduled()) {
    base::AutoLock lock(task_queue_lock_);
    if (task_queue_.empty())
      break;
    GpuTask* task = task_queue_.front().get();
    sync_point_order_data_->BeginProcessingOrderNumber(task->order_number());
    task->Run();
    if (!command_buffer_->scheduled() &&
        !service_->BlockThreadOnWaitSyncToken()) {
      sync_point_order_data_->PauseProcessingOrderNumber(task->order_number());
      // Don't pop the task if it was preempted - it may have been preempted, so
      // we need to execute it again later.
      DCHECK(task->is_repeatable());
      return;
    }
    sync_point_order_data_->FinishProcessingOrderNumber(task->order_number());
    task_queue_.pop();
  }
}

CommandBuffer::State InProcessCommandBuffer::GetLastState() {
  CheckSequencedThread();
  base::AutoLock lock(last_state_lock_);
  return last_state_;
}

void InProcessCommandBuffer::UpdateLastStateOnGpuThread() {
  CheckSequencedThread();
  command_buffer_lock_.AssertAcquired();
  base::AutoLock lock(last_state_lock_);
  command_buffer_->UpdateState();
  State state = command_buffer_->GetState();
  if (state.generation - last_state_.generation < 0x80000000U)
    last_state_ = state;
}

void InProcessCommandBuffer::FlushOnGpuThread(int32_t put_offset,
                                              bool snapshot_requested) {
  CheckSequencedThread();
  ScopedEvent handle_flush(&flush_event_);
  base::AutoLock lock(command_buffer_lock_);

  if (snapshot_requested && snapshot_requested_callback_)
    snapshot_requested_callback_.Run();

  if (!MakeCurrent())
    return;

  command_buffer_->Flush(put_offset, decoder_.get());
  // Update state before signaling the flush event.
  UpdateLastStateOnGpuThread();

  // If we've processed all pending commands but still have pending queries,
  // pump idle work until the query is passed.
  if (put_offset == command_buffer_->GetState().get_offset &&
      (decoder_->HasMoreIdleWork() || decoder_->HasPendingQueries())) {
    ScheduleDelayedWorkOnGpuThread();
  }
}

void InProcessCommandBuffer::PerformDelayedWorkOnGpuThread() {
  CheckSequencedThread();
  delayed_work_pending_ = false;
  base::AutoLock lock(command_buffer_lock_);
  // TODO(sunnyps): Should this use ScopedCrashKey instead?
  crash_keys::gpu_gl_context_is_virtual.Set(use_virtualized_gl_context_ ? "1"
                                                                        : "0");
  if (MakeCurrent()) {
    decoder_->PerformIdleWork();
    decoder_->ProcessPendingQueries(false);
    if (decoder_->HasMoreIdleWork() || decoder_->HasPendingQueries()) {
      ScheduleDelayedWorkOnGpuThread();
    }
  }
}

void InProcessCommandBuffer::ScheduleDelayedWorkOnGpuThread() {
  CheckSequencedThread();
  if (delayed_work_pending_)
    return;
  delayed_work_pending_ = true;
  service_->ScheduleDelayedWork(
      base::Bind(&InProcessCommandBuffer::PerformDelayedWorkOnGpuThread,
                 gpu_thread_weak_ptr_));
}

void InProcessCommandBuffer::Flush(int32_t put_offset) {
  CheckSequencedThread();
  if (GetLastState().error != error::kNoError)
    return;

  if (last_put_offset_ == put_offset)
    return;

  last_put_offset_ = put_offset;
  base::RepeatingClosure task = base::BindRepeating(
      &InProcessCommandBuffer::FlushOnGpuThread, gpu_thread_weak_ptr_,
      put_offset, snapshot_requested_);
  snapshot_requested_ = false;
  QueueRepeatableTask(std::move(task));

  flushed_fence_sync_release_ = next_fence_sync_release_ - 1;
}

void InProcessCommandBuffer::OrderingBarrier(int32_t put_offset) {
  Flush(put_offset);
}

CommandBuffer::State InProcessCommandBuffer::WaitForTokenInRange(int32_t start,
                                                                 int32_t end) {
  CheckSequencedThread();
  State last_state = GetLastState();
  while (!InRange(start, end, last_state.token) &&
         last_state.error == error::kNoError) {
    flush_event_.Wait();
    last_state = GetLastState();
  }
  return last_state;
}

CommandBuffer::State InProcessCommandBuffer::WaitForGetOffsetInRange(
    uint32_t set_get_buffer_count,
    int32_t start,
    int32_t end) {
  CheckSequencedThread();
  State last_state = GetLastState();
  while (((set_get_buffer_count != last_state.set_get_buffer_count) ||
          !InRange(start, end, last_state.get_offset)) &&
         last_state.error == error::kNoError) {
    flush_event_.Wait();
    last_state = GetLastState();
  }
  return last_state;
}

void InProcessCommandBuffer::SetGetBuffer(int32_t shm_id) {
  CheckSequencedThread();
  if (GetLastState().error != error::kNoError)
    return;

  base::WaitableEvent completion(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::OnceClosure task =
      base::BindOnce(&InProcessCommandBuffer::SetGetBufferOnGpuThread,
                     base::Unretained(this), shm_id, &completion);
  QueueOnceTask(false, std::move(task));
  completion.Wait();

  last_put_offset_ = 0;
}

void InProcessCommandBuffer::SetGetBufferOnGpuThread(
    int32_t shm_id,
    base::WaitableEvent* completion) {
  base::AutoLock lock(command_buffer_lock_);
  command_buffer_->SetGetBuffer(shm_id);
  UpdateLastStateOnGpuThread();
  completion->Signal();
}

scoped_refptr<Buffer> InProcessCommandBuffer::CreateTransferBuffer(
    size_t size,
    int32_t* id) {
  CheckSequencedThread();
  base::AutoLock lock(command_buffer_lock_);
  return command_buffer_->CreateTransferBuffer(size, id);
}

void InProcessCommandBuffer::DestroyTransferBuffer(int32_t id) {
  CheckSequencedThread();
  base::OnceClosure task =
      base::BindOnce(&InProcessCommandBuffer::DestroyTransferBufferOnGpuThread,
                     base::Unretained(this), id);

  QueueOnceTask(false, std::move(task));
}

void InProcessCommandBuffer::DestroyTransferBufferOnGpuThread(int32_t id) {
  base::AutoLock lock(command_buffer_lock_);
  command_buffer_->DestroyTransferBuffer(id);
}

void InProcessCommandBuffer::SetGpuControlClient(GpuControlClient* client) {
  gpu_control_client_ = client;
}

const Capabilities& InProcessCommandBuffer::GetCapabilities() const {
  return capabilities_;
}

const GpuFeatureInfo& InProcessCommandBuffer::GetGpuFeatureInfo() const {
  return service_->gpu_feature_info();
}

int32_t InProcessCommandBuffer::CreateImage(ClientBuffer buffer,
                                            size_t width,
                                            size_t height,
                                            unsigned internalformat) {
  CheckSequencedThread();

  DCHECK(gpu_memory_buffer_manager_);
  gfx::GpuMemoryBuffer* gpu_memory_buffer =
      reinterpret_cast<gfx::GpuMemoryBuffer*>(buffer);
  DCHECK(gpu_memory_buffer);

  int32_t new_id = g_next_image_id.GetNext() + 1;

  DCHECK(IsImageFromGpuMemoryBufferFormatSupported(
      gpu_memory_buffer->GetFormat(), capabilities_));
  DCHECK(IsImageFormatCompatibleWithGpuMemoryBufferFormat(
      internalformat, gpu_memory_buffer->GetFormat()));

  // This handle is owned by the GPU thread and must be passed to it or it
  // will leak. In otherwords, do not early out on error between here and the
  // queuing of the CreateImage task below.
  gfx::GpuMemoryBufferHandle handle =
      gfx::CloneHandleForIPC(gpu_memory_buffer->GetHandle());
  bool requires_sync_point = handle.type == gfx::IO_SURFACE_BUFFER;

  uint64_t fence_sync = 0;
  if (requires_sync_point) {
    fence_sync = GenerateFenceSyncRelease();

    // Previous fence syncs should be flushed already.
    DCHECK_EQ(fence_sync - 1, flushed_fence_sync_release_);
  }

  QueueOnceTask(
      false,
      base::BindOnce(&InProcessCommandBuffer::CreateImageOnGpuThread,
                     base::Unretained(this), new_id, handle,
                     gfx::Size(base::checked_cast<int>(width),
                               base::checked_cast<int>(height)),
                     gpu_memory_buffer->GetFormat(),
                     base::checked_cast<uint32_t>(internalformat), fence_sync));

  if (fence_sync) {
    flushed_fence_sync_release_ = fence_sync;
    SyncToken sync_token(GetNamespaceID(), GetCommandBufferID(), fence_sync);
    sync_token.SetVerifyFlush();
    gpu_memory_buffer_manager_->SetDestructionSyncToken(gpu_memory_buffer,
                                                        sync_token);
  }

  return new_id;
}

void InProcessCommandBuffer::CreateImageOnGpuThread(
    int32_t id,
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    uint32_t internalformat,
    uint64_t fence_sync) {
  gles2::ImageManager* image_manager = service_->image_manager();
  DCHECK(image_manager);
  if (image_manager->LookupImage(id)) {
    LOG(ERROR) << "Image already exists with same ID.";
    return;
  }

  switch (handle.type) {
    case gfx::SHARED_MEMORY_BUFFER: {
      if (!base::IsValueInRangeForNumericType<size_t>(handle.stride)) {
        LOG(ERROR) << "Invalid stride for image.";
        return;
      }
      scoped_refptr<gl::GLImageSharedMemory> image(
          new gl::GLImageSharedMemory(size, internalformat));
      if (!image->Initialize(handle.handle, handle.id, format, handle.offset,
                             handle.stride)) {
        LOG(ERROR) << "Failed to initialize image.";
        return;
      }

      image_manager->AddImage(image.get(), id);
      break;
    }
    default: {
      if (!image_factory_) {
        LOG(ERROR) << "Image factory missing but required by buffer type.";
        return;
      }

      scoped_refptr<gl::GLImage> image =
          image_factory_->CreateImageForGpuMemoryBuffer(
              handle, size, format, internalformat, kGpuMemoryBufferClientId,
              kNullSurfaceHandle);
      if (!image.get()) {
        LOG(ERROR) << "Failed to create image for buffer.";
        return;
      }

      image_manager->AddImage(image.get(), id);
      break;
    }
  }

  if (fence_sync)
    sync_point_client_state_->ReleaseFenceSync(fence_sync);
}

void InProcessCommandBuffer::DestroyImage(int32_t id) {
  CheckSequencedThread();

  QueueOnceTask(false,
                base::BindOnce(&InProcessCommandBuffer::DestroyImageOnGpuThread,
                               base::Unretained(this), id));
}

void InProcessCommandBuffer::DestroyImageOnGpuThread(int32_t id) {
  gles2::ImageManager* image_manager = service_->image_manager();
  DCHECK(image_manager);
  if (!image_manager->LookupImage(id)) {
    LOG(ERROR) << "Image with ID doesn't exist.";
    return;
  }

  image_manager->RemoveImage(id);
}

void InProcessCommandBuffer::OnConsoleMessage(int32_t id,
                                              const std::string& message) {
  // TODO(piman): implement this.
}

void InProcessCommandBuffer::CacheShader(const std::string& key,
                                         const std::string& shader) {
  // TODO(piman): implement this.
}

void InProcessCommandBuffer::OnFenceSyncRelease(uint64_t release) {
  SyncToken sync_token(GetNamespaceID(), GetCommandBufferID(), release);

  context_group_->mailbox_manager()->PushTextureUpdates(sync_token);

  sync_point_client_state_->ReleaseFenceSync(release);
}

bool InProcessCommandBuffer::OnWaitSyncToken(const SyncToken& sync_token) {
  DCHECK(!waiting_for_sync_point_);
  SyncPointManager* sync_point_manager = service_->sync_point_manager();
  DCHECK(sync_point_manager);

  MailboxManager* mailbox_manager = context_group_->mailbox_manager();
  DCHECK(mailbox_manager);

  if (service_->BlockThreadOnWaitSyncToken()) {
    // Wait if sync point wait is valid.
    if (sync_point_client_state_->Wait(
            sync_token,
            base::Bind(&base::WaitableEvent::Signal,
                       base::Unretained(&fence_sync_wait_event_)))) {
      fence_sync_wait_event_.Wait();
    }

    mailbox_manager->PullTextureUpdates(sync_token);
    return false;
  }

  waiting_for_sync_point_ = sync_point_client_state_->Wait(
      sync_token,
      base::Bind(&InProcessCommandBuffer::OnWaitSyncTokenCompleted,
                 gpu_thread_weak_ptr_factory_.GetWeakPtr(), sync_token));
  if (!waiting_for_sync_point_) {
    mailbox_manager->PullTextureUpdates(sync_token);
    return false;
  }

  command_buffer_->SetScheduled(false);
  return true;
}

void InProcessCommandBuffer::OnWaitSyncTokenCompleted(
    const SyncToken& sync_token) {
  DCHECK(waiting_for_sync_point_);
  context_group_->mailbox_manager()->PullTextureUpdates(sync_token);
  waiting_for_sync_point_ = false;
  command_buffer_->SetScheduled(true);
  service_->ScheduleTask(base::Bind(
      &InProcessCommandBuffer::ProcessTasksOnGpuThread, gpu_thread_weak_ptr_));
}

void InProcessCommandBuffer::OnDescheduleUntilFinished() {
  if (!service_->BlockThreadOnWaitSyncToken()) {
    DCHECK(command_buffer_->scheduled());
    DCHECK(decoder_->HasPollingWork());

    command_buffer_->SetScheduled(false);
  }
}

void InProcessCommandBuffer::OnRescheduleAfterFinished() {
  if (!service_->BlockThreadOnWaitSyncToken()) {
    DCHECK(!command_buffer_->scheduled());

    command_buffer_->SetScheduled(true);
    ProcessTasksOnGpuThread();
  }
}

void InProcessCommandBuffer::OnSwapBuffers(uint64_t swap_id, uint32_t flags) {
  pending_swap_completed_params_.push_back({swap_id, flags});
  pending_presented_params_.push_back({swap_id, flags});
}

void InProcessCommandBuffer::SignalSyncTokenOnGpuThread(
    const SyncToken& sync_token,
    base::OnceClosure callback) {
  base::RepeatingClosure maybe_pass_callback =
      base::AdaptCallbackForRepeating(WrapCallback(std::move(callback)));
  if (!sync_point_client_state_->Wait(sync_token, maybe_pass_callback)) {
    maybe_pass_callback.Run();
  }
}

void InProcessCommandBuffer::SignalQuery(unsigned query_id,
                                         base::OnceClosure callback) {
  CheckSequencedThread();
  QueueOnceTask(false,
                base::BindOnce(&InProcessCommandBuffer::SignalQueryOnGpuThread,
                               base::Unretained(this), query_id,
                               WrapCallback(std::move(callback))));
}

void InProcessCommandBuffer::SignalQueryOnGpuThread(
    unsigned query_id,
    base::OnceClosure callback) {
  decoder_->SetQueryCallback(query_id, std::move(callback));
}

void InProcessCommandBuffer::CreateGpuFence(uint32_t gpu_fence_id,
                                            ClientGpuFence source) {
  CheckSequencedThread();

  // Pass a cloned handle to the GPU process since the source ClientGpuFence
  // may go out of scope before the queued task runs.
  gfx::GpuFence* gpu_fence = gfx::GpuFence::FromClientGpuFence(source);
  gfx::GpuFenceHandle handle =
      gfx::CloneHandleForIPC(gpu_fence->GetGpuFenceHandle());

  QueueOnceTask(
      false, base::BindOnce(&InProcessCommandBuffer::CreateGpuFenceOnGpuThread,
                            base::Unretained(this), gpu_fence_id, handle));
}

void InProcessCommandBuffer::CreateGpuFenceOnGpuThread(
    uint32_t gpu_fence_id,
    const gfx::GpuFenceHandle& handle) {
  if (!GetFeatureInfo()->feature_flags().chromium_gpu_fence) {
    DLOG(ERROR) << "CHROMIUM_gpu_fence unavailable";
    command_buffer_->SetParseError(error::kLostContext);
    return;
  }

  gles2::GpuFenceManager* gpu_fence_manager = decoder_->GetGpuFenceManager();
  DCHECK(gpu_fence_manager);

  if (gpu_fence_manager->CreateGpuFenceFromHandle(gpu_fence_id, handle))
    return;

  // The insertion failed. This shouldn't happen, force context loss to avoid
  // inconsistent state.
  command_buffer_->SetParseError(error::kLostContext);
}

void InProcessCommandBuffer::GetGpuFence(
    uint32_t gpu_fence_id,
    base::OnceCallback<void(std::unique_ptr<gfx::GpuFence>)> callback) {
  CheckSequencedThread();
  auto task_runner = base::ThreadTaskRunnerHandle::IsSet()
                         ? base::ThreadTaskRunnerHandle::Get()
                         : nullptr;
  QueueOnceTask(
      false, base::BindOnce(&InProcessCommandBuffer::GetGpuFenceOnGpuThread,
                            base::Unretained(this), gpu_fence_id, task_runner,
                            std::move(callback)));
}

void InProcessCommandBuffer::GetGpuFenceOnGpuThread(
    uint32_t gpu_fence_id,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    base::OnceCallback<void(std::unique_ptr<gfx::GpuFence>)> callback) {
  if (!GetFeatureInfo()->feature_flags().chromium_gpu_fence) {
    DLOG(ERROR) << "CHROMIUM_gpu_fence unavailable";
    command_buffer_->SetParseError(error::kLostContext);
    return;
  }

  gles2::GpuFenceManager* manager = decoder_->GetGpuFenceManager();
  DCHECK(manager);

  std::unique_ptr<gfx::GpuFence> gpu_fence;
  if (manager->IsValidGpuFence(gpu_fence_id)) {
    gpu_fence = manager->GetGpuFence(gpu_fence_id);
  } else {
    // Retrieval failed. This shouldn't happen, force context loss to avoid
    // inconsistent state.
    DLOG(ERROR) << "GpuFence not found";
    command_buffer_->SetParseError(error::kLostContext);
  }

  // Execute callback on client thread using the supplied task_runner where
  // available, cf. WrapCallback and PostCallback.
  base::OnceClosure callback_closure =
      base::BindOnce(std::move(callback), std::move(gpu_fence));
  if (task_runner.get() && !task_runner->BelongsToCurrentThread()) {
    task_runner->PostTask(FROM_HERE, std::move(callback_closure));
  } else {
    std::move(callback_closure).Run();
  }
}

void InProcessCommandBuffer::SetLock(base::Lock*) {
  // No support for using on multiple threads.
  NOTREACHED();
}

void InProcessCommandBuffer::EnsureWorkVisible() {
  // This is only relevant for out-of-process command buffers.
}

CommandBufferNamespace InProcessCommandBuffer::GetNamespaceID() const {
  return CommandBufferNamespace::IN_PROCESS;
}

CommandBufferId InProcessCommandBuffer::GetCommandBufferID() const {
  return command_buffer_id_;
}

void InProcessCommandBuffer::FlushPendingWork() {
  // This is only relevant for out-of-process command buffers.
}

uint64_t InProcessCommandBuffer::GenerateFenceSyncRelease() {
  return next_fence_sync_release_++;
}

bool InProcessCommandBuffer::IsFenceSyncReleased(uint64_t release) {
  return release <= GetLastState().release_count;
}

void InProcessCommandBuffer::SignalSyncToken(const SyncToken& sync_token,
                                             base::OnceClosure callback) {
  CheckSequencedThread();
  QueueOnceTask(
      false, base::BindOnce(&InProcessCommandBuffer::SignalSyncTokenOnGpuThread,
                            base::Unretained(this), sync_token,
                            WrapCallback(std::move(callback))));
}

void InProcessCommandBuffer::WaitSyncTokenHint(const SyncToken& sync_token) {}

bool InProcessCommandBuffer::CanWaitUnverifiedSyncToken(
    const SyncToken& sync_token) {
  return sync_token.namespace_id() == GetNamespaceID();
}

void InProcessCommandBuffer::SetSnapshotRequested() {
  snapshot_requested_ = true;
}

#if defined(OS_WIN)
void InProcessCommandBuffer::DidCreateAcceleratedSurfaceChildWindow(
    SurfaceHandle parent_window,
    SurfaceHandle child_window) {
  // In the browser process call ::SetParent() directly.
  if (!gpu_channel_manager_delegate_) {
    ::SetParent(child_window, parent_window);
    // Move D3D window behind Chrome's window to avoid losing some messages.
    ::SetWindowPos(child_window, HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE);
    return;
  }

  // In the GPU process forward the request back to the browser process.
  gpu_channel_manager_delegate_->SendAcceleratedSurfaceCreatedChildWindow(
      parent_window, child_window);
}
#endif

void InProcessCommandBuffer::DidSwapBuffersComplete(
    SwapBuffersCompleteParams params) {
  params.swap_response.swap_id = pending_swap_completed_params_.front().swap_id;
  pending_swap_completed_params_.pop_front();

  if (!origin_task_runner_) {
    DidSwapBuffersCompleteOnOriginThread(std::move(params));
    return;
  }
  origin_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&InProcessCommandBuffer::DidSwapBuffersCompleteOnOriginThread,
                 client_thread_weak_ptr_, base::Passed(&params)));
}

const gles2::FeatureInfo* InProcessCommandBuffer::GetFeatureInfo() const {
  return context_group_->feature_info();
}

const GpuPreferences& InProcessCommandBuffer::GetGpuPreferences() const {
  return context_group_->gpu_preferences();
}

void InProcessCommandBuffer::SetSnapshotRequestedCallback(
    const base::Closure& callback) {
  snapshot_requested_callback_ = callback;
}

void InProcessCommandBuffer::BufferPresented(
    const gfx::PresentationFeedback& feedback) {
  SwapBufferParams params = pending_presented_params_.front();
  pending_presented_params_.pop_front();

  if (!origin_task_runner_) {
    BufferPresentedOnOriginThread(params.swap_id, params.flags, feedback);
    return;
  }
  origin_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&InProcessCommandBuffer::BufferPresentedOnOriginThread,
                 client_thread_weak_ptr_, params.swap_id, params.flags,
                 feedback));
}

void InProcessCommandBuffer::AddFilter(IPC::MessageFilter* message_filter) {
  NOTREACHED();
}

int32_t InProcessCommandBuffer::GetRouteID() const {
  NOTREACHED();
  return 0;
}

void InProcessCommandBuffer::DidSwapBuffersCompleteOnOriginThread(
    SwapBuffersCompleteParams params) {
  if (gpu_control_client_)
    gpu_control_client_->OnGpuControlSwapBuffersCompleted(params);
}

void InProcessCommandBuffer::BufferPresentedOnOriginThread(
    uint64_t swap_id,
    uint32_t flags,
    const gfx::PresentationFeedback& feedback) {
  if (gpu_control_client_)
    gpu_control_client_->OnSwapBufferPresented(swap_id, feedback);
  if (flags & gpu::SwapBuffersFlags::kPresentationFeedback ||
      (flags & gpu::SwapBuffersFlags::kVSyncParams &&
       feedback.flags & gfx::PresentationFeedback::kVSync)) {
    if (update_vsync_parameters_completion_callback_ &&
        feedback.timestamp != base::TimeTicks())
      update_vsync_parameters_completion_callback_.Run(feedback.timestamp,
                                                       feedback.interval);
  }
}

void InProcessCommandBuffer::SetUpdateVSyncParametersCallback(
    const UpdateVSyncParametersCallback& callback) {
  update_vsync_parameters_completion_callback_ = callback;
}

namespace {

void PostCallback(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    base::OnceClosure callback) {
  // The task_runner.get() check is to support using InProcessCommandBuffer on
  // a thread without a message loop.
  if (task_runner.get() && !task_runner->BelongsToCurrentThread()) {
    task_runner->PostTask(FROM_HERE, std::move(callback));
  } else {
    std::move(callback).Run();
  }
}

void RunOnTargetThread(base::OnceClosure callback) {
  DCHECK(!callback.is_null());
  std::move(callback).Run();
}

}  // anonymous namespace

base::OnceClosure InProcessCommandBuffer::WrapCallback(
    base::OnceClosure callback) {
  // Make sure the callback gets deleted on the target thread by passing
  // ownership.
  base::OnceClosure callback_on_client_thread =
      base::BindOnce(&RunOnTargetThread, std::move(callback));
  base::OnceClosure wrapped_callback =
      base::BindOnce(&PostCallback,
                     base::ThreadTaskRunnerHandle::IsSet()
                         ? base::ThreadTaskRunnerHandle::Get()
                         : nullptr,
                     std::move(callback_on_client_thread));
  return wrapped_callback;
}

InProcessCommandBuffer::GpuTask::GpuTask(base::OnceClosure callback,
                                         uint32_t order_number)
    : once_closure_(std::move(callback)), order_number_(order_number) {}

InProcessCommandBuffer::GpuTask::GpuTask(base::RepeatingClosure callback,
                                         uint32_t order_number)
    : repeating_closure_(std::move(callback)), order_number_(order_number) {}

InProcessCommandBuffer::GpuTask::~GpuTask() = default;

void InProcessCommandBuffer::GpuTask::Run() {
  if (once_closure_) {
    std::move(once_closure_).Run();
    return;
  }
  DCHECK(repeating_closure_) << "Trying to run a OnceClosure more than once.";
  repeating_closure_.Run();
}

}  // namespace gpu
