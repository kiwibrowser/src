// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_interform.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cfdf_document.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfdoc/cpdf_actionfields.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_pathdata.h"
#include "fpdfsdk/cpdfsdk_actionhandler.h"
#include "fpdfsdk/cpdfsdk_annot.h"
#include "fpdfsdk/cpdfsdk_annotiterator.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/cpdfsdk_widget.h"
#include "fpdfsdk/formfiller/cffl_formfiller.h"
#include "fpdfsdk/ipdfsdk_annothandler.h"
#include "fxjs/ijs_event_context.h"
#include "fxjs/ijs_runtime.h"
#include "third_party/base/stl_util.h"

#ifdef PDF_ENABLE_XFA
#include "fpdfsdk/cpdfsdk_xfawidget.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#include "fpdfsdk/fpdfxfa/cxfa_fwladaptertimermgr.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffdocview.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "xfa/fxfa/cxfa_ffwidgethandler.h"
#endif  // PDF_ENABLE_XFA

namespace {

constexpr uint32_t kWhiteBGR = FXSYS_BGR(255, 255, 255);

bool IsFormFieldTypeComboOrText(FormFieldType fieldType) {
  switch (fieldType) {
    case FormFieldType::kComboBox:
    case FormFieldType::kTextField:
      return true;
    default:
      return false;
  }
}

#ifdef PDF_ENABLE_XFA
bool IsFormFieldTypeXFA(FormFieldType fieldType) {
  switch (fieldType) {
    case FormFieldType::kXFA:
    case FormFieldType::kXFA_CheckBox:
    case FormFieldType::kXFA_ComboBox:
    case FormFieldType::kXFA_ImageField:
    case FormFieldType::kXFA_ListBox:
    case FormFieldType::kXFA_PushButton:
    case FormFieldType::kXFA_Signature:
    case FormFieldType::kXFA_TextField:
      return true;
    default:
      return false;
  }
}
#endif  // PDF_ENABLE_XFA

}  // namespace

CPDFSDK_InterForm::CPDFSDK_InterForm(CPDFSDK_FormFillEnvironment* pFormFillEnv)
    : m_pFormFillEnv(pFormFillEnv),
      m_pInterForm(
          pdfium::MakeUnique<CPDF_InterForm>(m_pFormFillEnv->GetPDFDocument())),
#ifdef PDF_ENABLE_XFA
      m_bXfaCalculate(true),
      m_bXfaValidationsEnabled(true),
#endif  // PDF_ENABLE_XFA
      m_bCalculate(true),
      m_bBusy(false),
      m_HighlightAlpha(0) {
  m_pInterForm->SetFormNotify(this);
  RemoveAllHighLights();
}

CPDFSDK_InterForm::~CPDFSDK_InterForm() {
  m_Map.clear();
#ifdef PDF_ENABLE_XFA
  m_XFAMap.clear();
#endif  // PDF_ENABLE_XFA
}

bool CPDFSDK_InterForm::HighlightWidgets() {
  return false;
}

CPDFSDK_Widget* CPDFSDK_InterForm::GetSibling(CPDFSDK_Widget* pWidget,
                                              bool bNext) const {
  auto pIterator = pdfium::MakeUnique<CPDFSDK_AnnotIterator>(
      pWidget->GetPageView(), CPDF_Annot::Subtype::WIDGET);

  if (bNext)
    return static_cast<CPDFSDK_Widget*>(pIterator->GetNextAnnot(pWidget));

  return static_cast<CPDFSDK_Widget*>(pIterator->GetPrevAnnot(pWidget));
}

