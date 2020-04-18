// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include <windows.h>

#include <algorithm>
#include <memory>
#include <sstream>

#include "core/fxcrt/fx_system.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_pathdata.h"
#include "core/fxge/win32/cfx_windowsdib.h"
#include "core/fxge/win32/win32_int.h"
#include "third_party/base/ptr_util.h"

// Has to come before gdiplus.h
namespace Gdiplus {
using std::min;
using std::max;
}  // namespace Gdiplus

#include <gdiplus.h>  // NOLINT

// TODO(thestig): Remove the infrequently used ones.
using Gdiplus::CombineMode;
using Gdiplus::DashCap;
using Gdiplus::DashCapFlat;
using Gdiplus::DashCapRound;
using Gdiplus::FillModeAlternate;
using Gdiplus::FillModeWinding;
using Gdiplus::GdiplusStartupInput;
using Gdiplus::GdiplusStartupOutput;
using Gdiplus::GpBitmap;
using Gdiplus::GpBrush;
using Gdiplus::GpDashCap;
using Gdiplus::GpDashStyle;
using Gdiplus::GpFillMode;
using Gdiplus::GpFont;
using Gdiplus::GpFontCollection;
using Gdiplus::GpFontFamily;
using Gdiplus::GpGraphics;
using Gdiplus::GpImage;
using Gdiplus::GpLineCap;
using Gdiplus::GpLineJoin;
using Gdiplus::GpMatrix;
using Gdiplus::GpPath;
using Gdiplus::GpPen;
using Gdiplus::GpPoint;
using Gdiplus::GpPointF;
using Gdiplus::GpRect;
using Gdiplus::GpRegion;
using Gdiplus::GpSolidFill;
using Gdiplus::GpStatus;
using Gdiplus::GpStringFormat;
using Gdiplus::GpUnit;
using Gdiplus::ImageLockModeRead;
using Gdiplus::InterpolationMode;
using Gdiplus::InterpolationModeBilinear;
using Gdiplus::InterpolationModeHighQuality;
using Gdiplus::InterpolationModeNearestNeighbor;
using Gdiplus::LineCap;
using Gdiplus::LineCapFlat;
using Gdiplus::LineCapRound;
using Gdiplus::LineCapSquare;
using Gdiplus::LineJoin;
using Gdiplus::LineJoinBevel;
using Gdiplus::LineJoinMiterClipped;
using Gdiplus::LineJoinRound;
using Gdiplus::PaletteFlagsHasAlpha;
using Gdiplus::PathPointTypeBezier;
using Gdiplus::PathPointTypeCloseSubpath;
using Gdiplus::PathPointTypeLine;
using Gdiplus::PathPointTypeStart;
using Gdiplus::PixelOffsetMode;
using Gdiplus::PixelOffsetModeHalf;
using Gdiplus::REAL;
using Gdiplus::SmoothingMode;
using Gdiplus::SmoothingModeAntiAlias;
using Gdiplus::SmoothingModeNone;
using Gdiplus::TextRenderingHint;
using Gdiplus::UnitPixel;
using Gdiplus::UnitWorld;

#define GdiFillType2Gdip(fill_type) \
  (fill_type == ALTERNATE ? FillModeAlternate : FillModeWinding)

namespace {

enum {
  FuncId_GdipCreatePath2,
  FuncId_GdipSetPenDashStyle,
  FuncId_GdipSetPenDashArray,
  FuncId_GdipSetPenDashCap197819,
  FuncId_GdipSetPenLineJoin,
  FuncId_GdipSetPenWidth,
  FuncId_GdipCreateFromHDC,
  FuncId_GdipSetPageUnit,
  FuncId_GdipSetSmoothingMode,
  FuncId_GdipCreateSolidFill,
  FuncId_GdipFillPath,
  FuncId_GdipDeleteBrush,
  FuncId_GdipCreatePen1,
  FuncId_GdipSetPenMiterLimit,
  FuncId_GdipDrawPath,
  FuncId_GdipDeletePen,
  FuncId_GdipDeletePath,
  FuncId_GdipDeleteGraphics,
  FuncId_GdipCreateBitmapFromFileICM,
  FuncId_GdipCreateBitmapFromStreamICM,
  FuncId_GdipGetImageHeight,
  FuncId_GdipGetImageWidth,
  FuncId_GdipGetImagePixelFormat,
  FuncId_GdipBitmapLockBits,
  FuncId_GdipGetImagePaletteSize,
  FuncId_GdipGetImagePalette,
  FuncId_GdipBitmapUnlockBits,
  FuncId_GdipDisposeImage,
  FuncId_GdipFillRectangle,
  FuncId_GdipCreateBitmapFromScan0,
  FuncId_GdipSetImagePalette,
  FuncId_GdipSetInterpolationMode,
  FuncId_GdipDrawImagePointsI,
  FuncId_GdipCreateBitmapFromGdiDib,
  FuncId_GdiplusStartup,
  FuncId_GdipDrawLineI,
  FuncId_GdipResetClip,
  FuncId_GdipCreatePath,
  FuncId_GdipAddPathPath,
  FuncId_GdipSetPathFillMode,
  FuncId_GdipSetClipPath,
  FuncId_GdipGetClip,
  FuncId_GdipCreateRegion,
  FuncId_GdipGetClipBoundsI,
  FuncId_GdipSetClipRegion,
  FuncId_GdipWidenPath,
  FuncId_GdipAddPathLine,
  FuncId_GdipAddPathRectangle,
  FuncId_GdipDeleteRegion,
  FuncId_GdipGetDC,
  FuncId_GdipReleaseDC,
  FuncId_GdipSetPenLineCap197819,
  FuncId_GdipSetPenDashOffset,
  FuncId_GdipResetPath,
  FuncId_GdipCreateRegionPath,
  FuncId_GdipCreateFont,
  FuncId_GdipGetFontSize,
  FuncId_GdipCreateFontFamilyFromName,
  FuncId_GdipSetTextRenderingHint,
  FuncId_GdipDrawDriverString,
  FuncId_GdipCreateMatrix2,
  FuncId_GdipDeleteMatrix,
  FuncId_GdipSetWorldTransform,
  FuncId_GdipResetWorldTransform,
  FuncId_GdipDeleteFontFamily,
  FuncId_GdipDeleteFont,
  FuncId_GdipNewPrivateFontCollection,
  FuncId_GdipDeletePrivateFontCollection,
  FuncId_GdipPrivateAddMemoryFont,
  FuncId_GdipGetFontCollectionFamilyList,
  FuncId_GdipGetFontCollectionFamilyCount,
  FuncId_GdipSetTextContrast,
  FuncId_GdipSetPixelOffsetMode,
  FuncId_GdipGetImageGraphicsContext,
  FuncId_GdipDrawImageI,
  FuncId_GdipDrawImageRectI,
  FuncId_GdipDrawString,
  FuncId_GdipSetPenTransform,
};

LPCSTR g_GdipFuncNames[] = {
    "GdipCreatePath2",
    "GdipSetPenDashStyle",
    "GdipSetPenDashArray",
    "GdipSetPenDashCap197819",
    "GdipSetPenLineJoin",
    "GdipSetPenWidth",
    "GdipCreateFromHDC",
    "GdipSetPageUnit",
    "GdipSetSmoothingMode",
    "GdipCreateSolidFill",
    "GdipFillPath",
    "GdipDeleteBrush",
    "GdipCreatePen1",
    "GdipSetPenMiterLimit",
    "GdipDrawPath",
    "GdipDeletePen",
    "GdipDeletePath",
    "GdipDeleteGraphics",
    "GdipCreateBitmapFromFileICM",
    "GdipCreateBitmapFromStreamICM",
    "GdipGetImageHeight",
    "GdipGetImageWidth",
    "GdipGetImagePixelFormat",
    "GdipBitmapLockBits",
    "GdipGetImagePaletteSize",
    "GdipGetImagePalette",
    "GdipBitmapUnlockBits",
    "GdipDisposeImage",
    "GdipFillRectangle",
    "GdipCreateBitmapFromScan0",
    "GdipSetImagePalette",
    "GdipSetInterpolationMode",
    "GdipDrawImagePointsI",
    "GdipCreateBitmapFromGdiDib",
    "GdiplusStartup",
    "GdipDrawLineI",
    "GdipResetClip",
    "GdipCreatePath",
    "GdipAddPathPath",
    "GdipSetPathFillMode",
    "GdipSetClipPath",
    "GdipGetClip",
    "GdipCreateRegion",
    "GdipGetClipBoundsI",
    "GdipSetClipRegion",
    "GdipWidenPath",
    "GdipAddPathLine",
    "GdipAddPathRectangle",
    "GdipDeleteRegion",
    "GdipGetDC",
    "GdipReleaseDC",
    "GdipSetPenLineCap197819",
    "GdipSetPenDashOffset",
    "GdipResetPath",
    "GdipCreateRegionPath",
    "GdipCreateFont",
    "GdipGetFontSize",
    "GdipCreateFontFamilyFromName",
    "GdipSetTextRenderingHint",
    "GdipDrawDriverString",
    "GdipCreateMatrix2",
    "GdipDeleteMatrix",
    "GdipSetWorldTransform",
    "GdipResetWorldTransform",
    "GdipDeleteFontFamily",
    "GdipDeleteFont",
    "GdipNewPrivateFontCollection",
    "GdipDeletePrivateFontCollection",
    "GdipPrivateAddMemoryFont",
    "GdipGetFontCollectionFamilyList",
    "GdipGetFontCollectionFamilyCount",
    "GdipSetTextContrast",
    "GdipSetPixelOffsetMode",
    "GdipGetImageGraphicsContext",
    "GdipDrawImageI",
    "GdipDrawImageRectI",
    "GdipDrawString",
    "GdipSetPenTransform",
};
static_assert(FX_ArraySize(g_GdipFuncNames) ==
                  static_cast<size_t>(FuncId_GdipSetPenTransform) + 1,
              "g_GdipFuncNames has wrong size");

typedef GpStatus(WINGDIPAPI* FuncType_GdipCreatePath2)(GDIPCONST GpPointF*,
                                                       GDIPCONST BYTE*,
                                                       INT,
                                                       GpFillMode,
                                                       GpPath** path);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenDashStyle)(
    GpPen* pen,
    GpDashStyle dashstyle);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenDashArray)(GpPen* pen,
                                                           GDIPCONST REAL* dash,
                                                           INT count);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenDashCap197819)(
    GpPen* pen,
    GpDashCap dashCap);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenLineJoin)(GpPen* pen,
                                                          GpLineJoin lineJoin);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenWidth)(GpPen* pen, REAL width);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateFromHDC)(HDC hdc,
                                                         GpGraphics** graphics);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPageUnit)(GpGraphics* graphics,
                                                       GpUnit unit);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetSmoothingMode)(
    GpGraphics* graphics,
    SmoothingMode smoothingMode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateSolidFill)(Gdiplus::ARGB color,
                                                           GpSolidFill** brush);
