/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/test/loopback_media_transport.h"

#include "absl/memory/memory.h"
#include "rtc_base/time_utils.h"

namespace webrtc {

namespace {

// Wrapper used to hand out unique_ptrs to loopback media transports without
// ownership changes.
class WrapperMediaTransport : public MediaTransportInterface {
 public:
  explicit WrapperMediaTransport(MediaTransportInterface* wrapped)
      : wrapped_(wrapped) {}

  RTCError SendAudioFrame(uint64_t channel_id,
                          MediaTransportEncodedAudioFrame frame) override {
    return wrapped_->SendAudioFrame(channel_id, std::move(frame));
  }

  RTCError SendVideoFrame(
      uint64_t channel_id,
      const MediaTransportEncodedVideoFrame& frame) override {
    return wrapped_->SendVideoFrame(channel_id, frame);
  }

  void SetKeyFrameRequestCallback(
      MediaTransportKeyFrameRequestCallback* callback) override {
    wrapped_->SetKeyFrameRequestCallback(callback);
  }

  RTCError RequestKeyFrame(uint64_t channel_id) override {
    return wrapped_->RequestKeyFrame(channel_id);
  }

  void SetReceiveAudioSink(MediaTransportAudioSinkInterface* sink) override {
    wrapped_->SetReceiveAudioSink(sink);
  }

  void SetReceiveVideoSink(MediaTransportVideoSinkInterface* sink) override {
    wrapped_->SetReceiveVideoSink(sink);
  }

  void AddTargetTransferRateObserver(
      TargetTransferRateObserver* observer) override {
    wrapped_->AddTargetTransferRateObserver(observer);
  }

  void RemoveTargetTransferRateObserver(
      TargetTransferRateObserver* observer) override {
    wrapped_->RemoveTargetTransferRateObserver(observer);
  }

  void SetMediaTransportStateCallback(
      MediaTransportStateCallback* callback) override {
    wrapped_->SetMediaTransportStateCallback(callback);
  }

  RTCError SendData(int channel_id,
                    const SendDataParams& params,
                    const rtc::CopyOnWriteBuffer& buffer) override {
    return wrapped_->SendData(channel_id, params, buffer);
  }

  RTCError CloseChannel(int channel_id) override {
    return wrapped_->CloseChannel(channel_id);
  }

  void SetDataSink(DataChannelSink* sink) override {
    wrapped_->SetDataSink(sink);
  }

  void SetAllocatedBitrateLimits(
      const MediaTransportAllocatedBitrateLimits& limits) override {}

 private:
  MediaTransportInterface* wrapped_;
};

}  // namespace

WrapperMediaTransportFactory::WrapperMediaTransportFactory(
    MediaTransportInterface* wrapped)
    : wrapped_(wrapped) {}

RTCErrorOr<std::unique_ptr<MediaTransportInterface>>
WrapperMediaTransportFactory::CreateMediaTransport(
    rtc::PacketTransportInternal* packet_transport,
    rtc::Thread* network_thread,
    const MediaTransportSettings& settings) {
  return {absl::make_unique<WrapperMediaTransport>(wrapped_)};
}

MediaTransportPair::LoopbackMediaTransport::LoopbackMediaTransport(
    rtc::Thread* thread,
    LoopbackMediaTransport* other)
    : thread_(thread), other_(other) {}

MediaTransportPair::LoopbackMediaTransport::~LoopbackMediaTransport() {
  rtc::CritScope lock(&sink_lock_);
  RTC_CHECK(audio_sink_ == nullptr);
  RTC_CHECK(video_sink_ == nullptr);
  RTC_CHECK(data_sink_ == nullptr);
  RTC_CHECK(target_transfer_rate_observers_.empty());
  RTC_CHECK(rtt_observers_.empty());
}

RTCError MediaTransportPair::LoopbackMediaTransport::SendAudioFrame(
    uint64_t channel_id,
    MediaTransportEncodedAudioFrame frame) {
  {
    rtc::CritScope lock(&stats_lock_);
    ++stats_.sent_audio_frames;
  }
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this, channel_id, frame] {
    other_->OnData(channel_id, frame);
  });
  return RTCError::OK();
}

