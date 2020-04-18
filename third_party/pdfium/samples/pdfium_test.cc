// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined PDF_ENABLE_SKIA && !defined _SKIA_SUPPORT_
#define _SKIA_SUPPORT_
#endif

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_attachment.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_ext.h"
#include "public/fpdf_formfill.h"
#include "public/fpdf_progressive.h"
#include "public/fpdf_structtree.h"
#include "public/fpdf_text.h"
#include "public/fpdfview.h"
#include "samples/pdfium_test_dump_helper.h"
#include "samples/pdfium_test_event_helper.h"
#include "samples/pdfium_test_write_helper.h"
#include "testing/test_support.h"
#include "testing/utils/path_service.h"
#include "third_party/base/logging.h"
#include "third_party/base/optional.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef ENABLE_CALLGRIND
#include <valgrind/callgrind.h>
#endif  // ENABLE_CALLGRIND

#ifdef PDF_ENABLE_V8
#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"
#endif  // PDF_ENABLE_V8

#ifdef _WIN32
#define access _access
#define snprintf _snprintf
#define R_OK 4
#endif

// wordexp is a POSIX function that is only available on OSX and non-Android
// Linux platforms.
#if defined(__APPLE__) || (defined(__linux__) && !defined(__ANDROID__))
#define WORDEXP_AVAILABLE
#endif

#ifdef WORDEXP_AVAILABLE
#include <wordexp.h>
#endif  // WORDEXP_AVAILABLE

enum OutputFormat {
  OUTPUT_NONE,
  OUTPUT_STRUCTURE,
  OUTPUT_TEXT,
  OUTPUT_PPM,
  OUTPUT_PNG,
  OUTPUT_ANNOT,
#ifdef _WIN32
  OUTPUT_BMP,
  OUTPUT_EMF,
  OUTPUT_PS2,
  OUTPUT_PS3,
#endif
#ifdef PDF_ENABLE_SKIA
  OUTPUT_SKP,
#endif
};

namespace {

struct Options {
  Options() = default;

  bool show_config = false;
  bool show_metadata = false;
  bool send_events = false;
  bool render_oneshot = false;
  bool save_attachments = false;
  bool save_images = false;
#ifdef PDF_ENABLE_V8
  bool disable_javascript = false;
#ifdef PDF_ENABLE_XFA
  bool disable_xfa = false;
#endif  // PDF_ENABLE_XFA
#endif  // PDF_ENABLE_V8
  bool pages = false;
  bool md5 = false;
#ifdef ENABLE_CALLGRIND
  bool callgrind_delimiters = false;
#endif  // ENABLE_CALLGRIND
  OutputFormat output_format = OUTPUT_NONE;
  std::string scale_factor_as_string;
  std::string exe_path;
  std::string bin_directory;
  std::string font_directory;
  int first_page = 0;  // First 0-based page number to renderer.
  int last_page = 0;   // Last 0-based page number to renderer.
};

Optional<std::string> ExpandDirectoryPath(const std::string& path) {
#if defined(WORDEXP_AVAILABLE)
  wordexp_t expansion;
  if (wordexp(path.c_str(), &expansion, 0) != 0 || expansion.we_wordc < 1) {
    wordfree(&expansion);
    return {};
  }
  // Need to contruct the return value before hand, since wordfree will
  // deallocate |expansion|.
  Optional<std::string> ret_val = {expansion.we_wordv[0]};
  wordfree(&expansion);
  return ret_val;
#else
  return {path};
#endif  // WORDEXP_AVAILABLE
}

struct FPDF_FORMFILLINFO_PDFiumTest : public FPDF_FORMFILLINFO {
  // Hold a map of the currently loaded pages in order to avoid them
  // to get loaded twice.
  std::map<int, ScopedFPDFPage> loaded_pages;

