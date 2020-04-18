// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfpf_skiafilefont.h"

#include "core/fxcrt/fx_stream.h"

CFPF_SkiaFileFont::CFPF_SkiaFileFont() = default;

CFPF_SkiaFileFont::~CFPF_SkiaFileFont() = default;

int32_t CFPF_SkiaFileFont::GetType() const {
  return FPF_SKIAFONTTYPE_File;
}
