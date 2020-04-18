// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/assistant/ui/logo_view/base_logo_view.h"

#include "build/buildflag.h"
#include "chromeos/assistant/buildflags.h"

#if BUILDFLAG(ENABLE_CROS_LIBASSISTANT)
#include "ash/assistant/ui/logo_view/logo_view.h"
#endif

namespace ash {

BaseLogoView::BaseLogoView() = default;

BaseLogoView::~BaseLogoView() = default;

// static
BaseLogoView* BaseLogoView::Create() {
#if BUILDFLAG(ENABLE_CROS_LIBASSISTANT)
  return new LogoView();
#endif
  return new BaseLogoView();
}

}  // namespace ash
