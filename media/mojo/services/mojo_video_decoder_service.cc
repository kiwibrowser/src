// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_video_decoder_service.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/optional.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/cdm_context.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_decoder_buffer_converter.h"
#include "media/mojo/services/mojo_cdm_service_context.h"
#include "media/mojo/services/mojo_media_client.h"
#include "media/mojo/services/mojo_media_log.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/handle.h"

namespace media {

namespace {

// Number of active (Decode() was called at least once)
// MojoVideoDecoderService instances that are alive.
//
// Since MojoVideoDecoderService is constructed only by the MediaFactory,
// this will only ever be accessed from a single thread.
static int32_t g_num_active_mvd_instances = 0;

class StaticSyncTokenClient : public VideoFrame::SyncTokenClient {
 public:
  explicit StaticSyncTokenClient(const gpu::SyncToken& sync_token)
      : sync_token_(sync_token) {}

  // VideoFrame::SyncTokenClient implementation
  void GenerateSyncToken(gpu::SyncToken* sync_token) final {
    *sync_token = sync_token_;
  }

  void WaitSyncToken(const gpu::SyncToken& sync_token) final {
    // NOP; we don't care what the old sync token was.
  }

 private:
  gpu::SyncToken sync_token_;

  DISALLOW_COPY_AND_ASSIGN(StaticSyncTokenClient);
};

}  // namespace

class VideoFrameHandleReleaserImpl final
    : public mojom::VideoFrameHandleReleaser {
 public:
  VideoFrameHandleReleaserImpl() { DVLOG(3) << __func__; }

  ~VideoFrameHandleReleaserImpl() final { DVLOG(3) << __func__; }

  // Register a VideoFrame to recieve release callbacks. A reference to |frame|
  // will be held until the remote client calls ReleaseVideoFrame() or is
  // disconnected.
  //
  // Returns an UnguessableToken which the client must use to release the
  // VideoFrame.
  base::UnguessableToken RegisterVideoFrame(scoped_refptr<VideoFrame> frame) {
    base::UnguessableToken token = base::UnguessableToken::Create();
    DVLOG(3) << __func__ << " => " << token.ToString();
    video_frames_[token] = std::move(frame);
    return token;
  }

  // mojom::MojoVideoFrameHandleReleaser implementation
  void ReleaseVideoFrame(const base::UnguessableToken& release_token,
                         const gpu::SyncToken& release_sync_token) final {
    DVLOG(3) << __func__ << "(" << release_token.ToString() << ")";
    auto it = video_frames_.find(release_token);
    if (it == video_frames_.end()) {
      mojo::ReportBadMessage("Unknown |release_token|.");
      return;
    }
    StaticSyncTokenClient client(release_sync_token);
    it->second->UpdateReleaseSyncToken(&client);
    video_frames_.erase(it);
  }

 private:
  // TODO(sandersd): Also track age, so that an overall limit can be enforced.
  std::map<base::UnguessableToken, scoped_refptr<VideoFrame>> video_frames_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameHandleReleaserImpl);
};

MojoVideoDecoderService::MojoVideoDecoderService(
    MojoMediaClient* mojo_media_client,
    MojoCdmServiceContext* mojo_cdm_service_context)
    : mojo_media_client_(mojo_media_client),
      mojo_cdm_service_context_(mojo_cdm_service_context),
      weak_factory_(this) {
  DVLOG(1) << __func__;
  DCHECK(mojo_media_client_);
  DCHECK(mojo_cdm_service_context_);
  weak_this_ = weak_factory_.GetWeakPtr();
}

MojoVideoDecoderService::~MojoVideoDecoderService() {
  DVLOG(1) << __func__;

  if (is_active_instance_)
    g_num_active_mvd_instances--;
}

void MojoVideoDecoderService::Construct(
    mojom::VideoDecoderClientAssociatedPtrInfo client,
    mojom::MediaLogAssociatedPtrInfo media_log,
    mojom::VideoFrameHandleReleaserRequest video_frame_handle_releaser,
    mojo::ScopedDataPipeConsumerHandle decoder_buffer_pipe,
    mojom::CommandBufferIdPtr command_buffer_id,
    const gfx::ColorSpace& target_color_space) {
  DVLOG(1) << __func__;

  if (decoder_) {
    // TODO(sandersd): Close the channel.
    DLOG(ERROR) << "|decoder_| already exists";
    return;
  }

  client_.Bind(std::move(client));

  media_log_ = std::make_unique<MojoMediaLog>(
      mojom::ThreadSafeMediaLogAssociatedPtr::Create(
          std::move(media_log), base::ThreadTaskRunnerHandle::Get()));

  video_frame_handle_releaser_ =
      mojo::MakeStrongBinding(std::make_unique<VideoFrameHandleReleaserImpl>(),
                              std::move(video_frame_handle_releaser));

  mojo_decoder_buffer_reader_.reset(
      new MojoDecoderBufferReader(std::move(decoder_buffer_pipe)));

  decoder_ = mojo_media_client_->CreateVideoDecoder(
      base::ThreadTaskRunnerHandle::Get(), media_log_.get(),
      std::move(command_buffer_id),
      base::Bind(&MojoVideoDecoderService::OnDecoderRequestedOverlayInfo,
                 weak_this_),
      target_color_space);
}

