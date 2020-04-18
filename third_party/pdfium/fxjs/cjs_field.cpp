// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_field.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfdoc/cpdf_formfield.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "fpdfsdk/cpdfsdk_interform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/cpdfsdk_widget.h"
#include "fxjs/cjs_color.h"
#include "fxjs/cjs_delaydata.h"
#include "fxjs/cjs_document.h"
#include "fxjs/cjs_icon.h"
#include "fxjs/js_resources.h"

namespace {

bool SetWidgetDisplayStatus(CPDFSDK_Widget* pWidget, int value) {
  if (!pWidget)
    return false;

  uint32_t dwFlag = pWidget->GetFlags();
  switch (value) {
    case 0:
      dwFlag &= ~ANNOTFLAG_INVISIBLE;
      dwFlag &= ~ANNOTFLAG_HIDDEN;
      dwFlag &= ~ANNOTFLAG_NOVIEW;
      dwFlag |= ANNOTFLAG_PRINT;
      break;
    case 1:
      dwFlag &= ~ANNOTFLAG_INVISIBLE;
      dwFlag &= ~ANNOTFLAG_NOVIEW;
      dwFlag |= (ANNOTFLAG_HIDDEN | ANNOTFLAG_PRINT);
      break;
    case 2:
      dwFlag &= ~ANNOTFLAG_INVISIBLE;
      dwFlag &= ~ANNOTFLAG_PRINT;
      dwFlag &= ~ANNOTFLAG_HIDDEN;
      dwFlag &= ~ANNOTFLAG_NOVIEW;
      break;
    case 3:
      dwFlag |= ANNOTFLAG_NOVIEW;
      dwFlag |= ANNOTFLAG_PRINT;
      dwFlag &= ~ANNOTFLAG_HIDDEN;
      break;
  }

  if (dwFlag != pWidget->GetFlags()) {
    pWidget->SetFlags(dwFlag);
    return true;
  }

  return false;
}

}  // namespace

const JSPropertySpec CJS_Field::PropertySpecs[] = {
    {"alignment", get_alignment_static, set_alignment_static},
    {"borderStyle", get_border_style_static, set_border_style_static},
    {"buttonAlignX", get_button_align_x_static, set_button_align_x_static},
    {"buttonAlignY", get_button_align_y_static, set_button_align_y_static},
    {"buttonFitBounds", get_button_fit_bounds_static,
     set_button_fit_bounds_static},
    {"buttonPosition", get_button_position_static, set_button_position_static},
    {"buttonScaleHow", get_button_scale_how_static,
     set_button_scale_how_static},
    {"buttonScaleWhen", get_button_scale_when_static,
     set_button_scale_when_static},
    {"calcOrderIndex", get_calc_order_index_static,
     set_calc_order_index_static},
    {"charLimit", get_char_limit_static, set_char_limit_static},
    {"comb", get_comb_static, set_comb_static},
    {"commitOnSelChange", get_commit_on_sel_change_static,
     set_commit_on_sel_change_static},
    {"currentValueIndices", get_current_value_indices_static,
     set_current_value_indices_static},
    {"defaultStyle", get_default_style_static, set_default_style_static},
    {"defaultValue", get_default_value_static, set_default_value_static},
    {"doNotScroll", get_do_not_scroll_static, set_do_not_scroll_static},
    {"doNotSpellCheck", get_do_not_spell_check_static,
     set_do_not_spell_check_static},
    {"delay", get_delay_static, set_delay_static},
    {"display", get_display_static, set_display_static},
    {"doc", get_doc_static, set_doc_static},
    {"editable", get_editable_static, set_editable_static},
    {"exportValues", get_export_values_static, set_export_values_static},
    {"hidden", get_hidden_static, set_hidden_static},
    {"fileSelect", get_file_select_static, set_file_select_static},
    {"fillColor", get_fill_color_static, set_fill_color_static},
    {"lineWidth", get_line_width_static, set_line_width_static},
    {"highlight", get_highlight_static, set_highlight_static},
    {"multiline", get_multiline_static, set_multiline_static},
    {"multipleSelection", get_multiple_selection_static,
     set_multiple_selection_static},
    {"name", get_name_static, set_name_static},
    {"numItems", get_num_items_static, set_num_items_static},
    {"page", get_page_static, set_page_static},
    {"password", get_password_static, set_password_static},
    {"print", get_print_static, set_print_static},
    {"radiosInUnison", get_radios_in_unison_static,
     set_radios_in_unison_static},
    {"readonly", get_readonly_static, set_readonly_static},
    {"rect", get_rect_static, set_rect_static},
    {"required", get_required_static, set_required_static},
    {"richText", get_rich_text_static, set_rich_text_static},
    {"richValue", get_rich_value_static, set_rich_value_static},
    {"rotation", get_rotation_static, set_rotation_static},
    {"strokeColor", get_stroke_color_static, set_stroke_color_static},
    {"style", get_style_static, set_style_static},
    {"submitName", get_submit_name_static, set_submit_name_static},
    {"textColor", get_text_color_static, set_text_color_static},
    {"textFont", get_text_font_static, set_text_font_static},
    {"textSize", get_text_size_static, set_text_size_static},
    {"type", get_type_static, set_type_static},
    {"userName", get_user_name_static, set_user_name_static},
    {"value", get_value_static, set_value_static},
    {"valueAsString", get_value_as_string_static, set_value_as_string_static},
    {"source", get_source_static, set_source_static}};

const JSMethodSpec CJS_Field::MethodSpecs[] = {
    {"browseForFileToSubmit", browseForFileToSubmit_static},
    {"buttonGetCaption", buttonGetCaption_static},
    {"buttonGetIcon", buttonGetIcon_static},
    {"buttonImportIcon", buttonImportIcon_static},
    {"buttonSetCaption", buttonSetCaption_static},
    {"buttonSetIcon", buttonSetIcon_static},
    {"checkThisBox", checkThisBox_static},
    {"clearItems", clearItems_static},
    {"defaultIsChecked", defaultIsChecked_static},
    {"deleteItemAt", deleteItemAt_static},
    {"getArray", getArray_static},
    {"getItemAt", getItemAt_static},
    {"getLock", getLock_static},
    {"insertItemAt", insertItemAt_static},
    {"isBoxChecked", isBoxChecked_static},
    {"isDefaultChecked", isDefaultChecked_static},
    {"setAction", setAction_static},
    {"setFocus", setFocus_static},
    {"setItems", setItems_static},
    {"setLock", setLock_static},
    {"signatureGetModifications", signatureGetModifications_static},
    {"signatureGetSeedValue", signatureGetSeedValue_static},
    {"signatureInfo", signatureInfo_static},
    {"signatureSetSeedValue", signatureSetSeedValue_static},
    {"signatureSign", signatureSign_static},
    {"signatureValidate", signatureValidate_static}};

int CJS_Field::ObjDefnID = -1;
const char CJS_Field::kName[] = "Field";

// static
int CJS_Field::GetObjDefnID() {
  return ObjDefnID;
}

