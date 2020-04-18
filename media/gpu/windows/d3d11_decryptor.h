// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_WINDOWS_D3D11_DECRYPTOR_H_
#define MEDIA_GPU_WINDOWS_D3D11_DECRYPTOR_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/base/decryptor.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

class CdmProxyContext;

class MEDIA_GPU_EXPORT D3D11Decryptor : public Decryptor {
 public:
  explicit D3D11Decryptor(CdmProxyContext* cdm_proxy_context);
  ~D3D11Decryptor() final;

  // Decryptor implementation.
  void RegisterNewKeyCB(StreamType stream_type,
                        const NewKeyCB& key_added_cb) final;
  void Decrypt(StreamType stream_type,
               scoped_refptr<DecoderBuffer> encrypted,
               const DecryptCB& decrypt_cb) final;
  void CancelDecrypt(StreamType stream_type) final;
  void InitializeAudioDecoder(const AudioDecoderConfig& config,
                              const DecoderInitCB& init_cb) final;
  void InitializeVideoDecoder(const VideoDecoderConfig& config,
                              const DecoderInitCB& init_cb) final;
  void DecryptAndDecodeAudio(scoped_refptr<DecoderBuffer> encrypted,
                             const AudioDecodeCB& audio_decode_cb) final;
  void DecryptAndDecodeVideo(scoped_refptr<DecoderBuffer> encrypted,
                             const VideoDecodeCB& video_decode_cb) final;
  void ResetDecoder(StreamType stream_type) final;
  void DeinitializeDecoder(StreamType stream_type) final;

 private:
  CdmProxyContext* cdm_proxy_context_;

  base::WeakPtrFactory<D3D11Decryptor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(D3D11Decryptor);
};

}  // namespace media

#endif  // MEDIA_GPU_WINDOWS_D3D11_DECRYPTOR_H_
