// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test.h"

#include <limits.h>

#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/fdrm/crypto/fx_crypt.h"
#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_text.h"
#include "public/fpdfview.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/image_diff/image_diff_png.h"
#include "testing/test_support.h"
#include "testing/utils/path_service.h"
#include "third_party/base/logging.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"

#ifdef PDF_ENABLE_V8
#include "v8/include/v8-platform.h"
#include "v8/include/v8.h"
#endif  // PDF_ENABLE_V8

namespace {

int GetBitmapBytesPerPixel(FPDF_BITMAP bitmap) {
  const int format = FPDFBitmap_GetFormat(bitmap);
  switch (format) {
    case FPDFBitmap_Gray:
      return 1;
    case FPDFBitmap_BGR:
      return 3;
    case FPDFBitmap_BGRx:
    case FPDFBitmap_BGRA:
      return 4;
    default:
      ASSERT(false);
      return 0;
  }
}

}  // namespace

EmbedderTest::EmbedderTest()
    : default_delegate_(pdfium::MakeUnique<EmbedderTest::Delegate>()),
      delegate_(default_delegate_.get()) {
  FPDF_FILEWRITE::version = 1;
  FPDF_FILEWRITE::WriteBlock = WriteBlockCallback;
}

EmbedderTest::~EmbedderTest() {}

void EmbedderTest::SetUp() {
  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = nullptr;
  config.m_v8EmbedderSlot = 0;
  config.m_pIsolate = external_isolate_;
  FPDF_InitLibraryWithConfig(&config);

  UNSUPPORT_INFO* info = static_cast<UNSUPPORT_INFO*>(this);
  memset(info, 0, sizeof(UNSUPPORT_INFO));
  info->version = 1;
  info->FSDK_UnSupport_Handler = UnsupportedHandlerTrampoline;
  FSDK_SetUnSpObjProcessHandler(info);

  saved_document_ = nullptr;
}

void EmbedderTest::TearDown() {
  // Use an EXPECT_EQ() here and continue to let TearDown() finish as cleanly as
  // possible. This can fail when an ASSERT test fails in a test case.
  EXPECT_EQ(0U, page_map_.size());
  EXPECT_EQ(0U, saved_page_map_.size());

  if (document_) {
    FORM_DoDocumentAAction(form_handle_, FPDFDOC_AACTION_WC);
    FPDFDOC_ExitFormFillEnvironment(form_handle_);
    FPDF_CloseDocument(document_);
  }

  FPDFAvail_Destroy(avail_);
  FPDF_DestroyLibrary();
  delete loader_;
}

bool EmbedderTest::CreateEmptyDocument() {
  document_ = FPDF_CreateNewDocument();
  if (!document_)
    return false;

  form_handle_ =
      SetupFormFillEnvironment(document_, JavaScriptOption::kEnableJavaScript);
  return true;
}

bool EmbedderTest::OpenDocument(const std::string& filename) {
  return OpenDocumentWithOptions(filename, nullptr,
                                 LinearizeOption::kDefaultLinearize,
                                 JavaScriptOption::kEnableJavaScript);
}

bool EmbedderTest::OpenDocumentLinearized(const std::string& filename) {
  return OpenDocumentWithOptions(filename, nullptr,
                                 LinearizeOption::kMustLinearize,
                                 JavaScriptOption::kEnableJavaScript);
}

bool EmbedderTest::OpenDocumentWithPassword(const std::string& filename,
                                            const char* password) {
  return OpenDocumentWithOptions(filename, password,
                                 LinearizeOption::kDefaultLinearize,
                                 JavaScriptOption::kEnableJavaScript);
}

bool EmbedderTest::OpenDocumentWithoutJavaScript(const std::string& filename) {
  return OpenDocumentWithOptions(filename, nullptr,
                                 LinearizeOption::kDefaultLinearize,
                                 JavaScriptOption::kDisableJavaScript);
}

bool EmbedderTest::OpenDocumentWithOptions(const std::string& filename,
                                           const char* password,
                                           LinearizeOption linearize_option,
                                           JavaScriptOption javascript_option) {
  std::string file_path;
  if (!PathService::GetTestFilePath(filename, &file_path))
    return false;

  file_contents_ = GetFileContents(file_path.c_str(), &file_length_);
  if (!file_contents_)
    return false;

  EXPECT_TRUE(!loader_);
  loader_ = new TestLoader(file_contents_.get(), file_length_);

  memset(&file_access_, 0, sizeof(file_access_));
  file_access_.m_FileLen = static_cast<unsigned long>(file_length_);
  file_access_.m_GetBlock = TestLoader::GetBlock;
  file_access_.m_Param = loader_;

  fake_file_access_ = pdfium::MakeUnique<FakeFileAccess>(&file_access_);
  return OpenDocumentHelper(password, linearize_option, javascript_option,
                            fake_file_access_.get(), &document_, &avail_,
                            &form_handle_);
}

