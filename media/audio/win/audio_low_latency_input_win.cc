// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/win/audio_low_latency_input_win.h"

#include <audiopolicy.h>
#include <mediaobj.h>
#include <objbase.h>
#include <uuids.h>
#include <wmcodecdsp.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_features.h"
#include "media/audio/win/avrt_wrapper_win.h"
#include "media/audio/win/core_audio_util_win.h"
#include "media/base/audio_block_fifo.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/base/channel_layout.h"
#include "media/base/limits.h"

using base::win::ScopedCOMInitializer;

namespace media {

namespace {

// Errors when initializing the audio client related to the audio format. Split
// by whether we're using format conversion or not. Used for reporting stats -
// do not renumber entries.
enum FormatRelatedInitError {
  kUnsupportedFormat = 0,
  kUnsupportedFormatWithFormatConversion = 1,
  kInvalidArgument = 2,
  kInvalidArgumentWithFormatConversion = 3,
  kCount
};

bool IsSupportedFormatForConversion(const WAVEFORMATEX& format) {
  if (format.nSamplesPerSec < limits::kMinSampleRate ||
      format.nSamplesPerSec > limits::kMaxSampleRate) {
    return false;
  }

  switch (format.wBitsPerSample) {
    case 8:
    case 16:
    case 32:
      break;
    default:
      return false;
  }

  if (GuessChannelLayout(format.nChannels) == CHANNEL_LAYOUT_UNSUPPORTED) {
    LOG(ERROR) << "Hardware configuration not supported for audio conversion";
    return false;
  }

  return true;
}

// Returns the index of the device in the device collection, or -1 for the
// default device, as used by the voice processing DMO.
base::Optional<WORD> GetAudioDeviceCollectionIndexFromId(
    const std::string& device_id,
    const EDataFlow data_flow) {
  // The default device is specified with -1.
  if (AudioDeviceDescription::IsDefaultDevice(device_id))
    return -1;

  WORD device_index = -1;
  HRESULT hr = E_FAIL;
  // The default communications does not have an index itself, so we need to
  // find the index for the underlying device.
  if (AudioDeviceDescription::IsCommunicationsDevice(device_id)) {
    const std::string communications_id =
        (data_flow == eCapture)
            ? CoreAudioUtil::GetCommunicationsInputDeviceID()
            : CoreAudioUtil::GetCommunicationsOutputDeviceID();
    hr = CoreAudioUtil::GetDeviceCollectionIndex(communications_id, data_flow,
                                                 &device_index);
  } else {
    // Otherwise, just look for the device_id directly.
    hr = CoreAudioUtil::GetDeviceCollectionIndex(device_id, data_flow,
                                                 &device_index);
  }

  if (FAILED(hr) || hr == S_FALSE)
    return base::nullopt;

  return device_index;
}

// Implementation of IMediaBuffer, as required for
// IMediaObject::ProcessOutput(). After consuming data provided by
// ProcessOutput(), call SetLength() to update the buffer availability.
// Example implementation:
// http://msdn.microsoft.com/en-us/library/dd376684(v=vs.85).aspx
class MediaBufferImpl : public IMediaBuffer {
 public:
  explicit MediaBufferImpl(DWORD max_length)
      : data_(new BYTE[max_length]), max_length_(max_length) {}

  // IMediaBuffer implementation.
  STDMETHOD(GetBufferAndLength)(BYTE** buffer, DWORD* length) {
    if (!buffer || !length)
      return E_POINTER;

    *buffer = data_.get();
    *length = length_;
    return S_OK;
  }

  STDMETHOD(GetMaxLength)(DWORD* max_length) {
    if (!max_length)
      return E_POINTER;

    *max_length = max_length_;
    return S_OK;
  }

  STDMETHOD(SetLength)(DWORD length) {
    if (length > max_length_)
      return E_INVALIDARG;

    length_ = length;
    return S_OK;
  }

  // IUnknown implementation.
  STDMETHOD_(ULONG, AddRef)() { return InterlockedIncrement(&ref_count_); }

  STDMETHOD(QueryInterface)(REFIID riid, void** object) {
    if (!object)
      return E_POINTER;
    if (riid != IID_IMediaBuffer && riid != IID_IUnknown)
      return E_NOINTERFACE;

    *object = static_cast<IMediaBuffer*>(this);
    AddRef();
    return S_OK;
  }

  STDMETHOD_(ULONG, Release)() {
    LONG ref_count = InterlockedDecrement(&ref_count_);
    if (ref_count == 0)
      delete this;

    return ref_count;
  }

 private:
  virtual ~MediaBufferImpl() {}

  std::unique_ptr<BYTE[]> data_;
  DWORD length_ = 0;
  const DWORD max_length_;
  LONG ref_count_ = 0;
};

}  // namespace

WASAPIAudioInputStream::WASAPIAudioInputStream(
    AudioManagerWin* manager,
    const AudioParameters& params,
    const std::string& device_id,
    const AudioManager::LogCallback& log_callback,
    AudioManagerBase::VoiceProcessingMode voice_processing_mode)
    : manager_(manager),
      device_id_(device_id),
      output_device_id_for_aec_(AudioDeviceDescription::kDefaultDeviceId),
      log_callback_(log_callback),
      use_voice_processing_(voice_processing_mode ==
                            AudioManagerBase::VoiceProcessingMode::kEnabled) {
  DCHECK(manager_);
  DCHECK(!device_id_.empty());
  DCHECK(!log_callback_.is_null());

  DVLOG_IF(1, use_voice_processing_) << "Using Windows voice capture DSP DMO.";

  // Load the Avrt DLL if not already loaded. Required to support MMCSS.
  bool avrt_init = avrt::Initialize();
  DCHECK(avrt_init) << "Failed to load the Avrt.dll";

  const SampleFormat kSampleFormat = kSampleFormatS16;

  // Set up the desired output format specified by the client.
  output_format_.wFormatTag = WAVE_FORMAT_PCM;
  output_format_.nChannels = params.channels();
  output_format_.nSamplesPerSec = params.sample_rate();
  output_format_.wBitsPerSample = SampleFormatToBitsPerChannel(kSampleFormat);
  output_format_.nBlockAlign =
      (output_format_.wBitsPerSample / 8) * output_format_.nChannels;
  output_format_.nAvgBytesPerSec =
      output_format_.nSamplesPerSec * output_format_.nBlockAlign;
  output_format_.cbSize = 0;

  // Set the input (capture) format to the desired output format. In most cases,
  // it will be used unchanged.
  input_format_ = output_format_;

  // Size in bytes of each audio frame.
  frame_size_bytes_ = input_format_.nBlockAlign;

  // Store size of audio packets which we expect to get from the audio
  // endpoint device in each capture event.
  packet_size_bytes_ = params.GetBytesPerBuffer(kSampleFormat);
  packet_size_frames_ = packet_size_bytes_ / input_format_.nBlockAlign;
  DVLOG(1) << "Number of bytes per audio frame  : " << frame_size_bytes_;
  DVLOG(1) << "Number of audio frames per packet: " << packet_size_frames_;

  // All events are auto-reset events and non-signaled initially.

  // Create the event which the audio engine will signal each time
  // a buffer becomes ready to be processed by the client.
  audio_samples_ready_event_.Set(CreateEvent(NULL, FALSE, FALSE, NULL));
  DCHECK(audio_samples_ready_event_.IsValid());

  // Create the event which will be set in Stop() when capturing shall stop.
  stop_capture_event_.Set(CreateEvent(NULL, FALSE, FALSE, NULL));
  DCHECK(stop_capture_event_.IsValid());
}

WASAPIAudioInputStream::~WASAPIAudioInputStream() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool WASAPIAudioInputStream::Open() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(OPEN_RESULT_OK, open_result_);

