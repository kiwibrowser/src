// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/mock_peer_connection_impl.h"

#include <stddef.h>

#include <vector>

#include "base/logging.h"
#include "content/renderer/media/webrtc/mock_data_channel_impl.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "third_party/webrtc/api/rtpreceiverinterface.h"
#include "third_party/webrtc/rtc_base/refcountedobject.h"

using testing::_;
using webrtc::AudioTrackInterface;
using webrtc::CreateSessionDescriptionObserver;
using webrtc::DtmfSenderInterface;
using webrtc::DtmfSenderObserverInterface;
using webrtc::IceCandidateInterface;
using webrtc::MediaStreamInterface;
using webrtc::PeerConnectionInterface;
using webrtc::SessionDescriptionInterface;
using webrtc::SetSessionDescriptionObserver;

namespace content {

class MockStreamCollection : public webrtc::StreamCollectionInterface {
 public:
  size_t count() override { return streams_.size(); }
  MediaStreamInterface* at(size_t index) override { return streams_[index]; }
  MediaStreamInterface* find(const std::string& id) override {
    for (size_t i = 0; i < streams_.size(); ++i) {
      if (streams_[i]->id() == id)
        return streams_[i];
    }
    return nullptr;
  }
  webrtc::MediaStreamTrackInterface* FindAudioTrack(
      const std::string& id) override {
    for (size_t i = 0; i < streams_.size(); ++i) {
      webrtc::MediaStreamTrackInterface* track =
          streams_.at(i)->FindAudioTrack(id);
      if (track)
        return track;
    }
    return nullptr;
  }
  webrtc::MediaStreamTrackInterface* FindVideoTrack(
      const std::string& id) override {
    for (size_t i = 0; i < streams_.size(); ++i) {
      webrtc::MediaStreamTrackInterface* track =
          streams_.at(i)->FindVideoTrack(id);
      if (track)
        return track;
    }
    return nullptr;
  }
  std::vector<webrtc::MediaStreamInterface*> FindStreamsOfTrack(
      webrtc::MediaStreamTrackInterface* track) {
    std::vector<webrtc::MediaStreamInterface*> streams_of_track;
    if (!track)
      return streams_of_track;
    for (size_t i = 0; i < streams_.size(); ++i) {
      if (streams_.at(i)->FindAudioTrack(track->id()) ||
          streams_.at(i)->FindVideoTrack(track->id())) {
        streams_of_track.push_back(streams_.at(i));
      }
    }
    return streams_of_track;
  }
  void AddStream(MediaStreamInterface* stream) {
    streams_.push_back(stream);
  }
  void RemoveStream(MediaStreamInterface* stream) {
    StreamVector::iterator it = streams_.begin();
    for (; it != streams_.end(); ++it) {
      if (it->get() == stream) {
        streams_.erase(it);
        break;
      }
    }
  }

 protected:
  ~MockStreamCollection() override {}

 private:
  typedef std::vector<rtc::scoped_refptr<MediaStreamInterface> >
      StreamVector;
  StreamVector streams_;
};

class MockDtmfSender : public DtmfSenderInterface {
 public:
  explicit MockDtmfSender(AudioTrackInterface* track)
      : track_(track), observer_(nullptr), duration_(0), inter_tone_gap_(0) {}
  void RegisterObserver(DtmfSenderObserverInterface* observer) override {
    observer_ = observer;
  }
  void UnregisterObserver() override { observer_ = nullptr; }
  bool CanInsertDtmf() override { return true; }
  bool InsertDtmf(const std::string& tones,
                  int duration,
                  int inter_tone_gap) override {
    tones_ = tones;
    duration_ = duration;
    inter_tone_gap_ = inter_tone_gap;
    return true;
  }
  const AudioTrackInterface* track() const override { return track_.get(); }
  std::string tones() const override { return tones_; }
  int duration() const override { return duration_; }
  int inter_tone_gap() const override { return inter_tone_gap_; }

 protected:
  ~MockDtmfSender() override {}

