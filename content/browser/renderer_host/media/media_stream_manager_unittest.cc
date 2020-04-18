// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/browser/renderer_host/media/media_stream_ui_proxy.h"
#include "content/browser/renderer_host/media/mock_video_capture_provider.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/media_observer.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_service_manager_context.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/fake_audio_log_factory.h"
#include "media/audio/test_audio_thread.h"
#include "media/base/media_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

#if defined(USE_ALSA)
#include "media/audio/alsa/audio_manager_alsa.h"
#elif defined(OS_ANDROID)
#include "media/audio/android/audio_manager_android.h"
#elif defined(OS_MACOSX)
#include "media/audio/mac/audio_manager_mac.h"
#elif defined(OS_WIN)
#include "media/audio/win/audio_manager_win.h"
#else
#include "media/audio/fake_audio_manager.h"
#endif

using testing::_;
using testing::Invoke;

namespace content {

#if defined(USE_ALSA)
typedef media::AudioManagerAlsa AudioManagerPlatform;
#elif defined(OS_MACOSX)
typedef media::AudioManagerMac AudioManagerPlatform;
#elif defined(OS_WIN)
typedef media::AudioManagerWin AudioManagerPlatform;
#elif defined(OS_ANDROID)
typedef media::AudioManagerAndroid AudioManagerPlatform;
#else
typedef media::FakeAudioManager AudioManagerPlatform;
#endif

namespace {

const char kMockSalt[] = "";

// This class mocks the audio manager and overrides some methods to ensure that
// we can run our tests on the buildbots.
class MockAudioManager : public AudioManagerPlatform {
 public:
  MockAudioManager()
      : AudioManagerPlatform(std::make_unique<media::TestAudioThread>(),
                             &fake_audio_log_factory_),
        num_output_devices_(2),
        num_input_devices_(2) {}
  ~MockAudioManager() override {}

  void GetAudioInputDeviceNames(
      media::AudioDeviceNames* device_names) override {
    DCHECK(device_names->empty());

    // AudioManagers add a default device when there is at least one real device
    if (num_input_devices_ > 0)
      device_names->push_back(media::AudioDeviceName::CreateDefault());

    for (size_t i = 0; i < num_input_devices_; i++) {
      device_names->push_back(media::AudioDeviceName(
          std::string("fake_device_name_") + base::NumberToString(i),
          std::string("fake_device_id_") + base::NumberToString(i)));
    }
  }

  void GetAudioOutputDeviceNames(
      media::AudioDeviceNames* device_names) override {
    DCHECK(device_names->empty());

    // AudioManagers add a default device when there is at least one real device
    if (num_output_devices_ > 0)
      device_names->push_back(media::AudioDeviceName::CreateDefault());

    for (size_t i = 0; i < num_output_devices_; i++) {
      device_names->push_back(media::AudioDeviceName(
          std::string("fake_device_name_") + base::NumberToString(i),
          std::string("fake_device_id_") + base::NumberToString(i)));
    }
  }

  media::AudioParameters GetDefaultOutputStreamParameters() override {
    return media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                  media::CHANNEL_LAYOUT_STEREO, 48000, 128);
  }

  media::AudioParameters GetOutputStreamParameters(
      const std::string& device_id) override {
    return media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                  media::CHANNEL_LAYOUT_STEREO, 48000, 128);
  }

  void SetNumAudioOutputDevices(size_t num_devices) {
    num_output_devices_ = num_devices;
  }

  void SetNumAudioInputDevices(size_t num_devices) {
    num_input_devices_ = num_devices;
  }

 private:
  media::FakeAudioLogFactory fake_audio_log_factory_;
  size_t num_output_devices_;
  size_t num_input_devices_;
  DISALLOW_COPY_AND_ASSIGN(MockAudioManager);
};

class MockMediaObserver : public MediaObserver {
 public:
  MOCK_METHOD0(OnAudioCaptureDevicesChanged, void());
  MOCK_METHOD0(OnVideoCaptureDevicesChanged, void());
  MOCK_METHOD6(
      OnMediaRequestStateChanged,
      void(int, int, int, const GURL&, MediaStreamType, MediaRequestState));
  MOCK_METHOD2(OnCreatingAudioStream, void(int, int));
  MOCK_METHOD5(OnSetCapturingLinkSecured,
               void(int, int, int, MediaStreamType, bool));
};

