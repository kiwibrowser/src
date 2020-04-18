// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_PDFFONTMGR_H_
#define XFA_FGAS_FONT_CFGAS_PDFFONTMGR_H_

#include <map>

#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/observable.h"
#include "core/fxcrt/retain_ptr.h"

class CFGAS_FontMgr;
class CFGAS_GEFont;
class CPDF_Document;
class CPDF_Font;

class CFGAS_PDFFontMgr : public Observable<CFGAS_PDFFontMgr> {
 public:
  explicit CFGAS_PDFFontMgr(CPDF_Document* pDoc, CFGAS_FontMgr* pFontMgr);
  ~CFGAS_PDFFontMgr();

  void SetFont(const RetainPtr<CFGAS_GEFont>& pFont, CPDF_Font* pPDFFont);
  RetainPtr<CFGAS_GEFont> GetFont(const WideStringView& wsFontFamily,
                                  uint32_t dwFontStyles,
                                  CPDF_Font** pPDFFont,
                                  bool bStrictMatch);

 private:
  RetainPtr<CFGAS_GEFont> FindFont(const ByteString& strFamilyName,
                                   bool bBold,
                                   bool bItalic,
                                   CPDF_Font** pPDFFont,
                                   bool bStrictMatch);
  ByteString PsNameToFontName(const ByteString& strPsName,
                              bool bBold,
                              bool bItalic);
  bool PsNameMatchDRFontName(const ByteStringView& bsPsName,
                             bool bBold,
                             bool bItalic,
                             const ByteString& bsDRFontName,
                             bool bStrictMatch);

  UnownedPtr<CPDF_Document> const m_pDoc;
  UnownedPtr<CFGAS_FontMgr> const m_pFontMgr;
  std::map<RetainPtr<CFGAS_GEFont>, CPDF_Font*> m_FDE2PDFFont;
  std::map<ByteString, RetainPtr<CFGAS_GEFont>> m_FontMap;
};

#endif  // XFA_FGAS_FONT_CFGAS_PDFFONTMGR_H_
