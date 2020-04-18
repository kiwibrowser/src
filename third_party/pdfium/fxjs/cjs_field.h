// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_FIELD_H_
#define FXJS_CJS_FIELD_H_

#include <string>
#include <vector>

#include "fxjs/JS_Define.h"

class CPDF_FormControl;
class CPDFSDK_Widget;
class CJS_Document;
struct CJS_DelayData;

enum FIELD_PROP {
  FP_BORDERSTYLE,
  FP_CURRENTVALUEINDICES,
  FP_DISPLAY,
  FP_HIDDEN,
  FP_LINEWIDTH,
  FP_RECT,
  FP_VALUE
};

class CJS_Field : public CJS_Object {
 public:
  static int GetObjDefnID();
  static void DefineJSObjects(CFXJS_Engine* pEngine);
  static void DoDelay(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                      CJS_DelayData* pData);

  explicit CJS_Field(v8::Local<v8::Object> pObject);
  ~CJS_Field() override;

  // CJS_Object
  void InitInstance(IJS_Runtime* pIRuntime) override;

  bool AttachField(CJS_Document* pDocument, const WideString& csFieldName);

  JS_STATIC_PROP(alignment, alignment, CJS_Field);
  JS_STATIC_PROP(borderStyle, border_style, CJS_Field);
  JS_STATIC_PROP(buttonAlignX, button_align_x, CJS_Field);
  JS_STATIC_PROP(buttonAlignY, button_align_y, CJS_Field);
  JS_STATIC_PROP(buttonFitBounds, button_fit_bounds, CJS_Field);
  JS_STATIC_PROP(buttonPosition, button_position, CJS_Field);
  JS_STATIC_PROP(buttonScaleHow, button_scale_how, CJS_Field);
  JS_STATIC_PROP(ButtonScaleWhen, button_scale_when, CJS_Field);
  JS_STATIC_PROP(calcOrderIndex, calc_order_index, CJS_Field);
  JS_STATIC_PROP(charLimit, char_limit, CJS_Field);
  JS_STATIC_PROP(comb, comb, CJS_Field);
  JS_STATIC_PROP(commitOnSelChange, commit_on_sel_change, CJS_Field);
  JS_STATIC_PROP(currentValueIndices, current_value_indices, CJS_Field);
  JS_STATIC_PROP(defaultStyle, default_style, CJS_Field);
  JS_STATIC_PROP(defaultValue, default_value, CJS_Field);
  JS_STATIC_PROP(doNotScroll, do_not_scroll, CJS_Field);
  JS_STATIC_PROP(doNotSpellCheck, do_not_spell_check, CJS_Field);
  JS_STATIC_PROP(delay, delay, CJS_Field);
  JS_STATIC_PROP(display, display, CJS_Field);
  JS_STATIC_PROP(doc, doc, CJS_Field);
  JS_STATIC_PROP(editable, editable, CJS_Field);
  JS_STATIC_PROP(exportValues, export_values, CJS_Field);
  JS_STATIC_PROP(fileSelect, file_select, CJS_Field);
  JS_STATIC_PROP(fillColor, fill_color, CJS_Field);
  JS_STATIC_PROP(hidden, hidden, CJS_Field);
  JS_STATIC_PROP(highlight, highlight, CJS_Field);
  JS_STATIC_PROP(lineWidth, line_width, CJS_Field);
  JS_STATIC_PROP(multiline, multiline, CJS_Field);
  JS_STATIC_PROP(multipleSelection, multiple_selection, CJS_Field);
  JS_STATIC_PROP(name, name, CJS_Field);
  JS_STATIC_PROP(numItems, num_items, CJS_Field);
  JS_STATIC_PROP(page, page, CJS_Field);
  JS_STATIC_PROP(password, password, CJS_Field);
  JS_STATIC_PROP(print, print, CJS_Field);
  JS_STATIC_PROP(radiosInUnison, radios_in_unison, CJS_Field);
  JS_STATIC_PROP(readonly, readonly, CJS_Field);
  JS_STATIC_PROP(rect, rect, CJS_Field);
  JS_STATIC_PROP(required, required, CJS_Field);
  JS_STATIC_PROP(richText, rich_text, CJS_Field);
  JS_STATIC_PROP(richValue, rich_value, CJS_Field);
  JS_STATIC_PROP(rotation, rotation, CJS_Field);
  JS_STATIC_PROP(strokeColor, stroke_color, CJS_Field);
  JS_STATIC_PROP(style, style, CJS_Field);
  JS_STATIC_PROP(submitName, submit_name, CJS_Field);
  JS_STATIC_PROP(textColor, text_color, CJS_Field);
  JS_STATIC_PROP(textFont, text_font, CJS_Field);
  JS_STATIC_PROP(textSize, text_size, CJS_Field);
  JS_STATIC_PROP(type, type, CJS_Field);
  JS_STATIC_PROP(userName, user_name, CJS_Field);
  JS_STATIC_PROP(value, value, CJS_Field);
  JS_STATIC_PROP(valueAsString, value_as_string, CJS_Field);
  JS_STATIC_PROP(source, source, CJS_Field);

