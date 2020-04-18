// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/media_devices_manager.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/renderer_host/media/in_process_video_capture_provider.h"
#include "content/browser/renderer_host/media/video_capture_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/audio/audio_device_name.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/fake_audio_log_factory.h"
#include "media/audio/fake_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "media/capture/video/fake_video_capture_device_factory.h"
#include "media/capture/video/video_capture_system_impl.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using testing::_;
using testing::SaveArg;

namespace content {

namespace {

const int kRenderProcessId = 1;
const int kRenderFrameId = 3;

// Number of client enumerations to simulate on each test run.
// This allows testing that a single call to low-level enumeration functions
// is performed when cache is enabled, regardless of the number of client calls.
const int kNumCalls = 3;

const auto kIgnoreLogMessageCB = base::BindRepeating([](const std::string&) {});

MediaDeviceSaltAndOrigin GetSaltAndOrigin(int /* process_id */,
                                          int /* frame_id */) {
  return MediaDeviceSaltAndOrigin(
      "fake_media_device_salt", "fake_group_id_salt",
      url::Origin::Create(GURL("https://test.com")));
}

// This class mocks the audio manager and overrides some methods to ensure that
// we can run simulate device changes.
class MockAudioManager : public media::FakeAudioManager {
 public:
  MockAudioManager()
      : FakeAudioManager(std::make_unique<media::TestAudioThread>(),
                         &fake_audio_log_factory_),
        num_output_devices_(2),
        num_input_devices_(2) {}
  ~MockAudioManager() override {}

  MOCK_METHOD1(MockGetAudioInputDeviceNames, void(media::AudioDeviceNames*));
  MOCK_METHOD1(MockGetAudioOutputDeviceNames, void(media::AudioDeviceNames*));

  void GetAudioInputDeviceNames(
      media::AudioDeviceNames* device_names) override {
    DCHECK(device_names->empty());
    for (size_t i = 0; i < num_input_devices_; i++) {
      device_names->push_back(media::AudioDeviceName(
          std::string("fake_device_name_") + base::NumberToString(i),
          std::string("fake_device_id_") + base::NumberToString(i)));
    }
    MockGetAudioInputDeviceNames(device_names);
  }

  void GetAudioOutputDeviceNames(
      media::AudioDeviceNames* device_names) override {
    DCHECK(device_names->empty());
    for (size_t i = 0; i < num_output_devices_; i++) {
      device_names->push_back(media::AudioDeviceName(
          std::string("fake_device_name_") + base::NumberToString(i),
          std::string("fake_device_id_") + base::NumberToString(i)));
    }
    MockGetAudioOutputDeviceNames(device_names);
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

// This class mocks the video capture device factory and overrides some methods
// to ensure that we can simulate device changes.
class MockVideoCaptureDeviceFactory
    : public media::FakeVideoCaptureDeviceFactory {
 public:
  MockVideoCaptureDeviceFactory() {}
  ~MockVideoCaptureDeviceFactory() override {}

  MOCK_METHOD0(MockGetDeviceDescriptors, void());
  void GetDeviceDescriptors(
      media::VideoCaptureDeviceDescriptors* device_descriptors) override {
    media::FakeVideoCaptureDeviceFactory::GetDeviceDescriptors(
        device_descriptors);
    MockGetDeviceDescriptors();
  }
};

class MockMediaDevicesListener : public blink::mojom::MediaDevicesListener {
 public:
  MockMediaDevicesListener() {}

  MOCK_METHOD2(OnDevicesChanged,
               void(MediaDeviceType, const MediaDeviceInfoArray&));

  blink::mojom::MediaDevicesListenerPtr CreateInterfacePtrAndBind() {
    blink::mojom::MediaDevicesListenerPtr listener;
    bindings_.AddBinding(this, mojo::MakeRequest(&listener));
    return listener;
  }

 private:
  mojo::BindingSet<blink::mojom::MediaDevicesListener> bindings_;
};

void VerifyDeviceAndGroupID(const std::vector<MediaDeviceInfoArray>& array) {
  for (const auto& device_infos : array) {
    for (const auto& device_info : device_infos) {
      EXPECT_FALSE(device_info.device_id.empty());
      EXPECT_FALSE(device_info.group_id.empty());
    }
  }
}

}  // namespace

class MediaDevicesManagerTest : public ::testing::Test {
 public:
  MediaDevicesManagerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        video_capture_device_factory_(nullptr) {}
  ~MediaDevicesManagerTest() override { audio_manager_->Shutdown(); }

