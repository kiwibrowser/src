// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_publicmethods.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fxcrt/fx_extension.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_interform.h"
#include "fxjs/JS_Define.h"
#include "fxjs/cjs_color.h"
#include "fxjs/cjs_event_context.h"
#include "fxjs/cjs_eventhandler.h"
#include "fxjs/cjs_field.h"
#include "fxjs/cjs_object.h"
#include "fxjs/cjs_runtime.h"
#include "fxjs/cjs_util.h"
#include "fxjs/js_resources.h"

// static
const JSMethodSpec CJS_PublicMethods::GlobalFunctionSpecs[] = {
    {"AFNumber_Format", AFNumber_Format_static},
    {"AFNumber_Keystroke", AFNumber_Keystroke_static},
    {"AFPercent_Format", AFPercent_Format_static},
    {"AFPercent_Keystroke", AFPercent_Keystroke_static},
    {"AFDate_FormatEx", AFDate_FormatEx_static},
    {"AFDate_KeystrokeEx", AFDate_KeystrokeEx_static},
    {"AFDate_Format", AFDate_Format_static},
    {"AFDate_Keystroke", AFDate_Keystroke_static},
    {"AFTime_FormatEx", AFTime_FormatEx_static},
    {"AFTime_KeystrokeEx", AFTime_KeystrokeEx_static},
    {"AFTime_Format", AFTime_Format_static},
    {"AFTime_Keystroke", AFTime_Keystroke_static},
    {"AFSpecial_Format", AFSpecial_Format_static},
    {"AFSpecial_Keystroke", AFSpecial_Keystroke_static},
    {"AFSpecial_KeystrokeEx", AFSpecial_KeystrokeEx_static},
    {"AFSimple", AFSimple_static},
    {"AFMakeNumber", AFMakeNumber_static},
    {"AFSimple_Calculate", AFSimple_Calculate_static},
    {"AFRange_Validate", AFRange_Validate_static},
    {"AFMergeChange", AFMergeChange_static},
    {"AFParseDateEx", AFParseDateEx_static},
    {"AFExtractNums", AFExtractNums_static},
};

namespace {

#if _FX_OS_ != _FX_OS_ANDROID_
constexpr double kDoubleCorrect = 0.000000000000001;
#endif

const wchar_t* const kMonths[] = {L"Jan", L"Feb", L"Mar", L"Apr",
                                  L"May", L"Jun", L"Jul", L"Aug",
                                  L"Sep", L"Oct", L"Nov", L"Dec"};

const wchar_t* const kFullMonths[] = {L"January", L"February", L"March",
                                      L"April",   L"May",      L"June",
                                      L"July",    L"August",   L"September",
                                      L"October", L"November", L"December"};

template <typename T>
T StrTrim(const T& str) {
  T result = str;
  result.Trim(' ');
  return result;
}

void AlertIfPossible(CJS_EventContext* pContext, const wchar_t* swMsg) {
  CPDFSDK_FormFillEnvironment* pFormFillEnv = pContext->GetFormFillEnv();
  if (pFormFillEnv)
    pFormFillEnv->JS_appAlert(swMsg, WideString(), 0, 3);
}

#if _FX_OS_ != _FX_OS_ANDROID_
ByteString CalculateString(double dValue,
                           int iDec,
                           int* iDec2,
                           bool* bNegative) {
  *bNegative = dValue < 0;
  if (*bNegative)
    dValue = -dValue;

  // Make sure the number of precision characters will fit.
  iDec = std::min(iDec, std::numeric_limits<double>::digits10);

  std::stringstream ss;
  ss << std::fixed << std::setprecision(iDec) << dValue;
  std::string value = ss.str();
  size_t pos = value.find('.');
  *iDec2 = pos == std::string::npos ? value.size() : static_cast<int>(pos);
  return ByteString(value.c_str());
}
#endif

WideString CalcMergedString(const CJS_EventHandler* event,
                            const WideString& value,
                            const WideString& change) {
  WideString prefix = value.Left(event->SelStart());
  WideString postfix;
  int end = event->SelEnd();
  if (end >= 0 && static_cast<size_t>(end) < value.GetLength())
    postfix = value.Right(value.GetLength() - static_cast<size_t>(end));
  return prefix + change + postfix;
}

template <CJS_Return (*F)(CJS_Runtime*,
                          const std::vector<v8::Local<v8::Value>>&)>
void JSGlobalFunc(const char* func_name_string,
                  const v8::FunctionCallbackInfo<v8::Value>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  std::vector<v8::Local<v8::Value>> parameters;
  for (int i = 0; i < info.Length(); ++i)
    parameters.push_back(info[i]);

  CJS_Return result = (*F)(pRuntime, parameters);
  if (result.HasError()) {
    pRuntime->Error(
        JSFormatErrorString(func_name_string, nullptr, result.Error()));
    return;
  }

  if (result.HasReturn())
    info.GetReturnValue().Set(result.Return());
}

int WithinBoundsOrZero(int value, size_t size) {
  return value >= 0 && static_cast<size_t>(value) < size ? value : 0;
}

int ValidStyleOrZero(int style) {
  return WithinBoundsOrZero(style, 4);
}

bool IsDigitSeparatorOrDecimalMark(int c) {
  return c == '.' || c == ',';
}

#if _FX_OS_ != _FX_OS_ANDROID_
bool IsStyleWithDigitSeparator(int style) {
  return style == 0 || style == 2;
}

char DigitSeparatorForStyle(int style) {
  ASSERT(IsStyleWithDigitSeparator(style));
  return style == 0 ? ',' : '.';
}
#endif

bool IsStyleWithCommaDecimalMark(int style) {
  return style >= 2;
}

char DecimalMarkForStyle(int style) {
  return IsStyleWithCommaDecimalMark(style) ? ',' : '.';
}

#if _FX_OS_ != _FX_OS_ANDROID_
void NormalizeDecimalMark(ByteString* str) {
  str->Replace(",", ".");
}
#endif

void NormalizeDecimalMarkW(WideString* str) {
  str->Replace(L",", L".");
}

bool IsValidMonth(int m) {
  return m >= 1 && m <= 12;
}

// TODO(thestig): Should this take the month into consideration?
bool IsValidDay(int d) {
  return d >= 1 && d <= 31;
}

// TODO(thestig): Should 24 be allowed? Similarly, 60 for minutes and seconds.
bool IsValid24Hour(int h) {
  return h >= 0 && h <= 24;
}

bool IsValidMinute(int m) {
  return m >= 0 && m <= 60;
}

bool IsValidSecond(int s) {
  return s >= 0 && s <= 60;
}

}  // namespace

CJS_PublicMethods::CJS_PublicMethods(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject) {}

CJS_PublicMethods::~CJS_PublicMethods() {}

