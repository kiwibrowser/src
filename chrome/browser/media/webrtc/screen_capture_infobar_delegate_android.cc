// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/screen_capture_infobar_delegate_android.h"

#include "base/callback_helpers.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/media/webrtc/desktop_streams_registry.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/media_stream_request.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_types.h"
#include "ui/base/l10n/l10n_util.h"

// static
void ScreenCaptureInfoBarDelegateAndroid::Create(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);

  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new ScreenCaptureInfoBarDelegateAndroid(web_contents, request,
                                                  callback))));
}

ScreenCaptureInfoBarDelegateAndroid::ScreenCaptureInfoBarDelegateAndroid(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : web_contents_(web_contents), request_(request), callback_(callback) {
  DCHECK_EQ(content::MEDIA_DESKTOP_VIDEO_CAPTURE, request.video_type);
}

ScreenCaptureInfoBarDelegateAndroid::~ScreenCaptureInfoBarDelegateAndroid() {
  if (!callback_.is_null()) {
    callback_.Run(content::MediaStreamDevices(),
                  content::MEDIA_DEVICE_FAILED_DUE_TO_SHUTDOWN, nullptr);
  }
}

infobars::InfoBarDelegate::InfoBarIdentifier
ScreenCaptureInfoBarDelegateAndroid::GetIdentifier() const {
  return SCREEN_CAPTURE_INFOBAR_DELEGATE_ANDROID;
}

base::string16 ScreenCaptureInfoBarDelegateAndroid::GetMessageText() const {
  return l10n_util::GetStringFUTF16(
      IDS_MEDIA_CAPTURE_SCREEN_INFOBAR_TEXT,
      url_formatter::FormatUrlForSecurityDisplay(request_.security_origin));
}

int ScreenCaptureInfoBarDelegateAndroid::GetIconId() const {
  return IDR_ANDROID_INFOBAR_MEDIA_STREAM_SCREEN;
}

base::string16 ScreenCaptureInfoBarDelegateAndroid::GetButtonLabel(
    InfoBarButton button) const {
  return l10n_util::GetStringUTF16((button == BUTTON_OK) ? IDS_PERMISSION_ALLOW
                                                         : IDS_PERMISSION_DENY);
}

bool ScreenCaptureInfoBarDelegateAndroid::Accept() {
  RunCallback(content::MEDIA_DEVICE_OK);
  return true;
}

bool ScreenCaptureInfoBarDelegateAndroid::Cancel() {
  RunCallback(content::MEDIA_DEVICE_PERMISSION_DENIED);
  return true;
}

void ScreenCaptureInfoBarDelegateAndroid::InfoBarDismissed() {
  RunCallback(content::MEDIA_DEVICE_PERMISSION_DISMISSED);
}

void ScreenCaptureInfoBarDelegateAndroid::RunCallback(
    content::MediaStreamRequestResult result) {
  DCHECK(!callback_.is_null());

  content::MediaStreamDevices devices;
  std::unique_ptr<content::MediaStreamUI> ui;
  if (result == content::MEDIA_DEVICE_OK) {
    content::DesktopMediaID screen_id = content::DesktopMediaID(
        content::DesktopMediaID::TYPE_SCREEN, webrtc::kFullDesktopScreenId);
    devices.push_back(content::MediaStreamDevice(
        content::MEDIA_DESKTOP_VIDEO_CAPTURE, screen_id.ToString(), "Screen"));

    ui = MediaCaptureDevicesDispatcher::GetInstance()
             ->GetMediaStreamCaptureIndicator()
             ->RegisterMediaStream(web_contents_, devices);
  }

  base::ResetAndReturn(&callback_).Run(devices, result, std::move(ui));
}
