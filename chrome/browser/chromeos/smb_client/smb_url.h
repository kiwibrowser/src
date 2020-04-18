// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_URL_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_URL_H_

#include <string>

#include "base/macros.h"
#include "url/gurl.h"

namespace chromeos {
namespace smb_client {

// Represents an SMB URL.
// This class stores a URL using GURL to a share and can contain either an
// resolved or unresolved host. The host can be replaced when the address is
// resolved by using ReplaceHost(). The passed URL must start with either
// "smb://" or "\\" when initializing with InitializeWithUrl.
class SmbUrl {
 public:
  // Initializes an empty and invalid SmbUrl.
  SmbUrl();
  ~SmbUrl();

  // Initializes SmbUrl and saves |url|. |url| must start with
  // either "smb://" or "\\". Returns true if |url| is valid.
  bool InitializeWithUrl(const std::string& url) WARN_UNUSED_RESULT;

  // Returns the host of the URL which can be resolved or unresolved.
  std::string GetHost() const;

  // Returns the full URL.
  const std::string& ToString() const;

  // Replaces the host to |new_host| and returns the full URL. Does not
  // change the original URL.
  std::string ReplaceHost(const std::string& new_host) const;

  // Returns true if the passed URL is valid and was properly parsed.
  bool IsValid() const;

 private:
  GURL url_;

  DISALLOW_COPY_AND_ASSIGN(SmbUrl);
};

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_URL_H_