// static
void CJS_PublicMethods::DefineJSObjects(CFXJS_Engine* pEngine) {
  for (const auto& spec : GlobalFunctionSpecs)
    pEngine->DefineGlobalMethod(spec.pName, spec.pMethodCall);
}

#define JS_STATIC_GLOBAL_FUN(fun_name)                   \
  void CJS_PublicMethods::fun_name##_static(             \
      const v8::FunctionCallbackInfo<v8::Value>& info) { \
    JSGlobalFunc<fun_name>(#fun_name, info);             \
  }

JS_STATIC_GLOBAL_FUN(AFNumber_Format);
JS_STATIC_GLOBAL_FUN(AFNumber_Keystroke);
JS_STATIC_GLOBAL_FUN(AFPercent_Format);
JS_STATIC_GLOBAL_FUN(AFPercent_Keystroke);
JS_STATIC_GLOBAL_FUN(AFDate_FormatEx);
JS_STATIC_GLOBAL_FUN(AFDate_KeystrokeEx);
JS_STATIC_GLOBAL_FUN(AFDate_Format);
JS_STATIC_GLOBAL_FUN(AFDate_Keystroke);
JS_STATIC_GLOBAL_FUN(AFTime_FormatEx);
JS_STATIC_GLOBAL_FUN(AFTime_KeystrokeEx);
JS_STATIC_GLOBAL_FUN(AFTime_Format);
JS_STATIC_GLOBAL_FUN(AFTime_Keystroke);
JS_STATIC_GLOBAL_FUN(AFSpecial_Format);
JS_STATIC_GLOBAL_FUN(AFSpecial_Keystroke);
JS_STATIC_GLOBAL_FUN(AFSpecial_KeystrokeEx);
JS_STATIC_GLOBAL_FUN(AFSimple);
JS_STATIC_GLOBAL_FUN(AFMakeNumber);
JS_STATIC_GLOBAL_FUN(AFSimple_Calculate);
JS_STATIC_GLOBAL_FUN(AFRange_Validate);
JS_STATIC_GLOBAL_FUN(AFMergeChange);
JS_STATIC_GLOBAL_FUN(AFParseDateEx);
JS_STATIC_GLOBAL_FUN(AFExtractNums);

bool CJS_PublicMethods::IsNumber(const WideString& str) {
  WideString sTrim = StrTrim(str);
  const wchar_t* pTrim = sTrim.c_str();
  const wchar_t* p = pTrim;
  bool bDot = false;
  bool bKXJS = false;

  wchar_t c;
  while ((c = *p) != L'\0') {
    if (IsDigitSeparatorOrDecimalMark(c)) {
      if (bDot)
        return false;
      bDot = true;
    } else if (c == L'-' || c == L'+') {
      if (p != pTrim)
        return false;
    } else if (c == L'e' || c == L'E') {
      if (bKXJS)
        return false;

      p++;
      c = *p;
      if (c != L'+' && c != L'-')
        return false;
      bKXJS = true;
    } else if (!std::iswdigit(c)) {
      return false;
    }
    p++;
  }

  return true;
}

bool CJS_PublicMethods::MaskSatisfied(wchar_t c_Change, wchar_t c_Mask) {
  switch (c_Mask) {
    case L'9':
      return !!std::iswdigit(c_Change);
    case L'A':
      return isascii(c_Change) && isalpha(c_Change);
    case L'O':
      return isascii(c_Change) && isalnum(c_Change);
    case L'X':
      return true;
    default:
      return (c_Change == c_Mask);
  }
}

bool CJS_PublicMethods::IsReservedMaskChar(wchar_t ch) {
  return ch == L'9' || ch == L'A' || ch == L'O' || ch == L'X';
}

double CJS_PublicMethods::AF_Simple(const wchar_t* sFuction,
                                    double dValue1,
                                    double dValue2) {
  if (FXSYS_wcsicmp(sFuction, L"AVG") == 0 ||
      FXSYS_wcsicmp(sFuction, L"SUM") == 0) {
    return dValue1 + dValue2;
  }
  if (FXSYS_wcsicmp(sFuction, L"PRD") == 0)
    return dValue1 * dValue2;
  if (FXSYS_wcsicmp(sFuction, L"MIN") == 0)
    return std::min(dValue1, dValue2);
  if (FXSYS_wcsicmp(sFuction, L"MAX") == 0)
    return std::max(dValue1, dValue2);
  return dValue1;
}

v8::Local<v8::Array> CJS_PublicMethods::AF_MakeArrayFromList(
    CJS_Runtime* pRuntime,
    v8::Local<v8::Value> val) {
  if (!val.IsEmpty() && val->IsArray())
    return pRuntime->ToArray(val);

  WideString wsStr = pRuntime->ToWideString(val);
  ByteString t = ByteString::FromUnicode(wsStr);
  const char* p = t.c_str();

  int nIndex = 0;
  v8::Local<v8::Array> StrArray = pRuntime->NewArray();
  while (*p) {
    const char* pTemp = strchr(p, ',');
    if (!pTemp) {
      pRuntime->PutArrayElement(
          StrArray, nIndex,
          pRuntime->NewString(StrTrim(ByteString(p)).c_str()));
      break;
    }

    pRuntime->PutArrayElement(
        StrArray, nIndex,
        pRuntime->NewString(StrTrim(ByteString(p, pTemp - p)).c_str()));

    nIndex++;
    p = ++pTemp;
  }
  return StrArray;
}

int CJS_PublicMethods::ParseStringInteger(const WideString& str,
                                          size_t nStart,
                                          size_t* pSkip,
                                          size_t nMaxStep) {
  int nRet = 0;
  size_t nSkip = 0;
  for (size_t i = nStart; i < str.GetLength(); ++i) {
    if (i - nStart > 10)
      break;

    wchar_t c = str[i];
    if (!std::iswdigit(c))
      break;

    nRet = nRet * 10 + FXSYS_DecimalCharToInt(c);
    ++nSkip;
    if (nSkip >= nMaxStep)
      break;
  }

  *pSkip = nSkip;
  return nRet;
}

WideString CJS_PublicMethods::ParseStringString(const WideString& str,
                                                size_t nStart,
                                                size_t* pSkip) {
  WideString swRet;
  swRet.Reserve(str.GetLength());
  for (size_t i = nStart; i < str.GetLength(); ++i) {
    wchar_t c = str[i];
    if (!std::iswdigit(c))
      break;

    swRet += c;
  }

  *pSkip = swRet.GetLength();
  return swRet;
}

double CJS_PublicMethods::ParseNormalDate(const WideString& value,
                                          bool* bWrongFormat) {
  double dt = JS_GetDateTime();

  int nYear = JS_GetYearFromTime(dt);
  int nMonth = JS_GetMonthFromTime(dt) + 1;
  int nDay = JS_GetDayFromTime(dt);
  int nHour = JS_GetHourFromTime(dt);
  int nMin = JS_GetMinFromTime(dt);
  int nSec = JS_GetSecFromTime(dt);

  int number[3];

  size_t nSkip = 0;
  size_t nLen = value.GetLength();
  size_t nIndex = 0;
  size_t i = 0;
  while (i < nLen) {
    if (nIndex > 2)
      break;

    wchar_t c = value[i];
    if (std::iswdigit(c)) {
      number[nIndex++] = ParseStringInteger(value, i, &nSkip, 4);
      i += nSkip;
    } else {
      i++;
    }
  }

  if (nIndex == 2) {
    // TODO(thestig): Should the else case set |bWrongFormat| to true?
    // case2: month/day
    // case3: day/month
    if (IsValidMonth(number[0]) && IsValidDay(number[1])) {
      nMonth = number[0];
      nDay = number[1];
    } else if (IsValidDay(number[0]) && IsValidMonth(number[1])) {
      nDay = number[0];
      nMonth = number[1];
    }

    if (bWrongFormat)
      *bWrongFormat = false;
  } else if (nIndex == 3) {
    // TODO(thestig): Should the else case set |bWrongFormat| to true?
    // case1: year/month/day
    // case2: month/day/year
    // case3: day/month/year
    if (number[0] > 12 && IsValidMonth(number[1]) && IsValidDay(number[2])) {
      nYear = number[0];
      nMonth = number[1];
      nDay = number[2];
    } else if (IsValidMonth(number[0]) && IsValidDay(number[1]) &&
               number[2] > 31) {
      nMonth = number[0];
      nDay = number[1];
      nYear = number[2];
    } else if (IsValidDay(number[0]) && IsValidMonth(number[1]) &&
               number[2] > 31) {
      nDay = number[0];
      nMonth = number[1];
      nYear = number[2];
    }

    if (bWrongFormat)
      *bWrongFormat = false;
  } else {
    if (bWrongFormat)
      *bWrongFormat = true;
    return dt;
  }

  // TODO(thestig): Should we set |bWrongFormat| to false here too?
  return JS_DateParse(WideString::Format(L"%d/%d/%d %d:%d:%d", nMonth, nDay,
                                         nYear, nHour, nMin, nSec));
}

double CJS_PublicMethods::MakeRegularDate(const WideString& value,
                                          const WideString& format,
                                          bool* bWrongFormat) {
  double dt = JS_GetDateTime();

  if (format.IsEmpty() || value.IsEmpty())
    return dt;

  int nYear = JS_GetYearFromTime(dt);
  int nMonth = JS_GetMonthFromTime(dt) + 1;
  int nDay = JS_GetDayFromTime(dt);
  int nHour = JS_GetHourFromTime(dt);
  int nMin = JS_GetMinFromTime(dt);
  int nSec = JS_GetSecFromTime(dt);

  int nYearSub = 99;  // nYear - 2000;

  bool bPm = false;
  bool bExit = false;
  bool bBadFormat = false;

  size_t i = 0;
  size_t j = 0;

  while (i < format.GetLength()) {
    if (bExit)
      break;

    wchar_t c = format[i];
    switch (c) {
      case ':':
      case '.':
      case '-':
      case '\\':
      case '/':
        i++;
        j++;
        break;

      case 'y':
      case 'm':
      case 'd':
      case 'H':
      case 'h':
      case 'M':
      case 's':
      case 't': {
        size_t oldj = j;
        size_t nSkip = 0;
        size_t remaining = format.GetLength() - i - 1;

        if (remaining == 0 || format[i + 1] != c) {
          switch (c) {
            case 'y':
              i++;
              j++;
              break;
            case 'm':
              nMonth = ParseStringInteger(value, j, &nSkip, 2);
              i++;
              j += nSkip;
              break;
            case 'd':
              nDay = ParseStringInteger(value, j, &nSkip, 2);
              i++;
              j += nSkip;
              break;
            case 'H':
              nHour = ParseStringInteger(value, j, &nSkip, 2);
              i++;
              j += nSkip;
              break;
            case 'h':
              nHour = ParseStringInteger(value, j, &nSkip, 2);
              i++;
              j += nSkip;
              break;
            case 'M':
              nMin = ParseStringInteger(value, j, &nSkip, 2);
              i++;
              j += nSkip;
              break;
            case 's':
              nSec = ParseStringInteger(value, j, &nSkip, 2);
              i++;
              j += nSkip;
              break;
            case 't':
              bPm = (j < value.GetLength() && value[j] == 'p');
              i++;
              j++;
              break;
          }
        } else if (remaining == 1 || format[i + 2] != c) {
          switch (c) {
            case 'y':
              nYear = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 'm':
              nMonth = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 'd':
              nDay = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 'H':
              nHour = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 'h':
              nHour = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 'M':
              nMin = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 's':
              nSec = ParseStringInteger(value, j, &nSkip, 2);
              i += 2;
              j += nSkip;
              break;
            case 't':
              bPm = (j + 1 < value.GetLength() && value[j] == 'p' &&
                     value[j + 1] == 'm');
              i += 2;
              j += 2;
              break;
          }
        } else if (remaining == 2 || format[i + 3] != c) {
          switch (c) {
            case 'm': {
              WideString sMonth = ParseStringString(value, j, &nSkip);
              bool bFind = false;
              for (int m = 0; m < 12; m++) {
                if (sMonth.CompareNoCase(kMonths[m]) == 0) {
                  nMonth = m + 1;
                  i += 3;
                  j += nSkip;
                  bFind = true;
                  break;
                }
              }

              if (!bFind) {
                nMonth = ParseStringInteger(value, j, &nSkip, 3);
                i += 3;
                j += nSkip;
              }
            } break;
            case 'y':
              break;
            default:
              i += 3;
              j += 3;
              break;
          }
        } else if (remaining == 3 || format[i + 4] != c) {
          switch (c) {
            case 'y':
              nYear = ParseStringInteger(value, j, &nSkip, 4);
              j += nSkip;
              i += 4;
              break;
            case 'm': {
              bool bFind = false;

              WideString sMonth = ParseStringString(value, j, &nSkip);
              sMonth.MakeLower();

              for (int m = 0; m < 12; m++) {
                WideString sFullMonths = kFullMonths[m];
                sFullMonths.MakeLower();

                if (sFullMonths.Contains(sMonth.c_str())) {
                  nMonth = m + 1;
                  i += 4;
                  j += nSkip;
                  bFind = true;
                  break;
                }
              }

              if (!bFind) {
                nMonth = ParseStringInteger(value, j, &nSkip, 4);
                i += 4;
                j += nSkip;
              }
            } break;
            default:
              i += 4;
              j += 4;
              break;
          }
        } else {
          if (j >= value.GetLength() || format[i] != value[j]) {
            bBadFormat = true;
            bExit = true;
          }
          i++;
          j++;
        }

        if (oldj == j) {
          bBadFormat = true;
          bExit = true;
        }
        break;
      }

      default:
        if (value.GetLength() <= j) {
          bExit = true;
        } else if (format[i] != value[j]) {
          bBadFormat = true;
          bExit = true;
        }

        i++;
        j++;
        break;
    }
  }

  if (bPm)
    nHour += 12;

  if (nYear >= 0 && nYear <= nYearSub)
    nYear += 2000;

  if (!bBadFormat) {
    bBadFormat = !IsValidMonth(nMonth) || !IsValidDay(nDay) ||
                 !IsValid24Hour(nHour) || !IsValidMinute(nMin) ||
                 !IsValidSecond(nSec);
  }

  double dRet;
  if (bBadFormat) {
    dRet = ParseNormalDate(value, &bBadFormat);
  } else {
    dRet = JS_MakeDate(JS_MakeDay(nYear, nMonth - 1, nDay),
                       JS_MakeTime(nHour, nMin, nSec, 0));
    if (std::isnan(dRet))
      dRet = JS_DateParse(value);
  }

  if (std::isnan(dRet))
    dRet = ParseNormalDate(value, &bBadFormat);

  if (bWrongFormat)
    *bWrongFormat = bBadFormat;

  return dRet;
}

WideString CJS_PublicMethods::MakeFormatDate(double dDate,
                                             const WideString& format) {
  WideString sRet;
  WideString sPart;

  int nYear = JS_GetYearFromTime(dDate);
  int nMonth = JS_GetMonthFromTime(dDate) + 1;
  int nDay = JS_GetDayFromTime(dDate);
  int nHour = JS_GetHourFromTime(dDate);
  int nMin = JS_GetMinFromTime(dDate);
  int nSec = JS_GetSecFromTime(dDate);

  size_t i = 0;
  while (i < format.GetLength()) {
    wchar_t c = format[i];
    size_t remaining = format.GetLength() - i - 1;
    sPart.clear();
    switch (c) {
      case 'y':
      case 'm':
      case 'd':
      case 'H':
      case 'h':
      case 'M':
      case 's':
      case 't':
        if (remaining == 0 || format[i + 1] != c) {
          switch (c) {
            case 'y':
              sPart += c;
              break;
            case 'm':
              sPart = WideString::Format(L"%d", nMonth);
              break;
            case 'd':
              sPart = WideString::Format(L"%d", nDay);
              break;
            case 'H':
              sPart = WideString::Format(L"%d", nHour);
              break;
            case 'h':
              sPart =
                  WideString::Format(L"%d", nHour > 12 ? nHour - 12 : nHour);
              break;
            case 'M':
              sPart = WideString::Format(L"%d", nMin);
              break;
            case 's':
              sPart = WideString::Format(L"%d", nSec);
              break;
            case 't':
              sPart += nHour > 12 ? 'p' : 'a';
              break;
          }
          i++;
        } else if (remaining == 1 || format[i + 2] != c) {
          switch (c) {
            case 'y':
              sPart = WideString::Format(L"%02d", nYear - (nYear / 100) * 100);
              break;
            case 'm':
              sPart = WideString::Format(L"%02d", nMonth);
              break;
            case 'd':
              sPart = WideString::Format(L"%02d", nDay);
              break;
            case 'H':
              sPart = WideString::Format(L"%02d", nHour);
              break;
            case 'h':
              sPart =
                  WideString::Format(L"%02d", nHour > 12 ? nHour - 12 : nHour);
              break;
            case 'M':
              sPart = WideString::Format(L"%02d", nMin);
              break;
            case 's':
              sPart = WideString::Format(L"%02d", nSec);
              break;
            case 't':
              sPart = nHour > 12 ? L"pm" : L"am";
              break;
          }
          i += 2;
        } else if (remaining == 2 || format[i + 3] != c) {
          switch (c) {
            case 'm':
              i += 3;
              if (IsValidMonth(nMonth))
                sPart += kMonths[nMonth - 1];
              break;
            default:
              i += 3;
              sPart += c;
              sPart += c;
              sPart += c;
              break;
          }
        } else if (remaining == 3 || format[i + 4] != c) {
          switch (c) {
            case 'y':
              sPart = WideString::Format(L"%04d", nYear);
              i += 4;
              break;
            case 'm':
              i += 4;
              if (IsValidMonth(nMonth))
                sPart += kFullMonths[nMonth - 1];
              break;
            default:
              i += 4;
              sPart += c;
              sPart += c;
              sPart += c;
              sPart += c;
              break;
          }
        } else {
          i++;
          sPart += c;
        }
        break;
      default:
        i++;
        sPart += c;
        break;
    }

    sRet += sPart;
  }

  return sRet;
}

// function AFNumber_Format(nDec, sepStyle, negStyle, currStyle, strCurrency,
// bCurrencyPrepend)
CJS_Return CJS_PublicMethods::AFNumber_Format(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
#if _FX_OS_ != _FX_OS_ANDROID_
  if (params.size() != 6)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventHandler* pEvent =
      pRuntime->GetCurrentEventContext()->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  WideString& Value = pEvent->Value();
  ByteString strValue = StrTrim(ByteString::FromUnicode(Value));
  if (strValue.IsEmpty())
    return CJS_Return(true);

  int iDec = abs(pRuntime->ToInt32(params[0]));
  int iSepStyle = ValidStyleOrZero(pRuntime->ToInt32(params[1]));
  int iNegStyle = ValidStyleOrZero(pRuntime->ToInt32(params[2]));
  // params[3] is iCurrStyle, it's not used.
  WideString wstrCurrency = pRuntime->ToWideString(params[4]);
  bool bCurrencyPrepend = pRuntime->ToBoolean(params[5]);

  // Processing decimal places
  NormalizeDecimalMark(&strValue);
  double dValue = atof(strValue.c_str());
  if (iDec > 0)
    dValue += kDoubleCorrect;

  // Calculating number string
  bool bNegative;
  int iDec2;
  strValue = CalculateString(dValue, iDec, &iDec2, &bNegative);
  if (strValue.IsEmpty()) {
    dValue = 0;
    strValue = CalculateString(dValue, iDec, &iDec2, &bNegative);
    if (strValue.IsEmpty()) {
      strValue = "0";
      iDec2 = 1;
    }
  }
  ASSERT(iDec2 >= 0);

  // Processing separator style
  if (static_cast<size_t>(iDec2) < strValue.GetLength()) {
    if (IsStyleWithCommaDecimalMark(iSepStyle))
      strValue.Replace(".", ",");

    if (iDec2 == 0)
      strValue.Insert(iDec2, '0');
  }
  if (IsStyleWithDigitSeparator(iSepStyle)) {
    char cSeparator = DigitSeparatorForStyle(iSepStyle);
    for (int iDecPositive = iDec2 - 3; iDecPositive > 0; iDecPositive -= 3)
      strValue.Insert(iDecPositive, cSeparator);
  }

  // Processing currency string
  Value = WideString::FromLocal(strValue.AsStringView());
  if (bCurrencyPrepend)
    Value = wstrCurrency + Value;
  else
    Value = Value + wstrCurrency;

  // Processing negative style
  if (bNegative) {
    if (iNegStyle == 0) {
      Value.InsertAtFront(L'-');
    } else if (iNegStyle == 2 || iNegStyle == 3) {
      Value.InsertAtFront(L'(');
      Value += L')';
    }
    if (iNegStyle == 1 || iNegStyle == 3) {
      if (CJS_Field* fTarget = pEvent->Target_Field()) {
        v8::Local<v8::Array> arColor = pRuntime->NewArray();
        pRuntime->PutArrayElement(arColor, 0, pRuntime->NewString(L"RGB"));
        pRuntime->PutArrayElement(arColor, 1, pRuntime->NewNumber(1));
        pRuntime->PutArrayElement(arColor, 2, pRuntime->NewNumber(0));
        pRuntime->PutArrayElement(arColor, 3, pRuntime->NewNumber(0));
        fTarget->set_text_color(pRuntime, arColor);
      }
    }
  } else {
    if (iNegStyle == 1 || iNegStyle == 3) {
      if (CJS_Field* fTarget = pEvent->Target_Field()) {
        v8::Local<v8::Array> arColor = pRuntime->NewArray();
        pRuntime->PutArrayElement(arColor, 0, pRuntime->NewString(L"RGB"));
        pRuntime->PutArrayElement(arColor, 1, pRuntime->NewNumber(0));
        pRuntime->PutArrayElement(arColor, 2, pRuntime->NewNumber(0));
        pRuntime->PutArrayElement(arColor, 3, pRuntime->NewNumber(0));

        CJS_Return result = fTarget->get_text_color(pRuntime);
        CFX_Color crProp = CJS_Color::ConvertArrayToPWLColor(
            pRuntime, pRuntime->ToArray(result.Return()));
        CFX_Color crColor =
            CJS_Color::ConvertArrayToPWLColor(pRuntime, arColor);
        if (crColor != crProp)
          fTarget->set_text_color(pRuntime, arColor);
      }
    }
  }
#endif
  return CJS_Return(true);
}

// function AFNumber_Keystroke(nDec, sepStyle, negStyle, currStyle, strCurrency,
// bCurrencyPrepend)
CJS_Return CJS_PublicMethods::AFNumber_Keystroke(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() < 2)
    return CJS_Return(false);

  CJS_EventContext* pContext = pRuntime->GetCurrentEventContext();
  CJS_EventHandler* pEvent = pContext->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  WideString& val = pEvent->Value();
  WideString& wstrChange = pEvent->Change();
  WideString wstrValue = val;

  if (pEvent->WillCommit()) {
    WideString swTemp = StrTrim(wstrValue);
    if (swTemp.IsEmpty())
      return CJS_Return(true);

    NormalizeDecimalMarkW(&swTemp);
    if (!IsNumber(swTemp.c_str())) {
      pEvent->Rc() = false;
      WideString sError = JSGetStringFromID(JSMessage::kInvalidInputError);
      AlertIfPossible(pContext, sError.c_str());
      return CJS_Return(sError);
    }
    // It happens after the last keystroke and before validating,
    return CJS_Return(true);
  }

  WideString wstrSelected;
  if (pEvent->SelStart() != -1) {
    wstrSelected = wstrValue.Mid(pEvent->SelStart(),
                                 pEvent->SelEnd() - pEvent->SelStart());
  }

  bool bHasSign = wstrValue.Contains(L'-') && !wstrSelected.Contains(L'-');
  if (bHasSign) {
    // can't insert "change" in front of sign position.
    if (!wstrSelected.IsEmpty() && pEvent->SelStart() == 0) {
      pEvent->Rc() = false;
      return CJS_Return(true);
    }
  }

  int iSepStyle = ValidStyleOrZero(pRuntime->ToInt32(params[1]));
  const wchar_t cSep = DecimalMarkForStyle(iSepStyle);

  bool bHasSep = wstrValue.Contains(cSep);
  for (size_t i = 0; i < wstrChange.GetLength(); ++i) {
    if (wstrChange[i] == cSep) {
      if (bHasSep) {
        pEvent->Rc() = false;
        return CJS_Return(true);
      }
      bHasSep = true;
      continue;
    }
    if (wstrChange[i] == L'-') {
      if (bHasSign) {
        pEvent->Rc() = false;
        return CJS_Return(true);
      }
      // sign's position is not correct
      if (i != 0) {
        pEvent->Rc() = false;
        return CJS_Return(true);
      }
      if (pEvent->SelStart() != 0) {
        pEvent->Rc() = false;
        return CJS_Return(true);
      }
      bHasSign = true;
      continue;
    }

    if (!std::iswdigit(wstrChange[i])) {
      pEvent->Rc() = false;
      return CJS_Return(true);
    }
  }

  val = CalcMergedString(pEvent, wstrValue, wstrChange);
  return CJS_Return(true);
}

