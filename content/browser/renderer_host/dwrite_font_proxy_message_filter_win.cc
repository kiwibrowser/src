// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/dwrite_font_proxy_message_filter_win.h"

#include <dwrite.h>
#include <shlobj.h>
#include <stddef.h>
#include <stdint.h>

#include <set>
#include <utility>

#include "base/callback_helpers.h"
#include "base/feature_list.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "ui/gfx/win/direct_write.h"
#include "ui/gfx/win/text_analysis_source.h"

namespace mswr = Microsoft::WRL;

namespace content {

namespace {

// This enum is used to define the buckets for an enumerated UMA histogram.
// Hence,
//   (a) existing enumerated constants should never be deleted or reordered, and
//   (b) new constants should only be appended at the end of the enumeration.
enum DirectWriteFontLoaderType {
  FILE_SYSTEM_FONT_DIR = 0,
  FILE_OUTSIDE_SANDBOX = 1,
  OTHER_LOADER = 2,
  FONT_WITH_MISSING_REQUIRED_STYLES = 3,

  FONT_LOADER_TYPE_MAX_VALUE
};

// This enum is used to define the buckets for an enumerated UMA histogram.
// Hence,
//   (a) existing enumerated constants should never be deleted or reordered, and
//   (b) new constants should only be appended at the end of the enumeration.
enum MessageFilterError {
  LAST_RESORT_FONT_GET_FONT_FAILED = 0,
  LAST_RESORT_FONT_ADD_FILES_FAILED = 1,
  LAST_RESORT_FONT_GET_FAMILY_FAILED = 2,
  ERROR_NO_COLLECTION = 3,
  MAP_CHARACTERS_NO_FAMILY = 4,
  ADD_FILES_FOR_FONT_CREATE_FACE_FAILED = 5,
  ADD_FILES_FOR_FONT_GET_FILE_COUNT_FAILED = 6,
  ADD_FILES_FOR_FONT_GET_FILES_FAILED = 7,
  ADD_FILES_FOR_FONT_GET_LOADER_FAILED = 8,
  ADD_FILES_FOR_FONT_QI_FAILED = 9,
  ADD_LOCAL_FILE_GET_REFERENCE_KEY_FAILED = 10,
  ADD_LOCAL_FILE_GET_PATH_LENGTH_FAILED = 11,
  ADD_LOCAL_FILE_GET_PATH_FAILED = 12,

