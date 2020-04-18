// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_output_device.h"

#include <stddef.h>
#include <stdint.h>

#include <cmath>
#include <utility>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/timer.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_output_controller.h"
#include "media/base/limits.h"

namespace media {

// Takes care of invoking the render callback on the audio thread.
// An instance of this class is created for each capture stream in
// OnStreamCreated().
class AudioOutputDevice::AudioThreadCallback
    : public AudioDeviceThread::Callback {
 public:
  AudioThreadCallback(const AudioParameters& audio_parameters,
                      base::SharedMemoryHandle memory,
                      AudioRendererSink::RenderCallback* render_callback);
  ~AudioThreadCallback() override;

  void MapSharedMemory() override;

  // Called whenever we receive notifications about pending data.
  void Process(uint32_t control_signal) override;

  // Returns whether the current thread is the audio device thread or not.
  // Will always return true if DCHECKs are not enabled.
  bool CurrentThreadIsAudioDeviceThread();

  // Sets |first_play_start_time_| to the current time unless it's already set,
  // in which case it's a no-op. The first call to this method MUST have
  // completed by the time we recieve our first Process() callback to avoid
  // data races.
  void InitializePlayStartTime();

 private:
  const base::TimeTicks start_time_;
  // If set, this is used to record the startup duration UMA stat.
  base::Optional<base::TimeTicks> first_play_start_time_;
  AudioRendererSink::RenderCallback* render_callback_;
  std::unique_ptr<AudioBus> output_bus_;
  uint64_t callback_num_;

  DISALLOW_COPY_AND_ASSIGN(AudioThreadCallback);
};

AudioOutputDevice::AudioOutputDevice(
    std::unique_ptr<AudioOutputIPC> ipc,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    int session_id,
    const std::string& device_id,
    base::TimeDelta authorization_timeout)
    : io_task_runner_(io_task_runner),
      callback_(NULL),
      ipc_(std::move(ipc)),
      state_(IDLE),
      session_id_(session_id),
      device_id_(device_id),
      stopping_hack_(false),
      did_receive_auth_(base::WaitableEvent::ResetPolicy::MANUAL,
                        base::WaitableEvent::InitialState::NOT_SIGNALED),
      output_params_(AudioParameters::UnavailableDeviceParams()),
      device_status_(OUTPUT_DEVICE_STATUS_ERROR_INTERNAL),
      auth_timeout_(authorization_timeout) {
  DCHECK(ipc_);
  DCHECK(io_task_runner_);
}

void AudioOutputDevice::Initialize(const AudioParameters& params,
                                   RenderCallback* callback) {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AudioOutputDevice::InitializeOnIOThread, this,
                                params, callback));
}

void AudioOutputDevice::InitializeOnIOThread(const AudioParameters& params,
                                             RenderCallback* callback) {
  DCHECK(!callback_) << "Calling Initialize() twice?";
  DCHECK(params.IsValid());
  audio_parameters_ = params;
  callback_ = callback;
}

AudioOutputDevice::~AudioOutputDevice() {
#if DCHECK_IS_ON()
  // Make sure we've stopped the stream properly before destructing |this|.
  DCHECK(audio_thread_lock_.Try());
  DCHECK_EQ(state_, IDLE);
  DCHECK(!audio_thread_);
  DCHECK(!audio_callback_);
  DCHECK(!stopping_hack_);
  audio_thread_lock_.Release();
#endif  // DCHECK_IS_ON()
}

void AudioOutputDevice::RequestDeviceAuthorization() {
  TRACE_EVENT0("audio", "AudioOutputDevice::RequestDeviceAuthorization");
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioOutputDevice::RequestDeviceAuthorizationOnIOThread,
                     this));
}

void AudioOutputDevice::Start() {
  TRACE_EVENT0("audio", "AudioOutputDevice::Start");
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioOutputDevice::CreateStreamOnIOThread, this));
}

void AudioOutputDevice::Stop() {
  TRACE_EVENT0("audio", "AudioOutputDevice::Stop");
  {
    base::AutoLock auto_lock(audio_thread_lock_);
    audio_thread_.reset();
    stopping_hack_ = true;
  }

  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AudioOutputDevice::ShutDownOnIOThread, this));
}

void AudioOutputDevice::Play() {
  TRACE_EVENT0("audio", "AudioOutputDevice::Play");
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AudioOutputDevice::PlayOnIOThread, this));
}

void AudioOutputDevice::Pause() {
  TRACE_EVENT0("audio", "AudioOutputDevice::Pause");
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AudioOutputDevice::PauseOnIOThread, this));
}

