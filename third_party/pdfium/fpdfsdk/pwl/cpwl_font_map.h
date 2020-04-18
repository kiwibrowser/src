// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_PWL_CPWL_FONT_MAP_H_
#define FPDFSDK_PWL_CPWL_FONT_MAP_H_

#include <memory>
#include <vector>

#include "core/fpdfdoc/ipvt_fontmap.h"
#include "core/fxcrt/fx_codepage.h"
#include "public/fpdf_sysfontinfo.h"

class CPDF_Document;
class CFX_SystemHandler;

struct CPWL_FontMap_Data {
  CPDF_Font* pFont;
  int32_t nCharset;
  ByteString sFontName;
};

struct CPWL_FontMap_Native {
  int32_t nCharset;
  ByteString sFontName;
};

class CPWL_FontMap : public IPVT_FontMap {
 public:
  explicit CPWL_FontMap(CFX_SystemHandler* pSystemHandler);
  ~CPWL_FontMap() override;

  // IPVT_FontMap
  CPDF_Font* GetPDFFont(int32_t nFontIndex) override;
  ByteString GetPDFFontAlias(int32_t nFontIndex) override;
  int32_t GetWordFontIndex(uint16_t word,
                           int32_t nCharset,
                           int32_t nFontIndex) override;
  int32_t CharCodeFromUnicode(int32_t nFontIndex, uint16_t word) override;
  int32_t CharSetFromUnicode(uint16_t word, int32_t nOldCharset) override;

  const CPWL_FontMap_Data* GetFontMapData(int32_t nIndex) const;
  static int32_t GetNativeCharset();
  ByteString GetNativeFontName(int32_t nCharset);

  static ByteString GetDefaultFontByCharset(int32_t nCharset);
  static const FPDF_CharsetFontMap defaultTTFMap[];

 protected:
  virtual void Initialize();
  virtual CPDF_Document* GetDocument();
  virtual CPDF_Font* FindFontSameCharset(ByteString* sFontAlias,
                                         int32_t nCharset);
  virtual void AddedFont(CPDF_Font* pFont, const ByteString& sFontAlias);

  bool KnowWord(int32_t nFontIndex, uint16_t word);

  void Empty();
  int32_t GetFontIndex(const ByteString& sFontName,
                       int32_t nCharset,
                       bool bFind);
  int32_t AddFontData(CPDF_Font* pFont,
                      const ByteString& sFontAlias,
                      int32_t nCharset = FX_CHARSET_Default);

  ByteString EncodeFontAlias(const ByteString& sFontName, int32_t nCharset);
  ByteString EncodeFontAlias(const ByteString& sFontName);

  std::vector<std::unique_ptr<CPWL_FontMap_Data>> m_Data;
  std::vector<std::unique_ptr<CPWL_FontMap_Native>> m_NativeFont;

 private:
  int32_t FindFont(const ByteString& sFontName,
                   int32_t nCharset = FX_CHARSET_Default);

  ByteString GetNativeFont(int32_t nCharset);
  CPDF_Font* AddFontToDocument(CPDF_Document* pDoc,
                               ByteString& sFontName,
                               uint8_t nCharset);
  bool IsStandardFont(const ByteString& sFontName);
  CPDF_Font* AddStandardFont(CPDF_Document* pDoc, ByteString& sFontName);
  CPDF_Font* AddSystemFont(CPDF_Document* pDoc,
                           ByteString& sFontName,
                           uint8_t nCharset);

  std::unique_ptr<CPDF_Document> m_pPDFDoc;
  UnownedPtr<CFX_SystemHandler> const m_pSystemHandler;
};

#endif  // FPDFSDK_PWL_CPWL_FONT_MAP_H_
