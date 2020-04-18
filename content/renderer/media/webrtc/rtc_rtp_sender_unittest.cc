// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_rtp_sender.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/mock_peer_connection_impl.h"
#include "content/renderer/media/webrtc/test/webrtc_stats_report_obtainer.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_rtc_stats.h"
#include "third_party/blink/public/platform/web_rtc_void_request.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"
#include "third_party/webrtc/api/stats/rtcstats_objects.h"
#include "third_party/webrtc/api/stats/rtcstatsreport.h"
#include "third_party/webrtc/api/test/mock_rtpsender.h"

using ::testing::_;
using ::testing::Return;

namespace content {

class RTCRtpSenderTest : public ::testing::Test {
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
    mock_webrtc_sender_ = new rtc::RefCountedObject<webrtc::MockRtpSender>();
  }

  void TearDown() override {
    sender_.reset();
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

  blink::WebMediaStreamTrack CreateWebTrack(const std::string& id) {
    blink::WebMediaStreamSource web_source;
    web_source.Initialize(
        blink::WebString::FromUTF8(id), blink::WebMediaStreamSource::kTypeAudio,
        blink::WebString::FromUTF8("local_audio_track"), false);
    MediaStreamAudioSource* audio_source = new MediaStreamAudioSource(true);
    // Takes ownership of |audio_source|.
    web_source.SetExtraData(audio_source);
    blink::WebMediaStreamTrack web_track;
    web_track.Initialize(web_source.Id(), web_source);
    audio_source->ConnectToTrack(web_track);
    return web_track;
  }

  std::unique_ptr<RTCRtpSender> CreateSender(
      blink::WebMediaStreamTrack web_track) {
    return std::make_unique<RTCRtpSender>(
        peer_connection_.get(), main_thread_,
        dependency_factory_->GetWebRtcSignalingThread(), stream_map_,
        mock_webrtc_sender_.get(), std::move(web_track),
        std::vector<blink::WebMediaStream>());
  }

  // Calls replaceTrack(), which is asynchronous, returning a callback that when
  // invoked waits for (run-loops) the operation to complete and returns whether
  // replaceTrack() was successful.
  base::OnceCallback<bool()> ReplaceTrack(
      blink::WebMediaStreamTrack web_track) {
    std::unique_ptr<base::RunLoop> run_loop = std::make_unique<base::RunLoop>();
    std::unique_ptr<bool> result_holder(new bool());
    // On complete, |*result_holder| is set with the result of replaceTrack()
    // and the |run_loop| quit.
    sender_->ReplaceTrack(
        web_track, base::BindOnce(&RTCRtpSenderTest::CallbackOnComplete,
                                  base::Unretained(this), result_holder.get(),
                                  run_loop.get()));
    // When the resulting callback is invoked, waits for |run_loop| to complete
    // and returns |*result_holder|.
    return base::BindOnce(&RTCRtpSenderTest::RunLoopAndReturnResult,
                          base::Unretained(this), std::move(result_holder),
                          std::move(run_loop));
  }

  scoped_refptr<WebRTCStatsReportObtainer> CallGetStats() {
    scoped_refptr<WebRTCStatsReportObtainer> obtainer =
        new WebRTCStatsReportObtainer();
    sender_->GetStats(obtainer->GetStatsCallbackWrapper());
    return obtainer;
  }

 protected:
  void CallbackOnComplete(bool* result_out,
                          base::RunLoop* run_loop,
                          bool result) {
    *result_out = result;
    run_loop->Quit();
  }

  bool RunLoopAndReturnResult(std::unique_ptr<bool> result_holder,
                              std::unique_ptr<base::RunLoop> run_loop) {
    run_loop->Run();
    return *result_holder;
  }

  // Message loop and child processes is needed for task queues and threading to
  // work, as is necessary to create tracks and adapters.
  base::MessageLoop message_loop_;
  ChildProcess child_process_;

  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<WebRtcMediaStreamAdapterMap> stream_map_;
  rtc::scoped_refptr<MockPeerConnectionImpl> peer_connection_;
  rtc::scoped_refptr<webrtc::MockRtpSender> mock_webrtc_sender_;
  std::unique_ptr<RTCRtpSender> sender_;
};

TEST_F(RTCRtpSenderTest, CreateSender) {
  auto web_track = CreateWebTrack("track_id");
  sender_ = CreateSender(web_track);
  EXPECT_FALSE(sender_->Track().IsNull());
  EXPECT_EQ(web_track.UniqueId(), sender_->Track().UniqueId());
}

TEST_F(RTCRtpSenderTest, CreateSenderWithNullTrack) {
  blink::WebMediaStreamTrack null_track;
  sender_ = CreateSender(null_track);
  EXPECT_TRUE(sender_->Track().IsNull());
}

TEST_F(RTCRtpSenderTest, ReplaceTrackSetsTrack) {
  auto web_track1 = CreateWebTrack("track1");
  sender_ = CreateSender(web_track1);

  auto web_track2 = CreateWebTrack("track2");
  EXPECT_CALL(*mock_webrtc_sender_, SetTrack(_)).WillOnce(Return(true));
  auto replaceTrackRunLoopAndGetResult = ReplaceTrack(web_track2);
  EXPECT_TRUE(std::move(replaceTrackRunLoopAndGetResult).Run());
  ASSERT_FALSE(sender_->Track().IsNull());
  EXPECT_EQ(web_track2.UniqueId(), sender_->Track().UniqueId());
}

TEST_F(RTCRtpSenderTest, ReplaceTrackWithNullTrack) {
  auto web_track = CreateWebTrack("track_id");
  sender_ = CreateSender(web_track);

  blink::WebMediaStreamTrack null_track;
  EXPECT_CALL(*mock_webrtc_sender_, SetTrack(_)).WillOnce(Return(true));
  auto replaceTrackRunLoopAndGetResult = ReplaceTrack(null_track);
  EXPECT_TRUE(std::move(replaceTrackRunLoopAndGetResult).Run());
  EXPECT_TRUE(sender_->Track().IsNull());
}

TEST_F(RTCRtpSenderTest, ReplaceTrackCanFail) {
  auto web_track = CreateWebTrack("track_id");
  sender_ = CreateSender(web_track);
  ASSERT_FALSE(sender_->Track().IsNull());
  EXPECT_EQ(web_track.UniqueId(), sender_->Track().UniqueId());

  blink::WebMediaStreamTrack null_track;
  EXPECT_CALL(*mock_webrtc_sender_, SetTrack(_)).WillOnce(Return(false));
  auto replaceTrackRunLoopAndGetResult = ReplaceTrack(null_track);
  EXPECT_FALSE(std::move(replaceTrackRunLoopAndGetResult).Run());
  // The track should not have been set.
  ASSERT_FALSE(sender_->Track().IsNull());
  EXPECT_EQ(web_track.UniqueId(), sender_->Track().UniqueId());
}

TEST_F(RTCRtpSenderTest, ReplaceTrackIsNotSetSynchronously) {
  auto web_track1 = CreateWebTrack("track1");
  sender_ = CreateSender(web_track1);

  auto web_track2 = CreateWebTrack("track2");
  EXPECT_CALL(*mock_webrtc_sender_, SetTrack(_)).WillOnce(Return(true));
  auto replaceTrackRunLoopAndGetResult = ReplaceTrack(web_track2);
  // The track should not be set until the run loop has executed.
  ASSERT_FALSE(sender_->Track().IsNull());
  EXPECT_NE(web_track2.UniqueId(), sender_->Track().UniqueId());
  // Wait for operation to run to ensure EXPECT_CALL is satisfied.
  std::move(replaceTrackRunLoopAndGetResult).Run();
}

TEST_F(RTCRtpSenderTest, GetStats) {
  auto web_track = CreateWebTrack("track_id");
  sender_ = CreateSender(web_track);

  // Make the mock return a blink version of the |webtc_report|. The mock does
  // not perform any stats filtering, we just set it to a dummy value.
  rtc::scoped_refptr<webrtc::RTCStatsReport> webrtc_report =
      webrtc::RTCStatsReport::Create(0u);
  webrtc_report->AddStats(
      std::make_unique<webrtc::RTCOutboundRTPStreamStats>("stats-id", 1234u));
  peer_connection_->SetGetStatsReport(webrtc_report);

  auto obtainer = CallGetStats();
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

TEST_F(RTCRtpSenderTest, CopiedSenderSharesInternalStates) {
  auto web_track = CreateWebTrack("track_id");
  sender_ = CreateSender(web_track);
  auto copy = std::make_unique<RTCRtpSender>(*sender_);
  // Copy shares original's ID.
  EXPECT_EQ(sender_->Id(), copy->Id());

  blink::WebMediaStreamTrack null_track;
  EXPECT_CALL(*mock_webrtc_sender_, SetTrack(_)).WillOnce(Return(true));
  auto replaceTrackRunLoopAndGetResult = ReplaceTrack(null_track);
  EXPECT_TRUE(std::move(replaceTrackRunLoopAndGetResult).Run());

  // Both original and copy shows a modified state.
  EXPECT_TRUE(sender_->Track().IsNull());
  EXPECT_TRUE(copy->Track().IsNull());
}

}  // namespace content
