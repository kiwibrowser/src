// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/mojo_audio_output_ipc.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "media/audio/audio_device_description.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace content {

namespace {

void TrivialAuthorizedCallback(media::OutputDeviceStatus,
                               const media::AudioParameters&,
                               const std::string&) {}

}  // namespace

MojoAudioOutputIPC::MojoAudioOutputIPC(
    FactoryAccessorCB factory_accessor,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : factory_accessor_(std::move(factory_accessor)),
      binding_(this),
      io_task_runner_(std::move(io_task_runner)),
      weak_factory_(this) {}

MojoAudioOutputIPC::~MojoAudioOutputIPC() {
  DCHECK(!AuthorizationRequested() && !StreamCreationRequested())
      << "CloseStream must be called before destructing the AudioOutputIPC";
  // No sequence check.
  // Destructing |weak_factory_| on any sequence is safe since it's not used
  // after the final call to CloseStream, where its pointers are invalidated.
}

void MojoAudioOutputIPC::RequestDeviceAuthorization(
    media::AudioOutputIPCDelegate* delegate,
    int session_id,
    const std::string& device_id) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(delegate);
  DCHECK(!delegate_);
  DCHECK(!AuthorizationRequested());
  DCHECK(!StreamCreationRequested());
  delegate_ = delegate;

  // We wrap the callback in a ScopedCallbackRunner to detect the case when the
  // mojo connection is terminated prior to receiving the response. In this
  // case, the callback runner will be destructed and call
  // ReceivedDeviceAuthorization with an error.
  DoRequestDeviceAuthorization(
      session_id, device_id,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&MojoAudioOutputIPC::ReceivedDeviceAuthorization,
                         weak_factory_.GetWeakPtr(), base::TimeTicks::Now()),
          media::OutputDeviceStatus::OUTPUT_DEVICE_STATUS_ERROR_INTERNAL,
          media::AudioParameters::UnavailableDeviceParams(), std::string()));
}

void MojoAudioOutputIPC::CreateStream(media::AudioOutputIPCDelegate* delegate,
                                      const media::AudioParameters& params) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(delegate);
  DCHECK(!StreamCreationRequested());
  if (!AuthorizationRequested()) {
    DCHECK(!delegate_);
    delegate_ = delegate;
    // No authorization requested yet. Request one for the default device.
    // Since the delegate didn't explicitly request authorization, we shouldn't
    // send a callback to it.
    DoRequestDeviceAuthorization(
        0, media::AudioDeviceDescription::kDefaultDeviceId,
        base::BindOnce(&TrivialAuthorizedCallback));
  }

  DCHECK_EQ(delegate_, delegate);
  // Since the creation callback won't fire if the provider binding is gone
  // and |this| owns |stream_provider_|, unretained is safe.
  stream_creation_start_time_ = base::TimeTicks::Now();
  media::mojom::AudioOutputStreamProviderClientPtr client_ptr;
  binding_.Bind(mojo::MakeRequest(&client_ptr));
  // Unretained is safe because |this| owns |binding_|.
  binding_.set_connection_error_with_reason_handler(
      base::BindOnce(&MojoAudioOutputIPC::ProviderClientBindingDisconnected,
                     base::Unretained(this)));
  stream_provider_->Acquire(params, std::move(client_ptr));
}

void MojoAudioOutputIPC::PlayStream() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  expected_state_ = kPlaying;
  if (stream_.is_bound())
    stream_->Play();
}

void MojoAudioOutputIPC::PauseStream() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  expected_state_ = kPaused;
  if (stream_.is_bound())
    stream_->Pause();
}

void MojoAudioOutputIPC::CloseStream() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  stream_provider_.reset();
  stream_.reset();
  binding_.Close();
  delegate_ = nullptr;
  expected_state_ = kPaused;
  volume_ = base::nullopt;

  // Cancel any pending callbacks for this stream.
  weak_factory_.InvalidateWeakPtrs();
}

void MojoAudioOutputIPC::SetVolume(double volume) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  volume_ = volume;
  if (stream_.is_bound())
    stream_->SetVolume(volume);
  // else volume is set when the stream is created.
}

