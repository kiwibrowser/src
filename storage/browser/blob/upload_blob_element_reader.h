// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_UPLOAD_BLOB_ELEMENT_READER_H_
#define STORAGE_BROWSER_BLOB_UPLOAD_BLOB_ELEMENT_READER_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/base/completion_once_callback.h"
#include "net/base/upload_element_reader.h"
#include "storage/browser/storage_browser_export.h"

namespace net {
class IOBuffer;
}

namespace storage {
class BlobDataHandle;
class BlobReader;

// This class is a wrapper around the BlobReader to make it conform
// to the net::UploadElementReader interface, and it also holds around the
// handle to the blob so it stays in memory while we read it.
class STORAGE_EXPORT UploadBlobElementReader : public net::UploadElementReader {
 public:
  explicit UploadBlobElementReader(std::unique_ptr<BlobDataHandle> handle);
  ~UploadBlobElementReader() override;

  int Init(net::CompletionOnceCallback callback) override;

  uint64_t GetContentLength() const override;

  uint64_t BytesRemaining() const override;

  bool IsInMemory() const override;

  int Read(net::IOBuffer* buf,
           int buf_length,
           net::CompletionOnceCallback callback) override;

  const std::string& uuid() const;

 private:
  std::unique_ptr<BlobDataHandle> handle_;
  std::unique_ptr<BlobReader> reader_;

  DISALLOW_COPY_AND_ASSIGN(UploadBlobElementReader);
};

}  // namespace storage
#endif  // STORAGE_BROWSER_BLOB_UPLOAD_BLOB_ELEMENT_READER_H_