  // Hold a pointer of FPDF_FORMHANDLE so that PDFium app hooks can
  // make use of it.
  FPDF_FORMHANDLE form_handle;
};

FPDF_FORMFILLINFO_PDFiumTest* ToPDFiumTestFormFillInfo(
    FPDF_FORMFILLINFO* form_fill_info) {
  return static_cast<FPDF_FORMFILLINFO_PDFiumTest*>(form_fill_info);
}

void OutputMD5Hash(const char* file_name, const char* buffer, int len) {
  // Get the MD5 hash and write it to stdout.
  std::string hash =
      GenerateMD5Base16(reinterpret_cast<const uint8_t*>(buffer), len);
  printf("MD5:%s:%s\n", file_name, hash.c_str());
}

// These example JS platform callback handlers are entirely optional,
// and exist here to show the flow of information from a document back
// to the embedder.
int ExampleAppAlert(IPDF_JSPLATFORM*,
                    FPDF_WIDESTRING msg,
                    FPDF_WIDESTRING title,
                    int type,
                    int icon) {
  printf("%ls", GetPlatformWString(title).c_str());
  if (icon || type)
    printf("[icon=%d,type=%d]", icon, type);
  printf(": %ls\n", GetPlatformWString(msg).c_str());
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
  printf("%ls: %ls, defaultValue=%ls, label=%ls, isPassword=%d, length=%d\n",
         GetPlatformWString(title).c_str(),
         GetPlatformWString(question).c_str(),
         GetPlatformWString(default_value).c_str(),
         GetPlatformWString(label).c_str(), is_password, length);

  // UTF-16, always LE regardless of platform.
  auto* ptr = static_cast<uint8_t*>(response);
  ptr[0] = 'N';
  ptr[1] = 0;
  ptr[2] = 'o';
  ptr[3] = 0;
  return 4;
}

void ExampleAppBeep(IPDF_JSPLATFORM*, int type) {
  printf("BEEP!!! %d\n", type);
}

void ExampleDocGotoPage(IPDF_JSPLATFORM*, int page_number) {
  printf("Goto Page: %d\n", page_number);
}

void ExampleDocMail(IPDF_JSPLATFORM*,
                    void* mailData,
                    int length,
                    FPDF_BOOL UI,
                    FPDF_WIDESTRING To,
                    FPDF_WIDESTRING Subject,
                    FPDF_WIDESTRING CC,
                    FPDF_WIDESTRING BCC,
                    FPDF_WIDESTRING Msg) {
  printf("Mail Msg: %d, to=%ls, cc=%ls, bcc=%ls, subject=%ls, body=%ls\n", UI,
         GetPlatformWString(To).c_str(), GetPlatformWString(CC).c_str(),
         GetPlatformWString(BCC).c_str(), GetPlatformWString(Subject).c_str(),
         GetPlatformWString(Msg).c_str());
}

void ExampleUnsupportedHandler(UNSUPPORT_INFO*, int type) {
  std::string feature = "Unknown";
  switch (type) {
    case FPDF_UNSP_DOC_XFAFORM:
      feature = "XFA";
      break;
    case FPDF_UNSP_DOC_PORTABLECOLLECTION:
      feature = "Portfolios_Packages";
      break;
    case FPDF_UNSP_DOC_ATTACHMENT:
    case FPDF_UNSP_ANNOT_ATTACHMENT:
      feature = "Attachment";
      break;
    case FPDF_UNSP_DOC_SECURITY:
      feature = "Rights_Management";
      break;
    case FPDF_UNSP_DOC_SHAREDREVIEW:
      feature = "Shared_Review";
      break;
    case FPDF_UNSP_DOC_SHAREDFORM_ACROBAT:
    case FPDF_UNSP_DOC_SHAREDFORM_FILESYSTEM:
    case FPDF_UNSP_DOC_SHAREDFORM_EMAIL:
      feature = "Shared_Form";
      break;
    case FPDF_UNSP_ANNOT_3DANNOT:
      feature = "3D";
      break;
    case FPDF_UNSP_ANNOT_MOVIE:
      feature = "Movie";
      break;
    case FPDF_UNSP_ANNOT_SOUND:
      feature = "Sound";
      break;
    case FPDF_UNSP_ANNOT_SCREEN_MEDIA:
    case FPDF_UNSP_ANNOT_SCREEN_RICHMEDIA:
      feature = "Screen";
      break;
    case FPDF_UNSP_ANNOT_SIG:
      feature = "Digital_Signature";
      break;
  }
  printf("Unsupported feature: %s.\n", feature.c_str());
}

bool ParseCommandLine(const std::vector<std::string>& args,
                      Options* options,
                      std::vector<std::string>* files) {
  if (args.empty())
    return false;

  options->exe_path = args[0];
  size_t cur_idx = 1;
  for (; cur_idx < args.size(); ++cur_idx) {
    const std::string& cur_arg = args[cur_idx];
    if (cur_arg == "--show-config") {
      options->show_config = true;
    } else if (cur_arg == "--show-metadata") {
      options->show_metadata = true;
    } else if (cur_arg == "--send-events") {
      options->send_events = true;
    } else if (cur_arg == "--render-oneshot") {
      options->render_oneshot = true;
    } else if (cur_arg == "--save-attachments") {
      options->save_attachments = true;
    } else if (cur_arg == "--save-images") {
      options->save_images = true;
#if PDF_ENABLE_V8
    } else if (cur_arg == "--disable-javascript") {
      options->disable_javascript = true;
#ifdef PDF_ENABLE_XFA
    } else if (cur_arg == "--disable-xfa") {
      options->disable_xfa = true;
#endif  // PDF_ENABLE_XFA
#endif  // PDF_ENABLE_V8
#ifdef ENABLE_CALLGRIND
    } else if (cur_arg == "--callgrind-delim") {
      options->callgrind_delimiters = true;
#endif  // ENABLE_CALLGRIND
    } else if (cur_arg == "--ppm") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --ppm argument\n");
        return false;
      }
      options->output_format = OUTPUT_PPM;
    } else if (cur_arg == "--png") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --png argument\n");
        return false;
      }
      options->output_format = OUTPUT_PNG;
    } else if (cur_arg == "--txt") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --txt argument\n");
        return false;
      }
      options->output_format = OUTPUT_TEXT;
    } else if (cur_arg == "--annot") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --annot argument\n");
        return false;
      }
      options->output_format = OUTPUT_ANNOT;