// static
void CJS_Field::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_Field::kName, FXJSOBJTYPE_DYNAMIC,
                                 JSConstructor<CJS_Field>, JSDestructor);
  DefineProps(pEngine, ObjDefnID, PropertySpecs, FX_ArraySize(PropertySpecs));
  DefineMethods(pEngine, ObjDefnID, MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJS_Field::CJS_Field(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject),
      m_pJSDoc(nullptr),
      m_pFormFillEnv(nullptr),
      m_nFormControlIndex(-1),
      m_bCanSet(false),
      m_bDelay(false) {}

CJS_Field::~CJS_Field() = default;

void CJS_Field::InitInstance(IJS_Runtime* pIRuntime) {}

// note: iControlNo = -1, means not a widget.
void CJS_Field::ParseFieldName(const std::wstring& strFieldNameParsed,
                               std::wstring& strFieldName,
                               int& iControlNo) {
  int iStart = strFieldNameParsed.find_last_of(L'.');
  if (iStart == -1) {
    strFieldName = strFieldNameParsed;
    iControlNo = -1;
    return;
  }
  std::wstring suffixal = strFieldNameParsed.substr(iStart + 1);
  iControlNo = FXSYS_wtoi(suffixal.c_str());
  if (iControlNo == 0) {
    int iSpaceStart;
    while ((iSpaceStart = suffixal.find_last_of(L" ")) != -1) {
      suffixal.erase(iSpaceStart, 1);
    }

    if (suffixal.compare(L"0") != 0) {
      strFieldName = strFieldNameParsed;
      iControlNo = -1;
      return;
    }
  }
  strFieldName = strFieldNameParsed.substr(0, iStart);
}

bool CJS_Field::AttachField(CJS_Document* pDocument,
                            const WideString& csFieldName) {
  m_pJSDoc = pDocument;
  m_pFormFillEnv.Reset(pDocument->GetFormFillEnv());
  m_bCanSet = m_pFormFillEnv->GetPermissions(FPDFPERM_FILL_FORM) ||
              m_pFormFillEnv->GetPermissions(FPDFPERM_ANNOT_FORM) ||
              m_pFormFillEnv->GetPermissions(FPDFPERM_MODIFY);

  CPDFSDK_InterForm* pRDInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pInterForm = pRDInterForm->GetInterForm();
  WideString swFieldNameTemp = csFieldName;
  swFieldNameTemp.Replace(L"..", L".");

  if (pInterForm->CountFields(swFieldNameTemp) <= 0) {
    std::wstring strFieldName;
    int iControlNo = -1;
    ParseFieldName(swFieldNameTemp.c_str(), strFieldName, iControlNo);
    if (iControlNo == -1)
      return false;

    m_FieldName = strFieldName.c_str();
    m_nFormControlIndex = iControlNo;
    return true;
  }

  m_FieldName = swFieldNameTemp;
  m_nFormControlIndex = -1;

  return true;
}

std::vector<CPDF_FormField*> CJS_Field::GetFormFields(
    CPDFSDK_FormFillEnvironment* pFormFillEnv,
    const WideString& csFieldName) {
  std::vector<CPDF_FormField*> fields;
  CPDFSDK_InterForm* pReaderInterForm = pFormFillEnv->GetInterForm();
  CPDF_InterForm* pInterForm = pReaderInterForm->GetInterForm();
  for (int i = 0, sz = pInterForm->CountFields(csFieldName); i < sz; ++i) {
    if (CPDF_FormField* pFormField = pInterForm->GetField(i, csFieldName))
      fields.push_back(pFormField);
  }
  return fields;
}

std::vector<CPDF_FormField*> CJS_Field::GetFormFields(
    const WideString& csFieldName) const {
  return CJS_Field::GetFormFields(m_pFormFillEnv.Get(), csFieldName);
}

void CJS_Field::UpdateFormField(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                                CPDF_FormField* pFormField,
                                bool bChangeMark,
                                bool bResetAP,
                                bool bRefresh) {
  CPDFSDK_InterForm* pInterForm = pFormFillEnv->GetInterForm();

  if (bResetAP) {
    std::vector<CPDFSDK_Annot::ObservedPtr> widgets;
    pInterForm->GetWidgets(pFormField, &widgets);

    FormFieldType fieldType = pFormField->GetFieldType();
    if (fieldType == FormFieldType::kComboBox ||
        fieldType == FormFieldType::kTextField) {
      for (auto& pObserved : widgets) {
        if (pObserved) {
          bool bFormatted = false;
          WideString sValue = static_cast<CPDFSDK_Widget*>(pObserved.Get())
                                  ->OnFormat(bFormatted);
          if (pObserved) {  // Not redundant, may be clobbered by OnFormat.
            static_cast<CPDFSDK_Widget*>(pObserved.Get())
                ->ResetAppearance(bFormatted ? &sValue : nullptr, false);
          }
        }
      }
    } else {
      for (auto& pObserved : widgets) {
        if (pObserved) {
          static_cast<CPDFSDK_Widget*>(pObserved.Get())
              ->ResetAppearance(nullptr, false);
        }
      }
    }
  }

  if (bRefresh) {
    // Refresh the widget list. The calls in |bResetAP| may have caused widgets
    // to be removed from the list. We need to call |GetWidgets| again to be
    // sure none of the widgets have been deleted.
    std::vector<CPDFSDK_Annot::ObservedPtr> widgets;
    pInterForm->GetWidgets(pFormField, &widgets);

    // TODO(dsinclair): Determine if all widgets share the same
    // CPDFSDK_InterForm. If that's the case, we can move the code to
    // |GetFormFillEnv| out of the loop.
    for (auto& pObserved : widgets) {
      if (pObserved) {
        CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pObserved.Get());
        pWidget->GetInterForm()->GetFormFillEnv()->UpdateAllViews(nullptr,
                                                                  pWidget);
      }
    }
  }

  if (bChangeMark)
    pFormFillEnv->SetChangeMark();
}

void CJS_Field::UpdateFormControl(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                                  CPDF_FormControl* pFormControl,
                                  bool bChangeMark,
                                  bool bResetAP,
                                  bool bRefresh) {
  ASSERT(pFormControl);

  CPDFSDK_InterForm* pForm = pFormFillEnv->GetInterForm();
  CPDFSDK_Widget* pWidget = pForm->GetWidget(pFormControl);

  if (pWidget) {
    CPDFSDK_Widget::ObservedPtr observed_widget(pWidget);
    if (bResetAP) {
      FormFieldType fieldType = pWidget->GetFieldType();
      if (fieldType == FormFieldType::kComboBox ||
          fieldType == FormFieldType::kTextField) {
        bool bFormatted = false;
        WideString sValue = pWidget->OnFormat(bFormatted);
        if (!observed_widget)
          return;
        pWidget->ResetAppearance(bFormatted ? &sValue : nullptr, false);
      } else {
        pWidget->ResetAppearance(nullptr, false);
      }
      if (!observed_widget)
        return;
    }

    if (bRefresh) {
      CPDFSDK_InterForm* pInterForm = pWidget->GetInterForm();
      pInterForm->GetFormFillEnv()->UpdateAllViews(nullptr, pWidget);
    }
  }

  if (bChangeMark)
    pFormFillEnv->SetChangeMark();
}

CPDFSDK_Widget* CJS_Field::GetWidget(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                                     CPDF_FormControl* pFormControl) {
  CPDFSDK_InterForm* pInterForm =
      static_cast<CPDFSDK_InterForm*>(pFormFillEnv->GetInterForm());
  return pInterForm ? pInterForm->GetWidget(pFormControl) : nullptr;
}

bool CJS_Field::ValueIsOccur(CPDF_FormField* pFormField,
                             WideString csOptLabel) {
  for (int i = 0, sz = pFormField->CountOptions(); i < sz; i++) {
    if (csOptLabel.Compare(pFormField->GetOptionLabel(i)) == 0)
      return true;
  }

  return false;
}

CPDF_FormControl* CJS_Field::GetSmartFieldControl(CPDF_FormField* pFormField) {
  if (!pFormField->CountControls() ||
      m_nFormControlIndex >= pFormField->CountControls())
    return nullptr;
  if (m_nFormControlIndex < 0)
    return pFormField->GetControl(0);
  return pFormField->GetControl(m_nFormControlIndex);
}

CJS_Return CJS_Field::get_alignment(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  switch (pFormControl->GetControlAlignment()) {
    case 0:
      return CJS_Return(pRuntime->NewString(L"left"));
    case 1:
      return CJS_Return(pRuntime->NewString(L"center"));
    case 2:
      return CJS_Return(pRuntime->NewString(L"right"));
  }
  return CJS_Return(pRuntime->NewString(L""));
}

CJS_Return CJS_Field::set_alignment(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_border_style(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (!pFormField)
    return CJS_Return(false);

  CPDFSDK_Widget* pWidget =
      GetWidget(m_pFormFillEnv.Get(), GetSmartFieldControl(pFormField));
  if (!pWidget)
    return CJS_Return(false);

  switch (pWidget->GetBorderStyle()) {
    case BorderStyle::SOLID:
      return CJS_Return(pRuntime->NewString(L"solid"));
    case BorderStyle::DASH:
      return CJS_Return(pRuntime->NewString(L"dashed"));
    case BorderStyle::BEVELED:
      return CJS_Return(pRuntime->NewString(L"beveled"));
    case BorderStyle::INSET:
      return CJS_Return(pRuntime->NewString(L"inset"));
    case BorderStyle::UNDERLINE:
      return CJS_Return(pRuntime->NewString(L"underline"));
  }
  return CJS_Return(pRuntime->NewString(L""));
}

CJS_Return CJS_Field::set_border_style(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);

  if (!m_bCanSet)
    return CJS_Return(false);

  ByteString byte_str = ByteString::FromUnicode(pRuntime->ToWideString(vp));
  if (m_bDelay) {
    AddDelay_String(FP_BORDERSTYLE, byte_str);
  } else {
    CJS_Field::SetBorderStyle(m_pFormFillEnv.Get(), m_FieldName,
                              m_nFormControlIndex, byte_str);
  }
  return CJS_Return(true);
}