void MojoAudioOutputIPC::ProviderClientBindingDisconnected(
    uint32_t disconnect_reason,
    const std::string& description) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(delegate_);
  if (disconnect_reason ==
      static_cast<uint32_t>(media::mojom::AudioOutputStreamObserver::
                                DisconnectReason::kPlatformError)) {
    delegate_->OnError();
  }
  // Otherwise, disconnection was due to the frame owning |this| being
  // destructed or having a navigation. In this case, |this| will soon be
  // cleaned up.
}

bool MojoAudioOutputIPC::AuthorizationRequested() const {
  return stream_provider_.is_bound();
}

bool MojoAudioOutputIPC::StreamCreationRequested() const {
  return binding_.is_bound();
}

media::mojom::AudioOutputStreamProviderRequest
MojoAudioOutputIPC::MakeProviderRequest() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(!AuthorizationRequested());
  media::mojom::AudioOutputStreamProviderRequest request =
      mojo::MakeRequest(&stream_provider_);

  // Don't set a connection error handler.
  // There are three possible reasons for a connection error.
  // 1. The connection is broken before authorization was completed. In this
  //    case, the ScopedCallbackRunner wrapping the callback will call the
  //    callback with failure.
  // 2. The connection is broken due to authorization being denied. In this
  //    case, the callback was called with failure first, so the state of the
  //    stream provider is irrelevant.
  // 3. The connection was broken after authorization succeeded. This is because
  //    of the frame owning this stream being destructed, and this object will
  //    be cleaned up soon.
  return request;
}

void MojoAudioOutputIPC::DoRequestDeviceAuthorization(
    int session_id,
    const std::string& device_id,
    AuthorizationCB callback) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  auto* factory = factory_accessor_.Run();
  if (!factory) {
    LOG(ERROR) << "MojoAudioOutputIPC failed to acquire factory";

    // Create a provider request for consistency with the normal case.
    MakeProviderRequest();
    // Resetting the callback asynchronously ensures consistent behaviour with
    // when the factory is destroyed before reply, i.e. calling
    // OnDeviceAuthorized with ERROR_INTERNAL in the normal case.
    // The AudioOutputIPCDelegate will call CloseStream as necessary.
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce([](AuthorizationCB cb) {}, std::move(callback)));
    return;
  }

  static_assert(sizeof(int) == sizeof(int32_t),
                "sizeof(int) == sizeof(int32_t)");
  factory->RequestDeviceAuthorization(MakeProviderRequest(), session_id,
                                      device_id, std::move(callback));
}

void MojoAudioOutputIPC::ReceivedDeviceAuthorization(
    base::TimeTicks auth_start_time,
    media::OutputDeviceStatus status,
    const media::AudioParameters& params,
    const std::string& device_id) const {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(delegate_);

  // Times over 15 s should be very rare, so we don't lose interesting data by
  // making it the upper limit.
  UMA_HISTOGRAM_CUSTOM_TIMES("Media.Audio.Render.OutputDeviceAuthorizationTime",
                             base::TimeTicks::Now() - auth_start_time,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromSeconds(15), 100);

  delegate_->OnDeviceAuthorized(status, params, device_id);
}

void MojoAudioOutputIPC::Created(media::mojom::AudioOutputStreamPtr stream,
                                 media::mojom::AudioDataPipePtr data_pipe) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(delegate_);

  UMA_HISTOGRAM_TIMES("Media.Audio.Render.OutputDeviceStreamCreationTime",
                      base::TimeTicks::Now() - stream_creation_start_time_);
  stream_ = std::move(stream);

  base::PlatformFile socket_handle;
  auto result =
      mojo::UnwrapPlatformFile(std::move(data_pipe->socket), &socket_handle);
  DCHECK_EQ(result, MOJO_RESULT_OK);

  base::SharedMemoryHandle memory_handle;
  mojo::UnwrappedSharedMemoryHandleProtection protection;
  size_t memory_length = 0;
  result = mojo::UnwrapSharedMemoryHandle(std::move(data_pipe->shared_memory),
                                          &memory_handle, &memory_length,
                                          &protection);
  DCHECK_EQ(result, MOJO_RESULT_OK);
  DCHECK_EQ(protection,
            mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);

  delegate_->OnStreamCreated(memory_handle, socket_handle,
                             expected_state_ == kPlaying);

  if (volume_)
    stream_->SetVolume(*volume_);
  if (expected_state_ == kPlaying)
    stream_->Play();
}

}  // namespace content
