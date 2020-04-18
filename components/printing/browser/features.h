// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_BROWSER_FEATURES_H_
#define COMPONENTS_PRINTING_BROWSER_FEATURES_H_

#include "base/feature_list.h"

namespace printing {
namespace features {

// Use pdf compositor service to generate PDF files for printing.
extern const base::Feature kUsePdfCompositorServiceForPrint;

}  // namespace features
}  // namespace printing

#endif  // COMPONENTS_PRINTING_BROWSER_FEATURES_H_