#ifdef PDF_ENABLE_SKIA
    } else if (cur_arg == "--skp") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --skp argument\n");
        return false;
      }
      options->output_format = OUTPUT_SKP;
#endif  // PDF_ENABLE_SKIA
    } else if (cur_arg.size() > 11 &&
               cur_arg.compare(0, 11, "--font-dir=") == 0) {
      if (!options->font_directory.empty()) {
        fprintf(stderr, "Duplicate --font-dir argument\n");
        return false;
      }
      std::string path = cur_arg.substr(11);
      auto expanded_path = ExpandDirectoryPath(path);
      if (!expanded_path) {
        fprintf(stderr, "Failed to expand --font-dir, %s\n", path.c_str());
        return false;
      }

      if (!PathService::DirectoryExists(expanded_path.value())) {
        fprintf(stderr, "--font-dir, %s, appears to not be a directory\n",
                path.c_str());
        return false;
      }

      options->font_directory = expanded_path.value();

#ifdef _WIN32
    } else if (cur_arg == "--emf") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --emf argument\n");
        return false;
      }
      options->output_format = OUTPUT_EMF;
    } else if (cur_arg == "--ps2") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --ps2 argument\n");
        return false;
      }
      options->output_format = OUTPUT_PS2;
    } else if (cur_arg == "--ps3") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --ps3 argument\n");
        return false;
      }
      options->output_format = OUTPUT_PS3;
    } else if (cur_arg == "--bmp") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --bmp argument\n");
        return false;
      }
      options->output_format = OUTPUT_BMP;
#endif  // _WIN32

