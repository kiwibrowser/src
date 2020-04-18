// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_AUDIO_RENDERER_H_
#define MEDIA_BASE_AUDIO_RENDERER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "media/base/buffering_state.h"
#include "media/base/media_export.h"
#include "media/base/pipeline_status.h"

namespace media {

class CdmContext;
class DemuxerStream;
class RendererClient;
class TimeSource;

class MEDIA_EXPORT AudioRenderer {
 public:
  AudioRenderer();

  // Stop all operations and fire all pending callbacks.
  virtual ~AudioRenderer();

  // Initialize an AudioRenderer with |stream|, executing |init_cb| upon
  // completion. If initialization fails, only |init_cb|
  // (not RendererClient::OnError) will be called.
  //
  // |cdm_context| can be used to handle encrypted streams. May be null if the
  // stream is not encrypted.
  //
  // AudioRenderer may be reinitialized for playback of a different demuxer
  // stream by calling Initialize again when the renderer is in a flushed
  // state (i.e. after Flush call, but before StartPlaying). This is used for
  // media track switching.
  virtual void Initialize(DemuxerStream* stream,
                          CdmContext* cdm_context,
                          RendererClient* client,
                          const PipelineStatusCB& init_cb) = 0;

  // Returns the TimeSource associated with audio rendering.
  virtual TimeSource* GetTimeSource() = 0;

  // Discard any audio data, executing |callback| when completed.
  //
  // Clients should expect |buffering_state_cb| to be called with
  // BUFFERING_HAVE_NOTHING while flushing is in progress.
  virtual void Flush(const base::Closure& callback) = 0;

  // Starts playback by reading from |stream| and decoding and rendering audio.
  //
  // Only valid to call after a successful Initialize() or Flush().
  virtual void StartPlaying() = 0;

  // Sets the output volume.
  virtual void SetVolume(float volume) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioRenderer);
};

}  // namespace media

#endif  // MEDIA_BASE_AUDIO_RENDERER_H_
