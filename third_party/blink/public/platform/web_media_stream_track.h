/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_STREAM_TRACK_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_STREAM_TRACK_H_

#include "base/optional.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class MediaStreamComponent;
class MediaStreamTrack;
class WebAudioSourceProvider;
class WebMediaConstraints;
class WebMediaStream;
class WebMediaStreamSource;
class WebString;

class WebMediaStreamTrack {
 public:
  enum class FacingMode { kNone, kUser, kEnvironment, kLeft, kRight };

  struct Settings {
    bool HasFrameRate() const { return frame_rate >= 0.0; }
    bool HasWidth() const { return width >= 0; }
    bool HasHeight() const { return height >= 0; }
    bool HasAspectRatio() const { return aspect_ratio >= 0.0; }
    bool HasFacingMode() const { return facing_mode != FacingMode::kNone; }
    bool HasVideoKind() const { return !video_kind.IsNull(); }
    bool HasFocalLengthX() const { return focal_length_x >= 0.0; }
    bool HasFocalLengthY() const { return focal_length_y >= 0.0; }
    bool HasDepthNear() const { return depth_near >= 0.0; }
    bool HasDepthFar() const { return depth_far >= 0.0; }
    // The variables are read from
    // MediaStreamTrack::GetSettings only.
    double frame_rate = -1.0;
    long width = -1;
    long height = -1;
    double aspect_ratio = -1.0;
    WebString device_id;
    WebString group_id;
    FacingMode facing_mode = FacingMode::kNone;
    base::Optional<bool> echo_cancellation;
    base::Optional<bool> auto_gain_control;
    base::Optional<bool> noise_supression;
    WebString echo_cancellation_type;
    // Media Capture Depth Stream Extensions.
    WebString video_kind;
    double focal_length_x = -1.0;
    double focal_length_y = -1.0;
    double depth_near = -1.0;
    double depth_far = -1.0;
  };

  class TrackData {
   public:
    TrackData() = default;
    virtual ~TrackData() = default;
    virtual void GetSettings(Settings&) = 0;
  };

  enum class ContentHintType {
    kNone,
    kAudioSpeech,
    kAudioMusic,
    kVideoMotion,
    kVideoDetail
  };

  WebMediaStreamTrack() = default;
  WebMediaStreamTrack(const WebMediaStreamTrack& other) { Assign(other); }
  ~WebMediaStreamTrack() { Reset(); }

  WebMediaStreamTrack& operator=(const WebMediaStreamTrack& other) {
    Assign(other);
    return *this;
  }
  BLINK_PLATFORM_EXPORT void Assign(const WebMediaStreamTrack&);

  BLINK_PLATFORM_EXPORT void Initialize(const WebMediaStreamSource&);
  BLINK_PLATFORM_EXPORT void Initialize(const WebString& id,
                                        const WebMediaStreamSource&);

  BLINK_PLATFORM_EXPORT void Reset();
  bool IsNull() const { return private_.IsNull(); }

  BLINK_PLATFORM_EXPORT WebString Id() const;
  BLINK_PLATFORM_EXPORT int UniqueId() const;

  BLINK_PLATFORM_EXPORT WebMediaStreamSource Source() const;
  BLINK_PLATFORM_EXPORT bool IsEnabled() const;
  BLINK_PLATFORM_EXPORT bool IsMuted() const;
  BLINK_PLATFORM_EXPORT ContentHintType ContentHint() const;
  BLINK_PLATFORM_EXPORT WebMediaConstraints Constraints() const;
  BLINK_PLATFORM_EXPORT void SetConstraints(const WebMediaConstraints&);

  // Extra data associated with this WebMediaStream.
  // If non-null, the extra data pointer will be deleted when the object is
  // destroyed.  Setting the track data pointer will cause any existing non-null
  // track data pointer to be deleted.
  BLINK_PLATFORM_EXPORT TrackData* GetTrackData() const;
  BLINK_PLATFORM_EXPORT void SetTrackData(TrackData*);

  // The lifetime of the WebAudioSourceProvider should outlive the
  // WebMediaStreamTrack, and clients are responsible for calling
  // SetSourceProvider(0) before the WebMediaStreamTrack is going away.
  BLINK_PLATFORM_EXPORT void SetSourceProvider(WebAudioSourceProvider*);

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebMediaStreamTrack(MediaStreamComponent*);
  BLINK_PLATFORM_EXPORT WebMediaStreamTrack& operator=(MediaStreamComponent*);
  BLINK_PLATFORM_EXPORT operator scoped_refptr<MediaStreamComponent>() const;
  BLINK_PLATFORM_EXPORT operator MediaStreamComponent*() const;
#endif

 private:
  WebPrivatePtr<MediaStreamComponent> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_STREAM_TRACK_H_
