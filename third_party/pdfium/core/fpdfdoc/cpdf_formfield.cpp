// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_formfield.h"

#include <memory>
#include <set>
#include <utility>

#include "core/fpdfapi/parser/cfdf_document.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfdoc/cpdf_defaultappearance.h"
#include "core/fpdfdoc/cpdf_formcontrol.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fpdfdoc/cpvt_generateap.h"
#include "third_party/base/stl_util.h"

namespace {

const int kFormListMultiSelect = 0x100;

const int kFormComboEdit = 0x100;

const int kFormRadioNoToggleOff = 0x100;
const int kFormRadioUnison = 0x200;

const int kFormTextMultiLine = 0x100;
const int kFormTextPassword = 0x200;
const int kFormTextNoScroll = 0x400;
const int kFormTextComb = 0x800;

bool IsUnison(CPDF_FormField* pField) {
  if (pField->GetType() == CPDF_FormField::CheckBox)
    return true;
  return (pField->GetFieldFlags() & 0x2000000) != 0;
}

}  // namespace

Optional<FormFieldType> IntToFormFieldType(int value) {
  if (value >= static_cast<int>(FormFieldType::kUnknown) &&
      value < static_cast<int>(kFormFieldTypeCount)) {
    return {static_cast<FormFieldType>(value)};
  }
  return {};
}

CPDF_Object* FPDF_GetFieldAttr(const CPDF_Dictionary* pFieldDict,
                               const char* name,
                               int nLevel) {
  static constexpr int kGetFieldMaxRecursion = 32;
  if (!pFieldDict || nLevel > kGetFieldMaxRecursion)
    return nullptr;

  CPDF_Object* pAttr = pFieldDict->GetDirectObjectFor(name);
  if (pAttr)
    return pAttr;

  CPDF_Dictionary* pParent = pFieldDict->GetDictFor("Parent");
  return pParent ? FPDF_GetFieldAttr(pParent, name, nLevel + 1) : nullptr;
}

WideString FPDF_GetFullName(CPDF_Dictionary* pFieldDict) {
  WideString full_name;
  std::set<CPDF_Dictionary*> visited;
  CPDF_Dictionary* pLevel = pFieldDict;
  while (pLevel) {
    visited.insert(pLevel);
    WideString short_name = pLevel->GetUnicodeTextFor("T");
    if (!short_name.IsEmpty()) {
      if (full_name.IsEmpty())
        full_name = short_name;
      else
        full_name = short_name + L"." + full_name;
    }
    pLevel = pLevel->GetDictFor("Parent");
    if (pdfium::ContainsKey(visited, pLevel))
      break;
  }
  return full_name;
}

CPDF_FormField::CPDF_FormField(CPDF_InterForm* pForm, CPDF_Dictionary* pDict)
    : m_Type(Unknown),
      m_pForm(pForm),
      m_pDict(pDict),
      m_FontSize(0),
      m_pFont(nullptr) {
  SyncFieldFlags();
}

CPDF_FormField::~CPDF_FormField() {}

