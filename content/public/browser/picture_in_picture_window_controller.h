// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_
#define CONTENT_PUBLIC_BROWSER_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_

#include "content/common/content_export.h"

namespace gfx {
class Size;
}  // namespace gfx

namespace viz {
class SurfaceId;
}  // namespace viz

namespace content {
class OverlayWindow;
class WebContents;

// Interface for Picture in Picture window controllers. This is currently tied
// to a WebContents |initiator| and created when a Picture in Picture window is
// to be shown. This allows creation of a single window for the initiator
// WebContents.
class PictureInPictureWindowController {
 public:
  // Gets a reference to the controller associated with |initiator| and creates
  // one if it does not exist. The returned pointer is guaranteed to be
  // non-null.
  CONTENT_EXPORT static PictureInPictureWindowController*
  GetOrCreateForWebContents(WebContents* initiator);

  virtual ~PictureInPictureWindowController() = default;

  // Shows the Picture-in-Picture window.
  // Returns the size of the window in pixels.
  virtual gfx::Size Show() = 0;

  virtual void Close() = 0;
  virtual void EmbedSurface(const viz::SurfaceId& surface_id,
                            const gfx::Size& natural_size) = 0;
  virtual OverlayWindow* GetWindowForTesting() = 0;
  virtual void UpdateLayerBounds() = 0;
  virtual bool IsPlayerActive() = 0;
  virtual WebContents* GetInitiatorWebContents() = 0;

  // Commands.
  // Returns true if the player is active (i.e. currently playing) after this
  // call.
  virtual bool TogglePlayPause() = 0;

 protected:
  // Use PictureInPictureWindowController::GetOrCreateForWebContents() to
  // create an instance.
  PictureInPictureWindowController() = default;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_
