// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_DATA_PIPE_TO_SOURCE_STREAM_H_
#define CONTENT_BROWSER_LOADER_DATA_PIPE_TO_SOURCE_STREAM_H_

#include "content/common/content_export.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/filter/source_stream.h"

namespace content {

class CONTENT_EXPORT DataPipeToSourceStream final : public net::SourceStream {
 public:
  explicit DataPipeToSourceStream(mojo::ScopedDataPipeConsumerHandle body);
  ~DataPipeToSourceStream() override;

  int Read(net::IOBuffer* buf,
           int buf_size,
           const net::CompletionCallback& callback) override;
  std::string Description() const override;

 private:
  void OnReadable(MojoResult result);
  void FinishReading();

  mojo::ScopedDataPipeConsumerHandle body_;
  mojo::SimpleWatcher handle_watcher_;

  bool inside_read_ = false;

  scoped_refptr<net::IOBuffer> output_buf_;
  int output_buf_size_ = 0;
  net::CompletionCallback pending_callback_;

  DISALLOW_COPY_AND_ASSIGN(DataPipeToSourceStream);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_DATA_PIPE_TO_SOURCE_STREAM_H_