void CPDF_FormField::SyncFieldFlags() {
  CPDF_Object* ft_attr = FPDF_GetFieldAttr(m_pDict.Get(), "FT");
  ByteString type_name = ft_attr ? ft_attr->GetString() : ByteString();
  CPDF_Object* ff_attr = FPDF_GetFieldAttr(m_pDict.Get(), "Ff");
  uint32_t flags = ff_attr ? ff_attr->GetInteger() : 0;
  m_Flags = 0;
  if (flags & FORMFLAG_READONLY)
    m_Flags |= FORMFLAG_READONLY;
  if (flags & FORMFLAG_REQUIRED)
    m_Flags |= FORMFLAG_REQUIRED;
  if (flags & FORMFLAG_NOEXPORT)
    m_Flags |= FORMFLAG_NOEXPORT;

  if (type_name == "Btn") {
    if (flags & 0x8000) {
      m_Type = RadioButton;
      if (flags & 0x4000)
        m_Flags |= kFormRadioNoToggleOff;
      if (flags & 0x2000000)
        m_Flags |= kFormRadioUnison;
    } else if (flags & 0x10000) {
      m_Type = PushButton;
    } else {
      m_Type = CheckBox;
    }
  } else if (type_name == "Tx") {
    if (flags & 0x100000) {
      m_Type = File;
    } else if (flags & 0x2000000) {
      m_Type = RichText;
    } else {
      m_Type = Text;
      if (flags & 0x1000)
        m_Flags |= kFormTextMultiLine;
      if (flags & 0x2000)
        m_Flags |= kFormTextPassword;
      if (flags & 0x800000)
        m_Flags |= kFormTextNoScroll;
      if (flags & 0x100000)
        m_Flags |= kFormTextComb;
    }
    LoadDA();
  } else if (type_name == "Ch") {
    if (flags & 0x20000) {
      m_Type = ComboBox;
      if (flags & 0x40000)
        m_Flags |= kFormComboEdit;
    } else {
      m_Type = ListBox;
      if (flags & 0x200000)
        m_Flags |= kFormListMultiSelect;
    }
    LoadDA();
  } else if (type_name == "Sig") {
    m_Type = Sign;
  }
}

WideString CPDF_FormField::GetFullName() const {
  return FPDF_GetFullName(m_pDict.Get());
}

bool CPDF_FormField::ResetField(bool bNotify) {
  switch (m_Type) {
    case CPDF_FormField::CheckBox:
    case CPDF_FormField::RadioButton: {
      int iCount = CountControls();
      if (iCount) {
        // TODO(weili): Check whether anything special needs to be done for
        // unison field. (When IsUnison(this) returns true/false.)
        for (int i = 0; i < iCount; i++)
          CheckControl(i, GetControl(i)->IsDefaultChecked(), false);
      }
      if (bNotify && m_pForm->GetFormNotify())
        m_pForm->GetFormNotify()->AfterCheckedStatusChange(this);
      break;
    }
    case CPDF_FormField::ComboBox:
    case CPDF_FormField::ListBox: {
      WideString csValue;
      ClearSelection();
      int iIndex = GetDefaultSelectedItem();
      if (iIndex >= 0)
        csValue = GetOptionLabel(iIndex);

      if (bNotify && !NotifyListOrComboBoxBeforeChange(csValue))
        return false;

      SetItemSelection(iIndex, true);
      if (bNotify)
        NotifyListOrComboBoxAfterChange();
      break;
    }
    case CPDF_FormField::Text:
    case CPDF_FormField::RichText:
    case CPDF_FormField::File:
    default: {
      CPDF_Object* pDV = FPDF_GetFieldAttr(m_pDict.Get(), "DV");
      WideString csDValue;
      if (pDV)
        csDValue = pDV->GetUnicodeText();

      CPDF_Object* pV = FPDF_GetFieldAttr(m_pDict.Get(), "V");
      WideString csValue;
      if (pV)
        csValue = pV->GetUnicodeText();

      CPDF_Object* pRV = FPDF_GetFieldAttr(m_pDict.Get(), "RV");
      if (!pRV && (csDValue == csValue))
        return false;

      if (bNotify && !NotifyBeforeValueChange(csDValue))
        return false;

      if (pDV) {
        std::unique_ptr<CPDF_Object> pClone = pDV->Clone();
        if (!pClone)
          return false;

        m_pDict->SetFor("V", std::move(pClone));
        if (pRV)
          m_pDict->SetFor("RV", pDV->Clone());
      } else {
        m_pDict->RemoveFor("V");
        m_pDict->RemoveFor("RV");
      }
      if (bNotify)
        NotifyAfterValueChange();
      break;
    }
  }
  return true;
}

int CPDF_FormField::GetControlIndex(const CPDF_FormControl* pControl) const {
  if (!pControl)
    return -1;

  auto it = std::find(m_ControlList.begin(), m_ControlList.end(), pControl);
  return it != m_ControlList.end() ? it - m_ControlList.begin() : -1;
}

