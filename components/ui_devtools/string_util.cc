// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/string_util.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/ui_devtools/Protocol.h"

namespace ui_devtools {
namespace protocol {

// static
std::unique_ptr<Value> StringUtil::parseJSON(const String& string) {
  DCHECK(base::IsStringUTF8(string));
  // TODO(mhashmi): 16-bit strings need to be handled
  return parseJSONCharacters(reinterpret_cast<const uint8_t*>(&string[0]),
                             string.length());
};

// static
void StringUtil::builderAppendQuotedString(StringBuilder& builder,
                                           const String& str) {
  builder.append('"');
  base::string16 str16 = base::UTF8ToUTF16(str);
  escapeWideStringForJSON(reinterpret_cast<const uint16_t*>(&str16[0]),
                          str16.length(), &builder);
  builder.append('"');
}

}  // namespace protocol
}  // namespace ui_devtools
