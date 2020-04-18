// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/JS_Define.h"

#include <time.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "fxjs/cjs_document.h"
#include "fxjs/cjs_object.h"

namespace {

double GetLocalTZA() {
  if (!FSDK_IsSandBoxPolicyEnabled(FPDF_POLICY_MACHINETIME_ACCESS))
    return 0;
  time_t t = 0;
  time(&t);
  localtime(&t);
#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
  // In gcc 'timezone' is a global variable declared in time.h. In VC++, that
  // variable was removed in VC++ 2015, with _get_timezone replacing it.
  long timezone = 0;
  _get_timezone(&timezone);
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
  return (double)(-(timezone * 1000));
}

int GetDaylightSavingTA(double d) {
  if (!FSDK_IsSandBoxPolicyEnabled(FPDF_POLICY_MACHINETIME_ACCESS))
    return 0;
  time_t t = (time_t)(d / 1000);
  struct tm* tmp = localtime(&t);
  if (!tmp)
    return 0;
  if (tmp->tm_isdst > 0)
    // One hour.
    return (int)60 * 60 * 1000;
  return 0;
}

double Mod(double x, double y) {
  double r = fmod(x, y);
  if (r < 0)
    r += y;
  return r;
}

bool IsLeapYear(int year) {
  return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 != 0));
}

int DayFromYear(int y) {
  return (int)(365 * (y - 1970.0) + floor((y - 1969.0) / 4) -
               floor((y - 1901.0) / 100) + floor((y - 1601.0) / 400));
}

double TimeFromYear(int y) {
  return 86400000.0 * DayFromYear(y);
}

static const uint16_t daysMonth[12] = {0,   31,  59,  90,  120, 151,
                                       181, 212, 243, 273, 304, 334};
static const uint16_t leapDaysMonth[12] = {0,   31,  60,  91,  121, 152,
                                           182, 213, 244, 274, 305, 335};

double TimeFromYearMonth(int y, int m) {
  const uint16_t* pMonth = IsLeapYear(y) ? leapDaysMonth : daysMonth;
  return TimeFromYear(y) + ((double)pMonth[m]) * 86400000;
}

int Day(double t) {
  return static_cast<int>(floor(t / 86400000.0));
}

int YearFromTime(double t) {
  // estimate the time.
  int y = 1970 + static_cast<int>(t / (365.2425 * 86400000.0));
  if (TimeFromYear(y) <= t) {
    while (TimeFromYear(y + 1) <= t)
      y++;
  } else {
    while (TimeFromYear(y) > t)
      y--;
  }
  return y;
}

int DayWithinYear(double t) {
  int year = YearFromTime(t);
  int day = Day(t);
  return day - DayFromYear(year);
}

int MonthFromTime(double t) {
  int day = DayWithinYear(t);
  int year = YearFromTime(t);
  if (0 <= day && day < 31)
    return 0;
  if (31 <= day && day < 59 + IsLeapYear(year))
    return 1;
  if ((59 + IsLeapYear(year)) <= day && day < (90 + IsLeapYear(year)))
    return 2;
  if ((90 + IsLeapYear(year)) <= day && day < (120 + IsLeapYear(year)))
    return 3;
  if ((120 + IsLeapYear(year)) <= day && day < (151 + IsLeapYear(year)))
    return 4;
  if ((151 + IsLeapYear(year)) <= day && day < (181 + IsLeapYear(year)))
    return 5;
  if ((181 + IsLeapYear(year)) <= day && day < (212 + IsLeapYear(year)))
    return 6;
  if ((212 + IsLeapYear(year)) <= day && day < (243 + IsLeapYear(year)))
    return 7;
  if ((243 + IsLeapYear(year)) <= day && day < (273 + IsLeapYear(year)))
    return 8;
  if ((273 + IsLeapYear(year)) <= day && day < (304 + IsLeapYear(year)))
    return 9;
  if ((304 + IsLeapYear(year)) <= day && day < (334 + IsLeapYear(year)))
    return 10;
  if ((334 + IsLeapYear(year)) <= day && day < (365 + IsLeapYear(year)))
    return 11;

  return -1;
}

int DateFromTime(double t) {
  int day = DayWithinYear(t);
  int year = YearFromTime(t);
  int leap = IsLeapYear(year);
  int month = MonthFromTime(t);
  switch (month) {
    case 0:
      return day + 1;
    case 1:
      return day - 30;
    case 2:
      return day - 58 - leap;
    case 3:
      return day - 89 - leap;
    case 4:
      return day - 119 - leap;
    case 5:
      return day - 150 - leap;
    case 6:
      return day - 180 - leap;
    case 7:
      return day - 211 - leap;
    case 8:
      return day - 242 - leap;
    case 9:
      return day - 272 - leap;
    case 10:
      return day - 303 - leap;
    case 11:
      return day - 333 - leap;
    default:
      return 0;
  }
}

}  // namespace

void JSDestructor(v8::Local<v8::Object> obj) {
  CFXJS_Engine::SetObjectPrivate(obj, nullptr);
}

