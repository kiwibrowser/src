// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/dom_distiller_features.h"

#include <string>

#include "base/command_line.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"

namespace dom_distiller {

bool IsEnableDomDistillerSet() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableDomDistiller);
}

bool IsEnableSyncArticlesSet() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableSyncArticles);
}

}  // namespace dom_distiller