 private:
  rtc::scoped_refptr<AudioTrackInterface> track_;
  DtmfSenderObserverInterface* observer_;
  std::string tones_;
  int duration_;
  int inter_tone_gap_;
};

FakeRtpSender::FakeRtpSender(
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
    : track_(std::move(track)) {}

FakeRtpSender::~FakeRtpSender() {}

bool FakeRtpSender::SetTrack(webrtc::MediaStreamTrackInterface* track) {
  NOTIMPLEMENTED();
  return false;
}

rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> FakeRtpSender::track()
    const {
  return track_;
}

uint32_t FakeRtpSender::ssrc() const {
  NOTIMPLEMENTED();
  return 0;
}

cricket::MediaType FakeRtpSender::media_type() const {
  NOTIMPLEMENTED();
  return cricket::MEDIA_TYPE_AUDIO;
}

std::string FakeRtpSender::id() const {
  NOTIMPLEMENTED();
  return "";
}

std::vector<std::string> FakeRtpSender::stream_ids() const {
  NOTIMPLEMENTED();
  return {};
}

webrtc::RtpParameters FakeRtpSender::GetParameters() const {
  NOTIMPLEMENTED();
  return webrtc::RtpParameters();
}

webrtc::RTCError FakeRtpSender::SetParameters(
    const webrtc::RtpParameters& parameters) {
  NOTIMPLEMENTED();
  return webrtc::RTCError::OK();
}

rtc::scoped_refptr<webrtc::DtmfSenderInterface> FakeRtpSender::GetDtmfSender()
    const {
  NOTIMPLEMENTED();
  return nullptr;
}

FakeRtpReceiver::FakeRtpReceiver(
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
    std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> streams)
    : track_(std::move(track)), streams_(std::move(streams)) {}

FakeRtpReceiver::~FakeRtpReceiver() {}

rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> FakeRtpReceiver::track()
    const {
  return track_;
}

std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>
FakeRtpReceiver::streams() const {
  return streams_;
}

cricket::MediaType FakeRtpReceiver::media_type() const {
  NOTIMPLEMENTED();
  return cricket::MEDIA_TYPE_AUDIO;
}

std::string FakeRtpReceiver::id() const {
  NOTIMPLEMENTED();
  return "";
}

webrtc::RtpParameters FakeRtpReceiver::GetParameters() const {
  NOTIMPLEMENTED();
  return webrtc::RtpParameters();
}

bool FakeRtpReceiver::SetParameters(const webrtc::RtpParameters& parameters) {
  NOTIMPLEMENTED();
  return false;
}

void FakeRtpReceiver::SetObserver(
    webrtc::RtpReceiverObserverInterface* observer) {
  NOTIMPLEMENTED();
}

std::vector<webrtc::RtpSource> FakeRtpReceiver::GetSources() const {
  NOTIMPLEMENTED();
  return std::vector<webrtc::RtpSource>();
}

const char MockPeerConnectionImpl::kDummyOffer[] = "dummy offer";
const char MockPeerConnectionImpl::kDummyAnswer[] = "dummy answer";

MockPeerConnectionImpl::MockPeerConnectionImpl(
    MockPeerConnectionDependencyFactory* factory,
    webrtc::PeerConnectionObserver* observer)
    : dependency_factory_(factory),
      local_streams_(new rtc::RefCountedObject<MockStreamCollection>),
      remote_streams_(new rtc::RefCountedObject<MockStreamCollection>),
      hint_audio_(false),
      hint_video_(false),
      getstats_result_(true),
      sdp_mline_index_(-1),
      observer_(observer) {
  ON_CALL(*this, SetLocalDescription(_, _)).WillByDefault(testing::Invoke(
      this, &MockPeerConnectionImpl::SetLocalDescriptionWorker));
  // TODO(hbos): Remove once no longer mandatory to implement.
  ON_CALL(*this, SetRemoteDescription(_, _)).WillByDefault(testing::Invoke(
      this, &MockPeerConnectionImpl::SetRemoteDescriptionWorker));
  ON_CALL(*this, SetRemoteDescriptionForMock(_, _))
      .WillByDefault(testing::Invoke(
          [this](
              std::unique_ptr<webrtc::SessionDescriptionInterface>* desc,
              rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface>*
                  observer) {
            SetRemoteDescriptionWorker(nullptr, desc->release());
          }));
}

MockPeerConnectionImpl::~MockPeerConnectionImpl() {}

rtc::scoped_refptr<webrtc::StreamCollectionInterface>
MockPeerConnectionImpl::local_streams() {
  return local_streams_;
}

rtc::scoped_refptr<webrtc::StreamCollectionInterface>
MockPeerConnectionImpl::remote_streams() {
  return remote_streams_;
}

rtc::scoped_refptr<webrtc::RtpSenderInterface> MockPeerConnectionImpl::AddTrack(
    webrtc::MediaStreamTrackInterface* track,
    std::vector<webrtc::MediaStreamInterface*> streams) {
  DCHECK(track);
  DCHECK_EQ(1u, streams.size());
  for (const auto& sender : senders_) {
    if (sender->track() == track)
      return nullptr;
  }
  for (auto* stream : streams) {
    if (!local_streams_->find(stream->id())) {
      stream_label_ = stream->id();
      local_streams_->AddStream(stream);
    }
  }
  auto* sender = new rtc::RefCountedObject<FakeRtpSender>(track);
  senders_.push_back(sender);
  return sender;
}

bool MockPeerConnectionImpl::RemoveTrack(webrtc::RtpSenderInterface* sender) {
  auto it = std::find(senders_.begin(), senders_.end(),
                      static_cast<FakeRtpSender*>(sender));
  if (it == senders_.end())
    return false;
  senders_.erase(it);
  auto track = sender->track();
  for (auto* stream : local_streams_->FindStreamsOfTrack(track)) {
    bool stream_has_senders = false;
    for (const auto& track : stream->GetAudioTracks()) {
      for (const auto& sender : senders_) {
        if (sender->track() == track) {
          stream_has_senders = true;
          break;
        }
      }
    }
    for (const auto& track : stream->GetVideoTracks()) {
      for (const auto& sender : senders_) {
        if (sender->track() == track) {
          stream_has_senders = true;
          break;
        }
      }
    }
    if (!stream_has_senders)
      local_streams_->RemoveStream(stream);
  }
  return true;
}

rtc::scoped_refptr<DtmfSenderInterface>
MockPeerConnectionImpl::CreateDtmfSender(AudioTrackInterface* track) {
  if (!track) {
    return nullptr;
  }
  return new rtc::RefCountedObject<MockDtmfSender>(track);
}

std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
MockPeerConnectionImpl::GetSenders() const {
  std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders;
  for (const auto& sender : senders_)
    senders.push_back(sender);
  return senders;
}

std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>>
MockPeerConnectionImpl::GetReceivers() const {
  std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers;
  for (size_t i = 0; i < remote_streams_->count(); ++i) {
    for (const auto& audio_track : remote_streams_->at(i)->GetAudioTracks()) {
      receivers.push_back(
          new rtc::RefCountedObject<FakeRtpReceiver>(audio_track));
    }
    for (const auto& video_track : remote_streams_->at(i)->GetVideoTracks()) {
      receivers.push_back(
          new rtc::RefCountedObject<FakeRtpReceiver>(video_track));
    }
  }
  return receivers;
}

rtc::scoped_refptr<webrtc::DataChannelInterface>
MockPeerConnectionImpl::CreateDataChannel(const std::string& label,
                      const webrtc::DataChannelInit* config) {
  return new rtc::RefCountedObject<MockDataChannel>(label, config);
}

bool MockPeerConnectionImpl::GetStats(
    webrtc::StatsObserver* observer,
    webrtc::MediaStreamTrackInterface* track,
    StatsOutputLevel level) {
  if (!getstats_result_)
    return false;

  DCHECK_EQ(kStatsOutputLevelStandard, level);
  webrtc::StatsReport report1(webrtc::StatsReport::NewTypedId(
      webrtc::StatsReport::kStatsReportTypeSsrc, "1234"));
  webrtc::StatsReport report2(webrtc::StatsReport::NewTypedId(
      webrtc::StatsReport::kStatsReportTypeSession, "nontrack"));
  report1.set_timestamp(42);
  report1.AddString(webrtc::StatsReport::kStatsValueNameFingerprint,
                    "trackvalue");

  webrtc::StatsReports reports;
  reports.push_back(&report1);

  // If selector is given, we pass back one report.
  // If selector is not given, we pass back two.
  if (!track) {
    report2.set_timestamp(44);
    report2.AddString(webrtc::StatsReport::kStatsValueNameFingerprintAlgorithm,
                      "somevalue");
    reports.push_back(&report2);
  }

  // Note that the callback is synchronous, not asynchronous; it will
  // happen before the request call completes.
  observer->OnComplete(reports);

  return true;
}

void MockPeerConnectionImpl::GetStats(
    webrtc::RTCStatsCollectorCallback* callback) {
  DCHECK(callback);
  DCHECK(stats_report_);
  callback->OnStatsDelivered(stats_report_);
}

void MockPeerConnectionImpl::GetStats(
    rtc::scoped_refptr<webrtc::RtpSenderInterface> selector,
    rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback> callback) {
  callback->OnStatsDelivered(stats_report_);
}

void MockPeerConnectionImpl::GetStats(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> selector,
    rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback> callback) {
  callback->OnStatsDelivered(stats_report_);
}

void MockPeerConnectionImpl::SetGetStatsReport(webrtc::RTCStatsReport* report) {
  stats_report_ = report;
}

const webrtc::SessionDescriptionInterface*
MockPeerConnectionImpl::local_description() const {
  return local_desc_.get();
}

const webrtc::SessionDescriptionInterface*
MockPeerConnectionImpl::remote_description() const {
  return remote_desc_.get();
}

void MockPeerConnectionImpl::AddRemoteStream(MediaStreamInterface* stream) {
  remote_streams_->AddStream(stream);
}

void MockPeerConnectionImpl::CreateOffer(
    CreateSessionDescriptionObserver* observer,
    const RTCOfferAnswerOptions& options) {
  DCHECK(observer);
  created_sessiondescription_.reset(
      dependency_factory_->CreateSessionDescription("unknown", kDummyOffer,
                                                    nullptr));
}

void MockPeerConnectionImpl::CreateAnswer(
    CreateSessionDescriptionObserver* observer,
    const RTCOfferAnswerOptions& options) {
  DCHECK(observer);
  created_sessiondescription_.reset(
      dependency_factory_->CreateSessionDescription("unknown", kDummyAnswer,
                                                    nullptr));
}

void MockPeerConnectionImpl::SetLocalDescriptionWorker(
    SetSessionDescriptionObserver* observer,
    SessionDescriptionInterface* desc) {
  desc->ToString(&description_sdp_);
  local_desc_.reset(desc);
}

void MockPeerConnectionImpl::SetRemoteDescriptionWorker(
    SetSessionDescriptionObserver* observer,
    SessionDescriptionInterface* desc) {
  desc->ToString(&description_sdp_);
  remote_desc_.reset(desc);
}

bool MockPeerConnectionImpl::SetConfiguration(
    const RTCConfiguration& configuration,
    webrtc::RTCError* error) {
  if (setconfiguration_error_type_ == webrtc::RTCErrorType::NONE) {
    return true;
  }
  error->set_type(setconfiguration_error_type_);
  return false;
}

bool MockPeerConnectionImpl::AddIceCandidate(
    const IceCandidateInterface* candidate) {
  sdp_mid_ = candidate->sdp_mid();
  sdp_mline_index_ = candidate->sdp_mline_index();
  return candidate->ToString(&ice_sdp_);
}

void MockPeerConnectionImpl::RegisterUMAObserver(
    webrtc::UMAObserver* observer) {
  NOTIMPLEMENTED();
}

webrtc::RTCError MockPeerConnectionImpl::SetBitrate(
    const webrtc::BitrateSettings& bitrate) {
  NOTIMPLEMENTED();
  return webrtc::RTCError::OK();
}

}  // namespace content
