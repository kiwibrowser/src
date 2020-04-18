// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/content/background_loader/background_loader_contents.h"

#include "base/synchronization/waitable_event.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace background_loader {

class BackgroundLoaderContentsTest : public testing::Test,
                                     public BackgroundLoaderContents::Delegate {
 public:
  BackgroundLoaderContentsTest();
  ~BackgroundLoaderContentsTest() override;

  void SetUp() override;
  void TearDown() override;

  void CanDownload(const base::Callback<void(bool)>& callback) override;

  BackgroundLoaderContents* contents() { return contents_.get(); }

  void DownloadCallback(bool download);
  // Sets "this" as delegate to the background loader contents.
  void SetDelegate();

  bool download() { return download_; }
  bool can_download_delegate_called() { return delegate_called_; }

  void MediaAccessCallback(const content::MediaStreamDevices& devices,
                           content::MediaStreamRequestResult result,
                           std::unique_ptr<content::MediaStreamUI> ui);
  content::MediaStreamDevices devices() { return devices_; }
  content::MediaStreamRequestResult request_result() { return request_result_; }
  content::MediaStreamUI* media_stream_ui() { return media_stream_ui_.get(); }

  void WaitForSignal() { waiter_.Wait(); }

 private:
  std::unique_ptr<BackgroundLoaderContents> contents_;
  bool download_;
  bool delegate_called_;
  content::MediaStreamDevices devices_;
  content::MediaStreamRequestResult request_result_;
  std::unique_ptr<content::MediaStreamUI> media_stream_ui_;
  base::WaitableEvent waiter_;
};

BackgroundLoaderContentsTest::BackgroundLoaderContentsTest()
    : download_(false),
      delegate_called_(false),
      waiter_(base::WaitableEvent::ResetPolicy::MANUAL,
              base::WaitableEvent::InitialState::NOT_SIGNALED){};

BackgroundLoaderContentsTest::~BackgroundLoaderContentsTest(){};

void BackgroundLoaderContentsTest::SetUp() {
  contents_.reset(new BackgroundLoaderContents());
  download_ = false;
  waiter_.Reset();
}

void BackgroundLoaderContentsTest::TearDown() {
  contents_.reset();
}

void BackgroundLoaderContentsTest::CanDownload(
    const base::Callback<void(bool)>& callback) {
  delegate_called_ = true;
  callback.Run(true);
}

void BackgroundLoaderContentsTest::DownloadCallback(bool download) {
  download_ = download;
  waiter_.Signal();
}

void BackgroundLoaderContentsTest::SetDelegate() {
  contents_->SetDelegate(this);
}

void BackgroundLoaderContentsTest::MediaAccessCallback(
    const content::MediaStreamDevices& devices,
    content::MediaStreamRequestResult result,
    std::unique_ptr<content::MediaStreamUI> ui) {
  devices_ = devices;
  request_result_ = result;
  media_stream_ui_.reset(ui.get());
  waiter_.Signal();
}

TEST_F(BackgroundLoaderContentsTest, NotVisible) {
  ASSERT_TRUE(contents()->IsNeverVisible(nullptr));
}

TEST_F(BackgroundLoaderContentsTest, SuppressDialogs) {
  ASSERT_TRUE(contents()->ShouldSuppressDialogs(nullptr));
}

TEST_F(BackgroundLoaderContentsTest, DoesNotFocusAfterCrash) {
  ASSERT_FALSE(contents()->ShouldFocusPageAfterCrash());
}

TEST_F(BackgroundLoaderContentsTest, CannotDownloadNoDelegate) {
  contents()->CanDownload(
      GURL::EmptyGURL(), std::string(),
      base::Bind(&BackgroundLoaderContentsTest::DownloadCallback,
                 base::Unretained(this)));
  WaitForSignal();
  ASSERT_FALSE(download());
  ASSERT_FALSE(can_download_delegate_called());
}

TEST_F(BackgroundLoaderContentsTest, CanDownload_DelegateCalledWhenSet) {
  SetDelegate();
  contents()->CanDownload(
      GURL::EmptyGURL(), std::string(),
      base::Bind(&BackgroundLoaderContentsTest::DownloadCallback,
                 base::Unretained(this)));
  WaitForSignal();
  ASSERT_TRUE(download());
  ASSERT_TRUE(can_download_delegate_called());
}