typedef GpStatus(WINGDIPAPI* FuncType_GdipFillPath)(GpGraphics* graphics,
                                                    GpBrush* brush,
                                                    GpPath* path);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeleteBrush)(GpBrush* brush);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreatePen1)(Gdiplus::ARGB color,
                                                      REAL width,
                                                      GpUnit unit,
                                                      GpPen** pen);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenMiterLimit)(GpPen* pen,
                                                            REAL miterLimit);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawPath)(GpGraphics* graphics,
                                                    GpPen* pen,
                                                    GpPath* path);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeletePen)(GpPen* pen);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeletePath)(GpPath* path);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeleteGraphics)(GpGraphics* graphics);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateBitmapFromFileICM)(
    GDIPCONST WCHAR* filename,
    GpBitmap** bitmap);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateBitmapFromStreamICM)(
    IStream* stream,
    GpBitmap** bitmap);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetImageWidth)(GpImage* image,
                                                         UINT* width);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetImageHeight)(GpImage* image,
                                                          UINT* height);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetImagePixelFormat)(
    GpImage* image,
    Gdiplus::PixelFormat* format);
typedef GpStatus(WINGDIPAPI* FuncType_GdipBitmapLockBits)(
    GpBitmap* bitmap,
    GDIPCONST GpRect* rect,
    UINT flags,
    Gdiplus::PixelFormat format,
    Gdiplus::BitmapData* lockedBitmapData);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetImagePalette)(
    GpImage* image,
    Gdiplus::ColorPalette* palette,
    INT size);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetImagePaletteSize)(GpImage* image,
                                                               INT* size);
typedef GpStatus(WINGDIPAPI* FuncType_GdipBitmapUnlockBits)(
    GpBitmap* bitmap,
    Gdiplus::BitmapData* lockedBitmapData);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDisposeImage)(GpImage* image);
typedef GpStatus(WINGDIPAPI* FuncType_GdipFillRectangle)(GpGraphics* graphics,
                                                         GpBrush* brush,
                                                         REAL x,
                                                         REAL y,
                                                         REAL width,
                                                         REAL height);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateBitmapFromScan0)(
    INT width,
    INT height,
    INT stride,
    Gdiplus::PixelFormat format,
    BYTE* scan0,
    GpBitmap** bitmap);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetImagePalette)(
    GpImage* image,
    GDIPCONST Gdiplus::ColorPalette* palette);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetInterpolationMode)(
    GpGraphics* graphics,
    InterpolationMode interpolationMode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawImagePointsI)(
    GpGraphics* graphics,
    GpImage* image,
    GDIPCONST GpPoint* dstpoints,
    INT count);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateBitmapFromGdiDib)(
    GDIPCONST BITMAPINFO* gdiBitmapInfo,
    VOID* gdiBitmapData,
    GpBitmap** bitmap);
typedef Gdiplus::Status(WINAPI* FuncType_GdiplusStartup)(
    OUT uintptr_t* token,
    const GdiplusStartupInput* input,
    OUT GdiplusStartupOutput* output);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawLineI)(GpGraphics* graphics,
                                                     GpPen* pen,
                                                     int x1,
                                                     int y1,
                                                     int x2,
                                                     int y2);
typedef GpStatus(WINGDIPAPI* FuncType_GdipResetClip)(GpGraphics* graphics);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreatePath)(GpFillMode brushMode,
                                                      GpPath** path);
typedef GpStatus(WINGDIPAPI* FuncType_GdipAddPathPath)(
    GpPath* path,
    GDIPCONST GpPath* addingPath,
    BOOL connect);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPathFillMode)(GpPath* path,
                                                           GpFillMode fillmode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetClipPath)(GpGraphics* graphics,
                                                       GpPath* path,
                                                       CombineMode combineMode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetClip)(GpGraphics* graphics,
                                                   GpRegion* region);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateRegion)(GpRegion** region);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetClipBoundsI)(GpGraphics* graphics,
                                                          GpRect* rect);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetClipRegion)(
    GpGraphics* graphics,
    GpRegion* region,
    CombineMode combineMode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipWidenPath)(GpPath* nativePath,
                                                     GpPen* pen,
                                                     GpMatrix* matrix,
                                                     REAL flatness);
typedef GpStatus(WINGDIPAPI* FuncType_GdipAddPathLine)(GpPath* path,
                                                       REAL x1,
                                                       REAL y1,
                                                       REAL x2,
                                                       REAL y2);
