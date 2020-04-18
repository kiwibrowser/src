// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/text_detection_impl_win.h"

#include <windows.foundation.collections.h>
#include <windows.globalization.h>
#include <memory>
#include <string>

#include "base/logging.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/text_detection_impl.h"

namespace shape_detection {

using ABI::Windows::Foundation::Collections::IVectorView;
using ABI::Windows::Globalization::ILanguageFactory;
using ABI::Windows::Media::Ocr::IOcrEngineStatics;
using ABI::Windows::Media::Ocr::IOcrLine;
using ABI::Windows::Media::Ocr::OcrLine;
using base::win::GetActivationFactory;
using base::win::ScopedHString;

// static
void TextDetectionImpl::Create(mojom::TextDetectionRequest request) {
  // OcrEngine class is only available in Win 10 onwards (v10.0.10240.0) that
  // documents in
  // https://docs.microsoft.com/en-us/uwp/api/windows.media.ocr.ocrengine.
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    DVLOG(1) << "Optical character recognition not supported before Windows 10";
    return;
  }
  DCHECK_GE(base::win::OSInfo::GetInstance()->version_number().build, 10240);

  // Loads functions dynamically at runtime to prevent library dependencies.
  if (!(base::win::ResolveCoreWinRTDelayload() &&
        ScopedHString::ResolveCoreWinRTStringDelayload())) {
    DLOG(ERROR) << "Failed loading functions from combase.dll";
    return;
  }

  // Text Detection specification only supports Latin-1 text as documented in
  // https://wicg.github.io/shape-detection-api/text.html#text-detection-api.
  // TODO(junwei.fu): https://crbug.com/794097 consider supporting other Latin
  // script language.
  ScopedHString language_hstring = ScopedHString::Create("en");
  if (!language_hstring.is_valid())
    return;