  // Verify that we are not already opened.
  if (opened_) {
    log_callback_.Run("WASAPIAIS::Open: already open");
    return false;
  }

  // Obtain a reference to the IMMDevice interface of the capturing
  // device with the specified unique identifier or role which was
  // set at construction.
  HRESULT hr = SetCaptureDevice();
  if (FAILED(hr)) {
    ReportOpenResult(hr);
    return false;
  }

  // If voice processing is enabled, initialize the DMO that is used for it. The
  // remainder of the function initializes an audio capture client (the normal
  // case). Either the DMO or the capture client is used.
  // TODO(grunell): Refactor out the audio capture client initialization to its
  // own function.
  if (use_voice_processing_) {
    opened_ = InitializeDmo();
    return opened_;
  }

  // Obtain an IAudioClient interface which enables us to create and initialize
  // an audio stream between an audio application and the audio engine.
  hr = endpoint_device_->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER,
                                  NULL, &audio_client_);
  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_ACTIVATION_FAILED;
    ReportOpenResult(hr);
    return false;
  }

#ifndef NDEBUG
  // Retrieve the stream format which the audio engine uses for its internal
  // processing/mixing of shared-mode streams. This function call is for
  // diagnostic purposes only and only in debug mode.
  hr = GetAudioEngineStreamFormat();
#endif

  // Verify that the selected audio endpoint supports the specified format
  // set during construction.
  hr = S_OK;
  if (!DesiredFormatIsSupported(&hr)) {
    open_result_ = OPEN_RESULT_FORMAT_NOT_SUPPORTED;
    ReportOpenResult(hr);
    return false;
  }

  // Initialize the audio stream between the client and the device using
  // shared mode and a lowest possible glitch-free latency.
  hr = InitializeAudioEngine();
  if (SUCCEEDED(hr) && converter_)
    open_result_ = OPEN_RESULT_OK_WITH_RESAMPLING;
  ReportOpenResult(hr);  // Report before we assign a value to |opened_|.
  opened_ = SUCCEEDED(hr);

  return opened_;
}

void WASAPIAudioInputStream::Start(AudioInputCallback* callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(callback);
  DLOG_IF(ERROR, !opened_) << "Open() has not been called successfully";
  if (!opened_)
    return;

  if (started_)
    return;

  // TODO(grunell): Refactor the |use_voice_processing_| conditions in this
  // function to clean up the code.
  if (use_voice_processing_) {
    // Pre-fill render buffer with silence.
    if (!CoreAudioUtil::FillRenderEndpointBufferWithSilence(
            audio_client_for_render_.Get(), audio_render_client_.Get())) {
      DLOG(WARNING) << "Failed to pre-fill render buffer with silence.";
    }
  } else {
    if (device_id_ == AudioDeviceDescription::kLoopbackWithMuteDeviceId &&
        system_audio_volume_) {
      BOOL muted = false;
      system_audio_volume_->GetMute(&muted);

      // If the system audio is muted at the time of capturing, then no need to
      // mute it again, and later we do not unmute system audio when stopping
      // capturing.
      if (!muted) {
        system_audio_volume_->SetMute(true, NULL);
        mute_done_ = true;
      }
    }
  }

  DCHECK(!sink_);
  sink_ = callback;

  // Starts periodic AGC microphone measurements if the AGC has been enabled
  // using SetAutomaticGainControl().
  StartAgc();

  // Create and start the thread that will drive the capturing by waiting for
  // capture events.
  DCHECK(!capture_thread_.get());
  capture_thread_.reset(new base::DelegateSimpleThread(
      this, "wasapi_capture_thread",
      base::SimpleThread::Options(base::ThreadPriority::REALTIME_AUDIO)));
  capture_thread_->Start();

  HRESULT hr = E_FAIL;
  if (use_voice_processing_) {
    hr = audio_client_for_render_->Start();
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to start output streaming: " << std::hex << hr
                  << ", proceeding without rendering.";
    }
  } else {
    // Start streaming data between the endpoint buffer and the audio engine.
    hr = audio_client_->Start();
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to start input streaming.";
      log_callback_.Run(base::StringPrintf(
          "WASAPIAIS::Start: Failed to start audio client, hresult = %#lx",
          hr));
    }

    if (SUCCEEDED(hr) && audio_render_client_for_loopback_.Get()) {
      hr = audio_render_client_for_loopback_->Start();
      if (FAILED(hr))
        log_callback_.Run(base::StringPrintf(
            "WASAPIAIS::Start: Failed to start render client for loopback, "
            "hresult = %#lx",
            hr));
    }
  }

  started_ = SUCCEEDED(hr);
}

void WASAPIAudioInputStream::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "WASAPIAudioInputStream::Stop()";
  if (!started_)
    return;

  // We have muted system audio for capturing, so we need to unmute it when
  // capturing stops.
  if (device_id_ == AudioDeviceDescription::kLoopbackWithMuteDeviceId &&
      mute_done_) {
    DCHECK(system_audio_volume_);
    if (system_audio_volume_) {
      system_audio_volume_->SetMute(false, NULL);
      mute_done_ = false;
    }
  }

  // Stops periodic AGC microphone measurements.
  StopAgc();

  // Shut down the capture thread.
  if (stop_capture_event_.IsValid()) {
    SetEvent(stop_capture_event_.Get());
  }

  // TODO(grunell): Refactor the |use_voice_processing_| conditions in this
  // function to clean up the code.
  if (use_voice_processing_) {
    // Stop the render audio streaming. The input streaming needs no explicit
    // stopping.
    HRESULT hr = audio_client_for_render_->Stop();
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to stop output streaming.";
    }
  } else {
    // Stop the input audio streaming.
    HRESULT hr = audio_client_->Stop();
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to stop input streaming.";
    }
  }

  // Wait until the thread completes and perform cleanup.
  if (capture_thread_) {
    SetEvent(stop_capture_event_.Get());
    capture_thread_->Join();
    capture_thread_.reset();
  }

  if (use_voice_processing_) {
    HRESULT hr = voice_capture_dmo_->FreeStreamingResources();
    if (FAILED(hr))
      DLOG(ERROR) << "Failed to free dmo resources.";
  }

  started_ = false;
  sink_ = NULL;
}

void WASAPIAudioInputStream::Close() {
  DVLOG(1) << "WASAPIAudioInputStream::Close()";
  // It is valid to call Close() before calling open or Start().
  // It is also valid to call Close() after Start() has been called.
  Stop();

  if (converter_)
    converter_->RemoveInput(this);

  ReportAndResetGlitchStats();

  // Inform the audio manager that we have been closed. This will cause our
  // destruction.
  manager_->ReleaseInputStream(this);
}

double WASAPIAudioInputStream::GetMaxVolume() {
  // Verify that Open() has been called succesfully, to ensure that an audio
  // session exists and that an ISimpleAudioVolume interface has been created.
  DLOG_IF(ERROR, !opened_) << "Open() has not been called successfully";
  if (!opened_)
    return 0.0;

  // The effective volume value is always in the range 0.0 to 1.0, hence
  // we can return a fixed value (=1.0) here.
  return 1.0;
}

void WASAPIAudioInputStream::SetVolume(double volume) {
  DVLOG(1) << "SetVolume(volume=" << volume << ")";
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_GE(volume, 0.0);
  DCHECK_LE(volume, 1.0);

  DLOG_IF(ERROR, !opened_) << "Open() has not been called successfully";
  if (!opened_)
    return;

  // Set a new master volume level. Valid volume levels are in the range
  // 0.0 to 1.0. Ignore volume-change events.
  HRESULT hr =
      simple_audio_volume_->SetMasterVolume(static_cast<float>(volume), NULL);
  if (FAILED(hr))
    DLOG(WARNING) << "Failed to set new input master volume.";

  // Update the AGC volume level based on the last setting above. Note that,
  // the volume-level resolution is not infinite and it is therefore not
  // possible to assume that the volume provided as input parameter can be
  // used directly. Instead, a new query to the audio hardware is required.
  // This method does nothing if AGC is disabled.
  UpdateAgcVolume();
}

