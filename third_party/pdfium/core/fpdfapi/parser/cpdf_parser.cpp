// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_parser.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_crypto_handler.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_linearized_header.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_security_handler.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/cpdf_syntax_parser.h"
#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fxcrt/autorestorer.h"
#include "core/fxcrt/cfx_memorystream.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_safe_types.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"

namespace {

// A limit on the size of the xref table. Theoretical limits are higher, but
// this may be large enough in practice.
const int32_t kMaxXRefSize = 1048576;

// "%PDF-1.7\n"
constexpr FX_FILESIZE kPDFHeaderSize = 9;

uint32_t GetVarInt(const uint8_t* p, int32_t n) {
  uint32_t result = 0;
  for (int32_t i = 0; i < n; ++i)
    result = result * 256 + p[i];
  return result;
}

int32_t GetStreamNCount(const RetainPtr<CPDF_StreamAcc>& pObjStream) {
  return pObjStream->GetDict()->GetIntegerFor("N");
}

int32_t GetStreamFirst(const RetainPtr<CPDF_StreamAcc>& pObjStream) {
  return pObjStream->GetDict()->GetIntegerFor("First");
}

}  // namespace

class CPDF_Parser::TrailerData {
 public:
  TrailerData() {}
  ~TrailerData() {}

  CPDF_Dictionary* GetMainTrailer() const { return main_trailer_.get(); }

  std::unique_ptr<CPDF_Dictionary> GetCombinedTrailer() const {
    std::unique_ptr<CPDF_Dictionary> result =
        ToDictionary(main_trailer_->Clone());

    // Info is optional.
    uint32_t info_obj_num = GetInfoObjNum();
    if (info_obj_num > 0)
      result->SetNewFor<CPDF_Reference>("Info", nullptr, GetInfoObjNum());

    // Root is required.
    result->SetNewFor<CPDF_Reference>("Root", nullptr, GetRootObjNum());
    return result;
  }

  void SetMainTrailer(std::unique_ptr<CPDF_Dictionary> trailer) {
    ASSERT(trailer);
    main_trailer_ = std::move(trailer);
    ApplyTrailer(main_trailer_.get());
  }

  void AppendTrailer(std::unique_ptr<CPDF_Dictionary> trailer) {
    ASSERT(trailer);
    ApplyTrailer(trailer.get());
  }

  void Clear() {
    main_trailer_.reset();
    last_info_obj_num_ = 0;
    last_root_obj_num_ = 0;
  }

  uint32_t GetInfoObjNum() const {
    const CPDF_Reference* pRef = ToReference(
        GetMainTrailer() ? GetMainTrailer()->GetObjectFor("Info") : nullptr);
    return pRef ? pRef->GetRefObjNum() : last_info_obj_num_;
  }

  uint32_t GetRootObjNum() const {
    const CPDF_Reference* pRef = ToReference(
        GetMainTrailer() ? GetMainTrailer()->GetObjectFor("Root") : nullptr);
    return pRef ? pRef->GetRefObjNum() : last_root_obj_num_;
  }

 private:
  void ApplyTrailer(const CPDF_Dictionary* dict) {
    // The most recent Info object number contained in last added trailer.
    // See PDF 1.7 spec, section 3.4.5 - Incremental Updates.
    const auto* pRef = ToReference(dict->GetObjectFor("Info"));
    if (pRef)
      last_info_obj_num_ = pRef->GetRefObjNum();

    const auto* pRoot = ToReference(dict->GetObjectFor("Root"));
    if (pRoot)
      last_root_obj_num_ = pRoot->GetRefObjNum();
  }

  std::unique_ptr<CPDF_Dictionary> main_trailer_;
  uint32_t last_info_obj_num_ = 0;
  uint32_t last_root_obj_num_ = 0;
};

CPDF_Parser::CPDF_Parser()
    : m_pSyntax(pdfium::MakeUnique<CPDF_SyntaxParser>()),
      m_bHasParsed(false),
      m_bXRefStream(false),
      m_FileVersion(0),
      m_TrailerData(pdfium::MakeUnique<TrailerData>()) {}

CPDF_Parser::~CPDF_Parser() {
  ReleaseEncryptHandler();
}

uint32_t CPDF_Parser::GetLastObjNum() const {
  return m_ObjectInfo.empty() ? 0 : m_ObjectInfo.rbegin()->first;
}

bool CPDF_Parser::IsValidObjectNumber(uint32_t objnum) const {
  return !m_ObjectInfo.empty() && objnum <= m_ObjectInfo.rbegin()->first;
}

FX_FILESIZE CPDF_Parser::GetObjectPositionOrZero(uint32_t objnum) const {
  auto it = m_ObjectInfo.find(objnum);
  return it != m_ObjectInfo.end() ? it->second.pos : 0;
}

CPDF_Parser::ObjectType CPDF_Parser::GetObjectType(uint32_t objnum) const {
  ASSERT(IsValidObjectNumber(objnum));
  auto it = m_ObjectInfo.find(objnum);
  return it != m_ObjectInfo.end() ? it->second.type : ObjectType::kFree;
}

uint16_t CPDF_Parser::GetObjectGenNum(uint32_t objnum) const {
  ASSERT(IsValidObjectNumber(objnum));
  auto it = m_ObjectInfo.find(objnum);
  return it != m_ObjectInfo.end() ? it->second.gennum : 0;
}

bool CPDF_Parser::IsObjectFreeOrNull(uint32_t objnum) const {
  switch (GetObjectType(objnum)) {
    case ObjectType::kFree:
    case ObjectType::kNull:
      return true;
    case ObjectType::kNotCompressed:
    case ObjectType::kCompressed:
      return false;
  }
  NOTREACHED();
  return false;
}

bool CPDF_Parser::IsObjectFree(uint32_t objnum) const {
  return GetObjectType(objnum) == ObjectType::kFree;
}

void CPDF_Parser::SetEncryptDictionary(CPDF_Dictionary* pDict) {
  m_pEncryptDict = pDict;
}

RetainPtr<IFX_SeekableReadStream> CPDF_Parser::GetFileAccess() const {
  return m_pSyntax->GetFileAccess();
}

void CPDF_Parser::ShrinkObjectMap(uint32_t objnum) {
  if (objnum == 0) {
    m_ObjectInfo.clear();
    return;
  }

  auto it = m_ObjectInfo.lower_bound(objnum);
  while (it != m_ObjectInfo.end()) {
    auto saved_it = it++;
    m_ObjectInfo.erase(saved_it);
  }

  if (!pdfium::ContainsKey(m_ObjectInfo, objnum - 1))
    m_ObjectInfo[objnum - 1].pos = 0;
}

