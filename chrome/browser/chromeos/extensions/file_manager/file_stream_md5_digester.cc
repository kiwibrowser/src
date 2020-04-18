// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/file_stream_md5_digester.h"

#include <utility>

#include "base/callback.h"
#include "net/base/net_errors.h"
#include "storage/browser/fileapi/file_stream_reader.h"

namespace drive {
namespace util {

namespace {

const int kMd5DigestBufferSize = 512 * 1024;  // 512 kB.

}  // namespace

FileStreamMd5Digester::FileStreamMd5Digester()
    : buffer_(new net::IOBuffer(kMd5DigestBufferSize)) {}

FileStreamMd5Digester::~FileStreamMd5Digester() {}

void FileStreamMd5Digester::GetMd5Digest(
    std::unique_ptr<storage::FileStreamReader> stream_reader,
    const ResultCallback& callback) {
  reader_ = std::move(stream_reader);
  base::MD5Init(&md5_context_);

  // Start the read/hash.
  ReadNextChunk(callback);
}

void FileStreamMd5Digester::ReadNextChunk(const ResultCallback& callback) {
  const int result =
      reader_->Read(buffer_.get(), kMd5DigestBufferSize,
                    base::Bind(&FileStreamMd5Digester::OnChunkRead,
                               base::Unretained(this), callback));
  if (result != net::ERR_IO_PENDING)
    OnChunkRead(callback, result);
}

void FileStreamMd5Digester::OnChunkRead(const ResultCallback& callback,
                                        int bytes_read) {
  if (bytes_read < 0) {
    // Error - just return empty string.
    callback.Run("");
    return;
  } else if (bytes_read == 0) {
    // EOF.
    base::MD5Digest digest;
    base::MD5Final(&digest, &md5_context_);
    std::string result = base::MD5DigestToBase16(digest);
    callback.Run(result);
    return;
  }

  // Read data and digest it.
  base::MD5Update(&md5_context_,
                  base::StringPiece(buffer_->data(), bytes_read));

  // Kick off the next read.
  ReadNextChunk(callback);
}

}  // namespace util
}  // namespace drive