// function AFPercent_Format(nDec, sepStyle)
CJS_Return CJS_PublicMethods::AFPercent_Format(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
#if _FX_OS_ != _FX_OS_ANDROID_
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventHandler* pEvent =
      pRuntime->GetCurrentEventContext()->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  WideString& Value = pEvent->Value();
  ByteString strValue = StrTrim(ByteString::FromUnicode(Value));
  if (strValue.IsEmpty())
    return CJS_Return(true);

  int iDec = abs(pRuntime->ToInt32(params[0]));
  int iSepStyle = ValidStyleOrZero(pRuntime->ToInt32(params[1]));

  // for processing decimal places
  double dValue = atof(strValue.c_str());
  dValue *= 100;
  if (iDec > 0)
    dValue += kDoubleCorrect;

  int iDec2;
  int iNegative = 0;
  strValue = fcvt(dValue, iDec, &iDec2, &iNegative);
  if (strValue.IsEmpty()) {
    dValue = 0;
    strValue = fcvt(dValue, iDec, &iDec2, &iNegative);
  }

  if (iDec2 < 0) {
    ByteString zeros;
    {
      pdfium::span<char> zeros_ptr = zeros.GetBuffer(abs(iDec2));
      std::fill(std::begin(zeros_ptr), std::end(zeros_ptr), '0');
    }
    zeros.ReleaseBuffer(abs(iDec2));
    strValue = zeros + strValue;
    iDec2 = 0;
  }
  int iMax = strValue.GetLength();
  if (iDec2 > iMax) {
    for (int iNum = 0; iNum <= iDec2 - iMax; iNum++)
      strValue += '0';

    iMax = iDec2 + 1;
  }

  // for processing seperator style
  if (iDec2 < iMax) {
    char mark = DecimalMarkForStyle(iSepStyle);
    strValue.Insert(iDec2, mark);
    iMax++;

    if (iDec2 == 0)
      strValue.Insert(iDec2, '0');
  }
  if (IsStyleWithDigitSeparator(iSepStyle)) {
    char cSeparator = DigitSeparatorForStyle(iSepStyle);
    for (int iDecPositive = iDec2 - 3; iDecPositive > 0; iDecPositive -= 3) {
      strValue.Insert(iDecPositive, cSeparator);
      iMax++;
    }
  }

  // negative mark
  if (iNegative)
    strValue.InsertAtFront('-');

  strValue += '%';
  Value = WideString::FromLocal(strValue.AsStringView());
#endif
  return CJS_Return(true);
}

