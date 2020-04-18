// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/media_stream_track_metrics.h"

#include <inttypes.h>
#include <set>
#include <string>

#include "base/md5.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_thread_impl.h"
#include "content/public/common/service_names.mojom.h"
#include "content/renderer/render_thread_impl.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

using webrtc::AudioTrackVector;
using webrtc::MediaStreamInterface;
using webrtc::MediaStreamTrackInterface;
using webrtc::PeerConnectionInterface;
using webrtc::VideoTrackVector;

namespace content {
namespace {
typedef std::set<std::string> IdSet;

template <class T>
IdSet GetTrackIds(const std::vector<rtc::scoped_refptr<T>>& tracks) {
  IdSet track_ids;
  for (const auto& track : tracks)
    track_ids.insert(track->id());
  return track_ids;
}

// TODO(tommi): Consolidate this and TrackObserver since these implementations
// are fundamentally achieving the same thing (aside from specific logic inside
// the OnChanged callbacks).
class MediaStreamObserver
    : public base::RefCountedThreadSafe<MediaStreamObserver>,
      public webrtc::ObserverInterface {
 public:
  typedef base::Callback<
      void(const IdSet& audio_track_ids, const IdSet& video_track_ids)>
          OnChangedCallback;

  MediaStreamObserver(
      const OnChangedCallback& callback,
      const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
      webrtc::MediaStreamInterface* stream)
      : main_thread_(main_thread), stream_(stream), callback_(callback) {
    signaling_thread_.DetachFromThread();
    stream_->RegisterObserver(this);
  }

  const scoped_refptr<webrtc::MediaStreamInterface>& stream() const {
    DCHECK(main_thread_->BelongsToCurrentThread());
    return stream_;
  }

  void Unregister() {
    DCHECK(main_thread_->BelongsToCurrentThread());
    callback_.Reset();
    stream_->UnregisterObserver(this);
    stream_ = nullptr;
  }

 private:
  friend class base::RefCountedThreadSafe<MediaStreamObserver>;
  ~MediaStreamObserver() override {
    DCHECK(!stream_.get()) << "must have been unregistered before deleting";
  }

  // webrtc::ObserverInterface implementation.
  void OnChanged() override {
    DCHECK(signaling_thread_.CalledOnValidThread());
    main_thread_->PostTask(
        FROM_HERE, base::BindOnce(&MediaStreamObserver::OnChangedOnMainThread,
                                  this, GetTrackIds(stream_->GetAudioTracks()),
                                  GetTrackIds(stream_->GetVideoTracks())));
  }

  void OnChangedOnMainThread(const IdSet& audio_track_ids,
                             const IdSet& video_track_ids) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    if (!callback_.is_null())
      callback_.Run(audio_track_ids, video_track_ids);
  }

  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<webrtc::MediaStreamInterface> stream_;
  OnChangedCallback callback_;  // Only touched on the main thread.
  base::ThreadChecker signaling_thread_;
};

}  // namespace

class MediaStreamTrackMetricsObserver {
 public:
  MediaStreamTrackMetricsObserver(
      MediaStreamTrackMetrics::StreamType stream_type,
      MediaStreamInterface* stream,
      MediaStreamTrackMetrics* owner);
  ~MediaStreamTrackMetricsObserver();

  // Sends begin/end messages for all tracks currently tracked.
  void SendLifetimeMessages(MediaStreamTrackMetrics::LifetimeEvent event);

  MediaStreamInterface* stream() {
    DCHECK(thread_checker_.CalledOnValidThread());
    return observer_->stream().get();
  }

  MediaStreamTrackMetrics::StreamType stream_type() {
    DCHECK(thread_checker_.CalledOnValidThread());
    return stream_type_;
  }

 private:
  void OnChanged(const IdSet& audio_track_ids, const IdSet& video_track_ids);

  void ReportAddedAndRemovedTracks(
      const IdSet& new_ids,
      const IdSet& old_ids,
      MediaStreamTrackMetrics::TrackType track_type);

  // Sends a lifetime message for the given tracks. OK to call with an
  // empty |ids|, in which case the method has no side effects.
  void ReportTracks(const IdSet& ids,
                    MediaStreamTrackMetrics::TrackType track_type,
                    MediaStreamTrackMetrics::LifetimeEvent event);

  // False until start/end of lifetime messages have been sent.
  bool has_reported_start_;
  bool has_reported_end_;

  // IDs of audio and video tracks in the stream being observed.
  IdSet audio_track_ids_;
  IdSet video_track_ids_;

  MediaStreamTrackMetrics::StreamType stream_type_;
  scoped_refptr<MediaStreamObserver> observer_;

  // Non-owning.
  MediaStreamTrackMetrics* owner_;
  base::ThreadChecker thread_checker_;
};

