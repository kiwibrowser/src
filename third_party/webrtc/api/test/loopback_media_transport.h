/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_TEST_LOOPBACK_MEDIA_TRANSPORT_H_
#define API_TEST_LOOPBACK_MEDIA_TRANSPORT_H_

#include <memory>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "api/media_transport_interface.h"
#include "rtc_base/async_invoker.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/thread.h"
#include "rtc_base/thread_checker.h"

namespace webrtc {

// Wrapper used to hand out unique_ptrs to loopback media transports without
// ownership changes to the underlying transport.
class WrapperMediaTransportFactory : public MediaTransportFactory {
 public:
  explicit WrapperMediaTransportFactory(MediaTransportInterface* wrapped);

  RTCErrorOr<std::unique_ptr<MediaTransportInterface>> CreateMediaTransport(
      rtc::PacketTransportInternal* packet_transport,
      rtc::Thread* network_thread,
      const MediaTransportSettings& settings) override;

 private:
  MediaTransportInterface* wrapped_;
};

// Contains two MediaTransportsInterfaces that are connected to each other.
// Currently supports audio only.
class MediaTransportPair {
 public:
  struct Stats {
    int sent_audio_frames = 0;
    int received_audio_frames = 0;
    int sent_video_frames = 0;
    int received_video_frames = 0;
  };

  explicit MediaTransportPair(rtc::Thread* thread)
      : first_(thread, &second_), second_(thread, &first_) {}

  // Ownership stays with MediaTransportPair
  MediaTransportInterface* first() { return &first_; }
  MediaTransportInterface* second() { return &second_; }

  std::unique_ptr<MediaTransportFactory> first_factory() {
    return absl::make_unique<WrapperMediaTransportFactory>(&first_);
  }

  std::unique_ptr<MediaTransportFactory> second_factory() {
    return absl::make_unique<WrapperMediaTransportFactory>(&second_);
  }

  void SetState(MediaTransportState state) {
    first_.SetState(state);
    second_.SetState(state);
  }

  void FlushAsyncInvokes() {
    first_.FlushAsyncInvokes();
    second_.FlushAsyncInvokes();
  }

  Stats FirstStats() { return first_.GetStats(); }
  Stats SecondStats() { return second_.GetStats(); }

 private:
  class LoopbackMediaTransport : public MediaTransportInterface {
   public:
    LoopbackMediaTransport(rtc::Thread* thread, LoopbackMediaTransport* other);

    ~LoopbackMediaTransport() override;

    RTCError SendAudioFrame(uint64_t channel_id,
                            MediaTransportEncodedAudioFrame frame) override;

    RTCError SendVideoFrame(
        uint64_t channel_id,
        const MediaTransportEncodedVideoFrame& frame) override;

    void SetKeyFrameRequestCallback(
        MediaTransportKeyFrameRequestCallback* callback) override;

    RTCError RequestKeyFrame(uint64_t channel_id) override;

    void SetReceiveAudioSink(MediaTransportAudioSinkInterface* sink) override;

    void SetReceiveVideoSink(MediaTransportVideoSinkInterface* sink) override;

    void AddTargetTransferRateObserver(
        TargetTransferRateObserver* observer) override;

    void RemoveTargetTransferRateObserver(
        TargetTransferRateObserver* observer) override;

    void AddRttObserver(MediaTransportRttObserver* observer) override;
    void RemoveRttObserver(MediaTransportRttObserver* observer) override;

    void SetMediaTransportStateCallback(
        MediaTransportStateCallback* callback) override;

    RTCError SendData(int channel_id,
                      const SendDataParams& params,
                      const rtc::CopyOnWriteBuffer& buffer) override;

    RTCError CloseChannel(int channel_id) override;

    void SetDataSink(DataChannelSink* sink) override;

    void SetState(MediaTransportState state);

    void FlushAsyncInvokes();

    Stats GetStats();

    void SetAllocatedBitrateLimits(
        const MediaTransportAllocatedBitrateLimits& limits) override;

   private:
    void OnData(uint64_t channel_id, MediaTransportEncodedAudioFrame frame);

    void OnData(uint64_t channel_id, MediaTransportEncodedVideoFrame frame);

    void OnData(int channel_id,
                DataMessageType type,
                const rtc::CopyOnWriteBuffer& buffer);

    void OnKeyFrameRequested(int channel_id);

    void OnRemoteCloseChannel(int channel_id);

    void OnStateChanged() RTC_RUN_ON(thread_);

    rtc::Thread* const thread_;
    rtc::CriticalSection sink_lock_;
    rtc::CriticalSection stats_lock_;

    MediaTransportAudioSinkInterface* audio_sink_ RTC_GUARDED_BY(sink_lock_) =
        nullptr;
    MediaTransportVideoSinkInterface* video_sink_ RTC_GUARDED_BY(sink_lock_) =
        nullptr;
    DataChannelSink* data_sink_ RTC_GUARDED_BY(sink_lock_) = nullptr;

    MediaTransportKeyFrameRequestCallback* key_frame_callback_
        RTC_GUARDED_BY(sink_lock_) = nullptr;

    MediaTransportStateCallback* state_callback_ RTC_GUARDED_BY(sink_lock_) =
        nullptr;

    std::vector<TargetTransferRateObserver*> target_transfer_rate_observers_
        RTC_GUARDED_BY(sink_lock_);
    std::vector<MediaTransportRttObserver*> rtt_observers_
        RTC_GUARDED_BY(sink_lock_);

    MediaTransportState state_ RTC_GUARDED_BY(thread_) =
        MediaTransportState::kPending;

    LoopbackMediaTransport* const other_;

    Stats stats_ RTC_GUARDED_BY(stats_lock_);

    rtc::AsyncInvoker invoker_;
  };

  LoopbackMediaTransport first_;
  LoopbackMediaTransport second_;
};

}  // namespace webrtc

#endif  // API_TEST_LOOPBACK_MEDIA_TRANSPORT_H_
