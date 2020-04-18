// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/skia_output_surface_impl.h"

#include "base/callback_helpers.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/resource_metadata.h"
#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "components/viz/service/display_embedder/skia_output_surface_impl_on_gpu.h"
#include "components/viz/service/display_embedder/viz_process_context_provider.h"
#include "components/viz/service/gl/gpu_service_impl.h"
#include "gpu/command_buffer/common/swap_buffers_complete_params.h"
#include "gpu/command_buffer/service/scheduler.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_gl_api_implementation.h"

namespace viz {

namespace {

template <typename... Args>
void PostAsyncTask(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const base::RepeatingCallback<void(Args...)>& callback,
    Args... args) {
  task_runner->PostTask(FROM_HERE, base::BindOnce(callback, args...));
}

template <typename... Args>
base::RepeatingCallback<void(Args...)> CreateSafeCallback(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const base::RepeatingCallback<void(Args...)>& callback) {
  return base::BindRepeating(&PostAsyncTask<Args...>, task_runner, callback);
}

}  // namespace

// A helper class for fullfilling promise image on the GPU thread.
template <class FullfillContextType>
class SkiaOutputSurfaceImpl::PromiseTextureHelper {
 public:
  using HelperType = PromiseTextureHelper<FullfillContextType>;

  PromiseTextureHelper(base::WeakPtr<SkiaOutputSurfaceImplOnGpu> impl_on_gpu,
                       FullfillContextType context)
      : impl_on_gpu_(impl_on_gpu), context_(std::move(context)) {}
  ~PromiseTextureHelper() = default;

  static sk_sp<SkImage> MakePromiseSkImage(
      SkiaOutputSurfaceImpl* impl,
      SkDeferredDisplayListRecorder* recorder,
      const GrBackendFormat& backend_format,
      gfx::Size size,
      GrMipMapped mip_mapped,
      GrSurfaceOrigin origin,
      SkColorType color_type,
      SkAlphaType alpha_type,
      sk_sp<SkColorSpace> color_space,
      FullfillContextType context) {
    DCHECK_CALLED_ON_VALID_THREAD(impl->thread_checker_);
    auto helper = std::make_unique<HelperType>(impl->impl_on_gpu_->weak_ptr(),
                                               std::move(context));
    auto image = recorder->makePromiseTexture(
        backend_format, size.width(), size.height(), mip_mapped, origin,
        color_type, alpha_type, color_space, HelperType::Fullfill,
        HelperType::Release, HelperType::Done, helper.get());
    if (image) {
      helper->Init(impl);
      helper.release();
    }
    return image;
  }

 private:
  void Init(SkiaOutputSurfaceImpl* impl);

  static void Fullfill(void* texture_context,
                       GrBackendTexture* backend_texture) {
    DCHECK(texture_context);
    auto* helper = static_cast<HelperType*>(texture_context);
    // The fullfill is always called by SkiaOutputSurfaceImplOnGpu::SwapBuffers
    // or SkiaOutputSurfaceImplOnGpu::FinishPaintRenderPass, so impl_on_gpu_
    // should be always valid.
    DCHECK(helper->impl_on_gpu_);
    helper->impl_on_gpu_->FullfillPromiseTexture(helper->context_,
                                                 backend_texture);
  }

  static void Release(void* texture_context) { DCHECK(texture_context); }

  static void Done(void* texture_context) {
    DCHECK(texture_context);
    std::unique_ptr<HelperType> helper(
        static_cast<HelperType*>(texture_context));
  }

  base::WeakPtr<SkiaOutputSurfaceImplOnGpu> impl_on_gpu_;

  // The data for calling the fullfill methods in SkiaOutputSurfaceImpl.
  FullfillContextType context_;