  MESSAGE_FILTER_ERROR_MAX_VALUE
};

void LogLoaderType(DirectWriteFontLoaderType loader_type) {
  UMA_HISTOGRAM_ENUMERATION("DirectWrite.Fonts.Proxy.LoaderType", loader_type,
                            FONT_LOADER_TYPE_MAX_VALUE);
}

void LogLastResortFontCount(size_t count) {
  UMA_HISTOGRAM_COUNTS_100("DirectWrite.Fonts.Proxy.LastResortFontCount",
                           count);
}

void LogLastResortFontFileCount(size_t count) {
  UMA_HISTOGRAM_COUNTS_100("DirectWrite.Fonts.Proxy.LastResortFontFileCount",
                           count);
}

void LogMessageFilterError(MessageFilterError error) {
  UMA_HISTOGRAM_ENUMERATION("DirectWrite.Fonts.Proxy.MessageFilterError", error,
                            MESSAGE_FILTER_ERROR_MAX_VALUE);
}

base::string16 GetWindowsFontsPath() {
  std::vector<base::char16> font_path_chars;
  // SHGetSpecialFolderPath requires at least MAX_PATH characters.
  font_path_chars.resize(MAX_PATH);
  BOOL result = SHGetSpecialFolderPath(nullptr /* hwndOwner - reserved */,
                                       font_path_chars.data(), CSIDL_FONTS,
                                       FALSE /* fCreate */);
  DCHECK(result);
  return base::i18n::FoldCase(font_path_chars.data());
}

// These are the fonts that Blink tries to load in getLastResortFallbackFont,
// and will crash if none can be loaded.
const wchar_t* kLastResortFontNames[] = {
    L"Sans",     L"Arial",   L"MS UI Gothic",    L"Microsoft Sans Serif",
    L"Segoe UI", L"Calibri", L"Times New Roman", L"Courier New"};

// Feature to enable loading font files from outside the system font directory.
const base::Feature kEnableCustomFonts {
  "DirectWriteCustomFonts", base::FEATURE_ENABLED_BY_DEFAULT
};

// Feature to force loading font files using the custom font file path. Has no
// effect if kEnableCustomFonts is disabled.
const base::Feature kForceCustomFonts {
  "ForceDirectWriteCustomFonts", base::FEATURE_DISABLED_BY_DEFAULT
};

struct RequiredFontStyle {
  const wchar_t* family_name;
  DWRITE_FONT_WEIGHT required_weight;
  DWRITE_FONT_STRETCH required_stretch;
  DWRITE_FONT_STYLE required_style;
};

const RequiredFontStyle kRequiredStyles[] = {
    // The regular version of Gill Sans is actually in the Gill Sans MT family,
    // and the Gill Sans family typically contains just the ultra-bold styles.
    {L"gill sans", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
     DWRITE_FONT_STYLE_NORMAL},
    {L"helvetica", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
     DWRITE_FONT_STYLE_NORMAL},
    {L"open sans", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
     DWRITE_FONT_STYLE_NORMAL},
};

// As a workaround for crbug.com/635932, refuse to load some common fonts that
// do not contain certain styles. We found that sometimes these fonts are
// installed only in specialized styles ('Open Sans' might only be available in
// the condensed light variant, or Helvetica might only be available in bold).
// That results in a poor user experience because websites that use those fonts
// usually expect them to be rendered in the regular variant.
bool CheckRequiredStylesPresent(IDWriteFontCollection* collection,
                                const base::string16& family_name,
                                uint32_t family_index) {
  for (const auto& font_style : kRequiredStyles) {
    if (base::EqualsCaseInsensitiveASCII(family_name, font_style.family_name)) {
      mswr::ComPtr<IDWriteFontFamily> family;
      if (FAILED(collection->GetFontFamily(family_index, &family))) {
        DCHECK(false);
        return true;
      }
      mswr::ComPtr<IDWriteFont> font;
      if (FAILED(family->GetFirstMatchingFont(
          font_style.required_weight, font_style.required_stretch,
          font_style.required_style, &font))) {
        DCHECK(false);
        return true;
      }

      // GetFirstMatchingFont doesn't require strict style matching, so check
      // the actual font that we got.
      if (font->GetWeight() != font_style.required_weight ||
          font->GetStretch() != font_style.required_stretch ||
          font->GetStyle() != font_style.required_style) {
        // Not really a loader type, but good to have telemetry on how often
        // fonts like these are encountered, and the data can be compared with
        // the other loader types.
        LogLoaderType(FONT_WITH_MISSING_REQUIRED_STYLES);
        return false;
      }
      break;
    }
  }
  return true;
}

}  // namespace

DWriteFontProxyImpl::DWriteFontProxyImpl()
    : windows_fonts_path_(GetWindowsFontsPath()),
      custom_font_file_loading_mode_(ENABLE) {}

DWriteFontProxyImpl::~DWriteFontProxyImpl() = default;

// static
void DWriteFontProxyImpl::Create(
    mojom::DWriteFontProxyRequest request,
    const service_manager::BindSourceInfo& source_info) {
  mojo::MakeStrongBinding(std::make_unique<DWriteFontProxyImpl>(),
                          std::move(request));
}

void DWriteFontProxyImpl::SetWindowsFontsPathForTesting(base::string16 path) {
  windows_fonts_path_.swap(path);
}

void DWriteFontProxyImpl::FindFamily(const base::string16& family_name,
                                     FindFamilyCallback callback) {
  InitializeDirectWrite();
  TRACE_EVENT0("dwrite", "FontProxyHost::OnFindFamily");
  UINT32 family_index = UINT32_MAX;
  if (collection_) {
    BOOL exists = FALSE;
    UINT32 index = UINT32_MAX;
    HRESULT hr =
        collection_->FindFamilyName(family_name.data(), &index, &exists);
    if (SUCCEEDED(hr) && exists &&
        CheckRequiredStylesPresent(collection_.Get(), family_name, index)) {
      family_index = index;
    }
  }
  std::move(callback).Run(family_index);
}

void DWriteFontProxyImpl::GetFamilyCount(GetFamilyCountCallback callback) {
  InitializeDirectWrite();
  TRACE_EVENT0("dwrite", "FontProxyHost::OnGetFamilyCount");
  std::move(callback).Run(collection_ ? collection_->GetFontFamilyCount() : 0);
}

void DWriteFontProxyImpl::GetFamilyNames(UINT32 family_index,
                                         GetFamilyNamesCallback callback) {
  InitializeDirectWrite();
  TRACE_EVENT0("dwrite", "FontProxyHost::OnGetFamilyNames");
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), std::vector<mojom::DWriteStringPairPtr>());
  if (!collection_)
    return;

  TRACE_EVENT0("dwrite", "FontProxyHost::DoGetFamilyNames");

  mswr::ComPtr<IDWriteFontFamily> family;
  HRESULT hr = collection_->GetFontFamily(family_index, &family);
  if (FAILED(hr)) {
    return;
  }

  mswr::ComPtr<IDWriteLocalizedStrings> localized_names;
  hr = family->GetFamilyNames(&localized_names);
  if (FAILED(hr)) {
    return;
  }

  size_t string_count = localized_names->GetCount();

  std::vector<base::char16> locale;
  std::vector<base::char16> name;
  std::vector<mojom::DWriteStringPairPtr> family_names;
  for (size_t index = 0; index < string_count; ++index) {
    UINT32 length = 0;
    hr = localized_names->GetLocaleNameLength(index, &length);
    if (FAILED(hr)) {
      return;
    }
    ++length;  // Reserve space for the null terminator.
    locale.resize(length);
    hr = localized_names->GetLocaleName(index, locale.data(), length);
    if (FAILED(hr)) {
      return;
    }
    CHECK_EQ(L'\0', locale[length - 1]);

    length = 0;
    hr = localized_names->GetStringLength(index, &length);
    if (FAILED(hr)) {
      return;
    }
    ++length;  // Reserve space for the null terminator.
    name.resize(length);
    hr = localized_names->GetString(index, name.data(), length);
    if (FAILED(hr)) {
      return;
    }
    CHECK_EQ(L'\0', name[length - 1]);

    family_names.emplace_back(base::in_place, base::string16(locale.data()),
                              base::string16(name.data()));
  }
  std::move(callback).Run(std::move(family_names));
}

