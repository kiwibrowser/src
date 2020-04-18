// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "components/viz/common/gpu/context_cache_controller.h"
#include "gpu/command_buffer/client/gles2_cmd_helper.h"
#include "gpu/command_buffer/client/gles2_trace_implementation.h"
#include "gpu/command_buffer/client/gpu_switches.h"
#include "gpu/command_buffer/client/implementation_base.h"
#include "gpu/command_buffer/client/raster_cmd_helper.h"
#include "gpu/command_buffer/client/raster_implementation.h"
#include "gpu/command_buffer/client/raster_implementation_gles.h"
#include "gpu/command_buffer/client/transfer_buffer.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/ipc/client/command_buffer_proxy_impl.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/skia_bindings/gles2_implementation_with_grcontext_support.h"
#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"
#include "services/ui/public/cpp/gpu/command_buffer_metrics.h"
#include "third_party/skia/include/core/SkTraceMemoryDump.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gl/trace_util.h"

class SkDiscardableMemory;

namespace {

// Derives from SkTraceMemoryDump and implements graphics specific memory
// backing functionality.
class SkiaGpuTraceMemoryDump : public SkTraceMemoryDump {
 public:
  // This should never outlive the provided ProcessMemoryDump, as it should
  // always be scoped to a single OnMemoryDump funciton call.
  explicit SkiaGpuTraceMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                                  uint64_t share_group_tracing_guid)
      : pmd_(pmd), share_group_tracing_guid_(share_group_tracing_guid) {}

  // Overridden from SkTraceMemoryDump:
  void dumpNumericValue(const char* dump_name,
                        const char* value_name,
                        const char* units,
                        uint64_t value) override {
    auto* dump = GetOrCreateAllocatorDump(dump_name);
    dump->AddScalar(value_name, units, value);
  }

  void setMemoryBacking(const char* dump_name,
                        const char* backing_type,
                        const char* backing_object_id) override {
    const uint64_t tracing_process_id =
        base::trace_event::MemoryDumpManager::GetInstance()
            ->GetTracingProcessId();

    // For uniformity, skia provides this value as a string. Convert back to a
    // uint32_t.
    uint32_t gl_id =
        std::strtoul(backing_object_id, nullptr /* str_end */, 10 /* base */);

    // Constants used by SkiaGpuTraceMemoryDump to identify different memory
    // types.
    const char* kGLTextureBackingType = "gl_texture";
    const char* kGLBufferBackingType = "gl_buffer";
    const char* kGLRenderbufferBackingType = "gl_renderbuffer";

    // Populated in if statements below.
    base::trace_event::MemoryAllocatorDumpGuid guid;

    if (strcmp(backing_type, kGLTextureBackingType) == 0) {
      guid = gl::GetGLTextureClientGUIDForTracing(share_group_tracing_guid_,
                                                  gl_id);
    } else if (strcmp(backing_type, kGLBufferBackingType) == 0) {
      guid = gl::GetGLBufferGUIDForTracing(tracing_process_id, gl_id);
    } else if (strcmp(backing_type, kGLRenderbufferBackingType) == 0) {
      guid = gl::GetGLRenderbufferGUIDForTracing(tracing_process_id, gl_id);
    }

    if (!guid.empty()) {
      pmd_->CreateSharedGlobalAllocatorDump(guid);

      auto* dump = GetOrCreateAllocatorDump(dump_name);

      const int kImportance = 2;
      pmd_->AddOwnershipEdge(dump->guid(), guid, kImportance);
    }
  }

  void setDiscardableMemoryBacking(
      const char* dump_name,
      const SkDiscardableMemory& discardable_memory_object) override {
    // We don't use this class for dumping discardable memory.
    NOTREACHED();
  }

  LevelOfDetail getRequestedDetails() const override {
    // TODO(ssid): Use MemoryDumpArgs to create light dumps when requested
    // (crbug.com/499731).
    return kObjectsBreakdowns_LevelOfDetail;
  }

  bool shouldDumpWrappedObjects() const override {
    // Chrome already dumps objects it imports into Skia. Avoid duplicate dumps
    // by asking Skia not to dump them.
    return false;
  }

