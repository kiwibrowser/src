// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "chrome/browser/extensions/api/image_writer_private/error_messages.h"
#include "chrome/browser/extensions/api/image_writer_private/operation.h"
#include "chrome/browser/extensions/api/image_writer_private/operation_manager.h"
#include "content/public/browser/browser_thread.h"

namespace extensions {
namespace image_writer {

using content::BrowserThread;

void Operation::Write(const base::Closure& continuation) {
  DCHECK(IsRunningInCorrectSequence());
  if (IsCancelled()) {
    return;
  }

  SetStage(image_writer_api::STAGE_WRITE);
  StartUtilityClient();

  int64_t file_size;
  if (!base::GetFileSize(image_path_, &file_size)) {
    Error(error::kImageReadError);
    return;
  }

  image_writer_client_->Write(
      base::Bind(&Operation::WriteImageProgress, this, file_size),
      base::Bind(&Operation::CompleteAndContinue, this, continuation),
      base::Bind(&Operation::Error, this), image_path_, device_path_);
}

void Operation::VerifyWrite(const base::Closure& continuation) {
  DCHECK(IsRunningInCorrectSequence());

  if (IsCancelled()) {
    return;
  }

  SetStage(image_writer_api::STAGE_VERIFYWRITE);
  StartUtilityClient();

  int64_t file_size;
  if (!base::GetFileSize(image_path_, &file_size)) {
    Error(error::kImageReadError);
    return;
  }

  image_writer_client_->Verify(
      base::Bind(&Operation::WriteImageProgress, this, file_size),
      base::Bind(&Operation::CompleteAndContinue, this, continuation),
      base::Bind(&Operation::Error, this), image_path_, device_path_);
}

}  // namespace image_writer
}  // namespace extensions
