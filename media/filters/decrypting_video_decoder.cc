// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/decrypting_video_decoder.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/decoder_buffer.h"
#include "media/base/media_log.h"
#include "media/base/video_frame.h"

namespace media {

const char DecryptingVideoDecoder::kDecoderName[] = "DecryptingVideoDecoder";

DecryptingVideoDecoder::DecryptingVideoDecoder(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    MediaLog* media_log)
    : task_runner_(task_runner),
      media_log_(media_log),
      state_(kUninitialized),
      decryptor_(NULL),
      key_added_while_decode_pending_(false),
      trace_id_(0),
      support_clear_content_(false),
      weak_factory_(this) {}

std::string DecryptingVideoDecoder::GetDisplayName() const {
  return kDecoderName;
}

void DecryptingVideoDecoder::Initialize(
    const VideoDecoderConfig& config,
    bool /* low_delay */,
    CdmContext* cdm_context,
    const InitCB& init_cb,
    const OutputCB& output_cb,
    const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) {
  DVLOG(2) << __func__ << ": " << config.AsHumanReadableString();

  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(state_ == kUninitialized ||
         state_ == kIdle ||
         state_ == kDecodeFinished) << state_;
  DCHECK(decode_cb_.is_null());
  DCHECK(reset_cb_.is_null());
  DCHECK(config.IsValidConfig());

  init_cb_ = BindToCurrentLoop(init_cb);
  if (!cdm_context) {
    // Once we have a CDM context, one should always be present.
    DCHECK(!support_clear_content_);
    base::ResetAndReturn(&init_cb_).Run(false);
    return;
  }

  if (!config.is_encrypted() && !support_clear_content_) {
    base::ResetAndReturn(&init_cb_).Run(false);
    return;
  }

  // Once initialized with encryption support, the value is sticky, so we'll use
  // the decryptor for clear content as well.
  support_clear_content_ = true;

  output_cb_ = BindToCurrentLoop(output_cb);
  weak_this_ = weak_factory_.GetWeakPtr();
  config_ = config;

  DCHECK(!waiting_for_decryption_key_cb.is_null());
  waiting_for_decryption_key_cb_ = waiting_for_decryption_key_cb;

  if (state_ == kUninitialized) {
    if (!cdm_context->GetDecryptor()) {
      MEDIA_LOG(DEBUG, media_log_) << GetDisplayName() << ": no decryptor";
      base::ResetAndReturn(&init_cb_).Run(false);
      return;
    }

    decryptor_ = cdm_context->GetDecryptor();
  } else {
    // Reinitialization (i.e. upon a config change). The new config can be
    // encrypted or clear.
    decryptor_->DeinitializeDecoder(Decryptor::kVideo);
  }

  state_ = kPendingDecoderInit;
  decryptor_->InitializeVideoDecoder(
      config_, BindToCurrentLoop(base::Bind(
                   &DecryptingVideoDecoder::FinishInitialization, weak_this_)));
}

void DecryptingVideoDecoder::Decode(scoped_refptr<DecoderBuffer> buffer,
                                    const DecodeCB& decode_cb) {
  DVLOG(3) << "Decode()";
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(state_ == kIdle ||
         state_ == kDecodeFinished ||
         state_ == kError) << state_;
  DCHECK(!decode_cb.is_null());
  CHECK(decode_cb_.is_null()) << "Overlapping decodes are not supported.";

  decode_cb_ = BindToCurrentLoop(decode_cb);

  if (state_ == kError) {
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  // Return empty frames if decoding has finished.
  if (state_ == kDecodeFinished) {
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::OK);
    return;
  }

  pending_buffer_to_decode_ = std::move(buffer);
  state_ = kPendingDecode;
  DecodePendingBuffer();
}

void DecryptingVideoDecoder::Reset(const base::Closure& closure) {
  DVLOG(2) << "Reset() - state: " << state_;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(state_ == kIdle ||
         state_ == kPendingDecode ||
         state_ == kWaitingForKey ||
         state_ == kDecodeFinished ||
         state_ == kError) << state_;
  DCHECK(init_cb_.is_null());  // No Reset() during pending initialization.
  DCHECK(reset_cb_.is_null());

  reset_cb_ = BindToCurrentLoop(closure);

  decryptor_->ResetDecoder(Decryptor::kVideo);

  // Reset() cannot complete if the decode callback is still pending.
  // Defer the resetting process in this case. The |reset_cb_| will be fired
  // after the decode callback is fired - see DecryptAndDecodeBuffer() and
  // DeliverFrame().
  if (state_ == kPendingDecode) {
    DCHECK(!decode_cb_.is_null());
    return;
  }

  if (state_ == kWaitingForKey) {
    DCHECK(!decode_cb_.is_null());
    pending_buffer_to_decode_ = NULL;
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::ABORTED);
  }

