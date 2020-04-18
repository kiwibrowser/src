// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_widget.h"

#include <memory>
#include <sstream>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfdoc/cpdf_defaultappearance.h"
#include "core/fpdfdoc/cpdf_formcontrol.h"
#include "core/fpdfdoc/cpdf_formfield.h"
#include "core/fpdfdoc/cpdf_iconfit.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_pathdata.h"
#include "core/fxge/cfx_renderdevice.h"
#include "fpdfsdk/cpdfsdk_actionhandler.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/cpdfsdk_interform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/formfiller/cba_fontmap.h"
#include "fpdfsdk/pwl/cpwl_appstream.h"
#include "fpdfsdk/pwl/cpwl_edit.h"

#ifdef PDF_ENABLE_XFA
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffdocview.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "xfa/fxfa/cxfa_ffwidgethandler.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#endif  // PDF_ENABLE_XFA

CPDFSDK_Widget::CPDFSDK_Widget(CPDF_Annot* pAnnot,
                               CPDFSDK_PageView* pPageView,
                               CPDFSDK_InterForm* pInterForm)
    : CPDFSDK_BAAnnot(pAnnot, pPageView),
      m_pInterForm(pInterForm),
      m_nAppearanceAge(0),
      m_nValueAge(0)
#ifdef PDF_ENABLE_XFA
      ,
      m_hMixXFAWidget(nullptr),
      m_pWidgetHandler(nullptr)
#endif  // PDF_ENABLE_XFA
{
}

CPDFSDK_Widget::~CPDFSDK_Widget() {}

#ifdef PDF_ENABLE_XFA
CXFA_FFWidget* CPDFSDK_Widget::GetMixXFAWidget() const {
  CPDFXFA_Context* pContext = m_pPageView->GetFormFillEnv()->GetXFAContext();
  if (pContext->GetFormType() == FormType::kXFAForeground) {
    if (!m_hMixXFAWidget) {
      if (CXFA_FFDocView* pDocView = pContext->GetXFADocView()) {
        WideString sName;
        if (GetFieldType() == FormFieldType::kRadioButton) {
          sName = GetAnnotName();
          if (sName.IsEmpty())
            sName = GetName();
        } else {
          sName = GetName();
        }

        if (!sName.IsEmpty())
          m_hMixXFAWidget = pDocView->GetWidgetByName(sName, nullptr);
      }
    }
    return m_hMixXFAWidget.Get();
  }
  return nullptr;
}

CXFA_FFWidget* CPDFSDK_Widget::GetGroupMixXFAWidget() {
  CPDFXFA_Context* pContext = m_pPageView->GetFormFillEnv()->GetXFAContext();
  if (pContext->GetFormType() != FormType::kXFAForeground)
    return nullptr;

  CXFA_FFDocView* pDocView = pContext->GetXFADocView();
  if (!pDocView)
    return nullptr;

  WideString sName = GetName();
  return !sName.IsEmpty() ? pDocView->GetWidgetByName(sName, nullptr) : nullptr;
}

CXFA_FFWidgetHandler* CPDFSDK_Widget::GetXFAWidgetHandler() const {
  CPDFXFA_Context* pContext = m_pPageView->GetFormFillEnv()->GetXFAContext();
  if (pContext->GetFormType() != FormType::kXFAForeground)
    return nullptr;

  if (!m_pWidgetHandler) {
    CXFA_FFDocView* pDocView = pContext->GetXFADocView();
    if (pDocView)
      m_pWidgetHandler = pDocView->GetWidgetHandler();
  }
  return m_pWidgetHandler.Get();
}

static XFA_EVENTTYPE GetXFAEventType(PDFSDK_XFAAActionType eXFAAAT) {
  XFA_EVENTTYPE eEventType = XFA_EVENT_Unknown;

  switch (eXFAAAT) {
    case PDFSDK_XFA_Click:
      eEventType = XFA_EVENT_Click;
      break;
    case PDFSDK_XFA_Full:
      eEventType = XFA_EVENT_Full;
      break;
    case PDFSDK_XFA_PreOpen:
      eEventType = XFA_EVENT_PreOpen;
      break;
    case PDFSDK_XFA_PostOpen:
      eEventType = XFA_EVENT_PostOpen;
      break;
  }

  return eEventType;
}

