// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_AUDIO_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_AUDIO_SOURCE_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/renderer/media/stream/media_stream_audio_deliverer.h"
#include "content/renderer/media/stream/media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

class MediaStreamAudioTrack;

// Represents a source of audio, and manages the delivery of audio data between
// the source implementation and one or more MediaStreamAudioTracks. This is a
// base class providing all the necessary functionality to connect tracks and
// have audio data delivered to them. Subclasses provide the actual audio source
// implementation (e.g., media::AudioCapturerSource), and should implement the
// EnsureSourceIsStarted() and EnsureSourceIsStopped() methods, and call
// SetFormat() and DeliverDataToTracks().
//
// This base class can be instantiated, to be used as a place-holder or a "null"
// source of audio. This can be useful for unit testing, wherever a mock is
// needed, and/or calls to DeliverDataToTracks() must be made at very specific
// times.
//
// An instance of this class is owned by blink::WebMediaStreamSource.
//
// Usage example:
//
//   class MyAudioSource : public MediaStreamSource { ... };
//
//   blink::WebMediaStreamSource blink_source = ...;
//   blink::WebMediaStreamTrack blink_track = ...;
//   blink_source.setExtraData(new MyAudioSource());  // Takes ownership.
//   if (MediaStreamAudioSource::From(blink_source)
//           ->ConnectToTrack(blink_track)) {
//     LOG(INFO) << "Success!";
//   } else {
//     LOG(ERROR) << "Failed!";
//   }
//   // Regardless of whether ConnectToTrack() succeeds, there will always be a
//   // MediaStreamAudioTrack instance created.
//   CHECK(MediaStreamAudioTrack::From(blink_track));
class CONTENT_EXPORT MediaStreamAudioSource : public MediaStreamSource {
 public:
  explicit MediaStreamAudioSource(bool is_local_source);
  MediaStreamAudioSource(bool is_local_source,
                         bool hotword_enabled,
                         bool disable_local_echo);
  ~MediaStreamAudioSource() override;

  // Returns the MediaStreamAudioSource instance owned by the given blink
  // |source| or null.
  static MediaStreamAudioSource* From(
      const blink::WebMediaStreamSource& source);

  // Provides a weak reference to this MediaStreamAudioSource. The weak pointer
  // may only be dereferenced on the main thread.
  base::WeakPtr<MediaStreamAudioSource> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Returns true if the source of audio is local to the application (e.g.,
  // microphone input or loopback audio capture) as opposed to audio being
  // streamed-in from outside the application.
  bool is_local_source() const { return is_local_source_; }

  // Connects this source to the given |track|, creating the appropriate
  // implementation of the content::MediaStreamAudioTrack interface, which
  // becomes associated with and owned by |track|. Returns true if the source
  // was successfully started.
  bool ConnectToTrack(const blink::WebMediaStreamTrack& track);

  // Returns the current format of the audio passing through this source to the
  // sinks. This can return invalid parameters if the source has not yet been
  // started. This method is thread-safe.
  media::AudioParameters GetAudioParameters() const;

  // These accessors return properties that are controlled via constraints.
  bool hotword_enabled() const { return hotword_enabled_; }
  bool disable_local_echo() const { return disable_local_echo_; }
  bool RenderToAssociatedSinkEnabled() const;

  // Returns a unique class identifier. Some subclasses override and use this
  // method to provide safe down-casting to their type.
  virtual void* GetClassIdentifier() const;

 protected:
  // Returns a new MediaStreamAudioTrack. |id| is the blink track's ID in UTF-8.
  // Subclasses may override this to provide an extended implementation.
  virtual std::unique_ptr<MediaStreamAudioTrack> CreateMediaStreamAudioTrack(
      const std::string& id);

  // Returns true if the source has already been started and has not yet been
  // stopped. Otherwise, attempts to start the source and returns true if
  // successful. While the source is running, it may provide audio on any thread
  // by calling DeliverDataToTracks().
  //
  // A default no-op implementation is provided in this base class. Subclasses
  // should override this method.
  virtual bool EnsureSourceIsStarted();

  // Stops the source and guarantees the the flow of audio data has stopped
  // (i.e., by the time this method returns, there will be no further calls to
  // DeliverDataToTracks() on any thread).
  //
  // A default no-op implementation is provided in this base class. Subclasses
  // should override this method.
  virtual void EnsureSourceIsStopped();

  // Called by subclasses to update the format of the audio passing through this
  // source to the sinks. This may be called at any time, before or after
  // tracks have been connected; but must be called at least once before
  // DeliverDataToTracks(). This method is thread-safe.
  void SetFormat(const media::AudioParameters& params);

  // Called by subclasses to deliver audio data to the currently-connected
  // tracks. This method is thread-safe.
  void DeliverDataToTracks(const media::AudioBus& audio_bus,
                           base::TimeTicks reference_time);

  // Called by subclasses when capture error occurs.
  // Note: This can be called on any thread, and will post a task to the main
  // thread to stop the source soon.
  void StopSourceOnError(const std::string& why);

  // Sets muted state and notifies it to all registered tracks.
  void SetMutedState(bool state);

  // Gets the TaskRunner for the main thread, for subclasses that need it.
  base::SingleThreadTaskRunner* GetTaskRunner() const;

 private:
  // MediaStreamSource override.
  void DoStopSource() final;

  // Removes |track| from the list of instances that get a copy of the source
  // audio data. The "stop callback" that was provided to the track calls
  // this.
  void StopAudioDeliveryTo(MediaStreamAudioTrack* track);

  // True if the source of audio is a local device. False if the source is
  // remote (e.g., streamed-in from a server).
  const bool is_local_source_;

  // Properties controlled by audio constraints.
  const bool hotword_enabled_;
  const bool disable_local_echo_;

  // Set to true once this source has been permanently stopped.
  bool is_stopped_;

  // Manages tracks connected to this source and the audio format and data flow.
  MediaStreamAudioDeliverer<MediaStreamAudioTrack> deliverer_;

  // The task runner for main thread. Also used to check that all methods that
  // could cause object graph or data flow changes are being called on the main
  // thread.
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Provides weak pointers so that MediaStreamAudioTracks won't call
  // StopAudioDeliveryTo() if this instance dies first.
  base::WeakPtrFactory<MediaStreamAudioSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamAudioSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_AUDIO_SOURCE_H_
