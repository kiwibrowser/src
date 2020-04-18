// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_STREAM_CONTROLS_H_
#define CONTENT_COMMON_MEDIA_MEDIA_STREAM_CONTROLS_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

// Names for media stream source capture types.
// These are values of the "TrackControls.stream_source" field, and are
// set via the "chromeMediaSource" constraint.
CONTENT_EXPORT extern const char kMediaStreamSourceTab[];
CONTENT_EXPORT extern const char kMediaStreamSourceScreen[];
CONTENT_EXPORT extern const char kMediaStreamSourceDesktop[];
CONTENT_EXPORT extern const char kMediaStreamSourceSystem[];

struct CONTENT_EXPORT TrackControls {
  TrackControls();
  explicit TrackControls(bool request);
  explicit TrackControls(const TrackControls& other);
  ~TrackControls();

  bool requested = false;

  // Source. This is "tab", "screen", "desktop", "system", or blank.
  // Consider replacing with MediaStreamType enum variables.
  std::string stream_source;  // audio.kMediaStreamSource

  // An empty string represents the default device.
  // A nonempty string represents a specific device.
  std::string device_id;
};

// StreamControls describes what is sent to the browser process
// from the renderer process in order to control the opening of a device
// pair. This may result in opening one audio and/or one video device.
// This has to be a struct with public members in order to allow it to
// be sent in the mojo IPC.
struct CONTENT_EXPORT StreamControls {
  StreamControls();
  StreamControls(bool request_audio, bool request_video);
  ~StreamControls();

  TrackControls audio;
  TrackControls video;
  // Hotword functionality (chromeos only)
  // See crbug.com/564574 for discussion on possibly #ifdef'ing this out.
  bool hotword_enabled = false;
  bool disable_local_echo = false;
};

}  // namespace content

#endif  // CONTENT_COMMON_MEDIA_MEDIA_STREAM_CONTROLS_H_