CPDFSDK_Widget* CPDFSDK_InterForm::GetWidget(CPDF_FormControl* pControl) const {
  if (!pControl || !m_pInterForm)
    return nullptr;

  CPDFSDK_Widget* pWidget = nullptr;
  const auto it = m_Map.find(pControl);
  if (it != m_Map.end())
    pWidget = it->second;
  if (pWidget)
    return pWidget;

  CPDF_Dictionary* pControlDict = pControl->GetWidget();
  CPDF_Document* pDocument = m_pFormFillEnv->GetPDFDocument();
  CPDFSDK_PageView* pPage = nullptr;

  if (CPDF_Dictionary* pPageDict = pControlDict->GetDictFor("P")) {
    int nPageIndex = pDocument->GetPageIndex(pPageDict->GetObjNum());
    if (nPageIndex >= 0)
      pPage = m_pFormFillEnv->GetPageView(nPageIndex);
  }

  if (!pPage) {
    int nPageIndex = GetPageIndexByAnnotDict(pDocument, pControlDict);
    if (nPageIndex >= 0)
      pPage = m_pFormFillEnv->GetPageView(nPageIndex);
  }

  if (!pPage)
    return nullptr;

  return static_cast<CPDFSDK_Widget*>(pPage->GetAnnotByDict(pControlDict));
}

void CPDFSDK_InterForm::GetWidgets(
    const WideString& sFieldName,
    std::vector<CPDFSDK_Annot::ObservedPtr>* widgets) const {
  for (int i = 0, sz = m_pInterForm->CountFields(sFieldName); i < sz; ++i) {
    CPDF_FormField* pFormField = m_pInterForm->GetField(i, sFieldName);
    ASSERT(pFormField);
    GetWidgets(pFormField, widgets);
  }
}

void CPDFSDK_InterForm::GetWidgets(
    CPDF_FormField* pField,
    std::vector<CPDFSDK_Annot::ObservedPtr>* widgets) const {
  for (int i = 0, sz = pField->CountControls(); i < sz; ++i) {
    CPDF_FormControl* pFormCtrl = pField->GetControl(i);
    ASSERT(pFormCtrl);
    CPDFSDK_Widget* pWidget = GetWidget(pFormCtrl);
    if (pWidget)
      widgets->emplace_back(pWidget);
  }
}

int CPDFSDK_InterForm::GetPageIndexByAnnotDict(
    CPDF_Document* pDocument,
    CPDF_Dictionary* pAnnotDict) const {
  ASSERT(pAnnotDict);

  for (int i = 0, sz = pDocument->GetPageCount(); i < sz; i++) {
    if (CPDF_Dictionary* pPageDict = pDocument->GetPageDictionary(i)) {
      if (CPDF_Array* pAnnots = pPageDict->GetArrayFor("Annots")) {
        for (int j = 0, jsz = pAnnots->GetCount(); j < jsz; j++) {
          CPDF_Object* pDict = pAnnots->GetDirectObjectAt(j);
          if (pAnnotDict == pDict)
            return i;
        }
      }
    }
  }

  return -1;
}

void CPDFSDK_InterForm::AddMap(CPDF_FormControl* pControl,
                               CPDFSDK_Widget* pWidget) {
  m_Map[pControl] = pWidget;
}

void CPDFSDK_InterForm::RemoveMap(CPDF_FormControl* pControl) {
  m_Map.erase(pControl);
}

void CPDFSDK_InterForm::EnableCalculate(bool bEnabled) {
  m_bCalculate = bEnabled;
}

bool CPDFSDK_InterForm::IsCalculateEnabled() const {
  return m_bCalculate;
}

#ifdef PDF_ENABLE_XFA
void CPDFSDK_InterForm::AddXFAMap(CXFA_FFWidget* hWidget,
                                  CPDFSDK_XFAWidget* pWidget) {
  ASSERT(hWidget);
  m_XFAMap[hWidget] = pWidget;
}

void CPDFSDK_InterForm::RemoveXFAMap(CXFA_FFWidget* hWidget) {
  ASSERT(hWidget);
  m_XFAMap.erase(hWidget);
}

CPDFSDK_XFAWidget* CPDFSDK_InterForm::GetXFAWidget(CXFA_FFWidget* hWidget) {
  ASSERT(hWidget);
  auto it = m_XFAMap.find(hWidget);
  return it != m_XFAMap.end() ? it->second : nullptr;
}

