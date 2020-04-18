// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_CBC_EAN8_H_
#define FXBARCODE_CBC_EAN8_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxge/fx_dib.h"
#include "fxbarcode/cbc_onecode.h"

class CBC_OnedEAN8Writer;

class CBC_EAN8 : public CBC_OneCode {
 public:
  CBC_EAN8();
  ~CBC_EAN8() override;

  // CBC_OneCode:
  bool Encode(const WideStringView& contents) override;
  bool RenderDevice(CFX_RenderDevice* device,
                    const CFX_Matrix* matrix) override;
  BC_TYPE GetType() override;

 private:
  CBC_OnedEAN8Writer* GetOnedEAN8Writer();
  WideString Preprocess(const WideStringView& contents);
  WideString m_renderContents;
};

#endif  // FXBARCODE_CBC_EAN8_H_