double WASAPIAudioInputStream::GetVolume() {
  DCHECK(opened_) << "Open() has not been called successfully";
  if (!opened_)
    return 0.0;

  // Retrieve the current volume level. The value is in the range 0.0 to 1.0.
  float level = 0.0f;
  HRESULT hr = simple_audio_volume_->GetMasterVolume(&level);
  if (FAILED(hr))
    DLOG(WARNING) << "Failed to get input master volume.";

  return static_cast<double>(level);
}

bool WASAPIAudioInputStream::IsMuted() {
  DCHECK(opened_) << "Open() has not been called successfully";
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!opened_)
    return false;

  // Retrieves the current muting state for the audio session.
  BOOL is_muted = FALSE;
  HRESULT hr = simple_audio_volume_->GetMute(&is_muted);
  if (FAILED(hr))
    DLOG(WARNING) << "Failed to get input master volume.";

  return is_muted != FALSE;
}

void WASAPIAudioInputStream::SetOutputDeviceForAec(
    const std::string& output_device_id) {
  if (!use_voice_processing_)
    return;

  if (output_device_id == output_device_id_for_aec_)
    return;

  output_device_id_for_aec_ = output_device_id;

  // Set devices.
  Microsoft::WRL::ComPtr<IPropertyStore> ps;
  HRESULT hr = voice_capture_dmo_->QueryInterface(IID_IPropertyStore, &ps);
  if (FAILED(hr) || !ps) {
    log_callback_.Run(base::StringPrintf(
        "WASAPIAIS:SetOutputDeviceForAec: Getting DMO property store failed."));
    return;
  }

  if (!SetDmoDevices(ps.Get())) {
    log_callback_.Run(
        "WASAPIAIS:SetOutputDeviceForAec: Setting device indices failed.");
    return;
  }

  // Recreate the dummy render client on the new output.
  hr = audio_client_for_render_->Stop();
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to stop output streaming.";
  }

  CreateDummyRenderClientsForDmo();

  if (!CoreAudioUtil::FillRenderEndpointBufferWithSilence(
          audio_client_for_render_.Get(), audio_render_client_.Get())) {
    DLOG(WARNING) << "Failed to pre-fill render buffer with silence.";
  }

  hr = audio_client_for_render_->Start();
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to start output streaming: " << std::hex << hr
                << ", proceeding without rendering.";
  }

  log_callback_.Run(base::StringPrintf(
      "WASAPIAIS:SetOutputDeviceForAec: Successfully updated AEC output "
      "device to %s",
      output_device_id.c_str()));
}

void WASAPIAudioInputStream::Run() {
  ScopedCOMInitializer com_init(ScopedCOMInitializer::kMTA);

  // Enable MMCSS to ensure that this thread receives prioritized access to
  // CPU resources.
  DWORD task_index = 0;
  HANDLE mm_task =
      avrt::AvSetMmThreadCharacteristics(L"Pro Audio", &task_index);
  bool mmcss_is_ok =
      (mm_task && avrt::AvSetMmThreadPriority(mm_task, AVRT_PRIORITY_CRITICAL));
  if (!mmcss_is_ok) {
    // Failed to enable MMCSS on this thread. It is not fatal but can lead
    // to reduced QoS at high load.
    DWORD err = GetLastError();
    LOG(WARNING) << "Failed to enable MMCSS (error code=" << err << ").";
  }

  // Allocate a buffer with a size that enables us to take care of cases like:
  // 1) The recorded buffer size is smaller, or does not match exactly with,
  //    the selected packet size used in each callback.
  // 2) The selected buffer size is larger than the recorded buffer size in
  //    each event.
  // In the case where no resampling is required, a single buffer should be
  // enough but in case we get buffers that don't match exactly, we'll go with
  // two. Same applies if we need to resample and the buffer ratio is perfect.
  // However if the buffer ratio is imperfect, we will need 3 buffers to safely
  // be able to buffer up data in cases where a conversion requires two audio
  // buffers (and we need to be able to write to the third one).
  size_t capture_buffer_size =
      std::max(2 * endpoint_buffer_size_frames_ * frame_size_bytes_,
               2 * packet_size_frames_ * frame_size_bytes_);
  int buffers_required = capture_buffer_size / packet_size_bytes_;
  if (converter_ && imperfect_buffer_size_conversion_)
    ++buffers_required;

  DCHECK(!fifo_);
  fifo_.reset(new AudioBlockFifo(input_format_.nChannels, packet_size_frames_,
                                 buffers_required));

  DVLOG(1) << "AudioBlockFifo buffer count: " << buffers_required;

  bool success =
      use_voice_processing_ ? RunWithDmo() : RunWithAudioCaptureClient();

  if (!success) {
    // TODO(henrika): perhaps it worth improving the cleanup here by e.g.
    // stopping the audio client, joining the thread etc.?
    NOTREACHED() << "WASAPI capturing failed with error code "
                 << GetLastError();
  }

  // Disable MMCSS.
  if (mm_task && !avrt::AvRevertMmThreadCharacteristics(mm_task)) {
    PLOG(WARNING) << "Failed to disable MMCSS";
  }

  fifo_.reset();
}

bool WASAPIAudioInputStream::RunWithAudioCaptureClient() {
  HANDLE wait_array[2] = {stop_capture_event_.Get(),
                          audio_samples_ready_event_.Get()};

  while (true) {
    // Wait for a close-down event or a new capture event.
    DWORD wait_result = WaitForMultipleObjects(2, wait_array, FALSE, INFINITE);
    switch (wait_result) {
      case WAIT_OBJECT_0 + 0:
        // |stop_capture_event_| has been set.
        return true;
      case WAIT_OBJECT_0 + 1:
        // |audio_samples_ready_event_| has been set.
        PullCaptureDataAndPushToSink();
        break;
      case WAIT_FAILED:
      default:
        return false;
    }
  }

  return false;
}

bool WASAPIAudioInputStream::RunWithDmo() {
  while (true) {
    // Poll every 5 ms, or wake up on capture stop signal.
    DWORD wait_result = WaitForSingleObject(stop_capture_event_.Get(), 5);
    switch (wait_result) {
      case WAIT_OBJECT_0:
        // |stop_capture_event_| has been set.
        return true;
      case WAIT_TIMEOUT:
        PullDmoCaptureDataAndPushToSink();
        if (!CoreAudioUtil::FillRenderEndpointBufferWithSilence(
                audio_client_for_render_.Get(), audio_render_client_.Get())) {
          DLOG(WARNING) << "Failed to fill render buffer with silence.";
        }
        break;
      case WAIT_FAILED:
      default:
        return false;
    }
  }

  return false;
}

