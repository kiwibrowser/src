// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/module_ids.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "chrome/grit/browser_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

struct { size_t id; const char* module_name_digest; }
  kExpectedInstallModules[] = {
    {1u, "c8cc47613e155f2129f480c6ced84549"},  // chrome.dll
    {2u, "49b78a23b0d8d5d8fb60d4e472b22764"},  // chrome_child.dll
  };

// Parses a line consisting of a positive decimal number and a 32-digit
// hexadecimal number, separated by a space. Inserts the output, if valid, into
// |module_ids|. Unexpected leading or trailing characters will cause
// the line to be ignored, as will invalid decimal/hexadecimal characters.
void ParseAdditionalModuleID(
    const base::StringPiece& line,
    ModuleIDs* module_ids) {
  DCHECK(module_ids);

  base::CStringTokenizer line_tokenizer(line.begin(), line.end(), " ");

  if (!line_tokenizer.GetNext())
    return;  // Empty string.
  base::StringPiece id_piece(line_tokenizer.token_piece());

  if (!line_tokenizer.GetNext())
    return;  // No delimiter (' ').
  base::StringPiece digest_piece(line_tokenizer.token_piece());

  if (line_tokenizer.GetNext())
    return;  // Unexpected trailing characters.

  unsigned id = 0;
  if (!StringToUint(id_piece, &id))
    return;  // First token was not decimal.

  if (digest_piece.length() != 32)
    return;  // Second token is not the right length.

  for (base::StringPiece::const_iterator it = digest_piece.begin();
       it != digest_piece.end(); ++it) {
    if (!base::IsHexDigit(*it))
      return;  // Second token has invalid characters.
  }

  // This is a valid line.
  module_ids->insert(std::make_pair(digest_piece.as_string(), id));
}

}  // namespace

void ParseAdditionalModuleIDs(
    const base::StringPiece& raw_data,
    ModuleIDs* module_ids) {
  DCHECK(module_ids);

  base::CStringTokenizer file_tokenizer(raw_data.begin(),
                                        raw_data.end(),
                                        "\r\n");
  while (file_tokenizer.GetNext()) {
    ParseAdditionalModuleID(base::StringPiece(file_tokenizer.token_piece()),
                            module_ids);
  }
}

void LoadModuleIDs(ModuleIDs* module_ids) {
  for (size_t i = 0; i < arraysize(kExpectedInstallModules); ++i) {
    module_ids->insert(
        std::make_pair(
            kExpectedInstallModules[i].module_name_digest,
            kExpectedInstallModules[i].id));
  }
  base::StringPiece additional_module_ids;
#if defined(GOOGLE_CHROME_BUILD)
  additional_module_ids =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_ADDITIONAL_MODULE_IDS);
#endif
  ParseAdditionalModuleIDs(additional_module_ids, module_ids);
}