static XFA_EVENTTYPE GetXFAEventType(CPDF_AAction::AActionType eAAT,
                                     bool bWillCommit) {
  XFA_EVENTTYPE eEventType = XFA_EVENT_Unknown;

  switch (eAAT) {
    case CPDF_AAction::CursorEnter:
      eEventType = XFA_EVENT_MouseEnter;
      break;
    case CPDF_AAction::CursorExit:
      eEventType = XFA_EVENT_MouseExit;
      break;
    case CPDF_AAction::ButtonDown:
      eEventType = XFA_EVENT_MouseDown;
      break;
    case CPDF_AAction::ButtonUp:
      eEventType = XFA_EVENT_MouseUp;
      break;
    case CPDF_AAction::GetFocus:
      eEventType = XFA_EVENT_Enter;
      break;
    case CPDF_AAction::LoseFocus:
      eEventType = XFA_EVENT_Exit;
      break;
    case CPDF_AAction::PageOpen:
    case CPDF_AAction::PageClose:
    case CPDF_AAction::PageVisible:
    case CPDF_AAction::PageInvisible:
      break;
    case CPDF_AAction::KeyStroke:
      if (!bWillCommit)
        eEventType = XFA_EVENT_Change;
      break;
    case CPDF_AAction::Validate:
      eEventType = XFA_EVENT_Validate;
      break;
    case CPDF_AAction::OpenPage:
    case CPDF_AAction::ClosePage:
    case CPDF_AAction::Format:
    case CPDF_AAction::Calculate:
    case CPDF_AAction::CloseDocument:
    case CPDF_AAction::SaveDocument:
    case CPDF_AAction::DocumentSaved:
    case CPDF_AAction::PrintDocument:
    case CPDF_AAction::DocumentPrinted:
      break;
    case CPDF_AAction::NumberOfActions:
      NOTREACHED();
      break;
  }

  return eEventType;
}

bool CPDFSDK_Widget::HasXFAAAction(PDFSDK_XFAAActionType eXFAAAT) {
  CXFA_FFWidget* hWidget = GetMixXFAWidget();
  if (!hWidget)
    return false;

  CXFA_FFWidgetHandler* pXFAWidgetHandler = GetXFAWidgetHandler();
  if (!pXFAWidgetHandler)
    return false;

  XFA_EVENTTYPE eEventType = GetXFAEventType(eXFAAAT);

  if ((eEventType == XFA_EVENT_Click || eEventType == XFA_EVENT_Change) &&
      GetFieldType() == FormFieldType::kRadioButton) {
    if (CXFA_FFWidget* hGroupWidget = GetGroupMixXFAWidget()) {
      CXFA_Node* node = hGroupWidget->GetNode();
      if (node->IsWidgetReady()) {
        if (pXFAWidgetHandler->HasEvent(node, eEventType))
          return true;
      }
    }
  }
  CXFA_Node* node = hWidget->GetNode();
  if (!node->IsWidgetReady())
    return false;
  return pXFAWidgetHandler->HasEvent(node, eEventType);
}

