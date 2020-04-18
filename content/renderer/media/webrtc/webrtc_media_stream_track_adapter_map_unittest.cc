// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "content/child/child_process.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"

namespace content {

class WebRtcMediaStreamTrackAdapterMapTest : public ::testing::Test {
 public:
  void SetUp() override {
    dependency_factory_.reset(new MockPeerConnectionDependencyFactory());
    main_thread_ = blink::scheduler::GetSingleThreadTaskRunnerForTesting();
    map_ = new WebRtcMediaStreamTrackAdapterMap(dependency_factory_.get(),
                                                main_thread_);
  }

  void TearDown() override { blink::WebHeap::CollectAllGarbageForTesting(); }

  scoped_refptr<base::SingleThreadTaskRunner> signaling_thread() const {
    return dependency_factory_->GetWebRtcSignalingThread();
  }

  blink::WebMediaStreamTrack CreateLocalTrack(const std::string& id) {
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

  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>
  GetOrCreateRemoteTrackAdapter(
      webrtc::MediaStreamTrackInterface* webrtc_track) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> adapter;
    signaling_thread()->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcMediaStreamTrackAdapterMapTest::
                           GetOrCreateRemoteTrackAdapterOnSignalingThread,
                       base::Unretained(this), base::Unretained(webrtc_track),
                       &adapter));
    RunMessageLoopsUntilIdle();
    return adapter;
  }

  void GetOrCreateRemoteTrackAdapterOnSignalingThread(
      webrtc::MediaStreamTrackInterface* webrtc_track,
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>* adapter) {
    DCHECK(signaling_thread()->BelongsToCurrentThread());
    *adapter = map_->GetOrCreateRemoteTrackAdapter(webrtc_track);
  }

  // Runs message loops on the webrtc signaling thread and the main thread until
  // idle.
  void RunMessageLoopsUntilIdle() {
    DCHECK(main_thread_->BelongsToCurrentThread());
    base::WaitableEvent waitable_event(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    signaling_thread()->PostTask(
        FROM_HERE, base::BindOnce(&WebRtcMediaStreamTrackAdapterMapTest::
                                      RunMessageLoopUntilIdleOnSignalingThread,
                                  base::Unretained(this), &waitable_event));
    waitable_event.Wait();
    base::RunLoop().RunUntilIdle();
  }

  void RunMessageLoopUntilIdleOnSignalingThread(
      base::WaitableEvent* waitable_event) {
    DCHECK(signaling_thread()->BelongsToCurrentThread());
    base::RunLoop().RunUntilIdle();
    waitable_event->Signal();
  }

 protected:
  // The ScopedTaskEnvironment prevents the ChildProcess from leaking a
  // TaskScheduler.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ChildProcess child_process_;

  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<WebRtcMediaStreamTrackAdapterMap> map_;
};

TEST_F(WebRtcMediaStreamTrackAdapterMapTest, AddAndRemoveLocalTrackAdapter) {
  blink::WebMediaStreamTrack web_track = CreateLocalTrack("local_track");
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> adapter_ref =
      map_->GetOrCreateLocalTrackAdapter(web_track);
  EXPECT_TRUE(adapter_ref->is_initialized());
  EXPECT_EQ(adapter_ref->GetAdapterForTesting(),
            map_->GetLocalTrackAdapter(web_track)->GetAdapterForTesting());
  EXPECT_EQ(1u, map_->GetLocalTrackCount());

  // "GetOrCreate" for already existing track.
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> adapter_ref2 =
      map_->GetOrCreateLocalTrackAdapter(web_track);
  EXPECT_EQ(adapter_ref->GetAdapterForTesting(),
            adapter_ref2->GetAdapterForTesting());
  EXPECT_EQ(1u, map_->GetLocalTrackCount());

  adapter_ref2.reset();  // Not the last reference.
  EXPECT_TRUE(adapter_ref->GetAdapterForTesting()->is_initialized());
  EXPECT_EQ(1u, map_->GetLocalTrackCount());

  // Destroying all references to the adapter should remove it from the map and
  // dispose it.
  adapter_ref.reset();
  EXPECT_EQ(0u, map_->GetLocalTrackCount());
  EXPECT_EQ(nullptr, map_->GetLocalTrackAdapter(web_track));
  // Allow the disposing of track to occur.
  RunMessageLoopsUntilIdle();
}

