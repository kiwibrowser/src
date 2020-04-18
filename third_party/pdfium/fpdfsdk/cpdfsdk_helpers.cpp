// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_helpers.h"

#include "constants/stream_dict_common.h"
#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfdoc/cpdf_annot.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fpdfdoc/cpdf_metadata.h"
#include "public/fpdf_ext.h"

#ifdef PDF_ENABLE_XFA
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#endif

namespace {

constexpr char kQuadPoints[] = "QuadPoints";

bool RaiseUnSupportError(int nError) {
  CFSDK_UnsupportInfo_Adapter* pAdapter =
      CPDF_ModuleMgr::Get()->GetUnsupportInfoAdapter();
  if (!pAdapter)
    return false;

  UNSUPPORT_INFO* info = static_cast<UNSUPPORT_INFO*>(pAdapter->GetUnspInfo());
  if (info && info->FSDK_UnSupport_Handler)
    info->FSDK_UnSupport_Handler(info, nError);
  return true;
}

#ifdef PDF_ENABLE_XFA
class FPDF_FileHandlerContext : public IFX_SeekableStream {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  // IFX_SeekableStream:
  FX_FILESIZE GetSize() override;
  bool IsEOF() override;
  FX_FILESIZE GetPosition() override;
  bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) override;
  size_t ReadBlock(void* buffer, size_t size) override;
  bool WriteBlock(const void* buffer, FX_FILESIZE offset, size_t size) override;
  bool Flush() override;

  void SetPosition(FX_FILESIZE pos) { m_nCurPos = pos; }

 protected:
  explicit FPDF_FileHandlerContext(FPDF_FILEHANDLER* pFS);
  ~FPDF_FileHandlerContext() override;

  FPDF_FILEHANDLER* m_pFS;
  FX_FILESIZE m_nCurPos;
};

FPDF_FileHandlerContext::FPDF_FileHandlerContext(FPDF_FILEHANDLER* pFS) {
  m_pFS = pFS;
  m_nCurPos = 0;
}

FPDF_FileHandlerContext::~FPDF_FileHandlerContext() {
  if (m_pFS && m_pFS->Release)
    m_pFS->Release(m_pFS->clientData);
}

FX_FILESIZE FPDF_FileHandlerContext::GetSize() {
  if (m_pFS && m_pFS->GetSize)
    return (FX_FILESIZE)m_pFS->GetSize(m_pFS->clientData);
  return 0;
}

bool FPDF_FileHandlerContext::IsEOF() {
  return m_nCurPos >= GetSize();
}

FX_FILESIZE FPDF_FileHandlerContext::GetPosition() {
  return m_nCurPos;
}

bool FPDF_FileHandlerContext::ReadBlock(void* buffer,
                                        FX_FILESIZE offset,
                                        size_t size) {
  if (!buffer || !size || !m_pFS->ReadBlock)
    return false;

  if (m_pFS->ReadBlock(m_pFS->clientData, (FPDF_DWORD)offset, buffer,
                       (FPDF_DWORD)size) == 0) {
    m_nCurPos = offset + size;
    return true;
  }
  return false;
}

size_t FPDF_FileHandlerContext::ReadBlock(void* buffer, size_t size) {
  if (!buffer || !size || !m_pFS->ReadBlock)
    return 0;

  FX_FILESIZE nSize = GetSize();
  if (m_nCurPos >= nSize)
    return 0;
  FX_FILESIZE dwAvail = nSize - m_nCurPos;
  if (dwAvail < (FX_FILESIZE)size)
    size = static_cast<size_t>(dwAvail);
  if (m_pFS->ReadBlock(m_pFS->clientData, (FPDF_DWORD)m_nCurPos, buffer,
                       (FPDF_DWORD)size) == 0) {
    m_nCurPos += size;
    return size;
  }

  return 0;
}

bool FPDF_FileHandlerContext::WriteBlock(const void* buffer,
                                         FX_FILESIZE offset,
                                         size_t size) {
  if (!m_pFS || !m_pFS->WriteBlock)
    return false;

  if (m_pFS->WriteBlock(m_pFS->clientData, (FPDF_DWORD)offset, buffer,
                        (FPDF_DWORD)size) == 0) {
    m_nCurPos = offset + size;
    return true;
  }
  return false;
}

bool FPDF_FileHandlerContext::Flush() {
  if (!m_pFS || !m_pFS->Flush)
    return true;

  return m_pFS->Flush(m_pFS->clientData) == 0;
}
#endif  // PDF_ENABLE_XFA

}  // namespace

