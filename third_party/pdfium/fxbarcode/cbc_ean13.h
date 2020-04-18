// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_CBC_EAN13_H_
#define FXBARCODE_CBC_EAN13_H_

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxge/fx_dib.h"
#include "fxbarcode/cbc_onecode.h"

class CBC_OnedEAN13Writer;

class CBC_EAN13 : public CBC_OneCode {
 public:
  CBC_EAN13();
  ~CBC_EAN13() override;

  // CBC_OneCode:
  bool Encode(const WideStringView& contents) override;
  bool RenderDevice(CFX_RenderDevice* device,
                    const CFX_Matrix* matrix) override;
  BC_TYPE GetType() override;

 private:
  CBC_OnedEAN13Writer* GetOnedEAN13Writer();
  WideString Preprocess(const WideStringView& contents);

  WideString m_renderContents;
};

#endif  // FXBARCODE_CBC_EAN13_H_
