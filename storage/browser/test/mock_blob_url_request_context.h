// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_MOCK_BLOB_URL_REQUEST_CONTEXT_H_
#define CONTENT_PUBLIC_TEST_MOCK_BLOB_URL_REQUEST_CONTEXT_H_

#include "base/macros.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_factory_impl.h"

namespace storage {
class BlobDataHandle;
class BlobStorageContext;
}

namespace content {

class MockBlobURLRequestContext : public net::URLRequestContext {
 public:
  MockBlobURLRequestContext();
  ~MockBlobURLRequestContext() override;

  storage::BlobStorageContext* blob_storage_context() const {
    return blob_storage_context_.get();
  }

 private:
  net::URLRequestJobFactoryImpl job_factory_;
  std::unique_ptr<storage::BlobStorageContext> blob_storage_context_;

  DISALLOW_COPY_AND_ASSIGN(MockBlobURLRequestContext);
};

class ScopedTextBlob {
 public:
  // Registers a blob with the given |id| that contains |data|.
  ScopedTextBlob(const MockBlobURLRequestContext& request_context,
                 const std::string& blob_id,
                 const std::string& data);
  ~ScopedTextBlob();

  // Returns a BlobDataHandle referring to the scoped blob.
  std::unique_ptr<storage::BlobDataHandle> GetBlobDataHandle();

 private:
  const std::string blob_id_;
  storage::BlobStorageContext* context_;
  std::unique_ptr<storage::BlobDataHandle> handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTextBlob);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_MOCK_BLOB_URL_REQUEST_CONTEXT_H_
