// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_MEDIA_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_MEDIA_CLIENT_H_

#include "ash/public/interfaces/media.mojom.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

class MediaClient : public ash::mojom::MediaClient,
                    MediaCaptureDevicesDispatcher::Observer {
 public:
  MediaClient();
  ~MediaClient() override;

  // Returns a pointer to the singleton MediaClient, or nullptr if none exists.
  static MediaClient* Get();

  // ash::mojom::MediaClient:
  void HandleMediaNextTrack() override;
  void HandleMediaPlayPause() override;
  void HandleMediaPrevTrack() override;
  void RequestCaptureState() override;
  void SuspendMediaSessions() override;

  // MediaCaptureDevicesDispatcher::Observer:
  void OnRequestUpdate(int render_process_id,
                       int render_frame_id,
                       content::MediaStreamType stream_type,
                       const content::MediaRequestState state) override;

 private:
  // Returns the media capture state for the current user at
  // |user_index|. (Note that this isn't stable, see implementation comment on
  // RequestCaptureState()).
  ash::mojom::MediaCaptureState GetMediaCaptureStateByIndex(int user_index);

  ash::mojom::MediaControllerPtr media_controller_;

  mojo::AssociatedBinding<ash::mojom::MediaClient> binding_;

  base::WeakPtrFactory<MediaClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaClient);
};

#endif  // CHROME_BROWSER_UI_ASH_MEDIA_CLIENT_H_
