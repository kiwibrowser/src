// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_INDICATOR_SPEC_H_
#define CHROME_BROWSER_VR_ELEMENTS_INDICATOR_SPEC_H_

#include <vector>

#include "chrome/browser/vr/elements/ui_element_name.h"
#include "chrome/browser/vr/model/capturing_state_model.h"
#include "ui/gfx/vector_icon_types.h"

namespace vr {

struct IndicatorSpec {
  IndicatorSpec(UiElementName name,
                UiElementName webvr_name,
                const gfx::VectorIcon& icon,
                int resource_string,
                int background_resource_string,
                int potential_resource_string,
                bool CapturingStateModel::*signal,
                bool CapturingStateModel::*background_signal,
                bool CapturingStateModel::*potential_signal,
                bool is_url);
  IndicatorSpec(const IndicatorSpec& other);
  ~IndicatorSpec();

  UiElementName name;
  UiElementName webvr_name;
  const gfx::VectorIcon& icon;
  int resource_string;
  int background_resource_string;
  int potential_resource_string;
  bool CapturingStateModel::*signal;
  bool CapturingStateModel::*background_signal;
  bool CapturingStateModel::*potential_signal;
  bool is_url;
};

std::vector<IndicatorSpec> GetIndicatorSpecs();

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_INDICATOR_SPEC_H_
