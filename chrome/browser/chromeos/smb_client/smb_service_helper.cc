// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service_helper.h"

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace chromeos {
namespace smb_client {

bool ParseUserPrincipalName(const std::string& user_principal_name,
                            std::string* user_name,
                            std::string* workgroup) {
  DCHECK(user_name);
  DCHECK(workgroup);
  std::vector<std::string> parts = base::SplitString(
      user_principal_name, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() != 2 || parts.at(0).empty() || parts.at(1).empty()) {
    // Don't log user_principal_name, it might contain sensitive data.
    LOG(ERROR) << "Failed to parse user principal name. Expected form "
                  "'user@some.realm'.";
    return false;
  }
  *user_name = std::move(parts.at(0));
  *workgroup = base::ToUpperASCII(std::move(parts.at(1)));
  return true;
}

}  // namespace smb_client
}  // namespace chromeos
