// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_FONT_H_
#define CORE_FXGE_CFX_FONT_H_

#include <memory>
#include <vector>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/fx_freetype.h"

#if defined _SKIA_SUPPORT_ || defined _SKIA_SUPPORT_PATHS_
#include "core/fxge/fx_font.h"
#endif

class CFX_FaceCache;
class CFX_GlyphBitmap;
class CFX_PathData;
class CFX_SubstFont;
class IFX_SeekableReadStream;

class CFX_Font {
 public:
  CFX_Font();
  ~CFX_Font();

  void LoadSubst(const ByteString& face_name,
                 bool bTrueType,
                 uint32_t flags,
                 int weight,
                 int italic_angle,
                 int CharsetCP,
                 bool bVertical);

  bool LoadEmbedded(const uint8_t* data, uint32_t size);
  FXFT_Face GetFace() const { return m_Face; }
  CFX_SubstFont* GetSubstFont() const { return m_pSubstFont.get(); }

#ifdef PDF_ENABLE_XFA
  bool LoadFile(const RetainPtr<IFX_SeekableReadStream>& pFile, int nFaceIndex);

  void SetFace(FXFT_Face face);
  void SetSubstFont(std::unique_ptr<CFX_SubstFont> subst);
#endif  // PDF_ENABLE_XFA

  const CFX_GlyphBitmap* LoadGlyphBitmap(uint32_t glyph_index,
                                         bool bFontStyle,
                                         const CFX_Matrix* pMatrix,
                                         uint32_t dest_width,
                                         int anti_alias,
                                         int& text_flags) const;
  const CFX_PathData* LoadGlyphPath(uint32_t glyph_index,
                                    uint32_t dest_width) const;

#if defined _SKIA_SUPPORT_ || defined _SKIA_SUPPORT_PATHS_
  CFX_TypeFace* GetDeviceCache() const;
#endif

  uint32_t GetGlyphWidth(uint32_t glyph_index);
  int GetAscent() const;
  int GetDescent() const;
  bool GetGlyphBBox(uint32_t glyph_index, FX_RECT* pBBox);
  bool IsItalic() const;
  bool IsBold() const;
  bool IsFixedWidth() const;
  bool IsVertical() const { return m_bVertical; }
  ByteString GetPsName() const;
  ByteString GetFamilyName() const;
  ByteString GetFaceName() const;
  bool IsTTFont() const;
  bool GetBBox(FX_RECT* pBBox);
  bool IsEmbedded() const { return m_bEmbedded; }
  uint8_t* GetSubData() const { return m_pGsubData.get(); }
  void SetSubData(uint8_t* data) { m_pGsubData.reset(data); }
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  void* GetPlatformFont() const { return m_pPlatformFont; }
  void SetPlatformFont(void* font) { m_pPlatformFont = font; }
#endif
  uint8_t* GetFontData() const { return m_pFontData; }
  uint32_t GetSize() const { return m_dwSize; }
  void AdjustMMParams(int glyph_index, int dest_width, int weight) const;

  CFX_PathData* LoadGlyphPathImpl(uint32_t glyph_index,
                                  uint32_t dest_width) const;

  static const size_t kAngleSkewArraySize = 30;
  static const char s_AngleSkew[kAngleSkewArraySize];
  static const size_t kWeightPowArraySize = 100;
  static const uint8_t s_WeightPow[kWeightPowArraySize];
  static const uint8_t s_WeightPow_11[kWeightPowArraySize];
  static const uint8_t s_WeightPow_SHIFTJIS[kWeightPowArraySize];

#ifdef PDF_ENABLE_XFA
 protected:
  std::unique_ptr<FXFT_StreamRec> m_pOwnedStream;
#endif  // PDF_ENABLE_XFA

 private:
  CFX_FaceCache* GetFaceCache() const;
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  void ReleasePlatformResource();
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  void DeleteFace();
  void ClearFaceCache();

  FXFT_Face m_Face;
  mutable UnownedPtr<CFX_FaceCache> m_FaceCache;
  std::unique_ptr<CFX_SubstFont> m_pSubstFont;
  std::vector<uint8_t> m_pFontDataAllocation;
  uint8_t* m_pFontData;
  std::unique_ptr<uint8_t, FxFreeDeleter> m_pGsubData;
  uint32_t m_dwSize;
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  void* m_pPlatformFont;
#endif
  bool m_bEmbedded;
  bool m_bVertical;
};

#endif  // CORE_FXGE_CFX_FONT_H_
