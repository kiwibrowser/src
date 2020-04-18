// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_app.h"

#include <utility>

#include "fpdfsdk/cpdfsdk_interform.h"
#include "fxjs/cjs_document.h"
#include "fxjs/cjs_timerobj.h"
#include "fxjs/global_timer.h"
#include "fxjs/ijs_event_context.h"
#include "fxjs/js_resources.h"

#ifdef PDF_ENABLE_XFA
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#endif  // PDF_ENABLE_XFA

namespace {

bool IsTypeKnown(v8::Local<v8::Value> value) {
  return !value.IsEmpty() &&
         (value->IsString() || value->IsNumber() || value->IsBoolean() ||
          value->IsDate() || value->IsObject() || value->IsNull() ||
          value->IsUndefined());
}

}  // namespace

#define JS_STR_VIEWERTYPE L"pdfium"
#define JS_STR_VIEWERVARIATION L"Full"
#define JS_STR_PLATFORM L"WIN"
#define JS_STR_LANGUAGE L"ENU"
#define JS_NUM_VIEWERVERSION 8
#ifdef PDF_ENABLE_XFA
#define JS_NUM_VIEWERVERSION_XFA 11
#endif  // PDF_ENABLE_XFA
#define JS_NUM_FORMSVERSION 7

const JSPropertySpec CJS_App::PropertySpecs[] = {
    {"activeDocs", get_active_docs_static, set_active_docs_static},
    {"calculate", get_calculate_static, set_calculate_static},
    {"formsVersion", get_forms_version_static, set_forms_version_static},
    {"fs", get_fs_static, set_fs_static},
    {"fullscreen", get_fullscreen_static, set_fullscreen_static},
    {"language", get_language_static, set_language_static},
    {"media", get_media_static, set_media_static},
    {"platform", get_platform_static, set_platform_static},
    {"runtimeHighlight", get_runtime_highlight_static,
     set_runtime_highlight_static},
    {"viewerType", get_viewer_type_static, set_viewer_type_static},
    {"viewerVariation", get_viewer_variation_static,
     set_viewer_variation_static},
    {"viewerVersion", get_viewer_version_static, set_viewer_version_static}};

const JSMethodSpec CJS_App::MethodSpecs[] = {
    {"alert", alert_static},
    {"beep", beep_static},
    {"browseForDoc", browseForDoc_static},
    {"clearInterval", clearInterval_static},
    {"clearTimeOut", clearTimeOut_static},
    {"execDialog", execDialog_static},
    {"execMenuItem", execMenuItem_static},
    {"findComponent", findComponent_static},
    {"goBack", goBack_static},
    {"goForward", goForward_static},
    {"launchURL", launchURL_static},
    {"mailMsg", mailMsg_static},
    {"newFDF", newFDF_static},
    {"newDoc", newDoc_static},
    {"openDoc", openDoc_static},
    {"openFDF", openFDF_static},
    {"popUpMenuEx", popUpMenuEx_static},
    {"popUpMenu", popUpMenu_static},
    {"response", response_static},
    {"setInterval", setInterval_static},
    {"setTimeOut", setTimeOut_static}};

int CJS_App::ObjDefnID = -1;

const char CJS_App::kName[] = "app";

// static
void CJS_App::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_App::kName, FXJSOBJTYPE_STATIC,
                                 JSConstructor<CJS_App>, JSDestructor);
  DefineProps(pEngine, ObjDefnID, PropertySpecs, FX_ArraySize(PropertySpecs));
  DefineMethods(pEngine, ObjDefnID, MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJS_App::CJS_App(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject), m_bCalculate(true), m_bRuntimeHighLight(false) {}

CJS_App::~CJS_App() = default;

CJS_Return CJS_App::get_active_docs(CJS_Runtime* pRuntime) {
  CJS_Document* pJSDocument = nullptr;
  v8::Local<v8::Object> pObj = pRuntime->GetThisObj();
  if (CFXJS_Engine::GetObjDefnID(pObj) == CJS_Document::GetObjDefnID())
    pJSDocument = static_cast<CJS_Document*>(pRuntime->GetObjectPrivate(pObj));

  v8::Local<v8::Array> aDocs = pRuntime->NewArray();
  pRuntime->PutArrayElement(
      aDocs, 0,
      pJSDocument ? v8::Local<v8::Value>(pJSDocument->ToV8Object())
                  : v8::Local<v8::Value>());
  if (pRuntime->GetArrayLength(aDocs) > 0)
    return CJS_Return(aDocs);
  return CJS_Return(pRuntime->NewUndefined());
}

CJS_Return CJS_App::set_active_docs(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_calculate(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewBoolean(m_bCalculate));
}

CJS_Return CJS_App::set_calculate(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  m_bCalculate = pRuntime->ToBoolean(vp);
  pRuntime->GetFormFillEnv()->GetInterForm()->EnableCalculate(m_bCalculate);
  return CJS_Return(true);
}

CJS_Return CJS_App::get_forms_version(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewNumber(JS_NUM_FORMSVERSION));
}

