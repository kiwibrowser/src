// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_MEDIA_STREAM_CAPTURE_INDICATOR_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_MEDIA_STREAM_CAPTURE_INDICATOR_H_

#include <unordered_map>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/status_icons/status_icon_menu_model.h"
#include "content/public/common/media_stream_request.h"

namespace content {
class WebContents;
}  // namespace content

namespace gfx {
class ImageSkia;
}  // namespace gfx

class StatusIcon;

// This indicator is owned by MediaCaptureDevicesDispatcher
// (MediaCaptureDevicesDispatcher is a singleton).
class MediaStreamCaptureIndicator
    : public base::RefCountedThreadSafe<MediaStreamCaptureIndicator>,
      public StatusIconMenuModel::Delegate {
 public:
  MediaStreamCaptureIndicator();

  // Registers a new media stream for |web_contents| and returns UI object
  // that's used by the content layer to notify about state of the stream.
  std::unique_ptr<content::MediaStreamUI> RegisterMediaStream(
      content::WebContents* web_contents,
      const content::MediaStreamDevices& devices);

  // Overrides from StatusIconMenuModel::Delegate implementation.
  void ExecuteCommand(int command_id, int event_flags) override;

  // Returns true if the |web_contents| is capturing user media (e.g., webcam or
  // microphone input).
  bool IsCapturingUserMedia(content::WebContents* web_contents) const;

  // Returns true if the |web_contents| is capturing video (e.g., webcam).
  bool IsCapturingVideo(content::WebContents* web_contents) const;

  // Returns true if the |web_contents| is capturing audio (e.g., microphone).
  bool IsCapturingAudio(content::WebContents* web_contents) const;

  // Returns true if the |web_contents| itself is being mirrored (e.g., a source
  // of media for remote broadcast).
  bool IsBeingMirrored(content::WebContents* web_contents) const;

  // Called when STOP button in media capture notification is clicked.
  void NotifyStopped(content::WebContents* web_contents) const;

 private:
  class UIDelegate;
  class WebContentsDeviceUsage;
  friend class WebContentsDeviceUsage;

  friend class base::RefCountedThreadSafe<MediaStreamCaptureIndicator>;
  ~MediaStreamCaptureIndicator() override;

  // Following functions/variables are executed/accessed only on UI thread.

  // Called by WebContentsDeviceUsage when it's about to destroy itself, i.e.
  // when WebContents is being destroyed.
  void UnregisterWebContents(content::WebContents* web_contents);

  // Updates the status tray menu. Called by WebContentsDeviceUsage.
  void UpdateNotificationUserInterface();

  // Helpers to create and destroy status tray icon. Called from
  // UpdateNotificationUserInterface().
  void EnsureStatusTrayIconResources();
  void MaybeCreateStatusTrayIcon(bool audio, bool video);
  void MaybeDestroyStatusTrayIcon();

  // Gets the status icon image and the string to use as the tooltip.
  void GetStatusTrayIconInfo(bool audio,
                             bool video,
                             gfx::ImageSkia* image,
                             base::string16* tool_tip);

  // Reference to our status icon - owned by the StatusTray. If null,
  // the platform doesn't support status icons.
  StatusIcon* status_icon_ = nullptr;

  // A map that contains the usage counts of the opened capture devices for each
  // WebContents instance.
  std::unordered_map<content::WebContents*,
                     std::unique_ptr<WebContentsDeviceUsage>>
      usage_map_;

  // A vector which maps command IDs to their associated WebContents
  // instance. This is rebuilt each time the status tray icon context menu is
  // updated.
  typedef std::vector<content::WebContents*> CommandTargets;
  CommandTargets command_targets_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamCaptureIndicator);
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_MEDIA_STREAM_CAPTURE_INDICATOR_H_