// AFPercent_Keystroke(nDec, sepStyle)
CJS_Return CJS_PublicMethods::AFPercent_Keystroke(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return AFNumber_Keystroke(pRuntime, params);
}

// function AFDate_FormatEx(cFormat)
CJS_Return CJS_PublicMethods::AFDate_FormatEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventContext* pContext = pRuntime->GetCurrentEventContext();
  CJS_EventHandler* pEvent = pContext->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  WideString& val = pEvent->Value();
  WideString strValue = val;
  if (strValue.IsEmpty())
    return CJS_Return(true);

  WideString sFormat = pRuntime->ToWideString(params[0]);
  double dDate;
  if (strValue.Contains(L"GMT")) {
    // for GMT format time
    // such as "Tue Aug 11 14:24:16 GMT+08002009"
    dDate = MakeInterDate(strValue);
  } else {
    dDate = MakeRegularDate(strValue, sFormat, nullptr);
  }

  if (std::isnan(dDate)) {
    WideString swMsg = WideString::Format(
        JSGetStringFromID(JSMessage::kParseDateError).c_str(), sFormat.c_str());
    AlertIfPossible(pContext, swMsg.c_str());
    return CJS_Return(false);
  }

  val = MakeFormatDate(dDate, sFormat);
  return CJS_Return(true);
}

