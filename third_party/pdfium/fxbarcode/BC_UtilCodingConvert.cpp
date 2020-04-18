// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxbarcode/BC_UtilCodingConvert.h"

CBC_UtilCodingConvert::CBC_UtilCodingConvert() {}

CBC_UtilCodingConvert::~CBC_UtilCodingConvert() {}

void CBC_UtilCodingConvert::UnicodeToLocale(const WideString& src,
                                            ByteString& dst) {
  dst = ByteString::FromUnicode(src);
}

void CBC_UtilCodingConvert::LocaleToUtf8(const ByteString& src,
                                         ByteString& dst) {
  WideString unicode = WideString::FromLocal(src.AsStringView());
  dst = unicode.UTF8Encode();
}

void CBC_UtilCodingConvert::LocaleToUtf8(const ByteString& src,
                                         std::vector<uint8_t>& dst) {
  WideString unicode = WideString::FromLocal(src.AsStringView());
  ByteString utf8 = unicode.UTF8Encode();
  dst = std::vector<uint8_t>(utf8.begin(), utf8.end());
}

void CBC_UtilCodingConvert::Utf8ToLocale(const std::vector<uint8_t>& src,
                                         ByteString& dst) {
  ByteString utf8;
  for (uint8_t value : src)
    utf8 += value;

  WideString unicode = WideString::FromUTF8(utf8.AsStringView());
  dst = ByteString::FromUnicode(unicode);
}

void CBC_UtilCodingConvert::Utf8ToLocale(const uint8_t* src,
                                         int32_t count,
                                         ByteString& dst) {
  WideString unicode = WideString::FromUTF8(ByteStringView(src, count));
  dst = ByteString::FromUnicode(unicode);
}

void CBC_UtilCodingConvert::UnicodeToUTF8(const WideString& src,
                                          ByteString& dst) {
  dst = src.UTF8Encode();
}