  MOCK_METHOD1(MockCallback, void(const MediaDeviceEnumeration&));

  void EnumerateCallback(base::RunLoop* run_loop,
                         const MediaDeviceEnumeration& result) {
    for (int i = 0; i < NUM_MEDIA_DEVICE_TYPES; ++i) {
      for (const auto& device_info : result[i]) {
        EXPECT_FALSE(device_info.device_id.empty());
        EXPECT_FALSE(device_info.group_id.empty());
      }
    }
    MockCallback(result);
    run_loop->Quit();
  }

 protected:
  void SetUp() override {
    audio_manager_ = std::make_unique<MockAudioManager>();
    audio_system_ =
        std::make_unique<media::AudioSystemImpl>(audio_manager_.get());
    auto video_capture_device_factory =
        std::make_unique<MockVideoCaptureDeviceFactory>();
    video_capture_device_factory_ = video_capture_device_factory.get();
    auto video_capture_system = std::make_unique<media::VideoCaptureSystemImpl>(
        std::move(video_capture_device_factory));
    auto video_capture_provider =
        std::make_unique<InProcessVideoCaptureProvider>(
            std::move(video_capture_system),
            base::ThreadTaskRunnerHandle::Get(), kIgnoreLogMessageCB);
    video_capture_manager_ = new VideoCaptureManager(
        std::move(video_capture_provider), kIgnoreLogMessageCB);
    media_devices_manager_.reset(new MediaDevicesManager(
        audio_system_.get(), video_capture_manager_, nullptr));
    media_devices_manager_->set_salt_and_origin_callback_for_testing(
        base::BindRepeating(&GetSaltAndOrigin));
  }

  void EnableCache(MediaDeviceType type) {
    media_devices_manager_->SetCachePolicy(
        type, MediaDevicesManager::CachePolicy::SYSTEM_MONITOR);
  }

