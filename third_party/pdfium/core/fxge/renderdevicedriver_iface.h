// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_RENDERDEVICEDRIVER_IFACE_H_
#define CORE_FXGE_RENDERDEVICEDRIVER_IFACE_H_

#include <memory>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"

class CFX_DIBitmap;
class CFX_DIBSource;
class CFX_Font;
class CFX_GraphStateData;
class CFX_ImageRenderer;
class CFX_Matrix;
class CFX_PathData;
class CPDF_ShadingPattern;
class FXTEXT_CHARPOS;
class PauseIndicatorIface;
struct FX_RECT;

class RenderDeviceDriverIface {
 public:
  virtual ~RenderDeviceDriverIface();

  virtual int GetDeviceCaps(int caps_id) const = 0;

  virtual bool StartRendering();
  virtual void EndRendering();
  virtual void SaveState() = 0;
  virtual void RestoreState(bool bKeepSaved) = 0;

  virtual bool SetClip_PathFill(const CFX_PathData* pPathData,
                                const CFX_Matrix* pObject2Device,
                                int fill_mode) = 0;
  virtual bool SetClip_PathStroke(const CFX_PathData* pPathData,
                                  const CFX_Matrix* pObject2Device,
                                  const CFX_GraphStateData* pGraphState);
  virtual bool DrawPath(const CFX_PathData* pPathData,
                        const CFX_Matrix* pObject2Device,
                        const CFX_GraphStateData* pGraphState,
                        uint32_t fill_color,
                        uint32_t stroke_color,
                        int fill_mode,
                        int blend_type) = 0;
  virtual bool SetPixel(int x, int y, uint32_t color);
  virtual bool FillRectWithBlend(const FX_RECT& rect,
                                 uint32_t fill_color,
                                 int blend_type);
  virtual bool DrawCosmeticLine(const CFX_PointF& ptMoveTo,
                                const CFX_PointF& ptLineTo,
                                uint32_t color,
                                int blend_type);

  virtual bool GetClipBox(FX_RECT* pRect) = 0;
  virtual bool GetDIBits(const RetainPtr<CFX_DIBitmap>& pBitmap,
                         int left,
                         int top);
  virtual RetainPtr<CFX_DIBitmap> GetBackDrop();
  virtual bool SetDIBits(const RetainPtr<CFX_DIBSource>& pBitmap,
                         uint32_t color,
                         const FX_RECT* pSrcRect,
                         int dest_left,
                         int dest_top,
                         int blend_type) = 0;
  virtual bool StretchDIBits(const RetainPtr<CFX_DIBSource>& pBitmap,
                             uint32_t color,
                             int dest_left,
                             int dest_top,
                             int dest_width,
                             int dest_height,
                             const FX_RECT* pClipRect,
                             uint32_t flags,
                             int blend_type) = 0;
  virtual bool StartDIBits(const RetainPtr<CFX_DIBSource>& pBitmap,
                           int bitmap_alpha,
                           uint32_t color,
                           const CFX_Matrix* pMatrix,
                           uint32_t flags,
                           std::unique_ptr<CFX_ImageRenderer>* handle,
                           int blend_type) = 0;
  virtual bool ContinueDIBits(CFX_ImageRenderer* handle,
                              PauseIndicatorIface* pPause);
  virtual bool DrawDeviceText(int nChars,
                              const FXTEXT_CHARPOS* pCharPos,
                              CFX_Font* pFont,
                              const CFX_Matrix* pObject2Device,
                              float font_size,
                              uint32_t color);
  virtual int GetDriverType() const;
  virtual void ClearDriver();
  virtual bool DrawShading(const CPDF_ShadingPattern* pPattern,
                           const CFX_Matrix* pMatrix,
                           const FX_RECT& clip_rect,
                           int alpha,
                           bool bAlphaMode);
  virtual bool SetBitsWithMask(const RetainPtr<CFX_DIBSource>& pBitmap,
                               const RetainPtr<CFX_DIBSource>& pMask,
                               int left,
                               int top,
                               int bitmap_alpha,
                               int blend_type);
#if defined _SKIA_SUPPORT_ || defined _SKIA_SUPPORT_PATHS_
  virtual void Flush();
#endif
};

#endif  // CORE_FXGE_RENDERDEVICEDRIVER_IFACE_H_
