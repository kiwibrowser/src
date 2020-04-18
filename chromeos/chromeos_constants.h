// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants related to ChromeOS.

#ifndef CHROMEOS_CHROMEOS_CONSTANTS_H_
#define CHROMEOS_CHROMEOS_CONSTANTS_H_

#include "base/files/file_path.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

CHROMEOS_EXPORT extern const base::FilePath::CharType kDriveCacheDirname[];
CHROMEOS_EXPORT extern const base::FilePath::CharType kNssCertDbPath[];
CHROMEOS_EXPORT extern const base::FilePath::CharType kNssDirPath[];
CHROMEOS_EXPORT extern const base::FilePath::CharType kNssKeyDbPath[];

}  // namespace chromeos

#endif  // CHROMEOS_CHROMEOS_CONSTANTS_H_
