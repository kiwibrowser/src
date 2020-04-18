// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_annot.h"

#include "fxjs/JS_Define.h"
#include "fxjs/cjs_event_context.h"
#include "fxjs/cjs_object.h"
#include "fxjs/js_resources.h"

namespace {

CPDFSDK_BAAnnot* ToBAAnnot(CPDFSDK_Annot* annot) {
  return static_cast<CPDFSDK_BAAnnot*>(annot);
}

}  // namespace

const JSPropertySpec CJS_Annot::PropertySpecs[] = {
    {"hidden", get_hidden_static, set_hidden_static},
    {"name", get_name_static, set_name_static},
    {"type", get_type_static, set_type_static}};

int CJS_Annot::ObjDefnID = -1;

const char CJS_Annot::kName[] = "Annot";

// static
int CJS_Annot::GetObjDefnID() {
  return ObjDefnID;
}

// static
void CJS_Annot::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_Annot::kName, FXJSOBJTYPE_DYNAMIC,
                                 JSConstructor<CJS_Annot>, JSDestructor);
  DefineProps(pEngine, ObjDefnID, PropertySpecs, FX_ArraySize(PropertySpecs));
}

CJS_Annot::CJS_Annot(v8::Local<v8::Object> pObject) : CJS_Object(pObject) {}

CJS_Annot::~CJS_Annot() = default;

CJS_Return CJS_Annot::get_hidden(CJS_Runtime* pRuntime) {
  if (!m_pAnnot)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDF_Annot* pPDFAnnot = ToBAAnnot(m_pAnnot.Get())->GetPDFAnnot();
  return CJS_Return(pRuntime->NewBoolean(
      CPDF_Annot::IsAnnotationHidden(pPDFAnnot->GetAnnotDict())));
}

CJS_Return CJS_Annot::set_hidden(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  // May invalidate m_pAnnot.
  bool bHidden = pRuntime->ToBoolean(vp);
  if (!m_pAnnot)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  uint32_t flags = ToBAAnnot(m_pAnnot.Get())->GetFlags();
  if (bHidden) {
    flags |= ANNOTFLAG_HIDDEN;
    flags |= ANNOTFLAG_INVISIBLE;
    flags |= ANNOTFLAG_NOVIEW;
    flags &= ~ANNOTFLAG_PRINT;
  } else {
    flags &= ~ANNOTFLAG_HIDDEN;
    flags &= ~ANNOTFLAG_INVISIBLE;
    flags &= ~ANNOTFLAG_NOVIEW;
    flags |= ANNOTFLAG_PRINT;
  }
  ToBAAnnot(m_pAnnot.Get())->SetFlags(flags);

  return CJS_Return(true);
}

CJS_Return CJS_Annot::get_name(CJS_Runtime* pRuntime) {
  if (!m_pAnnot)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(
      pRuntime->NewString(ToBAAnnot(m_pAnnot.Get())->GetAnnotName().c_str()));
}

CJS_Return CJS_Annot::set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  // May invalidate m_pAnnot.
  WideString annotName = pRuntime->ToWideString(vp);
  if (!m_pAnnot)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  ToBAAnnot(m_pAnnot.Get())->SetAnnotName(annotName);
  return CJS_Return(true);
}

CJS_Return CJS_Annot::get_type(CJS_Runtime* pRuntime) {
  if (!m_pAnnot)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(pRuntime->NewString(
      WideString::FromLocal(CPDF_Annot::AnnotSubtypeToString(
                                ToBAAnnot(m_pAnnot.Get())->GetAnnotSubtype())
                                .c_str())
          .c_str()));
}

CJS_Return CJS_Annot::set_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}