UnderlyingPageType* UnderlyingFromFPDFPage(FPDF_PAGE page) {
  return reinterpret_cast<UnderlyingPageType*>(page);
}

FPDF_PAGE FPDFPageFromUnderlying(UnderlyingPageType* page) {
  return reinterpret_cast<FPDF_PAGE>(page);
}

CPDF_Document* CPDFDocumentFromFPDFDocument(FPDF_DOCUMENT doc) {
  return reinterpret_cast<CPDF_Document*>(doc);
}

FPDF_DOCUMENT FPDFDocumentFromCPDFDocument(CPDF_Document* doc) {
#ifdef PDF_ENABLE_XFA
  if (doc && !doc->GetExtension())
    doc->SetExtension(new CPDFXFA_Context(pdfium::WrapUnique(doc)));
#endif  // PDF_ENABLE_XFA
  return reinterpret_cast<FPDF_DOCUMENT>(doc);
}

CPDF_Page* CPDFPageFromFPDFPage(FPDF_PAGE page) {
#ifdef PDF_ENABLE_XFA
  return page ? UnderlyingFromFPDFPage(page)->GetPDFPage() : nullptr;
#else   // PDF_ENABLE_XFA
  return UnderlyingFromFPDFPage(page);
#endif  // PDF_ENABLE_XFA
}

ByteString CFXByteStringFromFPDFWideString(FPDF_WIDESTRING wide_string) {
  return WideString::FromUTF16LE(wide_string,
                                 WideString::WStringLength(wide_string))
      .UTF8Encode();
}

void CheckUnSupportAnnot(CPDF_Document* pDoc, const CPDF_Annot* pPDFAnnot) {
  CPDF_Annot::Subtype nAnnotSubtype = pPDFAnnot->GetSubtype();
  if (nAnnotSubtype == CPDF_Annot::Subtype::THREED) {
    RaiseUnSupportError(FPDF_UNSP_ANNOT_3DANNOT);
  } else if (nAnnotSubtype == CPDF_Annot::Subtype::SCREEN) {
    const CPDF_Dictionary* pAnnotDict = pPDFAnnot->GetAnnotDict();
    ByteString cbString;
    if (pAnnotDict->KeyExist("IT"))
      cbString = pAnnotDict->GetStringFor("IT");
    if (cbString.Compare("Img") != 0)
      RaiseUnSupportError(FPDF_UNSP_ANNOT_SCREEN_MEDIA);
  } else if (nAnnotSubtype == CPDF_Annot::Subtype::MOVIE) {
    RaiseUnSupportError(FPDF_UNSP_ANNOT_MOVIE);
  } else if (nAnnotSubtype == CPDF_Annot::Subtype::SOUND) {
    RaiseUnSupportError(FPDF_UNSP_ANNOT_SOUND);
  } else if (nAnnotSubtype == CPDF_Annot::Subtype::RICHMEDIA) {
    RaiseUnSupportError(FPDF_UNSP_ANNOT_SCREEN_RICHMEDIA);
  } else if (nAnnotSubtype == CPDF_Annot::Subtype::FILEATTACHMENT) {
    RaiseUnSupportError(FPDF_UNSP_ANNOT_ATTACHMENT);
  } else if (nAnnotSubtype == CPDF_Annot::Subtype::WIDGET) {
    const CPDF_Dictionary* pAnnotDict = pPDFAnnot->GetAnnotDict();
    ByteString cbString;
    if (pAnnotDict->KeyExist("FT"))
      cbString = pAnnotDict->GetStringFor("FT");
    if (cbString.Compare("Sig") == 0)
      RaiseUnSupportError(FPDF_UNSP_ANNOT_SIG);
  }
}

