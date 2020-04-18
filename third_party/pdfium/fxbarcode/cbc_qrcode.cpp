// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
 * Copyright 2011 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fxbarcode/cbc_qrcode.h"

#include <memory>

#include "fxbarcode/qrcode/BC_QRCodeWriter.h"
#include "third_party/base/ptr_util.h"

CBC_QRCode::CBC_QRCode()
    : CBC_CodeBase(pdfium::MakeUnique<CBC_QRCodeWriter>()) {}

CBC_QRCode::~CBC_QRCode() {}

bool CBC_QRCode::SetErrorCorrectionLevel(int32_t level) {
  if (level < 0 || level > 3)
    return false;
  return GetQRCodeWriter()->SetErrorCorrectionLevel(level);
}

bool CBC_QRCode::Encode(const WideStringView& contents) {
  int32_t outWidth = 0;
  int32_t outHeight = 0;
  CBC_QRCodeWriter* pWriter = GetQRCodeWriter();
  std::unique_ptr<uint8_t, FxFreeDeleter> data(
      pWriter->Encode(WideString(contents), pWriter->GetErrorCorrectionLevel(),
                      outWidth, outHeight));
  if (!data)
    return false;
  return pWriter->RenderResult(data.get(), outWidth, outHeight);
}

bool CBC_QRCode::RenderDevice(CFX_RenderDevice* device,
                              const CFX_Matrix* matrix) {
  GetQRCodeWriter()->RenderDeviceResult(device, matrix);
  return true;
}

BC_TYPE CBC_QRCode::GetType() {
  return BC_QR_CODE;
}

CBC_QRCodeWriter* CBC_QRCode::GetQRCodeWriter() {
  return static_cast<CBC_QRCodeWriter*>(m_pBCWriter.get());
}