bool AudioOutputDevice::SetVolume(double volume) {
  TRACE_EVENT1("audio", "AudioOutputDevice::Pause", "volume", volume);

  if (volume < 0 || volume > 1.0)
    return false;

  return io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioOutputDevice::SetVolumeOnIOThread, this, volume));
}

OutputDeviceInfo AudioOutputDevice::GetOutputDeviceInfo() {
  TRACE_EVENT0("audio", "AudioOutputDevice::GetOutputDeviceInfo");
  DCHECK(!io_task_runner_->BelongsToCurrentThread());

  did_receive_auth_.Wait();
  return OutputDeviceInfo(AudioDeviceDescription::UseSessionIdToSelectDevice(
                              session_id_, device_id_)
                              ? matched_device_id_
                              : device_id_,
                          device_status_, output_params_);
}

bool AudioOutputDevice::IsOptimizedForHardwareParameters() {
  return true;
}

bool AudioOutputDevice::CurrentThreadIsRenderingThread() {
  // Since this function is supposed to be called on the rendering thread,
  // it's safe to access |audio_callback_| here. It will always be valid when
  // the rendering thread is running.
  return audio_callback_->CurrentThreadIsAudioDeviceThread();
}

void AudioOutputDevice::RequestDeviceAuthorizationOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, IDLE);

  state_ = AUTHORIZATION_REQUESTED;
  ipc_->RequestDeviceAuthorization(this, session_id_, device_id_);

  if (auth_timeout_ > base::TimeDelta()) {
    // Create the timer on the thread it's used on. It's guaranteed to be
    // deleted on the same thread since users must call Stop() before deleting
    // AudioOutputDevice; see ShutDownOnIOThread().
    auth_timeout_action_.reset(new base::OneShotTimer());
    auth_timeout_action_->Start(
        FROM_HERE, auth_timeout_,
        base::BindRepeating(&AudioOutputDevice::OnDeviceAuthorized, this,
                            OUTPUT_DEVICE_STATUS_ERROR_TIMED_OUT,
                            media::AudioParameters(), std::string()));
  }
}

void AudioOutputDevice::CreateStreamOnIOThread() {
  TRACE_EVENT0("audio", "AudioOutputDevice::Create");
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(callback_) << "Initialize hasn't been called";
  DCHECK_NE(state_, STREAM_CREATION_REQUESTED);

  if (!ipc_) {
    NotifyRenderCallbackOfError();
    return;
  }

  if (state_ == IDLE && !(did_receive_auth_.IsSignaled() && device_id_.empty()))
    RequestDeviceAuthorizationOnIOThread();

  ipc_->CreateStream(this, audio_parameters_);
  // By default, start playing right away.
  ipc_->PlayStream();
  state_ = STREAM_CREATION_REQUESTED;
}

void AudioOutputDevice::PlayOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (audio_callback_)
    audio_callback_->InitializePlayStartTime();

  if (ipc_)
    ipc_->PlayStream();
}

void AudioOutputDevice::PauseOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  if (ipc_)
    ipc_->PauseStream();
}

void AudioOutputDevice::ShutDownOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  if (ipc_)
    ipc_->CloseStream();

  state_ = IDLE;

  // Destoy the timer on the thread it's used on.
  auth_timeout_action_.reset();

  // We can run into an issue where ShutDownOnIOThread is called right after
  // OnStreamCreated is called in cases where Start/Stop are called before we
  // get the OnStreamCreated callback.  To handle that corner case, we call
  // Stop(). In most cases, the thread will already be stopped.
  //
  // Another situation is when the IO thread goes away before Stop() is called
  // in which case, we cannot use the message loop to close the thread handle
  // and can't rely on the main thread existing either.
  base::AutoLock auto_lock_(audio_thread_lock_);
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  audio_thread_.reset();
  audio_callback_.reset();
  stopping_hack_ = false;

  UMA_HISTOGRAM_BOOLEAN("Media.Audio.Render.StreamCallbackError",
                        had_callback_error_);
}

void AudioOutputDevice::SetVolumeOnIOThread(double volume) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->SetVolume(volume);
}

void AudioOutputDevice::OnError() {
  TRACE_EVENT0("audio", "AudioOutputDevice::OnError");

  DCHECK(io_task_runner_->BelongsToCurrentThread());

  // Do nothing if the stream has been closed.
  if (state_ == IDLE)
    return;

  had_callback_error_ = true;
  // Don't dereference the callback object if the audio thread
  // is stopped or stopping.  That could mean that the callback
  // object has been deleted.
  // TODO(tommi): Add an explicit contract for clearing the callback
  // object.  Possibly require calling Initialize again or provide
  // a callback object via Start() and clear it in Stop().
  NotifyRenderCallbackOfError();
}