void WASAPIAudioInputStream::PullCaptureDataAndPushToSink() {
  TRACE_EVENT1("audio", "WASAPIAudioInputStream::PullCaptureDataAndPushToSink",
               "sample rate", input_format_.nSamplesPerSec);

  // Pull data from the capture endpoint buffer until it's empty or an error
  // occurs.
  while (true) {
    BYTE* data_ptr = nullptr;
    UINT32 num_frames_to_read = 0;
    DWORD flags = 0;
    UINT64 device_position = 0;

    // Note: The units on this are 100ns intervals. Both GetBuffer() and
    // GetPosition() will handle the translation from the QPC value, so we just
    // need to convert from 100ns units into us. Which is just dividing by 10.0
    // since 10x100ns = 1us.
    UINT64 capture_time_100ns = 0;

    // Retrieve the amount of data in the capture endpoint buffer, replace it
    // with silence if required, create callbacks for each packet and store
    // non-delivered data for the next event.
    HRESULT hr =
        audio_capture_client_->GetBuffer(&data_ptr, &num_frames_to_read, &flags,
                                         &device_position, &capture_time_100ns);
    if (hr == AUDCLNT_S_BUFFER_EMPTY)
      break;

    ReportDelayStatsAndUpdateGlitchCount(
        num_frames_to_read, flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY,
        device_position,
        base::TimeTicks() +
            base::TimeDelta::FromMicroseconds(capture_time_100ns / 10.0));

    // TODO(grunell): Should we handle different errors explicitly? Perhaps exit
    // by setting |error = true|. What are the assumptions here that makes us
    // rely on the next WaitForMultipleObjects? Do we expect the next wait to be
    // successful sometimes?
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to get data from the capture buffer";
      break;
    }

    // TODO(dalecurtis, olka, grunell): Is this ever false? If it is, should we
    // handle |flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR|?
    if (audio_clock_) {
      // The reported timestamp from GetBuffer is not as reliable as the clock
      // from the client.  We've seen timestamps reported for USB audio devices,
      // be off by several days.  Furthermore we've seen them jump back in time
      // every 2 seconds or so.
      // TODO(grunell): Using the audio clock as capture time for the currently
      // processed buffer seems incorrect. http://crbug.com/825744.
      audio_clock_->GetPosition(&device_position, &capture_time_100ns);
    }

    base::TimeTicks capture_time;
    if (capture_time_100ns) {
      // See conversion notes on |capture_time_100ns|.
      capture_time +=
          base::TimeDelta::FromMicroseconds(capture_time_100ns / 10.0);
    } else {
      // We may not have an IAudioClock or GetPosition() may return zero.
      capture_time = base::TimeTicks::Now();
    }

    // Adjust |capture_time| for the FIFO before pushing.
    capture_time -= AudioTimestampHelper::FramesToTime(
        fifo_->GetAvailableFrames(), input_format_.nSamplesPerSec);

    // TODO(grunell): Since we check |hr == AUDCLNT_S_BUFFER_EMPTY| above,
    // should we instead assert that |num_frames_to_read != 0|?
    if (num_frames_to_read != 0) {
      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        fifo_->PushSilence(num_frames_to_read);
      } else {
        fifo_->Push(data_ptr, num_frames_to_read,
                    input_format_.wBitsPerSample / 8);
      }
    }

    hr = audio_capture_client_->ReleaseBuffer(num_frames_to_read);
    DLOG_IF(ERROR, FAILED(hr)) << "Failed to release capture buffer";

    // Get a cached AGC volume level which is updated once every second on the
    // audio manager thread. Note that, |volume| is also updated each time
    // SetVolume() is called through IPC by the render-side AGC.
    double volume = 0.0;
    GetAgcVolume(&volume);

    // Deliver captured data to the registered consumer using a packet size
    // which was specified at construction.
    while (fifo_->available_blocks()) {
      if (converter_) {
        if (imperfect_buffer_size_conversion_ &&
            fifo_->available_blocks() == 1) {
          // Special case. We need to buffer up more audio before we can convert
          // or else we'll suffer an underrun.
          // TODO(grunell): Verify this is really true.
          break;
        }
        converter_->Convert(convert_bus_.get());
        sink_->OnData(convert_bus_.get(), capture_time, volume);

        // Move the capture time forward for each vended block.
        capture_time += AudioTimestampHelper::FramesToTime(
            convert_bus_->frames(), output_format_.nSamplesPerSec);
      } else {
        sink_->OnData(fifo_->Consume(), capture_time, volume);

        // Move the capture time forward for each vended block.
        capture_time += AudioTimestampHelper::FramesToTime(
            packet_size_frames_, input_format_.nSamplesPerSec);
      }
    }
  }  // while (true)
}

void WASAPIAudioInputStream::PullDmoCaptureDataAndPushToSink() {
  TRACE_EVENT1("audio",
               "WASAPIAudioInputStream::PullDmoCaptureDataAndPushToSink",
               "sample rate", input_format_.nSamplesPerSec);

  // Pull data from the capture endpoint buffer until it's empty or an error
  // occurs.
  while (true) {
    DWORD status = 0;
    DMO_OUTPUT_DATA_BUFFER data_buffer = {0};
    data_buffer.pBuffer = media_buffer_.Get();

    // Get processed capture data from the DMO.
    HRESULT hr =
        voice_capture_dmo_->ProcessOutput(0,  // dwFlags
                                          1,  // cOutputBufferCount
                                          &data_buffer,
                                          &status);  // Must be ignored.
    if (FAILED(hr)) {
      DLOG(ERROR) << "DMO ProcessOutput failed, hr = 0x" << std::hex << hr;
      break;
    }

    BYTE* data;
    ULONG data_length = 0;
    // Get a pointer to the data buffer. This should be valid until the next
    // call to ProcessOutput.
    hr = media_buffer_->GetBufferAndLength(&data, &data_length);
    if (FAILED(hr)) {
      DLOG(ERROR) << "Could not get buffer, hr = 0x" << std::hex << hr;
      break;
    }

    if (data_length > 0) {
      const int samples_produced = data_length / frame_size_bytes_;

      base::TimeTicks capture_time;
      if (data_buffer.dwStatus & DMO_OUTPUT_DATA_BUFFERF_TIME &&
          data_buffer.rtTimestamp > 0) {
        // See conversion notes on |capture_time_100ns| in
        // PullCaptureDataAndPushToSink().
        capture_time +=
            base::TimeDelta::FromMicroseconds(data_buffer.rtTimestamp / 10.0);
      } else {
        // We may not get the timestamp from ProcessOutput(), fall back on
        // current timestamp.
        capture_time = base::TimeTicks::Now();
      }

      // Adjust |capture_time| for the FIFO before pushing.
      capture_time -= AudioTimestampHelper::FramesToTime(
          fifo_->GetAvailableFrames(), input_format_.nSamplesPerSec);

      fifo_->Push(data, samples_produced, input_format_.wBitsPerSample / 8);

      // Reset length to indicate buffer availability.
      hr = media_buffer_->SetLength(0);
      if (FAILED(hr))
        DLOG(ERROR) << "Could not reset length, hr = 0x" << std::hex << hr;

      // Get a cached AGC volume level which is updated once every second on the
      // audio manager thread. Note that, |volume| is also updated each time
      // SetVolume() is called through IPC by the render-side AGC.
      double volume = 0.0;
      GetAgcVolume(&volume);

      while (fifo_->available_blocks()) {
        if (converter_) {
          if (imperfect_buffer_size_conversion_ &&
              fifo_->available_blocks() == 1) {
            // Special case. We need to buffer up more audio before we can
            // convert or else we'll suffer an underrun.
            // TODO(grunell): Verify this is really true.
            break;
          }
          converter_->Convert(convert_bus_.get());
          sink_->OnData(convert_bus_.get(), capture_time, volume);

          // Move the capture time forward for each vended block.
          capture_time += AudioTimestampHelper::FramesToTime(
              convert_bus_->frames(), output_format_.nSamplesPerSec);
        } else {
          sink_->OnData(fifo_->Consume(), capture_time, volume);

          // Move the capture time forward for each vended block.
          capture_time += AudioTimestampHelper::FramesToTime(
              packet_size_frames_, input_format_.nSamplesPerSec);
        }
      }
    }  //  if (data_length > 0)

    if (!(data_buffer.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE)) {
      // The DMO cannot currently produce more data. This is the normal case;
      // otherwise it means the DMO had more than 10 ms of data available and
      // ProcessOutput should be called again.
      break;
    }
  }  // while (true)
}