class TestBrowserClient : public ContentBrowserClient {
 public:
  explicit TestBrowserClient(MediaObserver* media_observer)
      : media_observer_(media_observer) {}
  ~TestBrowserClient() override {}
  MediaObserver* GetMediaObserver() override { return media_observer_; }

 private:
  MediaObserver* media_observer_;
};

}  // namespace

class MediaStreamManagerTest : public ::testing::Test {
 public:
  MediaStreamManagerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
    audio_manager_ = std::make_unique<MockAudioManager>();
    audio_system_ =
        std::make_unique<media::AudioSystemImpl>(audio_manager_.get());
    auto video_capture_provider = std::make_unique<MockVideoCaptureProvider>();
    video_capture_provider_ = video_capture_provider.get();
    service_manager_context_ = std::make_unique<TestServiceManagerContext>();
    media_stream_manager_ = std::make_unique<MediaStreamManager>(
        audio_system_.get(), audio_manager_->GetTaskRunner(),
        std::move(video_capture_provider));
    media_observer_ = std::make_unique<MockMediaObserver>();
    browser_content_client_ =
        std::make_unique<TestBrowserClient>(media_observer_.get());
    SetBrowserClientForTesting(browser_content_client_.get());
    base::RunLoop().RunUntilIdle();

    ON_CALL(*video_capture_provider_, DoGetDeviceInfosAsync(_))
        .WillByDefault(Invoke(
            [](VideoCaptureProvider::GetDeviceInfosCallback& result_callback) {
              std::vector<media::VideoCaptureDeviceInfo> stub_results;
              base::ResetAndReturn(&result_callback).Run(stub_results);
            }));
  }

  ~MediaStreamManagerTest() override {
    audio_manager_->Shutdown();
    service_manager_context_.reset();
  }

  MOCK_METHOD1(Response, void(int index));
  void ResponseCallback(int index,
                        const MediaStreamDevices& devices,
                        std::unique_ptr<MediaStreamUIProxy> ui_proxy) {
    Response(index);
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  run_loop_.QuitClosure());
  }

 protected:
  std::string MakeMediaAccessRequest(int index) {
    const int render_process_id = 1;
    const int render_frame_id = 1;
    const int page_request_id = 1;
    const url::Origin security_origin;
    MediaStreamManager::MediaAccessRequestCallback callback =
        base::BindOnce(&MediaStreamManagerTest::ResponseCallback,
                       base::Unretained(this), index);
    StreamControls controls(true, true);
    return media_stream_manager_->MakeMediaAccessRequest(
        render_process_id, render_frame_id, page_request_id, controls,
        security_origin, std::move(callback));
  }

  // media_stream_manager_ needs to outlive thread_bundle_ because it is a
  // MessageLoopCurrent::DestructionObserver. audio_manager_ needs to outlive
  // thread_bundle_ because it uses the underlying message loop.
  std::unique_ptr<MediaStreamManager> media_stream_manager_;
  std::unique_ptr<MockMediaObserver> media_observer_;
  std::unique_ptr<ContentBrowserClient> browser_content_client_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  MockVideoCaptureProvider* video_capture_provider_;
  base::RunLoop run_loop_;

 private:
  std::unique_ptr<TestServiceManagerContext> service_manager_context_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamManagerTest);
};

TEST_F(MediaStreamManagerTest, MakeMediaAccessRequest) {
  MakeMediaAccessRequest(0);

  // Expecting the callback will be triggered and quit the test.
  EXPECT_CALL(*this, Response(0));
  run_loop_.Run();
}

TEST_F(MediaStreamManagerTest, MakeAndCancelMediaAccessRequest) {
  std::string label = MakeMediaAccessRequest(0);

  // Request cancellation notifies closing of all stream types.
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_AUDIO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_VIDEO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_TAB_AUDIO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_TAB_VIDEO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_, OnMediaRequestStateChanged(
                                    _, _, _, _, MEDIA_DESKTOP_VIDEO_CAPTURE,
                                    MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_, OnMediaRequestStateChanged(
                                    _, _, _, _, MEDIA_DESKTOP_AUDIO_CAPTURE,
                                    MEDIA_REQUEST_STATE_CLOSING));
  media_stream_manager_->CancelRequest(label);
  run_loop_.RunUntilIdle();
}