typedef GpStatus(WINGDIPAPI* FuncType_GdipAddPathRectangle)(GpPath* path,
                                                            REAL x,
                                                            REAL y,
                                                            REAL width,
                                                            REAL height);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeleteRegion)(GpRegion* region);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetDC)(GpGraphics* graphics,
                                                 HDC* hdc);
typedef GpStatus(WINGDIPAPI* FuncType_GdipReleaseDC)(GpGraphics* graphics,
                                                     HDC hdc);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenLineCap197819)(
    GpPen* pen,
    GpLineCap startCap,
    GpLineCap endCap,
    GpDashCap dashCap);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenDashOffset)(GpPen* pen,
                                                            REAL offset);
typedef GpStatus(WINGDIPAPI* FuncType_GdipResetPath)(GpPath* path);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateRegionPath)(GpPath* path,
                                                            GpRegion** region);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateFont)(
    GDIPCONST GpFontFamily* fontFamily,
    REAL emSize,
    INT style,
    Gdiplus::Unit unit,
    GpFont** font);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetFontSize)(GpFont* font,
                                                       REAL* size);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateFontFamilyFromName)(
    GDIPCONST WCHAR* name,
    GpFontCollection* fontCollection,
    GpFontFamily** FontFamily);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetTextRenderingHint)(
    GpGraphics* graphics,
    TextRenderingHint mode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawDriverString)(
    GpGraphics* graphics,
    GDIPCONST UINT16* text,
    INT length,
    GDIPCONST GpFont* font,
    GDIPCONST GpBrush* brush,
    GDIPCONST Gdiplus::PointF* positions,
    INT flags,
    GDIPCONST GpMatrix* matrix);
typedef GpStatus(WINGDIPAPI* FuncType_GdipCreateMatrix2)(REAL m11,
                                                         REAL m12,
                                                         REAL m21,
                                                         REAL m22,
                                                         REAL dx,
                                                         REAL dy,
                                                         GpMatrix** matrix);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeleteMatrix)(GpMatrix* matrix);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetWorldTransform)(
    GpGraphics* graphics,
    GpMatrix* matrix);
typedef GpStatus(WINGDIPAPI* FuncType_GdipResetWorldTransform)(
    GpGraphics* graphics);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeleteFontFamily)(
    GpFontFamily* FontFamily);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeleteFont)(GpFont* font);
typedef GpStatus(WINGDIPAPI* FuncType_GdipNewPrivateFontCollection)(
    GpFontCollection** fontCollection);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDeletePrivateFontCollection)(
    GpFontCollection** fontCollection);
typedef GpStatus(WINGDIPAPI* FuncType_GdipPrivateAddMemoryFont)(
    GpFontCollection* fontCollection,
    GDIPCONST void* memory,
    INT length);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetFontCollectionFamilyList)(
    GpFontCollection* fontCollection,
    INT numSought,
    GpFontFamily* gpfamilies[],
    INT* numFound);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetFontCollectionFamilyCount)(
    GpFontCollection* fontCollection,
    INT* numFound);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetTextContrast)(GpGraphics* graphics,
                                                           UINT contrast);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPixelOffsetMode)(
    GpGraphics* graphics,
    PixelOffsetMode pixelOffsetMode);
typedef GpStatus(WINGDIPAPI* FuncType_GdipGetImageGraphicsContext)(
    GpImage* image,
    GpGraphics** graphics);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawImageI)(GpGraphics* graphics,
                                                      GpImage* image,
                                                      INT x,
                                                      INT y);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawImageRectI)(GpGraphics* graphics,
                                                          GpImage* image,
                                                          INT x,
                                                          INT y,
                                                          INT width,
                                                          INT height);
typedef GpStatus(WINGDIPAPI* FuncType_GdipDrawString)(
    GpGraphics* graphics,
    GDIPCONST WCHAR* str,
    INT length,
    GDIPCONST GpFont* font,
    GDIPCONST Gdiplus::RectF* layoutRect,
    GDIPCONST GpStringFormat* stringFormat,
    GDIPCONST GpBrush* brush);
typedef GpStatus(WINGDIPAPI* FuncType_GdipSetPenTransform)(GpPen* pen,
                                                           GpMatrix* matrix);
#define CallFunc(funcname) \
  ((FuncType_##funcname)GdiplusExt.m_Functions[FuncId_##funcname])

GpBrush* GdipCreateBrushImpl(DWORD argb) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpSolidFill* solidBrush = nullptr;
  CallFunc(GdipCreateSolidFill)((Gdiplus::ARGB)argb, &solidBrush);
  return solidBrush;
}

RetainPtr<CFX_DIBitmap> StretchMonoToGray(
    int dest_width,
    int dest_height,
    const RetainPtr<CFX_DIBitmap>& pSource,
    FX_RECT* pClipRect) {
  bool bFlipX = dest_width < 0;
  if (bFlipX)
    dest_width = -dest_width;

  bool bFlipY = dest_height < 0;
  if (bFlipY)
    dest_height = -dest_height;

  int result_width = pClipRect->Width();
  int result_height = pClipRect->Height();
  int result_pitch = (result_width + 3) / 4 * 4;
  auto pStretched = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!pStretched->Create(result_width, result_height, FXDIB_8bppRgb))
    return nullptr;

  LPBYTE dest_buf = pStretched->GetBuffer();
  int src_width = pSource->GetWidth();
  int src_height = pSource->GetHeight();
  int y_unit = src_height / dest_height;
  int x_unit = src_width / dest_width;
  int area_unit = y_unit * x_unit;
  LPBYTE src_buf = pSource->GetBuffer();
  int src_pitch = pSource->GetPitch();
  for (int dest_y = 0; dest_y < result_height; dest_y++) {
    LPBYTE dest_scan = dest_buf + dest_y * result_pitch;
    int src_y_start = bFlipY ? (dest_height - 1 - dest_y - pClipRect->top)
                             : (dest_y + pClipRect->top);
    src_y_start = src_y_start * src_height / dest_height;
    LPBYTE src_scan = src_buf + src_y_start * src_pitch;
    for (int dest_x = 0; dest_x < result_width; dest_x++) {
      int sum = 0;
      int src_x_start = bFlipX ? (dest_width - 1 - dest_x - pClipRect->left)
                               : (dest_x + pClipRect->left);
      src_x_start = src_x_start * src_width / dest_width;
      int src_x_end = src_x_start + x_unit;
      LPBYTE src_line = src_scan;
      for (int src_y = 0; src_y < y_unit; src_y++) {
        for (int src_x = src_x_start; src_x < src_x_end; src_x++) {
          if (!(src_line[src_x / 8] & (1 << (7 - src_x % 8)))) {
            sum += 255;
          }
        }
        src_line += src_pitch;
      }
      dest_scan[dest_x] = 255 - sum / area_unit;
    }
  }
  return pStretched;
}