void WASAPIAudioInputStream::HandleError(HRESULT err) {
  NOTREACHED() << "Error code: " << err;
  if (sink_)
    sink_->OnError();
}

HRESULT WASAPIAudioInputStream::SetCaptureDevice() {
  DCHECK_EQ(OPEN_RESULT_OK, open_result_);
  DCHECK(!endpoint_device_.Get());

  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
  HRESULT hr =
      ::CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator));
  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_CREATE_INSTANCE;
    return hr;
  }

  // Retrieve the IMMDevice by using the specified role or the specified
  // unique endpoint device-identification string.

  if (device_id_ == AudioDeviceDescription::kDefaultDeviceId) {
    // Retrieve the default capture audio endpoint for the specified role.
    // Note that, in Windows Vista, the MMDevice API supports device roles
    // but the system-supplied user interface programs do not.
    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole,
                                             endpoint_device_.GetAddressOf());
  } else if (device_id_ == AudioDeviceDescription::kCommunicationsDeviceId) {
    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications,
                                             endpoint_device_.GetAddressOf());
  } else if (device_id_ == AudioDeviceDescription::kLoopbackWithMuteDeviceId) {
    // Capture the default playback stream.
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole,
                                             endpoint_device_.GetAddressOf());

    if (SUCCEEDED(hr)) {
      endpoint_device_->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                 NULL, &system_audio_volume_);
    }
  } else if (device_id_ == AudioDeviceDescription::kLoopbackInputDeviceId) {
    // Capture the default playback stream.
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole,
                                             endpoint_device_.GetAddressOf());
  } else {
    hr = enumerator->GetDevice(base::UTF8ToUTF16(device_id_).c_str(),
                               endpoint_device_.GetAddressOf());
  }

  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_NO_ENDPOINT;
    return hr;
  }

  // Verify that the audio endpoint device is active, i.e., the audio
  // adapter that connects to the endpoint device is present and enabled.
  DWORD state = DEVICE_STATE_DISABLED;
  hr = endpoint_device_->GetState(&state);
  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_NO_STATE;
    return hr;
  }

  if (!(state & DEVICE_STATE_ACTIVE)) {
    DLOG(ERROR) << "Selected capture device is not active.";
    open_result_ = OPEN_RESULT_DEVICE_NOT_ACTIVE;
    hr = E_ACCESSDENIED;
  }

  return hr;
}

