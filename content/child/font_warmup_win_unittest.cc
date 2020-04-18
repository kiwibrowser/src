// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/font_warmup_win.h"

#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/sys_byteorder.h"
#include "base/win/windows_version.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkString.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/ports/SkFontMgr.h"

namespace content {

namespace {

class TestSkTypeface : public SkTypeface {
 public:
  TestSkTypeface(const SkFontStyle& style,
                 const char* familyName,
                 SkFontTableTag tag,
                 const char* data,
                 size_t dataLength)
      : SkTypeface(style, 0),
        familyName_(familyName),
        tag_(tag),
        data_(data, data + dataLength) {}

 protected:
  SkScalerContext* onCreateScalerContext(const SkScalerContextEffects&,
                                         const SkDescriptor*) const override {
    ADD_FAILURE();
    return nullptr;
  }
  void onFilterRec(SkScalerContextRec*) const override { ADD_FAILURE(); }

  SkStreamAsset* onOpenStream(int* ttcIndex) const override {
    ADD_FAILURE();
    return nullptr;
  }

  int onGetVariationDesignPosition(
      SkFontArguments::VariationPosition::Coordinate coordinates[],
      int coordinateCount) const override {
    ADD_FAILURE();
    return -1;
  }

  void onGetFontDescriptor(SkFontDescriptor*, bool* isLocal) const override {
    ADD_FAILURE();
  }

  int onCharsToGlyphs(const void* chars,
                      Encoding,
                      uint16_t glyphs[],
                      int glyphCount) const override {
    ADD_FAILURE();
    return 0;
  }

  int onCountGlyphs() const override {
    ADD_FAILURE();
    return 0;
  }

  int onGetUPEM() const override {
    ADD_FAILURE();
    return 0;
  }
  bool onGetKerningPairAdjustments(const uint16_t glyphs[],
                                   int count,
                                   int32_t adjustments[]) const override {
    ADD_FAILURE();
    return false;
  }

  void onGetFamilyName(SkString* familyName) const override {
    *familyName = familyName_;
  }

  LocalizedStrings* onCreateFamilyNameIterator() const override {
    ADD_FAILURE();
    return nullptr;
  }

  int onGetTableTags(SkFontTableTag tags[]) const override {
    ADD_FAILURE();
    return 0;
  }

  size_t onGetTableData(SkFontTableTag tag,
                        size_t offset,
                        size_t length,
                        void* data) const override {
    size_t retsize = 0;
    if (tag == tag_) {
      retsize = length > data_.size() ? data_.size() : length;
      if (data)
        memcpy(data, &data_[0], retsize);
    }
    return retsize;
  }

  bool onComputeBounds(SkRect*) const override {
    ADD_FAILURE();
    return false;
  }

 private:
  SkString familyName_;
  SkFontTableTag tag_;
  std::vector<char> data_;
};

const char* kTestFontFamily = "GDITest";
const wchar_t* kTestFontFamilyW = L"GDITest";
const SkFontTableTag kTestFontTableTag = 0x11223344;
const char* kTestFontData = "GDITestGDITest";
const wchar_t* kTestFontFamilyInvalid = L"InvalidFont";

class TestSkFontMgr : public SkFontMgr {
 public:
  TestSkFontMgr() {
    content::SetPreSandboxWarmupFontMgrForTesting(sk_ref_sp(this));
  }
  ~TestSkFontMgr() override {
    content::SetPreSandboxWarmupFontMgrForTesting(nullptr);
  }

 protected:
  int onCountFamilies() const override { return 1; }

  void onGetFamilyName(int index, SkString* familyName) const override {
    if (index == 0)
      *familyName = kTestFontFamily;
  }

  SkFontStyleSet* onCreateStyleSet(int index) const override {
    ADD_FAILURE();
    return nullptr;
  }

  SkFontStyleSet* onMatchFamily(const char familyName[]) const override {
    ADD_FAILURE();
    return nullptr;
  }

