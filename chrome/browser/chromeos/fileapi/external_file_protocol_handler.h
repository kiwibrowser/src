// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_EXTERNAL_FILE_PROTOCOL_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_EXTERNAL_FILE_PROTOCOL_HANDLER_H_

#include "base/macros.h"
#include "net/url_request/url_request_job_factory.h"

namespace chromeos {

class ExternalFileProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  explicit ExternalFileProtocolHandler(void* profile_id);
  ~ExternalFileProtocolHandler() override;

  // Creates URLRequestJobs for drive:// URLs.
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  // The profile for processing Drive accesses. Should not be NULL.
  void* profile_id_;

  DISALLOW_COPY_AND_ASSIGN(ExternalFileProtocolHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_EXTERNAL_FILE_PROTOCOL_HANDLER_H_