  JS_STATIC_METHOD(browseForFileToSubmit, CJS_Field);
  JS_STATIC_METHOD(buttonGetCaption, CJS_Field);
  JS_STATIC_METHOD(buttonGetIcon, CJS_Field);
  JS_STATIC_METHOD(buttonImportIcon, CJS_Field);
  JS_STATIC_METHOD(buttonSetCaption, CJS_Field);
  JS_STATIC_METHOD(buttonSetIcon, CJS_Field);
  JS_STATIC_METHOD(checkThisBox, CJS_Field);
  JS_STATIC_METHOD(clearItems, CJS_Field);
  JS_STATIC_METHOD(defaultIsChecked, CJS_Field);
  JS_STATIC_METHOD(deleteItemAt, CJS_Field);
  JS_STATIC_METHOD(getArray, CJS_Field);
  JS_STATIC_METHOD(getItemAt, CJS_Field);
  JS_STATIC_METHOD(getLock, CJS_Field);
  JS_STATIC_METHOD(insertItemAt, CJS_Field);
  JS_STATIC_METHOD(isBoxChecked, CJS_Field);
  JS_STATIC_METHOD(isDefaultChecked, CJS_Field);
  JS_STATIC_METHOD(setAction, CJS_Field);
  JS_STATIC_METHOD(setFocus, CJS_Field);
  JS_STATIC_METHOD(setItems, CJS_Field);
  JS_STATIC_METHOD(setLock, CJS_Field);
  JS_STATIC_METHOD(signatureGetModifications, CJS_Field);
  JS_STATIC_METHOD(signatureGetSeedValue, CJS_Field);
  JS_STATIC_METHOD(signatureInfo, CJS_Field);
  JS_STATIC_METHOD(signatureSetSeedValue, CJS_Field);
  JS_STATIC_METHOD(signatureSign, CJS_Field);
  JS_STATIC_METHOD(signatureValidate, CJS_Field);

