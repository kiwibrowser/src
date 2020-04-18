// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/win/direct_write.h"

#include <wrl/client.h>

#include "base/command_line.h"
#include "base/debug/alias.h"
#include "base/metrics/field_trial.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "skia/ext/fontmgr_default_win.h"
#include "third_party/skia/include/ports/SkFontMgr.h"
#include "third_party/skia/include/ports/SkTypeface_win.h"
#include "ui/gfx/platform_font_win.h"
#include "ui/gfx/switches.h"

namespace gfx {
namespace win {

void CreateDWriteFactory(IDWriteFactory** factory) {
  Microsoft::WRL::ComPtr<IUnknown> factory_unknown;
  HRESULT hr =
      DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                          factory_unknown.GetAddressOf());
  if (FAILED(hr)) {
    base::debug::Alias(&hr);
    CHECK(false);
    return;
  }
  factory_unknown.CopyTo(factory);
}

void MaybeInitializeDirectWrite() {
  static bool tried_dwrite_initialize = false;
  if (tried_dwrite_initialize)
    return;
  tried_dwrite_initialize = true;

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableDirectWriteForUI)) {
    return;
  }

  Microsoft::WRL::ComPtr<IDWriteFactory> factory;
  CreateDWriteFactory(factory.GetAddressOf());

  if (!factory)
    return;

  // The skia call to create a new DirectWrite font manager instance can fail
  // if we are unable to get the system font collection from the DirectWrite
  // factory. The GetSystemFontCollection method in the IDWriteFactory
  // interface fails with E_INVALIDARG on certain Windows 7 gold versions
  // (6.1.7600.*). We should just use GDI in these cases.
  sk_sp<SkFontMgr> direct_write_font_mgr =
      SkFontMgr_New_DirectWrite(factory.Get());
  if (!direct_write_font_mgr)
    return;
  SetDefaultSkiaFactory(std::move(direct_write_font_mgr));
  gfx::PlatformFontWin::SetDirectWriteFactory(factory.Get());
}

}  // namespace win
}  // namespace gfx
