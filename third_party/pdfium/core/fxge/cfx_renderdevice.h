// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_RENDERDEVICE_H_
#define CORE_FXGE_CFX_RENDERDEVICE_H_

#include <memory>
#include <vector>

#include "core/fpdfdoc/cpdf_defaultappearance.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_color.h"
#include "core/fxge/fx_dib.h"

class CFX_DIBitmap;
class CFX_Font;
class CFX_GraphStateData;
class CFX_ImageRenderer;
class PauseIndicatorIface;
class RenderDeviceDriverIface;

#define FXDC_DEVICE_CLASS 1
#define FXDC_PIXEL_WIDTH 2
#define FXDC_PIXEL_HEIGHT 3
#define FXDC_BITS_PIXEL 4
#define FXDC_HORZ_SIZE 5
#define FXDC_VERT_SIZE 6
#define FXDC_RENDER_CAPS 7
#define FXDC_DISPLAY 1
#define FXDC_PRINTER 2

#define FXRC_GET_BITS 0x01
#define FXRC_BIT_MASK 0x02
#define FXRC_ALPHA_PATH 0x10
#define FXRC_ALPHA_IMAGE 0x20
#define FXRC_ALPHA_OUTPUT 0x40
#define FXRC_BLEND_MODE 0x80
#define FXRC_SOFT_CLIP 0x100
#define FXRC_CMYK_OUTPUT 0x200
#define FXRC_BITMASK_OUTPUT 0x400
#define FXRC_BYTEMASK_OUTPUT 0x800
#define FXRENDER_IMAGE_LOSSY 0x1000
#define FXRC_FILLSTROKE_PATH 0x2000
#define FXRC_SHADING 0x4000

#define FXFILL_ALTERNATE 1
#define FXFILL_WINDING 2
#define FXFILL_FULLCOVER 4
#define FXFILL_RECT_AA 8
#define FX_FILL_STROKE 16
#define FX_STROKE_ADJUST 32
#define FX_STROKE_TEXT_MODE 64
#define FX_FILL_TEXT_MODE 128
#define FX_ZEROAREA_FILL 256
#define FXFILL_NOPATHSMOOTH 512

#define FXTEXT_CLEARTYPE 0x01
#define FXTEXT_BGR_STRIPE 0x02
#define FXTEXT_PRINTGRAPHICTEXT 0x04
#define FXTEXT_NO_NATIVETEXT 0x08
#define FXTEXT_PRINTIMAGETEXT 0x10
#define FXTEXT_NOSMOOTH 0x20

class CFX_PathData;

enum class FXPT_TYPE : uint8_t { LineTo, BezierTo, MoveTo };

class FXTEXT_CHARPOS {
 public:
  FXTEXT_CHARPOS();
  FXTEXT_CHARPOS(const FXTEXT_CHARPOS&);
  ~FXTEXT_CHARPOS();

  CFX_PointF m_Origin;
  uint32_t m_Unicode;
  uint32_t m_GlyphIndex;
  uint32_t m_FontCharWidth;
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  uint32_t m_ExtGID;
#endif
  int32_t m_FallbackFontPosition;
  bool m_bGlyphAdjust;
  bool m_bFontStyle;
  float m_AdjustMatrix[4];
};

class CFX_RenderDevice {
 public:
  class StateRestorer {
   public:
    explicit StateRestorer(CFX_RenderDevice* pDevice);
    ~StateRestorer();

   private:
    UnownedPtr<CFX_RenderDevice> m_pDevice;
  };

  CFX_RenderDevice();
  virtual ~CFX_RenderDevice();

  // Take ownership of |pDriver|.
  void SetDeviceDriver(std::unique_ptr<RenderDeviceDriverIface> pDriver);
  RenderDeviceDriverIface* GetDeviceDriver() const {
    return m_pDeviceDriver.get();
  }

  void SaveState();
  void RestoreState(bool bKeepSaved);