  Microsoft::WRL::ComPtr<ILanguageFactory> language_factory;
  HRESULT hr =
      GetActivationFactory<ILanguageFactory,
                           RuntimeClass_Windows_Globalization_Language>(
          &language_factory);
  if (FAILED(hr)) {
    DLOG(ERROR) << "ILanguage factory failed: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  Microsoft::WRL::ComPtr<ABI::Windows::Globalization::ILanguage> language;
  hr = language_factory->CreateLanguage(language_hstring.get(), &language);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Create language failed: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  Microsoft::WRL::ComPtr<IOcrEngineStatics> engine_factory;
  hr = GetActivationFactory<IOcrEngineStatics,
                            RuntimeClass_Windows_Media_Ocr_OcrEngine>(
      &engine_factory);
  if (FAILED(hr)) {
    DLOG(ERROR) << "IOcrEngineStatics factory failed: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  boolean is_supported = false;
  hr = engine_factory->IsLanguageSupported(language.Get(), &is_supported);
  if (FAILED(hr) || !is_supported)
    return;

  Microsoft::WRL::ComPtr<IOcrEngine> ocr_engine;
  hr = engine_factory->TryCreateFromLanguage(language.Get(), &ocr_engine);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Create engine failed from language: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory;
  hr = GetActivationFactory<
      ISoftwareBitmapStatics,
      RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap>(&bitmap_factory);
  if (FAILED(hr)) {
    DLOG(ERROR) << "ISoftwareBitmapStatics factory failed: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  auto impl = std::make_unique<TextDetectionImplWin>(std::move(ocr_engine),
                                                     std::move(bitmap_factory));
  auto* impl_ptr = impl.get();
  impl_ptr->SetBinding(
      mojo::MakeStrongBinding(std::move(impl), std::move(request)));
}

TextDetectionImplWin::TextDetectionImplWin(
    Microsoft::WRL::ComPtr<IOcrEngine> ocr_engine,
    Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory)
    : ocr_engine_(std::move(ocr_engine)),
      bitmap_factory_(std::move(bitmap_factory)),
      weak_factory_(this) {
  DCHECK(ocr_engine_);
  DCHECK(bitmap_factory_);
}

TextDetectionImplWin::~TextDetectionImplWin() = default;

void TextDetectionImplWin::Detect(const SkBitmap& bitmap,
                                  DetectCallback callback) {
  if (FAILED(BeginDetect(bitmap))) {
    // No detection taking place; run |callback| with an empty array of results.
    std::move(callback).Run(std::vector<mojom::TextDetectionResultPtr>());
    return;
  }
  // Hold on the callback until AsyncOperation completes.
  recognize_text_callback_ = std::move(callback);
  // This prevents the Detect function from being called before the
  // AsyncOperation completes.
  binding_->PauseIncomingMethodCallProcessing();
}

HRESULT TextDetectionImplWin::BeginDetect(const SkBitmap& bitmap) {
  Microsoft::WRL::ComPtr<ISoftwareBitmap> win_bitmap =
      CreateWinBitmapFromSkBitmap(bitmap, bitmap_factory_.Get());
  if (!win_bitmap)
    return E_FAIL;

  // Recognize text asynchronously.
  AsyncOperation<OcrResult>::IAsyncOperationPtr async_op;
  const HRESULT hr = ocr_engine_->RecognizeAsync(win_bitmap.Get(), &async_op);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Recognize text asynchronously failed: "
                << logging::SystemErrorCodeToString(hr);
    return hr;
  }

  // Use WeakPtr to bind the callback so that the once callback will not be run
  // if this object has been already destroyed. |win_bitmap| needs to be kept
  // alive until OnTextDetected().
  return AsyncOperation<OcrResult>::BeginAsyncOperation(
      base::BindOnce(&TextDetectionImplWin::OnTextDetected,
                     weak_factory_.GetWeakPtr(), std::move(win_bitmap)),
      std::move(async_op));
}

std::vector<mojom::TextDetectionResultPtr>
TextDetectionImplWin::BuildTextDetectionResult(
    AsyncOperation<OcrResult>::IAsyncOperationPtr async_op) {
  std::vector<mojom::TextDetectionResultPtr> results;
  Microsoft::WRL::ComPtr<IOcrResult> ocr_result;
  HRESULT hr =
      async_op ? async_op->GetResults(ocr_result.GetAddressOf()) : E_FAIL;
  if (FAILED(hr)) {
    DLOG(ERROR) << "GetResults failed: "
                << logging::SystemErrorCodeToString(hr);
    return results;
  }

  Microsoft::WRL::ComPtr<IVectorView<OcrLine*>> ocr_lines;
  hr = ocr_result->get_Lines(ocr_lines.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "Get Lines failed: " << logging::SystemErrorCodeToString(hr);
    return results;
  }

  uint32_t count;
  hr = ocr_lines->get_Size(&count);
  if (FAILED(hr)) {
    DLOG(ERROR) << "get_Size failed: " << logging::SystemErrorCodeToString(hr);
    return results;
  }

  results.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    Microsoft::WRL::ComPtr<IOcrLine> line;
    hr = ocr_lines->GetAt(i, &line);
    if (FAILED(hr))
      break;

    HSTRING text;
    hr = line->get_Text(&text);
    if (FAILED(hr))
      break;

    auto result = shape_detection::mojom::TextDetectionResult::New();
    result->raw_value = ScopedHString(text).GetAsUTF8();
    results.push_back(std::move(result));
  }
  return results;
}

// |win_bitmap| is passed here so that it is kept alive until the AsyncOperation
// completes because RecognizeAsync does not hold a reference.
void TextDetectionImplWin::OnTextDetected(
    Microsoft::WRL::ComPtr<ISoftwareBitmap> /* win_bitmap */,
    AsyncOperation<OcrResult>::IAsyncOperationPtr async_op) {
  std::move(recognize_text_callback_)
      .Run(BuildTextDetectionResult(std::move(async_op)));
  binding_->ResumeIncomingMethodCallProcessing();
}

}  // namespace shape_detection
