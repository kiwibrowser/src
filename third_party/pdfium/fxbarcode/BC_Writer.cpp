// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxbarcode/BC_Writer.h"

CBC_Writer::CBC_Writer() {
  m_CharEncoding = 0;
  m_ModuleHeight = 1;
  m_ModuleWidth = 1;
  m_Height = 320;
  m_Width = 640;
  m_colorSpace = FXDIB_Argb;
  m_barColor = 0xff000000;
  m_backgroundColor = 0xffffffff;
}
CBC_Writer::~CBC_Writer() {}
bool CBC_Writer::SetCharEncoding(int32_t encoding) {
  m_CharEncoding = encoding;
  return true;
}
bool CBC_Writer::SetModuleHeight(int32_t moduleHeight) {
  if (moduleHeight > 10 || moduleHeight < 1) {
    return false;
  }
  m_ModuleHeight = moduleHeight;
  return true;
}
bool CBC_Writer::SetModuleWidth(int32_t moduleWidth) {
  if (moduleWidth > 10 || moduleWidth < 1) {
    return false;
  }
  m_ModuleWidth = moduleWidth;
  return true;
}
bool CBC_Writer::SetHeight(int32_t height) {
  m_Height = height;
  return true;
}
bool CBC_Writer::SetWidth(int32_t width) {
  m_Width = width;
  return true;
}
void CBC_Writer::SetBackgroundColor(FX_ARGB backgroundColor) {
  m_backgroundColor = backgroundColor;
}
void CBC_Writer::SetBarcodeColor(FX_ARGB foregroundColor) {
  m_barColor = foregroundColor;
}

RetainPtr<CFX_DIBitmap> CBC_Writer::CreateDIBitmap(int32_t width,
                                                   int32_t height) {
  auto pDIBitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  pDIBitmap->Create(width, height, m_colorSpace);
  return pDIBitmap;
}