  SkTypeface* onMatchFamilyStyle(const char familyName[],
                                 const SkFontStyle&) const override {
    if (strcmp(familyName, kTestFontFamily) == 0)
      return createTypeface();
    return nullptr;
  }

  SkTypeface* onMatchFamilyStyleCharacter(const char familyName[],
                                          const SkFontStyle&,
                                          const char* bcp47[],
                                          int bcp47Count,
                                          SkUnichar character) const override {
    ADD_FAILURE();
    return nullptr;
  }

  SkTypeface* onMatchFaceStyle(const SkTypeface*,
                               const SkFontStyle&) const override {
    ADD_FAILURE();
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromData(sk_sp<SkData>, int ttcIndex) const override {
    ADD_FAILURE();
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromStreamIndex(std::unique_ptr<SkStreamAsset>,
                                          int ttcIndex) const override {
    ADD_FAILURE();
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromFile(const char path[],
                                   int ttcIndex) const override {
    ADD_FAILURE();
    return nullptr;
  }

  sk_sp<SkTypeface> onLegacyMakeTypeface(const char familyName[],
                                         SkFontStyle style) const override {
    ADD_FAILURE();
    return nullptr;
  }

 private:
  SkTypeface* createTypeface() const {
    SkFontStyle style(400, 100, SkFontStyle::kUpright_Slant);

    return new TestSkTypeface(style, kTestFontFamily,
                              base::ByteSwap(kTestFontTableTag), kTestFontData,
                              strlen(kTestFontData));
  }
};

void InitLogFont(LOGFONTW* logfont, const wchar_t* fontname) {
  size_t length = std::min(sizeof(logfont->lfFaceName),
                           (wcslen(fontname) + 1) * sizeof(wchar_t));
  memcpy(logfont->lfFaceName, fontname, length);
}

content::GdiFontPatchData* SetupTest() {
  HMODULE module_handle;
  if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                         reinterpret_cast<LPCWSTR>(SetupTest),
                         &module_handle)) {
    WCHAR module_path[MAX_PATH];

    if (GetModuleFileNameW(module_handle, module_path, MAX_PATH) > 0) {
      base::FilePath path(module_path);
      content::ResetEmulatedGdiHandlesForTesting();
      return content::PatchGdiFontEnumeration(path);
    }
  }
  return nullptr;
}

int CALLBACK EnumFontCallbackTest(const LOGFONT* log_font,
                                  const TEXTMETRIC* text_metric,
                                  DWORD font_type,
                                  LPARAM param) {
  const NEWTEXTMETRICEX* new_text_metric =
      reinterpret_cast<const NEWTEXTMETRICEX*>(text_metric);

  return !(font_type & TRUETYPE_FONTTYPE) &&
         !(new_text_metric->ntmTm.ntmFlags & NTM_PS_OPENTYPE);
}

}  // namespace

TEST(GDIFontEmulationTest, CreateDeleteDCSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_FALSE(!patch_data);

  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  EXPECT_EQ(1u, GetEmulatedGdiHandleCountForTesting());
  EXPECT_TRUE(DeleteDC(hdc));
  EXPECT_EQ(0u, GetEmulatedGdiHandleCountForTesting());
}

TEST(GDIFontEmulationTest, CreateUniqueDCSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);

  HDC hdc1 = CreateCompatibleDC(0);
  EXPECT_NE(hdc1, nullptr);
  HDC hdc2 = CreateCompatibleDC(0);
  EXPECT_NE(hdc2, nullptr);
  EXPECT_NE(hdc1, hdc2);
  EXPECT_TRUE(DeleteDC(hdc2));
  EXPECT_EQ(1u, GetEmulatedGdiHandleCountForTesting());
  EXPECT_TRUE(DeleteDC(hdc1));
  EXPECT_EQ(0u, GetEmulatedGdiHandleCountForTesting());
}

TEST(GDIFontEmulationTest, CreateFontSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyW);
  HFONT font = CreateFontIndirectW(&logfont);
  EXPECT_NE(font, nullptr);
  EXPECT_TRUE(DeleteObject(font));
  EXPECT_EQ(0u, GetEmulatedGdiHandleCountForTesting());
}