  DISALLOW_COPY_AND_ASSIGN(PromiseTextureHelper);
};

template <class T>
void SkiaOutputSurfaceImpl::PromiseTextureHelper<T>::Init(
    SkiaOutputSurfaceImpl* impl) {}

// For YUVResourceMetadata, we need to record the |context_| pointer in
// |impl->yuv_resource_metadatas_|, because we have to create SkImage from YUV
// textures before drawing the ddl to a SKSurface.
// TODO(penghuang): Remove this hack when Skia supports drawing YUV textures
// directly.
template <>
void SkiaOutputSurfaceImpl::PromiseTextureHelper<YUVResourceMetadata>::Init(
    SkiaOutputSurfaceImpl* impl) {
  impl->yuv_resource_metadatas_.push_back(&context_);
}

SkiaOutputSurfaceImpl::SkiaOutputSurfaceImpl(
    GpuServiceImpl* gpu_service,
    gpu::SurfaceHandle surface_handle,
    scoped_refptr<VizProcessContextProvider> context_provider,
    SyntheticBeginFrameSource* synthetic_begin_frame_source)
    : SkiaOutputSurface(context_provider),
      gpu_service_(gpu_service),
      surface_handle_(surface_handle),
      synthetic_begin_frame_source_(synthetic_begin_frame_source),
      weak_ptr_factory_(this) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

SkiaOutputSurfaceImpl::~SkiaOutputSurfaceImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  recorder_ = nullptr;
  // Use GPU scheduler to release impl_on_gpu_ on the GPU thread, so all
  // scheduled tasks for the impl_on_gpu_ will be executed, before releasing
  // it. The GPU thread is the main thread of the viz process. It outlives the
  // compositor thread. We don't need worry about it for now.
  auto sequence_id = gpu_service_->skia_output_surface_sequence_id();
  auto callback =
      base::BindOnce([](std::unique_ptr<SkiaOutputSurfaceImplOnGpu>) {},
                     std::move(impl_on_gpu_));
  gpu_service_->scheduler()->ScheduleTask(gpu::Scheduler::Task(
      sequence_id, std::move(callback), std::vector<gpu::SyncToken>()));
}

void SkiaOutputSurfaceImpl::BindToClient(OutputSurfaceClient* client) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client);
  DCHECK(!client_);

  client_ = client;
  weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
  client_thread_task_runner_ = base::ThreadTaskRunnerHandle::Get();

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  auto sequence_id = gpu_service_->skia_output_surface_sequence_id();
  auto callback = base::BindOnce(&SkiaOutputSurfaceImpl::InitializeOnGpuThread,
                                 base::Unretained(this), &event);
  gpu_service_->scheduler()->ScheduleTask(gpu::Scheduler::Task(
      sequence_id, std::move(callback), std::vector<gpu::SyncToken>()));
  event.Wait();
}

void SkiaOutputSurfaceImpl::EnsureBackbuffer() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
}

void SkiaOutputSurfaceImpl::DiscardBackbuffer() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
}

void SkiaOutputSurfaceImpl::BindFramebuffer() {
  // TODO(penghuang): remove this method when GLRenderer is removed.
}

void SkiaOutputSurfaceImpl::SetDrawRectangle(const gfx::Rect& draw_rectangle) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
}

void SkiaOutputSurfaceImpl::Reshape(const gfx::Size& size,
                                    float device_scale_factor,
                                    const gfx::ColorSpace& color_space,
                                    bool has_alpha,
                                    bool use_stencil) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  recorder_ = nullptr;
  SkSurfaceCharacterization* characterization = nullptr;
  std::unique_ptr<base::WaitableEvent> event;
  if (characterization_.isValid()) {
    characterization_ =
        characterization_.createResized(size.width(), size.height());
  } else {
    characterization = &characterization_;
    // TODO(penghuang): avoid blocking compositor thread.
    // We don't have a valid surface characterization, so we have to wait
    // until reshape is finished on Gpu thread.
    event = std::make_unique<base::WaitableEvent>(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
  }

  auto sequence_id = gpu_service_->skia_output_surface_sequence_id();
  // impl_on_gpu_ is released on the GPU thread by a posted task from
  // SkiaOutputSurfaceImpl::dtor. So it is safe to use base::Unretained.
  auto callback = base::BindOnce(&SkiaOutputSurfaceImplOnGpu::Reshape,
                                 base::Unretained(impl_on_gpu_.get()), size,
                                 device_scale_factor, color_space, has_alpha,
                                 use_stencil, characterization, event.get());
  gpu_service_->scheduler()->ScheduleTask(gpu::Scheduler::Task(
      sequence_id, std::move(callback), std::vector<gpu::SyncToken>()));

  if (event)
    event->Wait();
  RecreateRecorder();
}

