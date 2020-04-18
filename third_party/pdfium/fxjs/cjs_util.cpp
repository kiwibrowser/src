// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_util.h"

#include <time.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <vector>

#include "core/fxcrt/fx_extension.h"
#include "fxjs/JS_Define.h"
#include "fxjs/cjs_event_context.h"
#include "fxjs/cjs_eventhandler.h"
#include "fxjs/cjs_object.h"
#include "fxjs/cjs_publicmethods.h"
#include "fxjs/cjs_runtime.h"
#include "fxjs/js_resources.h"

#if _FX_OS_ == _FX_OS_ANDROID_
#include <ctype.h>
#endif

namespace {

// Map PDF-style directives to equivalent wcsftime directives. Not
// all have direct equivalents, though.
struct TbConvert {
  const wchar_t* lpszJSMark;
  const wchar_t* lpszCppMark;
};

// Map PDF-style directives lacking direct wcsftime directives to
// the value with which they will be replaced.
struct TbConvertAdditional {
  const wchar_t* lpszJSMark;
  int iValue;
};

const TbConvert TbConvertTable[] = {
    {L"mmmm", L"%B"}, {L"mmm", L"%b"}, {L"mm", L"%m"},   {L"dddd", L"%A"},
    {L"ddd", L"%a"},  {L"dd", L"%d"},  {L"yyyy", L"%Y"}, {L"yy", L"%y"},
    {L"HH", L"%H"},   {L"hh", L"%I"},  {L"MM", L"%M"},   {L"ss", L"%S"},
    {L"TT", L"%p"},
#if defined(_WIN32)
    {L"tt", L"%p"},   {L"h", L"%#I"},
#else
    {L"tt", L"%P"},   {L"h", L"%l"},
#endif
};

}  // namespace

const JSMethodSpec CJS_Util::MethodSpecs[] = {
    {"printd", printd_static},
    {"printf", printf_static},
    {"printx", printx_static},
    {"scand", scand_static},
    {"byteToChar", byteToChar_static}};

int CJS_Util::ObjDefnID = -1;
const char CJS_Util::kName[] = "util";

