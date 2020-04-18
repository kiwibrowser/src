// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_PASSWORD_IMPORTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_PASSWORD_IMPORTER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

// Static-only class bundling together the API for importing passwords from a
// file.
class PasswordImporter {
 public:
  enum Result {
    SUCCESS,
    IO_ERROR,
    SYNTAX_ERROR,
    SEMANTIC_ERROR,
    NUM_IMPORT_RESULTS
  };

  typedef base::Callback<void(Result,
                              const std::vector<autofill::PasswordForm>&)>
      CompletionCallback;

  // Imports passwords from the file at |path|, and fires |completion| callback
  // on the calling thread with the passwords when ready. The only supported
  // file format is CSV.
  static void Import(const base::FilePath& path,
                     const CompletionCallback& completion);

  // Returns the file extensions corresponding to supported formats.
  static std::vector<std::vector<base::FilePath::StringType>>
  GetSupportedFileExtensions();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PasswordImporter);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_IMPORT_PASSWORD_IMPORTER_H_