void AudioOutputDevice::OnDeviceAuthorized(
    OutputDeviceStatus device_status,
    const media::AudioParameters& output_params,
    const std::string& matched_device_id) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  auth_timeout_action_.reset();

  // Do nothing if late authorization is received after timeout.
  if (!ipc_)
    return;

  UMA_HISTOGRAM_BOOLEAN("Media.Audio.Render.OutputDeviceAuthorizationTimedOut",
                        device_status == OUTPUT_DEVICE_STATUS_ERROR_TIMED_OUT);
  LOG_IF(WARNING, device_status == OUTPUT_DEVICE_STATUS_ERROR_TIMED_OUT)
      << "Output device authorization timed out";

  // It may happen that a second authorization is received as a result to a
  // call to Start() after Stop(). If the status for the second authorization
  // differs from the first, it will not be reflected in |device_status_|
  // to avoid a race.
  // This scenario is unlikely. If it occurs, the new value will be
  // different from OUTPUT_DEVICE_STATUS_OK, so the AudioOutputDevice
  // will enter the |ipc_| == nullptr state anyway, which is the safe thing to
  // do. This is preferable to holding a lock.
  if (!did_receive_auth_.IsSignaled()) {
    device_status_ = device_status;
    UMA_HISTOGRAM_ENUMERATION("Media.Audio.Render.OutputDeviceStatus",
                              device_status, OUTPUT_DEVICE_STATUS_MAX + 1);
  }

  if (device_status == OUTPUT_DEVICE_STATUS_OK) {
    TRACE_EVENT0("audio", "AudioOutputDevice authorized");

    if (!did_receive_auth_.IsSignaled()) {
      output_params_ = output_params;

      // It's possible to not have a matched device obtained via session id. It
      // means matching output device through |session_id_| failed and the
      // default device is used.
      DCHECK(AudioDeviceDescription::UseSessionIdToSelectDevice(session_id_,
                                                                device_id_) ||
             matched_device_id_.empty());
      matched_device_id_ = matched_device_id;

      DVLOG(1) << "AudioOutputDevice authorized, session_id: " << session_id_
               << ", device_id: " << device_id_
               << ", matched_device_id: " << matched_device_id_;

      did_receive_auth_.Signal();
    }
  } else {
    TRACE_EVENT1("audio", "AudioOutputDevice not authorized", "auth status",
                 device_status_);

    // Closing IPC forces a Signal(), so no clients are locked waiting
    // indefinitely after this method returns.
    ipc_->CloseStream();
    OnIPCClosed();

    NotifyRenderCallbackOfError();
  }
}

void AudioOutputDevice::OnStreamCreated(base::SharedMemoryHandle handle,
                                        base::SyncSocket::Handle socket_handle,
                                        bool playing_automatically) {
  TRACE_EVENT0("audio", "AudioOutputDevice::OnStreamCreated")

  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(base::SharedMemory::IsHandleValid(handle));
#if defined(OS_WIN)
  DCHECK(socket_handle);
#else
  DCHECK_GE(socket_handle, 0);
#endif
  DCHECK_GT(handle.GetSize(), 0u);

  if (state_ != STREAM_CREATION_REQUESTED)
    return;

  // We can receive OnStreamCreated() on the IO thread after the client has
  // called Stop() but before ShutDownOnIOThread() is processed. In such a
  // situation |callback_| might point to freed memory. Instead of starting
  // |audio_thread_| do nothing and wait for ShutDownOnIOThread() to get called.
  //
  // TODO(scherkus): The real fix is to have sane ownership semantics. The fact
  // that |callback_| (which should own and outlive this object!) can point to
  // freed memory is a mess. AudioRendererSink should be non-refcounted so that
  // owners (WebRtcAudioDeviceImpl, AudioRendererImpl, etc...) can Stop() and
  // delete as they see fit. AudioOutputDevice should internally use WeakPtr
  // to handle teardown and thread hopping. See http://crbug.com/151051 for
  // details.
  {
    base::AutoLock auto_lock(audio_thread_lock_);
    if (stopping_hack_)
      return;

    DCHECK(!audio_thread_);
    DCHECK(!audio_callback_);

    audio_callback_.reset(new AudioOutputDevice::AudioThreadCallback(
        audio_parameters_, handle, callback_));
    if (playing_automatically)
      audio_callback_->InitializePlayStartTime();
    audio_thread_.reset(new AudioDeviceThread(
        audio_callback_.get(), socket_handle, "AudioOutputDevice"));
  }
}

void AudioOutputDevice::OnIPCClosed() {
  TRACE_EVENT0("audio", "AudioOutputDevice::OnIPCClosed");
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  ipc_.reset();
  state_ = IDLE;

  // Signal to unblock any blocked threads waiting for parameters
  did_receive_auth_.Signal();
}