FormFieldType CPDF_FormField::GetFieldType() const {
  switch (m_Type) {
    case PushButton:
      return FormFieldType::kPushButton;
    case CheckBox:
      return FormFieldType::kCheckBox;
    case RadioButton:
      return FormFieldType::kRadioButton;
    case ComboBox:
      return FormFieldType::kComboBox;
    case ListBox:
      return FormFieldType::kListBox;
    case Text:
    case RichText:
    case File:
      return FormFieldType::kTextField;
    case Sign:
      return FormFieldType::kSignature;
    default:
      return FormFieldType::kUnknown;
  }
}

CPDF_AAction CPDF_FormField::GetAdditionalAction() const {
  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "AA");
  return CPDF_AAction(pObj ? pObj->GetDict() : nullptr);
}

WideString CPDF_FormField::GetAlternateName() const {
  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "TU");
  return pObj ? pObj->GetUnicodeText() : L"";
}

WideString CPDF_FormField::GetMappingName() const {
  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "TM");
  return pObj ? pObj->GetUnicodeText() : L"";
}

uint32_t CPDF_FormField::GetFieldFlags() const {
  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "Ff");
  return pObj ? pObj->GetInteger() : 0;
}

ByteString CPDF_FormField::GetDefaultStyle() const {
  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "DS");
  return pObj ? pObj->GetString() : "";
}

WideString CPDF_FormField::GetValue(bool bDefault) const {
  if (GetType() == CheckBox || GetType() == RadioButton)
    return GetCheckValue(bDefault);

  CPDF_Object* pValue = FPDF_GetFieldAttr(m_pDict.Get(), bDefault ? "DV" : "V");
  if (!pValue) {
    if (!bDefault) {
      if (m_Type == RichText)
        pValue = FPDF_GetFieldAttr(m_pDict.Get(), "V");
      if (!pValue && m_Type != Text)
        pValue = FPDF_GetFieldAttr(m_pDict.Get(), "DV");
    }
    if (!pValue)
      return WideString();
  }

  switch (pValue->GetType()) {
    case CPDF_Object::STRING:
    case CPDF_Object::STREAM:
      return pValue->GetUnicodeText();
    case CPDF_Object::ARRAY:
      pValue = pValue->AsArray()->GetDirectObjectAt(0);
      if (pValue)
        return pValue->GetUnicodeText();
      break;
    default:
      break;
  }
  return WideString();
}

WideString CPDF_FormField::GetValue() const {
  return GetValue(false);
}

WideString CPDF_FormField::GetDefaultValue() const {
  return GetValue(true);
}

bool CPDF_FormField::SetValue(const WideString& value,
                              bool bDefault,
                              bool bNotify) {
  switch (m_Type) {
    case CheckBox:
    case RadioButton: {
      SetCheckValue(value, bDefault, bNotify);
      return true;
    }
    case File:
    case RichText:
    case Text:
    case ComboBox: {
      WideString csValue = value;
      if (bNotify && !NotifyBeforeValueChange(csValue))
        return false;

      ByteString key(bDefault ? "DV" : "V");
      int iIndex = FindOptionValue(csValue);
      if (iIndex < 0) {
        ByteString bsEncodeText = PDF_EncodeText(csValue);
        m_pDict->SetNewFor<CPDF_String>(key, bsEncodeText, false);
        if (m_Type == RichText && !bDefault)
          m_pDict->SetNewFor<CPDF_String>("RV", bsEncodeText, false);
        m_pDict->RemoveFor("I");
      } else {
        m_pDict->SetNewFor<CPDF_String>(key, PDF_EncodeText(csValue), false);
        if (!bDefault) {
          ClearSelection();
          SetItemSelection(iIndex, true);
        }
      }
      if (bNotify)
        NotifyAfterValueChange();
      break;
    }
    case ListBox: {
      int iIndex = FindOptionValue(value);
      if (iIndex < 0)
        return false;

      if (bDefault && iIndex == GetDefaultSelectedItem())
        return false;

      if (bNotify && !NotifyBeforeSelectionChange(value))
        return false;

      if (!bDefault) {
        ClearSelection();
        SetItemSelection(iIndex, true);
      }
      if (bNotify)
        NotifyAfterSelectionChange();
      break;
    }
    default:
      break;
  }
  return true;
}

