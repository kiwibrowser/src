// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_GLOBALDATA_H_
#define FXJS_CJS_GLOBALDATA_H_

#include <memory>
#include <vector>

#include "core/fxcrt/cfx_binarybuf.h"
#include "fxjs/cjs_keyvalue.h"

class CPDFSDK_FormFillEnvironment;

class CJS_GlobalData_Element {
 public:
  CJS_GlobalData_Element() {}
  ~CJS_GlobalData_Element() {}

  CJS_KeyValue data;
  bool bPersistent;
};

class CJS_GlobalData {
 public:
  static CJS_GlobalData* GetRetainedInstance(CPDFSDK_FormFillEnvironment* pApp);
  void Release();

  void SetGlobalVariableNumber(const ByteString& propname, double dData);
  void SetGlobalVariableBoolean(const ByteString& propname, bool bData);
  void SetGlobalVariableString(const ByteString& propname,
                               const ByteString& sData);
  void SetGlobalVariableObject(const ByteString& propname,
                               const CJS_GlobalVariableArray& array);
  void SetGlobalVariableNull(const ByteString& propname);
  bool SetGlobalVariablePersistent(const ByteString& propname,
                                   bool bPersistent);
  bool DeleteGlobalVariable(const ByteString& propname);

  int32_t GetSize() const;
  CJS_GlobalData_Element* GetAt(int index) const;

 private:
  using iterator =
      std::vector<std::unique_ptr<CJS_GlobalData_Element>>::iterator;
  using const_iterator =
      std::vector<std::unique_ptr<CJS_GlobalData_Element>>::const_iterator;

  CJS_GlobalData();
  ~CJS_GlobalData();

  void LoadGlobalPersistentVariables();
  void SaveGlobalPersisitentVariables();

  CJS_GlobalData_Element* GetGlobalVariable(const ByteString& sPropname);
  iterator FindGlobalVariable(const ByteString& sPropname);
  const_iterator FindGlobalVariable(const ByteString& sPropname) const;

  void LoadFileBuffer(const wchar_t* sFilePath,
                      uint8_t*& pBuffer,
                      int32_t& nLength);
  void WriteFileBuffer(const wchar_t* sFilePath,
                       const char* pBuffer,
                       int32_t nLength);
  void MakeByteString(const ByteString& name,
                      CJS_KeyValue* pData,
                      CFX_BinaryBuf& sData);

  size_t m_RefCount;
  std::vector<std::unique_ptr<CJS_GlobalData_Element>> m_arrayGlobalData;
  WideString m_sFilePath;
};

#endif  // FXJS_CJS_GLOBALDATA_H_
