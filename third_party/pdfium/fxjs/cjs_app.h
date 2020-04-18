// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_APP_H_
#define FXJS_CJS_APP_H_

#include <memory>
#include <set>
#include <vector>

#include "fxjs/JS_Define.h"

class CJS_Runtime;
class GlobalTimer;

class CJS_App : public CJS_Object {
 public:
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  explicit CJS_App(v8::Local<v8::Object> pObject);
  ~CJS_App() override;

  void TimerProc(GlobalTimer* pTimer);
  void CancelProc(GlobalTimer* pTimer);

  static WideString SysPathToPDFPath(const WideString& sOldPath);

  JS_STATIC_PROP(activeDocs, active_docs, CJS_App);
  JS_STATIC_PROP(calculate, calculate, CJS_App);
  JS_STATIC_PROP(formsVersion, forms_version, CJS_App);
  JS_STATIC_PROP(fs, fs, CJS_App);
  JS_STATIC_PROP(fullscreen, fullscreen, CJS_App);
  JS_STATIC_PROP(language, language, CJS_App);
  JS_STATIC_PROP(media, media, CJS_App);
  JS_STATIC_PROP(platform, platform, CJS_App);
  JS_STATIC_PROP(runtimeHighlight, runtime_highlight, CJS_App);
  JS_STATIC_PROP(viewerType, viewer_type, CJS_App);
  JS_STATIC_PROP(viewerVariation, viewer_variation, CJS_App);
  JS_STATIC_PROP(viewerVersion, viewer_version, CJS_App);

  JS_STATIC_METHOD(alert, CJS_App);
  JS_STATIC_METHOD(beep, CJS_App);
  JS_STATIC_METHOD(browseForDoc, CJS_App);
  JS_STATIC_METHOD(clearInterval, CJS_App);
  JS_STATIC_METHOD(clearTimeOut, CJS_App);
  JS_STATIC_METHOD(execDialog, CJS_App);
  JS_STATIC_METHOD(execMenuItem, CJS_App);
  JS_STATIC_METHOD(findComponent, CJS_App);
  JS_STATIC_METHOD(goBack, CJS_App);
  JS_STATIC_METHOD(goForward, CJS_App);
  JS_STATIC_METHOD(launchURL, CJS_App);
  JS_STATIC_METHOD(mailMsg, CJS_App);
  JS_STATIC_METHOD(newFDF, CJS_App);
  JS_STATIC_METHOD(newDoc, CJS_App);
  JS_STATIC_METHOD(openDoc, CJS_App);
  JS_STATIC_METHOD(openFDF, CJS_App);
  JS_STATIC_METHOD(popUpMenuEx, CJS_App);
  JS_STATIC_METHOD(popUpMenu, CJS_App);
  JS_STATIC_METHOD(response, CJS_App);
  JS_STATIC_METHOD(setInterval, CJS_App);
  JS_STATIC_METHOD(setTimeOut, CJS_App);

 private:
  static int ObjDefnID;
  static const char kName[];
  static const JSPropertySpec PropertySpecs[];
  static const JSMethodSpec MethodSpecs[];

  CJS_Return get_active_docs(CJS_Runtime* pRuntime);
  CJS_Return set_active_docs(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_calculate(CJS_Runtime* pRuntime);
  CJS_Return set_calculate(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_forms_version(CJS_Runtime* pRuntime);
  CJS_Return set_forms_version(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_fs(CJS_Runtime* pRuntime);
  CJS_Return set_fs(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_fullscreen(CJS_Runtime* pRuntime);
  CJS_Return set_fullscreen(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_language(CJS_Runtime* pRuntime);
  CJS_Return set_language(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_media(CJS_Runtime* pRuntime);
  CJS_Return set_media(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_platform(CJS_Runtime* pRuntime);
  CJS_Return set_platform(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_runtime_highlight(CJS_Runtime* pRuntime);
  CJS_Return set_runtime_highlight(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp);

  CJS_Return get_viewer_type(CJS_Runtime* pRuntime);
  CJS_Return set_viewer_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_viewer_variation(CJS_Runtime* pRuntime);
  CJS_Return set_viewer_variation(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp);

  CJS_Return get_viewer_version(CJS_Runtime* pRuntime);
  CJS_Return set_viewer_version(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return alert(CJS_Runtime* pRuntime,
                   const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return beep(CJS_Runtime* pRuntime,
                  const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return browseForDoc(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return clearInterval(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return clearTimeOut(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return execDialog(CJS_Runtime* pRuntime,
                        const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return execMenuItem(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return findComponent(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return goBack(CJS_Runtime* pRuntime,
                    const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return goForward(CJS_Runtime* pRuntime,
                       const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return launchURL(CJS_Runtime* pRuntime,
                       const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return mailMsg(CJS_Runtime* pRuntime,
                     const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return newFDF(CJS_Runtime* pRuntime,
                    const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return newDoc(CJS_Runtime* pRuntime,
                    const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return openDoc(CJS_Runtime* pRuntime,
                     const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return openFDF(CJS_Runtime* pRuntime,
                     const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return popUpMenuEx(CJS_Runtime* pRuntime,
                         const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return popUpMenu(CJS_Runtime* pRuntime,
                       const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return response(CJS_Runtime* pRuntime,
                      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return setInterval(CJS_Runtime* pRuntime,
                         const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return setTimeOut(CJS_Runtime* pRuntime,
                        const std::vector<v8::Local<v8::Value>>& params);

  void RunJsScript(CJS_Runtime* pRuntime, const WideString& wsScript);
  void ClearTimerCommon(CJS_Runtime* pRuntime, v8::Local<v8::Value> param);

  bool m_bCalculate;
  bool m_bRuntimeHighLight;
  std::set<std::unique_ptr<GlobalTimer>> m_Timers;
};

#endif  // FXJS_CJS_APP_H_