  int GetWidth() const { return m_Width; }
  int GetHeight() const { return m_Height; }
  int GetDeviceClass() const { return m_DeviceClass; }
  int GetRenderCaps() const { return m_RenderCaps; }
  int GetDeviceCaps(int id) const;
  RetainPtr<CFX_DIBitmap> GetBitmap() const;
  void SetBitmap(const RetainPtr<CFX_DIBitmap>& pBitmap);
  bool CreateCompatibleBitmap(const RetainPtr<CFX_DIBitmap>& pDIB,
                              int width,
                              int height) const;
  const FX_RECT& GetClipBox() const { return m_ClipBox; }
  bool SetClip_PathFill(const CFX_PathData* pPathData,
                        const CFX_Matrix* pObject2Device,
                        int fill_mode);
#ifdef PDF_ENABLE_XFA
  bool SetClip_Rect(const CFX_RectF& rtClip);
#endif
  bool SetClip_Rect(const FX_RECT& pRect);
  bool SetClip_PathStroke(const CFX_PathData* pPathData,
                          const CFX_Matrix* pObject2Device,
                          const CFX_GraphStateData* pGraphState);
  bool DrawPath(const CFX_PathData* pPathData,
                const CFX_Matrix* pObject2Device,
                const CFX_GraphStateData* pGraphState,
                uint32_t fill_color,
                uint32_t stroke_color,
                int fill_mode) {
    return DrawPathWithBlend(pPathData, pObject2Device, pGraphState, fill_color,
                             stroke_color, fill_mode, FXDIB_BLEND_NORMAL);
  }
  bool DrawPathWithBlend(const CFX_PathData* pPathData,
                         const CFX_Matrix* pObject2Device,
                         const CFX_GraphStateData* pGraphState,
                         uint32_t fill_color,
                         uint32_t stroke_color,
                         int fill_mode,
                         int blend_type);
  bool FillRect(const FX_RECT& rect, uint32_t color) {
    return FillRectWithBlend(rect, color, FXDIB_BLEND_NORMAL);
  }

  RetainPtr<CFX_DIBitmap> GetBackDrop();
  bool GetDIBits(const RetainPtr<CFX_DIBitmap>& pBitmap, int left, int top);
  bool SetDIBits(const RetainPtr<CFX_DIBSource>& pBitmap, int left, int top) {
    return SetDIBitsWithBlend(pBitmap, left, top, FXDIB_BLEND_NORMAL);
  }
  bool SetDIBitsWithBlend(const RetainPtr<CFX_DIBSource>& pBitmap,
                          int left,
                          int top,
                          int blend_type);
  bool StretchDIBits(const RetainPtr<CFX_DIBSource>& pBitmap,
                     int left,
                     int top,
                     int dest_width,
                     int dest_height) {
    return StretchDIBitsWithFlagsAndBlend(pBitmap, left, top, dest_width,
                                          dest_height, 0, FXDIB_BLEND_NORMAL);
  }
  bool StretchDIBitsWithFlagsAndBlend(const RetainPtr<CFX_DIBSource>& pBitmap,
                                      int left,
                                      int top,
                                      int dest_width,
                                      int dest_height,
                                      uint32_t flags,
                                      int blend_type);
  bool SetBitMask(const RetainPtr<CFX_DIBSource>& pBitmap,
                  int left,
                  int top,
                  uint32_t color);
  bool StretchBitMask(const RetainPtr<CFX_DIBSource>& pBitmap,
                      int left,
                      int top,
                      int dest_width,
                      int dest_height,
                      uint32_t color);
  bool StretchBitMaskWithFlags(const RetainPtr<CFX_DIBSource>& pBitmap,
                               int left,
                               int top,
                               int dest_width,
                               int dest_height,
                               uint32_t color,
                               uint32_t flags);
  bool StartDIBits(const RetainPtr<CFX_DIBSource>& pBitmap,
                   int bitmap_alpha,
                   uint32_t color,
                   const CFX_Matrix* pMatrix,
                   uint32_t flags,
                   std::unique_ptr<CFX_ImageRenderer>* handle) {
    return StartDIBitsWithBlend(pBitmap, bitmap_alpha, color, pMatrix, flags,
                                handle, FXDIB_BLEND_NORMAL);
  }
  bool StartDIBitsWithBlend(const RetainPtr<CFX_DIBSource>& pBitmap,
                            int bitmap_alpha,
                            uint32_t color,
                            const CFX_Matrix* pMatrix,
                            uint32_t flags,
                            std::unique_ptr<CFX_ImageRenderer>* handle,
                            int blend_type);
  bool ContinueDIBits(CFX_ImageRenderer* handle, PauseIndicatorIface* pPause);