// static
void CJS_Util::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_Util::kName, FXJSOBJTYPE_STATIC,
                                 JSConstructor<CJS_Util>, JSDestructor);
  DefineMethods(pEngine, ObjDefnID, MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJS_Util::CJS_Util(v8::Local<v8::Object> pObject) : CJS_Object(pObject) {}

CJS_Util::~CJS_Util() = default;

CJS_Return CJS_Util::printf(CJS_Runtime* pRuntime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  const size_t iSize = params.size();
  if (iSize < 1)
    return CJS_Return(false);

  std::wstring unsafe_fmt_string(pRuntime->ToWideString(params[0]).c_str());
  std::vector<std::wstring> unsafe_conversion_specifiers;
  int iOffset = 0;
  int iOffend = 0;
  unsafe_fmt_string.insert(unsafe_fmt_string.begin(), L'S');
  while (iOffset != -1) {
    iOffend = unsafe_fmt_string.find(L"%", iOffset + 1);
    std::wstring strSub;
    if (iOffend == -1)
      strSub = unsafe_fmt_string.substr(iOffset);
    else
      strSub = unsafe_fmt_string.substr(iOffset, iOffend - iOffset);
    unsafe_conversion_specifiers.push_back(strSub);
    iOffset = iOffend;
  }

  std::wstring c_strResult;
  for (size_t iIndex = 0; iIndex < unsafe_conversion_specifiers.size();
       ++iIndex) {
    std::wstring c_strFormat = unsafe_conversion_specifiers[iIndex];
    if (iIndex == 0) {
      c_strResult = c_strFormat;
      continue;
    }

    if (iIndex >= iSize) {
      c_strResult += c_strFormat;
      continue;
    }

    WideString strSegment;
    switch (ParseDataType(&c_strFormat)) {
      case UTIL_INT:
        strSegment = WideString::Format(c_strFormat.c_str(),
                                        pRuntime->ToInt32(params[iIndex]));
        break;
      case UTIL_DOUBLE:
        strSegment = WideString::Format(c_strFormat.c_str(),
                                        pRuntime->ToDouble(params[iIndex]));
        break;
      case UTIL_STRING:
        strSegment =
            WideString::Format(c_strFormat.c_str(),
                               pRuntime->ToWideString(params[iIndex]).c_str());
        break;
      default:
        strSegment = WideString::Format(L"%ls", c_strFormat.c_str());
        break;
    }
    c_strResult += strSegment.c_str();
  }

  c_strResult.erase(c_strResult.begin());
  return CJS_Return(pRuntime->NewString(c_strResult.c_str()));
}

CJS_Return CJS_Util::printd(CJS_Runtime* pRuntime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  const size_t iSize = params.size();
  if (iSize < 2)
    return CJS_Return(false);

  if (params[1].IsEmpty() || !params[1]->IsDate())
    return CJS_Return(JSGetStringFromID(JSMessage::kSecondParamNotDateError));

  v8::Local<v8::Date> v8_date = params[1].As<v8::Date>();
  if (v8_date.IsEmpty() || std::isnan(pRuntime->ToDouble(v8_date))) {
    return CJS_Return(
        JSGetStringFromID(JSMessage::kSecondParamInvalidDateError));
  }

  double date = JS_LocalTime(pRuntime->ToDouble(v8_date));
  int year = JS_GetYearFromTime(date);
  int month = JS_GetMonthFromTime(date) + 1;  // One-based.
  int day = JS_GetDayFromTime(date);
  int hour = JS_GetHourFromTime(date);
  int min = JS_GetMinFromTime(date);
  int sec = JS_GetSecFromTime(date);

  if (params[0]->IsNumber()) {
    WideString swResult;
    switch (pRuntime->ToInt32(params[0])) {
      case 0:
        swResult = WideString::Format(L"D:%04d%02d%02d%02d%02d%02d", year,
                                      month, day, hour, min, sec);
        break;
      case 1:
        swResult = WideString::Format(L"%04d.%02d.%02d %02d:%02d:%02d", year,
                                      month, day, hour, min, sec);
        break;
      case 2:
        swResult = WideString::Format(L"%04d/%02d/%02d %02d:%02d:%02d", year,
                                      month, day, hour, min, sec);
        break;
      default:
        return CJS_Return(JSGetStringFromID(JSMessage::kValueError));
    }

    return CJS_Return(pRuntime->NewString(swResult.c_str()));
  }

  if (params[0]->IsString()) {
    // We don't support XFAPicture at the moment.
    if (iSize > 2 && pRuntime->ToBoolean(params[2]))
      return CJS_Return(JSGetStringFromID(JSMessage::kNotSupportedError));

    // Convert PDF-style format specifiers to wcsftime specifiers. Remove any
    // pre-existing %-directives before inserting our own.
    std::basic_string<wchar_t> cFormat =
        pRuntime->ToWideString(params[0]).c_str();
    cFormat.erase(std::remove(cFormat.begin(), cFormat.end(), '%'),
                  cFormat.end());

    for (size_t i = 0; i < FX_ArraySize(TbConvertTable); ++i) {
      int iStart = 0;
      int iEnd;
      while ((iEnd = cFormat.find(TbConvertTable[i].lpszJSMark, iStart)) !=
             -1) {
        cFormat.replace(iEnd, wcslen(TbConvertTable[i].lpszJSMark),
                        TbConvertTable[i].lpszCppMark);
        iStart = iEnd;
      }
    }

    if (year < 0)
      return CJS_Return(JSGetStringFromID(JSMessage::kValueError));

    const TbConvertAdditional cTableAd[] = {
        {L"m", month}, {L"d", day},
        {L"H", hour},  {L"h", hour > 12 ? hour - 12 : hour},
        {L"M", min},   {L"s", sec},
    };

    for (size_t i = 0; i < FX_ArraySize(cTableAd); ++i) {
      int iStart = 0;
      int iEnd;
      while ((iEnd = cFormat.find(cTableAd[i].lpszJSMark, iStart)) != -1) {
        if (iEnd > 0) {
          if (cFormat[iEnd - 1] == L'%') {
            iStart = iEnd + 1;
            continue;
          }
        }
        cFormat.replace(iEnd, wcslen(cTableAd[i].lpszJSMark),
                        WideString::Format(L"%d", cTableAd[i].iValue).c_str());
        iStart = iEnd;
      }
    }

    struct tm time = {};
    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = min;
    time.tm_sec = sec;

    wchar_t buf[64] = {};
    FXSYS_wcsftime(buf, 64, cFormat.c_str(), &time);
    cFormat = buf;
    return CJS_Return(pRuntime->NewString(cFormat.c_str()));
  }

  return CJS_Return(JSGetStringFromID(JSMessage::kTypeError));
}

CJS_Return CJS_Util::printx(CJS_Runtime* pRuntime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() < 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  return CJS_Return(
      pRuntime->NewString(printx(pRuntime->ToWideString(params[0]),
                                 pRuntime->ToWideString(params[1]))
                              .c_str()));
}

enum CaseMode { kPreserveCase, kUpperCase, kLowerCase };

static wchar_t TranslateCase(wchar_t input, CaseMode eMode) {
  if (eMode == kLowerCase && FXSYS_iswupper(input))
    return input | 0x20;
  if (eMode == kUpperCase && FXSYS_iswlower(input))
    return input & ~0x20;
  return input;
}

WideString CJS_Util::printx(const WideString& wsFormat,
                            const WideString& wsSource) {
  WideString wsResult;
  size_t iSourceIdx = 0;
  size_t iFormatIdx = 0;
  CaseMode eCaseMode = kPreserveCase;
  bool bEscaped = false;
  while (iFormatIdx < wsFormat.GetLength()) {
    if (bEscaped) {
      bEscaped = false;
      wsResult += wsFormat[iFormatIdx];
      ++iFormatIdx;
      continue;
    }
    switch (wsFormat[iFormatIdx]) {
      case '\\': {
        bEscaped = true;
        ++iFormatIdx;
      } break;
      case '<': {
        eCaseMode = kLowerCase;
        ++iFormatIdx;
      } break;
      case '>': {
        eCaseMode = kUpperCase;
        ++iFormatIdx;
      } break;
      case '=': {
        eCaseMode = kPreserveCase;
        ++iFormatIdx;
      } break;
      case '?': {
        if (iSourceIdx < wsSource.GetLength()) {
          wsResult += TranslateCase(wsSource[iSourceIdx], eCaseMode);
          ++iSourceIdx;
        }
        ++iFormatIdx;
      } break;
      case 'X': {
        if (iSourceIdx < wsSource.GetLength()) {
          if (isascii(wsSource[iSourceIdx]) && isalnum(wsSource[iSourceIdx])) {
            wsResult += TranslateCase(wsSource[iSourceIdx], eCaseMode);
            ++iFormatIdx;
          }
          ++iSourceIdx;
        } else {
          ++iFormatIdx;
        }
      } break;
      case 'A': {
        if (iSourceIdx < wsSource.GetLength()) {
          if (isascii(wsSource[iSourceIdx]) && isalpha(wsSource[iSourceIdx])) {
            wsResult += TranslateCase(wsSource[iSourceIdx], eCaseMode);
            ++iFormatIdx;
          }
          ++iSourceIdx;
        } else {
          ++iFormatIdx;
        }
      } break;
      case '9': {
        if (iSourceIdx < wsSource.GetLength()) {
          if (std::iswdigit(wsSource[iSourceIdx])) {
            wsResult += wsSource[iSourceIdx];
            ++iFormatIdx;
          }
          ++iSourceIdx;
        } else {
          ++iFormatIdx;
        }
      } break;
      case '*': {
        if (iSourceIdx < wsSource.GetLength()) {
          wsResult += TranslateCase(wsSource[iSourceIdx], eCaseMode);
          ++iSourceIdx;
        } else {
          ++iFormatIdx;
        }
      } break;
      default: {
        wsResult += wsFormat[iFormatIdx];
        ++iFormatIdx;
      } break;
    }
  }
  return wsResult;
}

CJS_Return CJS_Util::scand(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() < 2)
    return CJS_Return(false);

  WideString sFormat = pRuntime->ToWideString(params[0]);
  WideString sDate = pRuntime->ToWideString(params[1]);
  double dDate = JS_GetDateTime();
  if (sDate.GetLength() > 0)
    dDate = CJS_PublicMethods::MakeRegularDate(sDate, sFormat, nullptr);

  if (std::isnan(dDate))
    return CJS_Return(pRuntime->NewUndefined());
  return CJS_Return(pRuntime->NewDate(dDate));
}