void CJS_Field::SetBorderStyle(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                               const WideString& swFieldName,
                               int nControlIndex,
                               const ByteString& string) {
  ASSERT(pFormFillEnv);

  BorderStyle nBorderStyle = BorderStyle::SOLID;
  if (string == "solid")
    nBorderStyle = BorderStyle::SOLID;
  else if (string == "beveled")
    nBorderStyle = BorderStyle::BEVELED;
  else if (string == "dashed")
    nBorderStyle = BorderStyle::DASH;
  else if (string == "inset")
    nBorderStyle = BorderStyle::INSET;
  else if (string == "underline")
    nBorderStyle = BorderStyle::UNDERLINE;
  else
    return;

  std::vector<CPDF_FormField*> FieldArray =
      GetFormFields(pFormFillEnv, swFieldName);
  for (CPDF_FormField* pFormField : FieldArray) {
    if (nControlIndex < 0) {
      bool bSet = false;
      for (int i = 0, sz = pFormField->CountControls(); i < sz; ++i) {
        if (CPDFSDK_Widget* pWidget =
                GetWidget(pFormFillEnv, pFormField->GetControl(i))) {
          if (pWidget->GetBorderStyle() != nBorderStyle) {
            pWidget->SetBorderStyle(nBorderStyle);
            bSet = true;
          }
        }
      }
      if (bSet)
        UpdateFormField(pFormFillEnv, pFormField, true, true, true);
    } else {
      if (nControlIndex >= pFormField->CountControls())
        return;
      if (CPDF_FormControl* pFormControl =
              pFormField->GetControl(nControlIndex)) {
        if (CPDFSDK_Widget* pWidget = GetWidget(pFormFillEnv, pFormControl)) {
          if (pWidget->GetBorderStyle() != nBorderStyle) {
            pWidget->SetBorderStyle(nBorderStyle);
            UpdateFormControl(pFormFillEnv, pFormControl, true, true, true);
          }
        }
      }
    }
  }
}

CJS_Return CJS_Field::get_button_align_x(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  CPDF_IconFit IconFit = pFormControl->GetIconFit();

  float fLeft;
  float fBottom;
  IconFit.GetIconPosition(fLeft, fBottom);

  return CJS_Return(pRuntime->NewNumber(static_cast<int32_t>(fLeft)));
}

CJS_Return CJS_Field::set_button_align_x(CJS_Runtime* pRuntime,
                                         v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_button_align_y(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  CPDF_IconFit IconFit = pFormControl->GetIconFit();

  float fLeft;
  float fBottom;
  IconFit.GetIconPosition(fLeft, fBottom);

  return CJS_Return(pRuntime->NewNumber(static_cast<int32_t>(fBottom)));
}

CJS_Return CJS_Field::set_button_align_y(CJS_Runtime* pRuntime,
                                         v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_button_fit_bounds(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  return CJS_Return(
      pRuntime->NewBoolean(pFormControl->GetIconFit().GetFittingBounds()));
}

CJS_Return CJS_Field::set_button_fit_bounds(CJS_Runtime* pRuntime,
                                            v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_button_position(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewNumber(pFormControl->GetTextPosition()));
}

CJS_Return CJS_Field::set_button_position(CJS_Runtime* pRuntime,
                                          v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_button_scale_how(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      pFormControl->GetIconFit().IsProportionalScale() ? 0 : 1));
}

CJS_Return CJS_Field::set_button_scale_how(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_button_scale_when(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  CPDF_IconFit IconFit = pFormControl->GetIconFit();
  int ScaleM = IconFit.GetScaleMethod();
  switch (ScaleM) {
    case CPDF_IconFit::Always:
      return CJS_Return(
          pRuntime->NewNumber(static_cast<int32_t>(CPDF_IconFit::Always)));
    case CPDF_IconFit::Bigger:
      return CJS_Return(
          pRuntime->NewNumber(static_cast<int32_t>(CPDF_IconFit::Bigger)));
    case CPDF_IconFit::Never:
      return CJS_Return(
          pRuntime->NewNumber(static_cast<int32_t>(CPDF_IconFit::Never)));
    case CPDF_IconFit::Smaller:
      return CJS_Return(
          pRuntime->NewNumber(static_cast<int32_t>(CPDF_IconFit::Smaller)));
  }
  return CJS_Return(true);
}

CJS_Return CJS_Field::set_button_scale_when(CJS_Runtime* pRuntime,
                                            v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_calc_order_index(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kComboBox &&
      pFormField->GetFieldType() != FormFieldType::kTextField) {
    return CJS_Return(false);
  }

  CPDFSDK_InterForm* pRDInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pInterForm = pRDInterForm->GetInterForm();
  return CJS_Return(pRuntime->NewNumber(static_cast<int32_t>(
      pInterForm->FindFieldInCalculationOrder(pFormField))));
}

CJS_Return CJS_Field::set_calc_order_index(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_char_limit(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);
  return CJS_Return(
      pRuntime->NewNumber(static_cast<int32_t>(pFormField->GetMaxLen())));
}

CJS_Return CJS_Field::set_char_limit(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_comb(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  return CJS_Return(
      pRuntime->NewBoolean(!!(pFormField->GetFieldFlags() & FIELDFLAG_COMB)));
}

CJS_Return CJS_Field::set_comb(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_commit_on_sel_change(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kComboBox &&
      pFormField->GetFieldType() != FormFieldType::kListBox) {
    return CJS_Return(false);
  }

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_COMMITONSELCHANGE)));
}

CJS_Return CJS_Field::set_commit_on_sel_change(CJS_Runtime* pRuntime,
                                               v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_current_value_indices(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kComboBox &&
      pFormField->GetFieldType() != FormFieldType::kListBox) {
    return CJS_Return(false);
  }

  int count = pFormField->CountSelectedItems();
  if (count <= 0)
    return CJS_Return(pRuntime->NewNumber(-1));
  if (count == 1)
    return CJS_Return(pRuntime->NewNumber(pFormField->GetSelectedIndex(0)));

  v8::Local<v8::Array> SelArray = pRuntime->NewArray();
  for (int i = 0; i < count; i++) {
    pRuntime->PutArrayElement(
        SelArray, i, pRuntime->NewNumber(pFormField->GetSelectedIndex(i)));
  }
  if (SelArray.IsEmpty())
    return CJS_Return(pRuntime->NewArray());
  return CJS_Return(SelArray);
}

CJS_Return CJS_Field::set_current_value_indices(CJS_Runtime* pRuntime,
                                                v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);

  std::vector<uint32_t> array;
  if (vp->IsNumber()) {
    array.push_back(pRuntime->ToInt32(vp));
  } else if (!vp.IsEmpty() && vp->IsArray()) {
    v8::Local<v8::Array> SelArray = pRuntime->ToArray(vp);
    for (size_t i = 0; i < pRuntime->GetArrayLength(SelArray); i++) {
      array.push_back(
          pRuntime->ToInt32(pRuntime->GetArrayElement(SelArray, i)));
    }
  }

  if (m_bDelay) {
    AddDelay_WordArray(FP_CURRENTVALUEINDICES, array);
  } else {
    CJS_Field::SetCurrentValueIndices(m_pFormFillEnv.Get(), m_FieldName,
                                      m_nFormControlIndex, array);
  }
  return CJS_Return(true);
}

void CJS_Field::SetCurrentValueIndices(
    CPDFSDK_FormFillEnvironment* pFormFillEnv,
    const WideString& swFieldName,
    int nControlIndex,
    const std::vector<uint32_t>& array) {
  ASSERT(pFormFillEnv);
  std::vector<CPDF_FormField*> FieldArray =
      GetFormFields(pFormFillEnv, swFieldName);

  for (CPDF_FormField* pFormField : FieldArray) {
    FormFieldType fieldType = pFormField->GetFieldType();
    if (fieldType == FormFieldType::kComboBox ||
        fieldType == FormFieldType::kListBox) {
      uint32_t dwFieldFlags = pFormField->GetFieldFlags();
      pFormField->ClearSelection(true);
      for (size_t i = 0; i < array.size(); ++i) {
        if (i != 0 && !(dwFieldFlags & (1 << 21)))
          break;
        if (array[i] < static_cast<uint32_t>(pFormField->CountOptions()) &&
            !pFormField->IsItemSelected(array[i])) {
          pFormField->SetItemSelection(array[i], true);
        }
      }
      UpdateFormField(pFormFillEnv, pFormField, true, true, true);
    }
  }
}

CJS_Return CJS_Field::get_default_style(CJS_Runtime* pRuntime) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::set_default_style(CJS_Runtime* pRuntime,
                                        v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::get_default_value(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() == FormFieldType::kPushButton ||
      pFormField->GetFieldType() == FormFieldType::kSignature) {
    return CJS_Return(false);
  }

  return CJS_Return(pRuntime->NewString(pFormField->GetDefaultValue().c_str()));
}

CJS_Return CJS_Field::set_default_value(CJS_Runtime* pRuntime,
                                        v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_do_not_scroll(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_DONOTSCROLL)));
}

CJS_Return CJS_Field::set_do_not_scroll(CJS_Runtime* pRuntime,
                                        v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_do_not_spell_check(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField &&
      pFormField->GetFieldType() != FormFieldType::kComboBox) {
    return CJS_Return(false);
  }

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_DONOTSPELLCHECK)));
}

