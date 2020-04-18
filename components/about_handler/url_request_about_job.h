// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ABOUT_HANDLER_URL_REQUEST_ABOUT_JOB_H_
#define COMPONENTS_ABOUT_HANDLER_URL_REQUEST_ABOUT_JOB_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace about_handler {

class URLRequestAboutJob : public net::URLRequestJob {
 public:
  URLRequestAboutJob(net::URLRequest* request,
                     net::NetworkDelegate* network_delegate);

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  bool GetMimeType(std::string* mime_type) const override;

 private:
  ~URLRequestAboutJob() override;

  void StartAsync();

  base::WeakPtrFactory<URLRequestAboutJob> weak_factory_;
};

}  // namespace about_handler

#endif  // COMPONENTS_ABOUT_HANDLER_URL_REQUEST_ABOUT_JOB_H_