void CPDFSDK_InterForm::XfaEnableCalculate(bool bEnabled) {
  m_bXfaCalculate = bEnabled;
}
bool CPDFSDK_InterForm::IsXfaCalculateEnabled() const {
  return m_bXfaCalculate;
}

bool CPDFSDK_InterForm::IsXfaValidationsEnabled() {
  return m_bXfaValidationsEnabled;
}
void CPDFSDK_InterForm::XfaSetValidationsEnabled(bool bEnabled) {
  m_bXfaValidationsEnabled = bEnabled;
}

void CPDFSDK_InterForm::SynchronizeField(CPDF_FormField* pFormField) {
  for (int i = 0, sz = pFormField->CountControls(); i < sz; i++) {
    CPDF_FormControl* pFormCtrl = pFormField->GetControl(i);
    if (CPDFSDK_Widget* pWidget = GetWidget(pFormCtrl))
      pWidget->Synchronize(false);
  }
}
#endif  // PDF_ENABLE_XFA

void CPDFSDK_InterForm::OnCalculate(CPDF_FormField* pFormField) {
  if (!m_pFormFillEnv->IsJSPlatformPresent())
    return;

  if (m_bBusy)
    return;

  m_bBusy = true;

  if (!IsCalculateEnabled()) {
    m_bBusy = false;
    return;
  }

  IJS_Runtime* pRuntime = m_pFormFillEnv->GetIJSRuntime();
  int nSize = m_pInterForm->CountFieldsInCalculationOrder();
  for (int i = 0; i < nSize; i++) {
    CPDF_FormField* pField = m_pInterForm->GetFieldInCalculationOrder(i);
    if (!pField)
      continue;

    FormFieldType fieldType = pField->GetFieldType();
    if (!IsFormFieldTypeComboOrText(fieldType))
      continue;

    CPDF_AAction aAction = pField->GetAdditionalAction();
    if (!aAction.GetDict() || !aAction.ActionExist(CPDF_AAction::Calculate))
      continue;

    CPDF_Action action = aAction.GetAction(CPDF_AAction::Calculate);
    if (!action.GetDict())
      continue;

    WideString csJS = action.GetJavaScript();
    if (csJS.IsEmpty())
      continue;

    IJS_EventContext* pContext = pRuntime->NewEventContext();
    WideString sOldValue = pField->GetValue();
    WideString sValue = sOldValue;
    bool bRC = true;
    pContext->OnField_Calculate(pFormField, pField, sValue, bRC);

    Optional<IJS_Runtime::JS_Error> err = pContext->RunScript(csJS);
    pRuntime->ReleaseEventContext(pContext);
    if (!err && bRC && sValue.Compare(sOldValue) != 0)
      pField->SetValue(sValue, true);
  }
  m_bBusy = false;
}

WideString CPDFSDK_InterForm::OnFormat(CPDF_FormField* pFormField,
                                       bool& bFormatted) {
  WideString sValue = pFormField->GetValue();
  if (!m_pFormFillEnv->IsJSPlatformPresent()) {
    bFormatted = false;
    return sValue;
  }

  IJS_Runtime* pRuntime = m_pFormFillEnv->GetIJSRuntime();
  if (pFormField->GetFieldType() == FormFieldType::kComboBox &&
      pFormField->CountSelectedItems() > 0) {
    int index = pFormField->GetSelectedIndex(0);
    if (index >= 0)
      sValue = pFormField->GetOptionLabel(index);
  }

  bFormatted = false;

  CPDF_AAction aAction = pFormField->GetAdditionalAction();
  if (aAction.GetDict() && aAction.ActionExist(CPDF_AAction::Format)) {
    CPDF_Action action = aAction.GetAction(CPDF_AAction::Format);
    if (action.GetDict()) {
      WideString script = action.GetJavaScript();
      if (!script.IsEmpty()) {
        WideString Value = sValue;

        IJS_EventContext* pContext = pRuntime->NewEventContext();
        pContext->OnField_Format(pFormField, Value, true);

        Optional<IJS_Runtime::JS_Error> err = pContext->RunScript(script);
        pRuntime->ReleaseEventContext(pContext);
        if (!err) {
          sValue = Value;
          bFormatted = true;
        }
      }
    }
  }
  return sValue;
}