TEST_F(WebRtcMediaStreamTrackAdapterMapTest, AddAndRemoveRemoteTrackAdapter) {
  scoped_refptr<MockWebRtcAudioTrack> webrtc_track =
      MockWebRtcAudioTrack::Create("remote_track");
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> adapter_ref =
      GetOrCreateRemoteTrackAdapter(webrtc_track.get());
  EXPECT_TRUE(adapter_ref->is_initialized());
  EXPECT_EQ(
      adapter_ref->GetAdapterForTesting(),
      map_->GetRemoteTrackAdapter(webrtc_track.get())->GetAdapterForTesting());
  EXPECT_EQ(1u, map_->GetRemoteTrackCount());

  // "GetOrCreate" for already existing track.
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> adapter_ref2 =
      GetOrCreateRemoteTrackAdapter(webrtc_track.get());
  EXPECT_EQ(adapter_ref->GetAdapterForTesting(),
            adapter_ref2->GetAdapterForTesting());
  EXPECT_EQ(1u, map_->GetRemoteTrackCount());

  adapter_ref2.reset();  // Not the last reference.
  EXPECT_TRUE(adapter_ref->GetAdapterForTesting()->is_initialized());
  EXPECT_EQ(1u, map_->GetRemoteTrackCount());

  // Destroying all references to the adapter should remove it from the map and
  // dispose it.
  adapter_ref.reset();
  EXPECT_EQ(0u, map_->GetRemoteTrackCount());
  EXPECT_EQ(nullptr, map_->GetRemoteTrackAdapter(webrtc_track.get()));
  // Allow the disposing of track to occur.
  RunMessageLoopsUntilIdle();
}

TEST_F(WebRtcMediaStreamTrackAdapterMapTest,
       LocalAndRemoteTrackAdaptersWithSameID) {
  // Local and remote tracks should be able to use the same id without conflict.
  const char* id = "id";

  blink::WebMediaStreamTrack local_web_track = CreateLocalTrack(id);
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> local_adapter =
      map_->GetOrCreateLocalTrackAdapter(local_web_track);
  EXPECT_TRUE(local_adapter->is_initialized());
  EXPECT_EQ(
      local_adapter->GetAdapterForTesting(),
      map_->GetLocalTrackAdapter(local_web_track)->GetAdapterForTesting());
  EXPECT_EQ(1u, map_->GetLocalTrackCount());

  scoped_refptr<MockWebRtcAudioTrack> remote_webrtc_track =
      MockWebRtcAudioTrack::Create(id);
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> remote_adapter =
      GetOrCreateRemoteTrackAdapter(remote_webrtc_track.get());
  EXPECT_TRUE(remote_adapter->is_initialized());
  EXPECT_EQ(remote_adapter->GetAdapterForTesting(),
            map_->GetRemoteTrackAdapter(remote_webrtc_track.get())
                ->GetAdapterForTesting());
  EXPECT_NE(local_adapter->GetAdapterForTesting(),
            remote_adapter->GetAdapterForTesting());
  EXPECT_EQ(1u, map_->GetRemoteTrackCount());

  // Destroying all references to the adapters should remove them from the map.
  local_adapter.reset();
  remote_adapter.reset();
  EXPECT_EQ(0u, map_->GetLocalTrackCount());
  EXPECT_EQ(0u, map_->GetRemoteTrackCount());
  EXPECT_EQ(nullptr, map_->GetLocalTrackAdapter(local_web_track));
  EXPECT_EQ(nullptr, map_->GetRemoteTrackAdapter(remote_webrtc_track.get()));
  // Allow the disposing of tracks to occur.
  RunMessageLoopsUntilIdle();
}