bool CPDF_FormField::SetValue(const WideString& value, bool bNotify) {
  return SetValue(value, false, bNotify);
}

int CPDF_FormField::GetMaxLen() const {
  if (CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "MaxLen"))
    return pObj->GetInteger();

  for (auto& pControl : m_ControlList) {
    if (!pControl)
      continue;
    CPDF_Dictionary* pWidgetDict = pControl->GetWidget();
    if (pWidgetDict->KeyExist("MaxLen"))
      return pWidgetDict->GetIntegerFor("MaxLen");
  }
  return 0;
}

int CPDF_FormField::CountSelectedItems() const {
  CPDF_Object* pValue = FPDF_GetFieldAttr(m_pDict.Get(), "V");
  if (!pValue) {
    pValue = FPDF_GetFieldAttr(m_pDict.Get(), "I");
    if (!pValue)
      return 0;
  }

  if (pValue->IsString() || pValue->IsNumber())
    return pValue->GetString().IsEmpty() ? 0 : 1;
  if (CPDF_Array* pArray = pValue->AsArray())
    return pArray->GetCount();
  return 0;
}

int CPDF_FormField::GetSelectedIndex(int index) const {
  CPDF_Object* pValue = FPDF_GetFieldAttr(m_pDict.Get(), "V");
  if (!pValue) {
    pValue = FPDF_GetFieldAttr(m_pDict.Get(), "I");
    if (!pValue)
      return -1;
  }
  if (pValue->IsNumber())
    return pValue->GetInteger();

  WideString sel_value;
  if (pValue->IsString()) {
    if (index != 0)
      return -1;
    sel_value = pValue->GetUnicodeText();
  } else {
    CPDF_Array* pArray = pValue->AsArray();
    if (!pArray || index < 0)
      return -1;

    CPDF_Object* elementValue = pArray->GetDirectObjectAt(index);
    sel_value = elementValue ? elementValue->GetUnicodeText() : WideString();
  }
  if (index < CountSelectedOptions()) {
    int iOptIndex = GetSelectedOptionIndex(index);
    WideString csOpt = GetOptionValue(iOptIndex);
    if (csOpt == sel_value)
      return iOptIndex;
  }
  for (int i = 0; i < CountOptions(); i++) {
    if (sel_value == GetOptionValue(i))
      return i;
  }
  return -1;
}

bool CPDF_FormField::ClearSelection(bool bNotify) {
  if (bNotify && m_pForm->GetFormNotify()) {
    WideString csValue;
    int iIndex = GetSelectedIndex(0);
    if (iIndex >= 0)
      csValue = GetOptionLabel(iIndex);

    if (!NotifyListOrComboBoxBeforeChange(csValue))
      return false;
  }
  m_pDict->RemoveFor("V");
  m_pDict->RemoveFor("I");
  if (bNotify)
    NotifyListOrComboBoxAfterChange();
  return true;
}

bool CPDF_FormField::IsItemSelected(int index) const {
  ASSERT(GetType() == ComboBox || GetType() == ListBox);
  if (index < 0 || index >= CountOptions())
    return false;
  if (IsOptionSelected(index))
    return true;

  WideString opt_value = GetOptionValue(index);
  CPDF_Object* pValue = FPDF_GetFieldAttr(m_pDict.Get(), "V");
  if (!pValue) {
    pValue = FPDF_GetFieldAttr(m_pDict.Get(), "I");
    if (!pValue)
      return false;
  }

  if (pValue->IsString())
    return pValue->GetUnicodeText() == opt_value;

  if (pValue->IsNumber()) {
    if (pValue->GetString().IsEmpty())
      return false;
    return (pValue->GetInteger() == index);
  }

  CPDF_Array* pArray = pValue->AsArray();
  if (!pArray)
    return false;

  int iPos = -1;
  for (int j = 0; j < CountSelectedOptions(); j++) {
    if (GetSelectedOptionIndex(j) == index) {
      iPos = j;
      break;
    }
  }
  for (int i = 0; i < static_cast<int>(pArray->GetCount()); i++)
    if (pArray->GetDirectObjectAt(i)->GetUnicodeText() == opt_value &&
        i == iPos) {
      return true;
    }
  return false;
}