bool CPDF_Parser::InitSyntaxParser(
    const RetainPtr<IFX_SeekableReadStream>& file_access) {
  const int32_t header_offset = GetHeaderOffset(file_access);
  if (header_offset == kInvalidHeaderOffset)
    return false;
  if (file_access->GetSize() < header_offset + kPDFHeaderSize)
    return false;

  m_pSyntax->InitParser(file_access, header_offset);
  return ParseFileVersion();
}

bool CPDF_Parser::ParseFileVersion() {
  m_FileVersion = 0;
  uint8_t ch;
  if (!m_pSyntax->GetCharAt(5, ch))
    return false;

  if (std::isdigit(ch))
    m_FileVersion = FXSYS_DecimalCharToInt(static_cast<wchar_t>(ch)) * 10;

  if (!m_pSyntax->GetCharAt(7, ch))
    return false;

  if (std::isdigit(ch))
    m_FileVersion += FXSYS_DecimalCharToInt(static_cast<wchar_t>(ch));
  return true;
}

CPDF_Parser::Error CPDF_Parser::StartParse(
    const RetainPtr<IFX_SeekableReadStream>& pFileAccess,
    CPDF_Document* pDocument) {
  if (!InitSyntaxParser(pFileAccess))
    return FORMAT_ERROR;
  return StartParseInternal(pDocument);
}

CPDF_Parser::Error CPDF_Parser::StartParseInternal(CPDF_Document* pDocument) {
  ASSERT(!m_bHasParsed);
  m_bHasParsed = true;
  m_bXRefStream = false;

  m_pDocument = pDocument;

  bool bXRefRebuilt = false;

  m_LastXRefOffset = ParseStartXRef();

  if (m_LastXRefOffset > 0) {
    if (!LoadAllCrossRefV4(m_LastXRefOffset) &&
        !LoadAllCrossRefV5(m_LastXRefOffset)) {
      if (!RebuildCrossRef())
        return FORMAT_ERROR;

      bXRefRebuilt = true;
      m_LastXRefOffset = 0;
    }
  } else {
    if (!RebuildCrossRef())
      return FORMAT_ERROR;

    bXRefRebuilt = true;
  }
  Error eRet = SetEncryptHandler();
  if (eRet != SUCCESS)
    return eRet;

  m_pDocument->LoadDoc();
  if (!m_pDocument->GetRoot() || m_pDocument->GetPageCount() == 0) {
    if (bXRefRebuilt)
      return FORMAT_ERROR;

    ReleaseEncryptHandler();
    if (!RebuildCrossRef())
      return FORMAT_ERROR;

    eRet = SetEncryptHandler();
    if (eRet != SUCCESS)
      return eRet;

    m_pDocument->LoadDoc();
    if (!m_pDocument->GetRoot())
      return FORMAT_ERROR;
  }
  if (GetRootObjNum() == 0) {
    ReleaseEncryptHandler();
    if (!RebuildCrossRef() || GetRootObjNum() == 0)
      return FORMAT_ERROR;

    eRet = SetEncryptHandler();
    if (eRet != SUCCESS)
      return eRet;
  }
  if (m_pSecurityHandler && !m_pSecurityHandler->IsMetadataEncrypted()) {
    CPDF_Reference* pMetadata =
        ToReference(m_pDocument->GetRoot()->GetObjectFor("Metadata"));
    if (pMetadata)
      m_MetadataObjnum = pMetadata->GetRefObjNum();
  }
  return SUCCESS;
}

FX_FILESIZE CPDF_Parser::ParseStartXRef() {
  static constexpr char kStartXRefKeyword[] = "startxref";
  m_pSyntax->SetPos(m_pSyntax->m_FileLen - m_pSyntax->m_HeaderOffset -
                    strlen(kStartXRefKeyword));
  if (!m_pSyntax->BackwardsSearchToWord(kStartXRefKeyword, 4096))
    return 0;

  // Skip "startxref" keyword.
  m_pSyntax->GetKeyword();

  // Read XRef offset.
  bool bNumber;
  const ByteString xrefpos_str = m_pSyntax->GetNextWord(&bNumber);
  if (!bNumber || xrefpos_str.IsEmpty())
    return 0;

  const FX_SAFE_FILESIZE result = FXSYS_atoi64(xrefpos_str.c_str());
  if (!result.IsValid() || result.ValueOrDie() >= GetFileAccess()->GetSize())
    return 0;

  return result.ValueOrDie();
}

CPDF_Parser::Error CPDF_Parser::SetEncryptHandler() {
  ReleaseEncryptHandler();
  if (!GetTrailer())
    return FORMAT_ERROR;

  CPDF_Object* pEncryptObj = GetTrailer()->GetObjectFor("Encrypt");
  if (pEncryptObj) {
    if (CPDF_Dictionary* pEncryptDict = pEncryptObj->AsDictionary()) {
      SetEncryptDictionary(pEncryptDict);
    } else if (CPDF_Reference* pRef = pEncryptObj->AsReference()) {
      pEncryptObj = m_pDocument->GetOrParseIndirectObject(pRef->GetRefObjNum());
      if (pEncryptObj)
        SetEncryptDictionary(pEncryptObj->GetDict());
    }
  }

  if (m_pEncryptDict) {
    ByteString filter = m_pEncryptDict->GetStringFor("Filter");
    if (filter != "Standard")
      return HANDLER_ERROR;

    std::unique_ptr<CPDF_SecurityHandler> pSecurityHandler =
        pdfium::MakeUnique<CPDF_SecurityHandler>();
    if (!pSecurityHandler->OnInit(m_pEncryptDict.Get(), GetIDArray(),
                                  m_Password))
      return PASSWORD_ERROR;

    m_pSecurityHandler = std::move(pSecurityHandler);
  }
  return SUCCESS;
}

void CPDF_Parser::ReleaseEncryptHandler() {
  m_pSecurityHandler.reset();
  SetEncryptDictionary(nullptr);
}

FX_FILESIZE CPDF_Parser::GetObjectOffset(uint32_t objnum) const {
  if (!IsValidObjectNumber(objnum))
    return 0;

  if (GetObjectType(objnum) == ObjectType::kNotCompressed)
    return GetObjectPositionOrZero(objnum);

  if (GetObjectType(objnum) == ObjectType::kCompressed) {
    FX_FILESIZE pos = GetObjectPositionOrZero(objnum);
    return GetObjectPositionOrZero(pos);
  }
  return 0;
}

