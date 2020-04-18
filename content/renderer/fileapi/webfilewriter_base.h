// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FILEAPI_WEBFILEWRITER_BASE_H_
#define CONTENT_RENDERER_FILEAPI_WEBFILEWRITER_BASE_H_

#include <stdint.h>

#include "base/files/file.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_file_writer.h"
#include "url/gurl.h"

namespace blink {
class WebFileWriterClient;
}

namespace content {

class CONTENT_EXPORT WebFileWriterBase : public blink::WebFileWriter {
 public:
  WebFileWriterBase(const GURL& path, blink::WebFileWriterClient* client);
  ~WebFileWriterBase() override;

  // WebFileWriter implementation
  void Truncate(long long length) override;
  void Write(long long position, const blink::WebString& id) override;
  void Cancel() override;

 protected:
  // This calls DidSucceed() or DidFail() based on the value of |error_code|.
  void DidFinish(base::File::Error error_code);

  void DidWrite(int64_t bytes, bool complete);
  void DidSucceed();
  void DidFail(base::File::Error error_code);

  // Derived classes must provide these methods to asynchronously perform
  // the requested operation, and they must call the appropiate DidSomething
  // method upon completion and as progress is made in the Write case.
  virtual void DoTruncate(const GURL& path, int64_t offset) = 0;
  virtual void DoWrite(const GURL& path,
                       const std::string& blob_id,
                       int64_t offset) = 0;
  virtual void DoCancel() = 0;

 private:
  enum OperationType {
    kOperationNone,
    kOperationWrite,
    kOperationTruncate
  };

  enum CancelState {
    kCancelNotInProgress,
    kCancelSent,
    kCancelReceivedWriteResponse,
  };

  void FinishCancel();

  GURL path_;
  blink::WebFileWriterClient* client_;
  OperationType operation_;
  CancelState cancel_state_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_FILEAPI_WEBFILEWRITER_BASE_H_