double CJS_PublicMethods::MakeInterDate(const WideString& strValue) {
  std::vector<WideString> wsArray;
  WideString sTemp;
  for (const auto& c : strValue) {
    if (c == L' ' || c == L':') {
      wsArray.push_back(sTemp);
      sTemp.clear();
      continue;
    }
    sTemp += c;
  }
  wsArray.push_back(sTemp);
  if (wsArray.size() != 8)
    return 0;

  int nMonth = 1;
  sTemp = wsArray[1];
  for (size_t i = 0; i < FX_ArraySize(kMonths); ++i) {
    if (sTemp.Compare(kMonths[i]) == 0) {
      nMonth = i + 1;
      break;
    }
  }

  int nDay = FX_atof(wsArray[2].AsStringView());
  int nHour = FX_atof(wsArray[3].AsStringView());
  int nMin = FX_atof(wsArray[4].AsStringView());
  int nSec = FX_atof(wsArray[5].AsStringView());
  int nYear = FX_atof(wsArray[7].AsStringView());
  double dRet = JS_MakeDate(JS_MakeDay(nYear, nMonth - 1, nDay),
                            JS_MakeTime(nHour, nMin, nSec, 0));
  if (std::isnan(dRet))
    dRet = JS_DateParse(strValue);

  return dRet;
}

