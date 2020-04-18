// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/ios/browser/string_clipping_util.h"

#include <stddef.h>

#include "base/strings/string_util.h"

base::string16 GetStringByClippingLastWord(const base::string16& contents,
                                           size_t length) {
  if (contents.size() < length)
    return contents;

  base::string16 clipped_contents = contents.substr(0, length);
  size_t last_space_index =
      clipped_contents.find_last_of(base::kWhitespaceUTF16);
  if (last_space_index != base::string16::npos)
    clipped_contents.resize(last_space_index);
  return clipped_contents;
}