TEST_F(MediaStreamManagerTest, MakeMultipleRequests) {
  // First request.
  std::string label1 = MakeMediaAccessRequest(0);

  // Second request.
  int render_process_id = 2;
  int render_frame_id = 2;
  int page_request_id = 2;
  url::Origin security_origin;
  StreamControls controls(true, true);
  MediaStreamManager::MediaAccessRequestCallback callback = base::BindOnce(
      &MediaStreamManagerTest::ResponseCallback, base::Unretained(this), 1);
  std::string label2 = media_stream_manager_->MakeMediaAccessRequest(
      render_process_id, render_frame_id, page_request_id, controls,
      security_origin, std::move(callback));

  // Expecting the callbackS from requests will be triggered and quit the test.
  // Note, the callbacks might come in a different order depending on the
  // value of labels.
  EXPECT_CALL(*this, Response(0));
  EXPECT_CALL(*this, Response(1));
  run_loop_.Run();
}

TEST_F(MediaStreamManagerTest, MakeAndCancelMultipleRequests) {
  std::string label1 = MakeMediaAccessRequest(0);
  std::string label2 = MakeMediaAccessRequest(1);

  // Cancelled request notifies closing of all stream types.
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_AUDIO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_VIDEO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_TAB_AUDIO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_TAB_VIDEO_CAPTURE,
                                         MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_, OnMediaRequestStateChanged(
                                    _, _, _, _, MEDIA_DESKTOP_VIDEO_CAPTURE,
                                    MEDIA_REQUEST_STATE_CLOSING));
  EXPECT_CALL(*media_observer_, OnMediaRequestStateChanged(
                                    _, _, _, _, MEDIA_DESKTOP_AUDIO_CAPTURE,
                                    MEDIA_REQUEST_STATE_CLOSING));

  media_stream_manager_->CancelRequest(label1);

  // The request that proceeds sets state to MEDIA_REQUEST_STATE_REQUESTED when
  // starting a device enumeration, and to MEDIA_REQUEST_STATE_PENDING_APPROVAL
  // when the enumeration is completed and also when the request is posted to
  // the UI.
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_AUDIO_CAPTURE,
                                         MEDIA_REQUEST_STATE_REQUESTED));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_VIDEO_CAPTURE,
                                         MEDIA_REQUEST_STATE_REQUESTED));
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_AUDIO_CAPTURE,
                                         MEDIA_REQUEST_STATE_PENDING_APPROVAL))
      .Times(2);
  EXPECT_CALL(*media_observer_,
              OnMediaRequestStateChanged(_, _, _, _, MEDIA_DEVICE_VIDEO_CAPTURE,
                                         MEDIA_REQUEST_STATE_PENDING_APPROVAL))
      .Times(2);

  // Expecting the callback from the second request will be triggered and
  // quit the test.
  EXPECT_CALL(*this, Response(1));
  run_loop_.Run();
}

TEST_F(MediaStreamManagerTest, DeviceID) {
  url::Origin security_origin = url::Origin::Create(GURL("http://localhost"));
  const std::string unique_default_id(
      media::AudioDeviceDescription::kDefaultDeviceId);
  const std::string hashed_default_id =
      MediaStreamManager::GetHMACForMediaDeviceID(kMockSalt, security_origin,
                                                  unique_default_id);
  EXPECT_TRUE(MediaStreamManager::DoesMediaDeviceIDMatchHMAC(
      kMockSalt, security_origin, hashed_default_id, unique_default_id));
  EXPECT_EQ(unique_default_id, hashed_default_id);

  const std::string unique_communications_id(
      media::AudioDeviceDescription::kCommunicationsDeviceId);
  const std::string hashed_communications_id =
      MediaStreamManager::GetHMACForMediaDeviceID(kMockSalt, security_origin,
                                                  unique_communications_id);
  EXPECT_TRUE(MediaStreamManager::DoesMediaDeviceIDMatchHMAC(
      kMockSalt, security_origin, hashed_communications_id,
      unique_communications_id));
  EXPECT_EQ(unique_communications_id, hashed_communications_id);

  const std::string unique_other_id("other-unique-id");
  const std::string hashed_other_id =
      MediaStreamManager::GetHMACForMediaDeviceID(kMockSalt, security_origin,
                                                  unique_other_id);
  EXPECT_TRUE(MediaStreamManager::DoesMediaDeviceIDMatchHMAC(
      kMockSalt, security_origin, hashed_other_id, unique_other_id));
  EXPECT_NE(unique_other_id, hashed_other_id);
  EXPECT_EQ(hashed_other_id.size(), 64U);
  for (const char& c : hashed_other_id)
    EXPECT_TRUE(base::IsAsciiDigit(c) || (c >= 'a' && c <= 'f'));
}

}  // namespace content