// AFDate_KeystrokeEx(cFormat)
CJS_Return CJS_PublicMethods::AFDate_KeystrokeEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1) {
    return CJS_Return(
        WideString(L"AFDate_KeystrokeEx's parameters' size r not correct"));
  }

  CJS_EventContext* pContext = pRuntime->GetCurrentEventContext();
  CJS_EventHandler* pEvent = pContext->GetEventHandler();
  if (pEvent->WillCommit()) {
    if (!pEvent->m_pValue)
      return CJS_Return(false);

    const WideString& strValue = pEvent->Value();
    if (strValue.IsEmpty())
      return CJS_Return(true);

    WideString sFormat = pRuntime->ToWideString(params[0]);
    bool bWrongFormat = false;
    double dRet = MakeRegularDate(strValue, sFormat, &bWrongFormat);
    if (bWrongFormat || std::isnan(dRet)) {
      WideString swMsg = WideString::Format(
          JSGetStringFromID(JSMessage::kParseDateError).c_str(),
          sFormat.c_str());
      AlertIfPossible(pContext, swMsg.c_str());
      pEvent->Rc() = false;
      return CJS_Return(true);
    }
  }
  return CJS_Return(true);
}

CJS_Return CJS_PublicMethods::AFDate_Format(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  static constexpr const wchar_t* cFormats[] = {L"m/d",
                                                L"m/d/yy",
                                                L"mm/dd/yy",
                                                L"mm/yy",
                                                L"d-mmm",
                                                L"d-mmm-yy",
                                                L"dd-mmm-yy",
                                                L"yy-mm-dd",
                                                L"mmm-yy",
                                                L"mmmm-yy",
                                                L"mmm d, yyyy",
                                                L"mmmm d, yyyy",
                                                L"m/d/yy h:MM tt",
                                                L"m/d/yy HH:MM"};

  int iIndex =
      WithinBoundsOrZero(pRuntime->ToInt32(params[0]), FX_ArraySize(cFormats));
  std::vector<v8::Local<v8::Value>> newParams;
  newParams.push_back(pRuntime->NewString(cFormats[iIndex]));
  return AFDate_FormatEx(pRuntime, newParams);
}

// AFDate_KeystrokeEx(cFormat)
CJS_Return CJS_PublicMethods::AFDate_Keystroke(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  static constexpr const wchar_t* cFormats[] = {L"m/d",
                                                L"m/d/yy",
                                                L"mm/dd/yy",
                                                L"mm/yy",
                                                L"d-mmm",
                                                L"d-mmm-yy",
                                                L"dd-mmm-yy",
                                                L"yy-mm-dd",
                                                L"mmm-yy",
                                                L"mmmm-yy",
                                                L"mmm d, yyyy",
                                                L"mmmm d, yyyy",
                                                L"m/d/yy h:MM tt",
                                                L"m/d/yy HH:MM"};

  int iIndex =
      WithinBoundsOrZero(pRuntime->ToInt32(params[0]), FX_ArraySize(cFormats));
  std::vector<v8::Local<v8::Value>> newParams;
  newParams.push_back(pRuntime->NewString(cFormats[iIndex]));
  return AFDate_KeystrokeEx(pRuntime, newParams);
}

// function AFTime_Format(ptf)
CJS_Return CJS_PublicMethods::AFTime_Format(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  static constexpr const wchar_t* cFormats[] = {L"HH:MM", L"h:MM tt",
                                                L"HH:MM:ss", L"h:MM:ss tt"};

  int iIndex =
      WithinBoundsOrZero(pRuntime->ToInt32(params[0]), FX_ArraySize(cFormats));
  std::vector<v8::Local<v8::Value>> newParams;
  newParams.push_back(pRuntime->NewString(cFormats[iIndex]));
  return AFDate_FormatEx(pRuntime, newParams);
}

