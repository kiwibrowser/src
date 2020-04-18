// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_BC_UTILCODINGCONVERT_H_
#define FXBARCODE_BC_UTILCODINGCONVERT_H_

#include <vector>

#include "core/fxcrt/fx_string.h"

class CBC_UtilCodingConvert {
 public:
  CBC_UtilCodingConvert();
  virtual ~CBC_UtilCodingConvert();
  static void UnicodeToLocale(const WideString& source, ByteString& result);
  static void LocaleToUtf8(const ByteString& source, ByteString& result);
  static void LocaleToUtf8(const ByteString& source,
                           std::vector<uint8_t>& result);
  static void Utf8ToLocale(const std::vector<uint8_t>& source,
                           ByteString& result);
  static void Utf8ToLocale(const uint8_t* source,
                           int32_t count,
                           ByteString& result);
  static void UnicodeToUTF8(const WideString& source, ByteString& result);
};

#endif  // FXBARCODE_BC_UTILCODINGCONVERT_H_