void DWriteFontProxyImpl::GetFontFiles(uint32_t family_index,
                                       GetFontFilesCallback callback) {
  InitializeDirectWrite();
  TRACE_EVENT0("dwrite", "FontProxyHost::OnGetFontFiles");
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), std::vector<base::FilePath>(),
      std::vector<base::File>());
  if (!collection_)
    return;

  mswr::ComPtr<IDWriteFontFamily> family;
  HRESULT hr = collection_->GetFontFamily(family_index, &family);
  if (FAILED(hr)) {
    if (IsLastResortFallbackFont(family_index))
      LogMessageFilterError(LAST_RESORT_FONT_GET_FAMILY_FAILED);
    return;
  }

  UINT32 font_count = family->GetFontCount();

  std::set<base::string16> path_set;
  std::set<base::string16> custom_font_path_set;
  // Iterate through all the fonts in the family, and all the files for those
  // fonts. If anything goes wrong, bail on the entire family to avoid having
  // a partially-loaded font family.
  for (UINT32 font_index = 0; font_index < font_count; ++font_index) {
    mswr::ComPtr<IDWriteFont> font;
    hr = family->GetFont(font_index, &font);
    if (FAILED(hr)) {
      if (IsLastResortFallbackFont(family_index))
        LogMessageFilterError(LAST_RESORT_FONT_GET_FONT_FAILED);
      return;
    }

    if (!AddFilesForFont(&path_set, &custom_font_path_set, font.Get())) {
      if (IsLastResortFallbackFont(family_index))
        LogMessageFilterError(LAST_RESORT_FONT_ADD_FILES_FAILED);
    }
  }

  std::vector<base::File> file_handles;
  // For files outside the windows fonts directory we pass them to the renderer
  // as file handles. The renderer would be unable to open the files directly
  // due to sandbox policy (it would get ERROR_ACCESS_DENIED instead). Passing
  // handles allows the renderer to bypass the restriction and use the fonts.
  for (const base::string16& custom_font_path : custom_font_path_set) {
    // Specify FLAG_EXCLUSIVE_WRITE to prevent base::File from opening the file
    // with FILE_SHARE_WRITE access. FLAG_EXCLUSIVE_WRITE doesn't actually open
    // the file for write access.
    base::File file(base::FilePath(custom_font_path),
                    base::File::FLAG_OPEN | base::File::FLAG_READ |
                        base::File::FLAG_EXCLUSIVE_WRITE);
    if (file.IsValid()) {
      file_handles.push_back(std::move(file));
    }
  }

  std::vector<base::FilePath> file_paths;
  for (const auto& path : path_set) {
    file_paths.emplace_back(base::FilePath(path));
  }
  LogLastResortFontFileCount(file_paths.size());
  std::move(callback).Run(file_paths, std::move(file_handles));
}