// Ideally, all the cross reference entries should be verified.
// In reality, we rarely see well-formed cross references don't match
// with the objects. crbug/602650 showed a case where object numbers
// in the cross reference table are all off by one.
bool CPDF_Parser::VerifyCrossRefV4() {
  for (const auto& it : m_ObjectInfo) {
    if (it.second.pos == 0)
      continue;
    // Find the first non-zero position.
    FX_FILESIZE SavedPos = m_pSyntax->GetPos();
    m_pSyntax->SetPos(it.second.pos);
    bool is_num = false;
    ByteString num_str = m_pSyntax->GetNextWord(&is_num);
    m_pSyntax->SetPos(SavedPos);
    if (!is_num || num_str.IsEmpty() ||
        FXSYS_atoui(num_str.c_str()) != it.first) {
      // If the object number read doesn't match the one stored,
      // something is wrong with the cross reference table.
      return false;
    }
    break;
  }
  return true;
}

bool CPDF_Parser::LoadAllCrossRefV4(FX_FILESIZE xrefpos) {
  if (!LoadCrossRefV4(xrefpos, true))
    return false;

  std::unique_ptr<CPDF_Dictionary> trailer = LoadTrailerV4();
  if (!trailer)
    return false;

  m_TrailerData->SetMainTrailer(std::move(trailer));
  int32_t xrefsize = GetDirectInteger(GetTrailer(), "Size");
  if (xrefsize > 0 && xrefsize <= kMaxXRefSize)
    ShrinkObjectMap(xrefsize);

  std::vector<FX_FILESIZE> CrossRefList;
  std::vector<FX_FILESIZE> XRefStreamList;
  std::set<FX_FILESIZE> seen_xrefpos;

  CrossRefList.push_back(xrefpos);
  XRefStreamList.push_back(GetDirectInteger(GetTrailer(), "XRefStm"));
  seen_xrefpos.insert(xrefpos);

  // When the trailer doesn't have Prev entry or Prev entry value is not
  // numerical, GetDirectInteger() returns 0. Loading will end.
  xrefpos = GetDirectInteger(GetTrailer(), "Prev");
  while (xrefpos) {
    // Check for circular references.
    if (pdfium::ContainsKey(seen_xrefpos, xrefpos))
      return false;

    seen_xrefpos.insert(xrefpos);

    // SLOW ...
    CrossRefList.insert(CrossRefList.begin(), xrefpos);
    LoadCrossRefV4(xrefpos, true);

    std::unique_ptr<CPDF_Dictionary> pDict(LoadTrailerV4());
    if (!pDict)
      return false;

    xrefpos = GetDirectInteger(pDict.get(), "Prev");

    // SLOW ...
    XRefStreamList.insert(XRefStreamList.begin(),
                          pDict->GetIntegerFor("XRefStm"));
    m_TrailerData->AppendTrailer(std::move(pDict));
  }

  for (size_t i = 0; i < CrossRefList.size(); ++i) {
    if (!LoadCrossRefV4(CrossRefList[i], false))
      return false;

    if (XRefStreamList[i] && !LoadCrossRefV5(&XRefStreamList[i], false))
      return false;

    if (i == 0 && !VerifyCrossRefV4())
      return false;
  }
  return true;
}

bool CPDF_Parser::LoadLinearizedAllCrossRefV4(FX_FILESIZE xrefpos) {
  if (!LoadCrossRefV4(xrefpos, false))
    return false;

  std::unique_ptr<CPDF_Dictionary> trailer = LoadTrailerV4();
  if (!trailer)
    return false;

  m_TrailerData->SetMainTrailer(std::move(trailer));
  int32_t xrefsize = GetDirectInteger(GetTrailer(), "Size");
  if (xrefsize == 0)
    return false;

  std::vector<FX_FILESIZE> CrossRefList;
  std::vector<FX_FILESIZE> XRefStreamList;
  std::set<FX_FILESIZE> seen_xrefpos;

  CrossRefList.push_back(xrefpos);
  XRefStreamList.push_back(GetDirectInteger(GetTrailer(), "XRefStm"));
  seen_xrefpos.insert(xrefpos);

  xrefpos = GetDirectInteger(GetTrailer(), "Prev");
  while (xrefpos) {
    // Check for circular references.
    if (pdfium::ContainsKey(seen_xrefpos, xrefpos))
      return false;

    seen_xrefpos.insert(xrefpos);

    // SLOW ...
    CrossRefList.insert(CrossRefList.begin(), xrefpos);
    LoadCrossRefV4(xrefpos, true);

    std::unique_ptr<CPDF_Dictionary> pDict(LoadTrailerV4());
    if (!pDict)
      return false;

    xrefpos = GetDirectInteger(pDict.get(), "Prev");

    // SLOW ...
    XRefStreamList.insert(XRefStreamList.begin(),
                          pDict->GetIntegerFor("XRefStm"));
    m_TrailerData->AppendTrailer(std::move(pDict));
  }

  for (size_t i = 1; i < CrossRefList.size(); ++i) {
    if (!LoadCrossRefV4(CrossRefList[i], false))
      return false;

    if (XRefStreamList[i] && !LoadCrossRefV5(&XRefStreamList[i], false))
      return false;
  }
  return true;
}