 private:
  // Helper to create allocator dumps.
  base::trace_event::MemoryAllocatorDump* GetOrCreateAllocatorDump(
      const char* dump_name) {
    auto* dump = pmd_->GetAllocatorDump(dump_name);
    if (!dump)
      dump = pmd_->CreateAllocatorDump(dump_name);
    return dump;
  }

  base::trace_event::ProcessMemoryDump* pmd_;
  uint64_t share_group_tracing_guid_;

  DISALLOW_COPY_AND_ASSIGN(SkiaGpuTraceMemoryDump);
};

}  // namespace

namespace ui {

ContextProviderCommandBuffer::ContextProviderCommandBuffer(
    scoped_refptr<gpu::GpuChannelHost> channel,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    int32_t stream_id,
    gpu::SchedulingPriority stream_priority,
    gpu::SurfaceHandle surface_handle,
    const GURL& active_url,
    bool automatic_flushes,
    bool support_locking,
    bool support_grcontext,
    const gpu::SharedMemoryLimits& memory_limits,
    const gpu::ContextCreationAttribs& attributes,
    command_buffer_metrics::ContextType type)
    : stream_id_(stream_id),
      stream_priority_(stream_priority),
      surface_handle_(surface_handle),
      active_url_(active_url),
      automatic_flushes_(automatic_flushes),
      support_locking_(support_locking),
      support_grcontext_(support_grcontext),
      memory_limits_(memory_limits),
      attributes_(attributes),
      context_type_(type),
      channel_(std::move(channel)),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      impl_(nullptr) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK(channel_);
  context_thread_checker_.DetachFromThread();
}

ContextProviderCommandBuffer::~ContextProviderCommandBuffer() {
  DCHECK(main_thread_checker_.CalledOnValidThread() ||
         context_thread_checker_.CalledOnValidThread());

  if (bind_tried_ && bind_result_ == gpu::ContextResult::kSuccess) {
    // Clear the lock to avoid DCHECKs that the lock is being held during
    // shutdown.
    command_buffer_->SetLock(nullptr);
    // Disconnect lost callbacks during destruction.
    impl_->SetLostContextCallback(base::OnceClosure());
    // Unregister memory dump provider.
    base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
        this);
  }
}

gpu::CommandBufferProxyImpl*
ContextProviderCommandBuffer::GetCommandBufferProxy() {
  return command_buffer_.get();
}

uint32_t ContextProviderCommandBuffer::GetCopyTextureInternalFormat() {
  if (attributes_.alpha_size > 0)
    return GL_RGBA;
  DCHECK_NE(attributes_.red_size, 0);
  DCHECK_NE(attributes_.green_size, 0);
  DCHECK_NE(attributes_.blue_size, 0);
  return GL_RGB;
}

void ContextProviderCommandBuffer::AddRef() const {
  base::RefCountedThreadSafe<ContextProviderCommandBuffer>::AddRef();
}

void ContextProviderCommandBuffer::Release() const {
  base::RefCountedThreadSafe<ContextProviderCommandBuffer>::Release();
}