bool EmbedderTest::OpenDocumentHelper(const char* password,
                                      LinearizeOption linearize_option,
                                      JavaScriptOption javascript_option,
                                      FakeFileAccess* network_simulator,
                                      FPDF_DOCUMENT* document,
                                      FPDF_AVAIL* avail,
                                      FPDF_FORMHANDLE* form_handle) {
  network_simulator->AddSegment(0, 1024);
  network_simulator->SetRequestedDataAvailable();
  *avail = FPDFAvail_Create(network_simulator->GetFileAvail(),
                            network_simulator->GetFileAccess());
  if (FPDFAvail_IsLinearized(*avail) == PDF_LINEARIZED) {
    int32_t nRet = PDF_DATA_NOTAVAIL;
    while (nRet == PDF_DATA_NOTAVAIL) {
      network_simulator->SetRequestedDataAvailable();
      nRet =
          FPDFAvail_IsDocAvail(*avail, network_simulator->GetDownloadHints());
    }
    if (nRet == PDF_DATA_ERROR)
      return false;

    *document = FPDFAvail_GetDocument(*avail, password);
    if (!*document)
      return false;

    nRet = PDF_DATA_NOTAVAIL;
    while (nRet == PDF_DATA_NOTAVAIL) {
      network_simulator->SetRequestedDataAvailable();
      nRet =
          FPDFAvail_IsFormAvail(*avail, network_simulator->GetDownloadHints());
    }
    if (nRet == PDF_FORM_ERROR)
      return false;

    int page_count = FPDF_GetPageCount(*document);
    for (int i = 0; i < page_count; ++i) {
      nRet = PDF_DATA_NOTAVAIL;
      while (nRet == PDF_DATA_NOTAVAIL) {
        network_simulator->SetRequestedDataAvailable();
        nRet = FPDFAvail_IsPageAvail(*avail, i,
                                     network_simulator->GetDownloadHints());
      }

      if (nRet == PDF_DATA_ERROR)
        return false;
    }
  } else {
    if (linearize_option == LinearizeOption::kMustLinearize)
      return false;
    network_simulator->SetWholeFileAvailable();
    *document =
        FPDF_LoadCustomDocument(network_simulator->GetFileAccess(), password);
    if (!*document)
      return false;
  }
  *form_handle = SetupFormFillEnvironment(*document, javascript_option);

#ifdef PDF_ENABLE_XFA
  int doc_type = FPDF_GetFormType(*document);
  if (doc_type == FORMTYPE_XFA_FULL || doc_type == FORMTYPE_XFA_FOREGROUND)
    FPDF_LoadXFA(*document);
#endif  // PDF_ENABLE_XFA

  (void)FPDF_GetDocPermissions(*document);
  return true;
}

FPDF_FORMHANDLE EmbedderTest::SetupFormFillEnvironment(
    FPDF_DOCUMENT doc,
    JavaScriptOption javascript_option) {
  IPDF_JSPLATFORM* platform = static_cast<IPDF_JSPLATFORM*>(this);
  memset(platform, '\0', sizeof(IPDF_JSPLATFORM));
  platform->version = 2;
  platform->app_alert = AlertTrampoline;
  platform->m_isolate = external_isolate_;

  FPDF_FORMFILLINFO* formfillinfo = static_cast<FPDF_FORMFILLINFO*>(this);
  memset(formfillinfo, 0, sizeof(FPDF_FORMFILLINFO));
#ifdef PDF_ENABLE_XFA
  formfillinfo->version = 2;
#else   // PDF_ENABLE_XFA
  formfillinfo->version = 1;
#endif  // PDF_ENABLE_XFA
  formfillinfo->FFI_SetTimer = SetTimerTrampoline;
  formfillinfo->FFI_KillTimer = KillTimerTrampoline;
  formfillinfo->FFI_GetPage = GetPageTrampoline;
  if (javascript_option == JavaScriptOption::kEnableJavaScript)
    formfillinfo->m_pJsPlatform = platform;

  FPDF_FORMHANDLE form_handle =
      FPDFDOC_InitFormFillEnvironment(doc, formfillinfo);
  FPDF_SetFormFieldHighlightColor(form_handle, FPDF_FORMFIELD_UNKNOWN,
                                  0xFFE4DD);
  FPDF_SetFormFieldHighlightAlpha(form_handle, 100);
  return form_handle;
}

