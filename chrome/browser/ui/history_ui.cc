// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/history_ui.h"

#include "components/grit/components_scaled_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace history_ui {

base::RefCountedMemory* GetFaviconResourceBytes(ui::ScaleFactor scale_factor) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
      IDR_HISTORY_FAVICON, scale_factor);
}

}  // namespace history_ui