CJS_Return CJS_Field::set_do_not_spell_check(CJS_Runtime* pRuntime,
                                             v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

void CJS_Field::SetDelay(bool bDelay) {
  m_bDelay = bDelay;

  if (m_bDelay)
    return;
  if (m_pJSDoc)
    m_pJSDoc->DoFieldDelay(m_FieldName, m_nFormControlIndex);
}

CJS_Return CJS_Field::get_delay(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewBoolean(m_bDelay));
}

CJS_Return CJS_Field::set_delay(CJS_Runtime* pRuntime,
                                v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);

  SetDelay(pRuntime->ToBoolean(vp));
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_display(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  ASSERT(pFormField);

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDFSDK_Widget* pWidget =
      pInterForm->GetWidget(GetSmartFieldControl(pFormField));
  if (!pWidget)
    return CJS_Return(false);

  uint32_t dwFlag = pWidget->GetFlags();
  if (ANNOTFLAG_INVISIBLE & dwFlag || ANNOTFLAG_HIDDEN & dwFlag)
    return CJS_Return(pRuntime->NewNumber(1));

  if (ANNOTFLAG_PRINT & dwFlag) {
    if (ANNOTFLAG_NOVIEW & dwFlag)
      return CJS_Return(pRuntime->NewNumber(3));
    return CJS_Return(pRuntime->NewNumber(0));
  }
  return CJS_Return(pRuntime->NewNumber(2));
}

CJS_Return CJS_Field::set_display(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);

  if (m_bDelay) {
    AddDelay_Int(FP_DISPLAY, pRuntime->ToInt32(vp));
  } else {
    CJS_Field::SetDisplay(m_pFormFillEnv.Get(), m_FieldName,
                          m_nFormControlIndex, pRuntime->ToInt32(vp));
  }
  return CJS_Return(true);
}

void CJS_Field::SetDisplay(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                           const WideString& swFieldName,
                           int nControlIndex,
                           int number) {
  CPDFSDK_InterForm* pInterForm = pFormFillEnv->GetInterForm();
  std::vector<CPDF_FormField*> FieldArray =
      GetFormFields(pFormFillEnv, swFieldName);
  for (CPDF_FormField* pFormField : FieldArray) {
    if (nControlIndex < 0) {
      bool bAnySet = false;
      for (int i = 0, sz = pFormField->CountControls(); i < sz; ++i) {
        CPDF_FormControl* pFormControl = pFormField->GetControl(i);
        ASSERT(pFormControl);

        CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl);
        if (SetWidgetDisplayStatus(pWidget, number))
          bAnySet = true;
      }

      if (bAnySet)
        UpdateFormField(pFormFillEnv, pFormField, true, false, true);
    } else {
      if (nControlIndex >= pFormField->CountControls())
        return;

      CPDF_FormControl* pFormControl = pFormField->GetControl(nControlIndex);
      if (!pFormControl)
        return;

      CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl);
      if (SetWidgetDisplayStatus(pWidget, number))
        UpdateFormControl(pFormFillEnv, pFormControl, true, false, true);
    }
  }
}

CJS_Return CJS_Field::get_doc(CJS_Runtime* pRuntime) {
  return CJS_Return(m_pJSDoc->ToV8Object());
}

CJS_Return CJS_Field::set_doc(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::get_editable(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kComboBox)
    return CJS_Return(false);

  return CJS_Return(
      pRuntime->NewBoolean(!!(pFormField->GetFieldFlags() & FIELDFLAG_EDIT)));
}

CJS_Return CJS_Field::set_editable(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_export_values(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kCheckBox &&
      pFormField->GetFieldType() != FormFieldType::kRadioButton) {
    return CJS_Return(false);
  }

  v8::Local<v8::Array> ExportValuesArray = pRuntime->NewArray();
  if (m_nFormControlIndex < 0) {
    for (int i = 0, sz = pFormField->CountControls(); i < sz; i++) {
      CPDF_FormControl* pFormControl = pFormField->GetControl(i);
      pRuntime->PutArrayElement(
          ExportValuesArray, i,
          pRuntime->NewString(pFormControl->GetExportValue().c_str()));
    }
  } else {
    if (m_nFormControlIndex >= pFormField->CountControls())
      return CJS_Return(false);

    CPDF_FormControl* pFormControl =
        pFormField->GetControl(m_nFormControlIndex);
    if (!pFormControl)
      return CJS_Return(false);

    pRuntime->PutArrayElement(
        ExportValuesArray, 0,
        pRuntime->NewString(pFormControl->GetExportValue().c_str()));
  }
  return CJS_Return(ExportValuesArray);
}

CJS_Return CJS_Field::set_export_values(CJS_Runtime* pRuntime,
                                        v8::Local<v8::Value> vp) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kCheckBox &&
      pFormField->GetFieldType() != FormFieldType::kRadioButton) {
    return CJS_Return(false);
  }

  return CJS_Return(m_bCanSet && !vp.IsEmpty() && vp->IsArray());
}

CJS_Return CJS_Field::get_file_select(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_FILESELECT)));
}

CJS_Return CJS_Field::set_file_select(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_fill_color(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  ASSERT(pFormField);
  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  int iColorType;
  pFormControl->GetBackgroundColor(iColorType);

  CFX_Color color;
  if (iColorType == CFX_Color::kTransparent) {
    color = CFX_Color(CFX_Color::kTransparent);
  } else if (iColorType == CFX_Color::kGray) {
    color = CFX_Color(CFX_Color::kGray,
                      pFormControl->GetOriginalBackgroundColor(0));
  } else if (iColorType == CFX_Color::kRGB) {
    color =
        CFX_Color(CFX_Color::kRGB, pFormControl->GetOriginalBackgroundColor(0),
                  pFormControl->GetOriginalBackgroundColor(1),
                  pFormControl->GetOriginalBackgroundColor(2));
  } else if (iColorType == CFX_Color::kCMYK) {
    color =
        CFX_Color(CFX_Color::kCMYK, pFormControl->GetOriginalBackgroundColor(0),
                  pFormControl->GetOriginalBackgroundColor(1),
                  pFormControl->GetOriginalBackgroundColor(2),
                  pFormControl->GetOriginalBackgroundColor(3));
  } else {
    return CJS_Return(false);
  }

  v8::Local<v8::Value> array =
      CJS_Color::ConvertPWLColorToArray(pRuntime, color);
  if (array.IsEmpty())
    return CJS_Return(pRuntime->NewArray());
  return CJS_Return(array);
}

CJS_Return CJS_Field::set_fill_color(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);
  if (!m_bCanSet)
    return CJS_Return(false);
  if (vp.IsEmpty() || !vp->IsArray())
    return CJS_Return(false);
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_hidden(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  ASSERT(pFormField);

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDFSDK_Widget* pWidget =
      pInterForm->GetWidget(GetSmartFieldControl(pFormField));
  if (!pWidget)
    return CJS_Return(false);

  uint32_t dwFlags = pWidget->GetFlags();
  return CJS_Return(pRuntime->NewBoolean(ANNOTFLAG_INVISIBLE & dwFlags ||
                                         ANNOTFLAG_HIDDEN & dwFlags));
}

CJS_Return CJS_Field::set_hidden(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);

  if (m_bDelay) {
    AddDelay_Bool(FP_HIDDEN, pRuntime->ToBoolean(vp));
  } else {
    CJS_Field::SetHidden(m_pFormFillEnv.Get(), m_FieldName, m_nFormControlIndex,
                         pRuntime->ToBoolean(vp));
  }
  return CJS_Return(true);
}

void CJS_Field::SetHidden(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                          const WideString& swFieldName,
                          int nControlIndex,
                          bool b) {
  int display = b ? 1 /*Hidden*/ : 0 /*Visible*/;
  SetDisplay(pFormFillEnv, swFieldName, nControlIndex, display);
}

CJS_Return CJS_Field::get_highlight(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  int eHM = pFormControl->GetHighlightingMode();
  switch (eHM) {
    case CPDF_FormControl::None:
      return CJS_Return(pRuntime->NewString(L"none"));
    case CPDF_FormControl::Push:
      return CJS_Return(pRuntime->NewString(L"push"));
    case CPDF_FormControl::Invert:
      return CJS_Return(pRuntime->NewString(L"invert"));
    case CPDF_FormControl::Outline:
      return CJS_Return(pRuntime->NewString(L"outline"));
    case CPDF_FormControl::Toggle:
      return CJS_Return(pRuntime->NewString(L"toggle"));
  }
  return CJS_Return(true);
}

CJS_Return CJS_Field::set_highlight(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_line_width(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  ASSERT(pFormField);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  if (!pFormField->CountControls())
    return CJS_Return(false);

  CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormField->GetControl(0));
  if (!pWidget)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewNumber(pWidget->GetBorderWidth()));
}

