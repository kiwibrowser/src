// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_REGISTER_OBSERVER_H_
#define CONTENT_BROWSER_STREAMS_STREAM_REGISTER_OBSERVER_H_

#include "content/common/content_export.h"

namespace content {

class Stream;

class CONTENT_EXPORT StreamRegisterObserver {
 public:
  // Sent when the stream is registered.
  virtual void OnStreamRegistered(Stream* stream) = 0;

 protected:
  virtual ~StreamRegisterObserver() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_REGISTER_OBSERVER_H_