void SkiaOutputSurfaceImpl::SwapBuffers(OutputSurfaceFrame frame) {
  NOTREACHED();
}

uint32_t SkiaOutputSurfaceImpl::GetFramebufferCopyTextureFormat() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  return GL_RGB;
}

OverlayCandidateValidator* SkiaOutputSurfaceImpl::GetOverlayCandidateValidator()
    const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return nullptr;
}

bool SkiaOutputSurfaceImpl::IsDisplayedAsOverlayPlane() const {
  return false;
}

unsigned SkiaOutputSurfaceImpl::GetOverlayTextureId() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return 0;
}

gfx::BufferFormat SkiaOutputSurfaceImpl::GetOverlayBufferFormat() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return gfx::BufferFormat::RGBX_8888;
}

bool SkiaOutputSurfaceImpl::HasExternalStencilTest() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  return false;
}

void SkiaOutputSurfaceImpl::ApplyExternalStencil() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

#if BUILDFLAG(ENABLE_VULKAN)
gpu::VulkanSurface* SkiaOutputSurfaceImpl::GetVulkanSurface() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  return nullptr;
}
#endif

unsigned SkiaOutputSurfaceImpl::UpdateGpuFence() {
  return 0;
}

SkCanvas* SkiaOutputSurfaceImpl::GetSkCanvasForCurrentFrame() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(recorder_);
  DCHECK_EQ(current_render_pass_id_, 0u);

  return recorder_->getCanvas();
}

sk_sp<SkImage> SkiaOutputSurfaceImpl::MakePromiseSkImage(
    ResourceMetadata metadata) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(recorder_);

  // Convert internal format from GLES2 to platform GL.
  const auto* version_info = gpu_service_->context_for_skia()->GetVersionInfo();
  metadata.backend_format = GrBackendFormat::MakeGL(
      gl::GetInternalFormat(version_info,
                            *metadata.backend_format.getGLFormat()),
      *metadata.backend_format.getGLTarget());

  DCHECK(!metadata.mailbox.IsZero());
  resource_sync_tokens_.push_back(metadata.sync_token);

  return PromiseTextureHelper<ResourceMetadata>::MakePromiseSkImage(
      this, recorder_.get(), metadata.backend_format, metadata.size,
      metadata.mip_mapped, metadata.origin, metadata.color_type,
      metadata.alpha_type, metadata.color_space, std::move(metadata));
}

sk_sp<SkImage> SkiaOutputSurfaceImpl::MakePromiseSkImageFromYUV(
    std::vector<ResourceMetadata> metadatas,
    SkYUVColorSpace yuv_color_space) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(recorder_);

  DCHECK(metadatas.size() == 2 || metadatas.size() == 3);

  // TODO(penghuang): Create SkImage from YUV textures directly when it is
  // supported by Skia.
  YUVResourceMetadata yuv_metadata(std::move(metadatas), yuv_color_space);

  // Convert internal format from GLES2 to platform GL.
  const auto* version_info = gpu_service_->context_for_skia()->GetVersionInfo();
  auto backend_format = GrBackendFormat::MakeGL(
      gl::GetInternalFormat(version_info, GL_BGRA8_EXT), GL_TEXTURE_2D);

  return PromiseTextureHelper<YUVResourceMetadata>::MakePromiseSkImage(
      this, recorder_.get(), backend_format, yuv_metadata.size(),
      GrMipMapped::kNo, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType,
      kPremul_SkAlphaType, nullptr /* color_space */, std::move(yuv_metadata));
}