double JS_GetDateTime() {
  if (!FSDK_IsSandBoxPolicyEnabled(FPDF_POLICY_MACHINETIME_ACCESS))
    return 0;
  time_t t = time(nullptr);
  struct tm* pTm = localtime(&t);

  int year = pTm->tm_year + 1900;
  double t1 = TimeFromYear(year);

  return t1 + pTm->tm_yday * 86400000.0 + pTm->tm_hour * 3600000.0 +
         pTm->tm_min * 60000.0 + pTm->tm_sec * 1000.0;
}

int JS_GetYearFromTime(double dt) {
  return YearFromTime(dt);
}

int JS_GetMonthFromTime(double dt) {
  return MonthFromTime(dt);
}

int JS_GetDayFromTime(double dt) {
  return DateFromTime(dt);
}

int JS_GetHourFromTime(double dt) {
  return (int)Mod(floor(dt / (60 * 60 * 1000)), 24);
}

int JS_GetMinFromTime(double dt) {
  return (int)Mod(floor(dt / (60 * 1000)), 60);
}

int JS_GetSecFromTime(double dt) {
  return (int)Mod(floor(dt / 1000), 60);
}

double JS_LocalTime(double d) {
  return d + GetLocalTZA() + GetDaylightSavingTA(d);
}

double JS_DateParse(const WideString& str) {
  v8::Isolate* pIsolate = v8::Isolate::GetCurrent();
  v8::Isolate::Scope isolate_scope(pIsolate);
  v8::HandleScope scope(pIsolate);

  v8::Local<v8::Context> context = pIsolate->GetCurrentContext();

  // Use the built-in object method.
  v8::Local<v8::Value> v =
      context->Global()
          ->Get(context, v8::String::NewFromUtf8(pIsolate, "Date",
                                                 v8::NewStringType::kNormal)
                             .ToLocalChecked())
          .ToLocalChecked();
  if (v->IsObject()) {
    v8::Local<v8::Object> o = v->ToObject(context).ToLocalChecked();
    v = o->Get(context, v8::String::NewFromUtf8(pIsolate, "parse",
                                                v8::NewStringType::kNormal)
                            .ToLocalChecked())
            .ToLocalChecked();
    if (v->IsFunction()) {
      v8::Local<v8::Function> funC = v8::Local<v8::Function>::Cast(v);
      const int argc = 1;
      v8::Local<v8::Value> timeStr =
          CJS_Runtime::RuntimeFromIsolateCurrentContext(pIsolate)->NewString(
              str.AsStringView());
      v8::Local<v8::Value> argv[argc] = {timeStr};
      v = funC->Call(context, context->Global(), argc, argv).ToLocalChecked();
      if (v->IsNumber()) {
        double date = v->ToNumber(context).ToLocalChecked()->Value();
        if (!std::isfinite(date))
          return date;
        return JS_LocalTime(date);
      }
    }
  }
  return 0;
}

double JS_MakeDay(int nYear, int nMonth, int nDate) {
  double y = static_cast<double>(nYear);
  double m = static_cast<double>(nMonth);
  double dt = static_cast<double>(nDate);
  double ym = y + floor(m / 12);
  double mn = Mod(m, 12);
  double t = TimeFromYearMonth(static_cast<int>(ym), static_cast<int>(mn));
  if (YearFromTime(t) != ym || MonthFromTime(t) != mn || DateFromTime(t) != 1)
    return std::nan("");

  return Day(t) + dt - 1;
}

double JS_MakeTime(int nHour, int nMin, int nSec, int nMs) {
  double h = static_cast<double>(nHour);
  double m = static_cast<double>(nMin);
  double s = static_cast<double>(nSec);
  double milli = static_cast<double>(nMs);
  return h * 3600000 + m * 60000 + s * 1000 + milli;
}

double JS_MakeDate(double day, double time) {
  if (!std::isfinite(day) || !std::isfinite(time))
    return std::nan("");

  return day * 86400000 + time;
}

std::vector<v8::Local<v8::Value>> ExpandKeywordParams(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& originals,
    size_t nKeywords,
    ...) {
  ASSERT(nKeywords);

  std::vector<v8::Local<v8::Value>> result(nKeywords, v8::Local<v8::Value>());
  size_t size = std::min(originals.size(), nKeywords);
  for (size_t i = 0; i < size; ++i)
    result[i] = originals[i];

  if (originals.size() != 1 || !originals[0]->IsObject() ||
      originals[0]->IsArray()) {
    return result;
  }
  result[0] = v8::Local<v8::Value>();  // Make unknown.

  v8::Local<v8::Object> pObj = pRuntime->ToObject(originals[0]);
  va_list ap;
  va_start(ap, nKeywords);
  for (size_t i = 0; i < nKeywords; ++i) {
    const wchar_t* property = va_arg(ap, const wchar_t*);
    v8::Local<v8::Value> v8Value = pRuntime->GetObjectProperty(pObj, property);
    if (!v8Value->IsUndefined())
      result[i] = v8Value;
  }
  va_end(ap);

  return result;
}
