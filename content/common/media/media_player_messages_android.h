// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_PLAYER_MESSAGES_ANDROID_H_
#define CONTENT_COMMON_MEDIA_MEDIA_PLAYER_MESSAGES_ANDROID_H_

// IPC messages for android media player.

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "ipc/ipc_message_macros.h"
#include "media/blink/renderer_media_player_interface.h"
#include "media/gpu/ipc/common/media_param_traits.h"
#include "third_party/blink/public/platform/modules/remoteplayback/web_remote_playback_availability.h"
#include "ui/gfx/geometry/rect_f.h"
#include "url/gurl.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START MediaPlayerMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(MediaPlayerHostMsg_Initialize_Type,
                          MEDIA_PLAYER_TYPE_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(blink::WebRemotePlaybackAvailability,
                          blink::WebRemotePlaybackAvailability::kLast)

// Parameters to describe a media player
IPC_STRUCT_BEGIN(MediaPlayerHostMsg_Initialize_Params)
  IPC_STRUCT_MEMBER(MediaPlayerHostMsg_Initialize_Type, type)
  IPC_STRUCT_MEMBER(int, player_id)
  IPC_STRUCT_MEMBER(GURL, url)
  IPC_STRUCT_MEMBER(GURL, site_for_cookies)
  IPC_STRUCT_MEMBER(GURL, frame_url)
  IPC_STRUCT_MEMBER(bool, allow_credentials)
  IPC_STRUCT_MEMBER(int, delegate_id)
IPC_STRUCT_END()

// Chrome for Android seek message sequence is:
// 1. Renderer->Browser MediaPlayerHostMsg_Seek
//    This is the beginning of actual seek flow in response to web app requests
//    for seeks and browser MediaPlayerMsg_SeekRequests. With this message,
//    the renderer asks browser to perform actual seek. At most one of these
//    actual seeks will be in process between this message and renderer's later
//    receipt of MediaPlayerMsg_SeekCompleted from the browser.
// 2. Browser->Renderer MediaPlayerMsg_SeekCompleted
//    Once the browser determines the seek is complete, it sends this message to
//    notify the renderer of seek completion.
//
// Other seek-related IPC messages:
// Browser->Renderer MediaPlayerMsg_SeekRequest
//    Browser requests to begin a seek. All browser-initiated seeks must begin
//    with this request. Renderer controls actual seek initiation via the normal
//    seek flow, above, keeping web apps aware of seeks. These requests are
//    also allowed while another actual seek is in progress.
//
// Messages for notifying the render process of media playback status -------

// Media buffering has updated.
IPC_MESSAGE_ROUTED2(MediaPlayerMsg_MediaBufferingUpdate,
                    int /* player_id */,
                    int /* percent */)

// A media playback error has occurred.
IPC_MESSAGE_ROUTED2(MediaPlayerMsg_MediaError,
                    int /* player_id */,
                    int /* error */)

// Playback is completed.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_MediaPlaybackCompleted,
                    int /* player_id */)

// Media metadata has changed.
IPC_MESSAGE_ROUTED5(MediaPlayerMsg_MediaMetadataChanged,
                    int /* player_id */,
                    base::TimeDelta /* duration */,
                    int /* width */,
                    int /* height */,
                    bool /* success */)

// Requests renderer player to ask its client (blink HTMLMediaElement) to seek.
IPC_MESSAGE_ROUTED2(MediaPlayerMsg_SeekRequest,
                    int /* player_id */,
                    base::TimeDelta /* time_to_seek_to */)

// Media seek is completed.
IPC_MESSAGE_ROUTED2(MediaPlayerMsg_SeekCompleted,
                    int /* player_id */,
                    base::TimeDelta /* current_time */)

// Video size has changed.
IPC_MESSAGE_ROUTED3(MediaPlayerMsg_MediaVideoSizeChanged,
                    int /* player_id */,
                    int /* width */,
                    int /* height */)

// The current play time has updated.
IPC_MESSAGE_ROUTED3(MediaPlayerMsg_MediaTimeUpdate,
                    int /* player_id */,
                    base::TimeDelta /* current_timestamp */,
                    base::TimeTicks /* current_time_ticks */)

// The player has been released.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_MediaPlayerReleased,
                    int /* player_id */)

// The player exited fullscreen.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_DidExitFullscreen,
                    int /* player_id */)

// The player started playing.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_DidMediaPlayerPlay,
                    int /* player_id */)

// The player was paused.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_DidMediaPlayerPause,
                    int /* player_id */)

// Clank has connected to the remote device.
IPC_MESSAGE_ROUTED2(MediaPlayerMsg_ConnectedToRemoteDevice,
                    int /* player_id */,
                    std::string /* remote_playback_message */)

// Clank has disconnected from the remote device.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_DisconnectedFromRemoteDevice,
                    int /* player_id */)

// The remote playback has started.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_RemotePlaybackStarted,
                    int /* player_id */)

// The remote playback device selection has been cancelled.
IPC_MESSAGE_ROUTED1(MediaPlayerMsg_CancelledRemotePlaybackRequest,
                    int /* player_id */)

// The availability of remote devices has changed
IPC_MESSAGE_ROUTED2(MediaPlayerMsg_RemoteRouteAvailabilityChanged,
                    int /* player_id */,
                    blink::WebRemotePlaybackAvailability /* availability */)

// Messages for controlling the media playback in browser process ----------

// Destroy the media player object.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_DestroyMediaPlayer,
                    int /* player_id */)

// Initialize a media player object.
IPC_MESSAGE_ROUTED1(
    MediaPlayerHostMsg_Initialize,
    MediaPlayerHostMsg_Initialize_Params)

// Pause the player.
IPC_MESSAGE_ROUTED2(MediaPlayerHostMsg_Pause,
                    int /* player_id */,
                    bool /* is_media_related_action */)

// Release player resources after it was suspended.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_SuspendAndRelease, int /* player_id */)

// Perform a seek.
IPC_MESSAGE_ROUTED2(MediaPlayerHostMsg_Seek,
                    int /* player_id */,
                    base::TimeDelta /* time */)

// Start the player for playback.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_Start, int /* player_id */)

// Set the volume.
IPC_MESSAGE_ROUTED2(MediaPlayerHostMsg_SetVolume,
                    int /* player_id */,
                    double /* volume */)

// Set the poster image.
IPC_MESSAGE_ROUTED2(MediaPlayerHostMsg_SetPoster,
                    int /* player_id */,
                    GURL /* poster url */)

// Requests the player to enter fullscreen.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_EnterFullscreen, int /* player_id */)

// Play the media on a remote device, if possible.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_RequestRemotePlayback,
                    int /* player_id */)

// Control media playing on a remote device.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_RequestRemotePlaybackControl,
                    int /* player_id */)

// Stop playing media on a remote device.
IPC_MESSAGE_ROUTED1(MediaPlayerHostMsg_RequestRemotePlaybackStop,
                    int /* player_id */)

#endif  // #ifndef CONTENT_COMMON_MEDIA_MEDIA_PLAYER_MESSAGES_ANDROID_H_