CJS_Return CJS_PublicMethods::AFTime_Keystroke(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  static constexpr const wchar_t* cFormats[] = {L"HH:MM", L"h:MM tt",
                                                L"HH:MM:ss", L"h:MM:ss tt"};

  int iIndex =
      WithinBoundsOrZero(pRuntime->ToInt32(params[0]), FX_ArraySize(cFormats));
  std::vector<v8::Local<v8::Value>> newParams;
  newParams.push_back(pRuntime->NewString(cFormats[iIndex]));
  return AFDate_KeystrokeEx(pRuntime, newParams);
}

CJS_Return CJS_PublicMethods::AFTime_FormatEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return AFDate_FormatEx(pRuntime, params);
}

CJS_Return CJS_PublicMethods::AFTime_KeystrokeEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return AFDate_KeystrokeEx(pRuntime, params);
}

// function AFSpecial_Format(psf)
CJS_Return CJS_PublicMethods::AFSpecial_Format(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventHandler* pEvent =
      pRuntime->GetCurrentEventContext()->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  const WideString& wsSource = pEvent->Value();
  WideString wsFormat;
  switch (pRuntime->ToInt32(params[0])) {
    case 0:
      wsFormat = L"99999";
      break;
    case 1:
      wsFormat = L"99999-9999";
      break;
    case 2:
      if (CJS_Util::printx(L"9999999999", wsSource).GetLength() >= 10)
        wsFormat = L"(999) 999-9999";
      else
        wsFormat = L"999-9999";
      break;
    case 3:
      wsFormat = L"999-99-9999";
      break;
  }

  pEvent->Value() = CJS_Util::printx(wsFormat, wsSource);
  return CJS_Return(true);
}

// function AFSpecial_KeystrokeEx(mask)
CJS_Return CJS_PublicMethods::AFSpecial_KeystrokeEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() < 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventContext* pContext = pRuntime->GetCurrentEventContext();
  CJS_EventHandler* pEvent = pContext->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  const WideString& valEvent = pEvent->Value();
  WideString wstrMask = pRuntime->ToWideString(params[0]);
  if (wstrMask.IsEmpty())
    return CJS_Return(true);

  if (pEvent->WillCommit()) {
    if (valEvent.IsEmpty())
      return CJS_Return(true);

    size_t iIndexMask = 0;
    for (; iIndexMask < valEvent.GetLength(); ++iIndexMask) {
      if (!MaskSatisfied(valEvent[iIndexMask], wstrMask[iIndexMask]))
        break;
    }

    if (iIndexMask != wstrMask.GetLength() ||
        (iIndexMask != valEvent.GetLength() && wstrMask.GetLength() != 0)) {
      AlertIfPossible(pContext,
                      JSGetStringFromID(JSMessage::kInvalidInputError).c_str());
      pEvent->Rc() = false;
    }
    return CJS_Return(true);
  }

  WideString& wideChange = pEvent->Change();
  if (wideChange.IsEmpty())
    return CJS_Return(true);

  WideString wChange = wideChange;
  size_t iIndexMask = pEvent->SelStart();
  size_t combined_len = valEvent.GetLength() + wChange.GetLength() +
                        pEvent->SelStart() - pEvent->SelEnd();
  if (combined_len > wstrMask.GetLength()) {
    AlertIfPossible(pContext,
                    JSGetStringFromID(JSMessage::kParamTooLongError).c_str());
    pEvent->Rc() = false;
    return CJS_Return(true);
  }

  if (iIndexMask >= wstrMask.GetLength() && !wChange.IsEmpty()) {
    AlertIfPossible(pContext,
                    JSGetStringFromID(JSMessage::kParamTooLongError).c_str());
    pEvent->Rc() = false;
    return CJS_Return(true);
  }

  for (size_t i = 0; i < wChange.GetLength(); ++i) {
    if (iIndexMask >= wstrMask.GetLength()) {
      AlertIfPossible(pContext,
                      JSGetStringFromID(JSMessage::kParamTooLongError).c_str());
      pEvent->Rc() = false;
      return CJS_Return(true);
    }
    wchar_t wMask = wstrMask[iIndexMask];
    if (!IsReservedMaskChar(wMask))
      wChange.SetAt(i, wMask);

    if (!MaskSatisfied(wChange[i], wMask)) {
      pEvent->Rc() = false;
      return CJS_Return(true);
    }
    iIndexMask++;
  }
  wideChange = wChange;
  return CJS_Return(true);
}

// function AFSpecial_Keystroke(psf)
CJS_Return CJS_PublicMethods::AFSpecial_Keystroke(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventHandler* pEvent =
      pRuntime->GetCurrentEventContext()->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  const char* cFormat = "";
  switch (pRuntime->ToInt32(params[0])) {
    case 0:
      cFormat = "99999";
      break;
    case 1:
      cFormat = "999999999";
      break;
    case 2:
      if (pEvent->Value().GetLength() + pEvent->Change().GetLength() > 7)
        cFormat = "9999999999";
      else
        cFormat = "9999999";
      break;
    case 3:
      cFormat = "999999999";
      break;
  }

  std::vector<v8::Local<v8::Value>> params2;
  params2.push_back(pRuntime->NewString(cFormat));
  return AFSpecial_KeystrokeEx(pRuntime, params2);
}

CJS_Return CJS_PublicMethods::AFMergeChange(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventHandler* pEventHandler =
      pRuntime->GetCurrentEventContext()->GetEventHandler();

  WideString swValue;
  if (pEventHandler->m_pValue)
    swValue = pEventHandler->Value();

  if (pEventHandler->WillCommit())
    return CJS_Return(pRuntime->NewString(swValue.c_str()));

  WideString merged =
      CalcMergedString(pEventHandler, swValue, pEventHandler->Change());
  return CJS_Return(pRuntime->NewString(merged.c_str()));
}

CJS_Return CJS_PublicMethods::AFParseDateEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString sValue = pRuntime->ToWideString(params[0]);
  WideString sFormat = pRuntime->ToWideString(params[1]);
  double dDate = MakeRegularDate(sValue, sFormat, nullptr);
  if (std::isnan(dDate)) {
    WideString swMsg = WideString::Format(
        JSGetStringFromID(JSMessage::kParseDateError).c_str(), sFormat.c_str());
    AlertIfPossible(pRuntime->GetCurrentEventContext(), swMsg.c_str());
    return CJS_Return(false);
  }
  return CJS_Return(pRuntime->NewNumber(dDate));
}