gpu::ContextResult ContextProviderCommandBuffer::BindToCurrentThread() {
  // This is called on the thread the context will be used.
  DCHECK(context_thread_checker_.CalledOnValidThread());

  if (bind_tried_)
    return bind_result_;

  bind_tried_ = true;
  // Any early-out should set this to a failure code and return it.
  bind_result_ = gpu::ContextResult::kSuccess;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      default_task_runner_;
  if (!task_runner)
    task_runner = base::ThreadTaskRunnerHandle::Get();

  // This command buffer is a client-side proxy to the command buffer in the
  // GPU process.
  command_buffer_ = std::make_unique<gpu::CommandBufferProxyImpl>(
      std::move(channel_), gpu_memory_buffer_manager_, stream_id_, task_runner);
  bind_result_ = command_buffer_->Initialize(
      surface_handle_, /*shared_command_buffer=*/nullptr, stream_priority_,
      attributes_, active_url_);
  if (bind_result_ != gpu::ContextResult::kSuccess) {
    DLOG(ERROR) << "GpuChannelHost failed to create command buffer.";
    command_buffer_metrics::UmaRecordContextInitFailed(context_type_);
    return bind_result_;
  }

  if (attributes_.enable_oop_rasterization) {
    DCHECK(attributes_.enable_raster_interface);
    DCHECK(!attributes_.enable_gles2_interface);
    DCHECK(!support_grcontext_);
    // The raster helper writes the command buffer protocol.
    auto raster_helper =
        std::make_unique<gpu::raster::RasterCmdHelper>(command_buffer_.get());
    raster_helper->SetAutomaticFlushes(automatic_flushes_);
    bind_result_ =
        raster_helper->Initialize(memory_limits_.command_buffer_size);
    if (bind_result_ != gpu::ContextResult::kSuccess) {
      DLOG(ERROR) << "Failed to initialize RasterCmdHelper.";
      return bind_result_;
    }
    // The transfer buffer is used to copy resources between the client
    // process and the GPU process.
    transfer_buffer_ =
        std::make_unique<gpu::TransferBuffer>(raster_helper.get());

    // The RasterImplementation exposes the RasterInterface, as well as the
    // gpu::ContextSupport interface.
    auto raster_impl = std::make_unique<gpu::raster::RasterImplementation>(
        raster_helper.get(), transfer_buffer_.get(),
        attributes_.bind_generates_resource,
        attributes_.lose_context_when_out_of_memory, command_buffer_.get());
    bind_result_ = raster_impl->Initialize(memory_limits_);
    if (bind_result_ != gpu::ContextResult::kSuccess) {
      DLOG(ERROR) << "Failed to initialize RasterImplementation.";
      return bind_result_;
    }

    std::string type_name =
        command_buffer_metrics::ContextTypeToString(context_type_);
    std::string unique_context_name =
        base::StringPrintf("%s-%p", type_name.c_str(), raster_impl.get());
    raster_impl->TraceBeginCHROMIUM("gpu_toplevel",
                                    unique_context_name.c_str());

    impl_ = raster_impl.get();
    raster_interface_ = std::move(raster_impl);
    helper_ = std::move(raster_helper);
  } else {
    // The GLES2 helper writes the command buffer protocol.
    auto gles2_helper =
        std::make_unique<gpu::gles2::GLES2CmdHelper>(command_buffer_.get());
    gles2_helper->SetAutomaticFlushes(automatic_flushes_);
    bind_result_ = gles2_helper->Initialize(memory_limits_.command_buffer_size);
    if (bind_result_ != gpu::ContextResult::kSuccess) {
      DLOG(ERROR) << "Failed to initialize GLES2CmdHelper.";
      return bind_result_;
    }

    // The transfer buffer is used to copy resources between the client
    // process and the GPU process.
    transfer_buffer_ =
        std::make_unique<gpu::TransferBuffer>(gles2_helper.get());

    // The GLES2Implementation exposes the OpenGLES2 API, as well as the
    // gpu::ContextSupport interface.
    constexpr bool support_client_side_arrays = false;

    std::unique_ptr<gpu::gles2::GLES2Implementation> gles2_impl;
    if (support_grcontext_) {
      // GLES2ImplementationWithGrContextSupport adds a bit of overhead, so
      // we only use it if grcontext_support was requested.
      gles2_impl = std::make_unique<
          skia_bindings::GLES2ImplementationWithGrContextSupport>(
          gles2_helper.get(), /*share_group=*/nullptr, transfer_buffer_.get(),
          attributes_.bind_generates_resource,
          attributes_.lose_context_when_out_of_memory,
          support_client_side_arrays, command_buffer_.get());
    } else {
      gles2_impl = std::make_unique<gpu::gles2::GLES2Implementation>(
          gles2_helper.get(), /*share_group=*/nullptr, transfer_buffer_.get(),
          attributes_.bind_generates_resource,
          attributes_.lose_context_when_out_of_memory,
          support_client_side_arrays, command_buffer_.get());
    }
    bind_result_ = gles2_impl->Initialize(memory_limits_);
    if (bind_result_ != gpu::ContextResult::kSuccess) {
      DLOG(ERROR) << "Failed to initialize GLES2Implementation.";
      return bind_result_;
    }

    impl_ = gles2_impl.get();
    gles2_impl_ = std::move(gles2_impl);
    helper_ = std::move(gles2_helper);
  }

  if (command_buffer_->GetLastState().error != gpu::error::kNoError) {
    // The context was DOA, which can be caused by other contexts and we
    // could try again.
    LOG(ERROR) << "ContextResult::kTransientFailure: "
                  "Context dead on arrival. Last error: "
               << command_buffer_->GetLastState().error;
    bind_result_ = gpu::ContextResult::kTransientFailure;
    return bind_result_;
  }

  cache_controller_ =
      std::make_unique<viz::ContextCacheController>(impl_, task_runner);

  impl_->SetLostContextCallback(
      base::Bind(&ContextProviderCommandBuffer::OnLostContext,
                 // |this| owns the impl_, which holds the callback.
                 base::Unretained(this)));

  if (gles2_impl_) {
    // Grab the implementation directly instead of going through ContextGL()
    // because the lock hasn't been acquired yet.
    gpu::gles2::GLES2Interface* gl = gles2_impl_.get();
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kEnableGpuClientTracing)) {
      // This wraps the real GLES2Implementation and we should always use this
      // instead when it's present.
      trace_impl_ = std::make_unique<gpu::gles2::GLES2TraceImplementation>(
          gles2_impl_.get());
      gl = trace_impl_.get();
    }

    // Do this last once the context is set up.
    std::string type_name =
        command_buffer_metrics::ContextTypeToString(context_type_);
    std::string unique_context_name =
        base::StringPrintf("%s-%p", type_name.c_str(), gles2_impl_.get());
    gl->TraceBeginCHROMIUM("gpu_toplevel", unique_context_name.c_str());
  }

  // If support_locking_ is true, the context may be used from multiple
  // threads, and any async callstacks will need to hold the same lock, so
  // give it to the command buffer and cache controller.
  // We don't hold a lock here since there's no need, so set the lock very last
  // to prevent asserts that we're not holding it.
  if (support_locking_) {
    command_buffer_->SetLock(&context_lock_);
    cache_controller_->SetLock(&context_lock_);
  }
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      this, "ContextProviderCommandBuffer", std::move(task_runner));
  return bind_result_;
}