bool CPDF_Parser::ParseAndAppendCrossRefSubsectionData(
    uint32_t start_objnum,
    uint32_t count,
    std::vector<CrossRefObjData>* out_objects) {
  // Each entry shall be exactly 20 byte.
  // A sample entry looks like:
  // "0000000000 00007 f\r\n"
  static constexpr int32_t kEntryConstSize = 20;

  if (!out_objects) {
    FX_SAFE_FILESIZE pos = count;
    pos *= kEntryConstSize;
    pos += m_pSyntax->GetPos();
    if (!pos.IsValid())
      return false;
    m_pSyntax->SetPos(pos.ValueOrDie());
    return true;
  }
  const size_t start_obj_index = out_objects->size();
  FX_SAFE_SIZE_T new_size = start_obj_index;
  new_size += count;
  if (!new_size.IsValid())
    return false;

  if (new_size.ValueOrDie() > kMaxXRefSize)
    return false;

  const size_t max_entries_in_file =
      m_pSyntax->GetFileAccess()->GetSize() / kEntryConstSize;
  if (new_size.ValueOrDie() > max_entries_in_file)
    return false;

  out_objects->resize(new_size.ValueOrDie());

  std::vector<char> buf(1024 * kEntryConstSize + 1);
  buf.back() = '\0';

  int32_t nBlocks = count / 1024 + 1;
  for (int32_t block = 0; block < nBlocks; block++) {
    int32_t block_size = block == nBlocks - 1 ? count % 1024 : 1024;
    if (!m_pSyntax->ReadBlock(reinterpret_cast<uint8_t*>(buf.data()),
                              block_size * kEntryConstSize)) {
      return false;
    }

    for (int32_t i = 0; i < block_size; i++) {
      CrossRefObjData& obj_data =
          (*out_objects)[start_obj_index + block * 1024 + i];

      const uint32_t objnum = start_objnum + block * 1024 + i;

      obj_data.obj_num = objnum;

      ObjectInfo& info = obj_data.info;

      char* pEntry = &buf[i * kEntryConstSize];
      if (pEntry[17] == 'f') {
        info.pos = 0;
        info.type = ObjectType::kFree;
      } else {
        const FX_SAFE_FILESIZE offset = FXSYS_atoi64(pEntry);
        if (!offset.IsValid())
          return false;

        if (offset.ValueOrDie() == 0) {
          for (int32_t c = 0; c < 10; c++) {
            if (!std::isdigit(pEntry[c]))
              return false;
          }
        }

        info.pos = offset.ValueOrDie();

        // TODO(art-snake): The info.gennum is uint16_t, but version may be
        // greated than max<uint16_t>. Needs solve this issue.
        const int32_t version = FXSYS_atoi(pEntry + 11);
        info.gennum = version;
        info.type = ObjectType::kNotCompressed;
      }
    }
  }
  return true;
}

bool CPDF_Parser::ParseCrossRefV4(std::vector<CrossRefObjData>* out_objects) {
  if (out_objects)
    out_objects->clear();

  if (m_pSyntax->GetKeyword() != "xref")
    return false;
  std::vector<CrossRefObjData> result_objects;
  while (1) {
    FX_FILESIZE SavedPos = m_pSyntax->GetPos();
    bool bIsNumber;
    ByteString word = m_pSyntax->GetNextWord(&bIsNumber);
    if (word.IsEmpty()) {
      return false;
    }

    if (!bIsNumber) {
      m_pSyntax->SetPos(SavedPos);
      break;
    }

    uint32_t start_objnum = FXSYS_atoui(word.c_str());
    if (start_objnum >= kMaxObjectNumber)
      return false;

    uint32_t count = m_pSyntax->GetDirectNum();
    m_pSyntax->ToNextWord();
    SavedPos = m_pSyntax->GetPos();

    if (!ParseAndAppendCrossRefSubsectionData(
            start_objnum, count, out_objects ? &result_objects : nullptr)) {
      return false;
    }
  }
  if (out_objects)
    *out_objects = std::move(result_objects);
  return true;
}

bool CPDF_Parser::LoadCrossRefV4(FX_FILESIZE pos, bool bSkip) {
  m_pSyntax->SetPos(pos);
  std::vector<CrossRefObjData> objects;
  if (!ParseCrossRefV4(bSkip ? nullptr : &objects))
    return false;

  MergeCrossRefObjectsData(objects);
  return true;
}

void CPDF_Parser::MergeCrossRefObjectsData(
    const std::vector<CrossRefObjData>& objects) {
  for (const auto& obj : objects) {
    m_ObjectInfo[obj.obj_num] = obj.info;
  }
}

bool CPDF_Parser::LoadAllCrossRefV5(FX_FILESIZE xrefpos) {
  if (!LoadCrossRefV5(&xrefpos, true))
    return false;

  std::set<FX_FILESIZE> seen_xrefpos;
  while (xrefpos) {
    seen_xrefpos.insert(xrefpos);
    if (!LoadCrossRefV5(&xrefpos, false))
      return false;

    // Check for circular references.
    if (pdfium::ContainsKey(seen_xrefpos, xrefpos))
      return false;
  }
  m_ObjectStreamMap.clear();
  m_bXRefStream = true;
  return true;
}

