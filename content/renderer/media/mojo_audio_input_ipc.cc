// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/mojo_audio_input_ipc.h"

#include <utility>

#include "base/bind_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "media/audio/audio_device_description.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace content {

MojoAudioInputIPC::MojoAudioInputIPC(StreamCreatorCB stream_creator,
                                     StreamAssociatorCB stream_associator)
    : stream_creator_(std::move(stream_creator)),
      stream_associator_(std::move(stream_associator)),
      stream_client_binding_(this),
      factory_client_binding_(this),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(stream_creator_);
  DCHECK(stream_associator_);
}

MojoAudioInputIPC::~MojoAudioInputIPC() = default;

void MojoAudioInputIPC::CreateStream(media::AudioInputIPCDelegate* delegate,
                                     const media::AudioParameters& params,
                                     bool automatic_gain_control,
                                     uint32_t total_segments) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate);
  DCHECK(!delegate_);

  delegate_ = delegate;

  mojom::RendererAudioInputStreamFactoryClientPtr client;
  factory_client_binding_.Bind(mojo::MakeRequest(&client));
  factory_client_binding_.set_connection_error_handler(base::BindOnce(
      &media::AudioInputIPCDelegate::OnError, base::Unretained(delegate_)));

  stream_creation_start_time_ = base::TimeTicks::Now();
  stream_creator_.Run(std::move(client), params, automatic_gain_control,
                      total_segments);
}

void MojoAudioInputIPC::RecordStream() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stream_.is_bound());
  stream_->Record();
}

void MojoAudioInputIPC::SetVolume(double volume) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stream_.is_bound());
  stream_->SetVolume(volume);
}

void MojoAudioInputIPC::SetOutputDeviceForAec(
    const std::string& output_device_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stream_) << "Can only be called after the stream has been created";
  // Loopback streams have no stream ids and cannot be use echo cancellation
  if (stream_id_.has_value())
    stream_associator_.Run(*stream_id_, output_device_id);
}

void MojoAudioInputIPC::CloseStream() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  delegate_ = nullptr;
  if (factory_client_binding_.is_bound())
    factory_client_binding_.Unbind();
  if (stream_client_binding_.is_bound())
    stream_client_binding_.Unbind();
  stream_.reset();
}

void MojoAudioInputIPC::StreamCreated(
    media::mojom::AudioInputStreamPtr stream,
    media::mojom::AudioInputStreamClientRequest stream_client_request,
    media::mojom::AudioDataPipePtr data_pipe,
    bool initially_muted,
    const base::Optional<base::UnguessableToken>& stream_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate_);
  DCHECK(!stream_);
  DCHECK(!stream_client_binding_.is_bound());

  UMA_HISTOGRAM_TIMES("Media.Audio.Render.InputDeviceStreamCreationTime",
                      base::TimeTicks::Now() - stream_creation_start_time_);

  stream_ = std::move(stream);
  stream_client_binding_.Bind(std::move(stream_client_request));

  // Keep the stream_id, if we get one. Regular input stream have stream ids,
  // but Loopback streams do not.
  stream_id_ = stream_id;

  base::PlatformFile socket_handle;
  auto result =
      mojo::UnwrapPlatformFile(std::move(data_pipe->socket), &socket_handle);
  DCHECK_EQ(result, MOJO_RESULT_OK);

  base::SharedMemoryHandle memory_handle;
  mojo::UnwrappedSharedMemoryHandleProtection protection;
  result = mojo::UnwrapSharedMemoryHandle(std::move(data_pipe->shared_memory),
                                          &memory_handle, nullptr, &protection);
  DCHECK_EQ(result, MOJO_RESULT_OK);
  DCHECK_EQ(protection, mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);

  delegate_->OnStreamCreated(memory_handle, socket_handle, initially_muted);
}

void MojoAudioInputIPC::OnError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate_);
  delegate_->OnError();
}

void MojoAudioInputIPC::OnMutedStateChanged(bool is_muted) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate_);
  delegate_->OnMuted(is_muted);
}

}  // namespace content