namespace {

// Used with std::find_if.
struct ObserverFinder {
  ObserverFinder(MediaStreamTrackMetrics::StreamType stream_type,
                 MediaStreamInterface* stream)
      : stream_type(stream_type), stream_(stream) {}
  bool operator()(
      const std::unique_ptr<MediaStreamTrackMetricsObserver>& observer) {
    return stream_ == observer->stream() &&
           stream_type == observer->stream_type();
  }
  MediaStreamTrackMetrics::StreamType stream_type;
  MediaStreamInterface* stream_;
};

}  // namespace

MediaStreamTrackMetricsObserver::MediaStreamTrackMetricsObserver(
    MediaStreamTrackMetrics::StreamType stream_type,
    MediaStreamInterface* stream,
    MediaStreamTrackMetrics* owner)
    : has_reported_start_(false),
      has_reported_end_(false),
      audio_track_ids_(GetTrackIds(stream->GetAudioTracks())),
      video_track_ids_(GetTrackIds(stream->GetVideoTracks())),
      stream_type_(stream_type),
      observer_(new MediaStreamObserver(
            base::Bind(&MediaStreamTrackMetricsObserver::OnChanged,
                       base::Unretained(this)),
            base::ThreadTaskRunnerHandle::Get(),
            stream)),
      owner_(owner) {
}

MediaStreamTrackMetricsObserver::~MediaStreamTrackMetricsObserver() {
  DCHECK(thread_checker_.CalledOnValidThread());
  observer_->Unregister();
  SendLifetimeMessages(MediaStreamTrackMetrics::DISCONNECTED);
}

void MediaStreamTrackMetricsObserver::SendLifetimeMessages(
    MediaStreamTrackMetrics::LifetimeEvent event) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (event == MediaStreamTrackMetrics::CONNECTED) {
    // Both ICE CONNECTED and COMPLETED can trigger the first
    // start-of-life event, so we only report the first.
    if (has_reported_start_)
      return;
    DCHECK(!has_reported_start_ && !has_reported_end_);
    has_reported_start_ = true;
  } else {
    DCHECK(event == MediaStreamTrackMetrics::DISCONNECTED);

    // We only report the first end-of-life event, since there are
    // several cases where end-of-life can be reached. We also don't
    // report end unless we've reported start.
    if (has_reported_end_ || !has_reported_start_)
      return;
    has_reported_end_ = true;
  }

  ReportTracks(audio_track_ids_, MediaStreamTrackMetrics::AUDIO_TRACK, event);
  ReportTracks(video_track_ids_, MediaStreamTrackMetrics::VIDEO_TRACK, event);

  if (event == MediaStreamTrackMetrics::DISCONNECTED) {
    // After disconnection, we can get reconnected, so we need to
    // forget that we've sent lifetime events, while retaining all
    // other state.
    DCHECK(has_reported_start_ && has_reported_end_);
    has_reported_start_ = false;
    has_reported_end_ = false;
  }
}

void MediaStreamTrackMetricsObserver::OnChanged(
    const IdSet& audio_track_ids, const IdSet& video_track_ids) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // We only report changes after our initial report, and never after
  // our last report.
  if (has_reported_start_ && !has_reported_end_) {
    ReportAddedAndRemovedTracks(audio_track_ids,
                                audio_track_ids_,
                                MediaStreamTrackMetrics::AUDIO_TRACK);
    ReportAddedAndRemovedTracks(video_track_ids,
                                video_track_ids_,
                                MediaStreamTrackMetrics::VIDEO_TRACK);
  }

  // We always update our sets of tracks.
  audio_track_ids_ = audio_track_ids;
  video_track_ids_ = video_track_ids;
}

void MediaStreamTrackMetricsObserver::ReportAddedAndRemovedTracks(
    const IdSet& new_ids,
    const IdSet& old_ids,
    MediaStreamTrackMetrics::TrackType track_type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(has_reported_start_ && !has_reported_end_);

  IdSet added_tracks = base::STLSetDifference<IdSet>(new_ids, old_ids);
  IdSet removed_tracks = base::STLSetDifference<IdSet>(old_ids, new_ids);

  ReportTracks(added_tracks, track_type, MediaStreamTrackMetrics::CONNECTED);
  ReportTracks(
      removed_tracks, track_type, MediaStreamTrackMetrics::DISCONNECTED);
}

void MediaStreamTrackMetricsObserver::ReportTracks(
    const IdSet& ids,
    MediaStreamTrackMetrics::TrackType track_type,
    MediaStreamTrackMetrics::LifetimeEvent event) {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (IdSet::const_iterator it = ids.begin(); it != ids.end(); ++it) {
    owner_->SendLifetimeMessage(*it, track_type, event, stream_type_);
  }
}

MediaStreamTrackMetrics::MediaStreamTrackMetrics()
    : ice_state_(webrtc::PeerConnectionInterface::kIceConnectionNew) {}

MediaStreamTrackMetrics::~MediaStreamTrackMetrics() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& observer : observers_) {
    observer->SendLifetimeMessages(DISCONNECTED);
  }
}