void MojoVideoDecoderService::Initialize(const VideoDecoderConfig& config,
                                         bool low_delay,
                                         int32_t cdm_id,
                                         InitializeCallback callback) {
  DVLOG(1) << __func__ << " config = " << config.AsHumanReadableString()
           << ", cdm_id = " << cdm_id;

  if (!decoder_) {
    std::move(callback).Run(false, false, 1);
    return;
  }

  // Get CdmContext from cdm_id if the stream is encrypted.
  CdmContext* cdm_context = nullptr;
  if (cdm_id != CdmContext::kInvalidCdmId) {
    cdm_context_ref_ = mojo_cdm_service_context_->GetCdmContextRef(cdm_id);
    if (!cdm_context_ref_) {
      DVLOG(1) << "CdmContextRef not found for CDM id: " << cdm_id;
      std::move(callback).Run(false, false, 1);
      return;
    }

    cdm_context = cdm_context_ref_->GetCdmContext();
    DCHECK(cdm_context);
  }

  decoder_->Initialize(
      config, low_delay, cdm_context,
      base::Bind(&MojoVideoDecoderService::OnDecoderInitialized, weak_this_,
                 base::Passed(&callback)),
      base::BindRepeating(&MojoVideoDecoderService::OnDecoderOutput,
                          weak_this_),
      base::NullCallback());
}

void MojoVideoDecoderService::Decode(mojom::DecoderBufferPtr buffer,
                                     DecodeCallback callback) {
  DVLOG(3) << __func__ << " pts=" << buffer->timestamp.InMilliseconds();

  if (!decoder_) {
    std::move(callback).Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  if (!is_active_instance_) {
    is_active_instance_ = true;
    g_num_active_mvd_instances++;
    UMA_HISTOGRAM_EXACT_LINEAR("Media.MojoVideoDecoder.ActiveInstances",
                               g_num_active_mvd_instances, 64);
  }

  mojo_decoder_buffer_reader_->ReadDecoderBuffer(
      std::move(buffer), base::BindOnce(&MojoVideoDecoderService::OnReaderRead,
                                        weak_this_, std::move(callback)));
}

void MojoVideoDecoderService::Reset(ResetCallback callback) {
  DVLOG(2) << __func__;

  if (!decoder_) {
    std::move(callback).Run();
    return;
  }

  // Flush the reader so that pending decodes will be dispatched first.
  mojo_decoder_buffer_reader_->Flush(
      base::Bind(&MojoVideoDecoderService::OnReaderFlushed, weak_this_,
                 base::Passed(&callback)));
}

void MojoVideoDecoderService::OnDecoderInitialized(InitializeCallback callback,
                                                   bool success) {
  DVLOG(1) << __func__;
  DCHECK(decoder_);

  if (!success)
    cdm_context_ref_.reset();

  std::move(callback).Run(success, decoder_->NeedsBitstreamConversion(),
                          decoder_->GetMaxDecodeRequests());
}

void MojoVideoDecoderService::OnReaderRead(
    DecodeCallback callback,
    scoped_refptr<DecoderBuffer> buffer) {
  DVLOG(3) << __func__;

  if (!buffer) {
    std::move(callback).Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  decoder_->Decode(
      buffer, base::Bind(&MojoVideoDecoderService::OnDecoderDecoded, weak_this_,
                         base::Passed(&callback)));
}

void MojoVideoDecoderService::OnReaderFlushed(ResetCallback callback) {
  decoder_->Reset(base::Bind(&MojoVideoDecoderService::OnDecoderReset,
                             weak_this_, base::Passed(&callback)));
}

void MojoVideoDecoderService::OnDecoderDecoded(DecodeCallback callback,
                                               DecodeStatus status) {
  DVLOG(3) << __func__;
  std::move(callback).Run(status);
}

void MojoVideoDecoderService::OnDecoderReset(ResetCallback callback) {
  DVLOG(2) << __func__;
  std::move(callback).Run();
}

void MojoVideoDecoderService::OnDecoderOutput(
    const scoped_refptr<VideoFrame>& frame) {
  DVLOG(3) << __func__;
  DCHECK(client_);
  DCHECK(decoder_);

  // All MojoVideoDecoder-based decoders are hardware decoders. If you're the
  // first to implement an out-of-process decoder that is not power efficent,
  // you can remove this DCHECK.
  DCHECK(frame->metadata()->IsTrue(VideoFrameMetadata::POWER_EFFICIENT));

  base::Optional<base::UnguessableToken> release_token;
  if (frame->HasReleaseMailboxCB() && video_frame_handle_releaser_) {
    // |video_frame_handle_releaser_| is explicitly constructed with a
    // VideoFrameHandleReleaserImpl in Construct().
    VideoFrameHandleReleaserImpl* releaser =
        static_cast<VideoFrameHandleReleaserImpl*>(
            video_frame_handle_releaser_->impl());
    release_token = releaser->RegisterVideoFrame(frame);
  }

  client_->OnVideoFrameDecoded(frame, decoder_->CanReadWithoutStalling(),
                               std::move(release_token));
}

void MojoVideoDecoderService::OnOverlayInfoChanged(
    const OverlayInfo& overlay_info) {
  DVLOG(2) << __func__;
  DCHECK(client_);
  DCHECK(decoder_);
  DCHECK(provide_overlay_info_cb_);
  provide_overlay_info_cb_.Run(overlay_info);
}

void MojoVideoDecoderService::OnDecoderRequestedOverlayInfo(
    bool restart_for_transitions,
    const ProvideOverlayInfoCB& provide_overlay_info_cb) {
  DVLOG(2) << __func__;
  DCHECK(client_);
  DCHECK(decoder_);
  DCHECK(!provide_overlay_info_cb_);

  provide_overlay_info_cb_ = std::move(provide_overlay_info_cb);
  client_->RequestOverlayInfo(restart_for_transitions);
}

}  // namespace media
