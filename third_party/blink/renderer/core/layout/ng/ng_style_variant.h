// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_STYLE_VARIANT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_STYLE_VARIANT_H_

namespace blink {

// LayoutObject can have multiple style variations.
enum class NGStyleVariant { kStandard, kFirstLine, kEllipsis };

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_STYLE_VARIANT_H_
