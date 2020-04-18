// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_device_thread.h"

#include <limits>

#include "base/logging.h"
#include "base/sys_info.h"

namespace media {

// AudioDeviceThread::Callback implementation

AudioDeviceThread::Callback::Callback(const AudioParameters& audio_parameters,
                                      base::SharedMemoryHandle memory,
                                      bool read_only_memory,
                                      uint32_t segment_length,
                                      uint32_t total_segments)
    : audio_parameters_(audio_parameters),
      memory_length_(
          base::CheckMul(segment_length, total_segments).ValueOrDie()),
      total_segments_(total_segments),
      segment_length_(segment_length),
      // CHECK that the shared memory is large enough. The memory allocated
      // must be at least as large as expected.
      shared_memory_((CHECK(memory_length_ <= memory.GetSize()), memory),
                     read_only_memory) {
  CHECK_GT(total_segments_, 0u);
  thread_checker_.DetachFromThread();
}

AudioDeviceThread::Callback::~Callback() = default;

void AudioDeviceThread::Callback::InitializeOnAudioThread() {
  // Normally this function is called before the thread checker is used
  // elsewhere, but it's not guaranteed. DCHECK to ensure it was not used on
  // another thread before we get here.
  DCHECK(thread_checker_.CalledOnValidThread())
      << "Thread checker was attached on the wrong thread";
  DCHECK(!shared_memory_.memory());
  MapSharedMemory();
  CHECK(shared_memory_.memory());
}

// AudioDeviceThread implementation

AudioDeviceThread::AudioDeviceThread(Callback* callback,
                                     base::SyncSocket::Handle socket,
                                     const char* thread_name)
    : callback_(callback), thread_name_(thread_name), socket_(socket) {
  CHECK(base::PlatformThread::CreateWithPriority(
      0, this, &thread_handle_, base::ThreadPriority::REALTIME_AUDIO));

  DCHECK(!thread_handle_.is_null());
}

AudioDeviceThread::~AudioDeviceThread() {
  socket_.Shutdown();
  if (thread_handle_.is_null())
    return;
  base::PlatformThread::Join(thread_handle_);
}

void AudioDeviceThread::ThreadMain() {
  base::PlatformThread::SetName(thread_name_);
  callback_->InitializeOnAudioThread();

  uint32_t buffer_index = 0;
  while (true) {
    uint32_t pending_data = 0;
    size_t bytes_read = socket_.Receive(&pending_data, sizeof(pending_data));
    if (bytes_read != sizeof(pending_data))
      break;

    // std::numeric_limits<uint32_t>::max() is a special signal which is
    // returned after the browser stops the output device in response to a
    // renderer side request.
    //
    // Avoid running Process() for the paused signal, we still need to update
    // the buffer index for synchronized buffers though.
    //
    // See comments in AudioOutputController::DoPause() for details on why.
    if (pending_data != std::numeric_limits<uint32_t>::max())
      callback_->Process(pending_data);

    // The usage of synchronized buffers differs between input and output cases.
    //
    // Input: Let the other end know that we have read data, so that it can
    // verify it doesn't overwrite any data before read. The |buffer_index|
    // value is not used. For more details, see AudioInputSyncWriter::Write().
    //
    // Output: Let the other end know which buffer we just filled. The
    // |buffer_index| is used to ensure the other end is getting the buffer it
    // expects. For more details on how this works see
    // AudioSyncReader::WaitUntilDataIsReady().
    ++buffer_index;
    size_t bytes_sent = socket_.Send(&buffer_index, sizeof(buffer_index));
    if (bytes_sent != sizeof(buffer_index))
      break;
  }
}

}  // namespace media.
