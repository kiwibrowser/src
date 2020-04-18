// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_WRITE_OBSERVER_H_
#define CONTENT_BROWSER_STREAMS_STREAM_WRITE_OBSERVER_H_

namespace content {

class Stream;

class StreamWriteObserver {
 public:
  // Sent when space becomes available in the stream, and the source should
  // resume writing.
  virtual void OnSpaceAvailable(Stream* stream) = 0;

  // Sent when the stream is closed, and the writer should stop sending data.
  virtual void OnClose(Stream* stream) = 0;

 protected:
  virtual ~StreamWriteObserver() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_WRITE_OBSERVER_H_