#ifdef PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
    } else if (cur_arg.size() > 10 &&
               cur_arg.compare(0, 10, "--bin-dir=") == 0) {
      if (!options->bin_directory.empty()) {
        fprintf(stderr, "Duplicate --bin-dir argument\n");
        return false;
      }
      std::string path = cur_arg.substr(10);
      auto expanded_path = ExpandDirectoryPath(path);
      if (!expanded_path) {
        fprintf(stderr, "Failed to expand --bin-dir, %s\n", path.c_str());
        return false;
      }
      options->bin_directory = expanded_path.value();
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // PDF_ENABLE_V8

    } else if (cur_arg.size() > 8 && cur_arg.compare(0, 8, "--scale=") == 0) {
      if (!options->scale_factor_as_string.empty()) {
        fprintf(stderr, "Duplicate --scale argument\n");
        return false;
      }
      options->scale_factor_as_string = cur_arg.substr(8);
    } else if (cur_arg == "--show-structure") {
      if (options->output_format != OUTPUT_NONE) {
        fprintf(stderr, "Duplicate or conflicting --show-structure argument\n");
        return false;
      }
      options->output_format = OUTPUT_STRUCTURE;
    } else if (cur_arg.size() > 8 && cur_arg.compare(0, 8, "--pages=") == 0) {
      if (options->pages) {
        fprintf(stderr, "Duplicate --pages argument\n");
        return false;
      }
      options->pages = true;
      const std::string pages_string = cur_arg.substr(8);
      size_t first_dash = pages_string.find("-");
      if (first_dash == std::string::npos) {
        std::stringstream(pages_string) >> options->first_page;
        options->last_page = options->first_page;
      } else {
        std::stringstream(pages_string.substr(0, first_dash)) >>
            options->first_page;
        std::stringstream(pages_string.substr(first_dash + 1)) >>
            options->last_page;
      }
    } else if (cur_arg == "--md5") {
      options->md5 = true;
    } else if (cur_arg.size() >= 2 && cur_arg[0] == '-' && cur_arg[1] == '-') {
      fprintf(stderr, "Unrecognized argument %s\n", cur_arg.c_str());
      return false;
    } else {
      break;
    }
  }
  for (size_t i = cur_idx; i < args.size(); i++)
    files->push_back(args[i]);

  return true;
}

void PrintLastError() {
  unsigned long err = FPDF_GetLastError();
  fprintf(stderr, "Load pdf docs unsuccessful: ");
  switch (err) {
    case FPDF_ERR_SUCCESS:
      fprintf(stderr, "Success");
      break;
    case FPDF_ERR_UNKNOWN:
      fprintf(stderr, "Unknown error");
      break;
    case FPDF_ERR_FILE:
      fprintf(stderr, "File not found or could not be opened");
      break;
    case FPDF_ERR_FORMAT:
      fprintf(stderr, "File not in PDF format or corrupted");
      break;
    case FPDF_ERR_PASSWORD:
      fprintf(stderr, "Password required or incorrect password");
      break;
    case FPDF_ERR_SECURITY:
      fprintf(stderr, "Unsupported security scheme");
      break;
    case FPDF_ERR_PAGE:
      fprintf(stderr, "Page not found or content error");
      break;
    default:
      fprintf(stderr, "Unknown error %ld", err);
  }
  fprintf(stderr, ".\n");
  return;
}

FPDF_BOOL Is_Data_Avail(FX_FILEAVAIL* avail, size_t offset, size_t size) {
  return true;
}

void Add_Segment(FX_DOWNLOADHINTS* hints, size_t offset, size_t size) {}

FPDF_PAGE GetPageForIndex(FPDF_FORMFILLINFO* param,
                          FPDF_DOCUMENT doc,
                          int index) {
  FPDF_FORMFILLINFO_PDFiumTest* form_fill_info =
      ToPDFiumTestFormFillInfo(param);
  auto& loaded_pages = form_fill_info->loaded_pages;
  auto iter = loaded_pages.find(index);
  if (iter != loaded_pages.end())
    return iter->second.get();

  ScopedFPDFPage page(FPDF_LoadPage(doc, index));
  if (!page)
    return nullptr;

  // Mark the page as loaded first to prevent infinite recursion.
  FPDF_PAGE page_ptr = page.get();
  loaded_pages[index] = std::move(page);

  FPDF_FORMHANDLE& form_handle = form_fill_info->form_handle;
  FORM_OnAfterLoadPage(page_ptr, form_handle);
  FORM_DoPageAAction(page_ptr, form_handle, FPDFPAGE_AACTION_OPEN);
  return page_ptr;
}

// Note, for a client using progressive rendering you'd want to determine if you
// need the rendering to pause instead of always saying |true|. This is for
// testing to force the renderer to break whenever possible.
FPDF_BOOL NeedToPauseNow(IFSDK_PAUSE* p) {
  return true;
}

