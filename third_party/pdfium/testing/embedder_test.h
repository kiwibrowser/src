// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_EMBEDDER_TEST_H_
#define TESTING_EMBEDDER_TEST_H_

#include <map>
#include <memory>
#include <string>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_ext.h"
#include "public/fpdf_formfill.h"
#include "public/fpdf_save.h"
#include "public/fpdfview.h"
#include "testing/fake_file_access.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"

#ifdef PDF_ENABLE_V8
#include "v8/include/v8.h"
#endif  // PDF_ENABLE_v8

class TestLoader;

// This class is used to load a PDF document, and then run programatic
// API tests against it.
class EmbedderTest : public ::testing::Test,
                     public UNSUPPORT_INFO,
                     public IPDF_JSPLATFORM,
                     public FPDF_FORMFILLINFO,
                     public FPDF_FILEWRITE {
 public:
  enum class LinearizeOption { kDefaultLinearize, kMustLinearize };
  enum class JavaScriptOption { kDisableJavaScript, kEnableJavaScript };

  class Delegate {
   public:
    virtual ~Delegate() {}

    // Equivalent to UNSUPPORT_INFO::FSDK_UnSupport_Handler().
    virtual void UnsupportedHandler(int type) {}

    // Equivalent to IPDF_JSPLATFORM::app_alert().
    virtual int Alert(FPDF_WIDESTRING message,
                      FPDF_WIDESTRING title,
                      int type,
                      int icon) {
      return 0;
    }

    // Equivalent to FPDF_FORMFILLINFO::FFI_SetTimer().
    virtual int SetTimer(int msecs, TimerCallback fn) { return 0; }

    // Equivalent to FPDF_FORMFILLINFO::FFI_KillTimer().
    virtual void KillTimer(int id) {}

    // Equivalent to FPDF_FORMFILLINFO::FFI_GetPage().
    virtual FPDF_PAGE GetPage(FPDF_FORMFILLINFO* info,
                              FPDF_DOCUMENT document,
                              int page_index);
  };

  EmbedderTest();
  virtual ~EmbedderTest();

  void SetUp() override;
  void TearDown() override;

#ifdef PDF_ENABLE_V8
  // Call before SetUp to pass shared isolate, otherwise PDFium creates one.
  void SetExternalIsolate(void* isolate) {
    external_isolate_ = static_cast<v8::Isolate*>(isolate);
  }
#endif  // PDF_ENABLE_V8

  void SetDelegate(Delegate* delegate) {
    delegate_ = delegate ? delegate : default_delegate_.get();
  }

  FPDF_DOCUMENT document() { return document_; }
  FPDF_FORMHANDLE form_handle() { return form_handle_; }

  // Create an empty document, and its form fill environment. Returns true
  // on success or false on failure.
  bool CreateEmptyDocument();

  // Open the document specified by |filename|, and create its form fill
  // environment, or return false on failure. The |filename| is relative to
  // the test data directory where we store all the test files. |password| can
  // be nullptr if the file is not password protected. If |javascript_opts|
  // is kDisableJavascript, then the document will be given stubs in place
  // of the real JS engine.
  virtual bool OpenDocumentWithOptions(const std::string& filename,
                                       const char* password,
                                       LinearizeOption linearize_option,
                                       JavaScriptOption javascript_option);

  // Variants provided for convenience.
  bool OpenDocument(const std::string& filename);
  bool OpenDocumentLinearized(const std::string& filename);
  bool OpenDocumentWithPassword(const std::string& filename,
                                const char* password);
  bool OpenDocumentWithoutJavaScript(const std::string& filename);

  // Perform JavaScript actions that are to run at document open time.
  void DoOpenActions();

  // Determine the page numbers present in the document.
  int GetFirstPageNum();
  int GetPageCount();

  // Load a specific page of the open document with a given non-negative
  // |page_number|. On success, fire form events for the page and return a page
  // handle. On failure, return nullptr.
  // The caller does not own the returned page handle, but must call
  // UnloadPage() on it when done.
  // The caller cannot call this for a |page_number| if it already obtained and
  // holds the page handle for that page.
  FPDF_PAGE LoadPage(int page_number);

  // Fire form unload events and release the resources for a |page| obtained
  // from LoadPage(). Further use of |page| is prohibited after calling this.
  void UnloadPage(FPDF_PAGE page);

  // RenderLoadedPageWithFlags() with no flags.
  ScopedFPDFBitmap RenderLoadedPage(FPDF_PAGE page);

  // Convert |page| loaded via LoadPage() into a bitmap with the specified page
  // rendering |flags|.
  //
  // See public/fpdfview.h for a list of page rendering flags.
  ScopedFPDFBitmap RenderLoadedPageWithFlags(FPDF_PAGE page, int flags);

  // RenderSavedPageWithFlags() with no flags.
  ScopedFPDFBitmap RenderSavedPage(FPDF_PAGE page);

  // Convert |page| loaded via LoadSavedPage() into a bitmap with the specified
  // page rendering |flags|.
  //
  // See public/fpdfview.h for a list of page rendering flags.
  ScopedFPDFBitmap RenderSavedPageWithFlags(FPDF_PAGE page, int flags);

  // Convert |page| into a bitmap with the specified page rendering |flags|.
  // The form handle associated with |page| should be passed in via |handle|.
  // If |handle| is nullptr, then forms on the page will not be rendered.
  //
  // See public/fpdfview.h for a list of page rendering flags.
  // If none of the above Render methods are appropriate, then use this one.
  static ScopedFPDFBitmap RenderPageWithFlags(FPDF_PAGE page,
                                              FPDF_FORMHANDLE handle,
                                              int flags);

 protected:
  using PageNumberToHandleMap = std::map<int, FPDF_PAGE>;

  bool OpenDocumentHelper(const char* password,
                          LinearizeOption linearize_option,
                          JavaScriptOption javascript_option,
                          FakeFileAccess* network_simulator,
                          FPDF_DOCUMENT* document,
                          FPDF_AVAIL* avail,
                          FPDF_FORMHANDLE* form_handle);

  FPDF_FORMHANDLE SetupFormFillEnvironment(FPDF_DOCUMENT doc,
                                           JavaScriptOption javascript_option);

  // Return the hash of |bitmap|.
  static std::string HashBitmap(FPDF_BITMAP bitmap);

#ifndef NDEBUG
  // For debugging purposes.
  // Write |bitmap| to a png file.
  static void WriteBitmapToPng(FPDF_BITMAP bitmap, const std::string& filename);
#endif

  // Check |bitmap| to make sure it has the right dimensions and content.
  static void CompareBitmap(FPDF_BITMAP bitmap,
                            int expected_width,
                            int expected_height,
                            const char* expected_md5sum);

  void ClearString() { data_string_.clear(); }
  const std::string& GetString() const { return data_string_; }

  static int GetBlockFromString(void* param,
                                unsigned long pos,
                                unsigned char* buf,
                                unsigned long size);

  // See comments in the respective non-Saved versions of these methods.
  FPDF_DOCUMENT OpenSavedDocument(const char* password = nullptr);
  void CloseSavedDocument();
  FPDF_PAGE LoadSavedPage(int page_number);
  void CloseSavedPage(FPDF_PAGE page);

  void VerifySavedRendering(FPDF_PAGE page,
                            int width,
                            int height,
                            const char* md5);
  void VerifySavedDocument(int width, int height, const char* md5);

  void SetWholeFileAvailable();

  std::unique_ptr<Delegate> default_delegate_;
  Delegate* delegate_;

  FPDF_DOCUMENT document_ = nullptr;
  FPDF_FORMHANDLE form_handle_ = nullptr;
  FPDF_AVAIL avail_ = nullptr;
  FPDF_FILEACCESS file_access_;                       // must outlive |avail_|.
  std::unique_ptr<FakeFileAccess> fake_file_access_;  // must outlive |avail_|.

  void* external_isolate_ = nullptr;
  TestLoader* loader_ = nullptr;
  size_t file_length_ = 0;
  std::unique_ptr<char, pdfium::FreeDeleter> file_contents_;
  PageNumberToHandleMap page_map_;

  FPDF_DOCUMENT saved_document_ = nullptr;
  FPDF_FORMHANDLE saved_form_handle_ = nullptr;
  FPDF_AVAIL saved_avail_ = nullptr;
  FPDF_FILEACCESS saved_file_access_;  // must outlive |saved_avail_|.
  // must outlive |saved_avail_|.
  std::unique_ptr<FakeFileAccess> saved_fake_file_access_;
  PageNumberToHandleMap saved_page_map_;

 private:
  static void UnsupportedHandlerTrampoline(UNSUPPORT_INFO*, int type);
  static int AlertTrampoline(IPDF_JSPLATFORM* plaform,
                             FPDF_WIDESTRING message,
                             FPDF_WIDESTRING title,
                             int type,
                             int icon);
  static int SetTimerTrampoline(FPDF_FORMFILLINFO* info,
                                int msecs,
                                TimerCallback fn);
  static void KillTimerTrampoline(FPDF_FORMFILLINFO* info, int id);
  static FPDF_PAGE GetPageTrampoline(FPDF_FORMFILLINFO* info,
                                     FPDF_DOCUMENT document,
                                     int page_index);
  static int WriteBlockCallback(FPDF_FILEWRITE* pFileWrite,
                                const void* data,
                                unsigned long size);

  // Helper method for the methods below.
  static int GetPageNumberForPage(const PageNumberToHandleMap& page_map,
                                  FPDF_PAGE page);
  // Find |page| inside |page_map_| and return the associated page number, or -1
  // if |page| cannot be found.
  int GetPageNumberForLoadedPage(FPDF_PAGE page) const;

  // Same as GetPageNumberForLoadedPage(), but with |saved_page_map_|.
  int GetPageNumberForSavedPage(FPDF_PAGE page) const;

  std::string data_string_;
};

#endif  // TESTING_EMBEDDER_TEST_H_