RTCError MediaTransportPair::LoopbackMediaTransport::SendVideoFrame(
    uint64_t channel_id,
    const MediaTransportEncodedVideoFrame& frame) {
  {
    rtc::CritScope lock(&stats_lock_);
    ++stats_.sent_video_frames;
  }
  // Ensure that we own the referenced data.
  MediaTransportEncodedVideoFrame frame_copy = frame;
  frame_copy.Retain();
  invoker_.AsyncInvoke<void>(
      RTC_FROM_HERE, thread_, [this, channel_id, frame_copy]() mutable {
        other_->OnData(channel_id, std::move(frame_copy));
      });
  return RTCError::OK();
}

void MediaTransportPair::LoopbackMediaTransport::SetKeyFrameRequestCallback(
    MediaTransportKeyFrameRequestCallback* callback) {
  rtc::CritScope lock(&sink_lock_);
  if (callback) {
    RTC_CHECK(key_frame_callback_ == nullptr);
  }
  key_frame_callback_ = callback;
}

RTCError MediaTransportPair::LoopbackMediaTransport::RequestKeyFrame(
    uint64_t channel_id) {
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this, channel_id] {
    other_->OnKeyFrameRequested(channel_id);
  });
  return RTCError::OK();
}

void MediaTransportPair::LoopbackMediaTransport::SetReceiveAudioSink(
    MediaTransportAudioSinkInterface* sink) {
  rtc::CritScope lock(&sink_lock_);
  if (sink) {
    RTC_CHECK(audio_sink_ == nullptr);
  }
  audio_sink_ = sink;
}

void MediaTransportPair::LoopbackMediaTransport::SetReceiveVideoSink(
    MediaTransportVideoSinkInterface* sink) {
  rtc::CritScope lock(&sink_lock_);
  if (sink) {
    RTC_CHECK(video_sink_ == nullptr);
  }
  video_sink_ = sink;
}

void MediaTransportPair::LoopbackMediaTransport::AddTargetTransferRateObserver(
    TargetTransferRateObserver* observer) {
  RTC_CHECK(observer);
  {
    rtc::CritScope cs(&sink_lock_);
    RTC_CHECK(std::find(target_transfer_rate_observers_.begin(),
                        target_transfer_rate_observers_.end(),
                        observer) == target_transfer_rate_observers_.end());
    target_transfer_rate_observers_.push_back(observer);
  }
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this] {
    RTC_DCHECK_RUN_ON(thread_);
    const DataRate kBitrate = DataRate::kbps(300);
    const Timestamp now = Timestamp::us(rtc::TimeMicros());

    TargetTransferRate transfer_rate;
    transfer_rate.at_time = now;
    transfer_rate.target_rate = kBitrate;
    transfer_rate.network_estimate.at_time = now;
    transfer_rate.network_estimate.round_trip_time = TimeDelta::ms(20);
    transfer_rate.network_estimate.bwe_period = TimeDelta::seconds(3);
    transfer_rate.network_estimate.bandwidth = kBitrate;

    rtc::CritScope cs(&sink_lock_);

    for (auto* o : target_transfer_rate_observers_) {
      o->OnTargetTransferRate(transfer_rate);
    }
  });
}

void MediaTransportPair::LoopbackMediaTransport::
    RemoveTargetTransferRateObserver(TargetTransferRateObserver* observer) {
  rtc::CritScope cs(&sink_lock_);
  auto it = std::find(target_transfer_rate_observers_.begin(),
                      target_transfer_rate_observers_.end(), observer);
  if (it == target_transfer_rate_observers_.end()) {
    RTC_LOG(LS_WARNING)
        << "Attempt to remove an unknown TargetTransferRate observer";
    return;
  }
  target_transfer_rate_observers_.erase(it);
}

void MediaTransportPair::LoopbackMediaTransport::AddRttObserver(
    MediaTransportRttObserver* observer) {
  RTC_CHECK(observer);
  {
    rtc::CritScope cs(&sink_lock_);
    RTC_CHECK(std::find(rtt_observers_.begin(), rtt_observers_.end(),
                        observer) == rtt_observers_.end());
    rtt_observers_.push_back(observer);
  }
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this] {
    RTC_DCHECK_RUN_ON(thread_);

    rtc::CritScope cs(&sink_lock_);
    for (auto* o : rtt_observers_) {
      o->OnRttUpdated(20);
    }
  });
}

