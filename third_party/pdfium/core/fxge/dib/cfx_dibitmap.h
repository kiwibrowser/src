// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_DIB_CFX_DIBITMAP_H_
#define CORE_FXGE_DIB_CFX_DIBITMAP_H_

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/maybe_owned.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/dib/cfx_dibsource.h"
#include "third_party/base/stl_util.h"

class CFX_DIBitmap : public CFX_DIBSource {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  bool Create(int width,
              int height,
              FXDIB_Format format,
              uint8_t* pBuffer = nullptr,
              uint32_t pitch = 0);

  bool Copy(const RetainPtr<CFX_DIBSource>& pSrc);

  // CFX_DIBSource
  uint8_t* GetBuffer() const override;
  const uint8_t* GetScanline(int line) const override;
  void DownSampleScanline(int line,
                          uint8_t* dest_scan,
                          int dest_bpp,
                          int dest_width,
                          bool bFlipX,
                          int clip_left,
                          int clip_width) const override;

  void TakeOver(RetainPtr<CFX_DIBitmap>&& pSrcBitmap);
  bool ConvertFormat(FXDIB_Format format);
  void Clear(uint32_t color);

  uint32_t GetPixel(int x, int y) const;
  void SetPixel(int x, int y, uint32_t color);

  bool LoadChannel(FXDIB_Channel destChannel,
                   const RetainPtr<CFX_DIBSource>& pSrcBitmap,
                   FXDIB_Channel srcChannel);
  bool LoadChannel(FXDIB_Channel destChannel, int value);

  bool MultiplyAlpha(int alpha);
  bool MultiplyAlpha(const RetainPtr<CFX_DIBSource>& pAlphaMask);

  bool TransferBitmap(int dest_left,
                      int dest_top,
                      int width,
                      int height,
                      const RetainPtr<CFX_DIBSource>& pSrcBitmap,
                      int src_left,
                      int src_top);

  bool CompositeBitmap(int dest_left,
                       int dest_top,
                       int width,
                       int height,
                       const RetainPtr<CFX_DIBSource>& pSrcBitmap,
                       int src_left,
                       int src_top,
                       int blend_type = FXDIB_BLEND_NORMAL,
                       const CFX_ClipRgn* pClipRgn = nullptr,
                       bool bRgbByteOrder = false);

  bool CompositeMask(int dest_left,
                     int dest_top,
                     int width,
                     int height,
                     const RetainPtr<CFX_DIBSource>& pMask,
                     uint32_t color,
                     int src_left,
                     int src_top,
                     int blend_type = FXDIB_BLEND_NORMAL,
                     const CFX_ClipRgn* pClipRgn = nullptr,
                     bool bRgbByteOrder = false,
                     int alpha_flag = 0);

  bool CompositeRect(int dest_left,
                     int dest_top,
                     int width,
                     int height,
                     uint32_t color,
                     int alpha_flag);

  bool ConvertColorScale(uint32_t forecolor, uint32_t backcolor);

  static bool CalculatePitchAndSize(int height,
                                    int width,
                                    FXDIB_Format format,
                                    uint32_t* pitch,
                                    uint32_t* size);

#if defined _SKIA_SUPPORT_ || _SKIA_SUPPORT_PATHS_
  void PreMultiply();
#endif
#if defined _SKIA_SUPPORT_PATHS_
  void UnPreMultiply();
#endif

 protected:
  CFX_DIBitmap();
  CFX_DIBitmap(const CFX_DIBitmap& src);
  ~CFX_DIBitmap() override;

#if defined _SKIA_SUPPORT_PATHS_
  enum class Format { kCleared, kPreMultiplied, kUnPreMultiplied };
#endif

  MaybeOwned<uint8_t, FxFreeDeleter> m_pBuffer;
#if defined _SKIA_SUPPORT_PATHS_
  Format m_nFormat;
#endif

 private:
  void ConvertBGRColorScale(uint32_t forecolor, uint32_t backcolor);
  void ConvertCMYKColorScale(uint32_t forecolor, uint32_t backcolor);
  bool TransferWithUnequalFormats(FXDIB_Format dest_format,
                                  int dest_left,
                                  int dest_top,
                                  int width,
                                  int height,
                                  const RetainPtr<CFX_DIBSource>& pSrcBitmap,
                                  int src_left,
                                  int src_top);
  void TransferWithMultipleBPP(int dest_left,
                               int dest_top,
                               int width,
                               int height,
                               const RetainPtr<CFX_DIBSource>& pSrcBitmap,
                               int src_left,
                               int src_top);
  void TransferEqualFormatsOneBPP(int dest_left,
                                  int dest_top,
                                  int width,
                                  int height,
                                  const RetainPtr<CFX_DIBSource>& pSrcBitmap,
                                  int src_left,
                                  int src_top);
};

#endif  // CORE_FXGE_DIB_CFX_DIBITMAP_H_