void OutputImageMask(GpGraphics* pGraphics,
                     BOOL bMonoDevice,
                     const RetainPtr<CFX_DIBitmap>& pBitmap,
                     int dest_left,
                     int dest_top,
                     int dest_width,
                     int dest_height,
                     FX_ARGB argb,
                     const FX_RECT* pClipRect) {
  ASSERT(pBitmap->GetBPP() == 1);
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  int src_width = pBitmap->GetWidth(), src_height = pBitmap->GetHeight();
  int src_pitch = pBitmap->GetPitch();
  uint8_t* scan0 = pBitmap->GetBuffer();
  if (src_width == 1 && src_height == 1) {
    if ((scan0[0] & 0x80) == 0)
      return;

    GpSolidFill* solidBrush;
    CallFunc(GdipCreateSolidFill)((Gdiplus::ARGB)argb, &solidBrush);
    if (dest_width < 0) {
      dest_width = -dest_width;
      dest_left -= dest_width;
    }
    if (dest_height < 0) {
      dest_height = -dest_height;
      dest_top -= dest_height;
    }
    CallFunc(GdipFillRectangle)(pGraphics, solidBrush, (float)dest_left,
                                (float)dest_top, (float)dest_width,
                                (float)dest_height);
    CallFunc(GdipDeleteBrush)(solidBrush);
    return;
  }
  if (!bMonoDevice && abs(dest_width) < src_width &&
      abs(dest_height) < src_height) {
    FX_RECT image_rect(dest_left, dest_top, dest_left + dest_width,
                       dest_top + dest_height);
    image_rect.Normalize();
    FX_RECT image_clip = image_rect;
    image_clip.Intersect(*pClipRect);
    if (image_clip.IsEmpty())
      return;

    image_clip.Offset(-image_rect.left, -image_rect.top);
    RetainPtr<CFX_DIBitmap> pStretched;
    if (src_width * src_height > 10000) {
      pStretched =
          StretchMonoToGray(dest_width, dest_height, pBitmap, &image_clip);
    } else {
      pStretched =
          pBitmap->StretchTo(dest_width, dest_height, false, &image_clip);
    }
    GpBitmap* bitmap;
    CallFunc(GdipCreateBitmapFromScan0)(image_clip.Width(), image_clip.Height(),
                                        (image_clip.Width() + 3) / 4 * 4,
                                        PixelFormat8bppIndexed,
                                        pStretched->GetBuffer(), &bitmap);
    int a;
    int r;
    int g;
    int b;
    std::tie(a, r, g, b) = ArgbDecode(argb);
    UINT pal[258];
    pal[0] = 0;
    pal[1] = 256;
    for (int i = 0; i < 256; i++) {
      pal[i + 2] = ArgbEncode(i * a / 255, r, g, b);
    }
    CallFunc(GdipSetImagePalette)(bitmap, (Gdiplus::ColorPalette*)pal);
    CallFunc(GdipDrawImageI)(pGraphics, bitmap,
                             image_rect.left + image_clip.left,
                             image_rect.top + image_clip.top);
    CallFunc(GdipDisposeImage)(bitmap);
    return;
  }
  GpBitmap* bitmap;
  CallFunc(GdipCreateBitmapFromScan0)(src_width, src_height, src_pitch,
                                      PixelFormat1bppIndexed, scan0, &bitmap);
  UINT palette[4] = {PaletteFlagsHasAlpha, 2, 0, argb};
  CallFunc(GdipSetImagePalette)(bitmap, (Gdiplus::ColorPalette*)palette);
  Gdiplus::Point destinationPoints[] = {
      Gdiplus::Point(dest_left, dest_top),
      Gdiplus::Point(dest_left + dest_width, dest_top),
      Gdiplus::Point(dest_left, dest_top + dest_height)};
  CallFunc(GdipDrawImagePointsI)(pGraphics, bitmap, destinationPoints, 3);
  CallFunc(GdipDisposeImage)(bitmap);
}

void OutputImage(GpGraphics* pGraphics,
                 const RetainPtr<CFX_DIBitmap>& pBitmap,
                 const FX_RECT* pSrcRect,
                 int dest_left,
                 int dest_top,
                 int dest_width,
                 int dest_height) {
  int src_width = pSrcRect->Width(), src_height = pSrcRect->Height();
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  if (pBitmap->GetBPP() == 1 && (pSrcRect->left % 8)) {
    FX_RECT new_rect(0, 0, src_width, src_height);
    RetainPtr<CFX_DIBitmap> pCloned = pBitmap->Clone(pSrcRect);
    if (!pCloned)
      return;
    OutputImage(pGraphics, pCloned, &new_rect, dest_left, dest_top, dest_width,
                dest_height);
    return;
  }
  int src_pitch = pBitmap->GetPitch();
  uint8_t* scan0 = pBitmap->GetBuffer() + pSrcRect->top * src_pitch +
                   pBitmap->GetBPP() * pSrcRect->left / 8;
  GpBitmap* bitmap = nullptr;
  switch (pBitmap->GetFormat()) {
    case FXDIB_Argb:
      CallFunc(GdipCreateBitmapFromScan0)(src_width, src_height, src_pitch,
                                          PixelFormat32bppARGB, scan0, &bitmap);
      break;
    case FXDIB_Rgb32:
      CallFunc(GdipCreateBitmapFromScan0)(src_width, src_height, src_pitch,
                                          PixelFormat32bppRGB, scan0, &bitmap);
      break;
    case FXDIB_Rgb:
      CallFunc(GdipCreateBitmapFromScan0)(src_width, src_height, src_pitch,
                                          PixelFormat24bppRGB, scan0, &bitmap);
      break;
    case FXDIB_8bppRgb: {
      CallFunc(GdipCreateBitmapFromScan0)(src_width, src_height, src_pitch,
                                          PixelFormat8bppIndexed, scan0,
                                          &bitmap);
      UINT pal[258];
      pal[0] = 0;
      pal[1] = 256;
      for (int i = 0; i < 256; i++)
        pal[i + 2] = pBitmap->GetPaletteArgb(i);
      CallFunc(GdipSetImagePalette)(bitmap, (Gdiplus::ColorPalette*)pal);
      break;
    }
    case FXDIB_1bppRgb: {
      CallFunc(GdipCreateBitmapFromScan0)(src_width, src_height, src_pitch,
                                          PixelFormat1bppIndexed, scan0,
                                          &bitmap);
      break;
    }
  }
  if (dest_height < 0) {
    dest_height--;
  }
  if (dest_width < 0) {
    dest_width--;
  }
  Gdiplus::Point destinationPoints[] = {
      Gdiplus::Point(dest_left, dest_top),
      Gdiplus::Point(dest_left + dest_width, dest_top),
      Gdiplus::Point(dest_left, dest_top + dest_height)};
  CallFunc(GdipDrawImagePointsI)(pGraphics, bitmap, destinationPoints, 3);
  CallFunc(GdipDisposeImage)(bitmap);
}

