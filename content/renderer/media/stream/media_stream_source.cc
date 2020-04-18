// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_source.h"

#include "base/callback_helpers.h"
#include "base/logging.h"

namespace content {

const char MediaStreamSource::kSourceId[] = "sourceId";

MediaStreamSource::MediaStreamSource() {
}

MediaStreamSource::~MediaStreamSource() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(stop_callback_.is_null());
}

void MediaStreamSource::StopSource() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DoStopSource();
  FinalizeStopSource();
}

void MediaStreamSource::FinalizeStopSource() {
  if (!stop_callback_.is_null())
    base::ResetAndReturn(&stop_callback_).Run(Owner());
  Owner().SetReadyState(blink::WebMediaStreamSource::kReadyStateEnded);
}

void MediaStreamSource::SetSourceMuted(bool is_muted) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Although this change is valid only if the ready state isn't already Ended,
  // there's code further along (like in blink::MediaStreamTrack) which filters
  // that out alredy.
  Owner().SetReadyState(is_muted
                            ? blink::WebMediaStreamSource::kReadyStateMuted
                            : blink::WebMediaStreamSource::kReadyStateLive);
}

void MediaStreamSource::SetDevice(const MediaStreamDevice& device) {
  DCHECK(thread_checker_.CalledOnValidThread());
  device_ = device;
}

void MediaStreamSource::SetStopCallback(
    const SourceStoppedCallback& stop_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(stop_callback_.is_null());
  stop_callback_ = stop_callback;
}

void MediaStreamSource::ResetSourceStoppedCallback() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!stop_callback_.is_null());
  stop_callback_.Reset();
}

}  // namespace content
