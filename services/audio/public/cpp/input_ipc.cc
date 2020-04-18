// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/input_ipc.h"

#include <utility>

#include "base/bind_helpers.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/audio/public/mojom/constants.mojom.h"

namespace audio {

InputIPC::InputIPC(std::unique_ptr<service_manager::Connector> connector,
                   const std::string& device_id,
                   media::mojom::AudioLogPtr log)
    : stream_(),
      stream_client_binding_(this),
      device_id_(std::move(device_id)),
      stream_factory_(),
      stream_factory_info_(),
      log_(std::move(log)),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(connector);

  connector->BindInterface(audio::mojom::kServiceName,
                           mojo::MakeRequest(&stream_factory_info_));
}

InputIPC::~InputIPC() = default;

void InputIPC::CreateStream(media::AudioInputIPCDelegate* delegate,
                            const media::AudioParameters& params,
                            bool automatic_gain_control,
                            uint32_t total_segments) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate);
  DCHECK(!delegate_);

  delegate_ = delegate;

  if (!stream_factory_.is_bound())
    stream_factory_.Bind(std::move(stream_factory_info_));

  media::mojom::AudioInputStreamRequest stream_request =
      mojo::MakeRequest(&stream_);

  media::mojom::AudioInputStreamClientPtr client;
  stream_client_binding_.Bind(mojo::MakeRequest(&client));

  // Unretained is safe because we own the binding.
  stream_client_binding_.set_connection_error_handler(
      base::BindOnce(&InputIPC::OnError, base::Unretained(this)));

  // For now we don't care about key presses, so we pass a invalid buffer.
  mojo::ScopedSharedBufferHandle invalid_key_press_count_buffer;

  stream_factory_->CreateInputStream(
      std::move(stream_request), std::move(client), nullptr,
      log_ ? std::move(log_) : nullptr, device_id_, params, total_segments,
      automatic_gain_control, std::move(invalid_key_press_count_buffer),
      base::BindOnce(&InputIPC::StreamCreated, weak_factory_.GetWeakPtr()));
}

void InputIPC::StreamCreated(
    media::mojom::AudioDataPipePtr data_pipe,
    bool initially_muted,
    const base::Optional<base::UnguessableToken>& stream_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate_);

  if (data_pipe.is_null()) {
    OnError();
    return;
  }

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

void InputIPC::RecordStream() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stream_.is_bound());
  stream_->Record();
}

void InputIPC::SetVolume(double volume) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stream_.is_bound());
  stream_->SetVolume(volume);
}

void InputIPC::SetOutputDeviceForAec(const std::string& output_device_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(stream_factory_.is_bound());
  // Loopback streams have no stream ids and cannot be use echo cancellation
  if (stream_id_.has_value())
    stream_factory_->AssociateInputAndOutputForAec(*stream_id_,
                                                   output_device_id);
}

void InputIPC::CloseStream() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  delegate_ = nullptr;
  if (stream_client_binding_.is_bound())
    stream_client_binding_.Close();
  stream_.reset();
}

void InputIPC::OnError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate_);
  delegate_->OnError();
}

void InputIPC::OnMutedStateChanged(bool is_muted) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate_);
  delegate_->OnMuted(is_muted);
}

}  // namespace audio