void MediaTransportPair::LoopbackMediaTransport::RemoveRttObserver(
    MediaTransportRttObserver* observer) {
  rtc::CritScope cs(&sink_lock_);
  auto it = std::find(rtt_observers_.begin(), rtt_observers_.end(), observer);
  if (it == rtt_observers_.end()) {
    RTC_LOG(LS_WARNING) << "Attempt to remove an unknown RTT observer";
    return;
  }
  rtt_observers_.erase(it);
}

void MediaTransportPair::LoopbackMediaTransport::SetMediaTransportStateCallback(
    MediaTransportStateCallback* callback) {
  rtc::CritScope lock(&sink_lock_);
  state_callback_ = callback;
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this] {
    RTC_DCHECK_RUN_ON(thread_);
    OnStateChanged();
  });
}

RTCError MediaTransportPair::LoopbackMediaTransport::SendData(
    int channel_id,
    const SendDataParams& params,
    const rtc::CopyOnWriteBuffer& buffer) {
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_,
                             [this, channel_id, params, buffer] {
                               other_->OnData(channel_id, params.type, buffer);
                             });
  return RTCError::OK();
}

RTCError MediaTransportPair::LoopbackMediaTransport::CloseChannel(
    int channel_id) {
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this, channel_id] {
    other_->OnRemoteCloseChannel(channel_id);
    rtc::CritScope lock(&sink_lock_);
    if (data_sink_) {
      data_sink_->OnChannelClosed(channel_id);
    }
  });
  return RTCError::OK();
}

void MediaTransportPair::LoopbackMediaTransport::SetDataSink(
    DataChannelSink* sink) {
  rtc::CritScope lock(&sink_lock_);
  data_sink_ = sink;
}
void MediaTransportPair::LoopbackMediaTransport::SetState(
    MediaTransportState state) {
  invoker_.AsyncInvoke<void>(RTC_FROM_HERE, thread_, [this, state] {
    RTC_DCHECK_RUN_ON(thread_);
    state_ = state;
    OnStateChanged();
  });
}

void MediaTransportPair::LoopbackMediaTransport::FlushAsyncInvokes() {
  invoker_.Flush(thread_);
}

MediaTransportPair::Stats
MediaTransportPair::LoopbackMediaTransport::GetStats() {
  rtc::CritScope lock(&stats_lock_);
  return stats_;
}

void MediaTransportPair::LoopbackMediaTransport::OnData(
    uint64_t channel_id,
    MediaTransportEncodedAudioFrame frame) {
  {
    rtc::CritScope lock(&sink_lock_);
    if (audio_sink_) {
      audio_sink_->OnData(channel_id, frame);
    }
  }
  {
    rtc::CritScope lock(&stats_lock_);
    ++stats_.received_audio_frames;
  }
}

void MediaTransportPair::LoopbackMediaTransport::OnData(
    uint64_t channel_id,
    MediaTransportEncodedVideoFrame frame) {
  {
    rtc::CritScope lock(&sink_lock_);
    if (video_sink_) {
      video_sink_->OnData(channel_id, frame);
    }
  }
  {
    rtc::CritScope lock(&stats_lock_);
    ++stats_.received_video_frames;
  }
}

void MediaTransportPair::LoopbackMediaTransport::OnData(
    int channel_id,
    DataMessageType type,
    const rtc::CopyOnWriteBuffer& buffer) {
  rtc::CritScope lock(&sink_lock_);
  if (data_sink_) {
    data_sink_->OnDataReceived(channel_id, type, buffer);
  }
}

void MediaTransportPair::LoopbackMediaTransport::OnKeyFrameRequested(
    int channel_id) {
  rtc::CritScope lock(&sink_lock_);
  if (key_frame_callback_) {
    key_frame_callback_->OnKeyFrameRequested(channel_id);
  }
}

void MediaTransportPair::LoopbackMediaTransport::OnRemoteCloseChannel(
    int channel_id) {
  rtc::CritScope lock(&sink_lock_);
  if (data_sink_) {
    data_sink_->OnChannelClosing(channel_id);
    data_sink_->OnChannelClosed(channel_id);
  }
}

void MediaTransportPair::LoopbackMediaTransport::OnStateChanged() {
  rtc::CritScope lock(&sink_lock_);
  if (state_callback_) {
    state_callback_->OnStateChanged(state_);
  }
}

void MediaTransportPair::LoopbackMediaTransport::SetAllocatedBitrateLimits(
    const MediaTransportAllocatedBitrateLimits& limits) {}

}  // namespace webrtc
