// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_TABLE_CONSTANTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_TABLE_CONSTANTS_H_

namespace blink {

// https://html.spec.whatwg.org/multipage/tables.html#dom-colgroup-span
// https://html.spec.whatwg.org/multipage/tables.html#dom-col-span
// https://html.spec.whatwg.org/multipage/tables.html#dom-tdth-colspan
constexpr unsigned kDefaultColSpan = 1u;
constexpr unsigned kMinColSpan = 1u;
constexpr unsigned kMaxColSpan = 1000u;

// https://html.spec.whatwg.org/multipage/tables.html#dom-tdth-rowspan
constexpr unsigned kDefaultRowSpan = 1u;
constexpr unsigned kMaxRowSpan = 65534u;
constexpr unsigned kMinRowSpan = 0;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_TABLE_CONSTANTS_H_