CJS_Return CJS_App::set_forms_version(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_viewer_type(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewString(JS_STR_VIEWERTYPE));
}

CJS_Return CJS_App::set_viewer_type(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_viewer_variation(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewString(JS_STR_VIEWERVARIATION));
}

CJS_Return CJS_App::set_viewer_variation(CJS_Runtime* pRuntime,
                                         v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_viewer_version(CJS_Runtime* pRuntime) {
#ifdef PDF_ENABLE_XFA
  CPDFXFA_Context* pXFAContext = pRuntime->GetFormFillEnv()->GetXFAContext();
  if (pXFAContext->ContainsXFAForm())
    return CJS_Return(pRuntime->NewNumber(JS_NUM_VIEWERVERSION_XFA));
#endif  // PDF_ENABLE_XFA
  return CJS_Return(pRuntime->NewNumber(JS_NUM_VIEWERVERSION));
}

CJS_Return CJS_App::set_viewer_version(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_platform(CJS_Runtime* pRuntime) {
#ifdef PDF_ENABLE_XFA
  CPDFSDK_FormFillEnvironment* pFormFillEnv = pRuntime->GetFormFillEnv();
  if (!pFormFillEnv)
    return CJS_Return(false);

  WideString platfrom = pFormFillEnv->GetPlatform();
  if (!platfrom.IsEmpty())
    return CJS_Return(pRuntime->NewString(platfrom.c_str()));
#endif
  return CJS_Return(pRuntime->NewString(JS_STR_PLATFORM));
}

CJS_Return CJS_App::set_platform(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_language(CJS_Runtime* pRuntime) {
#ifdef PDF_ENABLE_XFA
  CPDFSDK_FormFillEnvironment* pFormFillEnv = pRuntime->GetFormFillEnv();
  if (!pFormFillEnv)
    return CJS_Return(false);

  WideString language = pFormFillEnv->GetLanguage();
  if (!language.IsEmpty())
    return CJS_Return(pRuntime->NewString(language.c_str()));
#endif
  return CJS_Return(pRuntime->NewString(JS_STR_LANGUAGE));
}

CJS_Return CJS_App::set_language(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

// creates a new fdf object that contains no data
// comment: need reader support
// note:
// CFDF_Document * CPDFSDK_FormFillEnvironment::NewFDF();
CJS_Return CJS_App::newFDF(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

// opens a specified pdf document and returns its document object
// comment:need reader support
// note: as defined in js reference, the proto of this function's fourth
// parmeters, how old an fdf document while do not show it.
// CFDF_Document * CPDFSDK_FormFillEnvironment::OpenFDF(string strPath,bool
// bUserConv);

CJS_Return CJS_App::openFDF(CJS_Runtime* pRuntime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_App::alert(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params) {
  std::vector<v8::Local<v8::Value>> newParams = ExpandKeywordParams(
      pRuntime, params, 4, L"cMsg", L"nIcon", L"nType", L"cTitle");

  if (!IsTypeKnown(newParams[0]))
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CPDFSDK_FormFillEnvironment* pFormFillEnv = pRuntime->GetFormFillEnv();
  if (!pFormFillEnv)
    return CJS_Return(pRuntime->NewNumber(0));

  WideString swMsg;
  if (newParams[0]->IsArray()) {
    v8::Local<v8::Array> carray = pRuntime->ToArray(newParams[0]);
    swMsg = L"[";
    for (size_t i = 0; i < pRuntime->GetArrayLength(carray); ++i) {
      if (i)
        swMsg += L", ";

      swMsg += pRuntime->ToWideString(pRuntime->GetArrayElement(carray, i));
    }
    swMsg += L"]";
  } else {
    swMsg = pRuntime->ToWideString(newParams[0]);
  }

  int iIcon = 0;
  if (IsTypeKnown(newParams[1]))
    iIcon = pRuntime->ToInt32(newParams[1]);

  int iType = 0;
  if (IsTypeKnown(newParams[2]))
    iType = pRuntime->ToInt32(newParams[2]);

  WideString swTitle;
  if (IsTypeKnown(newParams[3]))
    swTitle = pRuntime->ToWideString(newParams[3]);
  else
    swTitle = JSGetStringFromID(JSMessage::kAlert);

  pRuntime->BeginBlock();
  pFormFillEnv->KillFocusAnnot(0);
  v8::Local<v8::Value> ret = pRuntime->NewNumber(
      pFormFillEnv->JS_appAlert(swMsg, swTitle, iType, iIcon));
  pRuntime->EndBlock();

  return CJS_Return(ret);
}

CJS_Return CJS_App::beep(CJS_Runtime* pRuntime,
                         const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  pRuntime->GetFormFillEnv()->JS_appBeep(pRuntime->ToInt32(params[0]));
  return CJS_Return(true);
}

CJS_Return CJS_App::findComponent(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_App::popUpMenuEx(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_App::get_fs(CJS_Runtime* pRuntime) {
  return CJS_Return(false);
}

CJS_Return CJS_App::set_fs(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::setInterval(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() == 0 || params.size() > 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString script = pRuntime->ToWideString(params[0]);
  if (script.IsEmpty())
    return CJS_Return(JSGetStringFromID(JSMessage::kInvalidInputError));

  uint32_t dwInterval = params.size() > 1 ? pRuntime->ToInt32(params[1]) : 1000;
  auto timerRef = pdfium::MakeUnique<GlobalTimer>(
      this, pRuntime->GetFormFillEnv(), pRuntime, 0, script, dwInterval, 0);
  GlobalTimer* pTimerRef = timerRef.get();
  m_Timers.insert(std::move(timerRef));

  v8::Local<v8::Object> pRetObj =
      pRuntime->NewFXJSBoundObject(CJS_TimerObj::GetObjDefnID());
  if (pRetObj.IsEmpty())
    return CJS_Return(false);

  CJS_TimerObj* pJS_TimerObj =
      static_cast<CJS_TimerObj*>(pRuntime->GetObjectPrivate(pRetObj));
  pJS_TimerObj->SetTimer(pTimerRef);
  return CJS_Return(pRetObj);
}

CJS_Return CJS_App::setTimeOut(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() == 0 || params.size() > 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString script = pRuntime->ToWideString(params[0]);
  if (script.IsEmpty())
    return CJS_Return(JSGetStringFromID(JSMessage::kInvalidInputError));

  uint32_t dwTimeOut = params.size() > 1 ? pRuntime->ToInt32(params[1]) : 1000;
  auto timerRef = pdfium::MakeUnique<GlobalTimer>(
      this, pRuntime->GetFormFillEnv(), pRuntime, 1, script, dwTimeOut,
      dwTimeOut);
  GlobalTimer* pTimerRef = timerRef.get();
  m_Timers.insert(std::move(timerRef));

  v8::Local<v8::Object> pRetObj =
      pRuntime->NewFXJSBoundObject(CJS_TimerObj::GetObjDefnID());
  if (pRetObj.IsEmpty())
    return CJS_Return(false);

  CJS_TimerObj* pJS_TimerObj =
      static_cast<CJS_TimerObj*>(pRuntime->GetObjectPrivate(pRetObj));
  pJS_TimerObj->SetTimer(pTimerRef);
  return CJS_Return(pRetObj);
}

CJS_Return CJS_App::clearTimeOut(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_App::ClearTimerCommon(pRuntime, params[0]);
  return CJS_Return(true);
}

CJS_Return CJS_App::clearInterval(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CJS_App::ClearTimerCommon(pRuntime, params[0]);
  return CJS_Return(true);
}

void CJS_App::ClearTimerCommon(CJS_Runtime* pRuntime,
                               v8::Local<v8::Value> param) {
  if (!param->IsObject())
    return;

  v8::Local<v8::Object> pObj = pRuntime->ToObject(param);
  if (CFXJS_Engine::GetObjDefnID(pObj) != CJS_TimerObj::GetObjDefnID())
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(pObj);
  if (!pJSObj)
    return;

  CJS_TimerObj* pJS_TimerObj = static_cast<CJS_TimerObj*>(pJSObj);
  GlobalTimer::Cancel(pJS_TimerObj->GetTimerID());
}

CJS_Return CJS_App::execMenuItem(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

void CJS_App::TimerProc(GlobalTimer* pTimer) {
  CJS_Runtime* pRuntime = pTimer->GetRuntime();
  if (pRuntime && (!pTimer->IsOneShot() || pTimer->GetTimeOut() > 0))
    RunJsScript(pRuntime, pTimer->GetJScript());
}

void CJS_App::CancelProc(GlobalTimer* pTimer) {
  m_Timers.erase(pdfium::FakeUniquePtr<GlobalTimer>(pTimer));
}

void CJS_App::RunJsScript(CJS_Runtime* pRuntime, const WideString& wsScript) {
  if (!pRuntime->IsBlocking()) {
    IJS_EventContext* pContext = pRuntime->NewEventContext();
    pContext->OnExternal_Exec();
    pContext->RunScript(wsScript);
    pRuntime->ReleaseEventContext(pContext);
  }
}

CJS_Return CJS_App::goBack(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params) {
  // Not supported.
  return CJS_Return(true);
}

CJS_Return CJS_App::goForward(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  // Not supported.
  return CJS_Return(true);
}

CJS_Return CJS_App::mailMsg(CJS_Runtime* pRuntime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  std::vector<v8::Local<v8::Value>> newParams =
      ExpandKeywordParams(pRuntime, params, 6, L"bUI", L"cTo", L"cCc", L"cBcc",
                          L"cSubject", L"cMsg");

  if (!IsTypeKnown(newParams[0]))
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  bool bUI = pRuntime->ToBoolean(newParams[0]);
  WideString cTo;
  if (IsTypeKnown(newParams[1])) {
    cTo = pRuntime->ToWideString(newParams[1]);
  } else {
    // cTo parameter required when UI not invoked.
    if (!bUI)
      return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  }

  WideString cCc;
  if (IsTypeKnown(newParams[2]))
    cCc = pRuntime->ToWideString(newParams[2]);

  WideString cBcc;
  if (IsTypeKnown(newParams[3]))
    cBcc = pRuntime->ToWideString(newParams[3]);

  WideString cSubject;
  if (IsTypeKnown(newParams[4]))
    cSubject = pRuntime->ToWideString(newParams[4]);

  WideString cMsg;
  if (IsTypeKnown(newParams[5]))
    cMsg = pRuntime->ToWideString(newParams[5]);

  pRuntime->BeginBlock();
  pRuntime->GetFormFillEnv()->JS_docmailForm(nullptr, 0, bUI, cTo, cSubject,
                                             cCc, cBcc, cMsg);
  pRuntime->EndBlock();
  return CJS_Return(true);
}

CJS_Return CJS_App::launchURL(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_App::get_runtime_highlight(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewBoolean(m_bRuntimeHighLight));
}

CJS_Return CJS_App::set_runtime_highlight(CJS_Runtime* pRuntime,
                                          v8::Local<v8::Value> vp) {
  m_bRuntimeHighLight = pRuntime->ToBoolean(vp);
  return CJS_Return(true);
}

CJS_Return CJS_App::get_fullscreen(CJS_Runtime* pRuntime) {
  return CJS_Return(false);
}

CJS_Return CJS_App::set_fullscreen(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::popUpMenu(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_App::browseForDoc(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

WideString CJS_App::SysPathToPDFPath(const WideString& sOldPath) {
  WideString sRet = L"/";
  for (const wchar_t& c : sOldPath) {
    if (c != L':')
      sRet += (c == L'\\') ? L'/' : c;
  }
  return sRet;
}

CJS_Return CJS_App::newDoc(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_App::openDoc(CJS_Runtime* pRuntime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_App::response(CJS_Runtime* pRuntime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  std::vector<v8::Local<v8::Value>> newParams =
      ExpandKeywordParams(pRuntime, params, 5, L"cQuestion", L"cTitle",
                          L"cDefault", L"bPassword", L"cLabel");

  if (!IsTypeKnown(newParams[0]))
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString swQuestion = pRuntime->ToWideString(newParams[0]);
  WideString swTitle = L"PDF";
  if (IsTypeKnown(newParams[1]))
    swTitle = pRuntime->ToWideString(newParams[1]);

  WideString swDefault;
  if (IsTypeKnown(newParams[2]))
    swDefault = pRuntime->ToWideString(newParams[2]);

  bool bPassword = false;
  if (IsTypeKnown(newParams[3]))
    bPassword = pRuntime->ToBoolean(newParams[3]);

  WideString swLabel;
  if (IsTypeKnown(newParams[4]))
    swLabel = pRuntime->ToWideString(newParams[4]);

  const int MAX_INPUT_BYTES = 2048;
  std::vector<uint8_t> pBuff(MAX_INPUT_BYTES + 2);
  int nLengthBytes = pRuntime->GetFormFillEnv()->JS_appResponse(
      swQuestion, swTitle, swDefault, swLabel, bPassword, pBuff.data(),
      MAX_INPUT_BYTES);

  if (nLengthBytes < 0 || nLengthBytes > MAX_INPUT_BYTES)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamTooLongError));

  return CJS_Return(pRuntime->NewString(
      WideString::FromUTF16LE(reinterpret_cast<uint16_t*>(pBuff.data()),
                              nLengthBytes / sizeof(uint16_t))
          .c_str()));
}

CJS_Return CJS_App::get_media(CJS_Runtime* pRuntime) {
  return CJS_Return(false);
}

CJS_Return CJS_App::set_media(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_App::execDialog(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}
