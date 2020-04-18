// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_CBC_UPCA_H_
#define FXBARCODE_CBC_UPCA_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxge/fx_dib.h"
#include "fxbarcode/cbc_onecode.h"

class CBC_OnedUPCAWriter;

class CBC_UPCA : public CBC_OneCode {
 public:
  CBC_UPCA();
  ~CBC_UPCA() override;

  // CBC_CodeBase:
  bool Encode(const WideStringView& contents) override;
  bool RenderDevice(CFX_RenderDevice* device,
                    const CFX_Matrix* matrix) override;
  BC_TYPE GetType() override;

 private:
  CBC_OnedUPCAWriter* GetOnedUPCAWriter();
  WideString Preprocess(const WideStringView& contents);
  WideString m_renderContents;
};

#endif  // FXBARCODE_CBC_UPCA_H_
