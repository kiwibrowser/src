/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/mediastream/media_stream_center.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_audio_source_provider.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_center.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_descriptor.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_web_audio_source.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

MediaStreamCenter& MediaStreamCenter::Instance() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(MediaStreamCenter, center, ());
  return center;
}

MediaStreamCenter::MediaStreamCenter()
    : private_(Platform::Current()->CreateMediaStreamCenter(this)) {}

MediaStreamCenter::~MediaStreamCenter() = default;

void MediaStreamCenter::DidSetMediaStreamTrackEnabled(
    MediaStreamComponent* component) {
  if (private_) {
    if (component->Enabled()) {
      private_->DidEnableMediaStreamTrack(component);
    } else {
      private_->DidDisableMediaStreamTrack(component);
    }
  }
}

void MediaStreamCenter::DidCreateMediaStreamAndTracks(
    MediaStreamDescriptor* stream) {
  if (!private_)
    return;

  for (size_t i = 0; i < stream->NumberOfAudioComponents(); ++i)
    DidCreateMediaStreamTrack(stream->AudioComponent(i));

  for (size_t i = 0; i < stream->NumberOfVideoComponents(); ++i)
    DidCreateMediaStreamTrack(stream->VideoComponent(i));
}

void MediaStreamCenter::DidCreateMediaStreamTrack(MediaStreamComponent* track) {
  if (private_)
    private_->DidCreateMediaStreamTrack(track);
}

void MediaStreamCenter::DidCloneMediaStreamTrack(MediaStreamComponent* original,
                                                 MediaStreamComponent* clone) {
  if (private_)
    private_->DidCloneMediaStreamTrack(original, clone);
}

void MediaStreamCenter::DidSetContentHint(MediaStreamComponent* track) {
  if (private_)
    private_->DidSetContentHint(track);
}

std::unique_ptr<AudioSourceProvider>
MediaStreamCenter::CreateWebAudioSourceFromMediaStreamTrack(
    MediaStreamComponent* track) {
  DCHECK(track);
  if (private_) {
    return MediaStreamWebAudioSource::Create(base::WrapUnique(
        private_->CreateWebAudioSourceFromMediaStreamTrack(track)));
  }

  return nullptr;
}

void MediaStreamCenter::DidStopMediaStreamSource(MediaStreamSource* source) {
  if (private_)
    private_->DidStopMediaStreamSource(source);
}

void MediaStreamCenter::StopLocalMediaStream(const WebMediaStream& web_stream) {
  MediaStreamDescriptor* stream = web_stream;
  MediaStreamDescriptorClient* client = stream->Client();
  if (client)
    client->StreamEnded();
}

}  // namespace blink
