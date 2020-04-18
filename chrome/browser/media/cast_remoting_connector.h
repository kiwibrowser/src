// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_CAST_REMOTING_CONNECTOR_H_
#define CHROME_BROWSER_MEDIA_CAST_REMOTING_CONNECTOR_H_

#include <set>

#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "components/sessions/core/session_id.h"
#include "media/mojo/interfaces/mirror_service_remoting.mojom.h"
#include "media/mojo/interfaces/remoting.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace media_router {
class MediaRouter;
}

// CastRemotingConnector connects a single source (a media element in a render
// frame) with a single sink (a media player in a remote device). There is one
// instance of a CastRemotingConnector per source WebContents (representing a
// collection of render frames), and it is created on-demand. The source in the
// render process represents itself by providing a media::mojom::RemotingSource
// service instance. The sink is represented by a MediaRemoter in the Cast Media
// Router Provider that handles the communication with the remote device. The
// CastRemotingConnector and the MediaRemoter can communicate with each other
// through the media::mojom::MirrorServiceRemoter and
// media::mojom::MirrorServiceRemotingSource interfaces when a sink that is
// capable of remoting is available.
//
// Whenever a candidate media source is created in a render frame,
// ChromeContentBrowserClient will call CreateMediaRemoter() to instantiate a
// media::mojom::Remoter associated with the connector. A corresponding
// media::mojom::RemotingSource is provided by the caller for communications
// back to the media source in the render frame: The connector uses this to
// notify when a sink becomes available for remoting, and to pass binary
// messages from the sink back to the source.
//
// When the CastRemotingConnector is created, it registers itself in the
// media_router::MediaRouter with a tab ID that uniquely identifies it. When a
// mirroring route is created and available for remoting, the Cast MRP will
// create a MediaRemoter and notify MediaRouter, which notifies the
// CastRemotingConnector registered under the tab ID being remoted. At this
// point, the CastRemotingConnector can communicate with the MediaRemoter. When
// CastRemotingConnector gets notified that a sink is available, it notifies all
// RemotingSources, and some time later a RemotingSource can request the start
// of a remoting session.
//
// Note that only one RemotingSource can remote media at a time. Therefore,
// CastRemotingConnector must mediate among simultaneous requests for media
// remoting, and only allow one at a time. Currently, the policy is "first come,
// first served."
//
// When starting a remoting session, the Cast MRP will also set up a Cast
// Streaming session to provide a bitstream transport for the media. Once this
// is done, the MediaRemoter notifies the CastRemotingConnector. Then,
// CastRemotingConnector knows it can look-up and pass the mojo data pipe
// handles to CastRemotingSenders, and the remoting session will be fully active
// The CastRemotingConnector is responsible for passing small binary messages
// between the source and sink, while the CastRemotingSender handles the
// high-volume media data transfer.
//
// Please see the unit tests in cast_remoting_connector_unittest.cc as a
// reference for how CastRemotingConnector and a MediaRemoter interact to
// start/execute/stop remoting sessions.
class CastRemotingConnector : public base::SupportsUserData::Data,
                              public media::mojom::MirrorServiceRemotingSource {
 public:
  ~CastRemotingConnector() final;

  // Returns the instance of the CastRemotingConnector associated with
  // |source_contents|, creating a new instance if needed. Returns nullptr if
  // |source_contents| doesn't have a valid tab ID.
  static CastRemotingConnector* Get(content::WebContents* source_contents);

  // Used by ChromeContentBrowserClient to request a binding to a new
  // Remoter for each new source in a render frame.
  static void CreateMediaRemoter(content::RenderFrameHost* render_frame_host,
                                 media::mojom::RemotingSourcePtr source,
                                 media::mojom::RemoterRequest request);

  // Called when a MediaRemoter is created and started in the Cast MRP. This
  // call connects the CastRemotingConnector with the MediaRemoter. Remoting
  // sessions can only be started after this is called.
  void ConnectToService(
      media::mojom::MirrorServiceRemotingSourceRequest source_request,
      media::mojom::MirrorServiceRemoterPtr remoter);

 private:
  // Allow unit tests access to the private constructor and CreateBridge()
  // method, since unit tests don't have a complete browser (i.e., with a
  // WebContents and RenderFrameHost) to work with.
  friend class CastRemotingConnectorTest;

  // Implementation of the media::mojom::Remoter service for a single source in
  // a render frame. This is just a "lightweight bridge" that delegates calls
  // back-and-forth between a CastRemotingConnector and a
  // media::mojom::RemotingSource. An instance of this class is owned by its
  // mojo message pipe.
  class RemotingBridge;

  // Main constructor. |tab_id| refers to any remoted content managed
  // by this instance (i.e., any remoted content from one tab/WebContents).
  CastRemotingConnector(media_router::MediaRouter* router, SessionID tab_id);

  // Creates a RemotingBridge that implements the requested Remoter service, and
  // binds it to the interface |request|.
  void CreateBridge(media::mojom::RemotingSourcePtr source,
                    media::mojom::RemoterRequest request);

  // Called by the RemotingBridge constructor/destructor to register/deregister
  // an instance. This allows this connector to broadcast notifications to all
  // active sources.
  void RegisterBridge(RemotingBridge* bridge);
  void DeregisterBridge(RemotingBridge* bridge,
                        media::mojom::RemotingStopReason reason);

  // media::mojom::MirrorServiceRemotingSource implementations.
  void OnSinkAvailable(media::mojom::RemotingSinkMetadataPtr metadata) override;
  void OnMessageFromSink(const std::vector<uint8_t>& message) override;
  void OnStopped(media::mojom::RemotingStopReason reason) override;
  void OnError() override;

  // These methods are called by RemotingBridge to forward media::mojom::Remoter
  // calls from a source through to this connector. They ensure that only one
  // source is allowed to be in a remoting session at a time, and that no source
  // may interfere with any other.
  void StartRemoting(RemotingBridge* bridge);
  void StartRemotingDataStreams(
      RemotingBridge* bridge,
      mojo::ScopedDataPipeConsumerHandle audio_pipe,
      mojo::ScopedDataPipeConsumerHandle video_pipe,
      media::mojom::RemotingDataStreamSenderRequest audio_sender_request,
      media::mojom::RemotingDataStreamSenderRequest video_sender_request);
  void StopRemoting(RemotingBridge* bridge,
                    media::mojom::RemotingStopReason reason);
  void SendMessageToSink(RemotingBridge* bridge,
                         const std::vector<uint8_t>& message);
  void EstimateTransmissionCapacity(
      media::mojom::Remoter::EstimateTransmissionCapacityCallback callback);

  // Called when RTP streams are started.
  void OnDataStreamsStarted(
      mojo::ScopedDataPipeConsumerHandle audio_pipe,
      mojo::ScopedDataPipeConsumerHandle video_pipe,
      media::mojom::RemotingDataStreamSenderRequest audio_sender_request,
      media::mojom::RemotingDataStreamSenderRequest video_sender_request,
      int32_t audio_stream_id,
      int32_t video_stream_id);

  // Error handlers for message sending during an active remoting session. When
  // a failure occurs, these immediately force-stop remoting.
  void OnDataSendFailed();

  // Called when any connection error/lost occurs with the MediaRemoter.
  void OnMirrorServiceStopped();

  media_router::MediaRouter* const media_router_;

  const SessionID tab_id_;

  // Describes the remoting sink's metadata and its enabled features. The sink's
  // metadata is updated by the mirror service calling OnSinkAvailable() and
  // cleared when remoting stops.
  media::mojom::RemotingSinkMetadata sink_metadata_;

  // Set of registered RemotingBridges, maintained by RegisterBridge() and
  // DeregisterBridge(). These pointers are always valid while they are in this
  // set.
  std::set<RemotingBridge*> bridges_;

  // When non-null, an active remoting session is taking place, with this
  // pointing to the RemotingBridge being used to communicate with the source.
  RemotingBridge* active_bridge_;

  mojo::Binding<media::mojom::MirrorServiceRemotingSource> binding_;
  media::mojom::MirrorServiceRemoterPtr remoter_;

  // Produces weak pointers that are only valid for the current remoting
  // session. This is used to cancel any outstanding callbacks when a remoting
  // session is stopped.
  base::WeakPtrFactory<CastRemotingConnector> weak_factory_;

  // Key used with the base::SupportsUserData interface to search for an
  // instance of CastRemotingConnector owned by a WebContents.
  static const void* const kUserDataKey;

  DISALLOW_COPY_AND_ASSIGN(CastRemotingConnector);
};

#endif  // CHROME_BROWSER_MEDIA_CAST_REMOTING_CONNECTOR_H_
