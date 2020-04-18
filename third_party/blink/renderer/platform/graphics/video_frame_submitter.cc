// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/video_frame_submitter.h"

#include "base/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/filter_operations.h"
#include "cc/resources/video_resource_updater.h"
#include "cc/scheduler/video_frame_controller.h"
#include "components/viz/common/resources/resource_id.h"
#include "components/viz/common/resources/returned_resource.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "media/base/video_frame.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom-blink.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/modules/frame_sinks/embedded_frame_sink.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"

namespace blink {

namespace {

// TODO(danakj): One day the gpu::mojom::Mailbox type should be shared with
// blink directly and we won't need to use gpu::mojom::blink::Mailbox, nor the
// conversion through WTF::Vector.
gpu::mojom::blink::MailboxPtr SharedBitmapIdToGpuMailboxPtr(
    const viz::SharedBitmapId& id) {
  WTF::Vector<int8_t> name(GL_MAILBOX_SIZE_CHROMIUM);
  for (int i = 0; i < GL_MAILBOX_SIZE_CHROMIUM; ++i)
    name[i] = id.name[i];
  return {base::in_place, name};
}

// Delay to retry getting the context_provider.
constexpr int kGetContextProviderRetryMS = 150;

}  // namespace

VideoFrameSubmitter::VideoFrameSubmitter(
    WebContextProviderCallback context_provider_callback,
    std::unique_ptr<VideoFrameResourceProvider> resource_provider)
    : binding_(this),
      context_provider_callback_(context_provider_callback),
      resource_provider_(std::move(resource_provider)),
      is_rendering_(false),
      weak_ptr_factory_(this) {
  DETACH_FROM_THREAD(media_thread_checker_);
}

VideoFrameSubmitter::~VideoFrameSubmitter() {
  if (context_provider_)
    context_provider_->RemoveObserver(this);
}

void VideoFrameSubmitter::SetRotation(media::VideoRotation rotation) {
  rotation_ = rotation;
}

void VideoFrameSubmitter::EnableSubmission(
    viz::FrameSinkId id,
    WebFrameSinkDestroyedCallback frame_sink_destroyed_callback) {
  // TODO(lethalantidote): Set these fields earlier in the constructor. Will
  // need to construct VideoFrameSubmitter later in order to do this.
  frame_sink_id_ = id;
  frame_sink_destroyed_callback_ = frame_sink_destroyed_callback;
  if (resource_provider_->IsInitialized())
    StartSubmitting();
}

void VideoFrameSubmitter::StopUsingProvider() {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  if (is_rendering_)
    StopRendering();
  provider_ = nullptr;
}

void VideoFrameSubmitter::StopRendering() {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  DCHECK(is_rendering_);
  DCHECK(provider_);

  if (compositor_frame_sink_) {
    // Push out final frame.
    SubmitSingleFrame();
    compositor_frame_sink_->SetNeedsBeginFrame(false);
  }
  is_rendering_ = false;
}

void VideoFrameSubmitter::SubmitSingleFrame() {
  // If we haven't gotten a valid result yet from |context_provider_callback_|
  // |resource_provider_| will remain uninitalized.
  if (!resource_provider_->IsInitialized())
    return;

  viz::BeginFrameAck current_begin_frame_ack =
      viz::BeginFrameAck::CreateManualAckWithDamage();
  scoped_refptr<media::VideoFrame> video_frame = provider_->GetCurrentFrame();
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&VideoFrameSubmitter::SubmitFrame,
                                weak_ptr_factory_.GetWeakPtr(),
                                current_begin_frame_ack, video_frame));
  provider_->PutCurrentFrame();
}

void VideoFrameSubmitter::DidReceiveFrame() {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  DCHECK(provider_);

  // DidReceiveFrame is called before renderering has started, as a part of
  // PaintSingleFrame.
  if (!is_rendering_) {
    SubmitSingleFrame();
  }
}

void VideoFrameSubmitter::StartRendering() {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  DCHECK(!is_rendering_);
  if (compositor_frame_sink_)
    compositor_frame_sink_->SetNeedsBeginFrame(true);
  is_rendering_ = true;
}

void VideoFrameSubmitter::Initialize(cc::VideoFrameProvider* provider) {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  if (provider) {
    DCHECK(!provider_);
    provider_ = provider;
    context_provider_callback_.Run(
        base::BindOnce(&VideoFrameSubmitter::OnReceivedContextProvider,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void VideoFrameSubmitter::OnReceivedContextProvider(
    bool use_gpu_compositing,
    scoped_refptr<ui::ContextProviderCommandBuffer> context_provider) {
  // We could get a null |context_provider| back if the context is still lost.
  if (!context_provider && use_gpu_compositing) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(
            context_provider_callback_,
            base::BindOnce(&VideoFrameSubmitter::OnReceivedContextProvider,
                           weak_ptr_factory_.GetWeakPtr())),
        base::TimeDelta::FromMilliseconds(kGetContextProviderRetryMS));
    return;
  }

  context_provider_ = std::move(context_provider);
  if (use_gpu_compositing) {
    context_provider_->AddObserver(this);
    resource_provider_->Initialize(context_provider_.get(), nullptr);
  } else {
    resource_provider_->Initialize(nullptr, this);
  }

  if (frame_sink_id_.is_valid())
    StartSubmitting();
}

void VideoFrameSubmitter::StartSubmitting() {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  DCHECK(frame_sink_id_.is_valid());

  mojom::blink::EmbeddedFrameSinkProviderPtr provider;
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&provider));

  viz::mojom::blink::CompositorFrameSinkClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  provider->CreateCompositorFrameSink(
      frame_sink_id_, std::move(client),
      mojo::MakeRequest(&compositor_frame_sink_));

  if (is_rendering_)
    compositor_frame_sink_->SetNeedsBeginFrame(true);

  scoped_refptr<media::VideoFrame> video_frame = provider_->GetCurrentFrame();
  if (video_frame) {
    viz::BeginFrameAck current_begin_frame_ack =
        viz::BeginFrameAck::CreateManualAckWithDamage();
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&VideoFrameSubmitter::SubmitFrame,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  current_begin_frame_ack, video_frame));
    provider_->PutCurrentFrame();
  }
}