HRESULT WASAPIAudioInputStream::GetAudioEngineStreamFormat() {
  HRESULT hr = S_OK;
#ifndef NDEBUG
  // The GetMixFormat() method retrieves the stream format that the
  // audio engine uses for its internal processing of shared-mode streams.
  // The method always uses a WAVEFORMATEXTENSIBLE structure, instead
  // of a stand-alone WAVEFORMATEX structure, to specify the format.
  // An WAVEFORMATEXTENSIBLE structure can specify both the mapping of
  // channels to speakers and the number of bits of precision in each sample.
  base::win::ScopedCoMem<WAVEFORMATEXTENSIBLE> format_ex;
  hr =
      audio_client_->GetMixFormat(reinterpret_cast<WAVEFORMATEX**>(&format_ex));

  // See http://msdn.microsoft.com/en-us/windows/hardware/gg463006#EFH
  // for details on the WAVE file format.
  WAVEFORMATEX format = format_ex->Format;
  DVLOG(2) << "WAVEFORMATEX:";
  DVLOG(2) << "  wFormatTags    : 0x" << std::hex << format.wFormatTag;
  DVLOG(2) << "  nChannels      : " << format.nChannels;
  DVLOG(2) << "  nSamplesPerSec : " << format.nSamplesPerSec;
  DVLOG(2) << "  nAvgBytesPerSec: " << format.nAvgBytesPerSec;
  DVLOG(2) << "  nBlockAlign    : " << format.nBlockAlign;
  DVLOG(2) << "  wBitsPerSample : " << format.wBitsPerSample;
  DVLOG(2) << "  cbSize         : " << format.cbSize;

  DVLOG(2) << "WAVEFORMATEXTENSIBLE:";
  DVLOG(2) << " wValidBitsPerSample: "
           << format_ex->Samples.wValidBitsPerSample;
  DVLOG(2) << " dwChannelMask      : 0x" << std::hex
           << format_ex->dwChannelMask;
  if (format_ex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
    DVLOG(2) << " SubFormat          : KSDATAFORMAT_SUBTYPE_PCM";
  else if (format_ex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    DVLOG(2) << " SubFormat          : KSDATAFORMAT_SUBTYPE_IEEE_FLOAT";
  else if (format_ex->SubFormat == KSDATAFORMAT_SUBTYPE_WAVEFORMATEX)
    DVLOG(2) << " SubFormat          : KSDATAFORMAT_SUBTYPE_WAVEFORMATEX";
#endif
  return hr;
}

bool WASAPIAudioInputStream::DesiredFormatIsSupported(HRESULT* hr) {
  // An application that uses WASAPI to manage shared-mode streams can rely
  // on the audio engine to perform only limited format conversions. The audio
  // engine can convert between a standard PCM sample size used by the
  // application and the floating-point samples that the engine uses for its
  // internal processing. However, the format for an application stream
  // typically must have the same number of channels and the same sample
  // rate as the stream format used by the device.
  // Many audio devices support both PCM and non-PCM stream formats. However,
  // the audio engine can mix only PCM streams.
  base::win::ScopedCoMem<WAVEFORMATEX> closest_match;
  HRESULT hresult = audio_client_->IsFormatSupported(
      AUDCLNT_SHAREMODE_SHARED, &input_format_, &closest_match);
  DLOG_IF(ERROR, hresult == S_FALSE)
      << "Format is not supported but a closest match exists.";

  if (hresult == S_FALSE) {
    // Change the format we're going to ask for to better match with what the OS
    // can provide.  If we succeed in initializing the audio client in this
    // format and are able to convert from this format, we will do that
    // conversion.
    input_format_.nChannels = closest_match->nChannels;
    input_format_.nSamplesPerSec = closest_match->nSamplesPerSec;

    // If the closest match is fixed point PCM (WAVE_FORMAT_PCM or
    // KSDATAFORMAT_SUBTYPE_PCM), we use the closest match's bits per sample.
    // Otherwise, we keep the bits sample as is since we still request fixed
    // point PCM. In that case the closest match is typically in float format
    // (KSDATAFORMAT_SUBTYPE_IEEE_FLOAT).
    auto format_is_pcm = [](const WAVEFORMATEX* format) {
      if (format->wFormatTag == WAVE_FORMAT_PCM)
        return true;
      if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const WAVEFORMATEXTENSIBLE* format_ex =
            reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
        return format_ex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;
      }
      return false;
    };
    if (format_is_pcm(closest_match))
      input_format_.wBitsPerSample = closest_match->wBitsPerSample;

    input_format_.nBlockAlign =
        (input_format_.wBitsPerSample / 8) * input_format_.nChannels;
    input_format_.nAvgBytesPerSec =
        input_format_.nSamplesPerSec * input_format_.nBlockAlign;

    if (IsSupportedFormatForConversion(input_format_)) {
      DVLOG(1) << "Will convert capture audio from: \nbits: "
               << input_format_.wBitsPerSample
               << "\nsample rate: " << input_format_.nSamplesPerSec
               << "\nchannels: " << input_format_.nChannels
               << "\nblock align: " << input_format_.nBlockAlign
               << "\navg bytes per sec: " << input_format_.nAvgBytesPerSec;

      SetupConverterAndStoreFormatInfo();

      // Indicate that we're good to go with a close match.
      hresult = S_OK;
    }
  }

  // At this point, |hresult| == S_OK if the desired format is supported. If
  // |hresult| == S_FALSE, the OS supports a closest match but we don't support
  // conversion to it. Thus, SUCCEEDED() or FAILED() can't be used to determine
  // if the desired format is supported.
  *hr = hresult;
  return (hresult == S_OK);
}

void WASAPIAudioInputStream::SetupConverterAndStoreFormatInfo() {
  // Ideally, we want a 1:1 ratio between the buffers we get and the buffers
  // we give to OnData so that each buffer we receive from the OS can be
  // directly converted to a buffer that matches with what was asked for.
  const double buffer_ratio =
      output_format_.nSamplesPerSec / static_cast<double>(packet_size_frames_);
  double new_frames_per_buffer = input_format_.nSamplesPerSec / buffer_ratio;

  const auto input_layout = GuessChannelLayout(input_format_.nChannels);
  DCHECK_NE(CHANNEL_LAYOUT_UNSUPPORTED, input_layout);
  const auto output_layout = GuessChannelLayout(output_format_.nChannels);
  DCHECK_NE(CHANNEL_LAYOUT_UNSUPPORTED, output_layout);

  const AudioParameters input(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                              input_layout, input_format_.nSamplesPerSec,
                              static_cast<int>(new_frames_per_buffer));

  const AudioParameters output(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                               output_layout, output_format_.nSamplesPerSec,
                               packet_size_frames_);

  converter_.reset(new AudioConverter(input, output, false));
  converter_->AddInput(this);
  converter_->PrimeWithSilence();
  convert_bus_ = AudioBus::Create(output);

  // Update our packet size assumptions based on the new format.
  const auto new_bytes_per_buffer =
      static_cast<int>(new_frames_per_buffer) * input_format_.nBlockAlign;
  packet_size_frames_ = new_bytes_per_buffer / input_format_.nBlockAlign;
  packet_size_bytes_ = new_bytes_per_buffer;
  frame_size_bytes_ = input_format_.nBlockAlign;

  imperfect_buffer_size_conversion_ =
      std::modf(new_frames_per_buffer, &new_frames_per_buffer) != 0.0;
  DVLOG_IF(1, imperfect_buffer_size_conversion_)
      << "Audio capture data conversion: Need to inject fifo";
}

HRESULT WASAPIAudioInputStream::InitializeAudioEngine() {
  DCHECK_EQ(OPEN_RESULT_OK, open_result_);
  DWORD flags;
  // Use event-driven mode only fo regular input devices. For loopback the
  // EVENTCALLBACK flag is specified when intializing
  // |audio_render_client_for_loopback_|.
  if (AudioDeviceDescription::IsLoopbackDevice(device_id_)) {
    flags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_NOPERSIST;
  } else {
    flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST;
  }

  // Initialize the audio stream between the client and the device.
  // We connect indirectly through the audio engine by using shared mode.
  // The buffer duration is normally set to 0, which ensures that the buffer
  // size is the minimum buffer size needed to ensure that glitches do not occur
  // between the periodic processing passes. It can be set to 100 ms via a
  // feature.
  // Note: if the value is changed, update the description in
  // chrome/browser/flag_descriptions.cc.
  REFERENCE_TIME buffer_duration =
      base::FeatureList::IsEnabled(features::kIncreaseInputAudioBufferSize)
          ? 100 * 1000 * 10  // 100 ms expressed in 100-ns units.
          : 0;
  HRESULT hr = audio_client_->Initialize(
      AUDCLNT_SHAREMODE_SHARED, flags, buffer_duration,
      0,  // device period, n/a for shared mode.
      &input_format_,
      device_id_ == AudioDeviceDescription::kCommunicationsDeviceId
          ? &kCommunicationsSessionId
          : nullptr);

  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_AUDIO_CLIENT_INIT_FAILED;
    base::UmaHistogramSparse("Media.Audio.Capture.Win.InitError", hr);
    MaybeReportFormatRelatedInitError(hr);
    return hr;
  }

  // Retrieve the length of the endpoint buffer shared between the client
  // and the audio engine. The buffer length determines the maximum amount
  // of capture data that the audio engine can read from the endpoint buffer
  // during a single processing pass.
  hr = audio_client_->GetBufferSize(&endpoint_buffer_size_frames_);
  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_GET_BUFFER_SIZE_FAILED;
    return hr;
  }
  const int endpoint_buffer_size_ms =
      static_cast<double>(endpoint_buffer_size_frames_ * 1000) /
          input_format_.nSamplesPerSec +
      0.5;  // Round to closest integer
  UMA_HISTOGRAM_CUSTOM_TIMES(
      "Media.Audio.Capture.Win.EndpointBufferSize",
      base::TimeDelta::FromMilliseconds(endpoint_buffer_size_ms),
      base::TimeDelta::FromMilliseconds(1), base::TimeDelta::FromSeconds(1),
      50);
  DVLOG(1) << "Endpoint buffer size: " << endpoint_buffer_size_frames_
           << " frames (" << endpoint_buffer_size_ms << " ms)";

  // The period between processing passes by the audio engine is fixed for a
  // particular audio endpoint device and represents the smallest processing
  // quantum for the audio engine. This period plus the stream latency between
  // the buffer and endpoint device represents the minimum possible latency
  // that an audio application can achieve.
  REFERENCE_TIME device_period_shared_mode = 0;
  REFERENCE_TIME device_period_exclusive_mode = 0;
  HRESULT hr_dbg = audio_client_->GetDevicePeriod(
      &device_period_shared_mode, &device_period_exclusive_mode);
  if (SUCCEEDED(hr_dbg)) {
    // The 5000 addition is to round end result to closest integer.
    const int device_period_ms = (device_period_shared_mode + 5000) / 10000;
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Media.Audio.Capture.Win.DevicePeriod",
        base::TimeDelta::FromMilliseconds(device_period_ms),
        base::TimeDelta::FromMilliseconds(1), base::TimeDelta::FromSeconds(1),
        50);
    DVLOG(1) << "Device period: " << device_period_ms << " ms";
  }

  REFERENCE_TIME latency = 0;
  hr_dbg = audio_client_->GetStreamLatency(&latency);
  if (SUCCEEDED(hr_dbg)) {
    // The 5000 addition is to round end result to closest integer.
    const int latency_ms = (device_period_shared_mode + 5000) / 10000;
    UMA_HISTOGRAM_CUSTOM_TIMES("Media.Audio.Capture.Win.StreamLatency",
                               base::TimeDelta::FromMilliseconds(latency_ms),
                               base::TimeDelta::FromMilliseconds(1),
                               base::TimeDelta::FromSeconds(1), 50);
    DVLOG(1) << "Stream latency: " << latency_ms << " ms";
  }

  // Set the event handle that the audio engine will signal each time a buffer
  // becomes ready to be processed by the client.
  //
  // In loopback case the capture device doesn't receive any events, so we
  // need to create a separate playback client to get notifications. According
  // to MSDN:
  //
  //   A pull-mode capture client does not receive any events when a stream is
  //   initialized with event-driven buffering and is loopback-enabled. To
  //   work around this, initialize a render stream in event-driven mode. Each
  //   time the client receives an event for the render stream, it must signal
  //   the capture client to run the capture thread that reads the next set of
  //   samples from the capture endpoint buffer.
  //
  // http://msdn.microsoft.com/en-us/library/windows/desktop/dd316551(v=vs.85).aspx
  if (AudioDeviceDescription::IsLoopbackDevice(device_id_)) {
    hr = endpoint_device_->Activate(
        __uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL,
        &audio_render_client_for_loopback_);
    if (FAILED(hr)) {
      open_result_ = OPEN_RESULT_LOOPBACK_ACTIVATE_FAILED;
      return hr;
    }

    hr = audio_render_client_for_loopback_->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, 0, 0,
        &input_format_, NULL);
    if (FAILED(hr)) {
      open_result_ = OPEN_RESULT_LOOPBACK_INIT_FAILED;
      return hr;
    }

    hr = audio_render_client_for_loopback_->SetEventHandle(
        audio_samples_ready_event_.Get());
  } else {
    hr = audio_client_->SetEventHandle(audio_samples_ready_event_.Get());
  }

  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_SET_EVENT_HANDLE;
    return hr;
  }

  // Get access to the IAudioCaptureClient interface. This interface
  // enables us to read input data from the capture endpoint buffer.
  hr = audio_client_->GetService(IID_PPV_ARGS(&audio_capture_client_));
  if (FAILED(hr)) {
    open_result_ = OPEN_RESULT_NO_CAPTURE_CLIENT;
    return hr;
  }

  // Obtain a reference to the ISimpleAudioVolume interface which enables
  // us to control the master volume level of an audio session.
  hr = audio_client_->GetService(IID_PPV_ARGS(&simple_audio_volume_));
  if (FAILED(hr))
    open_result_ = OPEN_RESULT_NO_AUDIO_VOLUME;

  audio_client_->GetService(IID_PPV_ARGS(&audio_clock_));
  if (!audio_clock_)
    LOG(WARNING) << "IAudioClock unavailable, capture times may be inaccurate.";

  return hr;
}