gpu::SyncToken SkiaOutputSurfaceImpl::SkiaSwapBuffers(
    OutputSurfaceFrame frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(recorder_);

  gpu::SyncToken sync_token(gpu::CommandBufferNamespace::VIZ_OUTPUT_SURFACE,
                            impl_on_gpu_->command_buffer_id(),
                            ++sync_fence_release_);
  sync_token.SetVerifyFlush();

  auto ddl = recorder_->detach();
  DCHECK(ddl);
  RecreateRecorder();
  auto sequence_id = gpu_service_->skia_output_surface_sequence_id();
  // impl_on_gpu_ is released on the GPU thread by a posted task from
  // SkiaOutputSurfaceImpl::dtor. So it is safe to use base::Unretained.
  auto callback = base::BindOnce(
      &SkiaOutputSurfaceImplOnGpu::SwapBuffers,
      base::Unretained(impl_on_gpu_.get()), std::move(frame), std::move(ddl),
      std::move(yuv_resource_metadatas_), sync_fence_release_);
  gpu_service_->scheduler()->ScheduleTask(gpu::Scheduler::Task(
      sequence_id, std::move(callback), std::move(resource_sync_tokens_)));
  return sync_token;
}

SkCanvas* SkiaOutputSurfaceImpl::BeginPaintRenderPass(
    const RenderPassId& id,
    const gfx::Size& surface_size,
    ResourceFormat format,
    bool mipmap) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(gpu_service_->gr_context());
  DCHECK(!current_render_pass_id_);
  DCHECK(!offscreen_surface_recorder_);
  DCHECK(resource_sync_tokens_.empty());

  current_render_pass_id_ = id;

  auto gr_context_thread_safe = gpu_service_->gr_context()->threadSafeProxy();
  constexpr uint32_t flags = 0;
  // LegacyFontHost will get LCD text and skia figures out what type to use.
  SkSurfaceProps surface_props(flags, SkSurfaceProps::kLegacyFontHost_InitType);
  int msaa_sample_count = 0;
  SkColorType color_type =
      ResourceFormatToClosestSkColorType(true /*gpu_compositing */, format);
  SkImageInfo image_info =
      SkImageInfo::Make(surface_size.width(), surface_size.height(), color_type,
                        kPremul_SkAlphaType, nullptr /* color_space */);

  // TODO(penghuang): Figure out how to choose the right size.
  constexpr size_t kCacheMaxResourceBytes = 90 * 1024 * 1024;
  const auto* version_info = gpu_service_->context_for_skia()->GetVersionInfo();
  unsigned int texture_storage_format = TextureStorageFormat(format);
  auto backend_format = GrBackendFormat::MakeGL(
      gl::GetInternalFormat(version_info, texture_storage_format),
      GL_TEXTURE_2D);
  auto characterization = gr_context_thread_safe->createCharacterization(
      kCacheMaxResourceBytes, image_info, backend_format, msaa_sample_count,
      kTopLeft_GrSurfaceOrigin, surface_props, mipmap);
  DCHECK(characterization.isValid());
  offscreen_surface_recorder_ =
      std::make_unique<SkDeferredDisplayListRecorder>(characterization);
  return offscreen_surface_recorder_->getCanvas();
}

gpu::SyncToken SkiaOutputSurfaceImpl::FinishPaintRenderPass() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(gpu_service_->gr_context());
  DCHECK(current_render_pass_id_);
  DCHECK(offscreen_surface_recorder_);

  gpu::SyncToken sync_token(gpu::CommandBufferNamespace::VIZ_OUTPUT_SURFACE,
                            impl_on_gpu_->command_buffer_id(),
                            ++sync_fence_release_);
  sync_token.SetVerifyFlush();

  auto ddl = offscreen_surface_recorder_->detach();
  offscreen_surface_recorder_ = nullptr;
  DCHECK(ddl);

  auto sequence_id = gpu_service_->skia_output_surface_sequence_id();
  // impl_on_gpu_ is released on the GPU thread by a posted task from
  // SkiaOutputSurfaceImpl::dtor. So it is safe to use base::Unretained.
  auto callback = base::BindOnce(
      &SkiaOutputSurfaceImplOnGpu::FinishPaintRenderPass,
      base::Unretained(impl_on_gpu_.get()), current_render_pass_id_,
      std::move(ddl), std::move(yuv_resource_metadatas_), sync_fence_release_);
  gpu_service_->scheduler()->ScheduleTask(gpu::Scheduler::Task(
      sequence_id, std::move(callback), std::move(resource_sync_tokens_)));
  current_render_pass_id_ = 0;
  return sync_token;
}