void CheckUnSupportError(CPDF_Document* pDoc, uint32_t err_code) {
  // Security
  if (err_code == FPDF_ERR_SECURITY) {
    RaiseUnSupportError(FPDF_UNSP_DOC_SECURITY);
    return;
  }
  if (!pDoc)
    return;

  // Portfolios and Packages
  const CPDF_Dictionary* pRootDict = pDoc->GetRoot();
  if (pRootDict) {
    ByteString cbString;
    if (pRootDict->KeyExist("Collection")) {
      RaiseUnSupportError(FPDF_UNSP_DOC_PORTABLECOLLECTION);
      return;
    }
    if (pRootDict->KeyExist("Names")) {
      CPDF_Dictionary* pNameDict = pRootDict->GetDictFor("Names");
      if (pNameDict && pNameDict->KeyExist("EmbeddedFiles")) {
        RaiseUnSupportError(FPDF_UNSP_DOC_ATTACHMENT);
        return;
      }
      if (pNameDict && pNameDict->KeyExist("JavaScript")) {
        CPDF_Dictionary* pJSDict = pNameDict->GetDictFor("JavaScript");
        CPDF_Array* pArray = pJSDict ? pJSDict->GetArrayFor("Names") : nullptr;
        if (pArray) {
          for (size_t i = 0; i < pArray->GetCount(); i++) {
            ByteString cbStr = pArray->GetStringAt(i);
            if (cbStr.Compare("com.adobe.acrobat.SharedReview.Register") == 0) {
              RaiseUnSupportError(FPDF_UNSP_DOC_SHAREDREVIEW);
              return;
            }
          }
        }
      }
    }
  }

  // SharedForm
  const CPDF_Dictionary* pRoot = pDoc->GetRoot();
  if (pRoot) {
    CPDF_Stream* pStream = pRoot->GetStreamFor("Metadata");
    if (pStream) {
      CPDF_Metadata metaData(pStream);
      for (const auto& err : metaData.CheckForSharedForm())
        RaiseUnSupportError(static_cast<int>(err));
    }
  }

#ifndef PDF_ENABLE_XFA
  // XFA Forms
  CPDF_InterForm interform(pDoc);
  if (interform.HasXFAForm())
    RaiseUnSupportError(FPDF_UNSP_DOC_XFAFORM);
#endif  // PDF_ENABLE_XFA
}

#ifndef _WIN32
int g_LastError;
void SetLastError(int err) {
  g_LastError = err;
}

int GetLastError() {
  return g_LastError;
}
#endif  // _WIN32

void ProcessParseError(CPDF_Parser::Error err) {
  uint32_t err_code = FPDF_ERR_SUCCESS;
  // Translate FPDFAPI error code to FPDFVIEW error code
  switch (err) {
    case CPDF_Parser::SUCCESS:
      err_code = FPDF_ERR_SUCCESS;
      break;
    case CPDF_Parser::FILE_ERROR:
      err_code = FPDF_ERR_FILE;
      break;
    case CPDF_Parser::FORMAT_ERROR:
      err_code = FPDF_ERR_FORMAT;
      break;
    case CPDF_Parser::PASSWORD_ERROR:
      err_code = FPDF_ERR_PASSWORD;
      break;
    case CPDF_Parser::HANDLER_ERROR:
      err_code = FPDF_ERR_SECURITY;
      break;
  }
  SetLastError(err_code);
}

// 0 bit: FPDF_POLICY_MACHINETIME_ACCESS
static uint32_t foxit_sandbox_policy = 0xFFFFFFFF;

void FSDK_SetSandBoxPolicy(FPDF_DWORD policy, FPDF_BOOL enable) {
  switch (policy) {
    case FPDF_POLICY_MACHINETIME_ACCESS: {
      if (enable)
        foxit_sandbox_policy |= 0x01;
      else
        foxit_sandbox_policy &= 0xFFFFFFFE;
    } break;
    default:
      break;
  }
}

FPDF_BOOL FSDK_IsSandBoxPolicyEnabled(FPDF_DWORD policy) {
  switch (policy) {
    case FPDF_POLICY_MACHINETIME_ACCESS:
      return !!(foxit_sandbox_policy & 0x01);
    default:
      return false;
  }
}

unsigned long DecodeStreamMaybeCopyAndReturnLength(const CPDF_Stream* stream,
                                                   void* buffer,
                                                   unsigned long buflen) {
  ASSERT(stream);
  uint8_t* data = stream->GetRawData();
  uint32_t len = stream->GetRawSize();
  const CPDF_Dictionary* dict = stream->GetDict();
  const CPDF_Object* decoder =
      dict ? dict->GetDirectObjectFor("Filter") : nullptr;
  if (decoder && (decoder->IsArray() || decoder->IsName())) {
    // Decode the stream if one or more stream filters are specified.
    uint8_t* decoded_data = nullptr;
    uint32_t decoded_len = 0;
    ByteString dummy_last_decoder;
    CPDF_Dictionary* dummy_last_param;
    if (PDF_DataDecode(data, len, dict,
                       dict->GetIntegerFor(pdfium::stream::kDL), false,
                       &decoded_data, &decoded_len, &dummy_last_decoder,
                       &dummy_last_param)) {
      if (buffer && buflen >= decoded_len)
        memcpy(buffer, decoded_data, decoded_len);

      // Free the buffer for the decoded data if it was allocated by
      // PDF_DataDecode(). Note that for images with a single image-specific
      // filter, |decoded_data| is directly assigned to be |data|, so
      // |decoded_data| does not need to be freed.
      if (decoded_data != data)
        FX_Free(decoded_data);

      return decoded_len;
    }
  }
  // Copy the raw data and return its length if there is no valid filter
  // specified or if decoding failed.
  if (buffer && buflen >= len)
    memcpy(buffer, data, len);

  return len;
}

