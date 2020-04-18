// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/speech_recognition_error_struct_traits.h"

namespace mojo {

bool StructTraits<content::mojom::SpeechRecognitionErrorDataView,
                  content::SpeechRecognitionError>::
    Read(content::mojom::SpeechRecognitionErrorDataView data,
         content::SpeechRecognitionError* out) {
  if (!data.ReadCode(&out->code)) {
    return false;
  }
  if (!data.ReadDetails(&out->details)) {
    return false;
  }
  return true;
}

}  // namespace mojo
