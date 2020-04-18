// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/html_parser_reentry_permit.h"

namespace blink {

scoped_refptr<HTMLParserReentryPermit> HTMLParserReentryPermit::Create() {
  return base::AdoptRef(new HTMLParserReentryPermit());
}

HTMLParserReentryPermit::HTMLParserReentryPermit()
    : script_nesting_level_(0), parser_pause_flag_(false) {}

}  // namespace blink
