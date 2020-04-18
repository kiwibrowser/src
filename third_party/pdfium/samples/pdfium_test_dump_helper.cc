// Copyright 2018 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/pdfium_test_dump_helper.h"

#include <string.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "public/cpp/fpdf_scopers.h"
#include "testing/test_support.h"

namespace {

std::wstring ConvertToWString(const unsigned short* buf,
                              unsigned long buf_size) {
  std::wstring result;
  result.reserve(buf_size);
  std::copy(buf, buf + buf_size, std::back_inserter(result));
  return result;
}

}  // namespace

void DumpChildStructure(FPDF_STRUCTELEMENT child, int indent) {
  static const size_t kBufSize = 1024;
  unsigned short buf[kBufSize];
  unsigned long len = FPDF_StructElement_GetType(child, buf, kBufSize);
  printf("%*s%ls", indent * 2, "", ConvertToWString(buf, len).c_str());

  memset(buf, 0, sizeof(buf));
  len = FPDF_StructElement_GetTitle(child, buf, kBufSize);
  if (len > 0)
    printf(": '%ls'", ConvertToWString(buf, len).c_str());

  memset(buf, 0, sizeof(buf));
  len = FPDF_StructElement_GetAltText(child, buf, kBufSize);
  if (len > 0)
    printf(" (%ls)", ConvertToWString(buf, len).c_str());
  printf("\n");

  for (int i = 0; i < FPDF_StructElement_CountChildren(child); ++i) {
    FPDF_STRUCTELEMENT sub_child = FPDF_StructElement_GetChildAtIndex(child, i);
    // If the child is not an Element then this will return null. This can
    // happen if the element is things like an object reference or a stream.
    if (!sub_child)
      continue;

    DumpChildStructure(sub_child, indent + 1);
  }
}

void DumpPageStructure(FPDF_PAGE page, const int page_idx) {
  ScopedFPDFStructTree tree(FPDF_StructTree_GetForPage(page));
  if (!tree) {
    fprintf(stderr, "Failed to load struct tree for page %d\n", page_idx);
    return;
  }

  printf("Structure Tree for Page %d\n", page_idx);
  for (int i = 0; i < FPDF_StructTree_CountChildren(tree.get()); ++i) {
    FPDF_STRUCTELEMENT child = FPDF_StructTree_GetChildAtIndex(tree.get(), i);
    if (!child) {
      fprintf(stderr, "Failed to load child %d for page %d\n", i, page_idx);
      continue;
    }
    DumpChildStructure(child, 0);
  }
  printf("\n\n");
}

void DumpMetaData(FPDF_DOCUMENT doc) {
  constexpr const char* meta_tags[] = {"Title",        "Author",  "Subject",
                                       "Keywords",     "Creator", "Producer",
                                       "CreationDate", "ModDate"};
  for (const char* meta_tag : meta_tags) {
    char meta_buffer[4096];
    unsigned long len =
        FPDF_GetMetaText(doc, meta_tag, meta_buffer, sizeof(meta_buffer));
    if (!len)
      continue;

    auto* meta_string = reinterpret_cast<unsigned short*>(meta_buffer);
    printf("%-12s = %ls (%lu bytes)\n", meta_tag,
           GetPlatformWString(meta_string).c_str(), len);
  }
}
