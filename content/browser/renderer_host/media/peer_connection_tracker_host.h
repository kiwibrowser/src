// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_PEER_CONNECTION_TRACKER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_PEER_CONNECTION_TRACKER_HOST_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/power_monitor/power_observer.h"
#include "content/common/media/peer_connection_tracker.mojom.h"
#include "content/public/browser/browser_associated_interface.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"

struct PeerConnectionInfo;

namespace base {
class ListValue;
}  // namespace base

namespace content {

// This class is the host for PeerConnectionTracker in the browser process
// managed by RenderProcessHostImpl. It receives PeerConnection events from
// PeerConnectionTracker as IPC messages that it forwards to WebRTCInternals.
// It also forwards browser process events to PeerConnectionTracker via IPC.
class PeerConnectionTrackerHost
    : public BrowserMessageFilter,
      public base::PowerObserver,
      public BrowserAssociatedInterface<mojom::PeerConnectionTrackerHost>,
      public mojom::PeerConnectionTrackerHost {
 public:
  explicit PeerConnectionTrackerHost(int render_process_id);

  // content::BrowserMessageFilter override.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OverrideThreadForMessage(const IPC::Message& message,
                                BrowserThread::ID* thread) override;
  void OnChannelConnected(int32_t peer_pid) override;
  void OnChannelClosing() override;

  // base::PowerObserver override.
  void OnSuspend() override;

 protected:
  ~PeerConnectionTrackerHost() override;

 private:
  // Handlers for IPC messages coming from the renderer.
  void OnAddPeerConnection(const PeerConnectionInfo& info);
  void OnAddStats(int lid, const base::ListValue& value);
  void SendOnSuspendOnUIThread();

  // mojom::PeerConnectionTrackerHost implementation.
  void RemovePeerConnection(int lid) override;
  void UpdatePeerConnection(int lid,
                            const std::string& type,
                            const std::string& value) override;
  void GetUserMedia(const std::string& origin,
                    bool audio,
                    bool video,
                    const std::string& audio_constraints,
                    const std::string& video_constraints) override;
  void WebRtcEventLogWrite(int lid, const std::string& output) override;

  int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(PeerConnectionTrackerHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_PEER_CONNECTION_TRACKER_HOST_H_
