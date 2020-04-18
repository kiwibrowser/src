// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_TRACE_UPLOADER_H_
#define CONTENT_PUBLIC_BROWSER_TRACE_UPLOADER_H_

#include <stdint.h>

#include <memory>

#include "base/callback.h"
#include "base/values.h"

namespace content {

// Used to implement a trace upload service for use in about://tracing,
// which gets requested through the TracingDelegate.
class TraceUploader {
 public:
  // This should be called when the tracing is complete.
  // The bool denotes success or failure, the string is feedback
  // to show in the Tracing UI.
  typedef base::OnceCallback<void(bool, const std::string&)> UploadDoneCallback;
  // Call this to update the progress UI with the current bytes uploaded,
  // as well as the total.
  typedef base::RepeatingCallback<void(int64_t, int64_t)>
      UploadProgressCallback;

  virtual ~TraceUploader() {}

  enum UploadMode { COMPRESSED_UPLOAD, UNCOMPRESSED_UPLOAD };

  // Compresses and uploads the given file contents.
  virtual void DoUpload(const std::string& file_contents,
                        UploadMode upload_mode,
                        std::unique_ptr<const base::DictionaryValue> metadata,
                        const UploadProgressCallback& progress_callback,
                        UploadDoneCallback done_callback) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_TRACE_UPLOADER_H_
