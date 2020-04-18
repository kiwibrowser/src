// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_decryptor.h"

#include "base/logging.h"
#include "media/base/decoder_buffer.h"

namespace media {

D3D11Decryptor::D3D11Decryptor(CdmProxyContext* cdm_proxy_context)
    : cdm_proxy_context_(cdm_proxy_context), weak_factory_(this) {
  DCHECK(cdm_proxy_context_);
}

D3D11Decryptor::~D3D11Decryptor() {}

void D3D11Decryptor::RegisterNewKeyCB(StreamType stream_type,
                                      const NewKeyCB& new_key_cb) {
  // TODO(xhwang): Use RegisterNewKeyCB() on CdmContext, and remove
  // RegisterNewKeyCB from Decryptor interface.
  NOTREACHED();
}

void D3D11Decryptor::Decrypt(StreamType stream_type,
                             scoped_refptr<DecoderBuffer> encrypted,
                             const DecryptCB& decrypt_cb) {
  // TODO(rkuroiwa): Implemented this function using |cdm_proxy_context_|.
  NOTIMPLEMENTED();
}

void D3D11Decryptor::CancelDecrypt(StreamType stream_type) {
  // Decrypt() calls the DecryptCB synchronously so there's nothing to cancel.
}

void D3D11Decryptor::InitializeAudioDecoder(const AudioDecoderConfig& config,
                                            const DecoderInitCB& init_cb) {
  // D3D11Decryptor does not support audio decoding.
  init_cb.Run(false);
}

void D3D11Decryptor::InitializeVideoDecoder(const VideoDecoderConfig& config,
                                            const DecoderInitCB& init_cb) {
  // D3D11Decryptor does not support video decoding.
  init_cb.Run(false);
}

void D3D11Decryptor::DecryptAndDecodeAudio(
    scoped_refptr<DecoderBuffer> encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  NOTREACHED() << "D3D11Decryptor does not support audio decoding";
}

void D3D11Decryptor::DecryptAndDecodeVideo(
    scoped_refptr<DecoderBuffer> encrypted,
    const VideoDecodeCB& video_decode_cb) {
  NOTREACHED() << "D3D11Decryptor does not support video decoding";
}

void D3D11Decryptor::ResetDecoder(StreamType stream_type) {
  NOTREACHED() << "D3D11Decryptor does not support audio/video decoding";
}

void D3D11Decryptor::DeinitializeDecoder(StreamType stream_type) {
  // D3D11Decryptor does not support audio/video decoding, but since this can be
  // called any time after InitializeAudioDecoder/InitializeVideoDecoder,
  // nothing to be done here.
}

}  // namespace media
