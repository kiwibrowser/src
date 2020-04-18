// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_CHILD_PROCESS_SANDBOX_SUPPORT_IMPL_MAC_H_
#define CONTENT_CHILD_CHILD_PROCESS_SANDBOX_SUPPORT_IMPL_MAC_H_

#include <CoreText/CoreText.h>

namespace content {

// Load a font specified by |font| into |out| through communicating
// with browser.
bool LoadFont(CTFontRef font, CGFontRef* out, uint32_t* font_id);

};  // namespace content

#endif  // CONTENT_CHILD_CHILD_PROCESS_SANDBOX_SUPPORT_IMPL_MAC_H_
