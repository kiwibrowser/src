// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_set_remote_description_observer.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/optional.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_process.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/mock_peer_connection_impl.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/web/web_heap.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"
#include "third_party/webrtc/media/base/fakemediaengine.h"
#include "third_party/webrtc/pc/test/mock_peerconnection.h"

using ::testing::Return;

namespace content {

class WebRtcSetRemoteDescriptionObserverForTest
    : public WebRtcSetRemoteDescriptionObserver {
 public:
  bool called() const { return states_or_error_.has_value(); }
  bool result() const { return states_or_error_->ok(); }

  const WebRtcSetRemoteDescriptionObserver::States& states() const {
    DCHECK(called() && result());
    return states_or_error_->value();
  }
  const webrtc::RTCError& error() const {
    DCHECK(called() && !result());
    return states_or_error_->error();
  }

  // WebRtcSetRemoteDescriptionObserver implementation.
  void OnSetRemoteDescriptionComplete(
      webrtc::RTCErrorOr<States> states_or_error) override {
    states_or_error_ = std::move(states_or_error);
  }

 private:
  ~WebRtcSetRemoteDescriptionObserverForTest() override {}

  base::Optional<webrtc::RTCErrorOr<WebRtcSetRemoteDescriptionObserver::States>>
      states_or_error_;
  WebRtcSetRemoteDescriptionObserver::States states_;
};

class WebRtcSetRemoteDescriptionObserverHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    pc_ = new webrtc::MockPeerConnection(new webrtc::FakePeerConnectionFactory(
        std::unique_ptr<cricket::MediaEngineInterface>(
            new cricket::FakeMediaEngine())));
    dependency_factory_.reset(new MockPeerConnectionDependencyFactory());
    main_thread_ = blink::scheduler::GetSingleThreadTaskRunnerForTesting();
    scoped_refptr<WebRtcMediaStreamAdapterMap> map =
        new WebRtcMediaStreamAdapterMap(
            dependency_factory_.get(), main_thread_,
            new WebRtcMediaStreamTrackAdapterMap(dependency_factory_.get(),
                                                 main_thread_));
    observer_ = new WebRtcSetRemoteDescriptionObserverForTest();
    observer_handler_ = WebRtcSetRemoteDescriptionObserverHandler::Create(
        main_thread_, pc_, map, observer_);
  }

  void TearDown() override { blink::WebHeap::CollectAllGarbageForTesting(); }

  void InvokeOnSetRemoteDescriptionComplete(webrtc::RTCError error) {
    base::RunLoop run_loop;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WebRtcSetRemoteDescriptionObserverHandlerTest::
                InvokeOnSetRemoteDescriptionCompleteOnSignalingThread,
            base::Unretained(this), std::move(error),
            base::Unretained(&run_loop)));
    run_loop.Run();
  }

 protected:
  void InvokeOnSetRemoteDescriptionCompleteOnSignalingThread(
      webrtc::RTCError error,
      base::RunLoop* run_loop) {
    observer_handler_->OnSetRemoteDescriptionComplete(std::move(error));
    run_loop->Quit();
  }

  // The ScopedTaskEnvironment prevents the ChildProcess from leaking a
  // TaskScheduler.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ChildProcess child_process_;

  scoped_refptr<webrtc::MockPeerConnection> pc_;
  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<WebRtcSetRemoteDescriptionObserverForTest> observer_;
  scoped_refptr<WebRtcSetRemoteDescriptionObserverHandler> observer_handler_;

  std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers_;
};

TEST_F(WebRtcSetRemoteDescriptionObserverHandlerTest, OnSuccess) {
  scoped_refptr<MockWebRtcAudioTrack> added_track =
      MockWebRtcAudioTrack::Create("added_track");
  scoped_refptr<webrtc::MediaStreamInterface> added_stream(
      new rtc::RefCountedObject<MockMediaStream>("added_stream"));
  scoped_refptr<webrtc::RtpReceiverInterface> added_receiver(
      new rtc::RefCountedObject<FakeRtpReceiver>(
          added_track.get(),
          std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>(
              {added_stream.get()})));

  receivers_.push_back(added_receiver.get());
  EXPECT_CALL(*pc_, GetReceivers()).WillRepeatedly(Return(receivers_));

  InvokeOnSetRemoteDescriptionComplete(webrtc::RTCError::OK());
  EXPECT_TRUE(observer_->called());
  EXPECT_TRUE(observer_->result());

  EXPECT_EQ(1u, observer_->states().receiver_states.size());
  const WebRtcReceiverState& receiver_state =
      observer_->states().receiver_states[0];
  EXPECT_EQ(added_receiver, receiver_state.receiver);
  EXPECT_EQ(added_track, receiver_state.track_ref->webrtc_track());
  EXPECT_EQ(1u, receiver_state.stream_refs.size());
  EXPECT_EQ(added_stream,
            receiver_state.stream_refs[0]->adapter().webrtc_stream());
}

TEST_F(WebRtcSetRemoteDescriptionObserverHandlerTest, OnFailure) {
  webrtc::RTCError error(webrtc::RTCErrorType::INVALID_PARAMETER, "Oh noes!");
  InvokeOnSetRemoteDescriptionComplete(std::move(error));
  EXPECT_TRUE(observer_->called());
  EXPECT_FALSE(observer_->result());
  EXPECT_EQ(std::string("Oh noes!"), observer_->error().message());
}

}  // namespace content
