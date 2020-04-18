// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_DESKTOP_CAPTURE_ACCESS_HANDLER_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_DESKTOP_CAPTURE_ACCESS_HANDLER_H_

#include <list>

#include "chrome/browser/media/capture_access_handler_base.h"
#include "chrome/browser/media/media_access_handler.h"

namespace extensions {
class Extension;
}

// MediaAccessHandler for DesktopCapture API.
class DesktopCaptureAccessHandler : public CaptureAccessHandlerBase {
 public:
  DesktopCaptureAccessHandler();
  ~DesktopCaptureAccessHandler() override;

  // MediaAccessHandler implementation.
  bool SupportsStreamType(content::WebContents* web_contents,
                          const content::MediaStreamType type,
                          const extensions::Extension* extension) override;
  bool CheckMediaAccessPermission(
      content::RenderFrameHost* render_frame_host,
      const GURL& security_origin,
      content::MediaStreamType type,
      const extensions::Extension* extension) override;
  void HandleRequest(content::WebContents* web_contents,
                     const content::MediaStreamRequest& request,
                     const content::MediaResponseCallback& callback,
                     const extensions::Extension* extension) override;

 private:
  void ProcessScreenCaptureAccessRequest(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback,
      const extensions::Extension* extension);

  // Returns whether desktop capture is always approved for |extension|.
  // Currently component extensions and some whitelisted extensions are default
  // approved.
  static bool IsDefaultApproved(const extensions::Extension* extension);
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_DESKTOP_CAPTURE_ACCESS_HANDLER_H_