void CPDFSDK_InterForm::ResetFieldAppearance(CPDF_FormField* pFormField,
                                             const WideString* sValue,
                                             bool bValueChanged) {
  for (int i = 0, sz = pFormField->CountControls(); i < sz; i++) {
    CPDF_FormControl* pFormCtrl = pFormField->GetControl(i);
    ASSERT(pFormCtrl);
    if (CPDFSDK_Widget* pWidget = GetWidget(pFormCtrl))
      pWidget->ResetAppearance(sValue, bValueChanged);
  }
}

void CPDFSDK_InterForm::UpdateField(CPDF_FormField* pFormField) {
  auto* formfiller = m_pFormFillEnv->GetInteractiveFormFiller();
  for (int i = 0, sz = pFormField->CountControls(); i < sz; i++) {
    CPDF_FormControl* pFormCtrl = pFormField->GetControl(i);
    ASSERT(pFormCtrl);

    CPDFSDK_Widget* pWidget = GetWidget(pFormCtrl);
    if (!pWidget)
      continue;

    UnderlyingPageType* pPage = pWidget->GetUnderlyingPage();
    FX_RECT rect = formfiller->GetViewBBox(
        m_pFormFillEnv->GetPageView(pPage, false), pWidget);
    m_pFormFillEnv->Invalidate(pPage, rect);
  }
}

bool CPDFSDK_InterForm::OnKeyStrokeCommit(CPDF_FormField* pFormField,
                                          const WideString& csValue) {
  CPDF_AAction aAction = pFormField->GetAdditionalAction();
  if (!aAction.GetDict() || !aAction.ActionExist(CPDF_AAction::KeyStroke))
    return true;

  CPDF_Action action = aAction.GetAction(CPDF_AAction::KeyStroke);
  if (!action.GetDict())
    return true;

  CPDFSDK_ActionHandler* pActionHandler = m_pFormFillEnv->GetActionHandler();
  CPDFSDK_FieldAction fa;
  fa.bModifier = false;
  fa.bShift = false;
  fa.sValue = csValue;
  pActionHandler->DoAction_FieldJavaScript(
      action, CPDF_AAction::KeyStroke, m_pFormFillEnv.Get(), pFormField, &fa);
  return fa.bRC;
}

bool CPDFSDK_InterForm::OnValidate(CPDF_FormField* pFormField,
                                   const WideString& csValue) {
  CPDF_AAction aAction = pFormField->GetAdditionalAction();
  if (!aAction.GetDict() || !aAction.ActionExist(CPDF_AAction::Validate))
    return true;

  CPDF_Action action = aAction.GetAction(CPDF_AAction::Validate);
  if (!action.GetDict())
    return true;

  CPDFSDK_ActionHandler* pActionHandler = m_pFormFillEnv->GetActionHandler();
  CPDFSDK_FieldAction fa;
  fa.bModifier = false;
  fa.bShift = false;
  fa.sValue = csValue;
  pActionHandler->DoAction_FieldJavaScript(
      action, CPDF_AAction::Validate, m_pFormFillEnv.Get(), pFormField, &fa);
  return fa.bRC;
}

bool CPDFSDK_InterForm::DoAction_Hide(const CPDF_Action& action) {
  ASSERT(action.GetDict());

  CPDF_ActionFields af(&action);
  std::vector<CPDF_Object*> fieldObjects = af.GetAllFields();
  std::vector<CPDF_FormField*> fields = GetFieldFromObjects(fieldObjects);

  bool bHide = action.GetHideStatus();
  bool bChanged = false;

  for (CPDF_FormField* pField : fields) {
    for (int i = 0, sz = pField->CountControls(); i < sz; ++i) {
      CPDF_FormControl* pControl = pField->GetControl(i);
      ASSERT(pControl);

      if (CPDFSDK_Widget* pWidget = GetWidget(pControl)) {
        uint32_t nFlags = pWidget->GetFlags();
        nFlags &= ~ANNOTFLAG_INVISIBLE;
        nFlags &= ~ANNOTFLAG_NOVIEW;
        if (bHide)
          nFlags |= ANNOTFLAG_HIDDEN;
        else
          nFlags &= ~ANNOTFLAG_HIDDEN;
        pWidget->SetFlags(nFlags);
        pWidget->GetPageView()->UpdateView(pWidget);
        bChanged = true;
      }
    }
  }

  return bChanged;
}

