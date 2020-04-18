// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/permission/media_access_permission_request.h"
#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace android_webview {

class TestMediaAccessPermissionRequest : public MediaAccessPermissionRequest {
 public:
  TestMediaAccessPermissionRequest(
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback,
      const content::MediaStreamDevices& audio_devices,
      const content::MediaStreamDevices& video_devices)
      : MediaAccessPermissionRequest(request, callback) {
    audio_test_devices_ = audio_devices;
    video_test_devices_ = video_devices;
  }
};

class MediaAccessPermissionRequestTest : public testing::Test {
 protected:
  void SetUp() override {
    audio_device_id_ = "audio";
    video_device_id_ = "video";
    first_audio_device_id_ = "audio1";
    first_video_device_id_ = "video1";
  }

  std::unique_ptr<TestMediaAccessPermissionRequest> CreateRequest(
      std::string audio_id,
      std::string video_id) {
    content::MediaStreamDevices audio_devices;
    audio_devices.push_back(content::MediaStreamDevice(
        content::MEDIA_DEVICE_AUDIO_CAPTURE, first_audio_device_id_, "a2"));
    audio_devices.push_back(content::MediaStreamDevice(
        content::MEDIA_DEVICE_AUDIO_CAPTURE, audio_device_id_, "a1"));

    content::MediaStreamDevices video_devices;
    video_devices.push_back(content::MediaStreamDevice(
        content::MEDIA_DEVICE_VIDEO_CAPTURE, first_video_device_id_, "v2"));
    video_devices.push_back(content::MediaStreamDevice(
        content::MEDIA_DEVICE_VIDEO_CAPTURE, video_device_id_, "v1"));

    GURL origin("https://www.google.com");
    content::MediaStreamRequest request(
        0, 0, 0, origin, false, content::MEDIA_GENERATE_STREAM, audio_id,
        video_id, content::MEDIA_DEVICE_AUDIO_CAPTURE,
        content::MEDIA_DEVICE_VIDEO_CAPTURE, false /* disable_local_echo */);

    std::unique_ptr<TestMediaAccessPermissionRequest> permission_request;
    permission_request.reset(new TestMediaAccessPermissionRequest(
        request,
        base::BindRepeating(&MediaAccessPermissionRequestTest::Callback,
                            base::Unretained(this)),
        audio_devices, video_devices));
    return permission_request;
  }

  std::string audio_device_id_;
  std::string video_device_id_;
  std::string first_audio_device_id_;
  std::string first_video_device_id_;
  content::MediaStreamDevices devices_;
  content::MediaStreamRequestResult result_;

 private:
  void Callback(const content::MediaStreamDevices& devices,
                content::MediaStreamRequestResult result,
                std::unique_ptr<content::MediaStreamUI> ui) {
    devices_ = devices;
    result_ = result;
  }
};

TEST_F(MediaAccessPermissionRequestTest, TestGrantPermissionRequest) {
  std::unique_ptr<TestMediaAccessPermissionRequest> request =
      CreateRequest(audio_device_id_, video_device_id_);
  request->NotifyRequestResult(true);

  EXPECT_EQ(2u, devices_.size());
  EXPECT_EQ(content::MEDIA_DEVICE_OK, result_);

  bool audio_exist = false;
  bool video_exist = false;
  for (content::MediaStreamDevices::iterator i = devices_.begin();
       i != devices_.end(); ++i) {
    if (i->type == content::MEDIA_DEVICE_AUDIO_CAPTURE &&
        i->id == audio_device_id_) {
      audio_exist = true;
    } else if (i->type == content::MEDIA_DEVICE_VIDEO_CAPTURE &&
               i->id == video_device_id_) {
      video_exist = true;
    }
  }
  EXPECT_TRUE(audio_exist);
  EXPECT_TRUE(video_exist);
}

TEST_F(MediaAccessPermissionRequestTest, TestGrantPermissionRequestWithoutID) {
  std::unique_ptr<TestMediaAccessPermissionRequest> request =
      CreateRequest(std::string(), std::string());
  request->NotifyRequestResult(true);

  EXPECT_EQ(2u, devices_.size());
  EXPECT_EQ(content::MEDIA_DEVICE_OK, result_);

  bool audio_exist = false;
  bool video_exist = false;
  for (content::MediaStreamDevices::iterator i = devices_.begin();
       i != devices_.end(); ++i) {
    if (i->type == content::MEDIA_DEVICE_AUDIO_CAPTURE &&
        i->id == first_audio_device_id_) {
      audio_exist = true;
    } else if (i->type == content::MEDIA_DEVICE_VIDEO_CAPTURE &&
               i->id == first_video_device_id_) {
      video_exist = true;
    }
  }
  EXPECT_TRUE(audio_exist);
  EXPECT_TRUE(video_exist);
}

TEST_F(MediaAccessPermissionRequestTest, TestDenyPermissionRequest) {
  std::unique_ptr<TestMediaAccessPermissionRequest> request =
      CreateRequest(std::string(), std::string());
  request->NotifyRequestResult(false);
  EXPECT_TRUE(devices_.empty());
  EXPECT_EQ(content::MEDIA_DEVICE_PERMISSION_DENIED, result_);
}

}  // namespace android_webview
