// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_D3D11_VIDEO_DECODER_H_
#define MEDIA_GPU_D3D11_VIDEO_DECODER_H_

#include <string>

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/config/gpu_driver_bug_workarounds.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "media/base/video_decoder.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

class D3D11VideoDecoderImpl;
class D3D11VideoDecoderTest;

// Thread-hopping implementation of D3D11VideoDecoder.  It's meant to run on
// a random thread, and hop to the gpu main thread.  It does this so that it
// can use the D3D context etc.  What should really happen is that we should
// get (or share with other D3D11VideoDecoder instances) our own context, and
// just share the D3D texture with the main thread's context.  However, for
// now, it's easier to hop threads.
class MEDIA_GPU_EXPORT D3D11VideoDecoder : public VideoDecoder {
 public:
  static std::unique_ptr<VideoDecoder> Create(
      scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
      const gpu::GpuPreferences& gpu_preferences,
      const gpu::GpuDriverBugWorkarounds& gpu_workarounds,
      base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb);

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

  // Return false |config| definitely isn't going to work, so that we can fail
  // init without bothering with a thread hop.
  bool IsPotentiallySupported(const VideoDecoderConfig& config);

 protected:
  // Owners should call Destroy(). This is automatic via
  // std::default_delete<media::VideoDecoder> when held by a
  // std::unique_ptr<media::VideoDecoder>.
  ~D3D11VideoDecoder() override;

 private:
  friend class D3D11VideoDecoderTest;

  D3D11VideoDecoder(scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
                    const gpu::GpuPreferences& gpu_preferences,
                    const gpu::GpuDriverBugWorkarounds& gpu_workarounds,
                    std::unique_ptr<D3D11VideoDecoderImpl> impl);

  // The implementation, which we trampoline to the impl thread.
  // This must be freed on the impl thread.
  std::unique_ptr<D3D11VideoDecoderImpl> impl_;

  // Weak ptr to |impl_|, which we use for callbacks.
  base::WeakPtr<VideoDecoder> impl_weak_;

  // Task runner for |impl_|.  This must be the GPU main thread.
  scoped_refptr<base::SequencedTaskRunner> impl_task_runner_;

  gpu::GpuPreferences gpu_preferences_;
  gpu::GpuDriverBugWorkarounds gpu_workarounds_;

  base::WeakPtrFactory<D3D11VideoDecoder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(D3D11VideoDecoder);
};

}  // namespace media

#endif  // MEDIA_GPU_D3D11_VIDEO_DECODER_H_