bool CPDFSDK_InterForm::DoAction_SubmitForm(const CPDF_Action& action) {
  WideString sDestination = action.GetFilePath();
  if (sDestination.IsEmpty())
    return false;

  CPDF_Dictionary* pActionDict = action.GetDict();
  if (pActionDict->KeyExist("Fields")) {
    CPDF_ActionFields af(&action);
    uint32_t dwFlags = action.GetFlags();
    std::vector<CPDF_Object*> fieldObjects = af.GetAllFields();
    std::vector<CPDF_FormField*> fields = GetFieldFromObjects(fieldObjects);
    if (!fields.empty()) {
      bool bIncludeOrExclude = !(dwFlags & 0x01);
      if (!m_pInterForm->CheckRequiredFields(&fields, bIncludeOrExclude))
        return false;

      return SubmitFields(sDestination, fields, bIncludeOrExclude, false);
    }
  }
  if (!m_pInterForm->CheckRequiredFields(nullptr, true))
    return false;

  return SubmitForm(sDestination, false);
}

bool CPDFSDK_InterForm::SubmitFields(const WideString& csDestination,
                                     const std::vector<CPDF_FormField*>& fields,
                                     bool bIncludeOrExclude,
                                     bool bUrlEncoded) {
  ByteString textBuf = ExportFieldsToFDFTextBuf(fields, bIncludeOrExclude);
  size_t nBufSize = textBuf.GetLength();
  if (nBufSize == 0)
    return false;

  uint8_t* pLocalBuffer = FX_Alloc(uint8_t, nBufSize);
  memcpy(pLocalBuffer, textBuf.c_str(), nBufSize);
  uint8_t* pBuffer = pLocalBuffer;

  if (bUrlEncoded && !FDFToURLEncodedData(pBuffer, nBufSize)) {
    FX_Free(pLocalBuffer);
    return false;
  }

  m_pFormFillEnv->JS_docSubmitForm(pBuffer, nBufSize, csDestination);

  if (pBuffer != pLocalBuffer)
    FX_Free(pBuffer);

  FX_Free(pLocalBuffer);

  return true;
}

bool CPDFSDK_InterForm::FDFToURLEncodedData(uint8_t*& pBuf, size_t& nBufSize) {
  std::unique_ptr<CFDF_Document> pFDF =
      CFDF_Document::ParseMemory(pBuf, nBufSize);
  if (!pFDF)
    return true;

  CPDF_Dictionary* pMainDict = pFDF->GetRoot()->GetDictFor("FDF");
  if (!pMainDict)
    return false;

  CPDF_Array* pFields = pMainDict->GetArrayFor("Fields");
  if (!pFields)
    return false;

  std::ostringstream fdfEncodedData;
  for (uint32_t i = 0; i < pFields->GetCount(); i++) {
    CPDF_Dictionary* pField = pFields->GetDictAt(i);
    if (!pField)
      continue;
    WideString name;
    name = pField->GetUnicodeTextFor("T");
    ByteString name_b = ByteString::FromUnicode(name);
    ByteString csBValue = pField->GetStringFor("V");
    WideString csWValue = PDF_DecodeText(csBValue);
    ByteString csValue_b = ByteString::FromUnicode(csWValue);
    fdfEncodedData << name_b.c_str() << "=" << csValue_b.c_str();
    if (i != pFields->GetCount() - 1)
      fdfEncodedData << "&";
  }

  nBufSize = fdfEncodedData.tellp();
  if (nBufSize <= 0)
    return false;

  pBuf = FX_Alloc(uint8_t, nBufSize);
  memcpy(pBuf, fdfEncodedData.str().c_str(), nBufSize);
  return true;
}