  // Must outlive MediaDevicesManager as ~MediaDevicesManager() verifies it's
  // running on the IO thread.
  TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<MediaDevicesManager> media_devices_manager_;
  scoped_refptr<VideoCaptureManager> video_capture_manager_;
  MockVideoCaptureDeviceFactory* video_capture_device_factory_;
  std::unique_ptr<MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaDevicesManagerTest);
};

TEST_F(MediaDevicesManagerTest, EnumerateNoCacheAudioInput) {
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_))
      .Times(kNumCalls);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(0);
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(0);
  EXPECT_CALL(*this, MockCallback(_)).Times(kNumCalls);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateNoCacheVideoInput) {
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(0);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(kNumCalls);
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(0);
  EXPECT_CALL(*this, MockCallback(_)).Times(kNumCalls);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateNoCacheAudioOutput) {
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(0);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(0);
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_))
      .Times(kNumCalls);
  EXPECT_CALL(*this, MockCallback(_)).Times(kNumCalls);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateNoCacheAudio) {
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_))
      .Times(kNumCalls);
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_))
      .Times(kNumCalls);
  EXPECT_CALL(*this, MockCallback(_)).Times(kNumCalls);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateCacheAudio) {
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(1);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(0);
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(1);
  EXPECT_CALL(*this, MockCallback(_)).Times(kNumCalls);
  EnableCache(MEDIA_DEVICE_TYPE_AUDIO_INPUT);
  EnableCache(MEDIA_DEVICE_TYPE_AUDIO_OUTPUT);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateCacheVideo) {
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(0);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(1);
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(0);
  EXPECT_CALL(*this, MockCallback(_)).Times(kNumCalls);
  EnableCache(MEDIA_DEVICE_TYPE_VIDEO_INPUT);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateCacheAudioWithDeviceChanges) {
  MediaDeviceEnumeration enumeration;
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(2);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(0);
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(2);
  EXPECT_CALL(*this, MockCallback(_))
      .Times(2 * kNumCalls)
      .WillRepeatedly(SaveArg<0>(&enumeration));

  size_t num_audio_input_devices = 5;
  size_t num_audio_output_devices = 3;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);
  EnableCache(MEDIA_DEVICE_TYPE_AUDIO_INPUT);
  EnableCache(MEDIA_DEVICE_TYPE_AUDIO_OUTPUT);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
    EXPECT_EQ(num_audio_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_INPUT].size());
    EXPECT_EQ(num_audio_output_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT].size());
  }

  // Simulate device change
  num_audio_input_devices = 3;
  num_audio_output_devices = 4;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);
  media_devices_manager_->OnDevicesChanged(base::SystemMonitor::DEVTYPE_AUDIO);

  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
    EXPECT_EQ(num_audio_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_INPUT].size());
    EXPECT_EQ(num_audio_output_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT].size());
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateCacheVideoWithDeviceChanges) {
  MediaDeviceEnumeration enumeration;
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(0);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(2);
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(0);
  EXPECT_CALL(*this, MockCallback(_))
      .Times(2 * kNumCalls)
      .WillRepeatedly(SaveArg<0>(&enumeration));

  // Simulate device change
  size_t num_video_input_devices = 5;
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  EnableCache(MEDIA_DEVICE_TYPE_VIDEO_INPUT);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
    EXPECT_EQ(num_video_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_VIDEO_INPUT].size());
  }

  // Simulate device change
  num_video_input_devices = 9;
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  media_devices_manager_->OnDevicesChanged(
      base::SystemMonitor::DEVTYPE_VIDEO_CAPTURE);

  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
    EXPECT_EQ(num_video_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_VIDEO_INPUT].size());
  }
}

TEST_F(MediaDevicesManagerTest, EnumerateCacheAllWithDeviceChanges) {
  MediaDeviceEnumeration enumeration;
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(2);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(2);
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(2);
  EXPECT_CALL(*this, MockCallback(_))
      .Times(2 * kNumCalls)
      .WillRepeatedly(SaveArg<0>(&enumeration));

  size_t num_audio_input_devices = 5;
  size_t num_video_input_devices = 4;
  size_t num_audio_output_devices = 3;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);
  EnableCache(MEDIA_DEVICE_TYPE_AUDIO_INPUT);
  EnableCache(MEDIA_DEVICE_TYPE_AUDIO_OUTPUT);
  EnableCache(MEDIA_DEVICE_TYPE_VIDEO_INPUT);
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
    EXPECT_EQ(num_audio_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_INPUT].size());
    EXPECT_EQ(num_video_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_VIDEO_INPUT].size());
    EXPECT_EQ(num_audio_output_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT].size());
  }

  // Simulate device changes
  num_audio_input_devices = 3;
  num_video_input_devices = 2;
  num_audio_output_devices = 4;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);
  media_devices_manager_->OnDevicesChanged(base::SystemMonitor::DEVTYPE_AUDIO);
  media_devices_manager_->OnDevicesChanged(
      base::SystemMonitor::DEVTYPE_VIDEO_CAPTURE);

  for (int i = 0; i < kNumCalls; i++) {
    base::RunLoop run_loop;
    media_devices_manager_->EnumerateDevices(
        devices_to_enumerate,
        base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                   base::Unretained(this), &run_loop));
    run_loop.Run();
    EXPECT_EQ(num_audio_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_INPUT].size());
    EXPECT_EQ(num_video_input_devices,
              enumeration[MEDIA_DEVICE_TYPE_VIDEO_INPUT].size());
    EXPECT_EQ(num_audio_output_devices,
              enumeration[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT].size());
  }
}

