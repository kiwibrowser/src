// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/web_script_source.h"

#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"

namespace blink {

WebScriptSource::operator ScriptSourceCode() const {
  TextPosition position(OrdinalNumber::FromOneBasedInt(start_line),
                        OrdinalNumber::First());
  return ScriptSourceCode(code, ScriptSourceLocationType::kUnknown,
                          nullptr /* cache_handler */, url, position);
}

}  // namespace blink