gpu::gles2::GLES2Interface* ContextProviderCommandBuffer::ContextGL() {
  DCHECK(bind_tried_);
  DCHECK_EQ(bind_result_, gpu::ContextResult::kSuccess);
  CheckValidThreadOrLockAcquired();

  if (!attributes_.enable_gles2_interface) {
    DLOG(ERROR) << "Unexpected access to ContextGL()";
    return nullptr;
  }

  if (trace_impl_)
    return trace_impl_.get();
  return gles2_impl_.get();
}

gpu::raster::RasterInterface* ContextProviderCommandBuffer::RasterInterface() {
  DCHECK(bind_tried_);
  DCHECK_EQ(bind_result_, gpu::ContextResult::kSuccess);
  CheckValidThreadOrLockAcquired();

  if (raster_interface_)
    return raster_interface_.get();

  if (!attributes_.enable_raster_interface) {
    DLOG(ERROR) << "Unexpected access to RasterInterface()";
    return nullptr;
  }

  if (!gles2_impl_.get())
    return nullptr;

  raster_interface_ = std::make_unique<gpu::raster::RasterImplementationGLES>(
      gles2_impl_.get(), command_buffer_.get(), ContextCapabilities());
  return raster_interface_.get();
}

gpu::ContextSupport* ContextProviderCommandBuffer::ContextSupport() {
  return impl_;
}

class GrContext* ContextProviderCommandBuffer::GrContext() {
  DCHECK(bind_tried_);
  DCHECK_EQ(bind_result_, gpu::ContextResult::kSuccess);
  DCHECK(support_grcontext_);
  DCHECK(ContextSupport()->HasGrContextSupport());
  CheckValidThreadOrLockAcquired();

  if (gr_context_)
    return gr_context_->get();

  if (attributes_.enable_oop_rasterization)
    return nullptr;

