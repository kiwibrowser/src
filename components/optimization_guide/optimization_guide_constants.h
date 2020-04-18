// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPTIMIZATION_GUIDE_OPTIMIZATION_GUIDE_CONSTANTS_H_
#define COMPONENTS_OPTIMIZATION_GUIDE_OPTIMIZATION_GUIDE_CONSTANTS_H_

#include "base/files/file_path.h"

namespace optimization_guide {

// The name of the file that stores the unindexed hints.
extern const base::FilePath::CharType kUnindexedHintsFileName[];

extern const char kRulesetFormatVersionString[];

}  // namespace optimization_guide

#endif  // COMPONENTS_OPTIMIZATION_GUIDE_OPTIMIZATION_GUIDE_CONSTANTS_H_
