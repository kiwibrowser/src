// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/user_media_client_impl.h"

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "base/location.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner.h"
#include "content/renderer/media/stream/apply_constraints_processor.h"
#include "content/renderer/media/stream/media_stream_device_observer.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/webrtc/peer_connection_tracker.h"
#include "content/renderer/media/webrtc/webrtc_uma_histograms.h"
#include "content/renderer/media/webrtc_logging.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_user_gesture_indicator.h"
#include "third_party/blink/public/web/web_user_media_request.h"

namespace content {
namespace {

static int g_next_request_id = 0;

}  // namespace

UserMediaClientImpl::Request::Request(std::unique_ptr<UserMediaRequest> request)
    : user_media_request_(std::move(request)) {
  DCHECK(user_media_request_);
  DCHECK(apply_constraints_request_.IsNull());
  DCHECK(web_track_to_stop_.IsNull());
}

UserMediaClientImpl::Request::Request(
    const blink::WebApplyConstraintsRequest& request)
    : apply_constraints_request_(request) {
  DCHECK(!apply_constraints_request_.IsNull());
  DCHECK(!user_media_request_);
  DCHECK(web_track_to_stop_.IsNull());
}

UserMediaClientImpl::Request::Request(
    const blink::WebMediaStreamTrack& web_track_to_stop)
    : web_track_to_stop_(web_track_to_stop) {
  DCHECK(!web_track_to_stop_.IsNull());
  DCHECK(!user_media_request_);
  DCHECK(apply_constraints_request_.IsNull());
}

UserMediaClientImpl::Request::Request(Request&& other)
    : user_media_request_(std::move(other.user_media_request_)),
      apply_constraints_request_(other.apply_constraints_request_),
      web_track_to_stop_(other.web_track_to_stop_) {
#if DCHECK_IS_ON()
  int num_types = 0;
  if (IsUserMedia())
    num_types++;
  if (IsApplyConstraints())
    num_types++;
  if (IsStopTrack())
    num_types++;

  DCHECK_EQ(num_types, 1);
#endif
}

UserMediaClientImpl::Request& UserMediaClientImpl::Request::operator=(
    Request&& other) = default;
UserMediaClientImpl::Request::~Request() = default;

std::unique_ptr<UserMediaRequest>
UserMediaClientImpl::Request::MoveUserMediaRequest() {
  return std::move(user_media_request_);
}

UserMediaClientImpl::UserMediaClientImpl(
    RenderFrameImpl* render_frame,
    std::unique_ptr<UserMediaProcessor> user_media_processor,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : RenderFrameObserver(render_frame),
      user_media_processor_(std::move(user_media_processor)),
      apply_constraints_processor_(new ApplyConstraintsProcessor(
          base::BindRepeating(&UserMediaClientImpl::GetMediaDevicesDispatcher,
                              base::Unretained(this)),
          std::move(task_runner))),
      weak_factory_(this) {}

// base::Unretained(this) is safe here because |this| owns
// |user_media_processor_|.
UserMediaClientImpl::UserMediaClientImpl(
    RenderFrameImpl* render_frame,
    PeerConnectionDependencyFactory* dependency_factory,
    std::unique_ptr<MediaStreamDeviceObserver> media_stream_device_observer,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : UserMediaClientImpl(
          render_frame,
          std::make_unique<UserMediaProcessor>(
              render_frame,
              dependency_factory,
              std::move(media_stream_device_observer),
              base::BindRepeating(
                  &UserMediaClientImpl::GetMediaDevicesDispatcher,
                  base::Unretained(this))),
          std::move(task_runner)) {}

UserMediaClientImpl::~UserMediaClientImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Force-close all outstanding user media requests and local sources here,
  // before the outstanding WeakPtrs are invalidated, to ensure a clean
  // shutdown.
  WillCommitProvisionalLoad();
}

void UserMediaClientImpl::RequestUserMedia(
    const blink::WebUserMediaRequest& web_request) {
  // Save histogram data so we can see how much GetUserMedia is used.
  // The histogram counts the number of calls to the JS API
  // webGetUserMedia.
  UpdateWebRTCMethodCount(blink::WebRTCAPIName::kGetUserMedia);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!web_request.IsNull());
  DCHECK(web_request.Audio() || web_request.Video());
  // ownerDocument may be null if we are in a test.
  // In that case, it's OK to not check frame().
  DCHECK(web_request.OwnerDocument().IsNull() ||
         render_frame()->GetWebFrame() ==
             static_cast<blink::WebFrame*>(
                 web_request.OwnerDocument().GetFrame()));

  if (RenderThreadImpl::current()) {
    RenderThreadImpl::current()->peer_connection_tracker()->TrackGetUserMedia(
        web_request);
  }

  int request_id = g_next_request_id++;
  WebRtcLogMessage(base::StringPrintf(
      "UMCI::RequestUserMedia. request_id=%d, audio constraints=%s, "
      "video constraints=%s",
      request_id, web_request.AudioConstraints().ToString().Utf8().c_str(),
      web_request.VideoConstraints().ToString().Utf8().c_str()));

  // The value returned by isProcessingUserGesture() is used by the browser to
  // make decisions about the permissions UI. Its value can be lost while
  // switching threads, so saving its value here.
  bool user_gesture = blink::WebUserGestureIndicator::IsProcessingUserGesture(
      web_request.OwnerDocument().IsNull()
          ? nullptr
          : web_request.OwnerDocument().GetFrame());
  std::unique_ptr<UserMediaRequest> request_info =
      std::make_unique<UserMediaRequest>(request_id, web_request, user_gesture);
  pending_request_infos_.push_back(Request(std::move(request_info)));
  if (!is_processing_request_)
    MaybeProcessNextRequestInfo();
}

void UserMediaClientImpl::ApplyConstraints(
    const blink::WebApplyConstraintsRequest& web_request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(guidou): Implement applyConstraints(). http://crbug.com/338503
  pending_request_infos_.push_back(Request(web_request));
  if (!is_processing_request_)
    MaybeProcessNextRequestInfo();
}

void UserMediaClientImpl::StopTrack(
    const blink::WebMediaStreamTrack& web_track) {
  pending_request_infos_.push_back(Request(web_track));
  if (!is_processing_request_)
    MaybeProcessNextRequestInfo();
}

void UserMediaClientImpl::MaybeProcessNextRequestInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (is_processing_request_ || pending_request_infos_.empty())
    return;