void EmbedderTest::DoOpenActions() {
  ASSERT(form_handle_);
  FORM_DoDocumentJSAction(form_handle_);
  FORM_DoDocumentOpenAction(form_handle_);
}

int EmbedderTest::GetFirstPageNum() {
  int first_page = FPDFAvail_GetFirstPageNum(document_);
  (void)FPDFAvail_IsPageAvail(avail_, first_page,
                              fake_file_access_->GetDownloadHints());
  return first_page;
}

int EmbedderTest::GetPageCount() {
  int page_count = FPDF_GetPageCount(document_);
  for (int i = 0; i < page_count; ++i)
    (void)FPDFAvail_IsPageAvail(avail_, i,
                                fake_file_access_->GetDownloadHints());
  return page_count;
}

FPDF_PAGE EmbedderTest::LoadPage(int page_number) {
  ASSERT(form_handle_);
  ASSERT(page_number >= 0);
  ASSERT(!pdfium::ContainsKey(page_map_, page_number));

  FPDF_PAGE page = FPDF_LoadPage(document_, page_number);
  if (!page)
    return nullptr;

  FORM_OnAfterLoadPage(page, form_handle_);
  FORM_DoPageAAction(page, form_handle_, FPDFPAGE_AACTION_OPEN);
  page_map_[page_number] = page;
  return page;
}

void EmbedderTest::UnloadPage(FPDF_PAGE page) {
  ASSERT(form_handle_);

  int page_number = GetPageNumberForLoadedPage(page);
  if (page_number < 0) {
    NOTREACHED();
    return;
  }

  FORM_DoPageAAction(page, form_handle_, FPDFPAGE_AACTION_CLOSE);
  FORM_OnBeforeClosePage(page, form_handle_);
  FPDF_ClosePage(page);

  page_map_.erase(page_number);
}

ScopedFPDFBitmap EmbedderTest::RenderLoadedPage(FPDF_PAGE page) {
  return RenderLoadedPageWithFlags(page, 0);
}

ScopedFPDFBitmap EmbedderTest::RenderLoadedPageWithFlags(FPDF_PAGE page,
                                                         int flags) {
  if (GetPageNumberForLoadedPage(page) < 0) {
    NOTREACHED();
    return nullptr;
  }
  return RenderPageWithFlags(page, form_handle_, flags);
}

ScopedFPDFBitmap EmbedderTest::RenderSavedPage(FPDF_PAGE page) {
  return RenderSavedPageWithFlags(page, 0);
}

ScopedFPDFBitmap EmbedderTest::RenderSavedPageWithFlags(FPDF_PAGE page,
                                                        int flags) {
  if (GetPageNumberForSavedPage(page) < 0) {
    NOTREACHED();
    return nullptr;
  }
  return RenderPageWithFlags(page, saved_form_handle_, flags);
}

// static
ScopedFPDFBitmap EmbedderTest::RenderPageWithFlags(FPDF_PAGE page,
                                                   FPDF_FORMHANDLE handle,
                                                   int flags) {
  int width = static_cast<int>(FPDF_GetPageWidth(page));
  int height = static_cast<int>(FPDF_GetPageHeight(page));
  int alpha = FPDFPage_HasTransparency(page) ? 1 : 0;
  ScopedFPDFBitmap bitmap(FPDFBitmap_Create(width, height, alpha));
  FPDF_DWORD fill_color = alpha ? 0x00000000 : 0xFFFFFFFF;
  FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, fill_color);
  FPDF_RenderPageBitmap(bitmap.get(), page, 0, 0, width, height, 0, flags);
  FPDF_FFLDraw(handle, bitmap.get(), page, 0, 0, width, height, 0, flags);
  return bitmap;
}