unsigned long Utf16EncodeMaybeCopyAndReturnLength(const WideString& text,
                                                  void* buffer,
                                                  unsigned long buflen) {
  ByteString encoded_text = text.UTF16LE_Encode();
  unsigned long len = encoded_text.GetLength();
  if (buffer && len <= buflen)
    memcpy(buffer, encoded_text.c_str(), len);
  return len;
}

void FSRECTFFromCFXFloatRect(const CFX_FloatRect& rect, FS_RECTF* out_rect) {
  out_rect->left = rect.left;
  out_rect->top = rect.top;
  out_rect->right = rect.right;
  out_rect->bottom = rect.bottom;
}

CFX_FloatRect CFXFloatRectFromFSRECTF(const FS_RECTF& rect) {
  return CFX_FloatRect(rect.left, rect.bottom, rect.right, rect.top);
}

CPDF_Array* GetQuadPointsArrayFromDictionary(const CPDF_Dictionary* dict) {
  return dict ? dict->GetArrayFor("QuadPoints") : nullptr;
}

CPDF_Array* AddQuadPointsArrayToDictionary(CPDF_Dictionary* dict) {
  if (!dict)
    return nullptr;
  return dict->SetNewFor<CPDF_Array>(kQuadPoints);
}

bool IsValidQuadPointsIndex(const CPDF_Array* array, size_t index) {
  return array && index < array->GetCount() / 8;
}

bool GetQuadPointsAtIndex(const CPDF_Array* array,
                          size_t quad_index,
                          FS_QUADPOINTSF* quad_points) {
  ASSERT(quad_points);
  ASSERT(array);

  if (!IsValidQuadPointsIndex(array, quad_index))
    return false;

  quad_index *= 8;
  quad_points->x1 = array->GetNumberAt(quad_index);
  quad_points->y1 = array->GetNumberAt(quad_index + 1);
  quad_points->x2 = array->GetNumberAt(quad_index + 2);
  quad_points->y2 = array->GetNumberAt(quad_index + 3);
  quad_points->x3 = array->GetNumberAt(quad_index + 4);
  quad_points->y3 = array->GetNumberAt(quad_index + 5);
  quad_points->x4 = array->GetNumberAt(quad_index + 6);
  quad_points->y4 = array->GetNumberAt(quad_index + 7);
  return true;
}

bool GetQuadPointsFromDictionary(CPDF_Dictionary* dict,
                                 size_t quad_index,
                                 FS_QUADPOINTSF* quad_points) {
  ASSERT(quad_points);

  const CPDF_Array* pArray = GetQuadPointsArrayFromDictionary(dict);
  if (!pArray || quad_index >= pArray->GetCount() / 8)
    return false;

  quad_index *= 8;
  quad_points->x1 = pArray->GetNumberAt(quad_index);
  quad_points->y1 = pArray->GetNumberAt(quad_index + 1);
  quad_points->x2 = pArray->GetNumberAt(quad_index + 2);
  quad_points->y2 = pArray->GetNumberAt(quad_index + 3);
  quad_points->x3 = pArray->GetNumberAt(quad_index + 4);
  quad_points->y3 = pArray->GetNumberAt(quad_index + 5);
  quad_points->x4 = pArray->GetNumberAt(quad_index + 6);
  quad_points->y4 = pArray->GetNumberAt(quad_index + 7);
  return true;
}

#ifdef PDF_ENABLE_XFA
RetainPtr<IFX_SeekableStream> MakeSeekableStream(
    FPDF_FILEHANDLER* pFilehandler) {
  return pdfium::MakeRetain<FPDF_FileHandlerContext>(pFilehandler);
}
#endif  // PDF_ENABLE_XFA
