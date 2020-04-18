// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_AUDIO_DECODER_H_
#define MEDIA_MOJO_CLIENTS_MOJO_AUDIO_DECODER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/audio_decoder.h"
#include "media/mojo/interfaces/audio_decoder.mojom.h"
#include "media/mojo/interfaces/media_types.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class MojoDecoderBufferWriter;

// An AudioDecoder that proxies to a mojom::AudioDecoder.
class MojoAudioDecoder : public AudioDecoder, public mojom::AudioDecoderClient {
 public:
  MojoAudioDecoder(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                   mojom::AudioDecoderPtr remote_decoder);
  ~MojoAudioDecoder() final;

  // AudioDecoder implementation.
  std::string GetDisplayName() const final;
  void Initialize(
      const AudioDecoderConfig& config,
      CdmContext* cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) final;
  void Decode(scoped_refptr<DecoderBuffer> buffer,
              const DecodeCB& decode_cb) final;
  void Reset(const base::Closure& closure) final;
  bool NeedsBitstreamConversion() const final;

  // AudioDecoderClient implementation.
  void OnBufferDecoded(mojom::AudioBufferPtr buffer) final;

  void set_writer_capacity_for_testing(uint32_t capacity) {
    writer_capacity_ = capacity;
  }

 private:
  void BindRemoteDecoder();

  // Callback for connection error on |remote_decoder_|.
  void OnConnectionError();

  // Called when |remote_decoder_| finished initialization.
  void OnInitialized(bool success, bool needs_bitstream_conversion);

  // Called when |remote_decoder_| accepted or rejected DecoderBuffer.
  void OnDecodeStatus(DecodeStatus decode_status);

  // called when |remote_decoder_| finished Reset() sequence.
  void OnResetDone();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // This class is constructed on one thread and used exclusively on another
  // thread. This member is used to safely pass the AudioDecoderPtr from one
  // thread to another. It is set in the constructor and is consumed in
  // Initialize().
  mojom::AudioDecoderPtrInfo remote_decoder_info_;

  mojom::AudioDecoderPtr remote_decoder_;

  std::unique_ptr<MojoDecoderBufferWriter> mojo_decoder_buffer_writer_;

  uint32_t writer_capacity_ = 0;

  // Binding for AudioDecoderClient, bound to the |task_runner_|.
  mojo::AssociatedBinding<AudioDecoderClient> client_binding_;

  // We call the following callbacks to pass the information to the pipeline.
  // |output_cb_| is permanent while other three are called only once,
  // |decode_cb_| and |reset_cb_| are replaced by every by Decode() and Reset().
  InitCB init_cb_;
  OutputCB output_cb_;
  DecodeCB decode_cb_;
  base::Closure reset_cb_;

  // Flag telling whether this decoder requires bitstream conversion.
  // Passed from |remote_decoder_| as a result of its initialization.
  bool needs_bitstream_conversion_ = false;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioDecoder);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_AUDIO_DECODER_H_