FPDF_DOCUMENT EmbedderTest::OpenSavedDocument(const char* password) {
  memset(&saved_file_access_, 0, sizeof(saved_file_access_));
  saved_file_access_.m_FileLen = data_string_.size();
  saved_file_access_.m_GetBlock = GetBlockFromString;
  saved_file_access_.m_Param = &data_string_;

  saved_fake_file_access_ =
      pdfium::MakeUnique<FakeFileAccess>(&saved_file_access_);

  EXPECT_TRUE(OpenDocumentHelper(
      password, LinearizeOption::kDefaultLinearize,
      JavaScriptOption::kEnableJavaScript, saved_fake_file_access_.get(),
      &saved_document_, &saved_avail_, &saved_form_handle_));
  return saved_document_;
}

void EmbedderTest::CloseSavedDocument() {
  ASSERT(saved_document_);

  FPDFDOC_ExitFormFillEnvironment(saved_form_handle_);
  FPDF_CloseDocument(saved_document_);
  FPDFAvail_Destroy(saved_avail_);

  saved_form_handle_ = nullptr;
  saved_document_ = nullptr;
  saved_avail_ = nullptr;
}

FPDF_PAGE EmbedderTest::LoadSavedPage(int page_number) {
  ASSERT(saved_form_handle_);
  ASSERT(page_number >= 0);
  ASSERT(!pdfium::ContainsKey(saved_page_map_, page_number));

  FPDF_PAGE page = FPDF_LoadPage(saved_document_, page_number);
  if (!page)
    return nullptr;

  FORM_OnAfterLoadPage(page, saved_form_handle_);
  FORM_DoPageAAction(page, saved_form_handle_, FPDFPAGE_AACTION_OPEN);
  saved_page_map_[page_number] = page;
  return page;
}

void EmbedderTest::CloseSavedPage(FPDF_PAGE page) {
  ASSERT(saved_form_handle_);

  int page_number = GetPageNumberForSavedPage(page);
  if (page_number < 0) {
    NOTREACHED();
    return;
  }

  FORM_DoPageAAction(page, saved_form_handle_, FPDFPAGE_AACTION_CLOSE);
  FORM_OnBeforeClosePage(page, saved_form_handle_);
  FPDF_ClosePage(page);

  saved_page_map_.erase(page_number);
}

void EmbedderTest::VerifySavedRendering(FPDF_PAGE page,
                                        int width,
                                        int height,
                                        const char* md5) {
  ASSERT(saved_document_);
  ASSERT(page);

  ScopedFPDFBitmap bitmap = RenderSavedPageWithFlags(page, FPDF_ANNOT);
  CompareBitmap(bitmap.get(), width, height, md5);
}

void EmbedderTest::VerifySavedDocument(int width, int height, const char* md5) {
  OpenSavedDocument();
  FPDF_PAGE page = LoadSavedPage(0);
  VerifySavedRendering(page, width, height, md5);
  CloseSavedPage(page);
  CloseSavedDocument();
}

void EmbedderTest::SetWholeFileAvailable() {
  ASSERT(fake_file_access_);
  fake_file_access_->SetWholeFileAvailable();
}

FPDF_PAGE EmbedderTest::Delegate::GetPage(FPDF_FORMFILLINFO* info,
                                          FPDF_DOCUMENT document,
                                          int page_index) {
  EmbedderTest* test = static_cast<EmbedderTest*>(info);
  auto it = test->page_map_.find(page_index);
  return it != test->page_map_.end() ? it->second : nullptr;
}

// static
void EmbedderTest::UnsupportedHandlerTrampoline(UNSUPPORT_INFO* info,
                                                int type) {
  EmbedderTest* test = static_cast<EmbedderTest*>(info);
  test->delegate_->UnsupportedHandler(type);
}

// static
int EmbedderTest::AlertTrampoline(IPDF_JSPLATFORM* platform,
                                  FPDF_WIDESTRING message,
                                  FPDF_WIDESTRING title,
                                  int type,
                                  int icon) {
  EmbedderTest* test = static_cast<EmbedderTest*>(platform);
  return test->delegate_->Alert(message, title, type, icon);
}

// static
int EmbedderTest::SetTimerTrampoline(FPDF_FORMFILLINFO* info,
                                     int msecs,
                                     TimerCallback fn) {
  EmbedderTest* test = static_cast<EmbedderTest*>(info);
  return test->delegate_->SetTimer(msecs, fn);
}

// static
void EmbedderTest::KillTimerTrampoline(FPDF_FORMFILLINFO* info, int id) {
  EmbedderTest* test = static_cast<EmbedderTest*>(info);
  return test->delegate_->KillTimer(id);
}