CJS_Return CJS_Field::set_line_width(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);

  if (m_bDelay) {
    AddDelay_Int(FP_LINEWIDTH, pRuntime->ToInt32(vp));
  } else {
    CJS_Field::SetLineWidth(m_pFormFillEnv.Get(), m_FieldName,
                            m_nFormControlIndex, pRuntime->ToInt32(vp));
  }
  return CJS_Return(true);
}

void CJS_Field::SetLineWidth(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                             const WideString& swFieldName,
                             int nControlIndex,
                             int number) {
  CPDFSDK_InterForm* pInterForm = pFormFillEnv->GetInterForm();
  std::vector<CPDF_FormField*> FieldArray =
      GetFormFields(pFormFillEnv, swFieldName);
  for (CPDF_FormField* pFormField : FieldArray) {
    if (nControlIndex < 0) {
      bool bSet = false;
      for (int i = 0, sz = pFormField->CountControls(); i < sz; ++i) {
        CPDF_FormControl* pFormControl = pFormField->GetControl(i);
        ASSERT(pFormControl);

        if (CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl)) {
          if (number != pWidget->GetBorderWidth()) {
            pWidget->SetBorderWidth(number);
            bSet = true;
          }
        }
      }
      if (bSet)
        UpdateFormField(pFormFillEnv, pFormField, true, true, true);
    } else {
      if (nControlIndex >= pFormField->CountControls())
        return;
      if (CPDF_FormControl* pFormControl =
              pFormField->GetControl(nControlIndex)) {
        if (CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl)) {
          if (number != pWidget->GetBorderWidth()) {
            pWidget->SetBorderWidth(number);
            UpdateFormControl(pFormFillEnv, pFormControl, true, true, true);
          }
        }
      }
    }
  }
}

CJS_Return CJS_Field::get_multiline(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_MULTILINE)));
}

CJS_Return CJS_Field::set_multiline(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_multiple_selection(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kListBox)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_MULTISELECT)));
}

CJS_Return CJS_Field::set_multiple_selection(CJS_Runtime* pRuntime,
                                             v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_name(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewString(m_FieldName.c_str()));
}

CJS_Return CJS_Field::set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::get_num_items(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kComboBox &&
      pFormField->GetFieldType() != FormFieldType::kListBox) {
    return CJS_Return(false);
  }

  return CJS_Return(pRuntime->NewNumber(pFormField->CountOptions()));
}

CJS_Return CJS_Field::set_num_items(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::get_page(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (!pFormField)
    return CJS_Return(false);

  std::vector<CPDFSDK_Annot::ObservedPtr> widgets;
  m_pFormFillEnv->GetInterForm()->GetWidgets(pFormField, &widgets);
  if (widgets.empty())
    return CJS_Return(pRuntime->NewNumber(-1));

  v8::Local<v8::Array> PageArray = pRuntime->NewArray();
  int i = 0;
  for (const auto& pObserved : widgets) {
    if (!pObserved)
      return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

    auto* pWidget = static_cast<CPDFSDK_Widget*>(pObserved.Get());
    CPDFSDK_PageView* pPageView = pWidget->GetPageView();
    if (!pPageView)
      return CJS_Return(false);

    pRuntime->PutArrayElement(
        PageArray, i,
        pRuntime->NewNumber(static_cast<int32_t>(pPageView->GetPageIndex())));
    ++i;
  }
  return CJS_Return(PageArray);
}

CJS_Return CJS_Field::set_page(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Field::get_password(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_PASSWORD)));
}

CJS_Return CJS_Field::set_password(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_print(CJS_Runtime* pRuntime) {
  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  CPDFSDK_Widget* pWidget =
      pInterForm->GetWidget(GetSmartFieldControl(pFormField));
  if (!pWidget)
    return CJS_Return(false);

  return CJS_Return(
      pRuntime->NewBoolean(!!(pWidget->GetFlags() & ANNOTFLAG_PRINT)));
}

CJS_Return CJS_Field::set_print(CJS_Runtime* pRuntime,
                                v8::Local<v8::Value> vp) {
  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  if (!m_bCanSet)
    return CJS_Return(false);

  for (CPDF_FormField* pFormField : FieldArray) {
    if (m_nFormControlIndex < 0) {
      bool bSet = false;
      for (int i = 0, sz = pFormField->CountControls(); i < sz; ++i) {
        if (CPDFSDK_Widget* pWidget =
                pInterForm->GetWidget(pFormField->GetControl(i))) {
          uint32_t dwFlags = pWidget->GetFlags();
          if (pRuntime->ToBoolean(vp))
            dwFlags |= ANNOTFLAG_PRINT;
          else
            dwFlags &= ~ANNOTFLAG_PRINT;

          if (dwFlags != pWidget->GetFlags()) {
            pWidget->SetFlags(dwFlags);
            bSet = true;
          }
        }
      }

      if (bSet)
        UpdateFormField(m_pFormFillEnv.Get(), pFormField, true, false, true);

      continue;
    }

    if (m_nFormControlIndex >= pFormField->CountControls())
      return CJS_Return(false);

    if (CPDF_FormControl* pFormControl =
            pFormField->GetControl(m_nFormControlIndex)) {
      if (CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl)) {
        uint32_t dwFlags = pWidget->GetFlags();
        if (pRuntime->ToBoolean(vp))
          dwFlags |= ANNOTFLAG_PRINT;
        else
          dwFlags &= ~ANNOTFLAG_PRINT;

        if (dwFlags != pWidget->GetFlags()) {
          pWidget->SetFlags(dwFlags);
          UpdateFormControl(m_pFormFillEnv.Get(),
                            pFormField->GetControl(m_nFormControlIndex), true,
                            false, true);
        }
      }
    }
  }
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_radios_in_unison(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kRadioButton)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_RADIOSINUNISON)));
}

CJS_Return CJS_Field::set_radios_in_unison(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_readonly(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(FieldArray[0]->GetFieldFlags() & FIELDFLAG_READONLY)));
}

CJS_Return CJS_Field::set_readonly(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_rect(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDFSDK_Widget* pWidget =
      pInterForm->GetWidget(GetSmartFieldControl(pFormField));
  if (!pWidget)
    return CJS_Return(false);

  CFX_FloatRect crRect = pWidget->GetRect();
  v8::Local<v8::Array> rcArray = pRuntime->NewArray();
  pRuntime->PutArrayElement(
      rcArray, 0, pRuntime->NewNumber(static_cast<int32_t>(crRect.left)));
  pRuntime->PutArrayElement(
      rcArray, 1, pRuntime->NewNumber(static_cast<int32_t>(crRect.top)));
  pRuntime->PutArrayElement(
      rcArray, 2, pRuntime->NewNumber(static_cast<int32_t>(crRect.right)));
  pRuntime->PutArrayElement(
      rcArray, 3, pRuntime->NewNumber(static_cast<int32_t>(crRect.bottom)));

  return CJS_Return(rcArray);
}

CJS_Return CJS_Field::set_rect(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);
  if (vp.IsEmpty() || !vp->IsArray())
    return CJS_Return(false);

  v8::Local<v8::Array> rcArray = pRuntime->ToArray(vp);
  if (pRuntime->GetArrayLength(rcArray) < 4)
    return CJS_Return(false);

  float pArray[4];
  pArray[0] = static_cast<float>(
      pRuntime->ToInt32(pRuntime->GetArrayElement(rcArray, 0)));
  pArray[1] = static_cast<float>(
      pRuntime->ToInt32(pRuntime->GetArrayElement(rcArray, 1)));
  pArray[2] = static_cast<float>(
      pRuntime->ToInt32(pRuntime->GetArrayElement(rcArray, 2)));
  pArray[3] = static_cast<float>(
      pRuntime->ToInt32(pRuntime->GetArrayElement(rcArray, 3)));

  CFX_FloatRect crRect(pArray);
  if (m_bDelay) {
    AddDelay_Rect(FP_RECT, crRect);
  } else {
    CJS_Field::SetRect(m_pFormFillEnv.Get(), m_FieldName, m_nFormControlIndex,
                       crRect);
  }
  return CJS_Return(true);
}

