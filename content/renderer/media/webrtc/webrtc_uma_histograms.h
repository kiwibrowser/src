// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_UMA_HISTOGRAMS_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_UMA_HISTOGRAMS_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "content/public/common/media_stream_request.h"
#include "third_party/blink/public/platform/web_rtc_api_name.h"

namespace content {

// Used to investigate where UserMediaRequests end up.
// Only UserMediaRequests that do not log with LogUserMediaRequestResult
// should call LogUserMediaRequestWithNoResult.
//
// Elements in this enum should not be deleted or rearranged; the only
// permitted operation is to add new elements before
// NUM_MEDIA_STREAM_REQUEST_WITH_NO_RESULT.
enum MediaStreamRequestState {
  MEDIA_STREAM_REQUEST_EXPLICITLY_CANCELLED = 0,
  MEDIA_STREAM_REQUEST_NOT_GENERATED = 1,
  MEDIA_STREAM_REQUEST_PENDING_MEDIA_TRACKS = 2,
  NUM_MEDIA_STREAM_REQUEST_WITH_NO_RESULT
};

void LogUserMediaRequestWithNoResult(MediaStreamRequestState state);
void LogUserMediaRequestResult(MediaStreamRequestResult result);

// Helper method used to collect information about the number of times
// different WebRTC APIs are called from JavaScript.
//
// This contributes to two histograms; the former is a raw count of
// the number of times the APIs are called, and be viewed at
// chrome://histograms/WebRTC.webkitApiCount.
//
// The latter is a count of the number of times the APIs are called
// that gets incremented only once per "session" as established by the
// PerSessionWebRTCAPIMetrics singleton below. It can be viewed at
// chrome://histograms/WebRTC.webkitApiCountPerSession.
void UpdateWebRTCMethodCount(blink::WebRTCAPIName api_name);

// A singleton that keeps track of the number of MediaStreams being
// sent over PeerConnections. It uses the transition to zero such
// streams to demarcate the start of a new "session". Note that this
// is a rough approximation of sessions, as you could conceivably have
// multiple tabs using this renderer process, and each of them using
// PeerConnections.
//
// The UpdateWebRTCMethodCount function above uses this class to log a
// metric at most once per session.
class CONTENT_EXPORT PerSessionWebRTCAPIMetrics {
 public:
  virtual ~PerSessionWebRTCAPIMetrics();

  static PerSessionWebRTCAPIMetrics* GetInstance();

  // Increment/decrement the number of streams being sent or received
  // over any current PeerConnection.
  void IncrementStreamCounter();
  void DecrementStreamCounter();

 protected:
  friend struct base::DefaultSingletonTraits<PerSessionWebRTCAPIMetrics>;
  friend void UpdateWebRTCMethodCount(blink::WebRTCAPIName);

  // Protected so that unit tests can test without this being a
  // singleton.
  PerSessionWebRTCAPIMetrics();

  // Overridable by unit tests.
  virtual void LogUsage(blink::WebRTCAPIName api_name);

  // Called by UpdateWebRTCMethodCount above. Protected rather than
  // private so that unit tests can call it.
  void LogUsageOnlyOnce(blink::WebRTCAPIName api_name);

 private:
  void ResetUsage();

  int num_streams_;
  bool has_used_api_[static_cast<int>(blink::WebRTCAPIName::kInvalidName)];

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PerSessionWebRTCAPIMetrics);
};

} //  namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_UMA_HISTOGRAMS_H_
