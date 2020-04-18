// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_SAFE_BROWSING_MAC_DMG_ANALYZER_H_
#define CHROME_UTILITY_SAFE_BROWSING_MAC_DMG_ANALYZER_H_

#include "base/files/file.h"

namespace safe_browsing {

struct ArchiveAnalyzerResults;

namespace dmg {

void AnalyzeDMGFile(base::File dmg_file, ArchiveAnalyzerResults* results);

}  // namespace dmg
}  // namespace safe_browsing

#endif  // CHROME_UTILITY_SAFE_BROWSING_MAC_DMG_ANALYZER_H_