ByteString CPDFSDK_InterForm::ExportFieldsToFDFTextBuf(
    const std::vector<CPDF_FormField*>& fields,
    bool bIncludeOrExclude) {
  std::unique_ptr<CFDF_Document> pFDF = m_pInterForm->ExportToFDF(
      m_pFormFillEnv->JS_docGetFilePath(), fields, bIncludeOrExclude, false);

  return pFDF ? pFDF->WriteToString() : ByteString();
}

bool CPDFSDK_InterForm::SubmitForm(const WideString& sDestination,
                                   bool bUrlEncoded) {
  if (sDestination.IsEmpty())
    return false;

  if (!m_pFormFillEnv || !m_pInterForm)
    return false;

  std::unique_ptr<CFDF_Document> pFDFDoc =
      m_pInterForm->ExportToFDF(m_pFormFillEnv->JS_docGetFilePath(), false);
  if (!pFDFDoc)
    return false;

  ByteString fdfBuffer = pFDFDoc->WriteToString();

  if (fdfBuffer.IsEmpty())
    return false;

  uint8_t* pLocalBuffer = FX_Alloc(uint8_t, fdfBuffer.GetLength());
  memcpy(pLocalBuffer, fdfBuffer.c_str(), fdfBuffer.GetLength());

  uint8_t* pBuffer = pLocalBuffer;
  size_t nBufSize = fdfBuffer.GetLength();
  if (bUrlEncoded && !FDFToURLEncodedData(pBuffer, nBufSize)) {
    FX_Free(pLocalBuffer);
    return false;
  }

  m_pFormFillEnv->JS_docSubmitForm(pBuffer, nBufSize, sDestination);

  if (pBuffer != pLocalBuffer)
    FX_Free(pBuffer);

  FX_Free(pLocalBuffer);

  return true;
}

ByteString CPDFSDK_InterForm::ExportFormToFDFTextBuf() {
  std::unique_ptr<CFDF_Document> pFDF =
      m_pInterForm->ExportToFDF(m_pFormFillEnv->JS_docGetFilePath(), false);

  return pFDF ? pFDF->WriteToString() : ByteString();
}

void CPDFSDK_InterForm::DoAction_ResetForm(const CPDF_Action& action) {
  ASSERT(action.GetDict());

  CPDF_Dictionary* pActionDict = action.GetDict();
  if (!pActionDict->KeyExist("Fields")) {
    m_pInterForm->ResetForm(true);
    return;
  }

  CPDF_ActionFields af(&action);
  uint32_t dwFlags = action.GetFlags();

  std::vector<CPDF_Object*> fieldObjects = af.GetAllFields();
  std::vector<CPDF_FormField*> fields = GetFieldFromObjects(fieldObjects);
  m_pInterForm->ResetForm(fields, !(dwFlags & 0x01), true);
}

std::vector<CPDF_FormField*> CPDFSDK_InterForm::GetFieldFromObjects(
    const std::vector<CPDF_Object*>& objects) const {
  std::vector<CPDF_FormField*> fields;
  for (CPDF_Object* pObject : objects) {
    if (!pObject || !pObject->IsString())
      continue;

    WideString csName = pObject->GetUnicodeText();
    CPDF_FormField* pField = m_pInterForm->GetField(0, csName);
    if (pField)
      fields.push_back(pField);
  }
  return fields;
}

bool CPDFSDK_InterForm::BeforeValueChange(CPDF_FormField* pField,
                                          const WideString& csValue) {
  FormFieldType fieldType = pField->GetFieldType();
  if (!IsFormFieldTypeComboOrText(fieldType))
    return true;
  if (!OnKeyStrokeCommit(pField, csValue))
    return false;
  return OnValidate(pField, csValue);
}