void VideoFrameSubmitter::SubmitFrame(
    viz::BeginFrameAck begin_frame_ack,
    scoped_refptr<media::VideoFrame> video_frame) {
  TRACE_EVENT0("media", "VideoFrameSubmitter::SubmitFrame");
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  DCHECK(compositor_frame_sink_);

  viz::CompositorFrame compositor_frame;
  std::unique_ptr<viz::RenderPass> render_pass = viz::RenderPass::Create();

  render_pass->SetNew(1, gfx::Rect(video_frame->coded_size()),
                      gfx::Rect(video_frame->coded_size()), gfx::Transform());
  render_pass->filters = cc::FilterOperations();
  resource_provider_->AppendQuads(render_pass.get(), video_frame, rotation_);
  compositor_frame.metadata.begin_frame_ack = begin_frame_ack;
  compositor_frame.metadata.device_scale_factor = 1;
  compositor_frame.metadata.may_contain_video = true;

  std::vector<viz::ResourceId> resources;
  for (auto* quad : render_pass->quad_list) {
    for (viz::ResourceId resource_id : quad->resources) {
      resources.push_back(resource_id);
    }
  }
  resource_provider_->PrepareSendToParent(resources,
                                          &compositor_frame.resource_list);
  compositor_frame.render_pass_list.push_back(std::move(render_pass));

  if (compositor_frame.size_in_pixels() != current_size_in_pixels_) {
    parent_local_surface_id_allocator_.GenerateId();
    current_size_in_pixels_ = compositor_frame.size_in_pixels();
  }

  // TODO(lethalantidote): Address third/fourth arg in SubmitCompositorFrame.
  compositor_frame_sink_->SubmitCompositorFrame(
      parent_local_surface_id_allocator_.GetCurrentLocalSurfaceId(),
      std::move(compositor_frame), nullptr, 0);
  resource_provider_->ReleaseFrameResources();
}

void VideoFrameSubmitter::OnBeginFrame(const viz::BeginFrameArgs& args) {
  TRACE_EVENT0("media", "VideoFrameSubmitter::OnBeginFrame");
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  viz::BeginFrameAck current_begin_frame_ack =
      viz::BeginFrameAck(args.source_id, args.sequence_number, false);
  if (args.type == viz::BeginFrameArgs::MISSED) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack);
    return;
  }

  current_begin_frame_ack.has_damage = true;

  if (!provider_ ||
      !provider_->UpdateCurrentFrame(args.frame_time + args.interval,
                                     args.frame_time + 2 * args.interval) ||
      !is_rendering_) {
    compositor_frame_sink_->DidNotProduceFrame(current_begin_frame_ack);
    return;
  }

  scoped_refptr<media::VideoFrame> video_frame = provider_->GetCurrentFrame();

  SubmitFrame(current_begin_frame_ack, video_frame);

  provider_->PutCurrentFrame();
}

void VideoFrameSubmitter::OnContextLost() {
  // TODO(lethalantidote): This check will be obsolete once other TODO to move
  // field initialization earlier is fulfilled.
  if (frame_sink_destroyed_callback_)
    frame_sink_destroyed_callback_.Run();

  if (binding_.is_bound())
    binding_.Unbind();

  compositor_frame_sink_.reset();
  context_provider_->RemoveObserver(this);
  context_provider_ = nullptr;

  resource_provider_->OnContextLost();
  context_provider_callback_.Run(
      base::BindOnce(&VideoFrameSubmitter::OnReceivedContextProvider,
                     weak_ptr_factory_.GetWeakPtr()));
}

void VideoFrameSubmitter::DidReceiveCompositorFrameAck(
    const WTF::Vector<viz::ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  ReclaimResources(resources);
}

void VideoFrameSubmitter::ReclaimResources(
    const WTF::Vector<viz::ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(media_thread_checker_);
  WebVector<viz::ReturnedResource> temp_resources = resources;
  std::vector<viz::ReturnedResource> std_resources =
      temp_resources.ReleaseVector();
  resource_provider_->ReceiveReturnsFromParent(std_resources);
}

void VideoFrameSubmitter::DidPresentCompositorFrame(
    uint32_t presentation_token,
    mojo_base::mojom::blink::TimeTicksPtr time,
    WTF::TimeDelta refresh,
    uint32_t flags) {}

void VideoFrameSubmitter::DidDiscardCompositorFrame(
    uint32_t presentation_token) {}

void VideoFrameSubmitter::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const viz::SharedBitmapId& id) {
  DCHECK(compositor_frame_sink_);
  compositor_frame_sink_->DidAllocateSharedBitmap(
      std::move(buffer), SharedBitmapIdToGpuMailboxPtr(id));
}

void VideoFrameSubmitter::DidDeleteSharedBitmap(const viz::SharedBitmapId& id) {
  DCHECK(compositor_frame_sink_);
  compositor_frame_sink_->DidDeleteSharedBitmap(
      SharedBitmapIdToGpuMailboxPtr(id));
}

}  // namespace blink
