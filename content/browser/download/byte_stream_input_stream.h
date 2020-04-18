// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_BYTE_STREAM_INPUT_STREAM_H_
#define CONTENT_BROWSER_DOWNLOAD_BYTE_STREAM_INPUT_STREAM_H_

#include "components/download/public/common/input_stream.h"
#include "content/common/content_export.h"

namespace content {

class ByteStreamReader;

// Download input stream backed by a ByteStreamReader.
class CONTENT_EXPORT ByteStreamInputStream : public download::InputStream {
 public:
  explicit ByteStreamInputStream(
      std::unique_ptr<ByteStreamReader> stream_reader);
  ~ByteStreamInputStream() override;

  // download::InputStream
  bool IsEmpty() override;
  void RegisterDataReadyCallback(
      const mojo::SimpleWatcher::ReadyCallback& callback) override;
  void ClearDataReadyCallback() override;
  download::InputStream::StreamState Read(scoped_refptr<net::IOBuffer>* data,
                                          size_t* length) override;
  download::DownloadInterruptReason GetCompletionStatus() override;

 private:
  // ByteStreamReader to read from.
  std::unique_ptr<ByteStreamReader, base::OnTaskRunnerDeleter> stream_reader_;

  // Status when the response completes.
  download::DownloadInterruptReason completion_status_;

  DISALLOW_COPY_AND_ASSIGN(ByteStreamInputStream);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_BYTE_STREAM_INPUT_STREAM_H_
