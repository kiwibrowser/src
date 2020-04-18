// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_STREAM_INFO_H_
#define CONTENT_PUBLIC_BROWSER_STREAM_INFO_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace net {
class HttpResponseHeaders;
}

namespace content {

class StreamHandle;

// A convenience structure for passing around both a StreamHandle and associated
// metadata. The intent is so, when passing the stream's URL to a child process,
// the handle can be retained as long as it is needed while the rest of the
// StreamInfo is released.
struct CONTENT_EXPORT StreamInfo {
  StreamInfo();
  ~StreamInfo();

  // The handle to the stream itself.
  std::unique_ptr<StreamHandle> handle;

  // The original URL being redirected to this stream.
  GURL original_url;

  // The MIME type associated with this stream.
  std::string mime_type;

  // The HTTP response headers associated with this stream.
  scoped_refptr<net::HttpResponseHeaders> response_headers;

  DISALLOW_COPY_AND_ASSIGN(StreamInfo);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_STREAM_INFO_H_
