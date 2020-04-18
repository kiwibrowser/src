// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_EVENT_H_
#define FXJS_CJS_EVENT_H_

#include "fxjs/JS_Define.h"

class CJS_Event : public CJS_Object {
 public:
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  explicit CJS_Event(v8::Local<v8::Object> pObject);
  ~CJS_Event() override;

  JS_STATIC_PROP(change, change, CJS_Event);
  JS_STATIC_PROP(changeEx, change_ex, CJS_Event);
  JS_STATIC_PROP(commitKey, commit_key, CJS_Event);
  JS_STATIC_PROP(fieldFull, field_full, CJS_Event);
  JS_STATIC_PROP(keyDown, key_down, CJS_Event);
  JS_STATIC_PROP(modifier, modifier, CJS_Event);
  JS_STATIC_PROP(name, name, CJS_Event);
  JS_STATIC_PROP(rc, rc, CJS_Event);
  JS_STATIC_PROP(richChange, rich_change, CJS_Event);
  JS_STATIC_PROP(richChangeEx, rich_change_ex, CJS_Event);
  JS_STATIC_PROP(richValue, rich_value, CJS_Event);
  JS_STATIC_PROP(selEnd, sel_end, CJS_Event);
  JS_STATIC_PROP(selStart, sel_start, CJS_Event);
  JS_STATIC_PROP(shift, shift, CJS_Event);
  JS_STATIC_PROP(source, source, CJS_Event);
  JS_STATIC_PROP(target, target, CJS_Event);
  JS_STATIC_PROP(targetName, target_name, CJS_Event);
  JS_STATIC_PROP(type, type, CJS_Event);
  JS_STATIC_PROP(value, value, CJS_Event);
  JS_STATIC_PROP(willCommit, will_commit, CJS_Event);

 private:
  static int ObjDefnID;
  static const char kName[];
  static const JSPropertySpec PropertySpecs[];

  CJS_Return get_change(CJS_Runtime* pRuntime);
  CJS_Return set_change(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_change_ex(CJS_Runtime* pRuntime);
  CJS_Return set_change_ex(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_commit_key(CJS_Runtime* pRuntime);
  CJS_Return set_commit_key(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_field_full(CJS_Runtime* pRuntime);
  CJS_Return set_field_full(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_key_down(CJS_Runtime* pRuntime);
  CJS_Return set_key_down(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_modifier(CJS_Runtime* pRuntime);
  CJS_Return set_modifier(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_name(CJS_Runtime* pRuntime);
  CJS_Return set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rc(CJS_Runtime* pRuntime);
  CJS_Return set_rc(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rich_change(CJS_Runtime* pRuntime);
  CJS_Return set_rich_change(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rich_change_ex(CJS_Runtime* pRuntime);
  CJS_Return set_rich_change_ex(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rich_value(CJS_Runtime* pRuntime);
  CJS_Return set_rich_value(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_sel_end(CJS_Runtime* pRuntime);
  CJS_Return set_sel_end(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_sel_start(CJS_Runtime* pRuntime);
  CJS_Return set_sel_start(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_shift(CJS_Runtime* pRuntime);
  CJS_Return set_shift(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_source(CJS_Runtime* pRuntime);
  CJS_Return set_source(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_target(CJS_Runtime* pRuntime);
  CJS_Return set_target(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_target_name(CJS_Runtime* pRuntime);
  CJS_Return set_target_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_type(CJS_Runtime* pRuntime);
  CJS_Return set_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_value(CJS_Runtime* pRuntime);
  CJS_Return set_value(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_will_commit(CJS_Runtime* pRuntime);
  CJS_Return set_will_commit(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);
};

#endif  // FXJS_CJS_EVENT_H_
