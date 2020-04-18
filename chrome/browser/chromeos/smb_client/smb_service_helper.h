// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_SERVICE_HELPER_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_SERVICE_HELPER_H_

#include <string>
#include <vector>

#include <base/logging.h>

namespace chromeos {
namespace smb_client {

bool ParseUserPrincipalName(const std::string& user_principal_name,
                            std::string* user_name,
                            std::string* workgroup);

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_SERVICE_HELPER_H_