void AudioOutputDevice::NotifyRenderCallbackOfError() {
  TRACE_EVENT0("audio", "AudioOutputDevice::NotifyRenderCallbackOfError");
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  base::AutoLock auto_lock(audio_thread_lock_);
  // Avoid signaling error if Initialize() hasn't been called yet, or if
  // Stop() has already been called.
  if (callback_ && !stopping_hack_)
    callback_->OnRenderError();
}

// AudioOutputDevice::AudioThreadCallback

AudioOutputDevice::AudioThreadCallback::AudioThreadCallback(
    const AudioParameters& audio_parameters,
    base::SharedMemoryHandle memory,
    AudioRendererSink::RenderCallback* render_callback)
    : AudioDeviceThread::Callback(
          audio_parameters,
          memory,
          /*read only*/ false,
          ComputeAudioOutputBufferSize(audio_parameters),
          /*segment count*/ 1),
      start_time_(base::TimeTicks::Now()),
      first_play_start_time_(base::nullopt),
      render_callback_(render_callback),
      callback_num_(0) {}

AudioOutputDevice::AudioThreadCallback::~AudioThreadCallback() {
  UMA_HISTOGRAM_LONG_TIMES("Media.Audio.Render.OutputStreamDuration",
                           base::TimeTicks::Now() - start_time_);
}

void AudioOutputDevice::AudioThreadCallback::MapSharedMemory() {
  CHECK_EQ(total_segments_, 1u);
  CHECK(shared_memory_.Map(memory_length_));

  AudioOutputBuffer* buffer =
      reinterpret_cast<AudioOutputBuffer*>(shared_memory_.memory());
  output_bus_ = AudioBus::WrapMemory(audio_parameters_, buffer->audio);
  output_bus_->set_is_bitstream_format(audio_parameters_.IsBitstreamFormat());
}

// Called whenever we receive notifications about pending data.
void AudioOutputDevice::AudioThreadCallback::Process(uint32_t control_signal) {
  callback_num_++;

  // Read and reset the number of frames skipped.
  AudioOutputBuffer* buffer =
      reinterpret_cast<AudioOutputBuffer*>(shared_memory_.memory());
  uint32_t frames_skipped = buffer->params.frames_skipped;
  buffer->params.frames_skipped = 0;

  base::TimeDelta delay =
      base::TimeDelta::FromMicroseconds(buffer->params.delay_us);

  base::TimeTicks delay_timestamp =
      base::TimeTicks() +
      base::TimeDelta::FromMicroseconds(buffer->params.delay_timestamp_us);

  TRACE_EVENT_BEGIN2("audio", "AudioOutputDevice::FireRenderCallback",
                     "callback_num", callback_num_, "frames skipped",
                     frames_skipped);
  DVLOG(4) << __func__ << " delay:" << delay << " delay_timestamp:" << delay
           << " frames_skipped:" << frames_skipped;

  // When playback starts, we get an immediate callback to Process to make sure
  // that we have some data, we'll get another one after the device is awake and
  // ingesting data, which is what we want to track with this trace.
  if (callback_num_ == 2) {
    if (first_play_start_time_) {
      UMA_HISTOGRAM_TIMES("Media.Audio.Render.OutputDeviceStartTime",
                          base::TimeTicks::Now() - *first_play_start_time_);
    }
    TRACE_EVENT_ASYNC_END0("audio", "StartingPlayback", this);
  }

  // Update the audio-delay measurement, inform about the number of skipped
  // frames, and ask client to render audio.  Since |output_bus_| is wrapping
  // the shared memory the Render() call is writing directly into the shared
  // memory.
  render_callback_->Render(delay, delay_timestamp, frames_skipped,
                           output_bus_.get());

  if (audio_parameters_.IsBitstreamFormat()) {
    buffer->params.bitstream_data_size = output_bus_->GetBitstreamDataSize();
    buffer->params.bitstream_frames = output_bus_->GetBitstreamFrames();
  }
  TRACE_EVENT_END2("audio", "AudioOutputDevice::FireRenderCallback",
                   "timestamp (ms)",
                   (delay_timestamp - base::TimeTicks()).InMillisecondsF(),
                   "delay (ms)", delay.InMillisecondsF());
}

bool AudioOutputDevice::AudioThreadCallback::
    CurrentThreadIsAudioDeviceThread() {
  return thread_checker_.CalledOnValidThread();
}

void AudioOutputDevice::AudioThreadCallback::InitializePlayStartTime() {
  if (!first_play_start_time_.has_value())
    first_play_start_time_ = base::TimeTicks::Now();
}

}  // namespace media
