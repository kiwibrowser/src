// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/external_file_protocol_handler.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/fileapi/external_file_url_request_job.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace chromeos {

ExternalFileProtocolHandler::ExternalFileProtocolHandler(void* profile_id)
    : profile_id_(profile_id) {
}

ExternalFileProtocolHandler::~ExternalFileProtocolHandler() {
}

net::URLRequestJob* ExternalFileProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DVLOG(1) << "Handling url: " << request->url().spec();
  return new ExternalFileURLRequestJob(profile_id_, request, network_delegate);
}

}  // namespace chromeos