void DWriteFontProxyImpl::MapCharacters(const base::string16& text,
                                        mojom::DWriteFontStylePtr font_style,
                                        const base::string16& locale_name,
                                        uint32_t reading_direction,
                                        const base::string16& base_family_name,
                                        MapCharactersCallback callback) {
  InitializeDirectWrite();
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback),
      mojom::MapCharactersResult::New(
          UINT32_MAX, L"", text.length(), 0.0,
          mojom::DWriteFontStyle::New(DWRITE_FONT_STYLE_NORMAL,
                                      DWRITE_FONT_STRETCH_NORMAL,
                                      DWRITE_FONT_WEIGHT_NORMAL)));
  if (factory2_ == nullptr || collection_ == nullptr)
    return;
  if (font_fallback_ == nullptr) {
    if (FAILED(factory2_->GetSystemFontFallback(&font_fallback_))) {
      return;
    }
  }

  mswr::ComPtr<IDWriteFont> mapped_font;

  mswr::ComPtr<IDWriteNumberSubstitution> number_substitution;
  if (FAILED(factory2_->CreateNumberSubstitution(
          DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, locale_name.c_str(),
          TRUE /* ignoreUserOverride */, &number_substitution))) {
    DCHECK(false);
    return;
  }
  mswr::ComPtr<IDWriteTextAnalysisSource> analysis_source;
  if (FAILED(gfx::win::TextAnalysisSource::Create(
          &analysis_source, text, locale_name, number_substitution.Get(),
          static_cast<DWRITE_READING_DIRECTION>(reading_direction)))) {
    DCHECK(false);
    return;
  }

  auto result = mojom::MapCharactersResult::New(
      UINT32_MAX, L"", text.length(), 0.0,
      mojom::DWriteFontStyle::New(DWRITE_FONT_STYLE_NORMAL,
                                  DWRITE_FONT_STRETCH_NORMAL,
                                  DWRITE_FONT_WEIGHT_NORMAL));
  if (FAILED(font_fallback_->MapCharacters(
          analysis_source.Get(), 0, text.length(), collection_.Get(),
          base_family_name.c_str(),
          static_cast<DWRITE_FONT_WEIGHT>(font_style->font_weight),
          static_cast<DWRITE_FONT_STYLE>(font_style->font_slant),
          static_cast<DWRITE_FONT_STRETCH>(font_style->font_stretch),
          &result->mapped_length, &mapped_font, &result->scale))) {
    DCHECK(false);
    return;
  }

  if (mapped_font == nullptr) {
    std::move(callback).Run(std::move(result));
    return;
  }

  mswr::ComPtr<IDWriteFontFamily> mapped_family;
  if (FAILED(mapped_font->GetFontFamily(&mapped_family))) {
    DCHECK(false);
    return;
  }
  mswr::ComPtr<IDWriteLocalizedStrings> family_names;
  if (FAILED(mapped_family->GetFamilyNames(&family_names))) {
    DCHECK(false);
    return;
  }

  result->font_style->font_slant = mapped_font->GetStyle();
  result->font_style->font_stretch = mapped_font->GetStretch();
  result->font_style->font_weight = mapped_font->GetWeight();

  std::vector<base::char16> name;
  size_t name_count = family_names->GetCount();
  for (size_t name_index = 0; name_index < name_count; name_index++) {
    UINT32 name_length = 0;
    if (FAILED(family_names->GetStringLength(name_index, &name_length)))
      continue;  // Keep trying other names

    ++name_length;  // Reserve space for the null terminator.
    name.resize(name_length);
    if (FAILED(family_names->GetString(name_index, name.data(), name_length)))
      continue;
    UINT32 index = UINT32_MAX;
    BOOL exists = false;
    if (FAILED(collection_->FindFamilyName(name.data(), &index, &exists)) ||
        !exists)
      continue;

    // Found a matching family!
    result->family_index = index;
    result->family_name = name.data();
    std::move(callback).Run(std::move(result));
    return;
  }

  // Could not find a matching family
  LogMessageFilterError(MAP_CHARACTERS_NO_FAMILY);
  DCHECK_EQ(result->family_index, UINT32_MAX);
  DCHECK_GT(result->mapped_length, 0u);
}

