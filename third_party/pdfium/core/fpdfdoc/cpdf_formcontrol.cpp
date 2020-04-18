// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_formcontrol.h"

#include <algorithm>

#include "core/fpdfapi/page/cpdf_form.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfapi/render/cpdf_rendercontext.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fxge/cfx_renderdevice.h"

namespace {

const char* const g_sHighlightingMode[] = {
    // Must match order of HighlightingMode enum.
    "N", "I", "O", "P", "T"};

}  // namespace

CPDF_FormControl::CPDF_FormControl(CPDF_FormField* pField,
                                   CPDF_Dictionary* pWidgetDict)
    : m_pField(pField),
      m_pWidgetDict(pWidgetDict),
      m_pForm(m_pField->GetForm()) {}

CPDF_FormControl::~CPDF_FormControl() {}

ByteString CPDF_FormControl::GetOnStateName() const {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  ByteString csOn;
  CPDF_Dictionary* pAP = m_pWidgetDict->GetDictFor("AP");
  if (!pAP)
    return csOn;

  CPDF_Dictionary* pN = pAP->GetDictFor("N");
  if (!pN)
    return csOn;

  for (const auto& it : *pN) {
    if (it.first != "Off")
      return it.first;
  }
  return ByteString();
}

void CPDF_FormControl::SetOnStateName(const ByteString& csOn) {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  ByteString csValue = csOn;
  if (csValue.IsEmpty())
    csValue = "Yes";
  else if (csValue == "Off")
    csValue = "Yes";

  ByteString csAS = m_pWidgetDict->GetStringFor("AS", "Off");
  if (csAS != "Off")
    m_pWidgetDict->SetNewFor<CPDF_Name>("AS", csValue);

  CPDF_Dictionary* pAP = m_pWidgetDict->GetDictFor("AP");
  if (!pAP)
    return;

  for (const auto& it : *pAP) {
    CPDF_Object* pObj1 = it.second.get();
    if (!pObj1)
      continue;

    CPDF_Object* pObjDirect1 = pObj1->GetDirect();
    CPDF_Dictionary* pSubDict = pObjDirect1->AsDictionary();
    if (!pSubDict)
      continue;

    auto subdict_it = pSubDict->begin();
    while (subdict_it != pSubDict->end()) {
      const ByteString& csKey2 = subdict_it->first;
      CPDF_Object* pObj2 = subdict_it->second.get();
      ++subdict_it;
      if (!pObj2)
        continue;
      if (csKey2 != "Off") {
        pSubDict->ReplaceKey(csKey2, csValue);
        break;
      }
    }
  }
}

ByteString CPDF_FormControl::GetCheckedAPState() {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  ByteString csOn = GetOnStateName();
  if (GetType() == CPDF_FormField::RadioButton ||
      GetType() == CPDF_FormField::CheckBox) {
    if (ToArray(FPDF_GetFieldAttr(m_pField->GetDict(), "Opt")))
      csOn = ByteString::Format("%d", m_pField->GetControlIndex(this));
  }
  if (csOn.IsEmpty())
    csOn = "Yes";
  return csOn;
}

WideString CPDF_FormControl::GetExportValue() const {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  ByteString csOn = GetOnStateName();
  if (GetType() == CPDF_FormField::RadioButton ||
      GetType() == CPDF_FormField::CheckBox) {
    if (CPDF_Array* pArray =
            ToArray(FPDF_GetFieldAttr(m_pField->GetDict(), "Opt"))) {
      int iIndex = m_pField->GetControlIndex(this);
      csOn = pArray->GetStringAt(iIndex);
    }
  }
  if (csOn.IsEmpty())
    csOn = "Yes";
  return PDF_DecodeText(csOn);
}

bool CPDF_FormControl::IsChecked() const {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  ByteString csOn = GetOnStateName();
  ByteString csAS = m_pWidgetDict->GetStringFor("AS");
  return csAS == csOn;
}

bool CPDF_FormControl::IsDefaultChecked() const {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  CPDF_Object* pDV = FPDF_GetFieldAttr(m_pField->GetDict(), "DV");
  if (!pDV)
    return false;

  ByteString csDV = pDV->GetString();
  ByteString csOn = GetOnStateName();
  return (csDV == csOn);
}

void CPDF_FormControl::CheckControl(bool bChecked) {
  ASSERT(GetType() == CPDF_FormField::CheckBox ||
         GetType() == CPDF_FormField::RadioButton);
  ByteString csOn = GetOnStateName();
  ByteString csOldAS = m_pWidgetDict->GetStringFor("AS", "Off");
  ByteString csAS = "Off";
  if (bChecked)
    csAS = csOn;
  if (csOldAS == csAS)
    return;
  m_pWidgetDict->SetNewFor<CPDF_Name>("AS", csAS);
}

void CPDF_FormControl::DrawControl(CFX_RenderDevice* pDevice,
                                   CFX_Matrix* pMatrix,
                                   CPDF_Page* pPage,
                                   CPDF_Annot::AppearanceMode mode,
                                   const CPDF_RenderOptions* pOptions) {
  if (m_pWidgetDict->GetIntegerFor("F") & ANNOTFLAG_HIDDEN)
    return;

  CPDF_Stream* pStream = FPDFDOC_GetAnnotAP(m_pWidgetDict.Get(), mode);
  if (!pStream)
    return;

  CFX_Matrix form_matrix = pStream->GetDict()->GetMatrixFor("Matrix");
  CFX_FloatRect form_bbox =
      form_matrix.TransformRect(pStream->GetDict()->GetRectFor("BBox"));
  CFX_FloatRect arect = m_pWidgetDict->GetRectFor("Rect");
  CFX_Matrix matrix;
  matrix.MatchRect(arect, form_bbox);
  matrix.Concat(*pMatrix);
  CPDF_Form form(m_pField->GetForm()->GetDocument(),
                 m_pField->GetForm()->GetFormDict()->GetDictFor("DR"), pStream);
  form.ParseContent(nullptr, nullptr, nullptr, nullptr);
  CPDF_RenderContext context(pPage);
  context.AppendLayer(&form, &matrix);
  context.Render(pDevice, pOptions, nullptr);
}

