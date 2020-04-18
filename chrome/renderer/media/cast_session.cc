// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/cast_session.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/single_thread_task_runner.h"
#include "chrome/renderer/media/cast_session_delegate.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/video_encode_accelerator.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/video_frame.h"
#include "media/cast/cast_sender.h"
#include "media/cast/logging/logging_defines.h"

namespace {

void CreateVideoEncodeAccelerator(
    const media::cast::ReceiveVideoEncodeAcceleratorCallback& callback) {
  DCHECK(content::RenderThread::Get());

  // Delegate the call to content API on the render thread.
  content::CreateVideoEncodeAccelerator(callback);
}

void CreateVideoEncodeMemory(
    size_t size,
    const media::cast::ReceiveVideoEncodeMemoryCallback& callback) {
  DCHECK(content::RenderThread::Get());

  std::unique_ptr<base::SharedMemory> shm =
      content::RenderThread::Get()->HostAllocateSharedMemoryBuffer(size);
  DCHECK(shm) << "Failed to allocate shared memory";
  if (!shm->Map(size)) {
    NOTREACHED() << "Map failed";
  }
  callback.Run(std::move(shm));
}

}  // namespace

CastSession::CastSession()
    : delegate_(new CastSessionDelegate()),
      io_task_runner_(content::RenderThread::Get()->GetIOTaskRunner()) {}

CastSession::~CastSession() {
  // We should always be able to delete the object on the IO thread.
  CHECK(io_task_runner_->DeleteSoon(FROM_HERE, delegate_.release()));
}

void CastSession::StartAudio(const media::cast::FrameSenderConfig& config,
                             const AudioFrameInputAvailableCallback& callback,
                             const ErrorCallback& error_callback) {
  DCHECK(content::RenderThread::Get());

  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastSessionDelegate::StartAudio,
                                base::Unretained(delegate_.get()), config,
                                media::BindToCurrentLoop(callback),
                                media::BindToCurrentLoop(error_callback)));
}

void CastSession::StartVideo(const media::cast::FrameSenderConfig& config,
                             const VideoFrameInputAvailableCallback& callback,
                             const ErrorCallback& error_callback) {
  DCHECK(content::RenderThread::Get());

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &CastSessionDelegate::StartVideo, base::Unretained(delegate_.get()),
          config, media::BindToCurrentLoop(callback),
          media::BindToCurrentLoop(error_callback),
          media::BindToCurrentLoop(base::Bind(&CreateVideoEncodeAccelerator)),
          media::BindToCurrentLoop(base::Bind(&CreateVideoEncodeMemory))));
}

void CastSession::StartRemotingStream(
    int32_t stream_id,
    const media::cast::FrameSenderConfig& config,
    const ErrorCallback& error_callback) {
  DCHECK(content::RenderThread::Get());

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastSessionDelegate::StartRemotingStream,
                     base::Unretained(delegate_.get()), stream_id, config,
                     media::BindToCurrentLoop(error_callback)));
}

void CastSession::StartUDP(const net::IPEndPoint& remote_endpoint,
                           std::unique_ptr<base::DictionaryValue> options,
                           const ErrorCallback& error_callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastSessionDelegate::StartUDP,
                     base::Unretained(delegate_.get()), net::IPEndPoint(),
                     remote_endpoint, std::move(options),
                     media::BindToCurrentLoop(error_callback)));
}

void CastSession::ToggleLogging(bool is_audio, bool enable) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastSessionDelegate::ToggleLogging,
                     base::Unretained(delegate_.get()), is_audio, enable));
}

void CastSession::GetEventLogsAndReset(
    bool is_audio, const std::string& extra_data,
    const EventLogsCallback& callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastSessionDelegate::GetEventLogsAndReset,
                     base::Unretained(delegate_.get()), is_audio, extra_data,
                     media::BindToCurrentLoop(callback)));
}

void CastSession::GetStatsAndReset(bool is_audio,
                                   const StatsCallback& callback) {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastSessionDelegate::GetStatsAndReset,
                                base::Unretained(delegate_.get()), is_audio,
                                media::BindToCurrentLoop(callback)));
}
