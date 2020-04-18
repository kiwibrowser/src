// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cast_remoting_connector.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "chrome/browser/media/router/media_router.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "components/mirroring/browser/cast_remoting_sender.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using content::BrowserThread;
using media::mojom::RemotingStartFailReason;
using media::mojom::RemotingStopReason;
using media::mojom::RemotingSinkMetadata;

class CastRemotingConnector::RemotingBridge : public media::mojom::Remoter {
 public:
  // Constructs a "bridge" to delegate calls between the given |source| and
  // |connector|. |connector| must be valid at the time of construction, but is
  // otherwise a weak pointer that can become invalid during the lifetime of a
  // RemotingBridge.
  RemotingBridge(media::mojom::RemotingSourcePtr source,
                 CastRemotingConnector* connector)
      : source_(std::move(source)), connector_(connector) {
    DCHECK(connector_);
    source_.set_connection_error_handler(
        base::BindOnce(&RemotingBridge::Stop, base::Unretained(this),
                       RemotingStopReason::SOURCE_GONE));
    connector_->RegisterBridge(this);
  }

  ~RemotingBridge() final {
    if (connector_)
      connector_->DeregisterBridge(this, RemotingStopReason::SOURCE_GONE);
  }

  // The CastRemotingConnector calls these to call back to the RemotingSource.
  void OnSinkAvailable(const RemotingSinkMetadata& metadata) {
    source_->OnSinkAvailable(metadata.Clone());
  }
  void OnSinkGone() { source_->OnSinkGone(); }
  void OnStarted() { source_->OnStarted(); }
  void OnStartFailed(RemotingStartFailReason reason) {
    source_->OnStartFailed(reason);
  }
  void OnMessageFromSink(const std::vector<uint8_t>& message) {
    source_->OnMessageFromSink(message);
  }
  void OnStopped(RemotingStopReason reason) { source_->OnStopped(reason); }

  // The CastRemotingConnector calls this when it is no longer valid.
  void OnCastRemotingConnectorDestroyed() {
    connector_ = nullptr;
  }

  // media::mojom::Remoter implementation. The source calls these to start/stop
  // media remoting and send messages to the sink. These simply delegate to the
  // CastRemotingConnector, which mediates to establish only one remoting
  // session among possibly multiple requests. The connector will respond to
  // this request by calling one of: OnStarted() or OnStartFailed().
  void Start() final {
    if (connector_)
      connector_->StartRemoting(this);
  }
  void StartDataStreams(
      mojo::ScopedDataPipeConsumerHandle audio_pipe,
      mojo::ScopedDataPipeConsumerHandle video_pipe,
      media::mojom::RemotingDataStreamSenderRequest audio_sender_request,
      media::mojom::RemotingDataStreamSenderRequest video_sender_request)
      final {
    if (connector_) {
      connector_->StartRemotingDataStreams(
          this, std::move(audio_pipe), std::move(video_pipe),
          std::move(audio_sender_request), std::move(video_sender_request));
    }
  }
  void Stop(RemotingStopReason reason) final {
    if (connector_)
      connector_->StopRemoting(this, reason);
  }
  void SendMessageToSink(const std::vector<uint8_t>& message) final {
    if (connector_)
      connector_->SendMessageToSink(this, message);
  }
  void EstimateTransmissionCapacity(
      media::mojom::Remoter::EstimateTransmissionCapacityCallback callback)
      final {
    if (connector_)
      connector_->EstimateTransmissionCapacity(std::move(callback));
    else
      std::move(callback).Run(0);
  }

 private:
  media::mojom::RemotingSourcePtr source_;

  // Weak pointer. Will be set to nullptr if the CastRemotingConnector is
  // destroyed before this RemotingBridge.
  CastRemotingConnector* connector_;

  DISALLOW_COPY_AND_ASSIGN(RemotingBridge);
};

// static
const void* const CastRemotingConnector::kUserDataKey = &kUserDataKey;

