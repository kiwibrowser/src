// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/pdf_compositor/public/cpp/pdf_compositor_service_factory.h"

#include "build/build_config.h"
#include "components/services/pdf_compositor/pdf_compositor_service.h"
#include "content/public/utility/utility_thread.h"
#include "third_party/blink/public/platform/web_image_generator.h"
#include "third_party/skia/include/core/SkGraphics.h"

namespace printing {

std::unique_ptr<service_manager::Service> CreatePdfCompositorService(
    const std::string& creator) {
#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
  content::UtilityThread::Get()->EnsureBlinkInitializedWithSandboxSupport();
#else
  content::UtilityThread::Get()->EnsureBlinkInitialized();
#endif
  // Hook up blink's codecs so skia can call them.
  SkGraphics::SetImageGeneratorFromEncodedDataFactory(
      blink::WebImageGenerator::CreateAsSkImageGenerator);
  return printing::PdfCompositorService::Create(creator);
}

}  // namespace printing
