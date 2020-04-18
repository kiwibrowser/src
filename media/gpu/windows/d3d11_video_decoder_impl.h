// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_D3D11_VIDEO_DECODER_IMPL_H_
#define MEDIA_GPU_D3D11_VIDEO_DECODER_IMPL_H_

#include <d3d11.h>
#include <wrl/client.h>

#include <list>
#include <memory>
#include <string>
#include <tuple>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "gpu/command_buffer/service/sequence_id.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "media/base/callback_registry.h"
#include "media/base/video_decoder.h"
#include "media/gpu/gles2_decoder_helper.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/windows/d3d11_h264_accelerator.h"
#include "media/gpu/windows/output_with_release_mailbox_cb.h"

namespace media {

class MEDIA_GPU_EXPORT D3D11VideoDecoderImpl : public VideoDecoder,
                                               public D3D11VideoDecoderClient {
 public:
  explicit D3D11VideoDecoderImpl(
      base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb);
  ~D3D11VideoDecoderImpl() override;

  // VideoDecoder implementation:
  std::string GetDisplayName() const override;
  void Initialize(
      const VideoDecoderConfig& config,
      bool low_delay,
      CdmContext* cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) override;
  void Decode(scoped_refptr<DecoderBuffer> buffer,
              const DecodeCB& decode_cb) override;
  void Reset(const base::Closure& closure) override;
  bool NeedsBitstreamConversion() const override;
  bool CanReadWithoutStalling() const override;
  int GetMaxDecodeRequests() const override;

  // D3D11VideoDecoderClient implementation.
  D3D11PictureBuffer* GetPicture() override;
  void OutputResult(D3D11PictureBuffer* buffer) override;

  // Return a weak ptr, since D3D11VideoDecoder constructs callbacks for us.
  base::WeakPtr<D3D11VideoDecoderImpl> GetWeakPtr();

 private:
  enum class State {
    // Initializing resources required to create a codec.
    kInitializing,
    // Initialization has completed and we're running. This is the only state
    // in which |codec_| might be non-null. If |codec_| is null, a codec
    // creation is pending.
    kRunning,
    // The decoder cannot make progress because it doesn't have the key to
    // decrypt the buffer. Waiting for a new key to be available.
    // This should only be transitioned from kRunning, and should only
    // transition to kRunning.
    kWaitingForNewKey,
    // A fatal error occurred. A terminal state.
    kError,
  };

  void DoDecode();
  void CreatePictureBuffers();

  void OnMailboxReleased(scoped_refptr<D3D11PictureBuffer> buffer,
                         const gpu::SyncToken& sync_token);
  void OnSyncTokenReleased(scoped_refptr<D3D11PictureBuffer> buffer);

  // Callback to notify that new usable key is available.
  void NotifyNewKey();

  // Enter the kError state.  This will fail any pending |init_cb_| and / or
  // pending decode as well.
  void NotifyError(const char* reason);

  base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb_;
  gpu::CommandBufferStub* stub_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context_;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> video_device_;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> video_context_;

  std::unique_ptr<AcceleratedVideoDecoder> accelerated_video_decoder_;

  GUID decoder_guid_;

  std::list<std::pair<scoped_refptr<DecoderBuffer>, DecodeCB>>
      input_buffer_queue_;
  scoped_refptr<DecoderBuffer> current_buffer_;
  DecodeCB current_decode_cb_;
  base::TimeDelta current_timestamp_;

  // During init, these will be set.
  InitCB init_cb_;
  OutputCB output_cb_;
  bool is_encrypted_ = false;

  // It would be nice to unique_ptr these, but we give a ref to the VideoFrame
  // so that the texture is retained until the mailbox is opened.
  std::vector<scoped_refptr<D3D11PictureBuffer>> picture_buffers_;

  State state_ = State::kInitializing;

  // Callback registration to keep the new key callback registered.
  std::unique_ptr<CallbackRegistration> new_key_callback_registration_;

  // Wait sequence for sync points.
  gpu::SequenceId wait_sequence_id_;

  base::WeakPtrFactory<D3D11VideoDecoderImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(D3D11VideoDecoderImpl);
};

}  // namespace media

#endif  // MEDIA_GPU_D3D11_VIDEO_DECODER_IMPL_H_
