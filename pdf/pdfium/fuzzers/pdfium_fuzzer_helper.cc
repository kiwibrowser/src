// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This fuzzer is simplified & cleaned up pdfium/samples/pdfium_test.cc

#include "pdf/pdfium/fuzzers/pdfium_fuzzer_helper.h"

#include <assert.h>
#include <limits.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else  // Linux
#include <unistd.h>
#endif  // _WIN32

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "base/memory/free_deleter.h"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_dataavail.h"
#include "third_party/pdfium/public/fpdf_text.h"
#include "third_party/pdfium/testing/test_support.h"
#include "v8/include/v8-platform.h"

namespace {

int ExampleAppAlert(IPDF_JSPLATFORM*,
                    FPDF_WIDESTRING,
                    FPDF_WIDESTRING,
                    int,
                    int) {
  return 0;
}

int ExampleAppResponse(IPDF_JSPLATFORM*,
                       FPDF_WIDESTRING question,
                       FPDF_WIDESTRING title,
                       FPDF_WIDESTRING default_value,
                       FPDF_WIDESTRING label,
                       FPDF_BOOL is_password,
                       void* response,
                       int length) {
  // UTF-16, always LE regardless of platform.
  uint8_t* ptr = static_cast<uint8_t*>(response);
  ptr[0] = 'N';
  ptr[1] = 0;
  ptr[2] = 'o';
  ptr[3] = 0;
  return 4;
}

void ExampleDocGotoPage(IPDF_JSPLATFORM*, int pageNumber) {}

void ExampleDocMail(IPDF_JSPLATFORM*,
                    void* mailData,
                    int length,
                    FPDF_BOOL UI,
                    FPDF_WIDESTRING To,
                    FPDF_WIDESTRING Subject,
                    FPDF_WIDESTRING CC,
                    FPDF_WIDESTRING BCC,
                    FPDF_WIDESTRING Msg) {}

void ExampleUnsupportedHandler(UNSUPPORT_INFO*, int type) {}

FPDF_BOOL Is_Data_Avail(FX_FILEAVAIL* pThis, size_t offset, size_t size) {
  return true;
}

void Add_Segment(FX_DOWNLOADHINTS* pThis, size_t offset, size_t size) {}

std::string ProgramPath() {
  std::string result;

#ifdef _WIN32
  wchar_t wpath[MAX_PATH];
  char path[MAX_PATH];
  DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
  if (len != 0)
    result = std::string(path, len);
#elif defined(__APPLE__)
  char path[PATH_MAX];
  unsigned int len = PATH_MAX;
  if (!_NSGetExecutablePath(path, &len)) {
    std::unique_ptr<char, base::FreeDeleter> resolved_path(
        realpath(path, nullptr));
    if (resolved_path.get())
      result = std::string(resolved_path.get());
  }
#else  // Linux
  char path[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
  if (len > 0)
    result = std::string(path, len);
#endif
  return result;
}

}  // namespace

PDFiumFuzzerHelper::PDFiumFuzzerHelper() = default;

PDFiumFuzzerHelper::~PDFiumFuzzerHelper() = default;

bool PDFiumFuzzerHelper::OnFormFillEnvLoaded(FPDF_DOCUMENT doc) {
  return true;
}

void PDFiumFuzzerHelper::RenderPdf(const char* pBuf, size_t len) {
  IPDF_JSPLATFORM platform_callbacks;
  memset(&platform_callbacks, '\0', sizeof(platform_callbacks));
  platform_callbacks.version = 3;
  platform_callbacks.app_alert = ExampleAppAlert;
  platform_callbacks.app_response = ExampleAppResponse;
  platform_callbacks.Doc_gotoPage = ExampleDocGotoPage;
  platform_callbacks.Doc_mail = ExampleDocMail;

  FPDF_FORMFILLINFO form_callbacks;
  memset(&form_callbacks, '\0', sizeof(form_callbacks));
  form_callbacks.version = GetFormCallbackVersion();
  form_callbacks.m_pJsPlatform = &platform_callbacks;

  TestLoader loader(pBuf, len);
  FPDF_FILEACCESS file_access;
  memset(&file_access, '\0', sizeof(file_access));
  file_access.m_FileLen = static_cast<unsigned long>(len);
  file_access.m_GetBlock = TestLoader::GetBlock;
  file_access.m_Param = &loader;

  FX_FILEAVAIL file_avail;
  memset(&file_avail, '\0', sizeof(file_avail));
  file_avail.version = 1;
  file_avail.IsDataAvail = Is_Data_Avail;

  FX_DOWNLOADHINTS hints;
  memset(&hints, '\0', sizeof(hints));
  hints.version = 1;
  hints.AddSegment = Add_Segment;

  ScopedFPDFAvail pdf_avail(FPDFAvail_Create(&file_avail, &file_access));

  int nRet = PDF_DATA_NOTAVAIL;
  bool bIsLinearized = false;
  ScopedFPDFDocument doc;
  if (FPDFAvail_IsLinearized(pdf_avail.get()) == PDF_LINEARIZED) {
    doc.reset(FPDFAvail_GetDocument(pdf_avail.get(), nullptr));
    if (doc) {
      while (nRet == PDF_DATA_NOTAVAIL)
        nRet = FPDFAvail_IsDocAvail(pdf_avail.get(), &hints);

      if (nRet == PDF_DATA_ERROR)
        return;

      nRet = FPDFAvail_IsFormAvail(pdf_avail.get(), &hints);
      if (nRet == PDF_FORM_ERROR || nRet == PDF_FORM_NOTAVAIL)
        return;

      bIsLinearized = true;
    }
  } else {
    doc.reset(FPDF_LoadCustomDocument(&file_access, nullptr));
  }

  if (!doc)
    return;

  (void)FPDF_GetDocPermissions(doc.get());

  ScopedFPDFFormHandle form(
      FPDFDOC_InitFormFillEnvironment(doc.get(), &form_callbacks));
  if (!OnFormFillEnvLoaded(doc.get()))
    return;

  FPDF_SetFormFieldHighlightColor(form.get(), FPDF_FORMFIELD_UNKNOWN, 0xFFE4DD);
  FPDF_SetFormFieldHighlightAlpha(form.get(), 100);
  FORM_DoDocumentJSAction(form.get());
  FORM_DoDocumentOpenAction(form.get());

  int page_count = FPDF_GetPageCount(doc.get());
  for (int i = 0; i < page_count; ++i) {
    if (bIsLinearized) {
      nRet = PDF_DATA_NOTAVAIL;
      while (nRet == PDF_DATA_NOTAVAIL)
        nRet = FPDFAvail_IsPageAvail(pdf_avail.get(), i, &hints);

      if (nRet == PDF_DATA_ERROR)
        return;
    }
    RenderPage(doc.get(), form.get(), i);
  }
  FORM_DoDocumentAAction(form.get(), FPDFDOC_AACTION_WC);
}

bool PDFiumFuzzerHelper::RenderPage(FPDF_DOCUMENT doc,
                                    FPDF_FORMHANDLE form,
                                    const int page_index) {
  ScopedFPDFPage page(FPDF_LoadPage(doc, page_index));
  if (!page)
    return false;

  ScopedFPDFTextPage text_page(FPDFText_LoadPage(page.get()));
  FORM_OnAfterLoadPage(page.get(), form);
  FORM_DoPageAAction(page.get(), form, FPDFPAGE_AACTION_OPEN);

  const double scale = 1.0;
  int width = static_cast<int>(FPDF_GetPageWidth(page.get()) * scale);
  int height = static_cast<int>(FPDF_GetPageHeight(page.get()) * scale);
  ScopedFPDFBitmap bitmap(FPDFBitmap_Create(width, height, 0));
  if (bitmap) {
    FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap.get(), page.get(), 0, 0, width, height, 0, 0);
    FPDF_FFLDraw(form, bitmap.get(), page.get(), 0, 0, width, height, 0, 0);
  }
  FORM_DoPageAAction(page.get(), form, FPDFPAGE_AACTION_CLOSE);
  FORM_OnBeforeClosePage(page.get(), form);
  return !!bitmap;
}

// Initialize the library once for all runs of the fuzzer.
struct TestCase {
  TestCase() {
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
    platform = InitializeV8ForPDFiumWithStartupData(
        ProgramPath(), "", &natives_blob, &snapshot_blob);
#else
    platform = InitializeV8ForPDFium(ProgramPath());
#endif

    memset(&config, '\0', sizeof(config));
    config.version = 2;
    config.m_pUserFontPaths = nullptr;
    config.m_pIsolate = nullptr;
    config.m_v8EmbedderSlot = 0;
    FPDF_InitLibraryWithConfig(&config);

    memset(&unsupport_info, '\0', sizeof(unsupport_info));
    unsupport_info.version = 1;
    unsupport_info.FSDK_UnSupport_Handler = ExampleUnsupportedHandler;
    FSDK_SetUnSpObjProcessHandler(&unsupport_info);
  }

  std::unique_ptr<v8::Platform> platform;
  v8::StartupData natives_blob;
  v8::StartupData snapshot_blob;
  FPDF_LIBRARY_CONFIG config;
  UNSUPPORT_INFO unsupport_info;
};

static TestCase* test_case = new TestCase();
