// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXGRAPHICS_CXFA_GRAPHICS_H_
#define XFA_FXGRAPHICS_CXFA_GRAPHICS_H_

#include <memory>
#include <vector>

#include "core/fxcrt/fx_system.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/fx_dib.h"
#include "xfa/fxgraphics/cxfa_gecolor.h"

class CXFA_GEPath;

using FX_FillMode = int32_t;

enum class FX_HatchStyle {
  Horizontal = 0,
  Vertical = 1,
  ForwardDiagonal = 2,
  BackwardDiagonal = 3,
  Cross = 4,
  DiagonalCross = 5
};

class CFX_RenderDevice;

class CXFA_Graphics {
 public:
  explicit CXFA_Graphics(CFX_RenderDevice* renderDevice);
  ~CXFA_Graphics();

  void SaveGraphState();
  void RestoreGraphState();

  CFX_RectF GetClipRect() const;
  const CFX_Matrix* GetMatrix() const;
  CFX_RenderDevice* GetRenderDevice();

  void SetLineCap(CFX_GraphStateData::LineCap lineCap);
  void SetLineDash(float dashPhase, float* dashArray, int32_t dashCount);
  void SetSolidLineDash();
  void SetLineWidth(float lineWidth);
  void EnableActOnDash();
  void SetStrokeColor(const CXFA_GEColor& color);
  void SetFillColor(const CXFA_GEColor& color);
  void SetClipRect(const CFX_RectF& rect);
  void StrokePath(CXFA_GEPath* path, const CFX_Matrix* matrix);
  void FillPath(CXFA_GEPath* path,
                FX_FillMode fillMode,
                const CFX_Matrix* matrix);
  void ConcatMatrix(const CFX_Matrix* matrix);

 protected:
  int32_t m_type;

 private:
  struct TInfo {
    TInfo();
    explicit TInfo(const TInfo& info);
    TInfo& operator=(const TInfo& other);

    CFX_GraphStateData graphState;
    CFX_Matrix CTM;
    bool isActOnDash;
    CXFA_GEColor strokeColor;
    CXFA_GEColor fillColor;
  } m_info;

  void RenderDeviceStrokePath(const CXFA_GEPath* path,
                              const CFX_Matrix* matrix);
  void RenderDeviceFillPath(const CXFA_GEPath* path,
                            FX_FillMode fillMode,
                            const CFX_Matrix* matrix);

  void FillPathWithPattern(const CXFA_GEPath* path,
                           FX_FillMode fillMode,
                           const CFX_Matrix& matrix);
  void FillPathWithShading(const CXFA_GEPath* path,
                           FX_FillMode fillMode,
                           const CFX_Matrix& matrix);

  void SetDIBitsWithMatrix(const RetainPtr<CFX_DIBSource>& source,
                           const CFX_Matrix& matrix);

  CFX_RenderDevice* const m_renderDevice;  // Not owned.
  std::vector<std::unique_ptr<TInfo>> m_infoStack;
};

#endif  // XFA_FXGRAPHICS_CXFA_GRAPHICS_H_