void DWriteFontProxyImpl::InitializeDirectWrite() {
  if (direct_write_initialized_)
    return;
  direct_write_initialized_ = true;

  mswr::ComPtr<IDWriteFactory> factory;
  gfx::win::CreateDWriteFactory(&factory);
  if (factory == nullptr) {
    // We won't be able to load fonts, but we should still return messages so
    // renderers don't hang if they for some reason send us a font message.
    return;
  }

  // QueryInterface for IDWriteFactory2. It's ok for this to fail if we are
  // running an older version of DirectWrite (earlier than Win8.1).
  factory.As<IDWriteFactory2>(&factory2_);

  HRESULT hr = factory->GetSystemFontCollection(&collection_);
  DCHECK(SUCCEEDED(hr));

  if (!collection_) {
    base::UmaHistogramSparse(
        "DirectWrite.Fonts.Proxy.GetSystemFontCollectionResult", hr);
    LogMessageFilterError(ERROR_NO_COLLECTION);
    return;
  }

  if (!base::FeatureList::IsEnabled(kEnableCustomFonts))
    custom_font_file_loading_mode_ = DISABLE;
  else if (base::FeatureList::IsEnabled(kForceCustomFonts))
    custom_font_file_loading_mode_ = FORCE;

  // Temp code to help track down crbug.com/561873
  for (size_t font = 0; font < arraysize(kLastResortFontNames); font++) {
    uint32_t font_index = 0;
    BOOL exists = FALSE;
    if (SUCCEEDED(collection_->FindFamilyName(kLastResortFontNames[font],
                                              &font_index, &exists)) &&
        exists && font_index != UINT32_MAX) {
      last_resort_fonts_.push_back(font_index);
    }
  }
  LogLastResortFontCount(last_resort_fonts_.size());
}