GpPen* GdipCreatePenImpl(const CFX_GraphStateData* pGraphState,
                         const CFX_Matrix* pMatrix,
                         DWORD argb,
                         bool bTextMode) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  float width = pGraphState->m_LineWidth;
  if (!bTextMode) {
    float unit = pMatrix
                     ? 1.0f / ((pMatrix->GetXUnit() + pMatrix->GetYUnit()) / 2)
                     : 1.0f;
    width = std::max(width, unit);
  }
  GpPen* pPen = nullptr;
  CallFunc(GdipCreatePen1)((Gdiplus::ARGB)argb, width, UnitWorld, &pPen);
  LineCap lineCap = LineCapFlat;
  DashCap dashCap = DashCapFlat;
  bool bDashExtend = false;
  switch (pGraphState->m_LineCap) {
    case CFX_GraphStateData::LineCapButt:
      lineCap = LineCapFlat;
      break;
    case CFX_GraphStateData::LineCapRound:
      lineCap = LineCapRound;
      dashCap = DashCapRound;
      bDashExtend = true;
      break;
    case CFX_GraphStateData::LineCapSquare:
      lineCap = LineCapSquare;
      bDashExtend = true;
      break;
  }
  CallFunc(GdipSetPenLineCap197819)(pPen, lineCap, lineCap, dashCap);
  LineJoin lineJoin = LineJoinMiterClipped;
  switch (pGraphState->m_LineJoin) {
    case CFX_GraphStateData::LineJoinMiter:
      lineJoin = LineJoinMiterClipped;
      break;
    case CFX_GraphStateData::LineJoinRound:
      lineJoin = LineJoinRound;
      break;
    case CFX_GraphStateData::LineJoinBevel:
      lineJoin = LineJoinBevel;
      break;
  }
  CallFunc(GdipSetPenLineJoin)(pPen, lineJoin);
  if (pGraphState->m_DashCount) {
    float* pDashArray = FX_Alloc(
        float, pGraphState->m_DashCount + pGraphState->m_DashCount % 2);
    int nCount = 0;
    float on_leftover = 0, off_leftover = 0;
    for (int i = 0; i < pGraphState->m_DashCount; i += 2) {
      float on_phase = pGraphState->m_DashArray[i];
      float off_phase;
      if (i == pGraphState->m_DashCount - 1)
        off_phase = on_phase;
      else
        off_phase = pGraphState->m_DashArray[i + 1];
      on_phase /= width;
      off_phase /= width;
      if (on_phase + off_phase <= 0.00002f) {
        on_phase = 1.0f / 10;
        off_phase = 1.0f / 10;
      }
      if (bDashExtend) {
        if (off_phase < 1)
          off_phase = 0;
        else
          off_phase -= 1;
        on_phase += 1;
      }
      if (on_phase == 0 || off_phase == 0) {
        if (nCount == 0) {
          on_leftover += on_phase;
          off_leftover += off_phase;
        } else {
          pDashArray[nCount - 2] += on_phase;
          pDashArray[nCount - 1] += off_phase;
        }
      } else {
        pDashArray[nCount++] = on_phase + on_leftover;
        on_leftover = 0;
        pDashArray[nCount++] = off_phase + off_leftover;
        off_leftover = 0;
      }
    }
    CallFunc(GdipSetPenDashArray)(pPen, pDashArray, nCount);
    float phase = pGraphState->m_DashPhase;
    if (bDashExtend) {
      if (phase < 0.5f)
        phase = 0;
      else
        phase -= 0.5f;
    }
    CallFunc(GdipSetPenDashOffset)(pPen, phase);
    FX_Free(pDashArray);
    pDashArray = nullptr;
  }
  CallFunc(GdipSetPenMiterLimit)(pPen, pGraphState->m_MiterLimit);
  return pPen;
}

bool IsSmallTriangle(Gdiplus::PointF* points,
                     const CFX_Matrix* pMatrix,
                     int& v1,
                     int& v2) {
  int pairs[] = {1, 2, 0, 2, 0, 1};
  for (int i = 0; i < 3; i++) {
    int pair1 = pairs[i * 2];
    int pair2 = pairs[i * 2 + 1];

    CFX_PointF p1(points[pair1].X, points[pair1].Y);
    CFX_PointF p2(points[pair2].X, points[pair2].Y);
    if (pMatrix) {
      p1 = pMatrix->Transform(p1);
      p2 = pMatrix->Transform(p2);
    }

    CFX_PointF diff = p1 - p2;
    float distance_square = (diff.x * diff.x) + (diff.y * diff.y);
    if (distance_square < (1.0f * 2 + 1.0f / 4)) {
      v1 = i;
      v2 = pair1;
      return true;
    }
  }
  return false;
}

class GpStream final : public IStream {
 public:
  GpStream() : m_RefCount(1), m_ReadPos(0) {}
  ~GpStream() = default;

  // IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
                                           void** ppvObject) override {
    if (iid == __uuidof(IUnknown) || iid == __uuidof(IStream) ||
        iid == __uuidof(ISequentialStream)) {
      *ppvObject = static_cast<IStream*>(this);
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() override {
    return (ULONG)InterlockedIncrement(&m_RefCount);
  }
  ULONG STDMETHODCALLTYPE Release() override {
    ULONG res = (ULONG)InterlockedDecrement(&m_RefCount);
    if (res == 0) {
      delete this;
    }
    return res;
  }

  // ISequentialStream
  HRESULT STDMETHODCALLTYPE Read(void* output,
                                 ULONG cb,
                                 ULONG* pcbRead) override {
    if (pcbRead)
      *pcbRead = 0;

    if (m_ReadPos >= m_InterStream.tellp())
      return HRESULT_FROM_WIN32(ERROR_END_OF_MEDIA);

    size_t bytes_left = m_InterStream.tellp() - m_ReadPos;
    size_t bytes_out =
        std::min(pdfium::base::checked_cast<size_t>(cb), bytes_left);
    memcpy(output, m_InterStream.str().c_str() + m_ReadPos, bytes_out);
    m_ReadPos += bytes_out;
    if (pcbRead)
      *pcbRead = (ULONG)bytes_out;

    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Write(const void* input,
                                  ULONG cb,
                                  ULONG* pcbWritten) override {
    if (cb <= 0) {
      if (pcbWritten)
        *pcbWritten = 0;
      return S_OK;
    }
    m_InterStream.write(reinterpret_cast<const char*>(input), cb);
    if (pcbWritten)
      *pcbWritten = cb;
    return S_OK;
  }

  // IStream
  HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE CopyTo(IStream*,
                                   ULARGE_INTEGER,
                                   ULARGE_INTEGER*,
                                   ULARGE_INTEGER*) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE Commit(DWORD) override { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE Revert() override { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER,
                                       ULARGE_INTEGER,
                                       DWORD) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER,
                                         ULARGE_INTEGER,
                                         DWORD) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE Clone(IStream** stream) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove,
                                 DWORD dwOrigin,
                                 ULARGE_INTEGER* lpNewFilePointer) override {
    std::streamoff start;
    std::streamoff new_read_position;
    switch (dwOrigin) {
      case STREAM_SEEK_SET:
        start = 0;
        break;
      case STREAM_SEEK_CUR:
        start = m_ReadPos;
        break;
      case STREAM_SEEK_END:
        if (m_InterStream.tellp() < 0)
          return STG_E_SEEKERROR;
        start = m_InterStream.tellp();
        break;
      default:
        return STG_E_INVALIDFUNCTION;
    }
    new_read_position = start + liDistanceToMove.QuadPart;
    if (new_read_position > m_InterStream.tellp())
      return STG_E_SEEKERROR;

    m_ReadPos = new_read_position;
    if (lpNewFilePointer)
      lpNewFilePointer->QuadPart = m_ReadPos;

    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg,
                                 DWORD grfStatFlag) override {
    if (!pStatstg)
      return STG_E_INVALIDFUNCTION;

    ZeroMemory(pStatstg, sizeof(STATSTG));

    if (m_InterStream.tellp() < 0)
      return STG_E_SEEKERROR;

    pStatstg->cbSize.QuadPart = m_InterStream.tellp();
    return S_OK;
  }

 private:
  LONG m_RefCount;
  std::streamoff m_ReadPos;
  std::ostringstream m_InterStream;
};

struct PREVIEW3_DIBITMAP {
  BITMAPINFO* pbmi;
  int Stride;
  LPBYTE pScan0;
  GpBitmap* pBitmap;
  Gdiplus::BitmapData* pBitmapData;
  GpStream* pStream;
};

PREVIEW3_DIBITMAP* LoadDIBitmap(WINDIB_Open_Args_ args) {
  GpBitmap* pBitmap;
  GpStream* pStream = nullptr;
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  Gdiplus::Status status = Gdiplus::Ok;
  if (args.flags == WINDIB_OPEN_PATHNAME) {
    status = CallFunc(GdipCreateBitmapFromFileICM)((wchar_t*)args.path_name,
                                                   &pBitmap);
  } else {
    if (args.memory_size == 0 || !args.memory_base)
      return nullptr;

    pStream = new GpStream;
    pStream->Write(args.memory_base, (ULONG)args.memory_size, nullptr);
    status = CallFunc(GdipCreateBitmapFromStreamICM)(pStream, &pBitmap);
  }
  if (status != Gdiplus::Ok) {
    if (pStream)
      pStream->Release();

    return nullptr;
  }
  UINT height, width;
  CallFunc(GdipGetImageHeight)(pBitmap, &height);
  CallFunc(GdipGetImageWidth)(pBitmap, &width);
  Gdiplus::PixelFormat pixel_format;
  CallFunc(GdipGetImagePixelFormat)(pBitmap, &pixel_format);
  int info_size = sizeof(BITMAPINFOHEADER);
  int bpp = 24;
  int dest_pixel_format = PixelFormat24bppRGB;
  if (pixel_format == PixelFormat1bppIndexed) {
    info_size += 8;
    bpp = 1;
    dest_pixel_format = PixelFormat1bppIndexed;
  } else if (pixel_format == PixelFormat8bppIndexed) {
    info_size += 1024;
    bpp = 8;
    dest_pixel_format = PixelFormat8bppIndexed;
  } else if (pixel_format == PixelFormat32bppARGB) {
    bpp = 32;
    dest_pixel_format = PixelFormat32bppARGB;
  }
  LPBYTE buf = FX_Alloc(BYTE, info_size);
  BITMAPINFOHEADER* pbmih = (BITMAPINFOHEADER*)buf;
  pbmih->biBitCount = bpp;
  pbmih->biCompression = BI_RGB;
  pbmih->biHeight = -(int)height;
  pbmih->biPlanes = 1;
  pbmih->biWidth = width;
  Gdiplus::Rect rect(0, 0, width, height);
  Gdiplus::BitmapData* pBitmapData = FX_Alloc(Gdiplus::BitmapData, 1);
  CallFunc(GdipBitmapLockBits)(pBitmap, &rect, ImageLockModeRead,
                               dest_pixel_format, pBitmapData);
  if (pixel_format == PixelFormat1bppIndexed ||
      pixel_format == PixelFormat8bppIndexed) {
    DWORD* ppal = (DWORD*)(buf + sizeof(BITMAPINFOHEADER));
    struct {
      UINT flags;
      UINT Count;
      DWORD Entries[256];
    } pal;
    int size = 0;
    CallFunc(GdipGetImagePaletteSize)(pBitmap, &size);
    CallFunc(GdipGetImagePalette)(pBitmap, (Gdiplus::ColorPalette*)&pal, size);
    int entries = pixel_format == PixelFormat1bppIndexed ? 2 : 256;
    for (int i = 0; i < entries; i++) {
      ppal[i] = pal.Entries[i] & 0x00ffffff;
    }
  }
  PREVIEW3_DIBITMAP* pInfo = FX_Alloc(PREVIEW3_DIBITMAP, 1);
  pInfo->pbmi = (BITMAPINFO*)buf;
  pInfo->pScan0 = (LPBYTE)pBitmapData->Scan0;
  pInfo->Stride = pBitmapData->Stride;
  pInfo->pBitmap = pBitmap;
  pInfo->pBitmapData = pBitmapData;
  pInfo->pStream = pStream;
  return pInfo;
}

void FreeDIBitmap(PREVIEW3_DIBITMAP* pInfo) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipBitmapUnlockBits)(pInfo->pBitmap, pInfo->pBitmapData);
  CallFunc(GdipDisposeImage)(pInfo->pBitmap);
  FX_Free(pInfo->pBitmapData);
  FX_Free((LPBYTE)pInfo->pbmi);
  if (pInfo->pStream)
    pInfo->pStream->Release();
  FX_Free(pInfo);
}

}  // namespace

