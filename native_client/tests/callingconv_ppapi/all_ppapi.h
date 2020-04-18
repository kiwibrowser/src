/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/c/ppb_url_response_info.h"
#include "ppapi/c/pp_completion_callback.h"

union AllValues {
  void* val_void_ptr;
  char* val_char_ptr;
  uint32_t* val_uint32_t_ptr;

  struct PP_Var* val_struct_PP_Var_ptr;
  struct PP_FileInfo* val_struct_PP_FileInfo_ptr;

  uint16_t val_uint16_t;
  int16_t val_int16_t;
  uint32_t val_uint32_t;
  int32_t val_int32_t;
  uint64_t val_uint64_t;
  int64_t val_int64_t;

  // integers and enums
  PP_Bool val_PP_Bool;
  PP_TouchListType val_PP_TouchListType;
  PP_Instance val_PP_Instance;
  PP_Module val_PP_Modules;
  PP_Resource val_PP_Resouce;
  PP_URLRequestProperty val_PP_URLRequestProperty;
  PP_URLResponseProperty val_PP_URLResponseProperty;
  PP_InputEvent_Type val_PP_InputEvent_Type;

  // this are doubles
  PP_TimeTicks val_PP_TimeTicks;
  PP_Time val_PP_Time;

  struct PP_Var val_struct_PP_Var;
  struct PP_CompletionCallback val_struct_PP_CompletionCallback;
  struct PP_Point val_struct_PP_Point;
  struct PP_TouchPoint val_struct_PP_TouchPoint;
  struct PP_FloatPoint val_struct_PP_FloatPoint;
};
