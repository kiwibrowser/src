// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/printing/uri_components.h"

#include "url/third_party/mozilla/url_parse.h"

namespace chromeos {

UriComponents::UriComponents(bool encrypted,
                             const std::string& scheme,
                             const std::string& host,
                             int port,
                             const std::string& path)
    : encrypted_(encrypted),
      scheme_(scheme),
      host_(host),
      port_(port),
      path_(path) {}

UriComponents::UriComponents(const UriComponents&) = default;

}  // namespace chromeos
