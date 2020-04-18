// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_video_decoder.h"

#include "base/bind.h"
#include "base/callback.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/gpu/windows/d3d11_video_decoder_impl.h"

namespace {

// Check |weak_ptr| and run |cb| with |args| if it's non-null.
template <typename T, typename... Args>
void CallbackOnProperThread(base::WeakPtr<T> weak_ptr,
                            base::Callback<void(Args...)> cb,
                            Args... args) {
  if (weak_ptr.get())
    cb.Run(args...);
}

// Given a callback, |cb|, return another callback that will call |cb| after
// switching to the thread that BindToCurrent.... is called on.  We will check
// |weak_ptr| on the current thread.  This is different than just calling
// BindToCurrentLoop because we'll check the weak ptr.  If |cb| is some method
// of |T|, then one can use BindToCurrentLoop directly.  However, in our case,
// we have some unrelated callback that we'd like to call only if we haven't
// been destroyed yet.  I suppose this could also just be a method:
// template<CB, ...> D3D11VideoDecoder::CallSomeCallback(CB, ...) that's bound
// via BindToCurrentLoop directly.
template <typename T, typename... Args>
base::Callback<void(Args...)> BindToCurrentThreadIfWeakPtr(
    base::WeakPtr<T> weak_ptr,
    base::Callback<void(Args...)> cb) {
  return media::BindToCurrentLoop(
      base::Bind(&CallbackOnProperThread<T, Args...>, weak_ptr, cb));
}

}  // namespace

namespace media {

std::unique_ptr<VideoDecoder> D3D11VideoDecoder::Create(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    const gpu::GpuPreferences& gpu_preferences,
    const gpu::GpuDriverBugWorkarounds& gpu_workarounds,
    base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb) {
  // We create |impl_| on the wrong thread, but we never use it here.
  // Note that the output callback will hop to our thread, post the video
  // frame, and along with a callback that will hop back to the impl thread
  // when it's released.
  // Note that we WrapUnique<VideoDecoder> rather than D3D11VideoDecoder to make
  // this castable; the deleters have to match.
  return base::WrapUnique<VideoDecoder>(new D3D11VideoDecoder(
      std::move(gpu_task_runner), gpu_preferences, gpu_workarounds,
      std::make_unique<D3D11VideoDecoderImpl>(get_stub_cb)));
}

D3D11VideoDecoder::D3D11VideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    const gpu::GpuPreferences& gpu_preferences,
    const gpu::GpuDriverBugWorkarounds& gpu_workarounds,
    std::unique_ptr<D3D11VideoDecoderImpl> impl)
    : impl_(std::move(impl)),
      impl_task_runner_(std::move(gpu_task_runner)),
      gpu_preferences_(gpu_preferences),
      gpu_workarounds_(gpu_workarounds),
      weak_factory_(this) {
  impl_weak_ = impl_->GetWeakPtr();
}

D3D11VideoDecoder::~D3D11VideoDecoder() {
  // Post destruction to the main thread.  When this executes, it will also
  // cancel pending callbacks into |impl_| via |impl_weak_|.  Callbacks out
  // from |impl_| will be cancelled by |weak_factory_| when we return.
  impl_task_runner_->DeleteSoon(FROM_HERE, std::move(impl_));
}

std::string D3D11VideoDecoder::GetDisplayName() const {
  return "D3D11VideoDecoder";
}

void D3D11VideoDecoder::Initialize(
    const VideoDecoderConfig& config,
    bool low_delay,
    CdmContext* cdm_context,
    const InitCB& init_cb,
    const OutputCB& output_cb,
    const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) {
  if (!IsPotentiallySupported(config)) {
    DVLOG(3) << "D3D11 video decoder not supported for the config.";
    init_cb.Run(false);
    return;
  }

  // Bind our own init / output cb that hop to this thread, so we don't call the
  // originals on some other thread.
  // Important but subtle note: base::Bind will copy |config_| since it's a
  // const ref.
  // TODO(liberato): what's the lifetime of |cdm_context|?
  impl_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &VideoDecoder::Initialize, impl_weak_, config, low_delay, cdm_context,
          BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), init_cb),
          BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), output_cb),
          BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(),
                                       waiting_for_decryption_key_cb)));
}

void D3D11VideoDecoder::Decode(scoped_refptr<DecoderBuffer> buffer,
                               const DecodeCB& decode_cb) {
  impl_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &VideoDecoder::Decode, impl_weak_, std::move(buffer),
          BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), decode_cb)));
}

void D3D11VideoDecoder::Reset(const base::Closure& closure) {
  impl_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VideoDecoder::Reset, impl_weak_,
                                BindToCurrentThreadIfWeakPtr(
                                    weak_factory_.GetWeakPtr(), closure)));
}

bool D3D11VideoDecoder::NeedsBitstreamConversion() const {
  // Wrong thread, but it's okay.
  return impl_->NeedsBitstreamConversion();
}

bool D3D11VideoDecoder::CanReadWithoutStalling() const {
  // Wrong thread, but it's okay.
  return impl_->CanReadWithoutStalling();
}

int D3D11VideoDecoder::GetMaxDecodeRequests() const {
  // Wrong thread, but it's okay.
  return impl_->GetMaxDecodeRequests();
}

bool D3D11VideoDecoder::IsPotentiallySupported(
    const VideoDecoderConfig& config) {
  // TODO(liberato): All of this could be moved into MojoVideoDecoder, so that
  // it could run on the client side and save the IPC hop.

  // Must be H264.
  const bool is_h264 = config.profile() >= H264PROFILE_MIN &&
                       config.profile() <= H264PROFILE_MAX;

  if (!is_h264) {
    DVLOG(2) << "Profile is not H264.";
    return false;
  }

  // Must use NV12, which excludes HDR.
  if (config.profile() == H264PROFILE_HIGH10PROFILE) {
    DVLOG(2) << "High 10 profile is not supported.";
    return false;
  }

  // TODO(liberato): dxva checks IsHDR() in the target colorspace, but we don't
  // have the target colorspace.  It's commented as being for vpx, though, so
  // we skip it here for now.

  // Must use the validating decoder.
  if (gpu_preferences_.use_passthrough_cmd_decoder) {
    DVLOG(2) << "Must use validating decoder.";
    return false;
  }

  // Must allow zero-copy of nv12 textures.
  if (!gpu_preferences_.enable_zero_copy_dxgi_video) {
    DVLOG(2) << "Must allow zero-copy NV12.";
    return false;
  }

  if (gpu_workarounds_.disable_dxgi_zero_copy_video) {
    DVLOG(2) << "Must allow zero-copy video.";
    return false;
  }

  return true;
}

}  // namespace media
