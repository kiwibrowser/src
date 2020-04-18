// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_globalconsts.h"

#define GLOBAL_STRING(rt, name, value)                                       \
  (rt)->DefineGlobalConst(                                                   \
      (name), [](const v8::FunctionCallbackInfo<v8::Value>& info) {          \
        info.GetReturnValue().Set(                                           \
            CFXJS_Engine::EngineFromIsolateCurrentContext(info.GetIsolate()) \
                ->NewString(value));                                         \
      })

// static
void CJS_GlobalConsts::DefineJSObjects(CJS_Runtime* pRuntime) {
  GLOBAL_STRING(pRuntime, L"IDS_GREATER_THAN",
                L"Invalid value: must be greater than or equal to % s.");

  GLOBAL_STRING(pRuntime, L"IDS_GT_AND_LT",
                L"Invalid value: must be greater than or equal to % s "
                L"and less than or equal to % s.");

  GLOBAL_STRING(pRuntime, L"IDS_LESS_THAN",
                L"Invalid value: must be less than or equal to % s.");

  GLOBAL_STRING(pRuntime, L"IDS_INVALID_MONTH", L"**Invalid**");
  GLOBAL_STRING(
      pRuntime, L"IDS_INVALID_DATE",
      L"Invalid date / time: please ensure that the date / time exists.Field");

  GLOBAL_STRING(pRuntime, L"IDS_INVALID_VALUE",
                L"The value entered does not match the format of the field");

  GLOBAL_STRING(pRuntime, L"IDS_AM", L"am");
  GLOBAL_STRING(pRuntime, L"IDS_PM", L"pm");
  GLOBAL_STRING(pRuntime, L"IDS_MONTH_INFO",
                L"January[1] February[2] March[3] April[4] May[5] "
                L"June[6] July[7] August[8] September[9] October[10] "
                L"November[11] December[12] Sept[9] Jan[1] Feb[2] Mar[3] "
                L"Apr[4] Jun[6] Jul[7] Aug[8] Sep[9] Oct[10] Nov[11] "
                L"Dec[12]");

  GLOBAL_STRING(pRuntime, L"IDS_STARTUP_CONSOLE_MSG", L"** ^ _ ^ **");
}