TEST(GDIFontEmulationTest, CreateFontFailure) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyInvalid);
  HFONT font = CreateFontIndirectW(&logfont);
  EXPECT_EQ(font, nullptr);
}

TEST(GDIFontEmulationTest, EnumFontFamilySuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyW);
  int res = EnumFontFamiliesExW(hdc, &logfont, EnumFontCallbackTest, 0, 0);
  EXPECT_FALSE(res);
  EXPECT_TRUE(DeleteDC(hdc));
}

TEST(GDIFontEmulationTest, EnumFontFamilyFailure) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyInvalid);
  int res = EnumFontFamiliesExW(hdc, &logfont, EnumFontCallbackTest, 0, 0);
  EXPECT_TRUE(res);
  EXPECT_TRUE(DeleteDC(hdc));
}

TEST(GDIFontEmulationTest, DeleteDCFailure) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = reinterpret_cast<HDC>(0x55667788);
  EXPECT_FALSE(DeleteDC(hdc));
}

TEST(GDIFontEmulationTest, DeleteObjectFailure) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HFONT font = reinterpret_cast<HFONT>(0x88aabbcc);
  EXPECT_FALSE(DeleteObject(font));
}

TEST(GDIFontEmulationTest, GetFontDataSizeSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyW);
  HFONT font = CreateFontIndirectW(&logfont);
  EXPECT_NE(font, nullptr);
  EXPECT_EQ(SelectObject(hdc, font), nullptr);
  DWORD size = GetFontData(hdc, kTestFontTableTag, 0, nullptr, 0);
  DWORD data_size = static_cast<DWORD>(strlen(kTestFontData));
  EXPECT_EQ(size, data_size);
  EXPECT_TRUE(DeleteObject(font));
  EXPECT_TRUE(DeleteDC(hdc));
}

TEST(GDIFontEmulationTest, GetFontDataInvalidTagSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyW);
  HFONT font = CreateFontIndirectW(&logfont);
  EXPECT_NE(font, nullptr);
  EXPECT_EQ(SelectObject(hdc, font), nullptr);
  DWORD size = GetFontData(hdc, kTestFontTableTag + 1, 0, nullptr, 0);
  EXPECT_EQ(size, GDI_ERROR);
  EXPECT_TRUE(DeleteObject(font));
  EXPECT_TRUE(DeleteDC(hdc));
}

TEST(GDIFontEmulationTest, GetFontDataInvalidFontSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  DWORD size = GetFontData(hdc, kTestFontTableTag, 0, nullptr, 0);
  EXPECT_EQ(size, GDI_ERROR);
  EXPECT_TRUE(DeleteDC(hdc));
}

TEST(GDIFontEmulationTest, GetFontDataDataSuccess) {
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;
  TestSkFontMgr fontmgr;
  std::unique_ptr<GdiFontPatchData> patch_data(SetupTest());
  EXPECT_NE(patch_data, nullptr);
  HDC hdc = CreateCompatibleDC(0);
  EXPECT_NE(hdc, nullptr);
  LOGFONTW logfont = {0};
  InitLogFont(&logfont, kTestFontFamilyW);
  HFONT font = CreateFontIndirectW(&logfont);
  EXPECT_NE(font, nullptr);
  EXPECT_EQ(SelectObject(hdc, font), nullptr);
  DWORD data_size = static_cast<DWORD>(strlen(kTestFontData));
  std::vector<char> data(data_size);
  DWORD size = GetFontData(hdc, kTestFontTableTag, 0, &data[0], data.size());
  EXPECT_EQ(size, data_size);
  EXPECT_EQ(memcmp(&data[0], kTestFontData, data.size()), 0);
  EXPECT_TRUE(DeleteObject(font));
  EXPECT_TRUE(DeleteDC(hdc));
}

}  // namespace content
