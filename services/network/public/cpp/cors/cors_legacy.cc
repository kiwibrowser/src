// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/cors/cors_legacy.h"

#include <algorithm>
#include <string>
#include <vector>

#include "url/gurl.h"
#include "url/url_util.h"

namespace {

std::vector<std::string>* secure_origins = nullptr;

}  // namespace

namespace network {

namespace cors {

namespace legacy {

void RegisterSecureOrigins(const std::vector<std::string>& origins) {
  delete secure_origins;
  secure_origins = new std::vector<std::string>(origins.size());
  std::copy(origins.begin(), origins.end(), secure_origins->begin());
}

const std::vector<std::string>& GetSecureOrigins() {
  if (!secure_origins)
    secure_origins = new std::vector<std::string>;
  return *secure_origins;
}

}  // namespace legacy

}  // namespace cors

}  // namespace network
