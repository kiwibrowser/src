// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_SHARED_WORKER_SERVICE_H_
#define CONTENT_PUBLIC_BROWSER_SHARED_WORKER_SERVICE_H_

namespace url {
class Origin;
}  // namespace url

namespace content {

// An interface for managing shared workers. These may be run in a separate
// process, since multiple renderer processes can be talking to a single shared
// worker. All the methods below can only be called on the UI thread.
class CONTENT_EXPORT SharedWorkerService {
 public:
  // Terminates the given shared worker identified by its name, the URL of
  // its main script resource, and the constructor origin. Returns true on
  // success.
  virtual bool TerminateWorker(const GURL& url,
                               const std::string& name,
                               const url::Origin& constructor_origin) = 0;

 protected:
  virtual ~SharedWorkerService() = default;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_SHARED_WORKER_SERVICE_H_