void* CGdiplusExt::GdiAddFontMemResourceEx(void* pFontdata,
                                           uint32_t size,
                                           void* pdv,
                                           uint32_t* num_face) {
  if (!m_pGdiAddFontMemResourceEx)
    return nullptr;

  return m_pGdiAddFontMemResourceEx((PVOID)pFontdata, (DWORD)size, (PVOID)pdv,
                                    (DWORD*)num_face);
}

bool CGdiplusExt::GdiRemoveFontMemResourceEx(void* handle) {
  return m_pGdiRemoveFontMemResourseEx &&
         m_pGdiRemoveFontMemResourseEx((HANDLE)handle);
}

CGdiplusExt::CGdiplusExt() {}

CGdiplusExt::~CGdiplusExt() {
  FreeLibrary(m_GdiModule);
  FreeLibrary(m_hModule);
}

void CGdiplusExt::Load() {
  char buf[MAX_PATH];
  GetSystemDirectoryA(buf, MAX_PATH);
  ByteString dllpath = buf;
  dllpath += "\\GDIPLUS.DLL";
  m_hModule = LoadLibraryA(dllpath.c_str());
  if (!m_hModule)
    return;

  m_Functions.resize(FX_ArraySize(g_GdipFuncNames));
  for (size_t i = 0; i < FX_ArraySize(g_GdipFuncNames); ++i) {
    m_Functions[i] = GetProcAddress(m_hModule, g_GdipFuncNames[i]);
    if (!m_Functions[i]) {
      m_hModule = nullptr;
      return;
    }
  }

  uintptr_t gdiplusToken;
  GdiplusStartupInput gdiplusStartupInput;
  ((FuncType_GdiplusStartup)m_Functions[FuncId_GdiplusStartup])(
      &gdiplusToken, &gdiplusStartupInput, nullptr);
  m_GdiModule = LoadLibraryA("GDI32.DLL");
  if (!m_GdiModule)
    return;

  m_pGdiAddFontMemResourceEx =
      reinterpret_cast<FuncType_GdiAddFontMemResourceEx>(
          GetProcAddress(m_GdiModule, "AddFontMemResourceEx"));
  m_pGdiRemoveFontMemResourseEx =
      reinterpret_cast<FuncType_GdiRemoveFontMemResourceEx>(
          GetProcAddress(m_GdiModule, "RemoveFontMemResourceEx"));
}