bool CPDF_FormField::SetItemSelection(int index, bool bSelected, bool bNotify) {
  ASSERT(GetType() == ComboBox || GetType() == ListBox);
  if (index < 0 || index >= CountOptions())
    return false;

  WideString opt_value = GetOptionValue(index);
  if (bNotify && !NotifyListOrComboBoxBeforeChange(opt_value))
    return false;

  if (bSelected) {
    if (GetType() == ListBox) {
      SelectOption(index, true);
      if (!(m_Flags & kFormListMultiSelect)) {
        m_pDict->SetNewFor<CPDF_String>("V", PDF_EncodeText(opt_value), false);
      } else {
        CPDF_Array* pArray = m_pDict->SetNewFor<CPDF_Array>("V");
        for (int i = 0; i < CountOptions(); i++) {
          if (i == index || IsItemSelected(i)) {
            opt_value = GetOptionValue(i);
            pArray->AddNew<CPDF_String>(PDF_EncodeText(opt_value), false);
          }
        }
      }
    } else {
      m_pDict->SetNewFor<CPDF_String>("V", PDF_EncodeText(opt_value), false);
      CPDF_Array* pI = m_pDict->SetNewFor<CPDF_Array>("I");
      pI->AddNew<CPDF_Number>(index);
    }
  } else {
    CPDF_Object* pValue = FPDF_GetFieldAttr(m_pDict.Get(), "V");
    if (pValue) {
      if (GetType() == ListBox) {
        SelectOption(index, false);
        if (pValue->IsString()) {
          if (pValue->GetUnicodeText() == opt_value)
            m_pDict->RemoveFor("V");
        } else if (pValue->IsArray()) {
          auto pArray = pdfium::MakeUnique<CPDF_Array>();
          for (int i = 0; i < CountOptions(); i++) {
            if (i != index && IsItemSelected(i)) {
              opt_value = GetOptionValue(i);
              pArray->AddNew<CPDF_String>(PDF_EncodeText(opt_value), false);
            }
          }
          if (pArray->GetCount() > 0)
            m_pDict->SetFor("V", std::move(pArray));
        }
      } else {
        m_pDict->RemoveFor("V");
        m_pDict->RemoveFor("I");
      }
    }
  }
  if (bNotify)
    NotifyListOrComboBoxAfterChange();
  return true;
}

bool CPDF_FormField::IsItemDefaultSelected(int index) const {
  ASSERT(GetType() == ComboBox || GetType() == ListBox);
  if (index < 0 || index >= CountOptions())
    return false;
  int iDVIndex = GetDefaultSelectedItem();
  return iDVIndex >= 0 && iDVIndex == index;
}

int CPDF_FormField::GetDefaultSelectedItem() const {
  ASSERT(GetType() == ComboBox || GetType() == ListBox);
  CPDF_Object* pValue = FPDF_GetFieldAttr(m_pDict.Get(), "DV");
  if (!pValue)
    return -1;
  WideString csDV = pValue->GetUnicodeText();
  if (csDV.IsEmpty())
    return -1;
  for (int i = 0; i < CountOptions(); i++) {
    if (csDV == GetOptionValue(i))
      return i;
  }
  return -1;
}

int CPDF_FormField::CountOptions() const {
  CPDF_Array* pArray = ToArray(FPDF_GetFieldAttr(m_pDict.Get(), "Opt"));
  return pArray ? pArray->GetCount() : 0;
}