  CJS_Return get_text_color(CJS_Runtime* pRuntime);
  CJS_Return set_text_color(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

 private:
  static int ObjDefnID;
  static const char kName[];
  static const JSPropertySpec PropertySpecs[];
  static const JSMethodSpec MethodSpecs[];

  static void SetBorderStyle(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                             const WideString& swFieldName,
                             int nControlIndex,
                             const ByteString& string);
  static void SetCurrentValueIndices(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                                     const WideString& swFieldName,
                                     int nControlIndex,
                                     const std::vector<uint32_t>& array);
  static void SetDisplay(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                         const WideString& swFieldName,
                         int nControlIndex,
                         int number);
  static void SetHidden(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                        const WideString& swFieldName,
                        int nControlIndex,
                        bool b);
  static void SetLineWidth(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                           const WideString& swFieldName,
                           int nControlIndex,
                           int number);
  static void SetMultiline(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                           const WideString& swFieldName,
                           int nControlIndex,
                           bool b);
  static void SetRect(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                      const WideString& swFieldName,
                      int nControlIndex,
                      const CFX_FloatRect& rect);
  static void SetValue(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                       const WideString& swFieldName,
                       int nControlIndex,
                       const std::vector<WideString>& strArray);

  static void UpdateFormField(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                              CPDF_FormField* pFormField,
                              bool bChangeMark,
                              bool bResetAP,
                              bool bRefresh);
  static void UpdateFormControl(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                                CPDF_FormControl* pFormControl,
                                bool bChangeMark,
                                bool bResetAP,
                                bool bRefresh);

  static CPDFSDK_Widget* GetWidget(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                                   CPDF_FormControl* pFormControl);
  static std::vector<CPDF_FormField*> GetFormFields(
      CPDFSDK_FormFillEnvironment* pFormFillEnv,
      const WideString& csFieldName);

  CJS_Return get_alignment(CJS_Runtime* pRuntime);
  CJS_Return set_alignment(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_border_style(CJS_Runtime* pRuntime);
  CJS_Return set_border_style(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_button_align_x(CJS_Runtime* pRuntime);
  CJS_Return set_button_align_x(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_button_align_y(CJS_Runtime* pRuntime);
  CJS_Return set_button_align_y(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_button_fit_bounds(CJS_Runtime* pRuntime);
  CJS_Return set_button_fit_bounds(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp);

  CJS_Return get_button_position(CJS_Runtime* pRuntime);
  CJS_Return set_button_position(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp);

  CJS_Return get_button_scale_how(CJS_Runtime* pRuntime);
  CJS_Return set_button_scale_how(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp);

  CJS_Return get_button_scale_when(CJS_Runtime* pRuntime);
  CJS_Return set_button_scale_when(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp);

  CJS_Return get_calc_order_index(CJS_Runtime* pRuntime);
  CJS_Return set_calc_order_index(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp);

  CJS_Return get_char_limit(CJS_Runtime* pRuntime);
  CJS_Return set_char_limit(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_comb(CJS_Runtime* pRuntime);
  CJS_Return set_comb(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_commit_on_sel_change(CJS_Runtime* pRuntime);
  CJS_Return set_commit_on_sel_change(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp);

  CJS_Return get_current_value_indices(CJS_Runtime* pRuntime);
  CJS_Return set_current_value_indices(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp);

  CJS_Return get_default_style(CJS_Runtime* pRuntime);
  CJS_Return set_default_style(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_default_value(CJS_Runtime* pRuntime);
  CJS_Return set_default_value(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_do_not_scroll(CJS_Runtime* pRuntime);
  CJS_Return set_do_not_scroll(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_do_not_spell_check(CJS_Runtime* pRuntime);
  CJS_Return set_do_not_spell_check(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp);

  CJS_Return get_delay(CJS_Runtime* pRuntime);
  CJS_Return set_delay(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_display(CJS_Runtime* pRuntime);
  CJS_Return set_display(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_doc(CJS_Runtime* pRuntime);
  CJS_Return set_doc(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_editable(CJS_Runtime* pRuntime);
  CJS_Return set_editable(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_export_values(CJS_Runtime* pRuntime);
  CJS_Return set_export_values(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_file_select(CJS_Runtime* pRuntime);
  CJS_Return set_file_select(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_fill_color(CJS_Runtime* pRuntime);
  CJS_Return set_fill_color(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_hidden(CJS_Runtime* pRuntime);
  CJS_Return set_hidden(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_highlight(CJS_Runtime* pRuntime);
  CJS_Return set_highlight(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_line_width(CJS_Runtime* pRuntime);
  CJS_Return set_line_width(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_multiline(CJS_Runtime* pRuntime);
  CJS_Return set_multiline(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_multiple_selection(CJS_Runtime* pRuntime);
  CJS_Return set_multiple_selection(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp);

  CJS_Return get_name(CJS_Runtime* pRuntime);
  CJS_Return set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_num_items(CJS_Runtime* pRuntime);
  CJS_Return set_num_items(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_page(CJS_Runtime* pRuntime);
  CJS_Return set_page(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_password(CJS_Runtime* pRuntime);
  CJS_Return set_password(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_print(CJS_Runtime* pRuntime);
  CJS_Return set_print(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_radios_in_unison(CJS_Runtime* pRuntime);
  CJS_Return set_radios_in_unison(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp);

  CJS_Return get_readonly(CJS_Runtime* pRuntime);
  CJS_Return set_readonly(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rect(CJS_Runtime* pRuntime);
  CJS_Return set_rect(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_required(CJS_Runtime* pRuntime);
  CJS_Return set_required(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rich_text(CJS_Runtime* pRuntime);
  CJS_Return set_rich_text(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rich_value(CJS_Runtime* pRuntime);
  CJS_Return set_rich_value(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_rotation(CJS_Runtime* pRuntime);
  CJS_Return set_rotation(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_stroke_color(CJS_Runtime* pRuntime);
  CJS_Return set_stroke_color(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_style(CJS_Runtime* pRuntime);
  CJS_Return set_style(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_submit_name(CJS_Runtime* pRuntime);
  CJS_Return set_submit_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_text_font(CJS_Runtime* pRuntime);
  CJS_Return set_text_font(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_text_size(CJS_Runtime* pRuntime);
  CJS_Return set_text_size(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_type(CJS_Runtime* pRuntime);
  CJS_Return set_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_user_name(CJS_Runtime* pRuntime);
  CJS_Return set_user_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_value(CJS_Runtime* pRuntime);
  CJS_Return set_value(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_value_as_string(CJS_Runtime* pRuntime);
  CJS_Return set_value_as_string(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp);

  CJS_Return get_source(CJS_Runtime* pRuntime);
  CJS_Return set_source(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return browseForFileToSubmit(
      CJS_Runtime* pRuntime,
      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return buttonGetCaption(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return buttonGetIcon(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return buttonImportIcon(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return buttonSetCaption(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return buttonSetIcon(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return checkThisBox(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return clearItems(CJS_Runtime* pRuntime,
                        const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return defaultIsChecked(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return deleteItemAt(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return getArray(CJS_Runtime* pRuntime,
                      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return getItemAt(CJS_Runtime* pRuntime,
                       const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return getLock(CJS_Runtime* pRuntime,
                     const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return insertItemAt(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return isBoxChecked(CJS_Runtime* pRuntime,
                          const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return isDefaultChecked(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return setAction(CJS_Runtime* pRuntime,
                       const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return setFocus(CJS_Runtime* pRuntime,
                      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return setItems(CJS_Runtime* pRuntime,
                      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return setLock(CJS_Runtime* pRuntime,
                     const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return signatureGetModifications(
      CJS_Runtime* pRuntime,
      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return signatureGetSeedValue(
      CJS_Runtime* pRuntime,
      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return signatureInfo(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return signatureSetSeedValue(
      CJS_Runtime* pRuntime,
      const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return signatureSign(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return signatureValidate(CJS_Runtime* pRuntime,
                               const std::vector<v8::Local<v8::Value>>& params);

  void SetDelay(bool bDelay);
  void ParseFieldName(const std::wstring& strFieldNameParsed,
                      std::wstring& strFieldName,
                      int& iControlNo);
  std::vector<CPDF_FormField*> GetFormFields(
      const WideString& csFieldName) const;
  CPDF_FormControl* GetSmartFieldControl(CPDF_FormField* pFormField);
  bool ValueIsOccur(CPDF_FormField* pFormField, WideString csOptLabel);

  void AddDelay_Int(FIELD_PROP prop, int32_t n);
  void AddDelay_Bool(FIELD_PROP prop, bool b);
  void AddDelay_String(FIELD_PROP prop, const ByteString& string);
  void AddDelay_Rect(FIELD_PROP prop, const CFX_FloatRect& rect);
  void AddDelay_WordArray(FIELD_PROP prop, const std::vector<uint32_t>& array);
  void AddDelay_WideStringArray(FIELD_PROP prop,
                                const std::vector<WideString>& array);

  void DoDelay();

  CJS_Document* m_pJSDoc;
  CPDFSDK_FormFillEnvironment::ObservedPtr m_pFormFillEnv;
  WideString m_FieldName;
  int m_nFormControlIndex;
  bool m_bCanSet;
  bool m_bDelay;
};

#endif  // FXJS_CJS_FIELD_H_