bool RenderPage(const std::string& name,
                FPDF_DOCUMENT doc,
                FPDF_FORMHANDLE form,
                FPDF_FORMFILLINFO_PDFiumTest* form_fill_info,
                const int page_index,
                const Options& options,
                const std::string& events) {
  FPDF_PAGE page = GetPageForIndex(form_fill_info, doc, page_index);
  if (!page)
    return false;
  if (options.send_events)
    SendPageEvents(form, page, events);
  if (options.save_images)
    WriteImages(page, name.c_str(), page_index);
  if (options.output_format == OUTPUT_STRUCTURE) {
    DumpPageStructure(page, page_index);
    return true;
  }

  ScopedFPDFTextPage text_page(FPDFText_LoadPage(page));
  double scale = 1.0;
  if (!options.scale_factor_as_string.empty())
    std::stringstream(options.scale_factor_as_string) >> scale;

  auto width = static_cast<int>(FPDF_GetPageWidth(page) * scale);
  auto height = static_cast<int>(FPDF_GetPageHeight(page) * scale);
  int alpha = FPDFPage_HasTransparency(page) ? 1 : 0;
  ScopedFPDFBitmap bitmap(FPDFBitmap_Create(width, height, alpha));

  if (bitmap) {
    FPDF_DWORD fill_color = alpha ? 0x00000000 : 0xFFFFFFFF;
    FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, fill_color);

    if (options.render_oneshot) {
      // Note, client programs probably want to use this method instead of the
      // progressive calls. The progressive calls are if you need to pause the
      // rendering to update the UI, the PDF renderer will break when possible.
      FPDF_RenderPageBitmap(bitmap.get(), page, 0, 0, width, height, 0,
                            FPDF_ANNOT);
    } else {
      IFSDK_PAUSE pause;
      pause.version = 1;
      pause.NeedToPauseNow = &NeedToPauseNow;

      int rv = FPDF_RenderPageBitmap_Start(bitmap.get(), page, 0, 0, width,
                                           height, 0, FPDF_ANNOT, &pause);
      while (rv == FPDF_RENDER_TOBECOUNTINUED)
        rv = FPDF_RenderPage_Continue(page, &pause);
    }

    FPDF_FFLDraw(form, bitmap.get(), page, 0, 0, width, height, 0, FPDF_ANNOT);

    if (!options.render_oneshot)
      FPDF_RenderPage_Close(page);

    int stride = FPDFBitmap_GetStride(bitmap.get());
    const char* buffer =
        reinterpret_cast<const char*>(FPDFBitmap_GetBuffer(bitmap.get()));

    std::string image_file_name;
    switch (options.output_format) {
#ifdef _WIN32
      case OUTPUT_BMP:
        image_file_name =
            WriteBmp(name.c_str(), page_index, buffer, stride, width, height);
        break;

      case OUTPUT_EMF:
        WriteEmf(page, name.c_str(), page_index);
        break;

      case OUTPUT_PS2:
      case OUTPUT_PS3:
        WritePS(page, name.c_str(), page_index);
        break;
#endif
      case OUTPUT_TEXT:
        WriteText(page, name.c_str(), page_index);
        break;

      case OUTPUT_ANNOT:
        WriteAnnot(page, name.c_str(), page_index);
        break;

      case OUTPUT_PNG:
        image_file_name =
            WritePng(name.c_str(), page_index, buffer, stride, width, height);
        break;

      case OUTPUT_PPM:
        image_file_name =
            WritePpm(name.c_str(), page_index, buffer, stride, width, height);
        break;

#ifdef PDF_ENABLE_SKIA
      case OUTPUT_SKP: {
        std::unique_ptr<SkPictureRecorder> recorder(
            reinterpret_cast<SkPictureRecorder*>(
                FPDF_RenderPageSkp(page, width, height)));
        FPDF_FFLRecord(form, recorder.get(), page, 0, 0, width, height, 0, 0);
        image_file_name = WriteSkp(name.c_str(), page_index, recorder.get());
      } break;
#endif
      default:
        break;
    }

    // Write the filename and the MD5 of the buffer to stdout if we wrote a
    // file.
    if (options.md5 && !image_file_name.empty())
      OutputMD5Hash(image_file_name.c_str(), buffer, stride * height);
  } else {
    fprintf(stderr, "Page was too large to be rendered.\n");
  }

  FORM_DoPageAAction(page, form, FPDFPAGE_AACTION_CLOSE);
  FORM_OnBeforeClosePage(page, form);
  return !!bitmap;
}