WideString CPDF_FormField::GetOptionText(int index, int sub_index) const {
  CPDF_Array* pArray = ToArray(FPDF_GetFieldAttr(m_pDict.Get(), "Opt"));
  if (!pArray)
    return WideString();

  CPDF_Object* pOption = pArray->GetDirectObjectAt(index);
  if (!pOption)
    return WideString();
  if (CPDF_Array* pOptionArray = pOption->AsArray())
    pOption = pOptionArray->GetDirectObjectAt(sub_index);

  CPDF_String* pString = ToString(pOption);
  return pString ? pString->GetUnicodeText() : WideString();
}

WideString CPDF_FormField::GetOptionLabel(int index) const {
  return GetOptionText(index, 1);
}

WideString CPDF_FormField::GetOptionValue(int index) const {
  return GetOptionText(index, 0);
}

int CPDF_FormField::FindOption(WideString csOptLabel) const {
  for (int i = 0; i < CountOptions(); i++) {
    if (GetOptionValue(i) == csOptLabel)
      return i;
  }
  return -1;
}

int CPDF_FormField::FindOptionValue(const WideString& csOptValue) const {
  for (int i = 0; i < CountOptions(); i++) {
    if (GetOptionValue(i) == csOptValue)
      return i;
  }
  return -1;
}

#ifdef PDF_ENABLE_XFA
int CPDF_FormField::InsertOption(WideString csOptLabel,
                                 int index,
                                 bool bNotify) {
  if (csOptLabel.IsEmpty())
    return -1;

  if (bNotify && !NotifyListOrComboBoxBeforeChange(csOptLabel))
    return -1;

  ByteString csStr = PDF_EncodeText(csOptLabel);
  CPDF_Array* pOpt = ToArray(FPDF_GetFieldAttr(m_pDict.Get(), "Opt"));
  if (!pOpt)
    pOpt = m_pDict->SetNewFor<CPDF_Array>("Opt");

  int iCount = pdfium::base::checked_cast<int>(pOpt->GetCount());
  if (index >= iCount) {
    pOpt->AddNew<CPDF_String>(csStr, false);
    index = iCount;
  } else {
    pOpt->InsertNewAt<CPDF_String>(index, csStr, false);
  }

  if (bNotify)
    NotifyListOrComboBoxAfterChange();
  return index;
}

bool CPDF_FormField::ClearOptions(bool bNotify) {
  if (bNotify && m_pForm->GetFormNotify()) {
    WideString csValue;
    int iIndex = GetSelectedIndex(0);
    if (iIndex >= 0)
      csValue = GetOptionLabel(iIndex);
    if (!NotifyListOrComboBoxBeforeChange(csValue))
      return false;
  }

  m_pDict->RemoveFor("Opt");
  m_pDict->RemoveFor("V");
  m_pDict->RemoveFor("DV");
  m_pDict->RemoveFor("I");
  m_pDict->RemoveFor("TI");

  if (bNotify)
    NotifyListOrComboBoxAfterChange();

  return true;
}
#endif  // PDF_ENABLE_XFA

bool CPDF_FormField::CheckControl(int iControlIndex,
                                  bool bChecked,
                                  bool bNotify) {
  ASSERT(GetType() == CheckBox || GetType() == RadioButton);
  CPDF_FormControl* pControl = GetControl(iControlIndex);
  if (!pControl)
    return false;
  if (!bChecked && pControl->IsChecked() == bChecked)
    return false;

  WideString csWExport = pControl->GetExportValue();
  ByteString csBExport = PDF_EncodeText(csWExport);
  int iCount = CountControls();
  bool bUnison = IsUnison(this);
  for (int i = 0; i < iCount; i++) {
    CPDF_FormControl* pCtrl = GetControl(i);
    if (bUnison) {
      WideString csEValue = pCtrl->GetExportValue();
      if (csEValue == csWExport) {
        if (pCtrl->GetOnStateName() == pControl->GetOnStateName())
          pCtrl->CheckControl(bChecked);
        else if (bChecked)
          pCtrl->CheckControl(false);
      } else if (bChecked) {
        pCtrl->CheckControl(false);
      }
    } else {
      if (i == iControlIndex)
        pCtrl->CheckControl(bChecked);
      else if (bChecked)
        pCtrl->CheckControl(false);
    }
  }

  CPDF_Object* pOpt = FPDF_GetFieldAttr(m_pDict.Get(), "Opt");
  if (!ToArray(pOpt)) {
    if (bChecked) {
      m_pDict->SetNewFor<CPDF_Name>("V", csBExport);
    } else {
      ByteString csV;
      CPDF_Object* pV = FPDF_GetFieldAttr(m_pDict.Get(), "V");
      if (pV)
        csV = pV->GetString();
      if (csV == csBExport)
        m_pDict->SetNewFor<CPDF_Name>("V", "Off");
    }
  } else if (bChecked) {
    m_pDict->SetNewFor<CPDF_Name>("V", ByteString::Format("%d", iControlIndex));
  }
  if (bNotify && m_pForm->GetFormNotify())
    m_pForm->GetFormNotify()->AfterCheckedStatusChange(this);
  return true;
}

