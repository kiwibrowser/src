// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_LOGGING_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_LOGGING_H_

#include <string>

namespace content {

// This function will add |message| to the diagnostic WebRTC log, if started.
// Otherwise it will be ignored. Note that this log may be uploaded to a
// server by the embedder - no sensitive information should be logged. May be
// called on any thread.
// TODO(grunell): Create a macro for adding log messages. Messages should
// probably also go to the ordinary logging stream.
void WebRtcLogMessage(const std::string& message);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_LOGGING_H_
