// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_CBC_ONECODE_H_
#define FXBARCODE_CBC_ONECODE_H_

#include <memory>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "fxbarcode/cbc_codebase.h"

class CBC_OneDimWriter;
class CFX_Font;

class CBC_OneCode : public CBC_CodeBase {
 public:
  explicit CBC_OneCode(std::unique_ptr<CBC_Writer> pWriter);
  ~CBC_OneCode() override;

  virtual bool CheckContentValidity(const WideStringView& contents);
  virtual WideString FilterContents(const WideStringView& contents);

  virtual void SetPrintChecksum(bool checksum);
  virtual void SetDataLength(int32_t length);
  virtual void SetCalChecksum(bool calc);
  virtual bool SetFont(CFX_Font* cFont);
  virtual void SetFontSize(float size);
  virtual void SetFontStyle(int32_t style);
  virtual void SetFontColor(FX_ARGB color);

 private:
  CBC_OneDimWriter* GetOneDimWriter();
};

#endif  // FXBARCODE_CBC_ONECODE_H_
