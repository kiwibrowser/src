// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_LOG_UPLOADER_INTERFACE_H_
#define COMPONENTS_RAPPOR_LOG_UPLOADER_INTERFACE_H_

namespace rappor {

class LogUploaderInterface {
 public:
  LogUploaderInterface() {};
  virtual ~LogUploaderInterface() {};

  // Begin uploading logs.
  virtual void Start() = 0;

  // Stops uploading logs.
  virtual void Stop() = 0;

  // Adds an entry to the queue of logs to be uploaded to the server.  The
  // uploader makes no assumptions about the format of |log| and simply sends
  // it verbatim to the server.
  virtual void QueueLog(const std::string& log) = 0;
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_LOG_UPLOADER_INTERFACE_H_
