// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_CONTENTMARKITEM_H_
#define CORE_FPDFAPI_PAGE_CPDF_CONTENTMARKITEM_H_

#include <memory>

#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_Dictionary;

class CPDF_ContentMarkItem {
 public:
  enum ParamType { None, PropertiesDict, DirectDict };

  CPDF_ContentMarkItem();
  CPDF_ContentMarkItem(const CPDF_ContentMarkItem& that);
  ~CPDF_ContentMarkItem();

  CPDF_ContentMarkItem& operator=(CPDF_ContentMarkItem&& other) = default;

  ByteString GetName() const { return m_MarkName; }
  ParamType GetParamType() const { return m_ParamType; }
  const CPDF_Dictionary* GetParam() const;
  bool HasMCID() const;

  void SetName(const ByteString& name) { m_MarkName = name; }
  void SetDirectDict(std::unique_ptr<CPDF_Dictionary> pDict);
  void SetPropertiesDict(const CPDF_Dictionary* pDict);

 private:
  ByteString m_MarkName;
  ParamType m_ParamType = None;
  UnownedPtr<const CPDF_Dictionary> m_pPropertiesDict;
  std::unique_ptr<const CPDF_Dictionary> m_pDirectDict;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_CONTENTMARKITEM_H_
