// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_IMAGE_H_
#define CORE_FPDFAPI_PAGE_CPDF_IMAGE_H_

#include <memory>

#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/maybe_owned.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_DIBSource;
class CFX_DIBitmap;
class CPDF_Document;
class CPDF_Page;
class PauseIndicatorIface;
class IFX_SeekableReadStream;

class CPDF_Image : public Retainable {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  void ConvertStreamToIndirectObject();

  CPDF_Stream* GetStream() const { return m_pStream.Get(); }
  CPDF_Dictionary* GetDict() const;
  CPDF_Dictionary* GetOC() const { return m_pOC.Get(); }
  CPDF_Document* GetDocument() const { return m_pDocument.Get(); }

  int32_t GetPixelHeight() const { return m_Height; }
  int32_t GetPixelWidth() const { return m_Width; }

  bool IsInline() const { return m_bIsInline; }
  bool IsMask() const { return m_bIsMask; }
  bool IsInterpol() const { return m_bInterpolate; }

  RetainPtr<CFX_DIBSource> LoadDIBSource() const;

  void SetImage(const RetainPtr<CFX_DIBitmap>& pDIBitmap);
  void SetJpegImage(const RetainPtr<IFX_SeekableReadStream>& pFile);
  void SetJpegImageInline(const RetainPtr<IFX_SeekableReadStream>& pFile);

  void ResetCache(CPDF_Page* pPage, const RetainPtr<CFX_DIBitmap>& pDIBitmap);

  // Returns whether to Continue() or not.
  bool StartLoadDIBSource(const CPDF_Dictionary* pFormResource,
                          CPDF_Dictionary* pPageResource,
                          bool bStdCS = false,
                          uint32_t GroupFamily = 0,
                          bool bLoadMask = false);

  // Returns whether to Continue() or not.
  bool Continue(PauseIndicatorIface* pPause);

  RetainPtr<CFX_DIBSource> DetachBitmap();
  RetainPtr<CFX_DIBSource> DetachMask();

  RetainPtr<CFX_DIBSource> m_pDIBSource;
  RetainPtr<CFX_DIBSource> m_pMask;
  uint32_t m_MatteColor = 0;

 private:
  explicit CPDF_Image(CPDF_Document* pDoc);
  CPDF_Image(CPDF_Document* pDoc, std::unique_ptr<CPDF_Stream> pStream);
  CPDF_Image(CPDF_Document* pDoc, uint32_t dwStreamObjNum);
  ~CPDF_Image() override;

  void FinishInitialization(CPDF_Dictionary* pStreamDict);
  std::unique_ptr<CPDF_Dictionary> InitJPEG(uint8_t* pData, uint32_t size);

  int32_t m_Height = 0;
  int32_t m_Width = 0;
  bool m_bIsInline = false;
  bool m_bIsMask = false;
  bool m_bInterpolate = false;
  UnownedPtr<CPDF_Document> const m_pDocument;
  MaybeOwned<CPDF_Stream> m_pStream;
  UnownedPtr<CPDF_Dictionary> m_pOC;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_IMAGE_H_