void CJS_Field::SetRect(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                        const WideString& swFieldName,
                        int nControlIndex,
                        const CFX_FloatRect& rect) {
  CPDFSDK_InterForm* pInterForm = pFormFillEnv->GetInterForm();
  std::vector<CPDF_FormField*> FieldArray =
      GetFormFields(pFormFillEnv, swFieldName);
  for (CPDF_FormField* pFormField : FieldArray) {
    if (nControlIndex < 0) {
      bool bSet = false;
      for (int i = 0, sz = pFormField->CountControls(); i < sz; ++i) {
        CPDF_FormControl* pFormControl = pFormField->GetControl(i);
        ASSERT(pFormControl);

        if (CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl)) {
          CFX_FloatRect crRect = rect;

          CPDF_Page* pPDFPage = pWidget->GetPDFPage();
          crRect.Intersect(pPDFPage->GetPageBBox());

          if (!crRect.IsEmpty()) {
            CFX_FloatRect rcOld = pWidget->GetRect();
            if (crRect.left != rcOld.left || crRect.right != rcOld.right ||
                crRect.top != rcOld.top || crRect.bottom != rcOld.bottom) {
              pWidget->SetRect(crRect);
              bSet = true;
            }
          }
        }
      }

      if (bSet)
        UpdateFormField(pFormFillEnv, pFormField, true, true, true);

      continue;
    }

    if (nControlIndex >= pFormField->CountControls())
      return;
    if (CPDF_FormControl* pFormControl =
            pFormField->GetControl(nControlIndex)) {
      if (CPDFSDK_Widget* pWidget = pInterForm->GetWidget(pFormControl)) {
        CFX_FloatRect crRect = rect;

        CPDF_Page* pPDFPage = pWidget->GetPDFPage();
        crRect.Intersect(pPDFPage->GetPageBBox());

        if (!crRect.IsEmpty()) {
          CFX_FloatRect rcOld = pWidget->GetRect();
          if (crRect.left != rcOld.left || crRect.right != rcOld.right ||
              crRect.top != rcOld.top || crRect.bottom != rcOld.bottom) {
            pWidget->SetRect(crRect);
            UpdateFormControl(pFormFillEnv, pFormControl, true, true, true);
          }
        }
      }
    }
  }
}

CJS_Return CJS_Field::get_required(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() == FormFieldType::kPushButton)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_REQUIRED)));
}

CJS_Return CJS_Field::set_required(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_rich_text(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kTextField)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      !!(pFormField->GetFieldFlags() & FIELDFLAG_RICHTEXT)));
}