void MediaStreamTrackMetrics::AddStream(StreamType type,
                                        MediaStreamInterface* stream) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.push_back(
      std::make_unique<MediaStreamTrackMetricsObserver>(type, stream, this));
  SendLifeTimeMessageDependingOnIceState(observers_.back().get());
}

void MediaStreamTrackMetrics::RemoveStream(StreamType type,
                                           MediaStreamInterface* stream) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = std::find_if(observers_.begin(), observers_.end(),
                         ObserverFinder(type, stream));
  if (it == observers_.end()) {
    // Since external apps could call removeStream with a stream they
    // never added, this can happen without it being an error.
    return;
  }

  observers_.erase(it);
}

void MediaStreamTrackMetrics::IceConnectionChange(
    PeerConnectionInterface::IceConnectionState new_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ice_state_ = new_state;
  for (const auto& observer : observers_) {
    SendLifeTimeMessageDependingOnIceState(observer.get());
  }
}
void MediaStreamTrackMetrics::SendLifeTimeMessageDependingOnIceState(
    MediaStreamTrackMetricsObserver* observer) {
  // There is a state transition diagram for these states at
  // http://dev.w3.org/2011/webrtc/editor/webrtc.html#idl-def-RTCIceConnectionState
  switch (ice_state_) {
    case PeerConnectionInterface::kIceConnectionConnected:
    case PeerConnectionInterface::kIceConnectionCompleted:
      observer->SendLifetimeMessages(CONNECTED);
      break;

    case PeerConnectionInterface::kIceConnectionFailed:
      // We don't really need to handle FAILED (it is only supposed
      // to be preceded by CHECKING so we wouldn't yet have sent a
      // lifetime message) but we might as well use belt and
      // suspenders and handle it the same as the other "end call"
      // states. It will be ignored anyway if the call is not
      // already connected.
    case PeerConnectionInterface::kIceConnectionNew:
      // It's a bit weird to count NEW as an end-lifetime event, but
      // it's possible to transition directly from a connected state
      // (CONNECTED or COMPLETED) to NEW, which can then be followed
      // by a new connection. The observer will ignore the end
      // lifetime event if it was not preceded by a begin-lifetime
      // event.
    case PeerConnectionInterface::kIceConnectionDisconnected:
    case PeerConnectionInterface::kIceConnectionClosed:
      observer->SendLifetimeMessages(DISCONNECTED);
      break;

    default:
      // We ignore the remaining state (CHECKING) as it is never
      // involved in a transition from connected to disconnected or
      // vice versa.
      break;
  }
}

void MediaStreamTrackMetrics::SendLifetimeMessage(const std::string& track_id,
                                                  TrackType track_type,
                                                  LifetimeEvent event,
                                                  StreamType stream_type) {
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  // |render_thread| can be NULL in certain cases when running as part
  // |of a unit test.
  if (render_thread) {
    if (event == CONNECTED) {
      GetMediaStreamTrackMetricsHost()->AddTrack(
          MakeUniqueId(track_id, stream_type), track_type == AUDIO_TRACK,
          stream_type == RECEIVED_STREAM);
    } else {
      DCHECK_EQ(DISCONNECTED, event);
      GetMediaStreamTrackMetricsHost()->RemoveTrack(
          MakeUniqueId(track_id, stream_type));
    }
  }
}

uint64_t MediaStreamTrackMetrics::MakeUniqueIdImpl(uint64_t pc_id,
                                                   const std::string& track_id,
                                                   StreamType stream_type) {
  // We use a hash over the |track| pointer and the PeerConnection ID,
  // plus a boolean flag indicating whether the track is remote (since
  // you might conceivably have a remote track added back as a sent
  // track) as the unique ID.
  //
  // We don't need a cryptographically secure hash (which MD5 should
  // no longer be considered), just one with virtually zero chance of
  // collisions when faced with non-malicious data.
  std::string unique_id_string =
      base::StringPrintf("%" PRIu64 " %s %d",
                         pc_id,
                         track_id.c_str(),
                         stream_type == RECEIVED_STREAM ? 1 : 0);

  base::MD5Context ctx;
  base::MD5Init(&ctx);
  base::MD5Update(&ctx, unique_id_string);
  base::MD5Digest digest;
  base::MD5Final(&digest, &ctx);

  static_assert(sizeof(digest.a) > sizeof(uint64_t), "need a bigger digest");
  return *reinterpret_cast<uint64_t*>(digest.a);
}

uint64_t MediaStreamTrackMetrics::MakeUniqueId(const std::string& track_id,
                                               StreamType stream_type) {
  return MakeUniqueIdImpl(
      reinterpret_cast<uint64_t>(reinterpret_cast<void*>(this)), track_id,
      stream_type);
}

mojom::MediaStreamTrackMetricsHostPtr&
MediaStreamTrackMetrics::GetMediaStreamTrackMetricsHost() {
  if (!track_metrics_host_) {
    ChildThreadImpl::current()->GetConnector()->BindInterface(
        mojom::kBrowserServiceName, &track_metrics_host_);
  }
  return track_metrics_host_;
}

}  // namespace content