void RenderPdf(const std::string& name,
               const char* pBuf,
               size_t len,
               const Options& options,
               const std::string& events) {
  TestLoader loader(pBuf, len);

  FPDF_FILEACCESS file_access = {};
  file_access.m_FileLen = static_cast<unsigned long>(len);
  file_access.m_GetBlock = TestLoader::GetBlock;
  file_access.m_Param = &loader;

  FX_FILEAVAIL file_avail = {};
  file_avail.version = 1;
  file_avail.IsDataAvail = Is_Data_Avail;

  FX_DOWNLOADHINTS hints = {};
  hints.version = 1;
  hints.AddSegment = Add_Segment;

  // The pdf_avail must outlive doc.
  ScopedFPDFAvail pdf_avail(FPDFAvail_Create(&file_avail, &file_access));

  // The document must outlive |form_callbacks.loaded_pages|.
  ScopedFPDFDocument doc;

  int nRet = PDF_DATA_NOTAVAIL;
  bool bIsLinearized = false;
  if (FPDFAvail_IsLinearized(pdf_avail.get()) == PDF_LINEARIZED) {
    doc.reset(FPDFAvail_GetDocument(pdf_avail.get(), nullptr));
    if (doc) {
      while (nRet == PDF_DATA_NOTAVAIL)
        nRet = FPDFAvail_IsDocAvail(pdf_avail.get(), &hints);

      if (nRet == PDF_DATA_ERROR) {
        fprintf(stderr, "Unknown error in checking if doc was available.\n");
        return;
      }
      nRet = FPDFAvail_IsFormAvail(pdf_avail.get(), &hints);
      if (nRet == PDF_FORM_ERROR || nRet == PDF_FORM_NOTAVAIL) {
        fprintf(stderr,
                "Error %d was returned in checking if form was available.\n",
                nRet);
        return;
      }
      bIsLinearized = true;
    }
  } else {
    doc.reset(FPDF_LoadCustomDocument(&file_access, nullptr));
  }

  if (!doc) {
    PrintLastError();
    return;
  }

  (void)FPDF_GetDocPermissions(doc.get());

  if (options.show_metadata)
    DumpMetaData(doc.get());

  if (options.save_attachments)
    WriteAttachments(doc.get(), name);

  IPDF_JSPLATFORM platform_callbacks = {};
  platform_callbacks.version = 3;
  platform_callbacks.app_alert = ExampleAppAlert;
  platform_callbacks.app_response = ExampleAppResponse;
  platform_callbacks.app_beep = ExampleAppBeep;
  platform_callbacks.Doc_gotoPage = ExampleDocGotoPage;
  platform_callbacks.Doc_mail = ExampleDocMail;

  FPDF_FORMFILLINFO_PDFiumTest form_callbacks = {};
#ifdef PDF_ENABLE_XFA
  form_callbacks.version = 2;
#else   // PDF_ENABLE_XFA
  form_callbacks.version = 1;
#endif  // PDF_ENABLE_XFA
  form_callbacks.FFI_GetPage = GetPageForIndex;

#ifdef PDF_ENABLE_V8
  if (!options.disable_javascript)
    form_callbacks.m_pJsPlatform = &platform_callbacks;
#endif  // PDF_ENABLE_V8

  ScopedFPDFFormHandle form(
      FPDFDOC_InitFormFillEnvironment(doc.get(), &form_callbacks));
  form_callbacks.form_handle = form.get();

#ifdef PDF_ENABLE_XFA
  if (!options.disable_xfa && !options.disable_javascript) {
    int doc_type = FPDF_GetFormType(doc.get());
    if (doc_type == FORMTYPE_XFA_FULL || doc_type == FORMTYPE_XFA_FOREGROUND) {
      if (!FPDF_LoadXFA(doc.get()))
        fprintf(stderr, "LoadXFA unsuccessful, continuing anyway.\n");
    }
  }
#endif  // PDF_ENABLE_XFA

  FPDF_SetFormFieldHighlightColor(form.get(), FPDF_FORMFIELD_UNKNOWN, 0xFFE4DD);
  FPDF_SetFormFieldHighlightAlpha(form.get(), 100);
  FORM_DoDocumentJSAction(form.get());
  FORM_DoDocumentOpenAction(form.get());

#if _WIN32
  if (options.output_format == OUTPUT_PS2)
    FPDF_SetPrintMode(FPDF_PRINTMODE_POSTSCRIPT2);
  else if (options.output_format == OUTPUT_PS3)
    FPDF_SetPrintMode(FPDF_PRINTMODE_POSTSCRIPT3);
#endif

  int page_count = FPDF_GetPageCount(doc.get());
  int rendered_pages = 0;
  int bad_pages = 0;
  int first_page = options.pages ? options.first_page : 0;
  int last_page = options.pages ? options.last_page + 1 : page_count;
  for (int i = first_page; i < last_page; ++i) {
    if (bIsLinearized) {
      nRet = PDF_DATA_NOTAVAIL;
      while (nRet == PDF_DATA_NOTAVAIL)
        nRet = FPDFAvail_IsPageAvail(pdf_avail.get(), i, &hints);

      if (nRet == PDF_DATA_ERROR) {
        fprintf(stderr, "Unknown error in checking if page %d is available.\n",
                i);
        return;
      }
    }
    if (RenderPage(name, doc.get(), form.get(), &form_callbacks, i, options,
                   events)) {
      ++rendered_pages;
    } else {
      ++bad_pages;
    }
  }

  FORM_DoDocumentAAction(form.get(), FPDFDOC_AACTION_WC);
  fprintf(stderr, "Rendered %d pages.\n", rendered_pages);
  if (bad_pages)
    fprintf(stderr, "Skipped %d bad pages.\n", bad_pages);
}