CPDF_FormControl::HighlightingMode CPDF_FormControl::GetHighlightingMode() {
  if (!m_pWidgetDict)
    return Invert;

  ByteString csH = m_pWidgetDict->GetStringFor("H", "I");
  for (size_t i = 0; i < FX_ArraySize(g_sHighlightingMode); ++i) {
    if (csH == g_sHighlightingMode[i])
      return static_cast<HighlightingMode>(i);
  }
  return Invert;
}

CPDF_ApSettings CPDF_FormControl::GetMK() const {
  return CPDF_ApSettings(m_pWidgetDict ? m_pWidgetDict->GetDictFor("MK")
                                       : nullptr);
}

bool CPDF_FormControl::HasMKEntry(const ByteString& csEntry) const {
  return GetMK().HasMKEntry(csEntry);
}

int CPDF_FormControl::GetRotation() {
  return GetMK().GetRotation();
}

FX_ARGB CPDF_FormControl::GetColor(int& iColorType, const ByteString& csEntry) {
  return GetMK().GetColor(iColorType, csEntry);
}

float CPDF_FormControl::GetOriginalColor(int index, const ByteString& csEntry) {
  return GetMK().GetOriginalColor(index, csEntry);
}

void CPDF_FormControl::GetOriginalColor(int& iColorType,
                                        float fc[4],
                                        const ByteString& csEntry) {
  GetMK().GetOriginalColor(iColorType, fc, csEntry);
}

WideString CPDF_FormControl::GetCaption(const ByteString& csEntry) {
  return GetMK().GetCaption(csEntry);
}

CPDF_Stream* CPDF_FormControl::GetIcon(const ByteString& csEntry) {
  return GetMK().GetIcon(csEntry);
}

CPDF_IconFit CPDF_FormControl::GetIconFit() {
  return GetMK().GetIconFit();
}

int CPDF_FormControl::GetTextPosition() {
  return GetMK().GetTextPosition();
}

CPDF_Action CPDF_FormControl::GetAction() {
  if (!m_pWidgetDict)
    return CPDF_Action(nullptr);

  if (m_pWidgetDict->KeyExist("A"))
    return CPDF_Action(m_pWidgetDict->GetDictFor("A"));

  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pField->GetDict(), "A");
  return CPDF_Action(pObj ? pObj->GetDict() : nullptr);
}

CPDF_AAction CPDF_FormControl::GetAdditionalAction() {
  if (!m_pWidgetDict)
    return CPDF_AAction(nullptr);

  if (m_pWidgetDict->KeyExist("AA"))
    return CPDF_AAction(m_pWidgetDict->GetDictFor("AA"));
  return m_pField->GetAdditionalAction();
}

CPDF_DefaultAppearance CPDF_FormControl::GetDefaultAppearance() {
  if (!m_pWidgetDict)
    return CPDF_DefaultAppearance();

  if (m_pWidgetDict->KeyExist("DA"))
    return CPDF_DefaultAppearance(m_pWidgetDict->GetStringFor("DA"));

  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pField->GetDict(), "DA");
  if (pObj)
    return CPDF_DefaultAppearance(pObj->GetString());
  return m_pField->GetForm()->GetDefaultAppearance();
}

CPDF_Font* CPDF_FormControl::GetDefaultControlFont() {
  float fFontSize;
  CPDF_DefaultAppearance cDA = GetDefaultAppearance();
  Optional<ByteString> csFontNameTag = cDA.GetFont(&fFontSize);
  if (!csFontNameTag || csFontNameTag->IsEmpty())
    return nullptr;

  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pWidgetDict.Get(), "DR");
  if (CPDF_Dictionary* pDict = ToDictionary(pObj)) {
    CPDF_Dictionary* pFonts = pDict->GetDictFor("Font");
    if (pFonts) {
      CPDF_Dictionary* pElement = pFonts->GetDictFor(*csFontNameTag);
      if (pElement) {
        CPDF_Font* pFont =
            m_pField->GetForm()->GetDocument()->LoadFont(pElement);
        if (pFont)
          return pFont;
      }
    }
  }
  if (CPDF_Font* pFormFont = m_pField->GetForm()->GetFormFont(*csFontNameTag))
    return pFormFont;

  CPDF_Dictionary* pPageDict = m_pWidgetDict->GetDictFor("P");
  pObj = FPDF_GetFieldAttr(pPageDict, "Resources");
  if (CPDF_Dictionary* pDict = ToDictionary(pObj)) {
    CPDF_Dictionary* pFonts = pDict->GetDictFor("Font");
    if (pFonts) {
      CPDF_Dictionary* pElement = pFonts->GetDictFor(*csFontNameTag);
      if (pElement) {
        CPDF_Font* pFont =
            m_pField->GetForm()->GetDocument()->LoadFont(pElement);
        if (pFont)
          return pFont;
      }
    }
  }
  return nullptr;
}

int CPDF_FormControl::GetControlAlignment() {
  if (!m_pWidgetDict)
    return 0;
  if (m_pWidgetDict->KeyExist("Q"))
    return m_pWidgetDict->GetIntegerFor("Q", 0);

  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pField->GetDict(), "Q");
  if (pObj)
    return pObj->GetInteger();
  return m_pField->GetForm()->GetFormAlignment();
}
