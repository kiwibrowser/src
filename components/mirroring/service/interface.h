// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_INTERFACE_H_
#define COMPONENTS_MIRRORING_SERVICE_INTERFACE_H_

#include <string>

#include "media/capture/mojom/video_capture.mojom.h"
#include "net/base/ip_address.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace mirroring {

// TODO(xjz): All interfaces defined in this file will be replaced with mojo
// interfaces.

// Errors occurred in a mirroring session.
enum SessionError {
  ANSWER_TIME_OUT,                // ANSWER timeout.
  ANSWER_NOT_OK,                  // Not OK answer response.
  ANSWER_MISMATCHED_CAST_MODE,    // ANSWER cast mode mismatched.
  ANSWER_MISMATCHED_SSRC_LENGTH,  // ANSWER ssrc length mismatched with indexes.
  ANSWER_SELECT_MULTIPLE_AUDIO,   // Multiple audio streams selected by ANSWER.
  ANSWER_SELECT_MULTIPLE_VIDEO,   // Multiple video streams selected by ANSWER.
  ANSWER_SELECT_INVALID_INDEX,    // Invalid index was selected.
  ANSWER_NO_AUDIO_OR_VIDEO,       // ANSWER not select audio or video.
  AUDIO_CAPTURE_ERROR,            // Error occurred in audio capturing.
  VIDEO_CAPTURE_ERROR,            // Error occurred in video capturing.
  RTP_STREAM_ERROR,               // Error reported by RtpStream.
  ENCODING_ERROR,                 // Error occurred in encoding.
  CAST_TRANSPORT_ERROR,           // Error occurred in cast transport.
};

enum DeviceCapability {
  AUDIO_ONLY,
  VIDEO_ONLY,
  AUDIO_AND_VIDEO,
};

constexpr char kRemotingNamespace[] = "urn:x-cast:com.google.cast.remoting";
constexpr char kWebRtcNamespace[] = "urn:x-cast:com.google.cast.webrtc";

struct CastMessage {
  std::string message_namespace;
  std::string json_format_data;  // The content of the message.
};

class CastMessageChannel {
 public:
  virtual ~CastMessageChannel() {}
  virtual void Send(const CastMessage& message) = 0;
};

struct CastSinkInfo {
  CastSinkInfo();
  ~CastSinkInfo();
  CastSinkInfo(const CastSinkInfo& sink_info);

  net::IPAddress ip_address;
  std::string model_name;
  std::string friendly_name;
  DeviceCapability capability;
};

class SessionObserver {
 public:
  virtual ~SessionObserver() {}

  // Called when error occurred. The session will be stopped.
  virtual void OnError(SessionError error) = 0;

  // Called when session completes starting.
  virtual void DidStart() = 0;

  // Called when the session is stopped.
  virtual void DidStop() = 0;
};

class ResourceProvider {
 public:
  virtual ~ResourceProvider() {}

  virtual void GetVideoCaptureHost(
      media::mojom::VideoCaptureHostRequest request) = 0;
  virtual void GetNetworkContext(
      network::mojom::NetworkContextRequest request) = 0;
  // TODO(xjz): Add interface to get AudioCaptureHost.
  // TODO(xjz): Add interface for HW encoder profiles query and VEA create
  // support.
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_INTERFACE_H_
