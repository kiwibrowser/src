// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_READ_OBSERVER_H_
#define CONTENT_BROWSER_STREAMS_STREAM_READ_OBSERVER_H_

#include "content/common/content_export.h"

namespace content {

class Stream;

class CONTENT_EXPORT StreamReadObserver {
 public:
  // Sent when there is data available to be read from the stream.
  virtual void OnDataAvailable(Stream* stream) = 0;

 protected:
  virtual ~StreamReadObserver() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_READ_OBSERVER_H_