// static
CastRemotingConnector* CastRemotingConnector::Get(
    content::WebContents* contents) {
  DCHECK(contents);
  CastRemotingConnector* connector =
      static_cast<CastRemotingConnector*>(contents->GetUserData(kUserDataKey));
  if (!connector) {
    // TODO(xjz): Use TabAndroid::GetAndroidId() to get the tab ID when support
    // remoting on Android.
    const SessionID tab_id = SessionTabHelper::IdForTab(contents);
    if (!tab_id.is_valid())
      return nullptr;
    if (!media_router::MediaRouterEnabled(contents->GetBrowserContext()))
      return nullptr;
    connector = new CastRemotingConnector(
        media_router::MediaRouterFactory::GetApiForBrowserContext(
            contents->GetBrowserContext()),
        tab_id);
    contents->SetUserData(kUserDataKey, base::WrapUnique(connector));
  }
  return connector;
}

// static
void CastRemotingConnector::CreateMediaRemoter(
    content::RenderFrameHost* host,
    media::mojom::RemotingSourcePtr source,
    media::mojom::RemoterRequest request) {
  DCHECK(host);
  auto* const contents = content::WebContents::FromRenderFrameHost(host);
  if (!contents)
    return;
  CastRemotingConnector* const connector = CastRemotingConnector::Get(contents);
  if (!connector)
    return;
  connector->CreateBridge(std::move(source), std::move(request));
}

CastRemotingConnector::CastRemotingConnector(media_router::MediaRouter* router,
                                             SessionID tab_id)
    : media_router_(router),
      tab_id_(tab_id),
      active_bridge_(nullptr),
      binding_(this),
      weak_factory_(this) {
  VLOG(2) << "Register CastRemotingConnector for tab_id = " << tab_id_;
  media_router_->RegisterRemotingSource(tab_id_, this);
}

CastRemotingConnector::~CastRemotingConnector() {
  // Assume nothing about destruction/shutdown sequence of a tab. For example,
  // it's possible the owning WebContents will be destroyed before the Mojo
  // message pipes to the RemotingBridges have been closed.
  if (active_bridge_)
    StopRemoting(active_bridge_, RemotingStopReason::ROUTE_TERMINATED);
  for (RemotingBridge* notifyee : bridges_) {
    notifyee->OnSinkGone();
    notifyee->OnCastRemotingConnectorDestroyed();
  }
  media_router_->UnregisterRemotingSource(tab_id_);
}

void CastRemotingConnector::ConnectToService(
    media::mojom::MirrorServiceRemotingSourceRequest source_request,
    media::mojom::MirrorServiceRemoterPtr remoter) {
  DCHECK(!binding_.is_bound());
  DCHECK(!remoter_);
  DCHECK(remoter);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__;

  binding_.Bind(std::move(source_request));
  binding_.set_connection_error_handler(base::BindOnce(
      &CastRemotingConnector::OnMirrorServiceStopped, base::Unretained(this)));
  remoter_ = std::move(remoter);
  remoter_.set_connection_error_handler(base::BindOnce(
      &CastRemotingConnector::OnMirrorServiceStopped, base::Unretained(this)));
}

void CastRemotingConnector::OnMirrorServiceStopped() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__;

  if (binding_.is_bound())
    binding_.Close();
  remoter_.reset();

  sink_metadata_ = RemotingSinkMetadata();
  if (active_bridge_)
    StopRemoting(active_bridge_, RemotingStopReason::SERVICE_GONE);
  for (RemotingBridge* notifyee : bridges_)
    notifyee->OnSinkGone();
}

void CastRemotingConnector::CreateBridge(media::mojom::RemotingSourcePtr source,
                                         media::mojom::RemoterRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<RemotingBridge>(std::move(source), this),
      std::move(request));
}

void CastRemotingConnector::RegisterBridge(RemotingBridge* bridge) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(bridges_.find(bridge) == bridges_.end());

  bridges_.insert(bridge);
  if (remoter_ && !active_bridge_)
    bridge->OnSinkAvailable(sink_metadata_);
}

