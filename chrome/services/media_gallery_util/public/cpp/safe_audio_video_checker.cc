// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/media_gallery_util/public/cpp/safe_audio_video_checker.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/time/time.h"
#include "chrome/services/media_gallery_util/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

SafeAudioVideoChecker::SafeAudioVideoChecker(
    base::File file,
    ResultCallback callback,
    std::unique_ptr<service_manager::Connector> connector)
    : file_(std::move(file)),
      connector_(std::move(connector)),
      callback_(std::move(callback)) {
  DCHECK(callback_);
}

void SafeAudioVideoChecker::Start() {
  if (!file_.IsValid()) {
    std::move(callback_).Run(base::File::FILE_ERROR_SECURITY);
    return;
  }

  RetrieveMediaParser(connector_.get());
}

void SafeAudioVideoChecker::OnMediaParserCreated() {
  static constexpr auto kFileDecodeTime =
      base::TimeDelta::FromMilliseconds(250);

  media_parser()->CheckMediaFile(
      kFileDecodeTime, std::move(file_),
      base::BindOnce(&SafeAudioVideoChecker::CheckMediaFileDone,
                     base::Unretained(this)));
}

void SafeAudioVideoChecker::OnConnectionError() {
  CheckMediaFileDone(/*valid=*/false);
}

void SafeAudioVideoChecker::CheckMediaFileDone(bool valid) {
  std::move(callback_).Run(valid ? base::File::FILE_OK
                                 : base::File::FILE_ERROR_SECURITY);
}

SafeAudioVideoChecker::~SafeAudioVideoChecker() = default;
