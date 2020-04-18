// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_DESKTOP_STREAMS_REGISTRY_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_DESKTOP_STREAMS_REGISTRY_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "url/gurl.h"

// DesktopStreamsRegistry is used to store accepted desktop media streams for
// Desktop Capture API. Single instance of this class is created per browser in
// MediaCaptureDevicesDispatcher.
class DesktopStreamsRegistry {
 public:
  DesktopStreamsRegistry();
  ~DesktopStreamsRegistry();

  // Adds new stream to the registry. Called by the implementation of
  // desktopCapture.chooseDesktopMedia() API after user has approved access to
  // |source| for the |origin|. Returns identifier of the new stream.
  // |render_frame_id| refers to the RenderFrame requesting the stream.
  std::string RegisterStream(int render_process_id,
                             int render_frame_id,
                             const GURL& origin,
                             const content::DesktopMediaID& source,
                             const std::string& extension_name);

  // Validates stream identifier specified in getUserMedia(). Returns null
  // DesktopMediaID if the specified |id| is invalid, i.e. wasn't generated
  // using RegisterStream() or if it was generated for a different
  // RenderFrame/origin. Otherwise returns ID of the source and removes it from
  // the registry.
  content::DesktopMediaID RequestMediaForStreamId(const std::string& id,
                                                  int render_process_id,
                                                  int render_frame_id,
                                                  const GURL& origin,
                                                  std::string* extension_name);

 private:
  // Type used to store list of accepted desktop media streams.
  struct ApprovedDesktopMediaStream {
    ApprovedDesktopMediaStream();

    int render_process_id;
    int render_frame_id;
    GURL origin;
    content::DesktopMediaID source;
    std::string extension_name;
  };
  typedef std::map<std::string, ApprovedDesktopMediaStream> StreamsMap;

  // Helper function that removes an expired stream from the registry.
  void CleanupStream(const std::string& id);

  StreamsMap approved_streams_;

  DISALLOW_COPY_AND_ASSIGN(DesktopStreamsRegistry);
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_DESKTOP_STREAMS_REGISTRY_H_
