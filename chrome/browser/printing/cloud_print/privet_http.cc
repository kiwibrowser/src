// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_http.h"

#include <memory>
#include <utility>

#include "chrome/browser/printing/cloud_print/privet_http_impl.h"

namespace cloud_print {

// static
std::unique_ptr<PrivetV1HTTPClient> PrivetV1HTTPClient::CreateDefault(
    std::unique_ptr<PrivetHTTPClient> info_client) {
  if (!info_client)
    return nullptr;
  return std::make_unique<PrivetV1HTTPClientImpl>(std::move(info_client));
}

}  // namespace cloud_print