  bool DrawNormalText(int nChars,
                      const FXTEXT_CHARPOS* pCharPos,
                      CFX_Font* pFont,
                      float font_size,
                      const CFX_Matrix* pText2Device,
                      uint32_t fill_color,
                      uint32_t text_flags);
  bool DrawTextPath(int nChars,
                    const FXTEXT_CHARPOS* pCharPos,
                    CFX_Font* pFont,
                    float font_size,
                    const CFX_Matrix* pText2User,
                    const CFX_Matrix* pUser2Device,
                    const CFX_GraphStateData* pGraphState,
                    uint32_t fill_color,
                    uint32_t stroke_color,
                    CFX_PathData* pClippingPath,
                    int nFlag);

  void DrawFillRect(const CFX_Matrix* pUser2Device,
                    const CFX_FloatRect& rect,
                    const CFX_Color& color,
                    int32_t nTransparency);
  void DrawFillRect(const CFX_Matrix* pUser2Device,
                    const CFX_FloatRect& rect,
                    const FX_COLORREF& color);
  void DrawStrokeRect(const CFX_Matrix& mtUser2Device,
                      const CFX_FloatRect& rect,
                      const FX_COLORREF& color,
                      float fWidth);
  void DrawStrokeLine(const CFX_Matrix* pUser2Device,
                      const CFX_PointF& ptMoveTo,
                      const CFX_PointF& ptLineTo,
                      const FX_COLORREF& color,
                      float fWidth);
  void DrawBorder(const CFX_Matrix* pUser2Device,
                  const CFX_FloatRect& rect,
                  float fWidth,
                  const CFX_Color& color,
                  const CFX_Color& crLeftTop,
                  const CFX_Color& crRightBottom,
                  BorderStyle nStyle,
                  int32_t nTransparency);
  void DrawFillArea(const CFX_Matrix& mtUser2Device,
                    const std::vector<CFX_PointF>& points,
                    const FX_COLORREF& color);
  void DrawShadow(const CFX_Matrix& mtUser2Device,
                  bool bVertical,
                  bool bHorizontal,
                  const CFX_FloatRect& rect,
                  int32_t nTransparency,
                  int32_t nStartGray,
                  int32_t nEndGray);

#ifdef _SKIA_SUPPORT_
  virtual void DebugVerifyBitmapIsPreMultiplied() const;
  virtual bool SetBitsWithMask(const RetainPtr<CFX_DIBSource>& pBitmap,
                               const RetainPtr<CFX_DIBSource>& pMask,
                               int left,
                               int top,
                               int bitmap_alpha,
                               int blend_type);
#endif
#if defined _SKIA_SUPPORT_ || defined _SKIA_SUPPORT_PATHS_
  void Flush(bool release);
#endif

 private:
  void InitDeviceInfo();
  void UpdateClipBox();
  bool DrawFillStrokePath(const CFX_PathData* pPathData,
                          const CFX_Matrix* pObject2Device,
                          const CFX_GraphStateData* pGraphState,
                          uint32_t fill_color,
                          uint32_t stroke_color,
                          int fill_mode,
                          int blend_type);
  bool DrawCosmeticLine(const CFX_PointF& ptMoveTo,
                        const CFX_PointF& ptLineTo,
                        uint32_t color,
                        int fill_mode,
                        int blend_type);
  bool FillRectWithBlend(const FX_RECT& rect, uint32_t color, int blend_type);

  RetainPtr<CFX_DIBitmap> m_pBitmap;
  int m_Width;
  int m_Height;
  int m_bpp;
  int m_RenderCaps;
  int m_DeviceClass;
  FX_RECT m_ClipBox;
  std::unique_ptr<RenderDeviceDriverIface> m_pDeviceDriver;
};

#endif  // CORE_FXGE_CFX_RENDERDEVICE_H_
