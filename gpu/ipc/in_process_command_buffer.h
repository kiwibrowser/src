// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_IN_PROCESS_COMMAND_BUFFER_H_
#define GPU_IPC_IN_PROCESS_COMMAND_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "gpu/command_buffer/client/gpu_control.h"
#include "gpu/command_buffer/common/activity_flags.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "gpu/command_buffer/common/context_result.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/decoder_client.h"
#include "gpu/command_buffer/service/decoder_context.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/service/service_discardable_manager.h"
#include "gpu/command_buffer/service/service_transfer_cache.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/ipc/gl_in_process_context_export.h"
#include "gpu/ipc/service/image_transport_surface_delegate.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gpu_preference.h"

namespace base {
class SequenceChecker;
}

namespace gl {
class GLContext;
class GLShareGroup;
class GLSurface;
}

namespace gfx {
struct GpuFenceHandle;
class Size;
}

namespace gpu {

struct ContextCreationAttribs;
class MailboxManager;
class ServiceDiscardableManager;
class SyncPointClientState;
class SyncPointOrderData;
class SyncPointManager;
struct SwapBuffersCompleteParams;

namespace gles2 {
class FramebufferCompletenessCache;
class Outputter;
class ProgramCache;
class ShaderTranslatorCache;
}

class GpuChannelManagerDelegate;
class GpuMemoryBufferManager;
class ImageFactory;
class TransferBufferManager;

// This class provides a thread-safe interface to the global GPU service (for
// example GPU thread) when being run in single process mode.
// However, the behavior for accessing one context (i.e. one instance of this
// class) from different client threads is undefined.
class GL_IN_PROCESS_CONTEXT_EXPORT InProcessCommandBuffer
    : public CommandBuffer,
      public GpuControl,
      public CommandBufferServiceClient,
      public DecoderClient,
      public ImageTransportSurfaceDelegate {
 public:
  class Service;

  explicit InProcessCommandBuffer(const scoped_refptr<Service>& service);
  ~InProcessCommandBuffer() override;

  // If |surface| is not null, use it directly; in this case, the command
  // buffer gpu thread must be the same as the client thread. Otherwise create
  // a new GLSurface.
  // |gpu_channel_manager_delegate| should be non-null when the command buffer
  // is used in the GPU process for compositor to gpu thread communication.
  gpu::ContextResult Initialize(
      scoped_refptr<gl::GLSurface> surface,
      bool is_offscreen,
      SurfaceHandle window,
      const ContextCreationAttribs& attribs,
      InProcessCommandBuffer* share_group,
      GpuMemoryBufferManager* gpu_memory_buffer_manager,
      ImageFactory* image_factory,
      GpuChannelManagerDelegate* gpu_channel_manager_delegate,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  // CommandBuffer implementation:
  State GetLastState() override;
  void Flush(int32_t put_offset) override;
  void OrderingBarrier(int32_t put_offset) override;
  State WaitForTokenInRange(int32_t start, int32_t end) override;
  State WaitForGetOffsetInRange(uint32_t set_get_buffer_count,
                                int32_t start,
                                int32_t end) override;
  void SetGetBuffer(int32_t shm_id) override;
  scoped_refptr<Buffer> CreateTransferBuffer(size_t size, int32_t* id) override;
  void DestroyTransferBuffer(int32_t id) override;

  // GpuControl implementation:
  // NOTE: The GpuControlClient will be called on the client thread.
  void SetGpuControlClient(GpuControlClient*) override;
  const Capabilities& GetCapabilities() const override;
  int32_t CreateImage(ClientBuffer buffer,
                      size_t width,
                      size_t height,
                      unsigned internalformat) override;
  void DestroyImage(int32_t id) override;
  void SignalQuery(uint32_t query_id, base::OnceClosure callback) override;
  void CreateGpuFence(uint32_t gpu_fence_id, ClientGpuFence source) override;
  void GetGpuFence(uint32_t gpu_fence_id,
                   base::OnceCallback<void(std::unique_ptr<gfx::GpuFence>)>
                       callback) override;
  void SetLock(base::Lock*) override;
  void EnsureWorkVisible() override;
  CommandBufferNamespace GetNamespaceID() const override;
  CommandBufferId GetCommandBufferID() const override;
  void FlushPendingWork() override;
  uint64_t GenerateFenceSyncRelease() override;
  bool IsFenceSyncReleased(uint64_t release) override;
  void SignalSyncToken(const SyncToken& sync_token,
                       base::OnceClosure callback) override;
  void WaitSyncTokenHint(const SyncToken& sync_token) override;
  bool CanWaitUnverifiedSyncToken(const SyncToken& sync_token) override;
  void SetSnapshotRequested() override;

  // CommandBufferServiceClient implementation:
  CommandBatchProcessedResult OnCommandBatchProcessed() override;
  void OnParseError() override;

  // DecoderClient implementation:
  void OnConsoleMessage(int32_t id, const std::string& message) override;
  void CacheShader(const std::string& key, const std::string& shader) override;
  void OnFenceSyncRelease(uint64_t release) override;
  bool OnWaitSyncToken(const SyncToken& sync_token) override;
  void OnDescheduleUntilFinished() override;
  void OnRescheduleAfterFinished() override;
  void OnSwapBuffers(uint64_t swap_id, uint32_t flags) override;

// ImageTransportSurfaceDelegate implementation:
#if defined(OS_WIN)
  void DidCreateAcceleratedSurfaceChildWindow(
      SurfaceHandle parent_window,
      SurfaceHandle child_window) override;
#endif
  void DidSwapBuffersComplete(SwapBuffersCompleteParams params) override;
  const gles2::FeatureInfo* GetFeatureInfo() const override;
  const GpuPreferences& GetGpuPreferences() const override;

  void SetSnapshotRequestedCallback(const base::Closure& callback) override;
  void BufferPresented(const gfx::PresentationFeedback& feedback) override;

  void AddFilter(IPC::MessageFilter* message_filter) override;
  int32_t GetRouteID() const override;

  // Upstream this function to GpuControl if needs arise.
  const GpuFeatureInfo& GetGpuFeatureInfo() const;

  using UpdateVSyncParametersCallback =
      base::Callback<void(base::TimeTicks timebase, base::TimeDelta interval)>;
  void SetUpdateVSyncParametersCallback(
      const UpdateVSyncParametersCallback& callback);

  void DidSwapBuffersCompleteOnOriginThread(SwapBuffersCompleteParams params);
  void BufferPresentedOnOriginThread(uint64_t swap_id,
                                     uint32_t flags,
                                     const gfx::PresentationFeedback& feedback);

  // Mostly the GpuFeatureInfo from GpuInit will be used to create a gpu thread
  // service. In certain tests GpuInit is not part of the execution path, so
  // the test suite need to compute it and pass it to the default service.
  // See "gpu/ipc/in_process_command_buffer.cc".
  static void InitializeDefaultServiceForTesting(
      const GpuFeatureInfo& gpu_feature_info);

  gpu::ServiceTransferCache* GetTransferCacheForTest() const;

  static const int kGpuMemoryBufferClientId;

  // The serializer interface to the GPU service (i.e. thread).
  class Service {
   public:
    Service(const GpuPreferences& gpu_preferences,
            MailboxManager* mailbox_manager,
            scoped_refptr<gl::GLShareGroup> share_group,
            const GpuFeatureInfo& gpu_feature_info);

    virtual ~Service();

    virtual void AddRef() const = 0;
    virtual void Release() const = 0;

    // Queues a task to run as soon as possible.
    virtual void ScheduleTask(base::OnceClosure task) = 0;

    // Schedules |callback| to run at an appropriate time for performing delayed
    // work.
    virtual void ScheduleDelayedWork(base::OnceClosure task) = 0;

    virtual bool ForceVirtualizedGLContexts() = 0;
    virtual SyncPointManager* sync_point_manager() = 0;
    virtual bool BlockThreadOnWaitSyncToken() const = 0;

    const GpuPreferences& gpu_preferences();
    const GpuFeatureInfo& gpu_feature_info() { return gpu_feature_info_; }
    scoped_refptr<gl::GLShareGroup> share_group();
    MailboxManager* mailbox_manager() { return mailbox_manager_; }
    gles2::Outputter* outputter();
    gles2::ProgramCache* program_cache();
    gles2::ImageManager* image_manager() { return &image_manager_; }
    ServiceDiscardableManager* discardable_manager() {
      return &discardable_manager_;
    }
    gles2::ShaderTranslatorCache* shader_translator_cache() {
      return &shader_translator_cache_;
    }
    gles2::FramebufferCompletenessCache* framebuffer_completeness_cache() {
      return &framebuffer_completeness_cache_;
    }

   protected:
    const GpuPreferences gpu_preferences_;
    const GpuFeatureInfo gpu_feature_info_;
    std::unique_ptr<MailboxManager> owned_mailbox_manager_;
    MailboxManager* mailbox_manager_ = nullptr;
    std::unique_ptr<gles2::Outputter> outputter_;
    scoped_refptr<gl::GLShareGroup> share_group_;
    std::unique_ptr<gles2::ProgramCache> program_cache_;
    // No-op default initialization is used in in-process mode.
    GpuProcessActivityFlags activity_flags_;
    gles2::ImageManager image_manager_;
    ServiceDiscardableManager discardable_manager_;
    gles2::ShaderTranslatorCache shader_translator_cache_;
    gles2::FramebufferCompletenessCache framebuffer_completeness_cache_;
  };

 private:
  struct InitializeOnGpuThreadParams {
    bool is_offscreen;
    SurfaceHandle window;
    const ContextCreationAttribs& attribs;
    Capabilities* capabilities;  // Ouptut.
    InProcessCommandBuffer* share_command_buffer;
    ImageFactory* image_factory;

    InitializeOnGpuThreadParams(bool is_offscreen,
                                SurfaceHandle window,
                                const ContextCreationAttribs& attribs,
                                Capabilities* capabilities,
                                InProcessCommandBuffer* share_command_buffer,
                                ImageFactory* image_factory)
        : is_offscreen(is_offscreen),
          window(window),
          attribs(attribs),
          capabilities(capabilities),
          share_command_buffer(share_command_buffer),
          image_factory(image_factory) {}
  };

  gpu::ContextResult InitializeOnGpuThread(
      const InitializeOnGpuThreadParams& params);
  void Destroy();
  bool DestroyOnGpuThread();
  void FlushOnGpuThread(int32_t put_offset, bool snapshot_requested);
  void UpdateLastStateOnGpuThread();
  void ScheduleDelayedWorkOnGpuThread();
  bool MakeCurrent();
  base::OnceClosure WrapCallback(base::OnceClosure callback);

  void QueueOnceTask(bool out_of_order, base::OnceClosure task);
  void QueueRepeatableTask(base::RepeatingClosure task);

  void ProcessTasksOnGpuThread();
  void CheckSequencedThread();
  void OnWaitSyncTokenCompleted(const SyncToken& sync_token);
  void SignalSyncTokenOnGpuThread(const SyncToken& sync_token,
                                  base::OnceClosure callback);
  void SignalQueryOnGpuThread(unsigned query_id, base::OnceClosure callback);
  void DestroyTransferBufferOnGpuThread(int32_t id);
  void CreateImageOnGpuThread(int32_t id,
                              const gfx::GpuMemoryBufferHandle& handle,
                              const gfx::Size& size,
                              gfx::BufferFormat format,
                              uint32_t internalformat,
                              // uint32_t order_num,
                              uint64_t fence_sync);
  void DestroyImageOnGpuThread(int32_t id);
  void SetGetBufferOnGpuThread(int32_t shm_id, base::WaitableEvent* completion);
  void CreateGpuFenceOnGpuThread(uint32_t gpu_fence_id,
                                 const gfx::GpuFenceHandle& handle);
  void GetGpuFenceOnGpuThread(
      uint32_t gpu_fence_id,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      base::OnceCallback<void(std::unique_ptr<gfx::GpuFence>)> callback);

  // Callbacks on the gpu thread.
  void PerformDelayedWorkOnGpuThread();
  // Callback implementations on the client thread.
  void OnContextLost();

  const CommandBufferId command_buffer_id_;

  // Members accessed on the gpu thread (possibly with the exception of
  // creation):
  bool waiting_for_sync_point_ = false;
  bool use_virtualized_gl_context_ = false;

  scoped_refptr<base::SingleThreadTaskRunner> origin_task_runner_;
  std::unique_ptr<TransferBufferManager> transfer_buffer_manager_;
  std::unique_ptr<DecoderContext> decoder_;
  scoped_refptr<gl::GLContext> context_;
  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<SyncPointOrderData> sync_point_order_data_;
  scoped_refptr<SyncPointClientState> sync_point_client_state_;
  base::Closure context_lost_callback_;
  // Used to throttle PerformDelayedWorkOnGpuThread.
  bool delayed_work_pending_ = false;
  ImageFactory* image_factory_ = nullptr;
  GpuChannelManagerDelegate* gpu_channel_manager_delegate_ = nullptr;

  base::Closure snapshot_requested_callback_;
  bool snapshot_requested_ = false;

  // Members accessed on the client thread:
  GpuControlClient* gpu_control_client_ = nullptr;
#if DCHECK_IS_ON()
  bool context_lost_ = false;
#endif
  State last_state_;
  base::Lock last_state_lock_;
  int32_t last_put_offset_ = -1;
  Capabilities capabilities_;
  GpuMemoryBufferManager* gpu_memory_buffer_manager_ = nullptr;
  uint64_t next_fence_sync_release_ = 1;
  uint64_t flushed_fence_sync_release_ = 0;

  // Accessed on both threads:
  std::unique_ptr<CommandBufferService> command_buffer_;
  base::Lock command_buffer_lock_;
  base::WaitableEvent flush_event_;
  scoped_refptr<Service> service_;

  // The group of contexts that share namespaces with this context.
  scoped_refptr<gles2::ContextGroup> context_group_;

  scoped_refptr<gl::GLShareGroup> gl_share_group_;
  base::WaitableEvent fence_sync_wait_event_;

  // Only used with explicit scheduling and the gpu thread is the same as
  // the client thread.
  std::unique_ptr<base::SequenceChecker> sequence_checker_;

  base::Lock task_queue_lock_;
  class GpuTask {
   public:
    GpuTask(base::OnceClosure callback, uint32_t order_number);
    GpuTask(base::RepeatingClosure callback, uint32_t order_number);
    ~GpuTask();

    uint32_t order_number() { return order_number_; }
    bool is_repeatable() { return !!repeating_closure_; }
    void Run();

   private:
    base::OnceClosure once_closure_;
    base::RepeatingClosure repeating_closure_;
    uint32_t order_number_;

    DISALLOW_COPY_AND_ASSIGN(GpuTask);
  };
  base::queue<std::unique_ptr<GpuTask>> task_queue_;

  UpdateVSyncParametersCallback update_vsync_parameters_completion_callback_;

  // Params pushed each time we call OnSwapBuffers, and popped when a buffer
  // is presented or a swap completed.
  struct SwapBufferParams {
    uint64_t swap_id;
    uint32_t flags;
  };
  base::circular_deque<SwapBufferParams> pending_presented_params_;
  base::circular_deque<SwapBufferParams> pending_swap_completed_params_;

  base::WeakPtr<InProcessCommandBuffer> client_thread_weak_ptr_;
  base::WeakPtr<InProcessCommandBuffer> gpu_thread_weak_ptr_;
  base::WeakPtrFactory<InProcessCommandBuffer> client_thread_weak_ptr_factory_;
  base::WeakPtrFactory<InProcessCommandBuffer> gpu_thread_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InProcessCommandBuffer);
};

}  // namespace gpu

#endif  // GPU_IPC_IN_PROCESS_COMMAND_BUFFER_H_
