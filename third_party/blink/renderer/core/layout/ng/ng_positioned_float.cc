// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_positioned_float.h"

#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"

namespace blink {

NGPositionedFloat::NGPositionedFloat(
    scoped_refptr<NGLayoutResult> layout_result,
    const NGBfcOffset& bfc_offset)
    : layout_result(layout_result), bfc_offset(bfc_offset) {}

// Define the destructor here, so that we can forward-declare more in the header
// file.
NGPositionedFloat::~NGPositionedFloat() = default;

}  // namespace blink