LPVOID CGdiplusExt::LoadMemFont(LPBYTE pData, uint32_t size) {
  GpFontCollection* pCollection = nullptr;
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipNewPrivateFontCollection)(&pCollection);
  GpStatus status =
      CallFunc(GdipPrivateAddMemoryFont)(pCollection, pData, size);
  if (status == Gdiplus::Ok)
    return pCollection;

  CallFunc(GdipDeletePrivateFontCollection)(&pCollection);
  return nullptr;
}
void CGdiplusExt::DeleteMemFont(LPVOID pCollection) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  auto* pActualType = static_cast<GpFontCollection*>(pCollection);
  CallFunc(GdipDeletePrivateFontCollection)(&pActualType);
}
bool CGdiplusExt::GdipCreateBitmap(const RetainPtr<CFX_DIBitmap>& pBitmap,
                                   void** bitmap) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  Gdiplus::PixelFormat format;
  switch (pBitmap->GetFormat()) {
    case FXDIB_Rgb:
      format = PixelFormat24bppRGB;
      break;
    case FXDIB_Rgb32:
      format = PixelFormat32bppRGB;
      break;
    case FXDIB_Argb:
      format = PixelFormat32bppARGB;
      break;
    default:
      return false;
  }
  GpStatus status = CallFunc(GdipCreateBitmapFromScan0)(
      pBitmap->GetWidth(), pBitmap->GetHeight(), pBitmap->GetPitch(), format,
      pBitmap->GetBuffer(), (GpBitmap**)bitmap);
  return status == Gdiplus::Ok;
}
bool CGdiplusExt::GdipCreateFromImage(void* bitmap, void** graphics) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpStatus status = CallFunc(GdipGetImageGraphicsContext)(
      (GpBitmap*)bitmap, (GpGraphics**)graphics);
  return status == Gdiplus::Ok;
}
bool CGdiplusExt::GdipCreateFontFamilyFromName(const wchar_t* name,
                                               void* pFontCollection,
                                               void** pFamily) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpStatus status = CallFunc(GdipCreateFontFamilyFromName)(
      (GDIPCONST WCHAR*)name, (GpFontCollection*)pFontCollection,
      (GpFontFamily**)pFamily);
  return status == Gdiplus::Ok;
}
bool CGdiplusExt::GdipCreateFontFromFamily(void* pFamily,
                                           float font_size,
                                           int fontstyle,
                                           int flag,
                                           void** pFont) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpStatus status =
      CallFunc(GdipCreateFont)((GpFontFamily*)pFamily, font_size, fontstyle,
                               Gdiplus::Unit(flag), (GpFont**)pFont);
  return status == Gdiplus::Ok;
}
void CGdiplusExt::GdipGetFontSize(void* pFont, float* size) {
  REAL get_size;
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpStatus status = CallFunc(GdipGetFontSize)((GpFont*)pFont, &get_size);
  *size = (status == Gdiplus::Ok) ? static_cast<float>(get_size) : 0;
}
void CGdiplusExt::GdipSetTextRenderingHint(void* graphics, int mode) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipSetTextRenderingHint)((GpGraphics*)graphics,
                                     (TextRenderingHint)mode);
}
void CGdiplusExt::GdipSetPageUnit(void* graphics, uint32_t unit) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipSetPageUnit)((GpGraphics*)graphics, (GpUnit)unit);
}
bool CGdiplusExt::GdipDrawDriverString(void* graphics,
                                       unsigned short* text,
                                       int length,
                                       void* font,
                                       void* brush,
                                       void* positions,
                                       int flags,
                                       const void* matrix) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpStatus status = CallFunc(GdipDrawDriverString)(
      (GpGraphics*)graphics, (GDIPCONST UINT16*)text, (INT)length,
      (GDIPCONST GpFont*)font, (GDIPCONST GpBrush*)brush,
      (GDIPCONST Gdiplus::PointF*)positions, (INT)flags,
      (GDIPCONST GpMatrix*)matrix);
  return status == Gdiplus::Ok;
}
void CGdiplusExt::GdipCreateBrush(uint32_t fill_argb, void** pBrush) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipCreateSolidFill)((Gdiplus::ARGB)fill_argb,
                                (GpSolidFill**)pBrush);
}
void CGdiplusExt::GdipDeleteBrush(void* pBrush) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipDeleteBrush)((GpSolidFill*)pBrush);
}
void* CGdiplusExt::GdipCreateFontFromCollection(void* pFontCollection,
                                                float font_size,
                                                int fontstyle) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  int numFamilies = 0;
  GpStatus status = CallFunc(GdipGetFontCollectionFamilyCount)(
      (GpFontCollection*)pFontCollection, &numFamilies);
  if (status != Gdiplus::Ok)
    return nullptr;

  GpFontFamily* family_list[1];
  status = CallFunc(GdipGetFontCollectionFamilyList)(
      (GpFontCollection*)pFontCollection, 1, family_list, &numFamilies);
  if (status != Gdiplus::Ok)
    return nullptr;

  GpFont* pFont = nullptr;
  status = CallFunc(GdipCreateFont)(family_list[0], font_size, fontstyle,
                                    UnitPixel, &pFont);
  if (status != Gdiplus::Ok)
    return nullptr;

  return pFont;
}
void CGdiplusExt::GdipCreateMatrix(float a,
                                   float b,
                                   float c,
                                   float d,
                                   float e,
                                   float f,
                                   void** matrix) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipCreateMatrix2)(a, b, c, d, e, f, (GpMatrix**)matrix);
}
void CGdiplusExt::GdipDeleteMatrix(void* matrix) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipDeleteMatrix)((GpMatrix*)matrix);
}
void CGdiplusExt::GdipDeleteFontFamily(void* pFamily) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipDeleteFontFamily)((GpFontFamily*)pFamily);
}
void CGdiplusExt::GdipDeleteFont(void* pFont) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipDeleteFont)((GpFont*)pFont);
}
void CGdiplusExt::GdipSetWorldTransform(void* graphics, void* pMatrix) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipSetWorldTransform)((GpGraphics*)graphics, (GpMatrix*)pMatrix);
}
void CGdiplusExt::GdipDisposeImage(void* bitmap) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipDisposeImage)((GpBitmap*)bitmap);
}
void CGdiplusExt::GdipDeleteGraphics(void* graphics) {
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipDeleteGraphics)((GpGraphics*)graphics);
}
bool CGdiplusExt::StretchBitMask(HDC hDC,
                                 BOOL bMonoDevice,
                                 const RetainPtr<CFX_DIBitmap>& pBitmap,
                                 int dest_left,
                                 int dest_top,
                                 int dest_width,
                                 int dest_height,
                                 uint32_t argb,
                                 const FX_RECT* pClipRect,
                                 int flags) {
  ASSERT(pBitmap->GetBPP() == 1);
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  GpGraphics* pGraphics = nullptr;
  CallFunc(GdipCreateFromHDC)(hDC, &pGraphics);
  CallFunc(GdipSetPageUnit)(pGraphics, UnitPixel);
  if (flags & FXDIB_NOSMOOTH) {
    CallFunc(GdipSetInterpolationMode)(pGraphics,
                                       InterpolationModeNearestNeighbor);
  } else {
    CallFunc(GdipSetInterpolationMode)(pGraphics, InterpolationModeHighQuality);
  }
  OutputImageMask(pGraphics, bMonoDevice, pBitmap, dest_left, dest_top,
                  dest_width, dest_height, argb, pClipRect);
  CallFunc(GdipDeleteGraphics)(pGraphics);
  return true;
}
bool CGdiplusExt::StretchDIBits(HDC hDC,
                                const RetainPtr<CFX_DIBitmap>& pBitmap,
                                int dest_left,
                                int dest_top,
                                int dest_width,
                                int dest_height,
                                const FX_RECT* pClipRect,
                                int flags) {
  GpGraphics* pGraphics;
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipCreateFromHDC)(hDC, &pGraphics);
  CallFunc(GdipSetPageUnit)(pGraphics, UnitPixel);
  if (flags & FXDIB_NOSMOOTH) {
    CallFunc(GdipSetInterpolationMode)(pGraphics,
                                       InterpolationModeNearestNeighbor);
  } else if (pBitmap->GetWidth() > abs(dest_width) / 2 ||
             pBitmap->GetHeight() > abs(dest_height) / 2) {
    CallFunc(GdipSetInterpolationMode)(pGraphics, InterpolationModeHighQuality);
  } else {
    CallFunc(GdipSetInterpolationMode)(pGraphics, InterpolationModeBilinear);
  }
  FX_RECT src_rect(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
  OutputImage(pGraphics, pBitmap, &src_rect, dest_left, dest_top, dest_width,
              dest_height);
  CallFunc(GdipDeleteGraphics)(pGraphics);
  CallFunc(GdipDeleteGraphics)(pGraphics);
  return true;
}

