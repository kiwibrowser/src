// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_permission_request.h"

#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/android_theme_resources.h"
#else
#include "chrome/app/vector_icons/vector_icons.h"
#endif

DownloadPermissionRequest::DownloadPermissionRequest(
    base::WeakPtr<DownloadRequestLimiter::TabDownloadState> host)
    : host_(host) {
  const content::WebContents* web_contents = host_->web_contents();
  DCHECK(web_contents);
  request_origin_ = web_contents->GetURL().GetOrigin();
}

DownloadPermissionRequest::~DownloadPermissionRequest() {}

PermissionRequest::IconId DownloadPermissionRequest::GetIconId() const {
#if defined(OS_ANDROID)
  return IDR_ANDROID_INFOBAR_MULTIPLE_DOWNLOADS;
#else
  return kFileDownloadIcon;
#endif
}

#if defined(OS_ANDROID)
base::string16 DownloadPermissionRequest::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_MULTI_DOWNLOAD_WARNING);
}
#endif

base::string16 DownloadPermissionRequest::GetMessageTextFragment() const {
  return l10n_util::GetStringUTF16(IDS_MULTI_DOWNLOAD_PERMISSION_FRAGMENT);
}

GURL DownloadPermissionRequest::GetOrigin() const {
  return request_origin_;
}

void DownloadPermissionRequest::PermissionGranted() {
  if (host_) {
    // This may invalidate |host_|.
    host_->Accept();
  }
}

void DownloadPermissionRequest::PermissionDenied() {
  if (host_) {
    // This may invalidate |host_|.
    host_->Cancel();
  }
}

void DownloadPermissionRequest::Cancelled() {
  if (host_) {
    // This may invalidate |host_|.
    host_->CancelOnce();
  }
}

void DownloadPermissionRequest::RequestFinished() {
  delete this;
}

PermissionRequestType DownloadPermissionRequest::GetPermissionRequestType()
    const {
  return PermissionRequestType::DOWNLOAD;
}