TEST_F(MediaDevicesManagerTest, SubscribeDeviceChanges) {
  EXPECT_CALL(*audio_manager_, MockGetAudioOutputDeviceNames(_)).Times(3);
  EXPECT_CALL(*video_capture_device_factory_, MockGetDeviceDescriptors())
      .Times(3);
  EXPECT_CALL(*audio_manager_, MockGetAudioInputDeviceNames(_)).Times(3);

  size_t num_audio_input_devices = 5;
  size_t num_video_input_devices = 4;
  size_t num_audio_output_devices = 3;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);

  // Run an enumeration to make sure |media_devices_manager_| has the new
  // configuration.
  EXPECT_CALL(*this, MockCallback(_));
  MediaDevicesManager::BoolDeviceTypes devices_to_enumerate;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  devices_to_enumerate[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  base::RunLoop run_loop;
  media_devices_manager_->EnumerateDevices(
      devices_to_enumerate,
      base::Bind(&MediaDevicesManagerTest::EnumerateCallback,
                 base::Unretained(this), &run_loop));
  run_loop.Run();

  // Add device-change event listeners.
  MockMediaDevicesListener listener_audio_input;
  MediaDevicesManager::BoolDeviceTypes audio_input_devices_to_subscribe;
  audio_input_devices_to_subscribe[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  uint32_t audio_input_subscription_id =
      media_devices_manager_->SubscribeDeviceChangeNotifications(
          kRenderProcessId, kRenderFrameId, audio_input_devices_to_subscribe,
          listener_audio_input.CreateInterfacePtrAndBind());

  MockMediaDevicesListener listener_video_input;
  MediaDevicesManager::BoolDeviceTypes video_input_devices_to_subscribe;
  video_input_devices_to_subscribe[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  uint32_t video_input_subscription_id =
      media_devices_manager_->SubscribeDeviceChangeNotifications(
          kRenderProcessId, kRenderFrameId, video_input_devices_to_subscribe,
          listener_video_input.CreateInterfacePtrAndBind());

  MockMediaDevicesListener listener_audio_output;
  MediaDevicesManager::BoolDeviceTypes audio_output_devices_to_subscribe;
  audio_output_devices_to_subscribe[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  uint32_t audio_output_subscription_id =
      media_devices_manager_->SubscribeDeviceChangeNotifications(
          kRenderProcessId, kRenderFrameId, audio_output_devices_to_subscribe,
          listener_audio_output.CreateInterfacePtrAndBind());

  MockMediaDevicesListener listener_all;
  MediaDevicesManager::BoolDeviceTypes all_devices_to_subscribe;
  all_devices_to_subscribe[MEDIA_DEVICE_TYPE_AUDIO_INPUT] = true;
  all_devices_to_subscribe[MEDIA_DEVICE_TYPE_VIDEO_INPUT] = true;
  all_devices_to_subscribe[MEDIA_DEVICE_TYPE_AUDIO_OUTPUT] = true;
  media_devices_manager_->SubscribeDeviceChangeNotifications(
      kRenderProcessId, kRenderFrameId, all_devices_to_subscribe,
      listener_all.CreateInterfacePtrAndBind());

  MediaDeviceInfoArray notification_audio_input;
  MediaDeviceInfoArray notification_video_input;
  MediaDeviceInfoArray notification_audio_output;
  MediaDeviceInfoArray notification_all_audio_input;
  MediaDeviceInfoArray notification_all_video_input;
  MediaDeviceInfoArray notification_all_audio_output;
  EXPECT_CALL(listener_audio_input,
              OnDevicesChanged(MEDIA_DEVICE_TYPE_AUDIO_INPUT, _))
      .Times(1)
      .WillOnce(SaveArg<1>(&notification_audio_input));
  EXPECT_CALL(listener_video_input,
              OnDevicesChanged(MEDIA_DEVICE_TYPE_VIDEO_INPUT, _))
      .Times(1)
      .WillOnce(SaveArg<1>(&notification_video_input));
  EXPECT_CALL(listener_audio_output,
              OnDevicesChanged(MEDIA_DEVICE_TYPE_AUDIO_OUTPUT, _))
      .Times(1)
      .WillOnce(SaveArg<1>(&notification_audio_output));
  EXPECT_CALL(listener_all, OnDevicesChanged(MEDIA_DEVICE_TYPE_AUDIO_INPUT, _))
      .Times(2)
      .WillRepeatedly(SaveArg<1>(&notification_all_audio_input));
  EXPECT_CALL(listener_all, OnDevicesChanged(MEDIA_DEVICE_TYPE_VIDEO_INPUT, _))
      .Times(2)
      .WillRepeatedly(SaveArg<1>(&notification_all_video_input));
  EXPECT_CALL(listener_all, OnDevicesChanged(MEDIA_DEVICE_TYPE_AUDIO_OUTPUT, _))
      .Times(2)
      .WillRepeatedly(SaveArg<1>(&notification_all_audio_output));

  // Simulate device changes.
  num_audio_input_devices = 3;
  num_video_input_devices = 2;
  num_audio_output_devices = 4;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);
  media_devices_manager_->OnDevicesChanged(base::SystemMonitor::DEVTYPE_AUDIO);
  media_devices_manager_->OnDevicesChanged(
      base::SystemMonitor::DEVTYPE_VIDEO_CAPTURE);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(num_audio_input_devices, notification_audio_input.size());
  EXPECT_EQ(num_video_input_devices, notification_video_input.size());
  EXPECT_EQ(num_audio_output_devices, notification_audio_output.size());
  EXPECT_EQ(num_audio_input_devices, notification_all_audio_input.size());
  EXPECT_EQ(num_video_input_devices, notification_all_video_input.size());
  EXPECT_EQ(num_audio_output_devices, notification_all_audio_output.size());
  VerifyDeviceAndGroupID(
      {notification_audio_input, notification_video_input,
       notification_audio_output, notification_all_audio_input,
       notification_all_video_input, notification_all_audio_output});

  media_devices_manager_->UnsubscribeDeviceChangeNotifications(
      audio_input_subscription_id);
  media_devices_manager_->UnsubscribeDeviceChangeNotifications(
      video_input_subscription_id);
  media_devices_manager_->UnsubscribeDeviceChangeNotifications(
      audio_output_subscription_id);

  // Simulate further device changes. Only the objects still subscribed to the
  // device-change events will receive notifications.
  num_audio_input_devices = 2;
  num_video_input_devices = 1;
  num_audio_output_devices = 3;
  audio_manager_->SetNumAudioInputDevices(num_audio_input_devices);
  video_capture_device_factory_->SetToDefaultDevicesConfig(
      num_video_input_devices);
  audio_manager_->SetNumAudioOutputDevices(num_audio_output_devices);
  media_devices_manager_->OnDevicesChanged(base::SystemMonitor::DEVTYPE_AUDIO);
  media_devices_manager_->OnDevicesChanged(
      base::SystemMonitor::DEVTYPE_VIDEO_CAPTURE);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(num_audio_input_devices, notification_all_audio_input.size());
  EXPECT_EQ(num_video_input_devices, notification_all_video_input.size());
  EXPECT_EQ(num_audio_output_devices, notification_all_audio_output.size());
  VerifyDeviceAndGroupID({notification_all_audio_input,
                          notification_all_video_input,
                          notification_all_audio_output});
}

TEST_F(MediaDevicesManagerTest, GuessVideoGroupID) {
  MediaDeviceInfoArray audio_devices = {
      {media::AudioDeviceDescription::kDefaultDeviceId,
       "Default - Unique USB Mic (1234:5678)", "group_1"},
      {media::AudioDeviceDescription::kCommunicationsDeviceId,
       "communications - Unique USB Mic (1234:5678)", "group_1"},
      {"device_id_1", "Microphone (Logitech Webcam C930e)", "group_1"},
      {"device_id_2", "HD Pro Webcam C920", "group_2"},
      {"device_id_3", "Microsoft® LifeCam Cinema(TM)", "group_3"},
      {"device_id_4", "Repeated webcam", "group_4"},
      {"device_id_5", "Repeated webcam", "group_5"},
      {"device_id_6", "Dual-mic webcam device", "group_6"},
      {"device_id_7", "Dual-mic webcam device", "group_6"},
      {"device_id_8", "Repeated dual-mic webcam device", "group_7"},
      {"device_id_9", "Repeated dual-mic webcam device", "group_7"},
      {"device_id_10", "Repeated dual-mic webcam device", "group_8"},
      {"device_id_11", "Repeated dual-mic webcam device", "group_8"},
      {"device_id_12", "Unique USB Mic (1234:5678)", "group_1"},
      {"device_id_13", "Another Unique USB Mic (5678:9abc)", "group_9"},
      {"device_id_14", "Repeated USB Mic (8765:4321)", "group_10"},
      {"device_id_15", "Repeated USB Mic (8765:4321)", "group_11"},
      // Extra entry for communications device added here just to make sure
      // that it is not incorrectly detected as an extra real device. Real
      // enumerations contain only the first entry for the communications
      // device.
      {media::AudioDeviceDescription::kCommunicationsDeviceId,
       "communications - Unique USB Mic (1234:5678)", "group_1"},
  };

  MediaDeviceInfo logitech_video("logitech_video",
                                 "Logitech Webcam C930e (046d:0843)", "");
  MediaDeviceInfo hd_pro_video("hd_pro_video", "HD Pro Webcam C920 (046d:082d)",
                               "");
  MediaDeviceInfo lifecam_video(
      "lifecam_video", "Microsoft® LifeCam Cinema(TM) (045e:075d)", "");
  MediaDeviceInfo repeated_webcam1_video("repeated_webcam_1", "Repeated webcam",
                                         "");
  MediaDeviceInfo repeated_webcam2_video("repeated_webcam_2", "Repeated webcam",
                                         "");
  MediaDeviceInfo dual_mic_video("dual_mic_video",
                                 "Dual-mic webcam device (1111:1111)", "");
  MediaDeviceInfo webcam_only_video("webcam_only_video", "Webcam-only device",
                                    "");
  MediaDeviceInfo repeated_dual_mic1_video(
      "repeated_dual_mic1_video", "Repeated dual-mic webcam device", "");
  MediaDeviceInfo repeated_dual_mic2_video(
      "repeated_dual_mic2_video", "Repeated dual-mic webcam device", "");
  MediaDeviceInfo short_label_video("short_label_video", " ()", "");
  MediaDeviceInfo unique_usb_video("unique_usb_video",
                                   "Unique USB webcam (1234:5678)", "");
  MediaDeviceInfo another_unique_usb_video(
      "another_unique_usb_video", "Another Unique USB webcam (5678:9abc)", "");
  MediaDeviceInfo repeated_usb1_video("repeated_usb1_video",
                                      "Repeated USB webcam (8765:4321)", "");
  MediaDeviceInfo repeated_usb2_video("repeated_usb2_video",
                                      "Repeated USB webcam (8765:4321)", "");

  EXPECT_EQ(GuessVideoGroupID(audio_devices, logitech_video), "group_1");
  EXPECT_EQ(GuessVideoGroupID(audio_devices, hd_pro_video), "group_2");
  EXPECT_EQ(GuessVideoGroupID(audio_devices, lifecam_video), "group_3");
  EXPECT_EQ(GuessVideoGroupID(audio_devices, repeated_webcam1_video),
            repeated_webcam1_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, repeated_webcam2_video),
            repeated_webcam2_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, dual_mic_video), "group_6");
  EXPECT_EQ(GuessVideoGroupID(audio_devices, webcam_only_video),
            webcam_only_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, repeated_dual_mic1_video),
            repeated_dual_mic1_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, repeated_dual_mic2_video),
            repeated_dual_mic2_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, short_label_video),
            short_label_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, unique_usb_video), "group_1");
  EXPECT_EQ(GuessVideoGroupID(audio_devices, another_unique_usb_video),
            "group_9");
  EXPECT_EQ(GuessVideoGroupID(audio_devices, repeated_usb1_video),
            repeated_usb1_video.device_id);
  EXPECT_EQ(GuessVideoGroupID(audio_devices, repeated_usb2_video),
            repeated_usb2_video.device_id);
}

}  // namespace content
