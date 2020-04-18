// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FORMFILLER_CBA_FONTMAP_H_
#define FPDFSDK_FORMFILLER_CBA_FONTMAP_H_

#include "core/fxcrt/unowned_ptr.h"
#include "fpdfsdk/pwl/cpwl_font_map.h"

class CPDF_Dictionary;
class CPDFSDK_Annot;

class CBA_FontMap : public CPWL_FontMap {
 public:
  CBA_FontMap(CPDFSDK_Annot* pAnnot, CFX_SystemHandler* pSystemHandler);
  ~CBA_FontMap() override;

  void Reset();
  void SetDefaultFont(CPDF_Font* pFont, const ByteString& sFontName);
  void SetAPType(const ByteString& sAPType);

 private:
  // CPWL_FontMap:
  void Initialize() override;
  CPDF_Document* GetDocument() override;
  CPDF_Font* FindFontSameCharset(ByteString* sFontAlias,
                                 int32_t nCharset) override;
  void AddedFont(CPDF_Font* pFont, const ByteString& sFontAlias) override;

  CPDF_Font* FindResFontSameCharset(CPDF_Dictionary* pResDict,
                                    ByteString* sFontAlias,
                                    int32_t nCharset);
  CPDF_Font* GetAnnotDefaultFont(ByteString* csNameTag);
  void AddFontToAnnotDict(CPDF_Font* pFont, const ByteString& sAlias);

  UnownedPtr<CPDF_Document> m_pDocument;
  UnownedPtr<CPDF_Dictionary> m_pAnnotDict;
  UnownedPtr<CPDF_Font> m_pDefaultFont;
  ByteString m_sDefaultFontName;
  ByteString m_sAPType;
};

#endif  // FPDFSDK_FORMFILLER_CBA_FONTMAP_H_
