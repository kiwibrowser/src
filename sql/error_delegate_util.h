// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SQL_ERROR_DELEGATE_UTIL_H_
#define SQL_ERROR_DELEGATE_UTIL_H_

#include <string>

#include "base/files/file_path.h"
#include "sql/sql_export.h"

namespace sql {

// Returns true if it is highly unlikely that the database can recover from
// |error|.
SQL_EXPORT bool IsErrorCatastrophic(int error);

// Gets diagnostic info of the given |corrupted_file_path| that can be appended
// to a corrupt database diagnostics info. The file info are not localized as
// it's meant to be added to feedback reports and used by developers.
// Also the full file path is not appended as it might contain some PII. Instead
// only the last two components of the path are appended to distinguish between
// default and user profiles.
SQL_EXPORT std::string GetCorruptFileDiagnosticsInfo(
    const base::FilePath& corrupted_file_path);

}  // namespace sql

#endif  // SQL_ERROR_DELEGATE_UTIL_H_
