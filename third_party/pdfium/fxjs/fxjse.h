// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_FXJSE_H_
#define FXJS_FXJSE_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "v8/include/v8.h"

class CFXJSE_Arguments;
class CFXJSE_Value;
class CJS_Return;

// C++ object which is retrieved from v8 object's slot.
class CFXJSE_HostObject {
 public:
  virtual ~CFXJSE_HostObject() {}

  // Small layering violation here, but we need to distinguish between the
  // two kinds of subclasses.
  enum Type { kXFA, kFM2JS };
  Type type() const { return type_; }

 protected:
  explicit CFXJSE_HostObject(Type type) { type_ = type; }

  Type type_;
};

typedef CJS_Return (*FXJSE_MethodCallback)(
    const v8::FunctionCallbackInfo<v8::Value>& info,
    const WideString& functionName);
typedef void (*FXJSE_FuncCallback)(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args);
typedef void (*FXJSE_PropAccessor)(CFXJSE_Value* pObject,
                                   const ByteStringView& szPropName,
                                   CFXJSE_Value* pValue);
typedef int32_t (*FXJSE_PropTypeGetter)(CFXJSE_Value* pObject,
                                        const ByteStringView& szPropName,
                                        bool bQueryIn);

enum FXJSE_ClassPropTypes {
  FXJSE_ClassPropType_None,
  FXJSE_ClassPropType_Property,
  FXJSE_ClassPropType_Method
};

struct FXJSE_FUNCTION_DESCRIPTOR {
  const char* name;
  FXJSE_FuncCallback callbackProc;
};

struct FXJSE_CLASS_DESCRIPTOR {
  const char* name;
  const FXJSE_FUNCTION_DESCRIPTOR* methods;
  int32_t methNum;
  FXJSE_PropTypeGetter dynPropTypeGetter;
  FXJSE_PropAccessor dynPropGetter;
  FXJSE_PropAccessor dynPropSetter;
  FXJSE_MethodCallback dynMethodCall;
};

extern const FXJSE_CLASS_DESCRIPTOR GlobalClassDescriptor;
extern const FXJSE_CLASS_DESCRIPTOR NormalClassDescriptor;
extern const FXJSE_CLASS_DESCRIPTOR VariablesClassDescriptor;
extern const FXJSE_CLASS_DESCRIPTOR kFormCalcFM2JSDescriptor;

void FXJSE_ThrowMessage(const ByteStringView& utf8Message);

#endif  // FXJS_FXJSE_H_
