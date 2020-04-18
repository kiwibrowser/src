// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_FX_EXTENSION_H_
#define CORE_FXCRT_FX_EXTENSION_H_

#include <cctype>
#include <cwctype>
#include <memory>

#include "core/fxcrt/fx_string.h"
#include "third_party/base/span.h"

#if defined(USE_SYSTEM_ICUUC)
#include <unicode/uchar.h>
#else
#include "third_party/icu/source/common/unicode/uchar.h"
#endif

#define FX_INVALID_OFFSET static_cast<uint32_t>(-1)

#ifdef PDF_ENABLE_XFA
#define FX_IsOdd(a) ((a)&1)
#endif  // PDF_ENABLE_XFA

float FXSYS_wcstof(const wchar_t* pwsStr,
                   int32_t iLength = -1,
                   int32_t* pUsedLen = nullptr);
wchar_t* FXSYS_wcsncpy(wchar_t* dstStr, const wchar_t* srcStr, size_t count);
int32_t FXSYS_wcsnicmp(const wchar_t* s1, const wchar_t* s2, size_t count);

inline bool FXSYS_iswlower(int32_t c) {
  return u_islower(c);
}

inline bool FXSYS_iswupper(int32_t c) {
  return u_isupper(c);
}

inline int32_t FXSYS_towlower(wchar_t c) {
  return u_tolower(c);
}

inline int32_t FXSYS_towupper(wchar_t c) {
  return u_toupper(c);
}

inline bool FXSYS_iswalpha(wchar_t c) {
  return u_isalpha(c);
}

inline bool FXSYS_iswalnum(wchar_t c) {
  return u_isalnum(c);
}

inline bool FXSYS_iswspace(wchar_t c) {
  return u_isspace(c);
}

inline bool FXSYS_isHexDigit(const char c) {
  return !((c & 0x80) || !std::isxdigit(c));
}

inline int FXSYS_HexCharToInt(const char c) {
  if (!FXSYS_isHexDigit(c))
    return 0;
  char upchar = std::toupper(c);
  return upchar > '9' ? upchar - 'A' + 10 : upchar - '0';
}

inline bool FXSYS_isDecimalDigit(const char c) {
  return !((c & 0x80) || !std::isdigit(c));
}

inline bool FXSYS_isDecimalDigit(const wchar_t c) {
  return !!std::iswdigit(c);
}

inline int FXSYS_DecimalCharToInt(const char c) {
  return FXSYS_isDecimalDigit(c) ? c - '0' : 0;
}

inline int FXSYS_DecimalCharToInt(const wchar_t c) {
  return std::iswdigit(c) ? c - L'0' : 0;
}

void FXSYS_IntToTwoHexChars(uint8_t c, char* buf);
void FXSYS_IntToFourHexChars(uint16_t c, char* buf);

size_t FXSYS_ToUTF16BE(uint32_t unicode, char* buf);

#endif  // CORE_FXCRT_FX_EXTENSION_H_
