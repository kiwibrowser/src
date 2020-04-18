// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_PRINTING_URI_COMPONENTS_H_
#define CHROMEOS_PRINTING_URI_COMPONENTS_H_

#include <string>

#include "chromeos/chromeos_export.h"
#include "url/third_party/mozilla/url_parse.h"

namespace chromeos {

class CHROMEOS_EXPORT UriComponents {
 public:
  UriComponents(bool encrypted,
                const std::string& scheme,
                const std::string& host,
                int port,
                const std::string& path);

  UriComponents(const UriComponents&);

  bool encrypted() const { return encrypted_; }
  std::string scheme() const { return scheme_; }
  std::string host() const { return host_; }
  int port() const { return port_; }
  std::string path() const { return path_; }

 private:
  const bool encrypted_;
  const std::string scheme_;
  const std::string host_;
  const int port_;
  const std::string path_;
};

}  // namespace chromeos

#endif  // CHROMEOS_PRINTING_URI_COMPONENTS_H_