bool DWriteFontProxyImpl::AddFilesForFont(
    std::set<base::string16>* path_set,
    std::set<base::string16>* custom_font_path_set,
    IDWriteFont* font) {
  mswr::ComPtr<IDWriteFontFace> font_face;
  HRESULT hr;
  hr = font->CreateFontFace(&font_face);
  if (FAILED(hr)) {
    base::UmaHistogramSparse("DirectWrite.Fonts.Proxy.CreateFontFaceResult",
                             hr);
    LogMessageFilterError(ADD_FILES_FOR_FONT_CREATE_FACE_FAILED);
    return false;
  }

  UINT32 file_count;
  hr = font_face->GetFiles(&file_count, nullptr);
  if (FAILED(hr)) {
    LogMessageFilterError(ADD_FILES_FOR_FONT_GET_FILE_COUNT_FAILED);
    return false;
  }

  std::vector<mswr::ComPtr<IDWriteFontFile>> font_files;
  font_files.resize(file_count);
  hr = font_face->GetFiles(
      &file_count, reinterpret_cast<IDWriteFontFile**>(font_files.data()));
  if (FAILED(hr)) {
    LogMessageFilterError(ADD_FILES_FOR_FONT_GET_FILES_FAILED);
    return false;
  }

  for (unsigned int file_index = 0; file_index < file_count; ++file_index) {
    mswr::ComPtr<IDWriteFontFileLoader> loader;
    hr = font_files[file_index]->GetLoader(&loader);
    if (FAILED(hr)) {
      LogMessageFilterError(ADD_FILES_FOR_FONT_GET_LOADER_FAILED);
      return false;
    }

    mswr::ComPtr<IDWriteLocalFontFileLoader> local_loader;
    hr = loader.CopyTo(local_loader.GetAddressOf());  // QueryInterface.

    if (hr == E_NOINTERFACE) {
      // We could get here if the system font collection contains fonts that
      // are backed by something other than files in the system fonts folder.
      // I don't think that is actually possible, so for now we'll just
      // ignore it (result will be that we'll be unable to match any styles
      // for this font, forcing blink/skia to fall back to whatever font is
      // next). If we get telemetry indicating that this case actually
      // happens, we can implement this by exposing the loader via ipc. That
      // will likely be by loading the font data into shared memory, although
      // we could proxy the stream reads directly instead.
      LogLoaderType(OTHER_LOADER);
      DCHECK(false);

      return false;
    } else if (FAILED(hr)) {
      LogMessageFilterError(ADD_FILES_FOR_FONT_QI_FAILED);
      return false;
    }

    if (!AddLocalFile(path_set, custom_font_path_set, local_loader.Get(),
                      font_files[file_index].Get())) {
      return false;
    }
  }
  return true;
}

bool DWriteFontProxyImpl::AddLocalFile(
    std::set<base::string16>* path_set,
    std::set<base::string16>* custom_font_path_set,
    IDWriteLocalFontFileLoader* local_loader,
    IDWriteFontFile* font_file) {
  HRESULT hr;
  const void* key;
  UINT32 key_size;
  hr = font_file->GetReferenceKey(&key, &key_size);
  if (FAILED(hr)) {
    LogMessageFilterError(ADD_LOCAL_FILE_GET_REFERENCE_KEY_FAILED);
    return false;
  }

  UINT32 path_length = 0;
  hr = local_loader->GetFilePathLengthFromKey(key, key_size, &path_length);
  if (FAILED(hr)) {
    LogMessageFilterError(ADD_LOCAL_FILE_GET_PATH_LENGTH_FAILED);
    return false;
  }
  ++path_length;  // Reserve space for the null terminator.
  std::vector<base::char16> file_path_chars;
  file_path_chars.resize(path_length);
  hr = local_loader->GetFilePathFromKey(key, key_size, file_path_chars.data(),
                                        path_length);
  if (FAILED(hr)) {
    LogMessageFilterError(ADD_LOCAL_FILE_GET_PATH_FAILED);
    return false;
  }

  base::string16 file_path = base::i18n::FoldCase(file_path_chars.data());

  if (!base::StartsWith(file_path, windows_fonts_path_,
                        base::CompareCase::SENSITIVE) ||
      custom_font_file_loading_mode_ == FORCE) {
    LogLoaderType(FILE_OUTSIDE_SANDBOX);
    if (custom_font_file_loading_mode_ != DISABLE)
      custom_font_path_set->insert(file_path);
  } else {
    LogLoaderType(FILE_SYSTEM_FONT_DIR);
    path_set->insert(file_path);
  }
  return true;
}

bool DWriteFontProxyImpl::IsLastResortFallbackFont(uint32_t font_index) {
  for (auto iter = last_resort_fonts_.begin(); iter != last_resort_fonts_.end();
       ++iter) {
    if (*iter == font_index)
      return true;
  }
  return false;
}

}  // namespace content