WideString CPDF_FormField::GetCheckValue(bool bDefault) const {
  ASSERT(GetType() == CheckBox || GetType() == RadioButton);
  WideString csExport = L"Off";
  int iCount = CountControls();
  for (int i = 0; i < iCount; i++) {
    CPDF_FormControl* pControl = GetControl(i);
    bool bChecked =
        bDefault ? pControl->IsDefaultChecked() : pControl->IsChecked();
    if (bChecked) {
      csExport = pControl->GetExportValue();
      break;
    }
  }
  return csExport;
}

bool CPDF_FormField::SetCheckValue(const WideString& value,
                                   bool bDefault,
                                   bool bNotify) {
  ASSERT(GetType() == CheckBox || GetType() == RadioButton);
  int iCount = CountControls();
  for (int i = 0; i < iCount; i++) {
    CPDF_FormControl* pControl = GetControl(i);
    WideString csExport = pControl->GetExportValue();
    bool val = csExport == value;
    if (!bDefault)
      CheckControl(GetControlIndex(pControl), val);
    if (val)
      break;
  }
  if (bNotify && m_pForm->GetFormNotify())
    m_pForm->GetFormNotify()->AfterCheckedStatusChange(this);
  return true;
}

int CPDF_FormField::GetTopVisibleIndex() const {
  CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "TI");
  return pObj ? pObj->GetInteger() : 0;
}

int CPDF_FormField::CountSelectedOptions() const {
  CPDF_Array* pArray = ToArray(FPDF_GetFieldAttr(m_pDict.Get(), "I"));
  return pArray ? pArray->GetCount() : 0;
}

int CPDF_FormField::GetSelectedOptionIndex(int index) const {
  CPDF_Array* pArray = ToArray(FPDF_GetFieldAttr(m_pDict.Get(), "I"));
  if (!pArray)
    return -1;

  int iCount = pArray->GetCount();
  if (iCount < 0 || index >= iCount)
    return -1;
  return pArray->GetIntegerAt(index);
}

bool CPDF_FormField::IsOptionSelected(int iOptIndex) const {
  CPDF_Array* pArray = ToArray(FPDF_GetFieldAttr(m_pDict.Get(), "I"));
  if (!pArray)
    return false;

  for (const auto& pObj : *pArray) {
    if (pObj->GetInteger() == iOptIndex)
      return true;
  }
  return false;
}

