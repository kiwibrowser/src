// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_BATTOR_FINDER_H_
#define TOOLS_BATTOR_AGENT_BATTOR_FINDER_H_

#include <string>

#include "base/macros.h"

namespace battor {

class BattOrFinder {
 public:
  // Returns the path of the first BattOr that we find.
  static std::string FindBattOr();

  DISALLOW_COPY_AND_ASSIGN(BattOrFinder);
};

}  // namespace battor

#endif  // TOOLS_BATTOR_AGENT_BATTOR_FINDER_H_