void ShowConfig() {
  std::string config;
  std::string maybe_comma;
#ifdef PDF_ENABLE_V8
  config.append(maybe_comma);
  config.append("V8");
  maybe_comma = ",";
#endif  // PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  config.append(maybe_comma);
  config.append("V8_EXTERNAL");
  maybe_comma = ",";
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#ifdef PDF_ENABLE_XFA
  config.append(maybe_comma);
  config.append("XFA");
  maybe_comma = ",";
#endif  // PDF_ENABLE_XFA
#ifdef PDF_ENABLE_ASAN
  config.append(maybe_comma);
  config.append("ASAN");
  maybe_comma = ",";
#endif  // PDF_ENABLE_ASAN
  printf("%s\n", config.c_str());
}

constexpr char kUsageString[] =
    "Usage: pdfium_test [OPTION] [FILE]...\n"
    "  --show-config       - print build options and exit\n"
    "  --show-metadata     - print the file metadata\n"
    "  --show-structure    - print the structure elements from the document\n"
    "  --send-events       - send input described by .evt file\n"
    "  --render-oneshot    - render image without using progressive renderer\n"
    "  --save-attachments  - write embedded attachments "
    "<pdf-name>.attachment.<attachment-name>\n"
    "  --save-images       - write embedded images "
    "<pdf-name>.<page-number>.<object-number>.png\n"
#ifdef PDF_ENABLE_V8
    "  --disable-javascript- do not execute JS in PDF files"
#ifdef PDF_ENABLE_XFA
    "  --disable-xfa       - do not process XFA forms"
#endif  // PDF_ENABLE_XFA
#endif  // PDF_ENABLE_V8
#ifdef ENABLE_CALLGRIND
    "  --callgrind-delim   - delimit interesting section when using callgrind\n"
#endif  // ENABLE_CALLGRIND
    "  --bin-dir=<path>    - override path to v8 external data\n"
    "  --font-dir=<path>   - override path to external fonts\n"
    "  --scale=<number>    - scale output size by number (e.g. 0.5)\n"
    "  --pages=<number>(-<number>) - only render the given 0-based page(s)\n"