CJS_Return CJS_Field::set_rich_text(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_rich_value(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::set_rich_value(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_rotation(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewNumber(pFormControl->GetRotation()));
}

CJS_Return CJS_Field::set_rotation(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_stroke_color(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  int iColorType;
  pFormControl->GetBorderColor(iColorType);

  CFX_Color color;
  if (iColorType == CFX_Color::kTransparent) {
    color = CFX_Color(CFX_Color::kTransparent);
  } else if (iColorType == CFX_Color::kGray) {
    color =
        CFX_Color(CFX_Color::kGray, pFormControl->GetOriginalBorderColor(0));
  } else if (iColorType == CFX_Color::kRGB) {
    color = CFX_Color(CFX_Color::kRGB, pFormControl->GetOriginalBorderColor(0),
                      pFormControl->GetOriginalBorderColor(1),
                      pFormControl->GetOriginalBorderColor(2));
  } else if (iColorType == CFX_Color::kCMYK) {
    color = CFX_Color(CFX_Color::kCMYK, pFormControl->GetOriginalBorderColor(0),
                      pFormControl->GetOriginalBorderColor(1),
                      pFormControl->GetOriginalBorderColor(2),
                      pFormControl->GetOriginalBorderColor(3));
  } else {
    return CJS_Return(false);
  }

  v8::Local<v8::Value> array =
      CJS_Color::ConvertPWLColorToArray(pRuntime, color);
  if (array.IsEmpty())
    return CJS_Return(pRuntime->NewArray());
  return CJS_Return(array);
}

CJS_Return CJS_Field::set_stroke_color(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);
  if (vp.IsEmpty() || !vp->IsArray())
    return CJS_Return(false);
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_style(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kRadioButton &&
      pFormField->GetFieldType() != FormFieldType::kCheckBox) {
    return CJS_Return(false);
  }

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  WideString csWCaption = pFormControl->GetNormalCaption();
  ByteString csBCaption;

  switch (csWCaption[0]) {
    case L'l':
      csBCaption = "circle";
      break;
    case L'8':
      csBCaption = "cross";
      break;
    case L'u':
      csBCaption = "diamond";
      break;
    case L'n':
      csBCaption = "square";
      break;
    case L'H':
      csBCaption = "star";
      break;
    default:  // L'4'
      csBCaption = "check";
      break;
  }
  return CJS_Return(
      pRuntime->NewString(WideString::FromLocal(csBCaption.c_str()).c_str()));
}

CJS_Return CJS_Field::set_style(CJS_Runtime* pRuntime,
                                v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_submit_name(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::set_submit_name(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_text_color(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  Optional<CFX_Color::Type> iColorType;
  FX_ARGB color;
  CPDF_DefaultAppearance FieldAppearance = pFormControl->GetDefaultAppearance();
  std::tie(iColorType, color) = FieldAppearance.GetColor();

  CFX_Color crRet;
  if (!iColorType || *iColorType == CFX_Color::kTransparent) {
    crRet = CFX_Color(CFX_Color::kTransparent);
  } else {
    int32_t a;
    int32_t r;
    int32_t g;
    int32_t b;
    std::tie(a, r, g, b) = ArgbDecode(color);
    crRet = CFX_Color(CFX_Color::kRGB, r / 255.0f, g / 255.0f, b / 255.0f);
  }

  v8::Local<v8::Value> array =
      CJS_Color::ConvertPWLColorToArray(pRuntime, crRet);
  if (array.IsEmpty())
    return CJS_Return(pRuntime->NewArray());
  return CJS_Return(array);
}

CJS_Return CJS_Field::set_text_color(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);
  if (vp.IsEmpty() || !vp->IsArray())
    return CJS_Return(false);
  return CJS_Return(true);
}

CJS_Return CJS_Field::get_text_font(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  ASSERT(pFormField);
  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  FormFieldType fieldType = pFormField->GetFieldType();
  if (fieldType != FormFieldType::kPushButton &&
      fieldType != FormFieldType::kComboBox &&
      fieldType != FormFieldType::kListBox &&
      fieldType != FormFieldType::kTextField) {
    return CJS_Return(false);
  }

  CPDF_Font* pFont = pFormControl->GetDefaultControlFont();
  if (!pFont)
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewString(
      WideString::FromLocal(pFont->GetBaseFont().c_str()).c_str()));
}

CJS_Return CJS_Field::set_text_font(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);

  if (!m_bCanSet)
    return CJS_Return(false);
  return CJS_Return(
      !ByteString::FromUnicode(pRuntime->ToWideString(vp)).IsEmpty());
}

CJS_Return CJS_Field::get_text_size(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  ASSERT(pFormField);
  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  float fFontSize;
  CPDF_DefaultAppearance FieldAppearance = pFormControl->GetDefaultAppearance();
  FieldAppearance.GetFont(&fFontSize);
  return CJS_Return(pRuntime->NewNumber(static_cast<int>(fFontSize)));
}

CJS_Return CJS_Field::set_text_size(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_type(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  switch (pFormField->GetFieldType()) {
    case FormFieldType::kUnknown:
      return CJS_Return(pRuntime->NewString(L"unknown"));
    case FormFieldType::kPushButton:
      return CJS_Return(pRuntime->NewString(L"button"));
    case FormFieldType::kCheckBox:
      return CJS_Return(pRuntime->NewString(L"checkbox"));
    case FormFieldType::kRadioButton:
      return CJS_Return(pRuntime->NewString(L"radiobutton"));
    case FormFieldType::kComboBox:
      return CJS_Return(pRuntime->NewString(L"combobox"));
    case FormFieldType::kListBox:
      return CJS_Return(pRuntime->NewString(L"listbox"));
    case FormFieldType::kTextField:
      return CJS_Return(pRuntime->NewString(L"text"));
    case FormFieldType::kSignature:
      return CJS_Return(pRuntime->NewString(L"signature"));
    default:
      return CJS_Return(pRuntime->NewString(L"unknown"));
  }
}

CJS_Return CJS_Field::set_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::get_user_name(CJS_Runtime* pRuntime) {
  ASSERT(m_pFormFillEnv);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  return CJS_Return(
      pRuntime->NewString(FieldArray[0]->GetAlternateName().c_str()));
}

CJS_Return CJS_Field::set_user_name(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  ASSERT(m_pFormFillEnv);
  return CJS_Return(m_bCanSet);
}

CJS_Return CJS_Field::get_value(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  v8::Local<v8::Value> ret;

  CPDF_FormField* pFormField = FieldArray[0];
  switch (pFormField->GetFieldType()) {
    case FormFieldType::kPushButton:
      return CJS_Return(false);
    case FormFieldType::kComboBox:
    case FormFieldType::kTextField:
      ret = pRuntime->NewString(pFormField->GetValue().c_str());
      break;
    case FormFieldType::kListBox: {
      if (pFormField->CountSelectedItems() > 1) {
        v8::Local<v8::Array> ValueArray = pRuntime->NewArray();
        v8::Local<v8::Value> ElementValue;
        int iIndex;
        for (int i = 0, sz = pFormField->CountSelectedItems(); i < sz; i++) {
          iIndex = pFormField->GetSelectedIndex(i);
          ElementValue =
              pRuntime->NewString(pFormField->GetOptionValue(iIndex).c_str());
          if (wcslen(pRuntime->ToWideString(ElementValue).c_str()) == 0) {
            ElementValue =
                pRuntime->NewString(pFormField->GetOptionLabel(iIndex).c_str());
          }
          pRuntime->PutArrayElement(ValueArray, i, ElementValue);
        }
        ret = ValueArray;
      } else {
        ret = pRuntime->NewString(pFormField->GetValue().c_str());
      }
      break;
    }
    case FormFieldType::kCheckBox:
    case FormFieldType::kRadioButton: {
      bool bFind = false;
      for (int i = 0, sz = pFormField->CountControls(); i < sz; i++) {
        if (pFormField->GetControl(i)->IsChecked()) {
          ret = pRuntime->NewString(
              pFormField->GetControl(i)->GetExportValue().c_str());
          bFind = true;
          break;
        }
      }
      if (!bFind)
        ret = pRuntime->NewString(L"Off");

      break;
    }
    default:
      ret = pRuntime->NewString(pFormField->GetValue().c_str());
      break;
  }
  return CJS_Return(pRuntime->MaybeCoerceToNumber(ret));
}

CJS_Return CJS_Field::set_value(CJS_Runtime* pRuntime,
                                v8::Local<v8::Value> vp) {
  if (!m_bCanSet)
    return CJS_Return(false);

  std::vector<WideString> strArray;
  if (!vp.IsEmpty() && vp->IsArray()) {
    v8::Local<v8::Array> ValueArray = pRuntime->ToArray(vp);
    for (size_t i = 0; i < pRuntime->GetArrayLength(ValueArray); i++) {
      strArray.push_back(
          pRuntime->ToWideString(pRuntime->GetArrayElement(ValueArray, i)));
    }
  } else {
    strArray.push_back(pRuntime->ToWideString(vp));
  }

  if (m_bDelay) {
    AddDelay_WideStringArray(FP_VALUE, strArray);
  } else {
    CJS_Field::SetValue(m_pFormFillEnv.Get(), m_FieldName, m_nFormControlIndex,
                        strArray);
  }
  return CJS_Return(true);
}

void CJS_Field::SetValue(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                         const WideString& swFieldName,
                         int nControlIndex,
                         const std::vector<WideString>& strArray) {
  ASSERT(pFormFillEnv);
  if (strArray.empty())
    return;

  std::vector<CPDF_FormField*> FieldArray =
      GetFormFields(pFormFillEnv, swFieldName);

  for (CPDF_FormField* pFormField : FieldArray) {
    if (pFormField->GetFullName().Compare(swFieldName) != 0)
      continue;

    switch (pFormField->GetFieldType()) {
      case FormFieldType::kTextField:
      case FormFieldType::kComboBox:
        if (pFormField->GetValue() != strArray[0]) {
          pFormField->SetValue(strArray[0], true);
          UpdateFormField(pFormFillEnv, pFormField, true, false, true);
        }
        break;
      case FormFieldType::kCheckBox:
      case FormFieldType::kRadioButton:
        if (pFormField->GetValue() != strArray[0]) {
          pFormField->SetValue(strArray[0], true);
          UpdateFormField(pFormFillEnv, pFormField, true, false, true);
        }
        break;
      case FormFieldType::kListBox: {
        bool bModified = false;
        for (const auto& str : strArray) {
          if (!pFormField->IsItemSelected(pFormField->FindOption(str))) {
            bModified = true;
            break;
          }
        }
        if (bModified) {
          pFormField->ClearSelection(true);
          for (const auto& str : strArray) {
            int index = pFormField->FindOption(str);
            if (!pFormField->IsItemSelected(index))
              pFormField->SetItemSelection(index, true, true);
          }
          UpdateFormField(pFormFillEnv, pFormField, true, false, true);
        }
        break;
      }
      default:
        break;
    }
  }
}

CJS_Return CJS_Field::get_value_as_string(CJS_Runtime* pRuntime) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() == FormFieldType::kPushButton)
    return CJS_Return(false);

  if (pFormField->GetFieldType() == FormFieldType::kCheckBox) {
    if (!pFormField->CountControls())
      return CJS_Return(false);
    return CJS_Return(pRuntime->NewString(
        pFormField->GetControl(0)->IsChecked() ? L"Yes" : L"Off"));
  }

  if (pFormField->GetFieldType() == FormFieldType::kRadioButton &&
      !(pFormField->GetFieldFlags() & FIELDFLAG_RADIOSINUNISON)) {
    for (int i = 0, sz = pFormField->CountControls(); i < sz; i++) {
      if (pFormField->GetControl(i)->IsChecked()) {
        return CJS_Return(pRuntime->NewString(
            pFormField->GetControl(i)->GetExportValue().c_str()));
      }
    }
    return CJS_Return(pRuntime->NewString(L"Off"));
  }

  if (pFormField->GetFieldType() == FormFieldType::kListBox &&
      (pFormField->CountSelectedItems() > 1)) {
    return CJS_Return(pRuntime->NewString(L""));
  }
  return CJS_Return(pRuntime->NewString(pFormField->GetValue().c_str()));
}

CJS_Return CJS_Field::set_value_as_string(CJS_Runtime* pRuntime,
                                          v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::browseForFileToSubmit(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if ((pFormField->GetFieldFlags() & FIELDFLAG_FILESELECT) &&
      (pFormField->GetFieldType() == FormFieldType::kTextField)) {
    WideString wsFileName = m_pFormFillEnv->JS_fieldBrowse();
    if (!wsFileName.IsEmpty()) {
      pFormField->SetValue(wsFileName);
      UpdateFormField(m_pFormFillEnv.Get(), pFormField, true, true, true);
    }
    return CJS_Return(true);
  }
  return CJS_Return(false);
}

CJS_Return CJS_Field::buttonGetCaption(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  int nface = 0;
  int iSize = params.size();
  if (iSize >= 1)
    nface = pRuntime->ToInt32(params[0]);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  if (nface == 0) {
    return CJS_Return(
        pRuntime->NewString(pFormControl->GetNormalCaption().c_str()));
  } else if (nface == 1) {
    return CJS_Return(
        pRuntime->NewString(pFormControl->GetDownCaption().c_str()));
  } else if (nface == 2) {
    return CJS_Return(
        pRuntime->NewString(pFormControl->GetRolloverCaption().c_str()));
  }
  return CJS_Return(false);
}

CJS_Return CJS_Field::buttonGetIcon(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() >= 1) {
    int nFace = pRuntime->ToInt32(params[0]);
    if (nFace < 0 || nFace > 2)
      return CJS_Return(false);
  }

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kPushButton)
    return CJS_Return(false);

  CPDF_FormControl* pFormControl = GetSmartFieldControl(pFormField);
  if (!pFormControl)
    return CJS_Return(false);

  v8::Local<v8::Object> pObj =
      pRuntime->NewFXJSBoundObject(CJS_Icon::GetObjDefnID());
  if (pObj.IsEmpty())
    return CJS_Return(false);

  CJS_Icon* pJS_Icon = static_cast<CJS_Icon*>(pRuntime->GetObjectPrivate(pObj));
  if (!pJS_Icon)
    return CJS_Return(false);
  return CJS_Return(pJS_Icon->ToV8Object());
}

