// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPDF_INTERFORM_H_
#define CORE_FPDFDOC_CPDF_INTERFORM_H_

#include <map>
#include <memory>
#include <vector>

#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfdoc/cpdf_defaultappearance.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/unowned_ptr.h"

class CFieldTree;
class CFDF_Document;
class CPDF_Document;
class CPDF_Dictionary;
class CPDF_Font;
class CPDF_FormControl;
class CPDF_FormField;
class CPDF_Object;
class CPDF_Page;
class IPDF_FormNotify;

CPDF_Font* AddNativeInterFormFont(CPDF_Dictionary*& pFormDict,
                                  CPDF_Document* pDocument,
                                  ByteString* csNameTag);

class CPDF_InterForm {
 public:
  explicit CPDF_InterForm(CPDF_Document* pDocument);
  ~CPDF_InterForm();

  static void SetUpdateAP(bool bUpdateAP);
  static bool IsUpdateAPEnabled();
  static ByteString GenerateNewResourceName(const CPDF_Dictionary* pResDict,
                                            const char* csType,
                                            int iMinLen,
                                            const char* csPrefix);
  static CPDF_Font* AddStandardFont(CPDF_Document* pDocument,
                                    ByteString csFontName);
  static ByteString GetNativeFont(uint8_t iCharSet, void* pLogFont);
  static uint8_t GetNativeCharSet();
  static CPDF_Font* AddNativeFont(uint8_t iCharSet, CPDF_Document* pDocument);
  static CPDF_Font* AddNativeFont(CPDF_Document* pDocument);

  size_t CountFields(const WideString& csFieldName) const;
  CPDF_FormField* GetField(uint32_t index, const WideString& csFieldName) const;
  CPDF_FormField* GetFieldByDict(CPDF_Dictionary* pFieldDict) const;

  CPDF_FormControl* GetControlAtPoint(CPDF_Page* pPage,
                                      const CFX_PointF& point,
                                      int* z_order) const;
  CPDF_FormControl* GetControlByDict(const CPDF_Dictionary* pWidgetDict) const;

  bool NeedConstructAP() const;
  int CountFieldsInCalculationOrder();
  CPDF_FormField* GetFieldInCalculationOrder(int index);
  int FindFieldInCalculationOrder(const CPDF_FormField* pField);

  CPDF_Font* GetFormFont(ByteString csNameTag) const;
  CPDF_DefaultAppearance GetDefaultAppearance() const;
  int GetFormAlignment() const;

  bool CheckRequiredFields(const std::vector<CPDF_FormField*>* fields,
                           bool bIncludeOrExclude) const;

  std::unique_ptr<CFDF_Document> ExportToFDF(const WideString& pdf_path,
                                             bool bSimpleFileSpec) const;

  std::unique_ptr<CFDF_Document> ExportToFDF(
      const WideString& pdf_path,
      const std::vector<CPDF_FormField*>& fields,
      bool bIncludeOrExclude,
      bool bSimpleFileSpec) const;

  void ResetForm(const std::vector<CPDF_FormField*>& fields,
                 bool bIncludeOrExclude,
                 bool bNotify);
  void ResetForm(bool bNotify);

  void SetFormNotify(IPDF_FormNotify* pNotify);
  bool HasXFAForm() const;
  void FixPageFields(const CPDF_Page* pPage);

  IPDF_FormNotify* GetFormNotify() const { return m_pFormNotify.Get(); }
  CPDF_Document* GetDocument() const { return m_pDocument.Get(); }
  CPDF_Dictionary* GetFormDict() const { return m_pFormDict.Get(); }

 private:
  void LoadField(CPDF_Dictionary* pFieldDict, int nLevel);
  void AddTerminalField(CPDF_Dictionary* pFieldDict);
  CPDF_FormControl* AddControl(CPDF_FormField* pField,
                               CPDF_Dictionary* pWidgetDict);
  void FDF_ImportField(CPDF_Dictionary* pField,
                       const WideString& parent_name,
                       bool bNotify = false,
                       int nLevel = 0);

  static bool s_bUpdateAP;

  UnownedPtr<CPDF_Document> const m_pDocument;
  UnownedPtr<CPDF_Dictionary> m_pFormDict;
  std::map<const CPDF_Dictionary*, std::unique_ptr<CPDF_FormControl>>
      m_ControlMap;
  std::unique_ptr<CFieldTree> m_pFieldTree;
  ByteString m_bsEncoding;
  UnownedPtr<IPDF_FormNotify> m_pFormNotify;
};

#endif  // CORE_FPDFDOC_CPDF_INTERFORM_H_
