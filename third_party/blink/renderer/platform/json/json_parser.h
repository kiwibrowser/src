// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_JSON_JSON_PARSER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_JSON_JSON_PARSER_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include <memory>

namespace blink {

class JSONValue;

PLATFORM_EXPORT std::unique_ptr<JSONValue> ParseJSON(const String& json);

PLATFORM_EXPORT std::unique_ptr<JSONValue> ParseJSON(const String& json,
                                                     int max_depth);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_JSON_JSON_PARSER_H_