bool CPDF_Parser::RebuildCrossRef() {
  m_ObjectInfo.clear();
  m_TrailerData->Clear();

  ParserState state = ParserState::kDefault;
  int32_t inside_index = 0;
  uint32_t objnum = 0;
  uint32_t gennum = 0;
  int32_t depth = 0;
  const uint32_t kBufferSize = 4096;
  std::vector<uint8_t> buffer(kBufferSize);

  FX_FILESIZE pos = m_pSyntax->m_HeaderOffset;
  FX_FILESIZE start_pos = 0;
  FX_FILESIZE start_pos1 = 0;
  FX_FILESIZE last_obj = -1;
  FX_FILESIZE last_xref = -1;
  FX_FILESIZE last_trailer = -1;

  while (pos < m_pSyntax->m_FileLen) {
    const FX_FILESIZE saved_pos = pos;
    bool bOverFlow = false;
    uint32_t size =
        std::min((uint32_t)(m_pSyntax->m_FileLen - pos), kBufferSize);
    if (!m_pSyntax->GetFileAccess()->ReadBlock(buffer.data(), pos, size))
      break;

    for (uint32_t i = 0; i < size; i++) {
      uint8_t byte = buffer[i];
      switch (state) {
        case ParserState::kDefault:
          if (PDFCharIsWhitespace(byte)) {
            state = ParserState::kWhitespace;
          } else if (std::isdigit(byte)) {
            --i;
            state = ParserState::kWhitespace;
          } else if (byte == '%') {
            inside_index = 0;
            state = ParserState::kComment;
          } else if (byte == '(') {
            state = ParserState::kString;
            depth = 1;
          } else if (byte == '<') {
            inside_index = 1;
            state = ParserState::kHexString;
          } else if (byte == '\\') {
            state = ParserState::kEscapedString;
          } else if (byte == 't') {
            state = ParserState::kTrailer;
            inside_index = 1;
          }
          break;

        case ParserState::kWhitespace:
          if (std::isdigit(byte)) {
            start_pos = pos + i;
            state = ParserState::kObjNum;
            objnum = FXSYS_DecimalCharToInt(static_cast<wchar_t>(byte));
          } else if (byte == 't') {
            state = ParserState::kTrailer;
            inside_index = 1;
          } else if (byte == 'x') {
            state = ParserState::kXref;
            inside_index = 1;
          } else if (!PDFCharIsWhitespace(byte)) {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kObjNum:
          if (std::isdigit(byte)) {
            objnum = objnum * 10 +
                     FXSYS_DecimalCharToInt(static_cast<wchar_t>(byte));
          } else if (PDFCharIsWhitespace(byte)) {
            state = ParserState::kPostObjNum;
          } else {
            --i;
            state = ParserState::kEndObj;
            inside_index = 0;
          }
          break;

        case ParserState::kPostObjNum:
          if (std::isdigit(byte)) {
            start_pos1 = pos + i;
            state = ParserState::kGenNum;
            gennum = FXSYS_DecimalCharToInt(static_cast<wchar_t>(byte));
          } else if (byte == 't') {
            state = ParserState::kTrailer;
            inside_index = 1;
          } else if (!PDFCharIsWhitespace(byte)) {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kGenNum:
          if (std::isdigit(byte)) {
            gennum = gennum * 10 +
                     FXSYS_DecimalCharToInt(static_cast<wchar_t>(byte));
          } else if (PDFCharIsWhitespace(byte)) {
            state = ParserState::kPostGenNum;
          } else {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kPostGenNum:
          if (byte == 'o') {
            state = ParserState::kBeginObj;
            inside_index = 1;
          } else if (std::isdigit(byte)) {
            objnum = gennum;
            gennum = FXSYS_DecimalCharToInt(static_cast<wchar_t>(byte));
            start_pos = start_pos1;
            start_pos1 = pos + i;
            state = ParserState::kGenNum;
          } else if (byte == 't') {
            state = ParserState::kTrailer;
            inside_index = 1;
          } else if (!PDFCharIsWhitespace(byte)) {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kBeginObj:
          switch (inside_index) {
            case 1:
              if (byte != 'b') {
                --i;
                state = ParserState::kDefault;
              } else {
                inside_index++;
              }
              break;
            case 2:
              if (byte != 'j') {
                --i;
                state = ParserState::kDefault;
              } else {
                inside_index++;
              }
              break;
            case 3:
              if (PDFCharIsWhitespace(byte) || PDFCharIsDelimiter(byte)) {
                FX_FILESIZE obj_pos = start_pos - m_pSyntax->m_HeaderOffset;
                last_obj = start_pos;
                FX_FILESIZE obj_end = 0;
                std::unique_ptr<CPDF_Object> pObject =
                    ParseIndirectObjectAtByStrict(m_pDocument.Get(), obj_pos,
                                                  objnum, &obj_end);
                if (CPDF_Stream* pStream = ToStream(pObject.get())) {
                  if (CPDF_Dictionary* pDict = pStream->GetDict()) {
                    if ((pDict->KeyExist("Type")) &&
                        (pDict->GetStringFor("Type") == "XRef" &&
                         pDict->KeyExist("Size"))) {
                      CPDF_Object* pRoot = pDict->GetObjectFor("Root");
                      if (pRoot && pRoot->GetDict() &&
                          pRoot->GetDict()->GetObjectFor("Pages")) {
                        m_TrailerData->SetMainTrailer(
                            ToDictionary(pDict->Clone()));
                      }
                    }
                  }
                }

                FX_FILESIZE offset = 0;
                m_pSyntax->SetPos(obj_pos);
                offset = m_pSyntax->FindTag("obj", 0);
                if (offset == -1)
                  offset = 0;
                else
                  offset += 3;

                FX_FILESIZE nLen = obj_end - obj_pos - offset;
                if ((uint32_t)nLen > size - i) {
                  pos = obj_end + m_pSyntax->m_HeaderOffset;
                  bOverFlow = true;
                } else {
                  i += (uint32_t)nLen;
                }

                if (!m_ObjectInfo.empty() && IsValidObjectNumber(objnum) &&
                    m_ObjectInfo[objnum].pos) {
                  if (pObject) {
                    m_ObjectInfo[objnum].pos = obj_pos;
                    m_ObjectInfo[objnum].gennum = gennum;
                  }
                } else {
                  m_ObjectInfo[objnum].pos = obj_pos;
                  m_ObjectInfo[objnum].type = ObjectType::kNotCompressed;
                  m_ObjectInfo[objnum].gennum = gennum;
                }
              }
              --i;
              state = ParserState::kDefault;
              break;
          }
          break;

        case ParserState::kTrailer:
          if (inside_index == 7) {
            if (PDFCharIsWhitespace(byte) || PDFCharIsDelimiter(byte)) {
              last_trailer = pos + i - 7;
              m_pSyntax->SetPos(pos + i - m_pSyntax->m_HeaderOffset);

              std::unique_ptr<CPDF_Object> pObj =
                  m_pSyntax->GetObjectBody(m_pDocument.Get());
              if (pObj) {
                if (pObj->IsDictionary() || pObj->AsStream()) {
                  CPDF_Stream* pStream = pObj->AsStream();
                  if (CPDF_Dictionary* pTrailer =
                          pStream ? pStream->GetDict() : pObj->AsDictionary()) {
                    if (GetTrailer()) {
                      CPDF_Object* pRoot = pTrailer->GetObjectFor("Root");
                      CPDF_Reference* pRef = ToReference(pRoot);
                      if (!pRoot ||
                          (pRef && IsValidObjectNumber(pRef->GetRefObjNum()) &&
                           m_ObjectInfo[pRef->GetRefObjNum()].pos != 0)) {
                        auto it = pTrailer->begin();
                        while (it != pTrailer->end()) {
                          const ByteString& key = it->first;
                          CPDF_Object* pElement = it->second.get();
                          ++it;
                          uint32_t dwObjNum =
                              pElement ? pElement->GetObjNum() : 0;
                          if (dwObjNum) {
                            GetTrailer()->SetNewFor<CPDF_Reference>(
                                key, m_pDocument.Get(), dwObjNum);
                          } else {
                            GetTrailer()->SetFor(key, pElement->Clone());
                          }
                        }
                      }
                    } else {
                      m_TrailerData->SetMainTrailer(
                          ToDictionary(pObj->IsStream() ? pTrailer->Clone()
                                                        : std::move(pObj)));

                      FX_FILESIZE dwSavePos = m_pSyntax->GetPos();
                      ByteString strWord = m_pSyntax->GetKeyword();
                      if (!strWord.Compare("startxref")) {
                        bool bNumber;
                        ByteString bsOffset = m_pSyntax->GetNextWord(&bNumber);
                        if (bNumber)
                          m_LastXRefOffset = FXSYS_atoi(bsOffset.c_str());
                      }
                      m_pSyntax->SetPos(dwSavePos);
                    }
                  }
                }
              }
            }
            --i;
            state = ParserState::kDefault;
          } else if (byte == "trailer"[inside_index]) {
            inside_index++;
          } else {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kXref:
          if (inside_index == 4) {
            last_xref = pos + i - 4;
            state = ParserState::kWhitespace;
          } else if (byte == "xref"[inside_index]) {
            inside_index++;
          } else {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kComment:
          if (PDFCharIsLineEnding(byte))
            state = ParserState::kDefault;
          break;

        case ParserState::kString:
          if (byte == ')') {
            if (depth > 0)
              depth--;
          } else if (byte == '(') {
            depth++;
          }

          if (!depth)
            state = ParserState::kDefault;
          break;

        case ParserState::kHexString:
          if (byte == '>' || (byte == '<' && inside_index == 1))
            state = ParserState::kDefault;
          inside_index = 0;
          break;

        case ParserState::kEscapedString:
          if (PDFCharIsDelimiter(byte) || PDFCharIsWhitespace(byte)) {
            --i;
            state = ParserState::kDefault;
          }
          break;

        case ParserState::kEndObj:
          if (PDFCharIsWhitespace(byte)) {
            state = ParserState::kDefault;
          } else if (byte == '%' || byte == '(' || byte == '<' ||
                     byte == '\\') {
            state = ParserState::kDefault;
            --i;
          } else if (inside_index == 6) {
            state = ParserState::kDefault;
            --i;
          } else if (byte == "endobj"[inside_index]) {
            inside_index++;
          }
          break;
      }

      if (bOverFlow) {
        size = 0;
        break;
      }
    }
    pos += size;

    // If the position has not changed at all or went backwards in a loop
    // iteration, then break out to prevent infinite looping.
    if (pos <= saved_pos)
      break;
  }

  if (last_xref != -1 && last_xref > last_obj)
    last_trailer = last_xref;
  else if (last_trailer == -1 || last_xref < last_obj)
    last_trailer = m_pSyntax->m_FileLen;

  return GetTrailer() && !m_ObjectInfo.empty();
}

bool CPDF_Parser::LoadCrossRefV5(FX_FILESIZE* pos, bool bMainXRef) {
  std::unique_ptr<CPDF_Object> pObject(
      ParseIndirectObjectAt(m_pDocument.Get(), *pos, 0));
  if (!pObject)
    return false;

  uint32_t objnum = pObject->GetObjNum();
  if (!objnum)
    return false;

  CPDF_Object* pUnownedObject = pObject.get();
  if (m_pDocument) {
    const CPDF_Dictionary* pRootDict = m_pDocument->GetRoot();
    if (pRootDict && pRootDict->GetObjNum() == objnum)
      return false;
    if (!m_pDocument->ReplaceIndirectObjectIfHigherGeneration(
            objnum, std::move(pObject))) {
      return false;
    }
  }

  CPDF_Stream* pStream = pUnownedObject->AsStream();
  if (!pStream)
    return false;

  CPDF_Dictionary* pDict = pStream->GetDict();
  *pos = pDict->GetIntegerFor("Prev");
  int32_t size = pDict->GetIntegerFor("Size");
  if (size < 0)
    return false;

  std::unique_ptr<CPDF_Dictionary> pNewTrailer = ToDictionary(pDict->Clone());
  if (bMainXRef) {
    m_TrailerData->SetMainTrailer(std::move(pNewTrailer));
    ShrinkObjectMap(size);
    for (auto& it : m_ObjectInfo)
      it.second.type = ObjectType::kFree;
  } else {
    m_TrailerData->AppendTrailer(std::move(pNewTrailer));
  }

  std::vector<std::pair<int32_t, int32_t>> arrIndex;
  CPDF_Array* pArray = pDict->GetArrayFor("Index");
  if (pArray) {
    for (size_t i = 0; i < pArray->GetCount() / 2; i++) {
      CPDF_Object* pStartNumObj = pArray->GetObjectAt(i * 2);
      CPDF_Object* pCountObj = pArray->GetObjectAt(i * 2 + 1);

      if (ToNumber(pStartNumObj) && ToNumber(pCountObj)) {
        int nStartNum = pStartNumObj->GetInteger();
        int nCount = pCountObj->GetInteger();
        if (nStartNum >= 0 && nCount > 0)
          arrIndex.push_back(std::make_pair(nStartNum, nCount));
      }
    }
  }

  if (arrIndex.empty())
    arrIndex.push_back(std::make_pair(0, size));

  pArray = pDict->GetArrayFor("W");
  if (!pArray)
    return false;

  std::vector<uint32_t> WidthArray;
  FX_SAFE_UINT32 dwAccWidth = 0;
  for (size_t i = 0; i < pArray->GetCount(); ++i) {
    WidthArray.push_back(pArray->GetIntegerAt(i));
    dwAccWidth += WidthArray[i];
  }

  if (!dwAccWidth.IsValid() || WidthArray.size() < 3)
    return false;

  uint32_t totalWidth = dwAccWidth.ValueOrDie();
  auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pStream);
  pAcc->LoadAllDataFiltered();

  const uint8_t* pData = pAcc->GetData();
  uint32_t dwTotalSize = pAcc->GetSize();
  uint32_t segindex = 0;
  for (uint32_t i = 0; i < arrIndex.size(); i++) {
    int32_t startnum = arrIndex[i].first;
    if (startnum < 0)
      continue;

    uint32_t count = pdfium::base::checked_cast<uint32_t>(arrIndex[i].second);
    FX_SAFE_UINT32 dwCaculatedSize = segindex;
    dwCaculatedSize += count;
    dwCaculatedSize *= totalWidth;
    if (!dwCaculatedSize.IsValid() ||
        dwCaculatedSize.ValueOrDie() > dwTotalSize) {
      continue;
    }

    const uint8_t* segstart = pData + segindex * totalWidth;
    FX_SAFE_UINT32 dwMaxObjNum = startnum;
    dwMaxObjNum += count;
    uint32_t dwV5Size = m_ObjectInfo.empty() ? 0 : GetLastObjNum() + 1;
    if (!dwMaxObjNum.IsValid() || dwMaxObjNum.ValueOrDie() > dwV5Size)
      continue;

    for (uint32_t j = 0; j < count; j++) {
      ObjectType type = ObjectType::kNotCompressed;
      const uint8_t* entrystart = segstart + j * totalWidth;
      if (WidthArray[0]) {
        const int cross_ref_stream_obj_type =
            GetVarInt(entrystart, WidthArray[0]);
        type = GetObjectTypeFromCrossRefStreamType(cross_ref_stream_obj_type);
      }

      if (GetObjectType(startnum + j) == ObjectType::kNull) {
        FX_FILESIZE offset =
            GetVarInt(entrystart + WidthArray[0], WidthArray[1]);
        m_ObjectInfo[startnum + j].pos = offset;
        continue;
      }

      if (GetObjectType(startnum + j) != ObjectType::kFree)
        continue;

      ObjectInfo& info = m_ObjectInfo[startnum + j];

      info.type = type;
      if (type == ObjectType::kFree) {
        info.pos = 0;
      } else {
        const FX_FILESIZE entry_value =
            GetVarInt(entrystart + WidthArray[0], WidthArray[1]);
        if (type == ObjectType::kNotCompressed) {
          const auto object_offset = entry_value;
          info.pos = object_offset;
        } else {
          const auto archive_obj_num = entry_value;
          info.archive_obj_num = archive_obj_num;
          if (archive_obj_num < 0 || !IsValidObjectNumber(archive_obj_num))
            return false;
          m_ObjectInfo[archive_obj_num].type = ObjectType::kNull;
        }
      }
    }
    segindex += count;
  }
  return true;
}

const CPDF_Array* CPDF_Parser::GetIDArray() const {
  return GetTrailer() ? GetTrailer()->GetArrayFor("ID") : nullptr;
}

CPDF_Dictionary* CPDF_Parser::GetTrailer() const {
  return m_TrailerData->GetMainTrailer();
}

std::unique_ptr<CPDF_Dictionary> CPDF_Parser::GetCombinedTrailer() const {
  return m_TrailerData->GetCombinedTrailer();
}

uint32_t CPDF_Parser::GetInfoObjNum() {
  return m_TrailerData->GetInfoObjNum();
}

uint32_t CPDF_Parser::GetRootObjNum() {
  return m_TrailerData->GetRootObjNum();
}

std::unique_ptr<CPDF_Object> CPDF_Parser::ParseIndirectObject(
    CPDF_IndirectObjectHolder* pObjList,
    uint32_t objnum) {
  if (!IsValidObjectNumber(objnum))
    return nullptr;

  // Prevent circular parsing the same object.
  if (pdfium::ContainsKey(m_ParsingObjNums, objnum))
    return nullptr;

  pdfium::ScopedSetInsertion<uint32_t> local_insert(&m_ParsingObjNums, objnum);
  if (GetObjectType(objnum) == ObjectType::kNotCompressed ||
      GetObjectType(objnum) == ObjectType::kNull) {
    FX_FILESIZE pos = m_ObjectInfo[objnum].pos;
    if (pos <= 0)
      return nullptr;
    return ParseIndirectObjectAt(pObjList, pos, objnum);
  }
  if (GetObjectType(objnum) != ObjectType::kCompressed)
    return nullptr;

  RetainPtr<CPDF_StreamAcc> pObjStream =
      GetObjectStream(m_ObjectInfo[objnum].pos);
  if (!pObjStream)
    return nullptr;

  auto file = pdfium::MakeRetain<CFX_MemoryStream>(
      const_cast<uint8_t*>(pObjStream->GetData()),
      static_cast<size_t>(pObjStream->GetSize()), false);
  CPDF_SyntaxParser syntax;
  syntax.InitParser(file, 0);
  const int32_t offset = GetStreamFirst(pObjStream);

  // Read object numbers from |pObjStream| into a cache.
  if (!pdfium::ContainsKey(m_ObjCache, pObjStream)) {
    for (int32_t i = GetStreamNCount(pObjStream); i > 0; --i) {
      uint32_t thisnum = syntax.GetDirectNum();
      uint32_t thisoff = syntax.GetDirectNum();
      m_ObjCache[pObjStream][thisnum] = thisoff;
    }
  }

  const auto it = m_ObjCache[pObjStream].find(objnum);
  if (it == m_ObjCache[pObjStream].end())
    return nullptr;

  syntax.SetPos(offset + it->second);
  return syntax.GetObjectBody(pObjList);
}

RetainPtr<CPDF_StreamAcc> CPDF_Parser::GetObjectStream(uint32_t objnum) {
  auto it = m_ObjectStreamMap.find(objnum);
  if (it != m_ObjectStreamMap.end())
    return it->second;

  if (!m_pDocument)
    return nullptr;

  const CPDF_Stream* pStream =
      ToStream(m_pDocument->GetOrParseIndirectObject(objnum));
  if (!pStream)
    return nullptr;

  auto pStreamAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pStream);
  pStreamAcc->LoadAllDataFiltered();
  m_ObjectStreamMap[objnum] = pStreamAcc;
  return pStreamAcc;
}

std::unique_ptr<CPDF_Object> CPDF_Parser::ParseIndirectObjectAt(
    CPDF_IndirectObjectHolder* pObjList,
    FX_FILESIZE pos,
    uint32_t objnum) {
  return ParseIndirectObjectAtInternal(
      pObjList, pos, objnum, CPDF_SyntaxParser::ParseType::kLoose, nullptr);
}

std::unique_ptr<CPDF_Object> CPDF_Parser::ParseIndirectObjectAtInternal(
    CPDF_IndirectObjectHolder* pObjList,
    FX_FILESIZE pos,
    uint32_t objnum,
    CPDF_SyntaxParser::ParseType parse_type,
    FX_FILESIZE* pResultPos) {
  const FX_FILESIZE saved_pos = m_pSyntax->GetPos();
  m_pSyntax->SetPos(pos);
  auto result = m_pSyntax->GetIndirectObject(pObjList, parse_type);

  if (pResultPos)
    *pResultPos = m_pSyntax->GetPos();
  m_pSyntax->SetPos(saved_pos);

  if (result && objnum && result->GetObjNum() != objnum)
    return nullptr;

  const bool should_decrypt = m_pSecurityHandler &&
                              m_pSecurityHandler->GetCryptoHandler() &&
                              objnum != m_MetadataObjnum;
  if (should_decrypt)
    result = m_pSecurityHandler->GetCryptoHandler()->DecryptObjectTree(
        std::move(result));

  return result;
}

std::unique_ptr<CPDF_Object> CPDF_Parser::ParseIndirectObjectAtByStrict(
    CPDF_IndirectObjectHolder* pObjList,
    FX_FILESIZE pos,
    uint32_t objnum,
    FX_FILESIZE* pResultPos) {
  return ParseIndirectObjectAtInternal(
      pObjList, pos, objnum, CPDF_SyntaxParser::ParseType::kStrict, pResultPos);
}

uint32_t CPDF_Parser::GetFirstPageNo() const {
  return m_pLinearized ? m_pLinearized->GetFirstPageNo() : 0;
}

std::unique_ptr<CPDF_Dictionary> CPDF_Parser::LoadTrailerV4() {
  if (m_pSyntax->GetKeyword() != "trailer")
    return nullptr;

  return ToDictionary(m_pSyntax->GetObjectBody(m_pDocument.Get()));
}

uint32_t CPDF_Parser::GetPermissions() const {
  if (!m_pSecurityHandler)
    return 0xFFFFFFFF;

  uint32_t dwPermission = m_pSecurityHandler->GetPermissions();
  if (m_pEncryptDict && m_pEncryptDict->GetStringFor("Filter") == "Standard") {
    // See PDF Reference 1.7, page 123, table 3.20.
    dwPermission &= 0xFFFFFFFC;
    dwPermission |= 0xFFFFF0C0;
  }
  return dwPermission;
}

std::unique_ptr<CPDF_LinearizedHeader> CPDF_Parser::ParseLinearizedHeader() {
  return CPDF_LinearizedHeader::Parse(m_pSyntax.get());
}

CPDF_Parser::Error CPDF_Parser::StartLinearizedParse(
    const RetainPtr<IFX_SeekableReadStream>& pFileAccess,
    CPDF_Document* pDocument) {
  ASSERT(!m_bHasParsed);
  m_bXRefStream = false;
  m_LastXRefOffset = 0;

  if (!InitSyntaxParser(pFileAccess))
    return FORMAT_ERROR;

  m_pLinearized = ParseLinearizedHeader();
  if (!m_pLinearized)
    return StartParseInternal(std::move(pDocument));

  m_bHasParsed = true;
  m_pDocument = pDocument;

  m_LastXRefOffset = m_pLinearized->GetLastXRefOffset();
  FX_FILESIZE dwFirstXRefOffset = m_LastXRefOffset;
  bool bXRefRebuilt = false;
  bool bLoadV4 = LoadCrossRefV4(dwFirstXRefOffset, false);
  if (!bLoadV4 && !LoadCrossRefV5(&dwFirstXRefOffset, true)) {
    if (!RebuildCrossRef())
      return FORMAT_ERROR;

    bXRefRebuilt = true;
    m_LastXRefOffset = 0;
  }
  if (bLoadV4) {
    std::unique_ptr<CPDF_Dictionary> trailer = LoadTrailerV4();
    if (!trailer)
      return SUCCESS;

    m_TrailerData->SetMainTrailer(std::move(trailer));
    int32_t xrefsize = GetDirectInteger(GetTrailer(), "Size");
    if (xrefsize > 0)
      ShrinkObjectMap(xrefsize);
  }

  Error eRet = SetEncryptHandler();
  if (eRet != SUCCESS)
    return eRet;

  m_pDocument->LoadLinearizedDoc(m_pLinearized.get());
  if (!m_pDocument->GetRoot() || m_pDocument->GetPageCount() == 0) {
    if (bXRefRebuilt)
      return FORMAT_ERROR;

    ReleaseEncryptHandler();
    if (!RebuildCrossRef())
      return FORMAT_ERROR;

    eRet = SetEncryptHandler();
    if (eRet != SUCCESS)
      return eRet;

    m_pDocument->LoadLinearizedDoc(m_pLinearized.get());
    if (!m_pDocument->GetRoot())
      return FORMAT_ERROR;
  }

  if (GetRootObjNum() == 0) {
    ReleaseEncryptHandler();
    if (!RebuildCrossRef() || GetRootObjNum() == 0)
      return FORMAT_ERROR;

    eRet = SetEncryptHandler();
    if (eRet != SUCCESS)
      return eRet;
  }

  if (m_pSecurityHandler && m_pSecurityHandler->IsMetadataEncrypted()) {
    if (CPDF_Reference* pMetadata =
            ToReference(m_pDocument->GetRoot()->GetObjectFor("Metadata")))
      m_MetadataObjnum = pMetadata->GetRefObjNum();
  }
  return SUCCESS;
}

bool CPDF_Parser::LoadLinearizedAllCrossRefV5(FX_FILESIZE xrefpos) {
  if (!LoadCrossRefV5(&xrefpos, false))
    return false;

  std::set<FX_FILESIZE> seen_xrefpos;
  while (xrefpos) {
    seen_xrefpos.insert(xrefpos);
    if (!LoadCrossRefV5(&xrefpos, false))
      return false;

    // Check for circular references.
    if (pdfium::ContainsKey(seen_xrefpos, xrefpos))
      return false;
  }
  m_ObjectStreamMap.clear();
  m_bXRefStream = true;
  return true;
}

CPDF_Parser::Error CPDF_Parser::LoadLinearizedMainXRefTable() {
  const FX_SAFE_FILESIZE main_xref_offset = GetTrailer()->GetIntegerFor("Prev");
  if (!main_xref_offset.IsValid())
    return FORMAT_ERROR;

  if (main_xref_offset.ValueOrDie() == 0)
    return SUCCESS;

  const AutoRestorer<uint32_t> save_metadata_objnum(&m_MetadataObjnum);
  m_MetadataObjnum = 0;
  m_ObjectStreamMap.clear();
  m_ObjCache.clear();

  if (!LoadLinearizedAllCrossRefV4(main_xref_offset.ValueOrDie()) &&
      !LoadLinearizedAllCrossRefV5(main_xref_offset.ValueOrDie())) {
    m_LastXRefOffset = 0;
    return FORMAT_ERROR;
  }

  return SUCCESS;
}

CPDF_Parser::ObjectType CPDF_Parser::GetObjectTypeFromCrossRefStreamType(
    int cross_ref_stream_type) const {
  switch (cross_ref_stream_type) {
    case 0:
      return CPDF_Parser::ObjectType::kFree;
    case 1:
      return CPDF_Parser::ObjectType::kNotCompressed;
    case 2:
      return CPDF_Parser::ObjectType::kCompressed;
    default:
      return CPDF_Parser::ObjectType::kNull;
  }
}