bool CPDFSDK_Widget::OnXFAAAction(PDFSDK_XFAAActionType eXFAAAT,
                                  CPDFSDK_FieldAction* data,
                                  CPDFSDK_PageView* pPageView) {
  CPDFXFA_Context* pContext = m_pPageView->GetFormFillEnv()->GetXFAContext();

  CXFA_FFWidget* hWidget = GetMixXFAWidget();
  if (!hWidget)
    return false;

  XFA_EVENTTYPE eEventType = GetXFAEventType(eXFAAAT);
  if (eEventType == XFA_EVENT_Unknown)
    return false;

  CXFA_FFWidgetHandler* pXFAWidgetHandler = GetXFAWidgetHandler();
  if (!pXFAWidgetHandler)
    return false;

  CXFA_EventParam param;
  param.m_eType = eEventType;
  param.m_wsChange = data->sChange;
  param.m_iCommitKey = 0;
  param.m_bShift = data->bShift;
  param.m_iSelStart = data->nSelStart;
  param.m_iSelEnd = data->nSelEnd;
  param.m_wsFullText = data->sValue;
  param.m_bKeyDown = data->bKeyDown;
  param.m_bModifier = data->bModifier;
  param.m_wsNewText = data->sValue;
  if (data->nSelEnd > data->nSelStart)
    param.m_wsNewText.Delete(data->nSelStart, data->nSelEnd - data->nSelStart);

  for (const auto& c : data->sChange)
    param.m_wsNewText.Insert(data->nSelStart, c);

  param.m_wsPrevText = data->sValue;
  if ((eEventType == XFA_EVENT_Click || eEventType == XFA_EVENT_Change) &&
      GetFieldType() == FormFieldType::kRadioButton) {
    if (CXFA_FFWidget* hGroupWidget = GetGroupMixXFAWidget()) {
      CXFA_Node* node = hGroupWidget->GetNode();
      if (!node->IsWidgetReady())
        return false;

      param.m_pTarget = node;
      if (pXFAWidgetHandler->ProcessEvent(node, &param) !=
          XFA_EVENTERROR_Success) {
        return false;
      }
    }
  }

  int32_t nRet = XFA_EVENTERROR_NotExist;
  CXFA_Node* node = hWidget->GetNode();
  if (node->IsWidgetReady()) {
    param.m_pTarget = node;
    nRet = pXFAWidgetHandler->ProcessEvent(node, &param);
  }
  if (CXFA_FFDocView* pDocView = pContext->GetXFADocView())
    pDocView->UpdateDocView();

  return nRet == XFA_EVENTERROR_Success;
}

void CPDFSDK_Widget::Synchronize(bool bSynchronizeElse) {
  CXFA_FFWidget* hWidget = GetMixXFAWidget();
  if (!hWidget)
    return;

  CXFA_Node* node = hWidget->GetNode();
  if (!node->IsWidgetReady())
    return;

  CPDF_FormField* pFormField = GetFormField();
  switch (GetFieldType()) {
    case FormFieldType::kCheckBox:
    case FormFieldType::kRadioButton: {
      CPDF_FormControl* pFormCtrl = GetFormControl();
      XFA_CHECKSTATE eCheckState =
          pFormCtrl->IsChecked() ? XFA_CHECKSTATE_On : XFA_CHECKSTATE_Off;
      node->SetCheckState(eCheckState, true);
      break;
    }
    case FormFieldType::kTextField:
      node->SetValue(XFA_VALUEPICTURE_Edit, pFormField->GetValue());
      break;
    case FormFieldType::kComboBox:
    case FormFieldType::kListBox: {
      node->ClearAllSelections();

      for (int i = 0; i < pFormField->CountSelectedItems(); ++i) {
        int nIndex = pFormField->GetSelectedIndex(i);
        if (nIndex > -1 && nIndex < node->CountChoiceListItems(false))
          node->SetItemState(nIndex, true, false, false, true);
      }
      if (GetFieldType() == FormFieldType::kComboBox)
        node->SetValue(XFA_VALUEPICTURE_Edit, pFormField->GetValue());
      break;
    }
    default:
      break;
  }

  if (bSynchronizeElse) {
    CPDFXFA_Context* context = m_pPageView->GetFormFillEnv()->GetXFAContext();
    context->GetXFADocView()->ProcessValueChanged(node);
  }
}
#endif  // PDF_ENABLE_XFA

bool CPDFSDK_Widget::IsWidgetAppearanceValid(CPDF_Annot::AppearanceMode mode) {
  CPDF_Dictionary* pAP = m_pAnnot->GetAnnotDict()->GetDictFor("AP");
  if (!pAP)
    return false;

  // Choose the right sub-ap
  const char* ap_entry = "N";
  if (mode == CPDF_Annot::Down)
    ap_entry = "D";
  else if (mode == CPDF_Annot::Rollover)
    ap_entry = "R";
  if (!pAP->KeyExist(ap_entry))
    ap_entry = "N";

  // Get the AP stream or subdirectory
  CPDF_Object* psub = pAP->GetDirectObjectFor(ap_entry);
  if (!psub)
    return false;

  FormFieldType fieldType = GetFieldType();
  switch (fieldType) {
    case FormFieldType::kPushButton:
    case FormFieldType::kComboBox:
    case FormFieldType::kListBox:
    case FormFieldType::kTextField:
    case FormFieldType::kSignature:
      return psub->IsStream();
    case FormFieldType::kCheckBox:
    case FormFieldType::kRadioButton:
      if (CPDF_Dictionary* pSubDict = psub->AsDictionary()) {
        return !!pSubDict->GetStreamFor(GetAppState());
      }
      return false;
    default:
      return true;
  }
}

