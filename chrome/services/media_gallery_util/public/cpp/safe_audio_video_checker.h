// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_MEDIA_GALLERY_UTIL_PUBLIC_CPP_SAFE_AUDIO_VIDEO_CHECKER_H_
#define CHROME_SERVICES_MEDIA_GALLERY_UTIL_PUBLIC_CPP_SAFE_AUDIO_VIDEO_CHECKER_H_

#include "base/files/file.h"
#include "base/macros.h"
#include "chrome/services/media_gallery_util/public/cpp/media_parser_provider.h"
#include "chrome/services/media_gallery_util/public/mojom/media_parser.mojom.h"

namespace service_manager {
class Connector;
}

// Uses a utility process to validate a media file.  If the callback returns
// File::FILE_OK, then file appears to be valid.  File validation does not
// attempt to decode the entire file since that could take a considerable
// amount of time.
class SafeAudioVideoChecker : public MediaParserProvider {
 public:
  using ResultCallback = base::OnceCallback<void(base::File::Error result)>;

  // Takes responsibility for closing |file|.
  SafeAudioVideoChecker(base::File file,
                        ResultCallback callback,
                        std::unique_ptr<service_manager::Connector> connector);
  ~SafeAudioVideoChecker() override;

  // Checks the file. Can be called on a different thread than the UI thread.
  // Note that the callback specified in the construtor will be called on the
  // thread this method is called.
  void Start();

 private:
  // MediaParserProvider implementation:
  void OnMediaParserCreated() override;
  void OnConnectionError() override;

  // Media file check result.
  void CheckMediaFileDone(bool valid);

  // Media file to check.
  base::File file_;

  // Connector to the ServiceManager used to ind the MediaParser interface.
  std::unique_ptr<service_manager::Connector> connector_;

  // Report the check result to |callback_|.
  ResultCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(SafeAudioVideoChecker);
};

#endif  // CHROME_SERVICES_MEDIA_GALLERY_UTIL_PUBLIC_CPP_SAFE_AUDIO_VIDEO_CHECKER_H_
