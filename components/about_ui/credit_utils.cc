// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/about_ui/credit_utils.h"

#include <stdint.h>

#include "base/strings/string_piece.h"
#include "components/grit/components_resources.h"
#include "third_party/brotli/include/brotli/decode.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_ANDROID)
#include "base/android/jni_array.h"
#include "jni/CreditUtils_jni.h"
#endif

namespace about_ui {

std::string GetCredits(bool include_scripts) {
  std::string response;
  base::StringPiece raw_response =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_ABOUT_UI_CREDITS_HTML);
  const uint8_t* next_encoded_byte =
      reinterpret_cast<const uint8_t*>(raw_response.data());
  size_t input_size_remaining = raw_response.size();
  BrotliDecoderState* decoder = BrotliDecoderCreateInstance(
      nullptr /* no custom allocator */, nullptr /* no custom deallocator */,
      nullptr /* no custom memory handle */);
  CHECK(!!decoder);
  while (!BrotliDecoderIsFinished(decoder)) {
    size_t output_size_remaining = 0;
    CHECK(BrotliDecoderDecompressStream(
              decoder, &input_size_remaining, &next_encoded_byte,
              &output_size_remaining, nullptr,
              nullptr) != BROTLI_DECODER_RESULT_ERROR);
    const uint8_t* output_buffer =
        BrotliDecoderTakeOutput(decoder, &output_size_remaining);
    response.insert(response.end(), output_buffer,
                    output_buffer + output_size_remaining);
  }
  BrotliDecoderDestroyInstance(decoder);
  if (include_scripts) {
    response +=
        "\n<script src=\"chrome://resources/js/cr.js\"></script>\n"
        "<script src=\"chrome://credits/credits.js\"></script>\n";
  }
  response += "</body>\n</html>";
  return response;
}

#if defined(OS_ANDROID)
static base::android::ScopedJavaLocalRef<jbyteArray>
JNI_CreditUtils_GetJavaWrapperCredits(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  std::string html_content = GetCredits(false);
  const char* html_content_arr = html_content.c_str();
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(html_content_arr),
      html_content.size());
}
#endif

}  // namespace about_ui
