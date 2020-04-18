/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Note, we do not want to include stdlib.h or stdio.h in order to prevent
// tainting of the object file with gcc specific cruft.
// For example gcc's glibc stdio.h may macro expand certain calls which
// may not be compatible with the way newlib treats them.
// The code in this file is C++ so that we can use by reference
// argument passing which simplifies the testing code.
#include <stdint.h>
extern "C" {
#include "support.h"
}

// Note, FUNCTION_PREFIX and INVOCATION_PREFIX can
// be #defined on the compiler commandline via "-D"
// the multi-indirection is necessary to force argiment
#define TOKEN_CONCAT2(a, b) a ## b
#define TOKEN_CONCAT(a, b) TOKEN_CONCAT2(a, b)
#if defined(FUNCTION_PREFIX)
#define PREFIX_FUNCTION(f) TOKEN_CONCAT(FUNCTION_PREFIX, f)
#else
#define PREFIX_FUNCTION(f) f
#endif
#if defined(INVOCATION_PREFIX)
#define PREFIX_INVOCATION(f) TOKEN_CONCAT(INVOCATION_PREFIX, f)
#else
#define PREFIX_INVOCATION(f) f
#endif

namespace {

void new_test(const char* val) {
  randomize();
  emit_string("========================================");
  emit_string(val);
}

/* ====================================================================== */
// macro magic for defining get_value_XXX functions concisely
#define GET_VALUE(t) \
  t get_value_ ## t() { \
  randomize(); return g_input_values.val_ ## t; }

#define GET_VALUE_PTR(t) \
  t* get_value_ ## t ## _ptr() { \
  randomize(); return g_input_values.val_ ## t ## _ptr; }

#define GET_VALUE_STRUCT_PTR(t) \
  t* get_value_struct_ ## t ## _ptr() { \
    randomize(); return g_input_values.val_struct_  ## t ## _ptr; }

#define GET_VALUE_STRUCT(t) \
  struct t get_value_struct_ ## t() { \
  randomize(); return g_input_values.val_struct_  ## t; }

GET_VALUE(uint16_t)
GET_VALUE(uint32_t)
GET_VALUE(int32_t)
GET_VALUE(int64_t)
GET_VALUE(PP_TouchListType)
GET_VALUE(PP_TimeTicks)
GET_VALUE(PP_Instance)
GET_VALUE(PP_Module)
GET_VALUE(PP_Resource)
GET_VALUE(PP_Time)
GET_VALUE(PP_Bool)
GET_VALUE(PP_FileChooserMode_Dev)
GET_VALUE(PP_URLResponseProperty)
GET_VALUE(PP_URLRequestProperty)
GET_VALUE(PP_InputEvent_Type)
GET_VALUE(PP_LogLevel_Dev)
// function pointer
GET_VALUE(PPB_AudioInput_Callback)

GET_VALUE_PTR(void)
GET_VALUE_PTR(char)
GET_VALUE_PTR(PP_Resource)
GET_VALUE_PTR(uint32_t)

GET_VALUE_STRUCT_PTR(PP_FileInfo)
GET_VALUE_STRUCT_PTR(PP_Var)
GET_VALUE_STRUCT_PTR(PP_URLComponents_Dev)
GET_VALUE_STRUCT_PTR(PP_PrintSettings_Dev)
GET_VALUE_STRUCT_PTR(PP_VideoBitstreamBuffer_Dev)
GET_VALUE_STRUCT_PTR(PP_VideoCaptureDeviceInfo_Dev)

GET_VALUE_STRUCT(PP_CompletionCallback)
GET_VALUE_STRUCT(PP_Var)
GET_VALUE_STRUCT(PP_Point)
GET_VALUE_STRUCT(PP_FloatPoint)
GET_VALUE_STRUCT(PP_TouchPoint)
GET_VALUE_STRUCT(PP_ArrayOutput)

/* ====================================================================== */

void emit_double(const char* m, const double& val) {
  emit_integer(m, *reinterpret_cast<const int64_t*>(&val));
}

void emit_float(const char* m, const float& val) {
  emit_integer(m, *reinterpret_cast<const int32_t*>(&val));
}

// macro magic for defining emit_value_XXX functions concisely
#define EMIT_VALUE_PTR(t) \
  void emit_value_ ## t ## _ptr(const t* val) { emit_pointer(#t "*", val); }

#define EMIT_VALUE_STRUCT_PTR(t) \
  void emit_value_struct_ ## t ## _ptr(const struct t* val) { \
    emit_pointer("struct " #t "*", val); }

#define EMIT_VALUE_INT(t) \
  void emit_value_ ## t(const t val) { emit_integer(#t, val); }

#define EMIT_VALUE_DOUBLE(t)                                               \
  void emit_value_ ## t(const t& val) { emit_double(#t, val); }

EMIT_VALUE_PTR(void)
EMIT_VALUE_PTR(char)
EMIT_VALUE_PTR(uint32_t)
EMIT_VALUE_PTR(PP_Resource)

EMIT_VALUE_STRUCT_PTR(PP_FileInfo)
EMIT_VALUE_STRUCT_PTR(PP_Var)
EMIT_VALUE_STRUCT_PTR(PP_URLComponents_Dev)
EMIT_VALUE_STRUCT_PTR(PP_VideoBitstreamBuffer_Dev)
EMIT_VALUE_STRUCT_PTR(PP_VideoCaptureDeviceInfo_Dev)
EMIT_VALUE_STRUCT_PTR(PP_PrintSettings_Dev)

EMIT_VALUE_INT(uint16_t)
EMIT_VALUE_INT(uint32_t)
EMIT_VALUE_INT(int32_t)
EMIT_VALUE_INT(int64_t)
EMIT_VALUE_INT(PP_Bool)
EMIT_VALUE_INT(PP_TouchListType)
EMIT_VALUE_INT(PP_Instance)
EMIT_VALUE_INT(PP_Module)
EMIT_VALUE_INT(PP_Resource)
EMIT_VALUE_INT(PP_InputEvent_Type)
EMIT_VALUE_INT(PP_URLRequestProperty)
EMIT_VALUE_INT(PP_URLResponseProperty)
EMIT_VALUE_INT(PP_LogLevel_Dev)
EMIT_VALUE_INT(PP_FileChooserMode_Dev)

EMIT_VALUE_DOUBLE(PP_TimeTicks)
EMIT_VALUE_DOUBLE(PP_Time)

// Some 'oddballs' below which do not justify additional macros
void emit_value_PPB_AudioInput_Callback(const PPB_AudioInput_Callback& val) {
  emit_pointer("PPB_AudioInput_Callback", (void*)(intptr_t)val);
}

void emit_value_struct_PP_Var(const struct PP_Var& val) {
  emit_integer("PP_Var.type", val.type);
  emit_integer("PP_Var.padding", val.padding);
  emit_integer("PP_Var.value.as_id", val.value.as_id);
}

void emit_value_struct_PP_ArrayOutput(const struct PP_ArrayOutput& val) {
  emit_pointer("PP_ArrayOuput.GetDataBuffer",
               (void*)(intptr_t)val.GetDataBuffer);
  emit_pointer("PP_ArrayOuput.user_data", val.user_data);
}

void emit_value_struct_PP_FloatPoint(const struct PP_FloatPoint& val) {
  emit_float("PP_FloatPoint.x", val.x);
  emit_float("PP_FloatPoint.y", val.y);
}

void emit_value_struct_PP_Point(const struct PP_Point& val) {
  emit_integer("PP_Point.x", val.x);
  emit_integer("PP_Point.y", val.y);
}

void emit_value_struct_PP_CompletionCallback(
  const struct PP_CompletionCallback& val) {
  // the casting is to work around compiler warnings
  emit_pointer("PP_CompletionCallback.func", (void*)(size_t)val.func);
  emit_pointer("PP_CompletionCallback.user_data", val.user_data);
  emit_integer("PP_CompletionCallback.flags", val.flags);
}

void emit_value_struct_PP_TouchPoint(const struct PP_TouchPoint& val) {
  emit_integer("PP_TouchPoint.id", val.id);
  emit_float("PP_TouchPoint.position.x", val.position.x);
  emit_float("PP_TouchPoint.position.y", val.position.y);
  emit_float("PP_TouchPoint.radius.x", val.radius.x);
  emit_float("PP_TouchPoint.radius.y", val.radius.y);
  emit_float("PP_TouchPoint.rotation_angle", val.rotation_angle);
  emit_float("PP_TouchPoint.pressure", val.pressure);
}

}  // end anonymous namespace
