// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/app/blink_test_platform_support.h"

#include <windows.h>
#include <stddef.h>
#include <iostream>
#include <list>
#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/win/direct_write.h"

namespace content {

namespace {

bool SetupFonts() {
  // Load Ahem font. Ahem.ttf is copied to the build directory by
  // //third_party/test_fonts .
  base::FilePath base_path;
  base::PathService::Get(base::DIR_MODULE, &base_path);
  base::FilePath font_path =
      base_path.Append(FILE_PATH_LITERAL("/test_fonts/Ahem.ttf"));

  const char kRegisterFontFiles[] = "register-font-files";
  // DirectWrite sandbox registration.
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitchASCII(kRegisterFontFiles,
                                 base::WideToUTF8(font_path.value()));

  return true;
}

}  // namespace

bool CheckLayoutSystemDeps() {
  std::list<std::string> errors;

  // This metric will be 17 when font size is "Normal".
  // The size of drop-down menus depends on it.
  if (::GetSystemMetrics(SM_CXVSCROLL) != 17)
    errors.push_back("Must use normal size fonts (96 dpi).");

  NONCLIENTMETRICS metrics = {0};
  metrics.cbSize = sizeof(NONCLIENTMETRICS);
  bool success = !!::SystemParametersInfo(
      SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
  CHECK(success);
  LOGFONTW* system_fonts[] =
      {&metrics.lfStatusFont, &metrics.lfMenuFont, &metrics.lfSmCaptionFont};
  const wchar_t required_font[] = L"Segoe UI";
  int required_font_size = -12;
  for (size_t i = 0; i < arraysize(system_fonts); ++i) {
    if (system_fonts[i]->lfHeight != required_font_size ||
        wcscmp(required_font, system_fonts[i]->lfFaceName)) {
      errors.push_back("Must use either the Aero or Basic theme.");
      break;
    }
  }

  for (const auto& error : errors)
    std::cerr << error << "\n";
  return errors.empty();
}

bool BlinkTestPlatformInitialize() {
  return SetupFonts();
}

}  // namespace content