#ifdef _WIN32
    "  --bmp   - write page images <pdf-name>.<page-number>.bmp\n"
    "  --emf   - write page meta files <pdf-name>.<page-number>.emf\n"
    "  --ps2   - write page raw PostScript (Lvl 2) "
    "<pdf-name>.<page-number>.ps\n"
    "  --ps3   - write page raw PostScript (Lvl 3) "
    "<pdf-name>.<page-number>.ps\n"
#endif  // _WIN32
    "  --txt   - write page text in UTF32-LE <pdf-name>.<page-number>.txt\n"
    "  --png   - write page images <pdf-name>.<page-number>.png\n"
    "  --ppm   - write page images <pdf-name>.<page-number>.ppm\n"
    "  --annot - write annotation info <pdf-name>.<page-number>.annot.txt\n"
#ifdef PDF_ENABLE_SKIA
    "  --skp   - write page images <pdf-name>.<page-number>.skp\n"
#endif
    "  --md5   - write output image paths and their md5 hashes to stdout.\n"
    "";

}  // namespace

int main(int argc, const char* argv[]) {
  std::vector<std::string> args(argv, argv + argc);
  Options options;
  std::vector<std::string> files;
  if (!ParseCommandLine(args, &options, &files)) {
    fprintf(stderr, "%s", kUsageString);
    return 1;
  }

  if (options.show_config) {
    ShowConfig();
    return 0;
  }

  if (files.empty()) {
    fprintf(stderr, "No input files.\n");
    return 1;
  }

#ifdef PDF_ENABLE_V8
  std::unique_ptr<v8::Platform> platform;
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  v8::StartupData natives;
  v8::StartupData snapshot;
  platform = InitializeV8ForPDFiumWithStartupData(
      options.exe_path, options.bin_directory, &natives, &snapshot);
#else   // V8_USE_EXTERNAL_STARTUP_DATA
  platform = InitializeV8ForPDFium(options.exe_path);
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // PDF_ENABLE_V8

  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = nullptr;
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;

  const char* path_array[2];
  if (!options.font_directory.empty()) {
    path_array[0] = options.font_directory.c_str();
    path_array[1] = nullptr;
    config.m_pUserFontPaths = path_array;
  }
  FPDF_InitLibraryWithConfig(&config);

  UNSUPPORT_INFO unsupported_info = {};
  unsupported_info.version = 1;
  unsupported_info.FSDK_UnSupport_Handler = ExampleUnsupportedHandler;

  FSDK_SetUnSpObjProcessHandler(&unsupported_info);

  for (const std::string& filename : files) {
    size_t file_length = 0;
    std::unique_ptr<char, pdfium::FreeDeleter> file_contents =
        GetFileContents(filename.c_str(), &file_length);
    if (!file_contents)
      continue;
    fprintf(stderr, "Rendering PDF file %s.\n", filename.c_str());

#ifdef ENABLE_CALLGRIND
    if (options.callgrind_delimiters)
      CALLGRIND_START_INSTRUMENTATION;
#endif  // ENABLE_CALLGRIND

    std::string events;
    if (options.send_events) {
      std::string event_filename = filename;
      size_t event_length = 0;
      size_t extension_pos = event_filename.find(".pdf");
      if (extension_pos != std::string::npos) {
        event_filename.replace(extension_pos, 4, ".evt");
        if (access(event_filename.c_str(), R_OK) == 0) {
          fprintf(stderr, "Using event file %s.\n", event_filename.c_str());
          std::unique_ptr<char, pdfium::FreeDeleter> event_contents =
              GetFileContents(event_filename.c_str(), &event_length);
          if (event_contents) {
            fprintf(stderr, "Sending events from: %s\n",
                    event_filename.c_str());
            events = std::string(event_contents.get(), event_length);
          }
        }
      }
    }
    RenderPdf(filename, file_contents.get(), file_length, options, events);

#ifdef ENABLE_CALLGRIND
    if (options.callgrind_delimiters)
      CALLGRIND_STOP_INSTRUMENTATION;
#endif  // ENABLE_CALLGRIND
  }

  FPDF_DestroyLibrary();
#ifdef PDF_ENABLE_V8
  v8::V8::ShutdownPlatform();

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  free(const_cast<char*>(natives.data));
  free(const_cast<char*>(snapshot.data));
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // PDF_ENABLE_V8

  return 0;
}