void CastRemotingConnector::DeregisterBridge(RemotingBridge* bridge,
                                             RemotingStopReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(bridges_.find(bridge) != bridges_.end());

  if (bridge == active_bridge_)
    StopRemoting(bridge, reason);
  bridges_.erase(bridge);
}

void CastRemotingConnector::StartRemoting(RemotingBridge* bridge) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(bridges_.find(bridge) != bridges_.end());
  VLOG(2) << __func__;

  // Refuse to start if there is no remoting route available, or if remoting is
  // already active.
  if (!remoter_) {
    VLOG(2) << "Remoting start failed: No mirror service connected.";
    bridge->OnStartFailed(RemotingStartFailReason::SERVICE_NOT_CONNECTED);
    return;
  }
  if (active_bridge_) {
    VLOG(2) << "Remoting start failed: Cannot start multiple.";
    bridge->OnStartFailed(RemotingStartFailReason::CANNOT_START_MULTIPLE);
    return;
  }

  // Notify all other sources that the sink is no longer available for remoting.
  // A race condition is possible, where one of the other sources will try to
  // start remoting before receiving this notification; but that attempt will
  // just fail later on.
  for (RemotingBridge* notifyee : bridges_) {
    if (notifyee == bridge)
      continue;
    notifyee->OnSinkGone();
  }

  active_bridge_ = bridge;
  remoter_->Start();

  // Assume the remoting session is always started successfully. If any failure
  // occurs, OnError() will be called.
  bridge->OnStarted();
}

void CastRemotingConnector::StartRemotingDataStreams(
    RemotingBridge* bridge,
    mojo::ScopedDataPipeConsumerHandle audio_pipe,
    mojo::ScopedDataPipeConsumerHandle video_pipe,
    media::mojom::RemotingDataStreamSenderRequest audio_sender_request,
    media::mojom::RemotingDataStreamSenderRequest video_sender_request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__;

  // Refuse to start if there is no remoting route available, or if remoting is
  // not active for this |bridge|.
  if (!remoter_ || active_bridge_ != bridge)
    return;
  // Also, if neither audio nor video pipe was provided, or if a request for a
  // RemotingDataStreamSender was not provided for a data pipe, error-out early.
  if ((!audio_pipe.is_valid() && !video_pipe.is_valid()) ||
      (audio_pipe.is_valid() && !audio_sender_request.is_pending()) ||
      (video_pipe.is_valid() && !video_sender_request.is_pending())) {
    StopRemoting(active_bridge_, RemotingStopReason::DATA_SEND_FAILED);
    return;
  }

  const bool want_audio = audio_sender_request.is_pending();
  const bool want_video = video_sender_request.is_pending();
  remoter_->StartDataStreams(
      want_audio, want_video,
      base::BindOnce(&CastRemotingConnector::OnDataStreamsStarted,
                     weak_factory_.GetWeakPtr(), std::move(audio_pipe),
                     std::move(video_pipe), std::move(audio_sender_request),
                     std::move(video_sender_request)));
}

void CastRemotingConnector::OnDataStreamsStarted(
    mojo::ScopedDataPipeConsumerHandle audio_pipe,
    mojo::ScopedDataPipeConsumerHandle video_pipe,
    media::mojom::RemotingDataStreamSenderRequest audio_sender_request,
    media::mojom::RemotingDataStreamSenderRequest video_sender_request,
    int32_t audio_stream_id,
    int32_t video_stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(remoter_);
  VLOG(2) << __func__ << ": audio_stream_id = " << audio_stream_id
          << " video_stream_id = " << video_stream_id;

  if (!active_bridge_) {
    remoter_->Stop(media::mojom::RemotingStopReason::SOURCE_GONE);
    return;
  }

  if (audio_sender_request.is_pending() && audio_stream_id > -1) {
    mirroring::CastRemotingSender::FindAndBind(
        audio_stream_id, std::move(audio_pipe), std::move(audio_sender_request),
        base::BindOnce(&CastRemotingConnector::OnDataSendFailed,
                       weak_factory_.GetWeakPtr()));
  }
  if (video_sender_request.is_pending() && video_stream_id > -1) {
    mirroring::CastRemotingSender::FindAndBind(
        video_stream_id, std::move(video_pipe), std::move(video_sender_request),
        base::BindOnce(&CastRemotingConnector::OnDataSendFailed,
                       weak_factory_.GetWeakPtr()));
  }
}

