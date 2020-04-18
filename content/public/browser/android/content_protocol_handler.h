// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_ANDROID_CONTENT_PROTOCOL_HANDLER_H_
#define CONTENT_PUBLIC_BROWSER_ANDROID_CONTENT_PROTOCOL_HANDLER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "net/url_request/url_request_job_factory.h"

namespace base {
class TaskRunner;
}

namespace content {

// ProtocolHandler for content scheme jobs.
class CONTENT_EXPORT ContentProtocolHandler :
    public net::URLRequestJobFactory::ProtocolHandler {
 public:
  // Creates and returns a ContentProtocolHandler instance.
  static std::unique_ptr<ContentProtocolHandler> Create(
      const scoped_refptr<base::TaskRunner>& content_task_runner);

  ~ContentProtocolHandler() override {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_ANDROID_CONTENT_PROTOCOL_HANDLER_H_
