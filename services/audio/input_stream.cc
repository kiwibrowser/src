// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/input_stream.h"

#include <string>
#include <utility>

#include "base/bind_helpers.h"
#include "base/trace_event/trace_event.h"
#include "media/audio/audio_input_sync_writer.h"
#include "media/audio/audio_manager.h"
#include "media/base/audio_parameters.h"
#include "media/base/user_input_monitor.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/audio/user_input_monitor.h"

namespace audio {

namespace {
const int kMaxInputChannels = 3;
}

InputStream::InputStream(CreatedCallback created_callback,
                         DeleteCallback delete_callback,
                         media::mojom::AudioInputStreamRequest request,
                         media::mojom::AudioInputStreamClientPtr client,
                         media::mojom::AudioInputStreamObserverPtr observer,
                         media::mojom::AudioLogPtr log,
                         media::AudioManager* audio_manager,
                         std::unique_ptr<UserInputMonitor> user_input_monitor,
                         const std::string& device_id,
                         const media::AudioParameters& params,
                         uint32_t shared_memory_count,
                         bool enable_agc)
    : id_(base::UnguessableToken::Create()),
      binding_(this, std::move(request)),
      client_(std::move(client)),
      observer_(std::move(observer)),
      log_(log ? media::mojom::ThreadSafeAudioLogPtr::Create(std::move(log))
               : nullptr),
      created_callback_(std::move(created_callback)),
      delete_callback_(std::move(delete_callback)),
      foreign_socket_(),
      writer_(media::AudioInputSyncWriter::Create(
          log_ ? base::BindRepeating(&media::mojom::AudioLog::OnLogMessage,
                                     base::Unretained(log_->get()))
               : base::DoNothing(),
          shared_memory_count,
          params,
          &foreign_socket_)),
      user_input_monitor_(std::move(user_input_monitor)),
      weak_factory_(this) {
  DCHECK(audio_manager);
  DCHECK(binding_.is_bound());
  DCHECK(client_.is_bound());
  DCHECK(created_callback_);
  DCHECK(delete_callback_);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("audio", "audio::InputStream", this);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN2("audio", "InputStream", this, "device id",
                                    device_id, "params",
                                    params.AsHumanReadableString());

  // |this| owns these objects, so unretained is safe.
  base::RepeatingClosure error_handler = base::BindRepeating(
      &InputStream::OnStreamError, base::Unretained(this), false);
  binding_.set_connection_error_handler(error_handler);
  client_.set_connection_error_handler(error_handler);

  if (observer_)
    observer_.set_connection_error_handler(std::move(error_handler));

  if (log_)
    log_->get()->OnCreated(params, device_id);

  // Only MONO, STEREO and STEREO_AND_KEYBOARD_MIC channel layouts are expected,
  // see AudioManagerBase::MakeAudioInputStream().
  if (!params.IsValid() || (params.channels() > kMaxInputChannels)) {
    OnStreamError(true);
    return;
  }

  if (!writer_) {
    OnStreamError(true);
    return;
  }

  controller_ = media::AudioInputController::Create(
      audio_manager, this, writer_.get(), user_input_monitor_.get(), params,
      device_id, enable_agc);
}

InputStream::~InputStream() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);

  if (log_)
    log_->get()->OnClosed();

  if (observer_)
    observer_.ResetWithReason(
        static_cast<uint32_t>(media::mojom::AudioInputStreamObserver::
                                  DisconnectReason::kTerminatedByClient),
        std::string());

  if (created_callback_) {
    // Didn't manage to create the stream. Call the callback anyways as mandated
    // by mojo.
    std::move(created_callback_).Run(nullptr, false, base::nullopt);
  }

  if (!controller_) {
    // Didn't initialize properly, nothing to clean up.
    return;
  }

  // TODO(https://crbug.com/803102): remove AudioInputController::Close() after
  // content/ streams are removed, destructor should suffice.
  controller_->Close(base::OnceClosure());

  TRACE_EVENT_NESTABLE_ASYNC_END0("audio", "InputStream", this);
  TRACE_EVENT_NESTABLE_ASYNC_END0("audio", "audio::InputStream", this);
}

void InputStream::SetOutputDeviceForAec(const std::string& output_device_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  DCHECK(controller_);
  controller_->SetOutputDeviceForAec(output_device_id);
  if (log_)
    log_->get()->OnLogMessage("SetOutputDeviceForAec");
}

void InputStream::Record() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  DCHECK(controller_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT0("audio", "Record", this);

  controller_->Record();
  if (observer_)
    observer_->DidStartRecording();
  if (log_)
    log_->get()->OnStarted();
}

void InputStream::SetVolume(double volume) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  DCHECK(controller_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1("audio", "SetVolume", this, "volume",
                                      volume);

  if (volume < 0 || volume > 1) {
    mojo::ReportBadMessage("Invalid volume");
    OnStreamError(true);
    return;
  }

  controller_->SetVolume(volume);
  if (log_)
    log_->get()->OnSetVolume(volume);
}

void InputStream::OnCreated(bool initially_muted) {
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT0("audio", "Created", this);
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);

  base::ReadOnlySharedMemoryRegion shared_memory_region =
      writer_->TakeSharedMemoryRegion();
  if (!shared_memory_region.IsValid()) {
    OnStreamError(true);
    return;
  }

  mojo::ScopedSharedBufferHandle buffer_handle =
      mojo::WrapReadOnlySharedMemoryRegion(std::move(shared_memory_region));

  mojo::ScopedHandle socket_handle =
      mojo::WrapPlatformFile(foreign_socket_.Release());

  DCHECK(buffer_handle.is_valid());
  DCHECK(socket_handle.is_valid());

  std::move(created_callback_)
      .Run({base::in_place, std::move(buffer_handle), std::move(socket_handle)},
           initially_muted, id_);
}

void InputStream::OnError(media::AudioInputController::ErrorCode error_code) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT0("audio", "Error", this);

  client_->OnError();
  if (log_)
    log_->get()->OnError();
  OnStreamError(true);
}

void InputStream::OnLog(base::StringPiece message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (log_)
    log_->get()->OnLogMessage(message.as_string());
}

void InputStream::OnMuted(bool is_muted) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  client_->OnMutedStateChanged(is_muted);
}

void InputStream::OnStreamError(bool signalPlatformError) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  TRACE_EVENT_NESTABLE_ASYNC_INSTANT0("audio", "OnStreamError", this);

  if (signalPlatformError && observer_) {
    observer_.ResetWithReason(
        static_cast<uint32_t>(media::mojom::AudioInputStreamObserver::
                                  DisconnectReason::kPlatformError),
        std::string());
  }

  // Defer callback so we're not destructed while in the constructor.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&InputStream::CallDeleter, weak_factory_.GetWeakPtr()));
  binding_.Close();
}

void InputStream::CallDeleter() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);

  std::move(delete_callback_).Run(this);
}

}  // namespace audio
