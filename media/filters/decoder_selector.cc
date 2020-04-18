// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/decoder_selector.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/base/audio_decoder.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_log.h"
#include "media/base/video_decoder.h"
#include "media/filters/decoder_stream_traits.h"
#include "media/filters/decrypting_demuxer_stream.h"

namespace media {

static bool HasValidStreamConfig(DemuxerStream* stream) {
  switch (stream->type()) {
    case DemuxerStream::AUDIO:
      return stream->audio_decoder_config().IsValidConfig();
    case DemuxerStream::VIDEO:
      return stream->video_decoder_config().IsValidConfig();
    case DemuxerStream::TEXT:
    case DemuxerStream::UNKNOWN:
      NOTREACHED();
  }
  return false;
}

template <DemuxerStream::Type StreamType>
DecoderSelector<StreamType>::DecoderSelector(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    CreateDecodersCB create_decoders_cb,
    MediaLog* media_log)
    : task_runner_(task_runner),
      create_decoders_cb_(std::move(create_decoders_cb)),
      media_log_(media_log),
      weak_ptr_factory_(this) {}

template <DemuxerStream::Type StreamType>
DecoderSelector<StreamType>::~DecoderSelector() {
  DVLOG(2) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!select_decoder_cb_.is_null())
    ReturnNullDecoder();

  decoder_.reset();
  decrypted_stream_.reset();
}

template <DemuxerStream::Type StreamType>
void DecoderSelector<StreamType>::SelectDecoder(
    StreamTraits* traits,
    DemuxerStream* stream,
    CdmContext* cdm_context,
    const std::string& blacklisted_decoder,
    const SelectDecoderCB& select_decoder_cb,
    const typename Decoder::OutputCB& output_cb,
    const base::Closure& waiting_for_decryption_key_cb) {
  DVLOG(2) << __func__ << ": cdm_context=" << cdm_context
           << ", blacklisted_decoder=" << blacklisted_decoder;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(traits);
  DCHECK(stream);
  DCHECK(select_decoder_cb_.is_null());

  // Make sure |select_decoder_cb| runs on a different execution stack.
  select_decoder_cb_ = BindToCurrentLoop(select_decoder_cb);

  if (!HasValidStreamConfig(stream)) {
    DLOG(ERROR) << "Invalid stream config.";
    ReturnNullDecoder();
    return;
  }

  traits_ = traits;
  input_stream_ = stream;
  cdm_context_ = cdm_context;
  blacklisted_decoder_ = blacklisted_decoder;
  output_cb_ = output_cb;
  waiting_for_decryption_key_cb_ = waiting_for_decryption_key_cb;

  decoders_ = create_decoders_cb_.Run();
  config_ = traits_->GetDecoderConfig(input_stream_);

  InitializeDecoder();
}

template <DemuxerStream::Type StreamType>
void DecoderSelector<StreamType>::InitializeDecoder() {
  DVLOG(2) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!decoder_);

  // Select the next non-blacklisted decoder.
  while (!decoders_.empty()) {
    std::unique_ptr<Decoder> decoder(std::move(decoders_.front()));
    decoders_.erase(decoders_.begin());
    // When |decrypted_stream_| is selected, the |config_| has changed so ignore
    // the blacklist.
    if (decrypted_stream_ ||
        decoder->GetDisplayName() != blacklisted_decoder_) {
      decoder_ = std::move(decoder);
      break;
    }
  }

  if (!decoder_) {
    // No decoder could handle encrypted content, try to do decrypt-only.
    if (!tried_decrypting_demuxer_stream_ && config_.is_encrypted()) {
      InitializeDecryptingDemuxerStream();
      return;
    }

    ReturnNullDecoder();
    return;
  }

  DVLOG(2) << __func__ << ": initializing " << decoder_->GetDisplayName();
  traits_->InitializeDecoder(
      decoder_.get(), config_,
      input_stream_->liveness() == DemuxerStream::LIVENESS_LIVE, cdm_context_,
      base::Bind(&DecoderSelector<StreamType>::DecoderInitDone,
                 weak_ptr_factory_.GetWeakPtr()),
      output_cb_, waiting_for_decryption_key_cb_);
}

template <DemuxerStream::Type StreamType>
void DecoderSelector<StreamType>::DecoderInitDone(bool success) {
  DVLOG(2) << __func__ << ": " << decoder_->GetDisplayName()
           << " success=" << success;
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!success) {
    decoder_.reset();
    InitializeDecoder();
    return;
  }

  DVLOG(1) << __func__ << ": " << decoder_->GetDisplayName()
           << " selected. DecryptingDemuxerStream "
           << (decrypted_stream_ ? "also" : "not") << " selected.";

  decoders_.clear();
  base::ResetAndReturn(&select_decoder_cb_)
      .Run(std::move(decoder_), std::move(decrypted_stream_));
}

template <DemuxerStream::Type StreamType>
void DecoderSelector<StreamType>::ReturnNullDecoder() {
  DVLOG(1) << __func__ << ": No decoder selected.";
  DCHECK(task_runner_->BelongsToCurrentThread());
  decoders_.clear();
  base::ResetAndReturn(&select_decoder_cb_)
      .Run(std::unique_ptr<Decoder>(),
           std::unique_ptr<DecryptingDemuxerStream>());
}

template <DemuxerStream::Type StreamType>
void DecoderSelector<StreamType>::InitializeDecryptingDemuxerStream() {
  decrypted_stream_.reset(new DecryptingDemuxerStream(
      task_runner_, media_log_, waiting_for_decryption_key_cb_));

  decrypted_stream_->Initialize(
      input_stream_, cdm_context_,
      base::Bind(&DecoderSelector<StreamType>::DecryptingDemuxerStreamInitDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

template <DemuxerStream::Type StreamType>
void DecoderSelector<StreamType>::DecryptingDemuxerStreamInitDone(
    PipelineStatus status) {
  DVLOG(2) << __func__
           << ": status=" << MediaLog::PipelineStatusToString(status);
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(cdm_context_);

  // If DecryptingDemuxerStream initialization failed, we've already tried every
  // possible decoder, so we can just ReturnNullDecoder() here.
  if (status != PIPELINE_OK) {
    DCHECK(decoders_.empty());
    DCHECK(config_.is_encrypted());
    decrypted_stream_.reset();
    ReturnNullDecoder();
    return;
  }

  // If DecryptingDemuxerStream initialization succeeded, we'll use it to do
  // decryption and use a decoder to decode the clear stream. Otherwise, we'll
  // try to see whether any decoder can decrypt-and-decode the encrypted stream
  // directly. So in both cases, we'll initialize the decoders.
  input_stream_ = decrypted_stream_.get();
  config_ = traits_->GetDecoderConfig(input_stream_);
  DCHECK(!config_.is_encrypted());

  // If we're here we tried all the decoders w/ is_encrypted=true, try again
  // now that the stream is being decrypted by the demuxer.
  DCHECK(decoders_.empty());
  DCHECK(!tried_decrypting_demuxer_stream_);
  decoders_ = create_decoders_cb_.Run();
  tried_decrypting_demuxer_stream_ = true;

  InitializeDecoder();
}

// These forward declarations tell the compiler that we will use
// DecoderSelector with these arguments, allowing us to keep these definitions
// in our .cc without causing linker errors. This also means if anyone tries to
// instantiate a DecoderSelector with anything but these two specializations
// they'll most likely get linker errors.
template class DecoderSelector<DemuxerStream::AUDIO>;
template class DecoderSelector<DemuxerStream::VIDEO>;

}  // namespace media