  Request current_request = std::move(pending_request_infos_.front());
  pending_request_infos_.pop_front();
  is_processing_request_ = true;

  // base::Unretained() is safe here because |this| owns
  // |user_media_processor_|.
  if (current_request.IsUserMedia()) {
    user_media_processor_->ProcessRequest(
        current_request.MoveUserMediaRequest(),
        base::BindOnce(&UserMediaClientImpl::CurrentRequestCompleted,
                       base::Unretained(this)));
  } else if (current_request.IsApplyConstraints()) {
    apply_constraints_processor_->ProcessRequest(
        current_request.apply_constraints_request(),
        base::BindOnce(&UserMediaClientImpl::CurrentRequestCompleted,
                       base::Unretained(this)));
  } else {
    DCHECK(current_request.IsStopTrack());
    MediaStreamTrack* track =
        MediaStreamTrack::GetTrack(current_request.web_track_to_stop());
    if (track) {
      track->StopAndNotify(
          base::BindOnce(&UserMediaClientImpl::CurrentRequestCompleted,
                         weak_factory_.GetWeakPtr()));
    } else {
      CurrentRequestCompleted();
    }
  }
}

void UserMediaClientImpl::CurrentRequestCompleted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  is_processing_request_ = false;
  if (!pending_request_infos_.empty()) {
    render_frame()
        ->GetTaskRunner(blink::TaskType::kInternalMedia)
        ->PostTask(
            FROM_HERE,
            base::BindOnce(&UserMediaClientImpl::MaybeProcessNextRequestInfo,
                           weak_factory_.GetWeakPtr()));
  }
}

void UserMediaClientImpl::CancelUserMediaRequest(
    const blink::WebUserMediaRequest& web_request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  {
    // TODO(guidou): Remove this conditional logging. http://crbug.com/764293
    UserMediaRequest* request = user_media_processor_->CurrentRequest();
    if (request && request->web_request == web_request) {
      WebRtcLogMessage(base::StringPrintf(
          "UMCI::CancelUserMediaRequest. request_id=%d", request->request_id));
    }
  }

  bool did_remove_request = false;
  if (user_media_processor_->DeleteWebRequest(web_request)) {
    did_remove_request = true;
  } else {
    for (auto it = pending_request_infos_.begin();
         it != pending_request_infos_.end(); ++it) {
      if (it->IsUserMedia() &&
          it->user_media_request()->web_request == web_request) {
        pending_request_infos_.erase(it);
        did_remove_request = true;
        break;
      }
    }
  }

  if (did_remove_request) {
    // We can't abort the stream generation process.
    // Instead, erase the request. Once the stream is generated we will stop the
    // stream if the request does not exist.
    LogUserMediaRequestWithNoResult(MEDIA_STREAM_REQUEST_EXPLICITLY_CANCELLED);
  }
}

void UserMediaClientImpl::DeleteAllUserMediaRequests() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  user_media_processor_->StopAllProcessing();
  is_processing_request_ = false;
  pending_request_infos_.clear();
}

void UserMediaClientImpl::WillCommitProvisionalLoad() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Cancel all outstanding UserMediaRequests.
  DeleteAllUserMediaRequests();
}

void UserMediaClientImpl::SetMediaDevicesDispatcherForTesting(
    blink::mojom::MediaDevicesDispatcherHostPtr media_devices_dispatcher) {
  media_devices_dispatcher_ = std::move(media_devices_dispatcher);
}

const blink::mojom::MediaDevicesDispatcherHostPtr&
UserMediaClientImpl::GetMediaDevicesDispatcher() {
  if (!media_devices_dispatcher_) {
    render_frame()->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&media_devices_dispatcher_));
  }

  return media_devices_dispatcher_;
}

void UserMediaClientImpl::OnDestruct() {
  delete this;
}

}  // namespace content