void CastRemotingConnector::StopRemoting(RemotingBridge* bridge,
                                         RemotingStopReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__ << ": reason = " << reason;

  if (active_bridge_ != bridge)
    return;

  active_bridge_ = nullptr;

  // Cancel all outstanding callbacks related to the remoting session.
  weak_factory_.InvalidateWeakPtrs();

  // Reset |sink_metadata_|. Remoting can only be started after
  // OnSinkAvailable() is called again.
  sink_metadata_ = RemotingSinkMetadata();

  // Prevent the source from trying to start again until the Cast Provider has
  // indicated the stop operation has completed.
  bridge->OnSinkGone();
  // Note: At this point, all sources should think the sink is gone.

  if (remoter_)
    remoter_->Stop(reason);

  bridge->OnStopped(reason);
}

void CastRemotingConnector::OnStopped(RemotingStopReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__ << ": reason = " << reason;

  if (active_bridge_) {
    // This call will reset |sink_metadata_| and notify the source that sink is
    // gone.
    StopRemoting(active_bridge_, reason);
  } else if (reason == RemotingStopReason::USER_DISABLED) {
    // Notify all the sources that the sink is gone. Remoting can only be
    // started after OnSinkAvailable() is called again.
    sink_metadata_ = RemotingSinkMetadata();
    for (RemotingBridge* notifyee : bridges_)
      notifyee->OnSinkGone();
  }
}

void CastRemotingConnector::SendMessageToSink(
    RemotingBridge* bridge, const std::vector<uint8_t>& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // During an active remoting session, simply pass all binary messages through
  // to the sink.
  if (!remoter_ || active_bridge_ != bridge)
    return;
  remoter_->SendMessageToSink(message);
}

void CastRemotingConnector::OnMessageFromSink(
    const std::vector<uint8_t>& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // During an active remoting session, simply pass all binary messages through
  // to the source.
  if (!active_bridge_)
    return;
  active_bridge_->OnMessageFromSink(message);
}

void CastRemotingConnector::EstimateTransmissionCapacity(
    media::mojom::Remoter::EstimateTransmissionCapacityCallback callback) {
  if (remoter_)
    remoter_->EstimateTransmissionCapacity(std::move(callback));
  else
    std::move(callback).Run(0);
}

void CastRemotingConnector::OnSinkAvailable(
    media::mojom::RemotingSinkMetadataPtr metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__;

  // The receiver's metadata should be unchanged during an active remoting
  // session.
  if (active_bridge_) {
    LOG(WARNING) << "Unexpected OnSinkAvailable() call during an active"
                 << "remoting session.";
    return;
  }
  sink_metadata_ = *metadata;
#if !defined(OS_ANDROID)
  sink_metadata_.features.push_back(
      media::mojom::RemotingSinkFeature::RENDERING);
#endif

  for (RemotingBridge* notifyee : bridges_)
    notifyee->OnSinkAvailable(sink_metadata_);
}

void CastRemotingConnector::OnError() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__;

  if (active_bridge_)
    StopRemoting(active_bridge_, RemotingStopReason::UNEXPECTED_FAILURE);
}

void CastRemotingConnector::OnDataSendFailed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(2) << __func__;

  // A single data send failure is treated as fatal to an active remoting
  // session.
  if (active_bridge_)
    StopRemoting(active_bridge_, RemotingStopReason::DATA_SEND_FAILED);
}