  // TODO(vmiura): crbug.com/793508 Disable access to GrContext if
  // enable_gles2_interface is disabled, after removing any dependencies on
  // GrContext in OOP-Raster.

  size_t max_resource_cache_bytes;
  size_t max_glyph_cache_texture_bytes;
  skia_bindings::GrContextForGLES2Interface::
      DetermineCacheLimitsFromAvailableMemory(&max_resource_cache_bytes,
                                              &max_glyph_cache_texture_bytes);

  gpu::gles2::GLES2Interface* gl_interface;
  if (trace_impl_)
    gl_interface = trace_impl_.get();
  else
    gl_interface = gles2_impl_.get();

  gr_context_.reset(new skia_bindings::GrContextForGLES2Interface(
      gl_interface, ContextSupport(), ContextCapabilities(),
      max_resource_cache_bytes, max_glyph_cache_texture_bytes));
  cache_controller_->SetGrContext(gr_context_->get());

  // If GlContext is already lost, also abandon the new GrContext.
  if (gr_context_->get() &&
      gles2_impl_->GetGraphicsResetStatusKHR() != GL_NO_ERROR)
    gr_context_->get()->abandonContext();

  return gr_context_->get();
}

viz::ContextCacheController* ContextProviderCommandBuffer::CacheController() {
  CheckValidThreadOrLockAcquired();
  return cache_controller_.get();
}

void ContextProviderCommandBuffer::SetDefaultTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> default_task_runner) {
  DCHECK(!bind_tried_);
  default_task_runner_ = std::move(default_task_runner);
}

base::Lock* ContextProviderCommandBuffer::GetLock() {
  if (!support_locking_)
    return nullptr;
  return &context_lock_;
}

const gpu::Capabilities& ContextProviderCommandBuffer::ContextCapabilities()
    const {
  DCHECK(bind_tried_);
  DCHECK_EQ(bind_result_, gpu::ContextResult::kSuccess);
  CheckValidThreadOrLockAcquired();
  // Skips past the trace_impl_ as it doesn't have capabilities.
  return impl_->capabilities();
}

const gpu::GpuFeatureInfo& ContextProviderCommandBuffer::GetGpuFeatureInfo()
    const {
  DCHECK(bind_tried_);
  DCHECK_EQ(bind_result_, gpu::ContextResult::kSuccess);
  CheckValidThreadOrLockAcquired();
  if (!command_buffer_ || !command_buffer_->channel()) {
    static const gpu::GpuFeatureInfo default_gpu_feature_info;
    return default_gpu_feature_info;
  }
  return command_buffer_->channel()->gpu_feature_info();
}

void ContextProviderCommandBuffer::OnLostContext() {
  CheckValidThreadOrLockAcquired();

  for (auto& observer : observers_)
    observer.OnContextLost();
  if (gr_context_)
    gr_context_->OnLostContext();

  gpu::CommandBuffer::State state = GetCommandBufferProxy()->GetLastState();
  command_buffer_metrics::UmaRecordContextLost(context_type_, state.error,
                                               state.context_lost_reason);
}

void ContextProviderCommandBuffer::AddObserver(viz::ContextLostObserver* obs) {
  CheckValidThreadOrLockAcquired();
  observers_.AddObserver(obs);
}

void ContextProviderCommandBuffer::RemoveObserver(
    viz::ContextLostObserver* obs) {
  CheckValidThreadOrLockAcquired();
  observers_.RemoveObserver(obs);
}

bool ContextProviderCommandBuffer::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  DCHECK(bind_tried_);
  DCHECK_EQ(bind_result_, gpu::ContextResult::kSuccess);

  base::Optional<base::AutoLock> hold;
  if (support_locking_)
    hold.emplace(context_lock_);

  impl_->OnMemoryDump(args, pmd);
  helper_->OnMemoryDump(args, pmd);

  if (gr_context_) {
    context_thread_checker_.DetachFromThread();
    SkiaGpuTraceMemoryDump trace_memory_dump(
        pmd, gles2_impl_->ShareGroupTracingGUID());
    gr_context_->get()->dumpMemoryStatistics(&trace_memory_dump);
    context_thread_checker_.DetachFromThread();
  }
  return true;
}

}  // namespace ui