  DCHECK(decode_cb_.is_null());
  DoReset();
}

DecryptingVideoDecoder::~DecryptingVideoDecoder() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (state_ == kUninitialized)
    return;

  if (decryptor_) {
    decryptor_->DeinitializeDecoder(Decryptor::kVideo);
    decryptor_ = NULL;
  }
  pending_buffer_to_decode_ = NULL;
  if (!init_cb_.is_null())
    base::ResetAndReturn(&init_cb_).Run(false);
  if (!decode_cb_.is_null())
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::ABORTED);
  if (!reset_cb_.is_null())
    base::ResetAndReturn(&reset_cb_).Run();
}

void DecryptingVideoDecoder::FinishInitialization(bool success) {
  DVLOG(2) << "FinishInitialization()";
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kPendingDecoderInit) << state_;
  DCHECK(!init_cb_.is_null());
  DCHECK(reset_cb_.is_null());  // No Reset() before initialization finished.
  DCHECK(decode_cb_.is_null());  // No Decode() before initialization finished.

  if (!success) {
    MEDIA_LOG(DEBUG, media_log_) << GetDisplayName()
                                 << ": failed to init decoder on decryptor";
    base::ResetAndReturn(&init_cb_).Run(false);
    decryptor_ = NULL;
    state_ = kError;
    return;
  }

  decryptor_->RegisterNewKeyCB(
      Decryptor::kVideo,
      BindToCurrentLoop(
          base::Bind(&DecryptingVideoDecoder::OnKeyAdded, weak_this_)));

  // Success!
  state_ = kIdle;
  base::ResetAndReturn(&init_cb_).Run(true);
}


void DecryptingVideoDecoder::DecodePendingBuffer() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kPendingDecode) << state_;
  TRACE_EVENT_ASYNC_BEGIN0(
      "media", "DecryptingVideoDecoder::DecodePendingBuffer", ++trace_id_);

  int buffer_size = 0;
  if (!pending_buffer_to_decode_->end_of_stream()) {
    buffer_size = pending_buffer_to_decode_->data_size();
  }

  decryptor_->DecryptAndDecodeVideo(
      pending_buffer_to_decode_, BindToCurrentLoop(base::Bind(
          &DecryptingVideoDecoder::DeliverFrame, weak_this_, buffer_size)));
}

