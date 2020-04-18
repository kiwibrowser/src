// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_ERRORS_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_ERRORS_H_

#include "base/files/file.h"
#include "chromeos/dbus/smb_provider_client.h"

namespace chromeos {
namespace smb_client {

// Translates an smbprovider::ErrorType to a base::File::Error. Since
// smbprovider::ErrorType is a superset of base::File::Error, errors that do not
// map directly are logged and mapped to the generic failed error.
base::File::Error TranslateToFileError(smbprovider::ErrorType error);

// Translates a base::File::Error to an smbprovider::ErrorType. There is an
// explicit smbprovider::ErrorType for each base::File::Error.
smbprovider::ErrorType TranslateToErrorType(base::File::Error error);

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_ERRORS_H_