CJS_Return CJS_PublicMethods::AFSimple(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  return CJS_Return(pRuntime->NewNumber(static_cast<double>(AF_Simple(
      pRuntime->ToWideString(params[0]).c_str(), pRuntime->ToDouble(params[1]),
      pRuntime->ToDouble(params[2])))));
}

CJS_Return CJS_PublicMethods::AFMakeNumber(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString ws = pRuntime->ToWideString(params[0]);
  NormalizeDecimalMarkW(&ws);

  v8::Local<v8::Value> val =
      pRuntime->MaybeCoerceToNumber(pRuntime->NewString(ws.c_str()));
  if (!val->IsNumber())
    return CJS_Return(pRuntime->NewNumber(0));
  return CJS_Return(val);
}

CJS_Return CJS_PublicMethods::AFSimple_Calculate(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  if ((params[1].IsEmpty() || !params[1]->IsArray()) && !params[1]->IsString())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CPDFSDK_InterForm* pReaderInterForm =
      pRuntime->GetFormFillEnv()->GetInterForm();
  CPDF_InterForm* pInterForm = pReaderInterForm->GetInterForm();

  WideString sFunction = pRuntime->ToWideString(params[0]);
  double dValue = wcscmp(sFunction.c_str(), L"PRD") == 0 ? 1.0 : 0.0;

  v8::Local<v8::Array> FieldNameArray =
      AF_MakeArrayFromList(pRuntime, params[1]);
  int nFieldsCount = 0;
  for (size_t i = 0; i < pRuntime->GetArrayLength(FieldNameArray); ++i) {
    WideString wsFieldName =
        pRuntime->ToWideString(pRuntime->GetArrayElement(FieldNameArray, i));

    for (size_t j = 0; j < pInterForm->CountFields(wsFieldName); ++j) {
      CPDF_FormField* pFormField = pInterForm->GetField(j, wsFieldName);
      if (!pFormField)
        continue;

      double dTemp = 0.0;
      switch (pFormField->GetFieldType()) {
        case FormFieldType::kTextField:
        case FormFieldType::kComboBox: {
          WideString trimmed = pFormField->GetValue();
          trimmed.TrimRight();
          trimmed.TrimLeft();
          dTemp = FX_atof(trimmed.AsStringView());
          break;
        }
        case FormFieldType::kPushButton:
          break;
        case FormFieldType::kCheckBox:
        case FormFieldType::kRadioButton:
          for (int c = 0; c < pFormField->CountControls(); ++c) {
            CPDF_FormControl* pFormCtrl = pFormField->GetControl(c);
            if (!pFormField || !pFormCtrl->IsChecked())
              continue;

            WideString trimmed = pFormCtrl->GetExportValue();
            trimmed.TrimRight();
            trimmed.TrimLeft();
            dTemp = FX_atof(trimmed.AsStringView());
            break;
          }
          break;
        case FormFieldType::kListBox:
          if (pFormField->CountSelectedItems() <= 1) {
            WideString trimmed = pFormField->GetValue();
            trimmed.TrimRight();
            trimmed.TrimLeft();
            dTemp = FX_atof(trimmed.AsStringView());
          }
          break;
        default:
          break;
      }

      if (i == 0 && j == 0 &&
          (wcscmp(sFunction.c_str(), L"MIN") == 0 ||
           wcscmp(sFunction.c_str(), L"MAX") == 0)) {
        dValue = dTemp;
      }
      dValue = AF_Simple(sFunction.c_str(), dValue, dTemp);

      nFieldsCount++;
    }
  }

  if (wcscmp(sFunction.c_str(), L"AVG") == 0 && nFieldsCount > 0)
    dValue /= nFieldsCount;

  dValue = floor(dValue * FXSYS_pow(10, 6) + 0.49) / FXSYS_pow(10, 6);

  CJS_EventContext* pContext = pRuntime->GetCurrentEventContext();
  if (pContext->GetEventHandler()->m_pValue) {
    pContext->GetEventHandler()->Value() =
        pRuntime->ToWideString(pRuntime->NewNumber(dValue));
  }

  return CJS_Return(true);
}

/* This function validates the current event to ensure that its value is
** within the specified range. */
CJS_Return CJS_PublicMethods::AFRange_Validate(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 4)
    CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_EventContext* pContext = pRuntime->GetCurrentEventContext();
  CJS_EventHandler* pEvent = pContext->GetEventHandler();
  if (!pEvent->m_pValue)
    return CJS_Return(false);

  if (pEvent->Value().IsEmpty())
    return CJS_Return(true);

  double dEentValue = atof(ByteString::FromUnicode(pEvent->Value()).c_str());
  bool bGreaterThan = pRuntime->ToBoolean(params[0]);
  double dGreaterThan = pRuntime->ToDouble(params[1]);
  bool bLessThan = pRuntime->ToBoolean(params[2]);
  double dLessThan = pRuntime->ToDouble(params[3]);
  WideString swMsg;

  if (bGreaterThan && bLessThan) {
    if (dEentValue < dGreaterThan || dEentValue > dLessThan)
      swMsg = WideString::Format(
          JSGetStringFromID(JSMessage::kRangeBetweenError).c_str(),
          pRuntime->ToWideString(params[1]).c_str(),
          pRuntime->ToWideString(params[3]).c_str());
  } else if (bGreaterThan) {
    if (dEentValue < dGreaterThan)
      swMsg = WideString::Format(
          JSGetStringFromID(JSMessage::kRangeGreaterError).c_str(),
          pRuntime->ToWideString(params[1]).c_str());
  } else if (bLessThan) {
    if (dEentValue > dLessThan)
      swMsg = WideString::Format(
          JSGetStringFromID(JSMessage::kRangeLessError).c_str(),
          pRuntime->ToWideString(params[3]).c_str());
  }

  if (!swMsg.IsEmpty()) {
    AlertIfPossible(pContext, swMsg.c_str());
    pEvent->Rc() = false;
  }
  return CJS_Return(true);
}

CJS_Return CJS_PublicMethods::AFExtractNums(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString str = pRuntime->ToWideString(params[0]);
  if (str.GetLength() > 0 && IsDigitSeparatorOrDecimalMark(str[0]))
    str.InsertAtFront(L'0');

  WideString sPart;
  v8::Local<v8::Array> nums = pRuntime->NewArray();
  int nIndex = 0;
  for (const auto& wc : str) {
    if (std::iswdigit(wc)) {
      sPart += wc;
    } else if (sPart.GetLength() > 0) {
      pRuntime->PutArrayElement(nums, nIndex,
                                pRuntime->NewString(sPart.c_str()));
      sPart.clear();
      nIndex++;
    }
  }
  if (sPart.GetLength() > 0)
    pRuntime->PutArrayElement(nums, nIndex, pRuntime->NewString(sPart.c_str()));

  if (pRuntime->GetArrayLength(nums) > 0)
    return CJS_Return(nums);
  return CJS_Return(pRuntime->NewUndefined());
}