void WASAPIAudioInputStream::ReportOpenResult(HRESULT hr) const {
  DCHECK(!opened_);  // This method must be called before we set this flag.
  UMA_HISTOGRAM_ENUMERATION("Media.Audio.Capture.Win.Open", open_result_,
                            OPEN_RESULT_MAX + 1);
  if (open_result_ != OPEN_RESULT_OK &&
      open_result_ != OPEN_RESULT_OK_WITH_RESAMPLING) {
    log_callback_.Run(base::StringPrintf(
        "WASAPIAIS::Open: failed, result = %d, hresult = %#lx, "
        "input format = %#x/%d/%ld/%d/%d/%ld/%d, "
        "output format = %#x/%d/%ld/%d/%d/%ld/%d",
        // clang-format off
        open_result_, hr,
        input_format_.wFormatTag, input_format_.nChannels,
        input_format_.nSamplesPerSec, input_format_.wBitsPerSample,
        input_format_.nBlockAlign, input_format_.nAvgBytesPerSec,
        input_format_.cbSize,
        output_format_.wFormatTag, output_format_.nChannels,
        output_format_.nSamplesPerSec, output_format_.wBitsPerSample,
        output_format_.nBlockAlign, output_format_.nAvgBytesPerSec,
        output_format_.cbSize));
    // clang-format on
  }
}

void WASAPIAudioInputStream::MaybeReportFormatRelatedInitError(
    HRESULT hr) const {
  if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT && hr != E_INVALIDARG)
    return;

  const FormatRelatedInitError format_related_error =
      hr == AUDCLNT_E_UNSUPPORTED_FORMAT
          ? converter_.get()
                ? FormatRelatedInitError::kUnsupportedFormatWithFormatConversion
                : FormatRelatedInitError::kUnsupportedFormat
          // Otherwise |hr| == E_INVALIDARG.
          : converter_.get()
                ? FormatRelatedInitError::kInvalidArgumentWithFormatConversion
                : FormatRelatedInitError::kInvalidArgument;
  base::UmaHistogramEnumeration(
      "Media.Audio.Capture.Win.InitError.FormatRelated", format_related_error,
      FormatRelatedInitError::kCount);
}

bool WASAPIAudioInputStream::InitializeDmo() {
  HRESULT hr = ::CoCreateInstance(CLSID_CWMAudioAEC, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IMediaObject, &voice_capture_dmo_);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Creating DMO failed.";
    return false;
  }

  if (!SetDmoProperties())
    return false;

  if (!SetDmoFormat())
    return false;

  hr = voice_capture_dmo_->AllocateStreamingResources();
  if (FAILED(hr)) {
    DLOG(ERROR) << "Allocating DMO resources failed.";
    return false;
  }

  SetupConverterAndStoreFormatInfo();

  media_buffer_ =
      new MediaBufferImpl(endpoint_buffer_size_frames_ * frame_size_bytes_);

  if (!CreateDummyRenderClientsForDmo())
    return false;

  // Get volume interface.
  Microsoft::WRL::ComPtr<IAudioSessionManager> audio_session_manager;
  hr = endpoint_device_->Activate(__uuidof(IAudioSessionManager),
                                  CLSCTX_INPROC_SERVER, NULL,
                                  &audio_session_manager);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Obtaining audio session manager failed.";
    return false;
  }
  hr = audio_session_manager->GetSimpleAudioVolume(
      NULL,   // AudioSessionGuid. NULL for default session.
      FALSE,  // CrossProcessSession.
      &simple_audio_volume_);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Obtaining audio volume interface failed.";
    return false;
  }

  return true;
}

bool WASAPIAudioInputStream::SetDmoProperties() {
  Microsoft::WRL::ComPtr<IPropertyStore> ps;
  HRESULT hr = voice_capture_dmo_->QueryInterface(IID_IPropertyStore, &ps);
  if (FAILED(hr) || !ps) {
    DLOG(ERROR) << "Getting DMO property store failed.";
    return false;
  }

  // Set devices.
  if (!SetDmoDevices(ps.Get())) {
    DLOG(ERROR) << "Setting device indices failed.";
    return false;
  }

  // Set DMO mode to AEC only.
  if (FAILED(CoreAudioUtil::SetVtI4Property(
          ps.Get(), MFPKEY_WMAAECMA_SYSTEM_MODE, SINGLE_CHANNEL_AEC))) {
    DLOG(ERROR) << "Setting DMO system mode failed.";
    return false;
  }

  // Enable the feature mode. This lets us override the default processing
  // settings below.
  if (FAILED(CoreAudioUtil::SetBoolProperty(
          ps.Get(), MFPKEY_WMAAECMA_FEATURE_MODE, VARIANT_TRUE))) {
    DLOG(ERROR) << "Setting DMO feature mode failed.";
    return false;
  }

  // Disable analog AGC (default enabled).
  if (FAILED(CoreAudioUtil::SetBoolProperty(
          ps.Get(), MFPKEY_WMAAECMA_MIC_GAIN_BOUNDER, VARIANT_FALSE))) {
    DLOG(ERROR) << "Setting DMO mic gain bounder failed.";
    return false;
  }

  // Disable noise suppression (default enabled).
  if (FAILED(CoreAudioUtil::SetVtI4Property(ps.Get(), MFPKEY_WMAAECMA_FEATR_NS,
                                            0))) {
    DLOG(ERROR) << "Disabling DMO NS failed.";
    return false;
  }

  return true;
}