TEST_F(BackgroundLoaderContentsTest, ShouldNotCreateWebContents) {
  ASSERT_FALSE(contents()->ShouldCreateWebContents(
      nullptr /* contents */, nullptr /* opener */,
      nullptr /* source_site_instance */, 0 /* route_id */,
      0 /* main_frame_route_id */, 0 /* main_frame_widget_route_id */,
      content::mojom::WindowContainerType::NORMAL /* window_container_type */,
      GURL() /* opener_url */, "foo" /* frame_name */,
      GURL::EmptyGURL() /* target_url */, "bar" /* partition_id */,
      nullptr /* session_storage_namespace */));
}

TEST_F(BackgroundLoaderContentsTest, ShouldNotAddNewContents) {
  bool blocked;
  contents()->AddNewContents(
      nullptr /* source */,
      std::unique_ptr<content::WebContents>() /* new_contents */,
      WindowOpenDisposition::CURRENT_TAB /* disposition */,
      gfx::Rect() /* initial_rect */, false /* user_gesture */,
      &blocked /* was_blocked */);
  ASSERT_TRUE(blocked);
}

TEST_F(BackgroundLoaderContentsTest, DoesNotGiveMediaAccessPermission) {
  content::MediaStreamRequest request(
      0 /* render_process_id */, 0 /* render_frame_id */,
      0 /* page_request_id */, GURL::EmptyGURL() /* security_origin */,
      false /* user_gesture */,
      content::MediaStreamRequestType::MEDIA_DEVICE_ACCESS /* request_type */,
      std::string() /* requested_audio_device_id */,
      std::string() /* requested_video_device_id */,
      content::MediaStreamType::MEDIA_TAB_AUDIO_CAPTURE /* audio_type */,
      content::MediaStreamType::MEDIA_TAB_VIDEO_CAPTURE /* video_type */,
      false /* disable_local_echo */);
  contents()->RequestMediaAccessPermission(
      nullptr /* contents */, request /* request */,
      base::Bind(&BackgroundLoaderContentsTest::MediaAccessCallback,
                 base::Unretained(this)));
  WaitForSignal();
  // No devices allowed.
  ASSERT_TRUE(devices().empty());
  // Permission has been dismissed rather than denied.
  ASSERT_EQ(
      content::MediaStreamRequestResult::MEDIA_DEVICE_PERMISSION_DISMISSED,
      request_result());
  ASSERT_EQ(nullptr, media_stream_ui());
}

TEST_F(BackgroundLoaderContentsTest, CheckMediaAccessPermissionFalse) {
  ASSERT_FALSE(contents()->CheckMediaAccessPermission(
      nullptr /* contents */, GURL::EmptyGURL() /* security_origin */,
      content::MediaStreamType::MEDIA_TAB_VIDEO_CAPTURE /* type */));
}

TEST_F(BackgroundLoaderContentsTest, AdjustPreviewsState) {
  content::PreviewsState previews_state;

  // If the state starts out as off or disabled, it should stay that way.
  previews_state = content::PREVIEWS_OFF;
  contents()->AdjustPreviewsStateForNavigation(nullptr, &previews_state);
  EXPECT_EQ(previews_state, content::PREVIEWS_OFF);
  previews_state = content::PREVIEWS_NO_TRANSFORM;
  contents()->AdjustPreviewsStateForNavigation(nullptr, &previews_state);
  EXPECT_EQ(previews_state, content::PREVIEWS_NO_TRANSFORM);

  // If the state starts out as a state unfriendly to offlining, we should
  // and out the unfriendly previews.
  previews_state = content::SERVER_LOFI_ON | content::CLIENT_LOFI_ON;
  contents()->AdjustPreviewsStateForNavigation(nullptr, &previews_state);
  EXPECT_EQ(previews_state, content::SERVER_LOFI_ON);

  // If the state starts out as offlining friendly previews, we should preserve
  // them.
  previews_state = content::PARTIAL_CONTENT_SAFE_PREVIEWS;
  contents()->AdjustPreviewsStateForNavigation(nullptr, &previews_state);
  EXPECT_EQ(previews_state, content::PARTIAL_CONTENT_SAFE_PREVIEWS);

  // If there are only offlining unfriendly previews, they should all get turned
  // off.
  previews_state = content::CLIENT_LOFI_ON;
  contents()->AdjustPreviewsStateForNavigation(nullptr, &previews_state);
  EXPECT_EQ(previews_state, content::PREVIEWS_OFF);
}

}  // namespace background_loader