FormFieldType CPDFSDK_Widget::GetFieldType() const {
  CPDF_FormField* pField = GetFormField();
  return pField ? pField->GetFieldType() : FormFieldType::kUnknown;
}

bool CPDFSDK_Widget::IsAppearanceValid() {
#ifdef PDF_ENABLE_XFA
  CPDFXFA_Context* pContext = m_pPageView->GetFormFillEnv()->GetXFAContext();
  FormType formType = pContext->GetFormType();
  if (formType == FormType::kXFAFull)
    return true;
#endif  // PDF_ENABLE_XFA
  return CPDFSDK_BAAnnot::IsAppearanceValid();
}

int CPDFSDK_Widget::GetLayoutOrder() const {
  return 2;
}

int CPDFSDK_Widget::GetFieldFlags() const {
  CPDF_InterForm* pPDFInterForm = m_pInterForm->GetInterForm();
  CPDF_FormControl* pFormControl =
      pPDFInterForm->GetControlByDict(m_pAnnot->GetAnnotDict());
  CPDF_FormField* pFormField = pFormControl->GetField();
  return pFormField->GetFieldFlags();
}

bool CPDFSDK_Widget::IsSignatureWidget() const {
  return GetFieldType() == FormFieldType::kSignature;
}

CPDF_FormField* CPDFSDK_Widget::GetFormField() const {
  CPDF_FormControl* pControl = GetFormControl();
  return pControl ? pControl->GetField() : nullptr;
}

CPDF_FormControl* CPDFSDK_Widget::GetFormControl() const {
  CPDF_InterForm* pPDFInterForm = m_pInterForm->GetInterForm();
  return pPDFInterForm->GetControlByDict(GetAnnotDict());
}

CPDF_FormControl* CPDFSDK_Widget::GetFormControl(
    CPDF_InterForm* pInterForm,
    const CPDF_Dictionary* pAnnotDict) {
  ASSERT(pAnnotDict);
  return pInterForm->GetControlByDict(pAnnotDict);
}

int CPDFSDK_Widget::GetRotate() const {
  CPDF_FormControl* pCtrl = GetFormControl();
  return pCtrl->GetRotation() % 360;
}

#ifdef PDF_ENABLE_XFA
WideString CPDFSDK_Widget::GetName() const {
  return GetFormField()->GetFullName();
}
#endif  // PDF_ENABLE_XFA

bool CPDFSDK_Widget::GetFillColor(FX_COLORREF& color) const {
  CPDF_FormControl* pFormCtrl = GetFormControl();
  int iColorType = 0;
  color = ArgbToColorRef(pFormCtrl->GetBackgroundColor(iColorType));
  return iColorType != CFX_Color::kTransparent;
}

bool CPDFSDK_Widget::GetBorderColor(FX_COLORREF& color) const {
  CPDF_FormControl* pFormCtrl = GetFormControl();
  int iColorType = 0;
  color = ArgbToColorRef(pFormCtrl->GetBorderColor(iColorType));
  return iColorType != CFX_Color::kTransparent;
}

bool CPDFSDK_Widget::GetTextColor(FX_COLORREF& color) const {
  CPDF_FormControl* pFormCtrl = GetFormControl();
  CPDF_DefaultAppearance da = pFormCtrl->GetDefaultAppearance();
  FX_ARGB argb;
  Optional<CFX_Color::Type> iColorType;
  std::tie(iColorType, argb) = da.GetColor();
  if (!iColorType)
    return false;

  color = ArgbToColorRef(argb);
  return *iColorType != CFX_Color::kTransparent;
}

float CPDFSDK_Widget::GetFontSize() const {
  CPDF_FormControl* pFormCtrl = GetFormControl();
  CPDF_DefaultAppearance pDa = pFormCtrl->GetDefaultAppearance();
  float fFontSize;
  pDa.GetFont(&fFontSize);
  return fFontSize;
}

