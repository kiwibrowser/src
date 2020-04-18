// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_rtp_receiver.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "content/child/child_process.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/mock_peer_connection_impl.h"
#include "content/renderer/media/webrtc/test/webrtc_stats_report_obtainer.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_rtc_stats.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"
#include "third_party/webrtc/api/stats/rtcstats_objects.h"
#include "third_party/webrtc/api/stats/rtcstatsreport.h"
#include "third_party/webrtc/api/test/mock_rtpreceiver.h"

namespace content {

class RTCRtpReceiverTest : public ::testing::Test {
 public:
  void SetUp() override {
    dependency_factory_.reset(new MockPeerConnectionDependencyFactory());
    main_thread_ = blink::scheduler::GetSingleThreadTaskRunnerForTesting();
    stream_map_ = new WebRtcMediaStreamAdapterMap(
        dependency_factory_.get(), main_thread_,
        new WebRtcMediaStreamTrackAdapterMap(dependency_factory_.get(),
                                             main_thread_));
    peer_connection_ = new rtc::RefCountedObject<MockPeerConnectionImpl>(
        dependency_factory_.get(), nullptr);
  }

  void TearDown() override {
    receiver_.reset();
    // Syncing up with the signaling thread ensures any pending operations on
    // that thread are executed. If they post back to the main thread, such as
    // the sender's destructor traits, this is allowed to execute before the
    // test shuts down the threads.
    SyncWithSignalingThread();
    blink::WebHeap::CollectAllGarbageForTesting();
  }

  // Wait for the signaling thread to perform any queued tasks, executing tasks
  // posted to the current thread in the meantime while waiting.
  void SyncWithSignalingThread() const {
    base::RunLoop run_loop;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE, run_loop.QuitClosure());
    run_loop.Run();
  }

  std::unique_ptr<RTCRtpReceiver> CreateReceiver(
      scoped_refptr<webrtc::MediaStreamTrackInterface> webrtc_track) {
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_adapter;
    base::RunLoop run_loop;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&RTCRtpReceiverTest::CreateReceiverOnSignalingThread,
                       base::Unretained(this), std::move(webrtc_track),
                       base::Unretained(&track_adapter),
                       base::Unretained(&run_loop)));
    run_loop.Run();
    DCHECK(mock_webrtc_receiver_);
    DCHECK(track_adapter);
    return std::make_unique<RTCRtpReceiver>(
        peer_connection_.get(), main_thread_,
        dependency_factory_->GetWebRtcSignalingThread(),
        mock_webrtc_receiver_.get(), std::move(track_adapter),
        std::vector<
            std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>());
  }

  scoped_refptr<WebRTCStatsReportObtainer> GetStats() {
    scoped_refptr<WebRTCStatsReportObtainer> obtainer =
        new WebRTCStatsReportObtainer();
    receiver_->GetStats(obtainer->GetStatsCallbackWrapper());
    return obtainer;
  }

 protected:
  void CreateReceiverOnSignalingThread(
      scoped_refptr<webrtc::MediaStreamTrackInterface> webrtc_track,
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>*
          track_adapter,
      base::RunLoop* run_loop) {
    mock_webrtc_receiver_ =
        new rtc::RefCountedObject<webrtc::MockRtpReceiver>();
    *track_adapter =
        stream_map_->track_adapter_map()->GetOrCreateRemoteTrackAdapter(
            webrtc_track);
    run_loop->Quit();
  }

  // Message loop and child processes is needed for task queues and threading to
  // work, as is necessary to create tracks and adapters.
  base::MessageLoop message_loop_;
  ChildProcess child_process_;

  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<WebRtcMediaStreamAdapterMap> stream_map_;
  rtc::scoped_refptr<MockPeerConnectionImpl> peer_connection_;
  rtc::scoped_refptr<webrtc::MockRtpReceiver> mock_webrtc_receiver_;
  std::unique_ptr<RTCRtpReceiver> receiver_;
};

TEST_F(RTCRtpReceiverTest, CreateReceiver) {
  scoped_refptr<MockWebRtcAudioTrack> webrtc_track =
      MockWebRtcAudioTrack::Create("webrtc_track");
  receiver_ = CreateReceiver(webrtc_track);
  EXPECT_FALSE(receiver_->Track().IsNull());
  EXPECT_EQ(receiver_->Track().Id().Utf8(), webrtc_track->id());
  EXPECT_EQ(&receiver_->webrtc_track(), webrtc_track);
}

TEST_F(RTCRtpReceiverTest, ShallowCopy) {
  scoped_refptr<MockWebRtcAudioTrack> webrtc_track =
      MockWebRtcAudioTrack::Create("webrtc_track");
  receiver_ = CreateReceiver(webrtc_track);
  auto copy = receiver_->ShallowCopy();
  EXPECT_EQ(&receiver_->webrtc_track(), webrtc_track);
  auto* webrtc_receiver = receiver_->webrtc_receiver();
  auto web_track_unique_id = receiver_->Track().UniqueId();
  // Copy is identical to original.
  EXPECT_EQ(copy->webrtc_receiver(), webrtc_receiver);
  EXPECT_EQ(&copy->webrtc_track(), webrtc_track);
  EXPECT_EQ(copy->Track().UniqueId(), web_track_unique_id);
  // Copy keeps the internal state alive.
  receiver_.reset();
  EXPECT_EQ(copy->webrtc_receiver(), webrtc_receiver);
  EXPECT_EQ(&copy->webrtc_track(), webrtc_track);
  EXPECT_EQ(copy->Track().UniqueId(), web_track_unique_id);
}

TEST_F(RTCRtpReceiverTest, GetStats) {
  scoped_refptr<MockWebRtcAudioTrack> webrtc_track =
      MockWebRtcAudioTrack::Create("webrtc_track");
  receiver_ = CreateReceiver(webrtc_track);

  // Make the mock return a blink version of the |webtc_report|. The mock does
  // not perform any stats filtering, we just set it to a dummy value.
  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(0u);
  webrtc_report->AddStats(
      std::make_unique<webrtc::RTCInboundRTPStreamStats>("stats-id", 1234u));
  peer_connection_->SetGetStatsReport(webrtc_report);

  auto obtainer = GetStats();
  // Make sure the operation is async.
  EXPECT_FALSE(obtainer->report());
  // Wait for the report, this performs the necessary run-loop.
  auto* report = obtainer->WaitForReport();
  EXPECT_TRUE(report);

  // Verify dummy value.
  EXPECT_EQ(report->Size(), 1u);
  auto stats = report->GetStats(blink::WebString::FromUTF8("stats-id"));
  EXPECT_TRUE(stats);
  EXPECT_EQ(stats->Timestamp(), 1.234);
}

}  // namespace content
