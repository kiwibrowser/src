// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include <vector>

#include "fxbarcode/utils.h"

bool BC_FX_ByteString_Replace(ByteString& dst,
                              uint32_t first,
                              uint32_t last,
                              int32_t count,
                              char c) {
  if (first > last || count <= 0) {
    return false;
  }
  dst.Delete(first, last - first);
  for (int32_t i = 0; i < count; i++) {
    dst.InsertAtFront(c);
  }
  return true;
}

void BC_FX_ByteString_Append(ByteString& dst, int32_t count, char c) {
  for (int32_t i = 0; i < count; i++)
    dst += c;
}
void BC_FX_ByteString_Append(ByteString& dst, const std::vector<uint8_t>& ba) {
  for (uint8_t value : ba)
    dst += value;
}