int CPDFSDK_Widget::GetSelectedIndex(int nIndex) const {
#ifdef PDF_ENABLE_XFA
  if (CXFA_FFWidget* hWidget = GetMixXFAWidget()) {
    CXFA_Node* node = hWidget->GetNode();
    if (node->IsWidgetReady()) {
      if (nIndex < node->CountSelectedItems())
        return node->GetSelectedItem(nIndex);
    }
  }
#endif  // PDF_ENABLE_XFA
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetSelectedIndex(nIndex);
}

#ifdef PDF_ENABLE_XFA
WideString CPDFSDK_Widget::GetValue(bool bDisplay) const {
  if (CXFA_FFWidget* hWidget = GetMixXFAWidget()) {
    CXFA_Node* node = hWidget->GetNode();
    if (node->IsWidgetReady()) {
      return node->GetValue(bDisplay ? XFA_VALUEPICTURE_Display
                                     : XFA_VALUEPICTURE_Edit);
    }
  }
#else
WideString CPDFSDK_Widget::GetValue() const {
#endif  // PDF_ENABLE_XFA
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetValue();
}

WideString CPDFSDK_Widget::GetDefaultValue() const {
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetDefaultValue();
}

WideString CPDFSDK_Widget::GetOptionLabel(int nIndex) const {
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetOptionLabel(nIndex);
}

int CPDFSDK_Widget::CountOptions() const {
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->CountOptions();
}

bool CPDFSDK_Widget::IsOptionSelected(int nIndex) const {
#ifdef PDF_ENABLE_XFA
  if (CXFA_FFWidget* hWidget = GetMixXFAWidget()) {
    CXFA_Node* node = hWidget->GetNode();
    if (node->IsWidgetReady()) {
      if (nIndex > -1 && nIndex < node->CountChoiceListItems(false))
        return node->GetItemState(nIndex);

      return false;
    }
  }
#endif  // PDF_ENABLE_XFA
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->IsItemSelected(nIndex);
}

int CPDFSDK_Widget::GetTopVisibleIndex() const {
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetTopVisibleIndex();
}

bool CPDFSDK_Widget::IsChecked() const {
#ifdef PDF_ENABLE_XFA
  if (CXFA_FFWidget* hWidget = GetMixXFAWidget()) {
    CXFA_Node* node = hWidget->GetNode();
    if (node->IsWidgetReady())
      return node->GetCheckState() == XFA_CHECKSTATE_On;
  }
#endif  // PDF_ENABLE_XFA
  CPDF_FormControl* pFormCtrl = GetFormControl();
  return pFormCtrl->IsChecked();
}

int CPDFSDK_Widget::GetAlignment() const {
  CPDF_FormControl* pFormCtrl = GetFormControl();
  return pFormCtrl->GetControlAlignment();
}

int CPDFSDK_Widget::GetMaxLen() const {
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetMaxLen();
}

void CPDFSDK_Widget::SetCheck(bool bChecked, bool bNotify) {
  CPDF_FormControl* pFormCtrl = GetFormControl();
  CPDF_FormField* pFormField = pFormCtrl->GetField();
  pFormField->CheckControl(pFormField->GetControlIndex(pFormCtrl), bChecked,
                           bNotify);
#ifdef PDF_ENABLE_XFA
  if (!IsWidgetAppearanceValid(CPDF_Annot::Normal))
    ResetAppearance(true);
  if (!bNotify)
    Synchronize(true);
#endif  // PDF_ENABLE_XFA
}

void CPDFSDK_Widget::SetValue(const WideString& sValue, bool bNotify) {
  CPDF_FormField* pFormField = GetFormField();
  pFormField->SetValue(sValue, bNotify);
#ifdef PDF_ENABLE_XFA
  if (!bNotify)
    Synchronize(true);
#endif  // PDF_ENABLE_XFA
}

void CPDFSDK_Widget::SetOptionSelection(int index,
                                        bool bSelected,
                                        bool bNotify) {
  CPDF_FormField* pFormField = GetFormField();
  pFormField->SetItemSelection(index, bSelected, bNotify);
#ifdef PDF_ENABLE_XFA
  if (!bNotify)
    Synchronize(true);
#endif  // PDF_ENABLE_XFA
}

void CPDFSDK_Widget::ClearSelection(bool bNotify) {
  CPDF_FormField* pFormField = GetFormField();
  pFormField->ClearSelection(bNotify);
#ifdef PDF_ENABLE_XFA
  if (!bNotify)
    Synchronize(true);
#endif  // PDF_ENABLE_XFA
}

void CPDFSDK_Widget::SetTopVisibleIndex(int index) {}

void CPDFSDK_Widget::SetAppModified() {
  m_bAppModified = true;
}

void CPDFSDK_Widget::ClearAppModified() {
  m_bAppModified = false;
}

bool CPDFSDK_Widget::IsAppModified() const {
  return m_bAppModified;
}

#ifdef PDF_ENABLE_XFA
void CPDFSDK_Widget::ResetAppearance(bool bValueChanged) {
  switch (GetFieldType()) {
    case FormFieldType::kTextField:
    case FormFieldType::kComboBox: {
      bool bFormatted = false;
      WideString sValue = OnFormat(bFormatted);
      ResetAppearance(bFormatted ? &sValue : nullptr, true);
      break;
    }
    default:
      ResetAppearance(nullptr, false);
      break;
  }
}
#endif  // PDF_ENABLE_XFA

void CPDFSDK_Widget::ResetAppearance(const WideString* sValue,
                                     bool bValueChanged) {
  SetAppModified();

  m_nAppearanceAge++;
  if (bValueChanged)
    m_nValueAge++;

  CPWL_AppStream appStream(this, GetAPDict());
  switch (GetFieldType()) {
    case FormFieldType::kPushButton:
      appStream.SetAsPushButton();
      break;
    case FormFieldType::kCheckBox:
      appStream.SetAsCheckBox();
      break;
    case FormFieldType::kRadioButton:
      appStream.SetAsRadioButton();
      break;
    case FormFieldType::kComboBox:
      appStream.SetAsComboBox(sValue);
      break;
    case FormFieldType::kListBox:
      appStream.SetAsListBox();
      break;
    case FormFieldType::kTextField:
      appStream.SetAsTextField(sValue);
      break;
    default:
      break;
  }

  m_pAnnot->ClearCachedAP();
}

WideString CPDFSDK_Widget::OnFormat(bool& bFormatted) {
  CPDF_FormField* pFormField = GetFormField();
  ASSERT(pFormField);
  return m_pInterForm->OnFormat(pFormField, bFormatted);
}

void CPDFSDK_Widget::ResetFieldAppearance(bool bValueChanged) {
  CPDF_FormField* pFormField = GetFormField();
  ASSERT(pFormField);
  m_pInterForm->ResetFieldAppearance(pFormField, nullptr, bValueChanged);
}

void CPDFSDK_Widget::DrawAppearance(CFX_RenderDevice* pDevice,
                                    const CFX_Matrix& mtUser2Device,
                                    CPDF_Annot::AppearanceMode mode,
                                    const CPDF_RenderOptions* pOptions) {
  FormFieldType fieldType = GetFieldType();

  if ((fieldType == FormFieldType::kCheckBox ||
       fieldType == FormFieldType::kRadioButton) &&
      mode == CPDF_Annot::Normal &&
      !IsWidgetAppearanceValid(CPDF_Annot::Normal)) {
    CFX_GraphStateData gsd;
    gsd.m_LineWidth = 0.0f;

    CFX_PathData pathData;
    pathData.AppendRect(GetRect());
    pDevice->DrawPath(&pathData, &mtUser2Device, &gsd, 0, 0xFFAAAAAA,
                      FXFILL_ALTERNATE);
  } else {
    CPDFSDK_BAAnnot::DrawAppearance(pDevice, mtUser2Device, mode, pOptions);
  }
}

void CPDFSDK_Widget::UpdateField() {
  CPDF_FormField* pFormField = GetFormField();
  ASSERT(pFormField);
  m_pInterForm->UpdateField(pFormField);
}

void CPDFSDK_Widget::DrawShadow(CFX_RenderDevice* pDevice,
                                CPDFSDK_PageView* pPageView) {
  FormFieldType fieldType = GetFieldType();
  if (!m_pInterForm->IsNeedHighLight(fieldType))
    return;

  CFX_Matrix page2device;
  pPageView->GetCurrentMatrix(page2device);

  CFX_FloatRect rcDevice = GetRect();
  CFX_PointF tmp =
      page2device.Transform(CFX_PointF(rcDevice.left, rcDevice.bottom));
  rcDevice.left = tmp.x;
  rcDevice.bottom = tmp.y;

  tmp = page2device.Transform(CFX_PointF(rcDevice.right, rcDevice.top));
  rcDevice.right = tmp.x;
  rcDevice.top = tmp.y;
  rcDevice.Normalize();

  pDevice->FillRect(rcDevice.ToFxRect(),
                    AlphaAndColorRefToArgb(
                        static_cast<int>(m_pInterForm->GetHighlightAlpha()),
                        m_pInterForm->GetHighlightColor(fieldType)));
}

CFX_FloatRect CPDFSDK_Widget::GetClientRect() const {
  CFX_FloatRect rcWindow = GetRotatedRect();
  float fBorderWidth = (float)GetBorderWidth();
  switch (GetBorderStyle()) {
    case BorderStyle::BEVELED:
    case BorderStyle::INSET:
      fBorderWidth *= 2.0f;
      break;
    default:
      break;
  }
  return rcWindow.GetDeflated(fBorderWidth, fBorderWidth);
}

CFX_FloatRect CPDFSDK_Widget::GetRotatedRect() const {
  CFX_FloatRect rectAnnot = GetRect();
  float fWidth = rectAnnot.Width();
  float fHeight = rectAnnot.Height();

  CPDF_FormControl* pControl = GetFormControl();
  CFX_FloatRect rcPDFWindow;
  switch (abs(pControl->GetRotation() % 360)) {
    case 0:
    case 180:
    default:
      rcPDFWindow = CFX_FloatRect(0, 0, fWidth, fHeight);
      break;
    case 90:
    case 270:
      rcPDFWindow = CFX_FloatRect(0, 0, fHeight, fWidth);
      break;
  }

  return rcPDFWindow;
}

CFX_Matrix CPDFSDK_Widget::GetMatrix() const {
  CFX_Matrix mt;
  CPDF_FormControl* pControl = GetFormControl();
  CFX_FloatRect rcAnnot = GetRect();
  float fWidth = rcAnnot.Width();
  float fHeight = rcAnnot.Height();

  switch (abs(pControl->GetRotation() % 360)) {
    default:
    case 0:
      break;
    case 90:
      mt = CFX_Matrix(0, 1, -1, 0, fWidth, 0);
      break;
    case 180:
      mt = CFX_Matrix(-1, 0, 0, -1, fWidth, fHeight);
      break;
    case 270:
      mt = CFX_Matrix(0, -1, 1, 0, 0, fHeight);
      break;
  }

  return mt;
}

CFX_Color CPDFSDK_Widget::GetTextPWLColor() const {
  CFX_Color crText = CFX_Color(CFX_Color::kGray, 0);

  CPDF_FormControl* pFormCtrl = GetFormControl();
  CPDF_DefaultAppearance da = pFormCtrl->GetDefaultAppearance();

  float fc[4];
  Optional<CFX_Color::Type> iColorType = da.GetColor(fc);
  if (iColorType)
    crText = CFX_Color(*iColorType, fc[0], fc[1], fc[2], fc[3]);

  return crText;
}

CFX_Color CPDFSDK_Widget::GetBorderPWLColor() const {
  CFX_Color crBorder;

  CPDF_FormControl* pFormCtrl = GetFormControl();
  int32_t iColorType;
  float fc[4];
  pFormCtrl->GetOriginalBorderColor(iColorType, fc);
  if (iColorType > 0)
    crBorder = CFX_Color(iColorType, fc[0], fc[1], fc[2], fc[3]);

  return crBorder;
}

CFX_Color CPDFSDK_Widget::GetFillPWLColor() const {
  CFX_Color crFill;

  CPDF_FormControl* pFormCtrl = GetFormControl();
  int32_t iColorType;
  float fc[4];
  pFormCtrl->GetOriginalBackgroundColor(iColorType, fc);
  if (iColorType > 0)
    crFill = CFX_Color(iColorType, fc[0], fc[1], fc[2], fc[3]);

  return crFill;
}

bool CPDFSDK_Widget::OnAAction(CPDF_AAction::AActionType type,
                               CPDFSDK_FieldAction* data,
                               CPDFSDK_PageView* pPageView) {
  CPDFSDK_FormFillEnvironment* pFormFillEnv = pPageView->GetFormFillEnv();

#ifdef PDF_ENABLE_XFA
  CPDFXFA_Context* pContext = pFormFillEnv->GetXFAContext();
  if (CXFA_FFWidget* hWidget = GetMixXFAWidget()) {
    XFA_EVENTTYPE eEventType = GetXFAEventType(type, data->bWillCommit);

    if (eEventType != XFA_EVENT_Unknown) {
      if (CXFA_FFWidgetHandler* pXFAWidgetHandler = GetXFAWidgetHandler()) {
        CXFA_EventParam param;
        param.m_eType = eEventType;
        param.m_wsChange = data->sChange;
        param.m_iCommitKey = 0;
        param.m_bShift = data->bShift;
        param.m_iSelStart = data->nSelStart;
        param.m_iSelEnd = data->nSelEnd;
        param.m_wsFullText = data->sValue;
        param.m_bKeyDown = data->bKeyDown;
        param.m_bModifier = data->bModifier;
        param.m_wsNewText = data->sValue;
        if (data->nSelEnd > data->nSelStart)
          param.m_wsNewText.Delete(data->nSelStart,
                                   data->nSelEnd - data->nSelStart);
        for (int i = data->sChange.GetLength() - 1; i >= 0; i--)
          param.m_wsNewText.Insert(data->nSelStart, data->sChange[i]);
        param.m_wsPrevText = data->sValue;

        int32_t nRet = XFA_EVENTERROR_NotExist;
        CXFA_Node* node = hWidget->GetNode();
        if (node->IsWidgetReady()) {
          param.m_pTarget = node;
          nRet = pXFAWidgetHandler->ProcessEvent(node, &param);
        }

        if (CXFA_FFDocView* pDocView = pContext->GetXFADocView())
          pDocView->UpdateDocView();

        if (nRet == XFA_EVENTERROR_Success)
          return true;
      }
    }
  }
#endif  // PDF_ENABLE_XFA

  CPDF_Action action = GetAAction(type);
  if (action.GetDict() && action.GetType() != CPDF_Action::Unknown) {
    CPDFSDK_ActionHandler* pActionHandler = pFormFillEnv->GetActionHandler();
    return pActionHandler->DoAction_Field(action, type, pFormFillEnv,
                                          GetFormField(), data);
  }
  return false;
}

CPDF_Action CPDFSDK_Widget::GetAAction(CPDF_AAction::AActionType eAAT) {
  switch (eAAT) {
    case CPDF_AAction::CursorEnter:
    case CPDF_AAction::CursorExit:
    case CPDF_AAction::ButtonDown:
    case CPDF_AAction::ButtonUp:
    case CPDF_AAction::GetFocus:
    case CPDF_AAction::LoseFocus:
    case CPDF_AAction::PageOpen:
    case CPDF_AAction::PageClose:
    case CPDF_AAction::PageVisible:
    case CPDF_AAction::PageInvisible:
      return CPDFSDK_BAAnnot::GetAAction(eAAT);

    case CPDF_AAction::KeyStroke:
    case CPDF_AAction::Format:
    case CPDF_AAction::Validate:
    case CPDF_AAction::Calculate: {
      CPDF_FormField* pField = GetFormField();
      if (pField->GetAdditionalAction().GetDict())
        return pField->GetAdditionalAction().GetAction(eAAT);
      return CPDFSDK_BAAnnot::GetAAction(eAAT);
    }
    default:
      break;
  }

  return CPDF_Action(nullptr);
}

WideString CPDFSDK_Widget::GetAlternateName() const {
  CPDF_FormField* pFormField = GetFormField();
  return pFormField->GetAlternateName();
}