CJS_Return CJS_Field::buttonImportIcon(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::buttonSetCaption(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::buttonSetIcon(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::checkThisBox(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  int iSize = params.size();
  if (iSize < 1)
    return CJS_Return(false);

  if (!m_bCanSet)
    return CJS_Return(false);

  int nWidget = pRuntime->ToInt32(params[0]);
  bool bCheckit = true;
  if (iSize >= 2)
    bCheckit = pRuntime->ToBoolean(params[1]);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (pFormField->GetFieldType() != FormFieldType::kCheckBox &&
      pFormField->GetFieldType() != FormFieldType::kRadioButton) {
    return CJS_Return(false);
  }
  if (nWidget < 0 || nWidget >= pFormField->CountControls())
    return CJS_Return(false);

  // TODO(weili): Check whether anything special needed for radio button.
  // (When pFormField->GetFieldType() == FormFieldType::kRadioButton.)
  pFormField->CheckControl(nWidget, bCheckit, true);

  UpdateFormField(m_pFormFillEnv.Get(), pFormField, true, true, true);
  return CJS_Return(true);
}

CJS_Return CJS_Field::clearItems(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::defaultIsChecked(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_bCanSet)
    return CJS_Return(false);

  int iSize = params.size();
  if (iSize < 1)
    return CJS_Return(false);

  int nWidget = pRuntime->ToInt32(params[0]);
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (nWidget < 0 || nWidget >= pFormField->CountControls())
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      pFormField->GetFieldType() == FormFieldType::kCheckBox ||
      pFormField->GetFieldType() == FormFieldType::kRadioButton));
}

CJS_Return CJS_Field::deleteItemAt(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::getArray(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  std::vector<std::unique_ptr<WideString>> swSort;
  for (CPDF_FormField* pFormField : FieldArray) {
    swSort.push_back(
        std::unique_ptr<WideString>(new WideString(pFormField->GetFullName())));
  }

  std::sort(swSort.begin(), swSort.end(),
            [](const std::unique_ptr<WideString>& p1,
               const std::unique_ptr<WideString>& p2) { return *p1 < *p2; });

  v8::Local<v8::Array> FormFieldArray = pRuntime->NewArray();
  int j = 0;
  for (const auto& pStr : swSort) {
    v8::Local<v8::Object> pObj =
        pRuntime->NewFXJSBoundObject(CJS_Field::GetObjDefnID());
    if (pObj.IsEmpty())
      return CJS_Return(false);

    CJS_Field* pJSField =
        static_cast<CJS_Field*>(pRuntime->GetObjectPrivate(pObj));
    pJSField->AttachField(m_pJSDoc, *pStr);
    pRuntime->PutArrayElement(FormFieldArray, j++,
                              pJSField
                                  ? v8::Local<v8::Value>(pJSField->ToV8Object())
                                  : v8::Local<v8::Value>());
  }
  return CJS_Return(FormFieldArray);
}

CJS_Return CJS_Field::getItemAt(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  int iSize = params.size();
  int nIdx = -1;
  if (iSize >= 1)
    nIdx = pRuntime->ToInt32(params[0]);

  bool bExport = true;
  if (iSize >= 2)
    bExport = pRuntime->ToBoolean(params[1]);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if ((pFormField->GetFieldType() == FormFieldType::kListBox) ||
      (pFormField->GetFieldType() == FormFieldType::kComboBox)) {
    if (nIdx == -1 || nIdx > pFormField->CountOptions())
      nIdx = pFormField->CountOptions() - 1;
    if (bExport) {
      WideString strval = pFormField->GetOptionValue(nIdx);
      if (strval.IsEmpty()) {
        return CJS_Return(
            pRuntime->NewString(pFormField->GetOptionLabel(nIdx).c_str()));
      }
      return CJS_Return(pRuntime->NewString(strval.c_str()));
    }
    return CJS_Return(
        pRuntime->NewString(pFormField->GetOptionLabel(nIdx).c_str()));
  }
  return CJS_Return(false);
}

CJS_Return CJS_Field::getLock(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::insertItemAt(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::isBoxChecked(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  int nIndex = -1;
  if (params.size() >= 1)
    nIndex = pRuntime->ToInt32(params[0]);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (nIndex < 0 || nIndex >= pFormField->CountControls())
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      ((pFormField->GetFieldType() == FormFieldType::kCheckBox ||
        pFormField->GetFieldType() == FormFieldType::kRadioButton) &&
       pFormField->GetControl(nIndex)->IsChecked() != 0)));
}

CJS_Return CJS_Field::isDefaultChecked(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  int nIndex = -1;
  if (params.size() >= 1)
    nIndex = pRuntime->ToInt32(params[0]);

  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  if (nIndex < 0 || nIndex >= pFormField->CountControls())
    return CJS_Return(false);

  return CJS_Return(pRuntime->NewBoolean(
      ((pFormField->GetFieldType() == FormFieldType::kCheckBox ||
        pFormField->GetFieldType() == FormFieldType::kRadioButton) &&
       pFormField->GetControl(nIndex)->IsDefaultChecked() != 0)));
}

CJS_Return CJS_Field::setAction(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::setFocus(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  std::vector<CPDF_FormField*> FieldArray = GetFormFields(m_FieldName);
  if (FieldArray.empty())
    return CJS_Return(false);

  CPDF_FormField* pFormField = FieldArray[0];
  int32_t nCount = pFormField->CountControls();
  if (nCount < 1)
    return CJS_Return(false);

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDFSDK_Widget* pWidget = nullptr;
  if (nCount == 1) {
    pWidget = pInterForm->GetWidget(pFormField->GetControl(0));
  } else {
    UnderlyingPageType* pPage =
        UnderlyingFromFPDFPage(m_pFormFillEnv->GetCurrentPage());
    if (!pPage)
      return CJS_Return(false);
    if (CPDFSDK_PageView* pCurPageView =
            m_pFormFillEnv->GetPageView(pPage, true)) {
      for (int32_t i = 0; i < nCount; i++) {
        if (CPDFSDK_Widget* pTempWidget =
                pInterForm->GetWidget(pFormField->GetControl(i))) {
          if (pTempWidget->GetPDFPage() == pCurPageView->GetPDFPage()) {
            pWidget = pTempWidget;
            break;
          }
        }
      }
    }
  }

  if (pWidget) {
    CPDFSDK_Annot::ObservedPtr pObserved(pWidget);
    m_pFormFillEnv->SetFocusAnnot(&pObserved);
  }

  return CJS_Return(true);
}

CJS_Return CJS_Field::setItems(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::setLock(CJS_Runtime* pRuntime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::signatureGetModifications(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::signatureGetSeedValue(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::signatureInfo(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::signatureSetSeedValue(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::signatureSign(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::signatureValidate(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(false);
}

CJS_Return CJS_Field::get_source(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Field::set_source(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

void CJS_Field::AddDelay_Int(FIELD_PROP prop, int32_t n) {
  auto pNewData =
      pdfium::MakeUnique<CJS_DelayData>(prop, m_nFormControlIndex, m_FieldName);
  pNewData->num = n;
  m_pJSDoc->AddDelayData(std::move(pNewData));
}

void CJS_Field::AddDelay_Bool(FIELD_PROP prop, bool b) {
  auto pNewData =
      pdfium::MakeUnique<CJS_DelayData>(prop, m_nFormControlIndex, m_FieldName);
  pNewData->b = b;
  m_pJSDoc->AddDelayData(std::move(pNewData));
}

void CJS_Field::AddDelay_String(FIELD_PROP prop, const ByteString& string) {
  auto pNewData =
      pdfium::MakeUnique<CJS_DelayData>(prop, m_nFormControlIndex, m_FieldName);
  pNewData->string = string;
  m_pJSDoc->AddDelayData(std::move(pNewData));
}

void CJS_Field::AddDelay_Rect(FIELD_PROP prop, const CFX_FloatRect& rect) {
  auto pNewData =
      pdfium::MakeUnique<CJS_DelayData>(prop, m_nFormControlIndex, m_FieldName);
  pNewData->rect = rect;
  m_pJSDoc->AddDelayData(std::move(pNewData));
}

void CJS_Field::AddDelay_WordArray(FIELD_PROP prop,
                                   const std::vector<uint32_t>& array) {
  auto pNewData =
      pdfium::MakeUnique<CJS_DelayData>(prop, m_nFormControlIndex, m_FieldName);
  pNewData->wordarray = array;
  m_pJSDoc->AddDelayData(std::move(pNewData));
}

void CJS_Field::AddDelay_WideStringArray(FIELD_PROP prop,
                                         const std::vector<WideString>& array) {
  auto pNewData =
      pdfium::MakeUnique<CJS_DelayData>(prop, m_nFormControlIndex, m_FieldName);
  pNewData->widestringarray = array;
  m_pJSDoc->AddDelayData(std::move(pNewData));
}

void CJS_Field::DoDelay(CPDFSDK_FormFillEnvironment* pFormFillEnv,
                        CJS_DelayData* pData) {
  ASSERT(pFormFillEnv);
  switch (pData->eProp) {
    case FP_BORDERSTYLE:
      CJS_Field::SetBorderStyle(pFormFillEnv, pData->sFieldName,
                                pData->nControlIndex, pData->string);
      break;
    case FP_CURRENTVALUEINDICES:
      CJS_Field::SetCurrentValueIndices(pFormFillEnv, pData->sFieldName,
                                        pData->nControlIndex, pData->wordarray);
      break;
    case FP_DISPLAY:
      CJS_Field::SetDisplay(pFormFillEnv, pData->sFieldName,
                            pData->nControlIndex, pData->num);
      break;
    case FP_HIDDEN:
      CJS_Field::SetHidden(pFormFillEnv, pData->sFieldName,
                           pData->nControlIndex, pData->b);
      break;
    case FP_LINEWIDTH:
      CJS_Field::SetLineWidth(pFormFillEnv, pData->sFieldName,
                              pData->nControlIndex, pData->num);
      break;
    case FP_RECT:
      CJS_Field::SetRect(pFormFillEnv, pData->sFieldName, pData->nControlIndex,
                         pData->rect);
      break;
    case FP_VALUE:
      CJS_Field::SetValue(pFormFillEnv, pData->sFieldName, pData->nControlIndex,
                          pData->widestringarray);
      break;
    default:
      NOTREACHED();
  }
}
