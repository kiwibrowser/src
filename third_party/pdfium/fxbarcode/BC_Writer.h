// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_BC_WRITER_H_
#define FXBARCODE_BC_WRITER_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/fx_dib.h"
#include "fxbarcode/utils.h"

class CBC_Writer {
 public:
  CBC_Writer();
  virtual ~CBC_Writer();
  virtual bool SetCharEncoding(int32_t encoding);
  virtual bool SetModuleHeight(int32_t moduleHeight);
  virtual bool SetModuleWidth(int32_t moduleWidth);
  virtual bool SetHeight(int32_t height);
  virtual bool SetWidth(int32_t width);
  virtual void SetBackgroundColor(FX_ARGB backgroundColor);
  virtual void SetBarcodeColor(FX_ARGB foregroundColor);

 protected:
  RetainPtr<CFX_DIBitmap> CreateDIBitmap(int32_t width, int32_t height);
  int32_t m_CharEncoding;
  int32_t m_ModuleHeight;
  int32_t m_ModuleWidth;
  int32_t m_Height;
  int32_t m_Width;
  FXDIB_Format m_colorSpace;
  FX_ARGB m_barColor;
  FX_ARGB m_backgroundColor;
};

#endif  // FXBARCODE_BC_WRITER_H_
