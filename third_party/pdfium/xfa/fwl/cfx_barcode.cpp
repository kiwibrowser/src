// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfx_barcode.h"

#include <memory>

#include "fxbarcode/cbc_codabar.h"
#include "fxbarcode/cbc_code128.h"
#include "fxbarcode/cbc_code39.h"
#include "fxbarcode/cbc_codebase.h"
#include "fxbarcode/cbc_datamatrix.h"
#include "fxbarcode/cbc_ean13.h"
#include "fxbarcode/cbc_ean8.h"
#include "fxbarcode/cbc_pdf417i.h"
#include "fxbarcode/cbc_qrcode.h"
#include "fxbarcode/cbc_upca.h"
#include "fxbarcode/utils.h"
#include "third_party/base/ptr_util.h"

namespace {

std::unique_ptr<CBC_CodeBase> CreateBarCodeEngineObject(BC_TYPE type) {
  switch (type) {
    case BC_CODE39:
      return pdfium::MakeUnique<CBC_Code39>();
    case BC_CODABAR:
      return pdfium::MakeUnique<CBC_Codabar>();
    case BC_CODE128:
      return pdfium::MakeUnique<CBC_Code128>(BC_CODE128_B);
    case BC_CODE128_B:
      return pdfium::MakeUnique<CBC_Code128>(BC_CODE128_B);
    case BC_CODE128_C:
      return pdfium::MakeUnique<CBC_Code128>(BC_CODE128_C);
    case BC_EAN8:
      return pdfium::MakeUnique<CBC_EAN8>();
    case BC_UPCA:
      return pdfium::MakeUnique<CBC_UPCA>();
    case BC_EAN13:
      return pdfium::MakeUnique<CBC_EAN13>();
    case BC_QR_CODE:
      return pdfium::MakeUnique<CBC_QRCode>();
    case BC_PDF417:
      return pdfium::MakeUnique<CBC_PDF417I>();
    case BC_DATAMATRIX:
      return pdfium::MakeUnique<CBC_DataMatrix>();
    case BC_UNKNOWN:
    default:
      return nullptr;
  }
}

}  // namespace

CFX_Barcode::CFX_Barcode() {}

CFX_Barcode::~CFX_Barcode() {}

std::unique_ptr<CFX_Barcode> CFX_Barcode::Create(BC_TYPE type) {
  auto barcodeEngine = CreateBarCodeEngineObject(type);
  std::unique_ptr<CFX_Barcode> barcode(new CFX_Barcode());
  barcode->m_pBCEngine.swap(barcodeEngine);
  return barcode;
}

BC_TYPE CFX_Barcode::GetType() {
  return m_pBCEngine ? m_pBCEngine->GetType() : BC_UNKNOWN;
}

bool CFX_Barcode::SetCharEncoding(BC_CHAR_ENCODING encoding) {
  return m_pBCEngine ? m_pBCEngine->SetCharEncoding(encoding) : false;
}

bool CFX_Barcode::SetModuleHeight(int32_t moduleHeight) {
  return m_pBCEngine ? m_pBCEngine->SetModuleHeight(moduleHeight) : false;
}

bool CFX_Barcode::SetModuleWidth(int32_t moduleWidth) {
  return m_pBCEngine ? m_pBCEngine->SetModuleWidth(moduleWidth) : false;
}

bool CFX_Barcode::SetHeight(int32_t height) {
  return m_pBCEngine ? m_pBCEngine->SetHeight(height) : false;
}

bool CFX_Barcode::SetWidth(int32_t width) {
  return m_pBCEngine ? m_pBCEngine->SetWidth(width) : false;
}

bool CFX_Barcode::SetPrintChecksum(bool checksum) {
  switch (GetType()) {
    case BC_CODE39:
    case BC_CODABAR:
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
    case BC_EAN8:
    case BC_EAN13:
    case BC_UPCA:
      return m_pBCEngine ? (static_cast<CBC_OneCode*>(m_pBCEngine.get())
                                ->SetPrintChecksum(checksum),
                            true)
                         : false;
    default:
      return false;
  }
}

bool CFX_Barcode::SetDataLength(int32_t length) {
  switch (GetType()) {
    case BC_CODE39:
    case BC_CODABAR:
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
    case BC_EAN8:
    case BC_EAN13:
    case BC_UPCA:
      return m_pBCEngine ? (static_cast<CBC_OneCode*>(m_pBCEngine.get())
                                ->SetDataLength(length),
                            true)
                         : false;
    default:
      return false;
  }
}

bool CFX_Barcode::SetCalChecksum(bool state) {
  switch (GetType()) {
    case BC_CODE39:
    case BC_CODABAR:
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
    case BC_EAN8:
    case BC_EAN13:
    case BC_UPCA:
      return m_pBCEngine ? (static_cast<CBC_OneCode*>(m_pBCEngine.get())
                                ->SetCalChecksum(state),
                            true)
                         : false;
    default:
      return false;
  }
}