bool CPDF_FormField::SelectOption(int iOptIndex, bool bSelected, bool bNotify) {
  CPDF_Array* pArray = m_pDict->GetArrayFor("I");
  if (!pArray) {
    if (!bSelected)
      return true;

    pArray = m_pDict->SetNewFor<CPDF_Array>("I");
  }

  bool bReturn = false;
  for (size_t i = 0; i < pArray->GetCount(); i++) {
    int iFind = pArray->GetIntegerAt(i);
    if (iFind == iOptIndex) {
      if (bSelected)
        return true;

      if (bNotify && m_pForm->GetFormNotify()) {
        WideString csValue = GetOptionLabel(iOptIndex);
        if (!NotifyListOrComboBoxBeforeChange(csValue))
          return false;
      }
      pArray->RemoveAt(i);
      bReturn = true;
      break;
    }

    if (iFind > iOptIndex) {
      if (!bSelected)
        continue;

      if (bNotify && m_pForm->GetFormNotify()) {
        WideString csValue = GetOptionLabel(iOptIndex);
        if (!NotifyListOrComboBoxBeforeChange(csValue))
          return false;
      }
      pArray->InsertNewAt<CPDF_Number>(i, iOptIndex);
      bReturn = true;
      break;
    }
  }
  if (!bReturn) {
    if (bSelected)
      pArray->AddNew<CPDF_Number>(iOptIndex);

    if (pArray->IsEmpty())
      m_pDict->RemoveFor("I");
  }
  if (bNotify)
    NotifyListOrComboBoxAfterChange();

  return true;
}

bool CPDF_FormField::ClearSelectedOptions(bool bNotify) {
  if (bNotify && m_pForm->GetFormNotify()) {
    WideString csValue;
    int iIndex = GetSelectedIndex(0);
    if (iIndex >= 0)
      csValue = GetOptionLabel(iIndex);

    if (!NotifyListOrComboBoxBeforeChange(csValue))
      return false;
  }
  m_pDict->RemoveFor("I");
  if (bNotify)
    NotifyListOrComboBoxAfterChange();

  return true;
}

void CPDF_FormField::LoadDA() {
  CPDF_Dictionary* pFormDict = m_pForm->GetFormDict();
  if (!pFormDict)
    return;

  ByteString DA;
  if (CPDF_Object* pObj = FPDF_GetFieldAttr(m_pDict.Get(), "DA"))
    DA = pObj->GetString();

  if (DA.IsEmpty())
    DA = pFormDict->GetStringFor("DA");

  if (DA.IsEmpty())
    return;

  CPDF_Dictionary* pDR = pFormDict->GetDictFor("DR");
  if (!pDR)
    return;

  CPDF_Dictionary* pFont = pDR->GetDictFor("Font");
  if (!pFont)
    return;

  CPDF_DefaultAppearance appearance(DA);
  Optional<ByteString> font_name = appearance.GetFont(&m_FontSize);
  if (!font_name)
    return;

  CPDF_Dictionary* pFontDict = pFont->GetDictFor(*font_name);
  if (!pFontDict)
    return;

  m_pFont = m_pForm->GetDocument()->LoadFont(pFontDict);
}

bool CPDF_FormField::NotifyBeforeSelectionChange(const WideString& value) {
  if (!m_pForm->GetFormNotify())
    return true;
  return m_pForm->GetFormNotify()->BeforeSelectionChange(this, value);
}

void CPDF_FormField::NotifyAfterSelectionChange() {
  if (!m_pForm->GetFormNotify())
    return;
  m_pForm->GetFormNotify()->AfterSelectionChange(this);
}

bool CPDF_FormField::NotifyBeforeValueChange(const WideString& value) {
  if (!m_pForm->GetFormNotify())
    return true;
  return m_pForm->GetFormNotify()->BeforeValueChange(this, value);
}

void CPDF_FormField::NotifyAfterValueChange() {
  if (!m_pForm->GetFormNotify())
    return;
  m_pForm->GetFormNotify()->AfterValueChange(this);
}

bool CPDF_FormField::NotifyListOrComboBoxBeforeChange(const WideString& value) {
  switch (GetType()) {
    case ListBox:
      return NotifyBeforeSelectionChange(value);
    case ComboBox:
      return NotifyBeforeValueChange(value);
    default:
      return true;
  }
}

void CPDF_FormField::NotifyListOrComboBoxAfterChange() {
  switch (GetType()) {
    case ListBox:
      NotifyAfterSelectionChange();
      break;
    case ComboBox:
      NotifyAfterValueChange();
      break;
    default:
      break;
  }
}