void DecryptingVideoDecoder::DeliverFrame(
    int buffer_size,
    Decryptor::Status status,
    const scoped_refptr<VideoFrame>& frame) {
  DVLOG(3) << "DeliverFrame() - status: " << status;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, kPendingDecode) << state_;
  DCHECK(!decode_cb_.is_null());
  DCHECK(pending_buffer_to_decode_.get());

  TRACE_EVENT_ASYNC_END2(
      "media", "DecryptingVideoDecoder::DecodePendingBuffer", trace_id_,
      "buffer_size", buffer_size, "status", status);

  bool need_to_try_again_if_nokey_is_returned = key_added_while_decode_pending_;
  key_added_while_decode_pending_ = false;

  scoped_refptr<DecoderBuffer> scoped_pending_buffer_to_decode =
      std::move(pending_buffer_to_decode_);

  if (!reset_cb_.is_null()) {
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::ABORTED);
    DoReset();
    return;
  }

  DCHECK_EQ(status == Decryptor::kSuccess, frame.get() != NULL);

  if (status == Decryptor::kError) {
    DVLOG(2) << "DeliverFrame() - kError";
    MEDIA_LOG(ERROR, media_log_) << GetDisplayName() << ": decode error";
    state_ = kError;
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  if (status == Decryptor::kNoKey) {
    std::string key_id =
        scoped_pending_buffer_to_decode->decrypt_config()->key_id();
    std::string missing_key_id = base::HexEncode(key_id.data(), key_id.size());
    DVLOG(1) << "DeliverFrame() - no key for key ID " << missing_key_id;
    MEDIA_LOG(INFO, media_log_) << GetDisplayName() << ": no key for key ID "
                                << missing_key_id;

    // Set |pending_buffer_to_decode_| back as we need to try decoding the
    // pending buffer again when new key is added to the decryptor.
    pending_buffer_to_decode_ = std::move(scoped_pending_buffer_to_decode);

    if (need_to_try_again_if_nokey_is_returned) {
      // The |state_| is still kPendingDecode.
      MEDIA_LOG(INFO, media_log_) << GetDisplayName()
                                  << ": key was added, resuming decode";
      DecodePendingBuffer();
      return;
    }

    state_ = kWaitingForKey;
    waiting_for_decryption_key_cb_.Run();
    return;
  }

  if (status == Decryptor::kNeedMoreData) {
    DVLOG(2) << "DeliverFrame() - kNeedMoreData";
    state_ = scoped_pending_buffer_to_decode->end_of_stream() ? kDecodeFinished
                                                              : kIdle;
    base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::OK);
    return;
  }

  DCHECK_EQ(status, Decryptor::kSuccess);
  CHECK(frame);

  // Frame returned with kSuccess should not be an end-of-stream frame.
  DCHECK(!frame->metadata()->IsTrue(VideoFrameMetadata::END_OF_STREAM));

  // If color space is not set, use the color space in the |config_|.
  if (!frame->ColorSpace().IsValid()) {
    DVLOG(3) << "Setting color space using information in the config.";
    frame->metadata()->SetInteger(VideoFrameMetadata::COLOR_SPACE,
                                  config_.color_space());
    if (config_.color_space_info() != VideoColorSpace())
      frame->set_color_space(config_.color_space_info().ToGfxColorSpace());
  }

  output_cb_.Run(frame);

  if (scoped_pending_buffer_to_decode->end_of_stream()) {
    // Set |pending_buffer_to_decode_| back as we need to keep flushing the
    // decryptor.
    pending_buffer_to_decode_ = std::move(scoped_pending_buffer_to_decode);
    DecodePendingBuffer();
    return;
  }

  state_ = kIdle;
  base::ResetAndReturn(&decode_cb_).Run(DecodeStatus::OK);
}

void DecryptingVideoDecoder::OnKeyAdded() {
  DVLOG(2) << "OnKeyAdded()";
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (state_ == kPendingDecode) {
    key_added_while_decode_pending_ = true;
    return;
  }

  if (state_ == kWaitingForKey) {
    MEDIA_LOG(INFO, media_log_) << GetDisplayName()
                                << ": key added, resuming decode";
    state_ = kPendingDecode;
    DecodePendingBuffer();
  }
}

void DecryptingVideoDecoder::DoReset() {
  DCHECK(init_cb_.is_null());
  DCHECK(decode_cb_.is_null());
  state_ = kIdle;
  base::ResetAndReturn(&reset_cb_).Run();
}

}  // namespace media