bool WASAPIAudioInputStream::SetDmoFormat() {
  DMO_MEDIA_TYPE mt;  // Media type.
  mt.majortype = MEDIATYPE_Audio;
  mt.subtype = MEDIASUBTYPE_PCM;
  mt.lSampleSize = 0;
  mt.bFixedSizeSamples = TRUE;
  mt.bTemporalCompression = FALSE;
  mt.formattype = FORMAT_WaveFormatEx;

  HRESULT hr = MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
  if (FAILED(hr)) {
    DLOG(ERROR) << "Init media type for DMO failed.";
    return false;
  }

  WAVEFORMATEX* dmo_output_format =
      reinterpret_cast<WAVEFORMATEX*>(mt.pbFormat);
  dmo_output_format->wFormatTag = WAVE_FORMAT_PCM;
  dmo_output_format->nChannels = 1;
  dmo_output_format->nSamplesPerSec = 16000;
  dmo_output_format->nAvgBytesPerSec = 32000;
  dmo_output_format->nBlockAlign = 2;
  dmo_output_format->wBitsPerSample = 16;
  dmo_output_format->cbSize = 0;

  DCHECK(IsSupportedFormatForConversion(*dmo_output_format));

  // Store the format used.
  input_format_.wFormatTag = dmo_output_format->wFormatTag;
  input_format_.nChannels = dmo_output_format->nChannels;
  input_format_.nSamplesPerSec = dmo_output_format->nSamplesPerSec;
  input_format_.wBitsPerSample = dmo_output_format->wBitsPerSample;
  input_format_.nBlockAlign = dmo_output_format->nBlockAlign;
  input_format_.nAvgBytesPerSec = dmo_output_format->nAvgBytesPerSec;
  input_format_.cbSize = dmo_output_format->cbSize;

  hr = voice_capture_dmo_->SetOutputType(0, &mt, 0);
  MoFreeMediaType(&mt);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Setting DMO output type failed.";
    return false;
  }

  // We use 10 ms buffer size for the DMO.
  endpoint_buffer_size_frames_ = input_format_.nSamplesPerSec / 100;

  return true;
}

bool WASAPIAudioInputStream::SetDmoDevices(IPropertyStore* ps) {
  // Look up the input device's index.
  const base::Optional<WORD> input_device_index =
      GetAudioDeviceCollectionIndexFromId(device_id_, eCapture);

  if (!input_device_index) {
    log_callback_.Run(
        base::StringPrintf("WASAPIAIS:SetDmoDevices: Could not "
                           "resolve input device index for %s",
                           device_id_.c_str()));
    return false;
  }

  // Look up the output device's index.
  const base::Optional<WORD> output_device_index =
      GetAudioDeviceCollectionIndexFromId(output_device_id_for_aec_, eRender);
  if (!output_device_index) {
    log_callback_.Run(
        base::StringPrintf("WASAPIAIS:SetDmoDevices: Could not "
                           "resolve output device index for %s",
                           output_device_id_for_aec_.c_str()));
    return false;
  }

  // The DEVICE_INDEXES property packs the input and output indices into the
  // upper and lower halves of a LONG.
  LONG device_index_value =
      (static_cast<ULONG>(*output_device_index) << 16) +
      (static_cast<ULONG>(*input_device_index) & 0x0000ffff);
  return !FAILED(CoreAudioUtil::SetVtI4Property(
      ps, MFPKEY_WMAAECMA_DEVICE_INDEXES, device_index_value));
}

bool WASAPIAudioInputStream::CreateDummyRenderClientsForDmo() {
  Microsoft::WRL::ComPtr<IAudioClient> audio_client(CoreAudioUtil::CreateClient(
      output_device_id_for_aec_, eRender, eConsole));
  if (!audio_client.Get()) {
    DLOG(ERROR) << "Failed to create audio client for dummy rendering for DMO.";
    return false;
  }

  WAVEFORMATPCMEX mix_format;
  HRESULT hr =
      CoreAudioUtil::GetSharedModeMixFormat(audio_client.Get(), &mix_format);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to get mix format.";
    return false;
  }

  hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                0,  // Stream flags
                                0,  // Buffer duration
                                0,  // Device period
                                reinterpret_cast<WAVEFORMATEX*>(&mix_format),
                                NULL);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Failed to initalize audio client for rendering.";
    return false;
  }

  Microsoft::WRL::ComPtr<IAudioRenderClient> audio_render_client =
      CoreAudioUtil::CreateRenderClient(audio_client.Get());
  if (!audio_render_client.Get()) {
    DLOG(ERROR) << "Failed to create audio render client.";
    return false;
  }

  audio_client_for_render_ = audio_client;
  audio_render_client_ = audio_render_client;

  return true;
}

double WASAPIAudioInputStream::ProvideInput(AudioBus* audio_bus,
                                            uint32_t frames_delayed) {
  fifo_->Consume()->CopyTo(audio_bus);
  return 1.0;
}

void WASAPIAudioInputStream::ReportDelayStatsAndUpdateGlitchCount(
    UINT32 frames_in_buffer,
    bool discontinuity_flagged,
    UINT64 device_position,
    base::TimeTicks capture_time) {
  // Report delay. Don't report if no valid capture time.
  // Unreasonably large delays are clamped at 1 second. Some devices sometimes
  // have capture timestamps way off.
  if (capture_time > base::TimeTicks()) {
    base::TimeDelta delay = base::TimeTicks::Now() - capture_time;
    UMA_HISTOGRAM_CUSTOM_TIMES("Media.Audio.Capture.DeviceLatency", delay,
                               base::TimeDelta::FromMilliseconds(1),
                               base::TimeDelta::FromSeconds(1), 50);
  }

  // Detect glitch. Detect and count separately based on expected device
  // position and the discontinuity flag since they have showed to not always
  // be consistent with each other.
  if (expected_next_device_position_ != 0) {
    if (device_position > expected_next_device_position_) {
      ++total_glitches_;
      auto lost_frames = device_position - expected_next_device_position_;
      total_lost_frames_ += lost_frames;
      if (lost_frames > largest_glitch_frames_)
        largest_glitch_frames_ = lost_frames;
    } else if (device_position < expected_next_device_position_) {
      ++total_device_position_less_than_expected_;
    }
    if (discontinuity_flagged)
      ++total_discontinuities_;
    if (device_position > expected_next_device_position_ &&
        discontinuity_flagged) {
      ++total_concurrent_glitch_and_discontinuities_;
    }
  }

  expected_next_device_position_ = device_position + frames_in_buffer;
}

void WASAPIAudioInputStream::ReportAndResetGlitchStats() {
  UMA_HISTOGRAM_COUNTS("Media.Audio.Capture.Glitches", total_glitches_);
  UMA_HISTOGRAM_COUNTS("Media.Audio.Capture.Win.DevicePositionLessThanExpected",
                       total_device_position_less_than_expected_);
  UMA_HISTOGRAM_COUNTS("Media.Audio.Capture.Win.Discontinuities",
                       total_discontinuities_);
  UMA_HISTOGRAM_COUNTS(
      "Media.Audio.Capture.Win.ConcurrentGlitchAndDiscontinuities",
      total_concurrent_glitch_and_discontinuities_);

  double lost_frames_ms =
      (total_lost_frames_ * 1000) / input_format_.nSamplesPerSec;
  std::string log_message = base::StringPrintf(
      "WASAPIAIS: Total glitches=%d. Total frames lost=%llu (%.0lf ms). Total "
      "discontinuities=%d. Total concurrent glitch and discont=%d. Total low "
      "device "
      "positions=%d.",
      total_glitches_, total_lost_frames_, lost_frames_ms,
      total_discontinuities_, total_concurrent_glitch_and_discontinuities_,
      total_device_position_less_than_expected_);
  log_callback_.Run(log_message);

  if (total_glitches_ != 0) {
    UMA_HISTOGRAM_LONG_TIMES("Media.Audio.Capture.LostFramesInMs",
                             base::TimeDelta::FromMilliseconds(lost_frames_ms));
    int64_t largest_glitch_ms =
        (largest_glitch_frames_ * 1000) / input_format_.nSamplesPerSec;
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Media.Audio.Capture.LargestGlitchMs",
        base::TimeDelta::FromMilliseconds(largest_glitch_ms),
        base::TimeDelta::FromMilliseconds(1), base::TimeDelta::FromMinutes(1),
        50);
    DLOG(WARNING) << log_message;
  }

  expected_next_device_position_ = 0;
  total_glitches_ = 0;
  total_device_position_less_than_expected_ = 0;
  total_discontinuities_ = 0;
  total_concurrent_glitch_and_discontinuities_ = 0;
  total_lost_frames_ = 0;
  largest_glitch_frames_ = 0;
}

}  // namespace media