sk_sp<SkImage> SkiaOutputSurfaceImpl::MakePromiseSkImageFromRenderPass(
    const RenderPassId& id,
    const gfx::Size& size,
    ResourceFormat format,
    bool mipmap) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(recorder_);

  // Convert internal format from GLES2 to platform GL.
  const auto* version_info = gpu_service_->context_for_skia()->GetVersionInfo();
  unsigned int texture_storage_format = TextureStorageFormat(format);
  auto backend_format = GrBackendFormat::MakeGL(
      gl::GetInternalFormat(version_info, texture_storage_format),
      GL_TEXTURE_2D);
  SkColorType color_type =
      ResourceFormatToClosestSkColorType(true /*gpu_compositing */, format);
  return PromiseTextureHelper<RenderPassId>::MakePromiseSkImage(
      this, recorder_.get(), backend_format, size,
      mipmap ? GrMipMapped::kYes : GrMipMapped::kNo, kTopLeft_GrSurfaceOrigin,
      color_type, kPremul_SkAlphaType, nullptr /* color_space */, id);
}

void SkiaOutputSurfaceImpl::RemoveRenderPassResource(
    std::vector<RenderPassId> ids) {
  DCHECK(!ids.empty());
  auto sequence_id = gpu_service_->skia_output_surface_sequence_id();
  // impl_on_gpu_ is released on the GPU thread by a posted task from
  // SkiaOutputSurfaceImpl::dtor. So it is safe to use base::Unretained.
  auto callback =
      base::BindOnce(&SkiaOutputSurfaceImplOnGpu::RemoveRenderPassResource,
                     base::Unretained(impl_on_gpu_.get()), std::move(ids));
  gpu_service_->scheduler()->ScheduleTask(gpu::Scheduler::Task(
      sequence_id, std::move(callback), std::vector<gpu::SyncToken>()));
}

void SkiaOutputSurfaceImpl::InitializeOnGpuThread(base::WaitableEvent* event) {
  base::ScopedClosureRunner scoped_runner(
      base::BindOnce(&base::WaitableEvent::Signal, base::Unretained(event)));
  auto did_swap_buffer_complete_callback = base::BindRepeating(
      &SkiaOutputSurfaceImpl::DidSwapBuffersComplete, weak_ptr_);
  auto buffer_presented_callback =
      base::BindRepeating(&SkiaOutputSurfaceImpl::BufferPresented, weak_ptr_);
  impl_on_gpu_ = std::make_unique<SkiaOutputSurfaceImplOnGpu>(
      gpu_service_, surface_handle_,
      CreateSafeCallback(client_thread_task_runner_,
                         did_swap_buffer_complete_callback),
      CreateSafeCallback(client_thread_task_runner_,
                         buffer_presented_callback));
  capabilities_ = impl_on_gpu_->capabilities();
}

void SkiaOutputSurfaceImpl::RecreateRecorder() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(characterization_.isValid());
  recorder_ =
      std::make_unique<SkDeferredDisplayListRecorder>(characterization_);
  // TODO(penghuang): remove the unnecessary getCanvas() call, when the
  // recorder crash is fixed in skia.
  recorder_->getCanvas();
}

void SkiaOutputSurfaceImpl::DidSwapBuffersComplete(
    gpu::SwapBuffersCompleteParams params) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client_);

  if (!params.texture_in_use_responses.empty())
    client_->DidReceiveTextureInUseResponses(params.texture_in_use_responses);
  if (!params.ca_layer_params.is_empty)
    client_->DidReceiveCALayerParams(params.ca_layer_params);
  client_->DidReceiveSwapBuffersAck();
}

void SkiaOutputSurfaceImpl::BufferPresented(
    const gfx::PresentationFeedback& feedback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client_);
  client_->DidReceivePresentationFeedback(feedback);
  if (synthetic_begin_frame_source_ &&
      feedback.flags & gfx::PresentationFeedback::kVSync) {
    // TODO(brianderson): We should not be receiving 0 intervals.
    synthetic_begin_frame_source_->OnUpdateVSyncParameters(
        feedback.timestamp, feedback.interval.is_zero()
                                ? BeginFrameArgs::DefaultInterval()
                                : feedback.interval);
  }
}

}  // namespace viz