bool CFX_Barcode::SetFont(CFX_Font* pFont) {
  switch (GetType()) {
    case BC_CODE39:
    case BC_CODABAR:
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
    case BC_EAN8:
    case BC_EAN13:
    case BC_UPCA:
      return m_pBCEngine
                 ? static_cast<CBC_OneCode*>(m_pBCEngine.get())->SetFont(pFont)
                 : false;
    default:
      return false;
  }
}

bool CFX_Barcode::SetFontSize(float size) {
  switch (GetType()) {
    case BC_CODE39:
    case BC_CODABAR:
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
    case BC_EAN8:
    case BC_EAN13:
    case BC_UPCA:
      return m_pBCEngine ? (static_cast<CBC_OneCode*>(m_pBCEngine.get())
                                ->SetFontSize(size),
                            true)
                         : false;
    default:
      return false;
  }
}

bool CFX_Barcode::SetFontColor(FX_ARGB color) {
  switch (GetType()) {
    case BC_CODE39:
    case BC_CODABAR:
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
    case BC_EAN8:
    case BC_EAN13:
    case BC_UPCA:
      return m_pBCEngine ? (static_cast<CBC_OneCode*>(m_pBCEngine.get())
                                ->SetFontColor(color),
                            true)
                         : false;
    default:
      return false;
  }
}

bool CFX_Barcode::SetTextLocation(BC_TEXT_LOC location) {
  typedef bool (CBC_CodeBase::*memptrtype)(BC_TEXT_LOC);
  memptrtype memptr = nullptr;
  switch (GetType()) {
    case BC_CODE39:
      memptr = (memptrtype)&CBC_Code39::SetTextLocation;
      break;
    case BC_CODABAR:
      memptr = (memptrtype)&CBC_Codabar::SetTextLocation;
      break;
    case BC_CODE128:
    case BC_CODE128_B:
    case BC_CODE128_C:
      memptr = (memptrtype)&CBC_Code128::SetTextLocation;
      break;
    default:
      break;
  }
  return m_pBCEngine && memptr ? (m_pBCEngine.get()->*memptr)(location) : false;
}

bool CFX_Barcode::SetWideNarrowRatio(int8_t ratio) {
  typedef bool (CBC_CodeBase::*memptrtype)(int8_t);
  memptrtype memptr = nullptr;
  switch (GetType()) {
    case BC_CODE39:
      memptr = (memptrtype)&CBC_Code39::SetWideNarrowRatio;
      break;
    case BC_CODABAR:
      memptr = (memptrtype)&CBC_Codabar::SetWideNarrowRatio;
      break;
    default:
      break;
  }
  return m_pBCEngine && memptr ? (m_pBCEngine.get()->*memptr)(ratio) : false;
}

bool CFX_Barcode::SetStartChar(char start) {
  typedef bool (CBC_CodeBase::*memptrtype)(char);
  memptrtype memptr = nullptr;
  switch (GetType()) {
    case BC_CODABAR:
      memptr = (memptrtype)&CBC_Codabar::SetStartChar;
      break;
    default:
      break;
  }
  return m_pBCEngine && memptr ? (m_pBCEngine.get()->*memptr)(start) : false;
}

bool CFX_Barcode::SetEndChar(char end) {
  typedef bool (CBC_CodeBase::*memptrtype)(char);
  memptrtype memptr = nullptr;
  switch (GetType()) {
    case BC_CODABAR:
      memptr = (memptrtype)&CBC_Codabar::SetEndChar;
      break;
    default:
      break;
  }
  return m_pBCEngine && memptr ? (m_pBCEngine.get()->*memptr)(end) : false;
}

bool CFX_Barcode::SetErrorCorrectionLevel(int32_t level) {
  typedef bool (CBC_CodeBase::*memptrtype)(int32_t);
  memptrtype memptr = nullptr;
  switch (GetType()) {
    case BC_QR_CODE:
      memptr = (memptrtype)&CBC_QRCode::SetErrorCorrectionLevel;
      break;
    case BC_PDF417:
      memptr = (memptrtype)&CBC_PDF417I::SetErrorCorrectionLevel;
      break;
    default:
      return false;
  }
  return m_pBCEngine && memptr ? (m_pBCEngine.get()->*memptr)(level) : false;
}

bool CFX_Barcode::SetTruncated(bool truncated) {
  typedef void (CBC_CodeBase::*memptrtype)(bool);
  memptrtype memptr = nullptr;
  switch (GetType()) {
    case BC_PDF417:
      memptr = (memptrtype)&CBC_PDF417I::SetTruncated;
      break;
    default:
      break;
  }
  return m_pBCEngine && memptr ? ((m_pBCEngine.get()->*memptr)(truncated), true)
                               : false;
}

bool CFX_Barcode::Encode(const WideStringView& contents) {
  return m_pBCEngine && m_pBCEngine->Encode(contents);
}

bool CFX_Barcode::RenderDevice(CFX_RenderDevice* device,
                               const CFX_Matrix* matrix) {
  return m_pBCEngine && m_pBCEngine->RenderDevice(device, matrix);
}
