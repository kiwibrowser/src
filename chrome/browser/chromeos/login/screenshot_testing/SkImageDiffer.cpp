// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WARNING! This file is copied from third_party/skia/tools/skpdiff and slightly
// modified to be compilable outside Skia and suit chromium style. Some comments
// can make no sense.
// TODO(elizavetai): remove this file and reuse the original one in Skia

#include "base/compiler_specific.h"
#include "chrome/browser/chromeos/login/screenshot_testing/SkImageDiffer.h"

const double SkImageDiffer::RESULT_CORRECT = 1.0f;
const double SkImageDiffer::RESULT_INCORRECT = 0.0f;

SkImageDiffer::SkImageDiffer() {
}

SkImageDiffer::~SkImageDiffer() {
}

SkImageDiffer::Result::Result() {
}