void CPDFSDK_InterForm::AfterValueChange(CPDF_FormField* pField) {
#ifdef PDF_ENABLE_XFA
  SynchronizeField(pField);
#endif  // PDF_ENABLE_XFA

  FormFieldType fieldType = pField->GetFieldType();
  if (!IsFormFieldTypeComboOrText(fieldType))
    return;

  OnCalculate(pField);
  bool bFormatted = false;
  WideString sValue = OnFormat(pField, bFormatted);
  ResetFieldAppearance(pField, bFormatted ? &sValue : nullptr, true);
  UpdateField(pField);
}

bool CPDFSDK_InterForm::BeforeSelectionChange(CPDF_FormField* pField,
                                              const WideString& csValue) {
  if (pField->GetFieldType() != FormFieldType::kListBox)
    return true;
  if (!OnKeyStrokeCommit(pField, csValue))
    return false;
  return OnValidate(pField, csValue);
}

void CPDFSDK_InterForm::AfterSelectionChange(CPDF_FormField* pField) {
  if (pField->GetFieldType() != FormFieldType::kListBox)
    return;

  OnCalculate(pField);
  ResetFieldAppearance(pField, nullptr, true);
  UpdateField(pField);
}

void CPDFSDK_InterForm::AfterCheckedStatusChange(CPDF_FormField* pField) {
  FormFieldType fieldType = pField->GetFieldType();
  if (fieldType != FormFieldType::kCheckBox &&
      fieldType != FormFieldType::kRadioButton)
    return;

  OnCalculate(pField);
  UpdateField(pField);
}

void CPDFSDK_InterForm::AfterFormReset(CPDF_InterForm* pForm) {
  OnCalculate(nullptr);
}

bool CPDFSDK_InterForm::IsNeedHighLight(FormFieldType fieldType) {
  if (fieldType == FormFieldType::kUnknown)
    return false;

#ifdef PDF_ENABLE_XFA
  // For the XFA fields, we need to return if the specific field type has
  // highlight enabled or if the general XFA field type has it enabled.
  if (IsFormFieldTypeXFA(fieldType)) {
    if (!m_NeedsHighlight[static_cast<size_t>(fieldType)])
      return m_NeedsHighlight[static_cast<size_t>(FormFieldType::kXFA)];
  }
#endif  // PDF_ENABLE_XFA
  return m_NeedsHighlight[static_cast<size_t>(fieldType)];
}

void CPDFSDK_InterForm::RemoveAllHighLights() {
  std::fill(m_HighlightColor, m_HighlightColor + kFormFieldTypeCount,
            kWhiteBGR);
  std::fill(m_NeedsHighlight, m_NeedsHighlight + kFormFieldTypeCount, false);
}

void CPDFSDK_InterForm::SetHighlightColor(FX_COLORREF clr,
                                          FormFieldType fieldType) {
  if (fieldType == FormFieldType::kUnknown)
    return;

  m_HighlightColor[static_cast<size_t>(fieldType)] = clr;
  m_NeedsHighlight[static_cast<size_t>(fieldType)] = true;
}

void CPDFSDK_InterForm::SetAllHighlightColors(FX_COLORREF clr) {
  for (auto type : kFormFieldTypes) {
    m_HighlightColor[static_cast<size_t>(type)] = clr;
    m_NeedsHighlight[static_cast<size_t>(type)] = true;
  }
}

FX_COLORREF CPDFSDK_InterForm::GetHighlightColor(FormFieldType fieldType) {
  if (fieldType == FormFieldType::kUnknown)
    return kWhiteBGR;

#ifdef PDF_ENABLE_XFA
  // For the XFA fields, we need to return the specific field type highlight
  // colour or the general XFA field type colour if present.
  if (IsFormFieldTypeXFA(fieldType)) {
    if (!m_NeedsHighlight[static_cast<size_t>(fieldType)] &&
        m_NeedsHighlight[static_cast<size_t>(FormFieldType::kXFA)]) {
      return m_HighlightColor[static_cast<size_t>(FormFieldType::kXFA)];
    }
  }
#endif  // PDF_ENABLE_XFA
  return m_HighlightColor[static_cast<size_t>(fieldType)];
}
