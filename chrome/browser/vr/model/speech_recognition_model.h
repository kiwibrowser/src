// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_SPEECH_RECOGNITION_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_SPEECH_RECOGNITION_MODEL_H_

#include "base/strings/string16.h"

namespace vr {

struct SpeechRecognitionModel {
  int speech_recognition_state = 0;
  bool has_or_can_request_audio_permission = true;
  base::string16 recognition_result;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_SPEECH_RECOGNITION_MODEL_H_