TEST_F(WebRtcMediaStreamTrackAdapterMapTest, GetMissingLocalTrackAdapter) {
  blink::WebMediaStreamTrack local_web_track = CreateLocalTrack("missing");
  EXPECT_EQ(nullptr, map_->GetLocalTrackAdapter(local_web_track));
}

TEST_F(WebRtcMediaStreamTrackAdapterMapTest, GetMissingRemoteTrackAdapter) {
  scoped_refptr<MockWebRtcAudioTrack> webrtc_track =
      MockWebRtcAudioTrack::Create("missing");
  EXPECT_EQ(nullptr, map_->GetRemoteTrackAdapter(webrtc_track.get()));
}

// Continuously calls GetOrCreateLocalTrackAdapter() on the main thread and
// GetOrCreateRemoteTrackAdapter() on the signaling thread hoping to hit
// deadlocks if the operations were to synchronize with the other thread while
// holding the lock.
//
// Note that this deadlock has been notoriously difficult to reproduce. This
// test is added as an attempt to guard against this type of regression, but do
// not trust that if this test passes there is no risk of deadlock.
class WebRtcMediaStreamTrackAdapterMapStressTest
    : public WebRtcMediaStreamTrackAdapterMapTest {
 public:
  WebRtcMediaStreamTrackAdapterMapStressTest()
      : WebRtcMediaStreamTrackAdapterMapTest(), remaining_iterations_(0u) {}

  void RunStressTest(size_t iterations) {
    base::RunLoop run_loop;
    remaining_iterations_ = iterations;
    PostSignalingThreadLoop();
    MainThreadLoop(&run_loop);
    run_loop.Run();
    // The run loop ensures all operations have began executing, but does not
    // guarantee that all of them are complete, i.e. that track adapters have
    // been fully initialized and subequently disposed. For that we need to run
    // until idle or else we may tear down the test prematurely.
    RunMessageLoopsUntilIdle();
  }

  void MainThreadLoop(base::RunLoop* run_loop) {
    for (size_t i = 0u; i < 5u; ++i) {
      map_->GetOrCreateLocalTrackAdapter(CreateLocalTrack("local_track_id"));
    }
    if (--remaining_iterations_ > 0) {
      PostSignalingThreadLoop();
      PostMainThreadLoop(run_loop);
    } else {
      // We are now done, but there may still be operations pending to execute
      // on signaling thread so we perform Quit() in a post to the signaling
      // thread. This ensures that Quit() is called after all operations have
      // began executing (but does not guarantee that all operations have
      // completed).
      signaling_thread()->PostTask(
          FROM_HERE,
          base::BindOnce(&WebRtcMediaStreamTrackAdapterMapStressTest::
                             QuitRunLoopOnSignalingThread,
                         base::Unretained(this), base::Unretained(run_loop)));
    }
  }

  void PostMainThreadLoop(base::RunLoop* run_loop) {
    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WebRtcMediaStreamTrackAdapterMapStressTest::MainThreadLoop,
            base::Unretained(this), base::Unretained(run_loop)));
  }

  void SignalingThreadLoop() {
    std::vector<std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>>
        track_refs;
    for (size_t i = 0u; i < 5u; ++i) {
      track_refs.push_back(map_->GetOrCreateRemoteTrackAdapter(
          MockWebRtcAudioTrack::Create("remote_track_id")));
    }
    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcMediaStreamTrackAdapterMapStressTest::
                           DestroyAdapterRefsOnMainThread,
                       base::Unretained(this), std::move(track_refs)));
  }

  void PostSignalingThreadLoop() {
    signaling_thread()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WebRtcMediaStreamTrackAdapterMapStressTest::SignalingThreadLoop,
            base::Unretained(this)));
  }

  void DestroyAdapterRefsOnMainThread(
      std::vector<std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>>
          track_refs) {}

  void QuitRunLoopOnSignalingThread(base::RunLoop* run_loop) {
    run_loop->Quit();
  }

 private:
  size_t remaining_iterations_;
};

TEST_F(WebRtcMediaStreamTrackAdapterMapStressTest, StressTest) {
  const size_t kNumStressTestIterations = 1000u;
  RunStressTest(kNumStressTestIterations);
}

}  // namespace content
