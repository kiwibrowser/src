// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_EDIT_CPDF_CREATOR_H_
#define CORE_FPDFAPI_EDIT_CPDF_CREATOR_H_

#include <map>
#include <memory>
#include <vector>

#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/maybe_owned.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_Array;
class CPDF_CryptoHandler;
class CPDF_SecurityHandler;
class CPDF_Dictionary;
class CPDF_Document;
class CPDF_Object;
class CPDF_Parser;

#define FPDFCREATE_INCREMENTAL 1
#define FPDFCREATE_NO_ORIGINAL 2

class CPDF_Creator {
 public:
  explicit CPDF_Creator(CPDF_Document* pDoc,
                        const RetainPtr<IFX_WriteStream>& archive);
  ~CPDF_Creator();

  void RemoveSecurity();
  bool Create(uint32_t flags);
  int32_t Continue();
  bool SetFileVersion(int32_t fileVersion);

  IFX_ArchiveStream* GetArchive() { return m_Archive.get(); }

  uint32_t GetNextObjectNumber() { return ++m_dwLastObjNum; }
  uint32_t GetLastObjectNumber() const { return m_dwLastObjNum; }
  CPDF_CryptoHandler* GetCryptoHandler();
  CPDF_Document* GetDocument() const { return m_pDocument.Get(); }
  CPDF_Array* GetIDArray() const { return m_pIDArray.get(); }
  CPDF_Dictionary* GetEncryptDict() const { return m_pEncryptDict.Get(); }
  uint32_t GetEncryptObjectNumber() const { return m_dwEncryptObjNum; }

  uint32_t GetObjectOffset(uint32_t objnum) { return m_ObjectOffsets[objnum]; }
  bool HasObjectNumber(uint32_t objnum) {
    return m_ObjectOffsets.find(objnum) != m_ObjectOffsets.end();
  }
  void SetObjectOffset(uint32_t objnum, FX_FILESIZE offset) {
    m_ObjectOffsets[objnum] = offset;
  }
  bool IsIncremental() const { return !!(m_dwFlags & FPDFCREATE_INCREMENTAL); }
  bool IsOriginal() const { return !(m_dwFlags & FPDFCREATE_NO_ORIGINAL); }

 private:
  void Clear();

  void InitOldObjNumOffsets();
  void InitNewObjNumOffsets();
  void InitID();

  int32_t WriteDoc_Stage1();
  int32_t WriteDoc_Stage2();
  int32_t WriteDoc_Stage3();
  int32_t WriteDoc_Stage4();

  bool WriteOldIndirectObject(uint32_t objnum);
  bool WriteOldObjs();
  bool WriteNewObjs();
  bool WriteDirectObj(uint32_t objnum, const CPDF_Object* pObj, bool bEncrypt);
  bool WriteIndirectObj(uint32_t objnum, const CPDF_Object* pObj);

  bool WriteStream(const CPDF_Object* pStream,
                   uint32_t objnum,
                   CPDF_CryptoHandler* pCrypto);

  bool IsXRefNeedEnd();

  UnownedPtr<CPDF_Document> const m_pDocument;
  UnownedPtr<CPDF_Parser> const m_pParser;
  bool m_bSecurityChanged;
  UnownedPtr<CPDF_Dictionary> m_pEncryptDict;
  uint32_t m_dwEncryptObjNum;
  fxcrt::MaybeOwned<CPDF_SecurityHandler> m_pSecurityHandler;
  UnownedPtr<CPDF_Object> m_pMetadata;
  uint32_t m_dwLastObjNum;
  std::unique_ptr<IFX_ArchiveStream> m_Archive;
  FX_FILESIZE m_SavedOffset;
  int32_t m_iStage;
  uint32_t m_dwFlags;
  uint32_t m_CurObjNum;
  FX_FILESIZE m_XrefStart;
  std::map<uint32_t, FX_FILESIZE> m_ObjectOffsets;
  std::vector<uint32_t> m_NewObjNumArray;  // Sorted, ascending.
  std::unique_ptr<CPDF_Array> m_pIDArray;
  int32_t m_FileVersion;
};

#endif  // CORE_FPDFAPI_EDIT_CPDF_CREATOR_H_