bool CGdiplusExt::DrawPath(HDC hDC,
                           const CFX_PathData* pPathData,
                           const CFX_Matrix* pObject2Device,
                           const CFX_GraphStateData* pGraphState,
                           uint32_t fill_argb,
                           uint32_t stroke_argb,
                           int fill_mode) {
  auto& pPoints = pPathData->GetPoints();
  if (pPoints.empty())
    return true;

  GpGraphics* pGraphics = nullptr;
  CGdiplusExt& GdiplusExt =
      ((CWin32Platform*)CFX_GEModule::Get()->GetPlatformData())->m_GdiplusExt;
  CallFunc(GdipCreateFromHDC)(hDC, &pGraphics);
  CallFunc(GdipSetPageUnit)(pGraphics, UnitPixel);
  CallFunc(GdipSetPixelOffsetMode)(pGraphics, PixelOffsetModeHalf);
  GpMatrix* pMatrix = nullptr;
  if (pObject2Device) {
    CallFunc(GdipCreateMatrix2)(pObject2Device->a, pObject2Device->b,
                                pObject2Device->c, pObject2Device->d,
                                pObject2Device->e, pObject2Device->f, &pMatrix);
    CallFunc(GdipSetWorldTransform)(pGraphics, pMatrix);
  }
  Gdiplus::PointF* points = FX_Alloc(Gdiplus::PointF, pPoints.size());
  BYTE* types = FX_Alloc(BYTE, pPoints.size());
  int nSubPathes = 0;
  bool bSubClose = false;
  int pos_subclose = 0;
  bool bSmooth = false;
  int startpoint = 0;
  for (size_t i = 0; i < pPoints.size(); i++) {
    points[i].X = pPoints[i].m_Point.x;
    points[i].Y = pPoints[i].m_Point.y;

    CFX_PointF pos = pPoints[i].m_Point;
    if (pObject2Device)
      pos = pObject2Device->Transform(pos);

    if (pos.x > 50000 * 1.0f)
      points[i].X = 50000 * 1.0f;
    if (pos.x < -50000 * 1.0f)
      points[i].X = -50000 * 1.0f;
    if (pos.y > 50000 * 1.0f)
      points[i].Y = 50000 * 1.0f;
    if (pos.y < -50000 * 1.0f)
      points[i].Y = -50000 * 1.0f;

    FXPT_TYPE point_type = pPoints[i].m_Type;
    if (point_type == FXPT_TYPE::MoveTo) {
      types[i] = PathPointTypeStart;
      nSubPathes++;
      bSubClose = false;
      startpoint = i;
    } else if (point_type == FXPT_TYPE::LineTo) {
      types[i] = PathPointTypeLine;
      if (pPoints[i - 1].IsTypeAndOpen(FXPT_TYPE::MoveTo) &&
          (i == pPoints.size() - 1 ||
           pPoints[i + 1].IsTypeAndOpen(FXPT_TYPE::MoveTo)) &&
          points[i].Y == points[i - 1].Y && points[i].X == points[i - 1].X) {
        points[i].X += 0.01f;
        continue;
      }
      if (!bSmooth && points[i].X != points[i - 1].X &&
          points[i].Y != points[i - 1].Y)
        bSmooth = true;
    } else if (point_type == FXPT_TYPE::BezierTo) {
      types[i] = PathPointTypeBezier;
      bSmooth = true;
    }
    if (pPoints[i].m_CloseFigure) {
      if (bSubClose)
        types[pos_subclose] &= ~PathPointTypeCloseSubpath;
      else
        bSubClose = true;
      pos_subclose = i;
      types[i] |= PathPointTypeCloseSubpath;
      if (!bSmooth && points[i].X != points[startpoint].X &&
          points[i].Y != points[startpoint].Y)
        bSmooth = true;
    }
  }
  if (fill_mode & FXFILL_NOPATHSMOOTH) {
    bSmooth = false;
    CallFunc(GdipSetSmoothingMode)(pGraphics, SmoothingModeNone);
  } else if (!(fill_mode & FXFILL_FULLCOVER)) {
    if (!bSmooth && (fill_mode & 3))
      bSmooth = true;

    if (bSmooth || (pGraphState && pGraphState->m_LineWidth > 2))
      CallFunc(GdipSetSmoothingMode)(pGraphics, SmoothingModeAntiAlias);
  }
  int new_fill_mode = fill_mode & 3;
  if (pPoints.size() == 4 && !pGraphState) {
    int v1, v2;
    if (IsSmallTriangle(points, pObject2Device, v1, v2)) {
      GpPen* pPen = nullptr;
      CallFunc(GdipCreatePen1)(fill_argb, 1.0f, UnitPixel, &pPen);
      CallFunc(GdipDrawLineI)(
          pGraphics, pPen, FXSYS_round(points[v1].X), FXSYS_round(points[v1].Y),
          FXSYS_round(points[v2].X), FXSYS_round(points[v2].Y));
      CallFunc(GdipDeletePen)(pPen);
      return true;
    }
  }
  GpPath* pGpPath = nullptr;
  CallFunc(GdipCreatePath2)(points, types, pPoints.size(),
                            GdiFillType2Gdip(new_fill_mode), &pGpPath);
  if (!pGpPath) {
    if (pMatrix)
      CallFunc(GdipDeleteMatrix)(pMatrix);

    FX_Free(points);
    FX_Free(types);
    CallFunc(GdipDeleteGraphics)(pGraphics);
    return false;
  }
  if (new_fill_mode) {
    GpBrush* pBrush = GdipCreateBrushImpl(fill_argb);
    CallFunc(GdipSetPathFillMode)(pGpPath, GdiFillType2Gdip(new_fill_mode));
    CallFunc(GdipFillPath)(pGraphics, pBrush, pGpPath);
    CallFunc(GdipDeleteBrush)(pBrush);
  }
  if (pGraphState && stroke_argb) {
    GpPen* pPen = GdipCreatePenImpl(pGraphState, pObject2Device, stroke_argb,
                                    !!(fill_mode & FX_STROKE_TEXT_MODE));
    if (nSubPathes == 1) {
      CallFunc(GdipDrawPath)(pGraphics, pPen, pGpPath);
    } else {
      int iStart = 0;
      for (size_t i = 0; i < pPoints.size(); i++) {
        if (i == pPoints.size() - 1 || types[i + 1] == PathPointTypeStart) {
          GpPath* pSubPath;
          CallFunc(GdipCreatePath2)(points + iStart, types + iStart,
                                    i - iStart + 1,
                                    GdiFillType2Gdip(new_fill_mode), &pSubPath);
          iStart = i + 1;
          CallFunc(GdipDrawPath)(pGraphics, pPen, pSubPath);
          CallFunc(GdipDeletePath)(pSubPath);
        }
      }
    }
    CallFunc(GdipDeletePen)(pPen);
  }
  if (pMatrix)
    CallFunc(GdipDeleteMatrix)(pMatrix);
  FX_Free(points);
  FX_Free(types);
  CallFunc(GdipDeletePath)(pGpPath);
  CallFunc(GdipDeleteGraphics)(pGraphics);
  return true;
}

RetainPtr<CFX_DIBitmap> CGdiplusExt::LoadDIBitmap(WINDIB_Open_Args_ args) {
  PREVIEW3_DIBITMAP* pInfo = ::LoadDIBitmap(args);
  if (!pInfo)
    return nullptr;

  int height = abs(pInfo->pbmi->bmiHeader.biHeight);
  int width = pInfo->pbmi->bmiHeader.biWidth;
  int dest_pitch = (width * pInfo->pbmi->bmiHeader.biBitCount + 31) / 32 * 4;
  LPBYTE pData = FX_Alloc2D(BYTE, dest_pitch, height);
  if (dest_pitch == pInfo->Stride) {
    memcpy(pData, pInfo->pScan0, dest_pitch * height);
  } else {
    for (int i = 0; i < height; i++) {
      memcpy(pData + dest_pitch * i, pInfo->pScan0 + pInfo->Stride * i,
             dest_pitch);
    }
  }
  RetainPtr<CFX_DIBitmap> pDIBitmap = FX_WindowsDIB_LoadFromBuf(
      pInfo->pbmi, pData, pInfo->pbmi->bmiHeader.biBitCount == 32);
  FX_Free(pData);
  FreeDIBitmap(pInfo);
  return pDIBitmap;
}
