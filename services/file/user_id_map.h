// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILE_USER_ID_MAP_H_
#define SERVICES_FILE_USER_ID_MAP_H_

#include <string>
#include "base/files/file_path.h"

namespace file {

// These methods are called from BrowserContext::Initialize() to associate
// the BrowserContext's Service user-id with its user directory.
void AssociateServiceUserIdWithUserDir(const std::string& user_id,
                                     const base::FilePath& user_dir);
void ForgetServiceUserIdUserDirAssociation(const std::string& user_id);

base::FilePath GetUserDirForUserId(const std::string& user_id);

}  // namespace file

#endif  // SERVICES_FILE_USER_ID_MAP_H_
