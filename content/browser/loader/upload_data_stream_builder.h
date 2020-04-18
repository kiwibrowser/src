// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_UPLOAD_DATA_STREAM_BUILDER_H_
#define CONTENT_BROWSER_LOADER_UPLOAD_DATA_STREAM_BUILDER_H_

#include <memory>

#include "content/common/content_export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace storage {
class FileSystemContext;
}

namespace net {
class UploadDataStream;
}

namespace network {
class ResourceRequestBody;
}

namespace storage {
class BlobStorageContext;
}

namespace content {

class CONTENT_EXPORT UploadDataStreamBuilder {
 public:
  // Creates a new UploadDataStream from this request body.
  //
  // If |body| contains any blob references, the caller is responsible for
  // making sure them outlive the returned value of UploadDataStream. We do this
  // by binding the BlobDataHandles of them to ResourceRequestBody in
  // ResourceDispatcherHostImpl::BeginRequest().
  //
  // |file_system_context| is used to create a FileStreamReader for files with
  // filesystem URLs.  |file_task_runner| is used to perform file operations
  // when the data gets uploaded.
  static std::unique_ptr<net::UploadDataStream> Build(
      network::ResourceRequestBody* body,
      storage::BlobStorageContext* blob_context,
      storage::FileSystemContext* file_system_context,
      base::SingleThreadTaskRunner* file_task_runner);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_UPLOAD_DATA_STREAM_BUILDER_H_