// static
FPDF_PAGE EmbedderTest::GetPageTrampoline(FPDF_FORMFILLINFO* info,
                                          FPDF_DOCUMENT document,
                                          int page_index) {
  return static_cast<EmbedderTest*>(info)->delegate_->GetPage(info, document,
                                                              page_index);
}

// static
std::string EmbedderTest::HashBitmap(FPDF_BITMAP bitmap) {
  uint8_t digest[16];
  CRYPT_MD5Generate(static_cast<uint8_t*>(FPDFBitmap_GetBuffer(bitmap)),
                    FPDFBitmap_GetWidth(bitmap) *
                        GetBitmapBytesPerPixel(bitmap) *
                        FPDFBitmap_GetHeight(bitmap),
                    digest);
  return CryptToBase16(digest);
}

#ifndef NDEBUG
// static
void EmbedderTest::WriteBitmapToPng(FPDF_BITMAP bitmap,
                                    const std::string& filename) {
  const int stride = FPDFBitmap_GetStride(bitmap);
  const int width = FPDFBitmap_GetWidth(bitmap);
  const int height = FPDFBitmap_GetHeight(bitmap);
  const auto* buffer =
      static_cast<const unsigned char*>(FPDFBitmap_GetBuffer(bitmap));

  std::vector<unsigned char> png_encoding;
  bool encoded;
  if (FPDFBitmap_GetFormat(bitmap) == FPDFBitmap_Gray) {
    encoded = image_diff_png::EncodeGrayPNG(buffer, width, height, stride,
                                            &png_encoding);
  } else {
    encoded = image_diff_png::EncodeBGRAPNG(buffer, width, height, stride,
                                            /*discard_transparency=*/false,
                                            &png_encoding);
  }

  ASSERT_TRUE(encoded);
  ASSERT_LT(filename.size(), 256u);

  std::ofstream png_file;
  png_file.open(filename, std::ios_base::out | std::ios_base::binary);
  png_file.write(reinterpret_cast<char*>(&png_encoding.front()),
                 png_encoding.size());
  ASSERT_TRUE(png_file.good());
  png_file.close();
}
#endif

// static
void EmbedderTest::CompareBitmap(FPDF_BITMAP bitmap,
                                 int expected_width,
                                 int expected_height,
                                 const char* expected_md5sum) {
  ASSERT_EQ(expected_width, FPDFBitmap_GetWidth(bitmap));
  ASSERT_EQ(expected_height, FPDFBitmap_GetHeight(bitmap));

  // The expected stride is calculated using the same formula as in
  // CFX_DIBitmap::CalculatePitchAndSize(), which sets the bitmap stride.
  const int expected_stride =
      (expected_width * GetBitmapBytesPerPixel(bitmap) * 8 + 31) / 32 * 4;
  ASSERT_EQ(expected_stride, FPDFBitmap_GetStride(bitmap));

  if (!expected_md5sum)
    return;

  EXPECT_EQ(expected_md5sum, HashBitmap(bitmap));
}

// static
int EmbedderTest::WriteBlockCallback(FPDF_FILEWRITE* pFileWrite,
                                     const void* data,
                                     unsigned long size) {
  EmbedderTest* pThis = static_cast<EmbedderTest*>(pFileWrite);
  pThis->data_string_.append(static_cast<const char*>(data), size);
  return 1;
}

// static
int EmbedderTest::GetBlockFromString(void* param,
                                     unsigned long pos,
                                     unsigned char* buf,
                                     unsigned long size) {
  std::string* new_file = static_cast<std::string*>(param);
  if (!new_file || pos + size < pos)
    return 0;

  unsigned long file_size = new_file->size();
  if (pos + size > file_size)
    return 0;

  memcpy(buf, new_file->data() + pos, size);
  return 1;
}

// static
int EmbedderTest::GetPageNumberForPage(const PageNumberToHandleMap& page_map,
                                       FPDF_PAGE page) {
  for (const auto& it : page_map) {
    if (it.second == page) {
      int page_number = it.first;
      ASSERT(page_number >= 0);
      return page_number;
    }
  }
  return -1;
}

int EmbedderTest::GetPageNumberForLoadedPage(FPDF_PAGE page) const {
  return GetPageNumberForPage(page_map_, page);
}

int EmbedderTest::GetPageNumberForSavedPage(FPDF_PAGE page) const {
  return GetPageNumberForPage(saved_page_map_, page);
}