CJS_Return CJS_Util::byteToChar(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() < 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  int arg = pRuntime->ToInt32(params[0]);
  if (arg < 0 || arg > 255)
    return CJS_Return(JSGetStringFromID(JSMessage::kValueError));

  WideString wStr(static_cast<wchar_t>(arg));
  return CJS_Return(pRuntime->NewString(wStr.c_str()));
}

// Ensure that sFormat contains at most one well-understood printf formatting
// directive which is safe to use with a single argument, and return the type
// of argument expected, or -1 otherwise. If -1 is returned, it is NOT safe
// to use sFormat with printf() and it must be copied byte-by-byte.
int CJS_Util::ParseDataType(std::wstring* sFormat) {
  enum State { BEFORE, FLAGS, WIDTH, PRECISION, SPECIFIER, AFTER };

  int result = -1;
  State state = BEFORE;
  size_t precision_digits = 0;
  size_t i = 0;
  while (i < sFormat->length()) {
    wchar_t c = (*sFormat)[i];
    switch (state) {
      case BEFORE:
        if (c == L'%')
          state = FLAGS;
        break;
      case FLAGS:
        if (c == L'+' || c == L'-' || c == L'#' || c == L' ') {
          // Stay in same state.
        } else {
          state = WIDTH;
          continue;  // Re-process same character.
        }
        break;
      case WIDTH:
        if (c == L'*')
          return -1;
        if (std::iswdigit(c)) {
          // Stay in same state.
        } else if (c == L'.') {
          state = PRECISION;
        } else {
          state = SPECIFIER;
          continue;  // Re-process same character.
        }
        break;
      case PRECISION:
        if (c == L'*')
          return -1;
        if (std::iswdigit(c)) {
          // Stay in same state.
          ++precision_digits;
        } else {
          state = SPECIFIER;
          continue;  // Re-process same character.
        }
        break;
      case SPECIFIER:
        if (c == L'c' || c == L'C' || c == L'd' || c == L'i' || c == L'o' ||
            c == L'u' || c == L'x' || c == L'X') {
          result = UTIL_INT;
        } else if (c == L'e' || c == L'E' || c == L'f' || c == L'g' ||
                   c == L'G') {
          result = UTIL_DOUBLE;
        } else if (c == L's' || c == L'S') {
          // Map s to S since we always deal internally with wchar_t strings.
          // TODO(tsepez): Probably 100% borked. %S is not a standard
          // conversion.
          (*sFormat)[i] = L'S';
          result = UTIL_STRING;
        } else {
          return -1;
        }
        state = AFTER;
        break;
      case AFTER:
        if (c == L'%')
          return -1;
        // Stay in same state until string exhausted.
        break;
    }
    ++i;
  }
  // See https://crbug.com/740166
  if (result == UTIL_INT && precision_digits > 2)
    return -1;

  return result;
}
