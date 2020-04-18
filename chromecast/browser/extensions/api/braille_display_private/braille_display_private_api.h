// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_API_BRAILLE_DISPLAY_PRIVATE_BRAILLE_DISPLAY_PRIVATE_API_H_
#define CHROMECAST_BROWSER_EXTENSIONS_API_BRAILLE_DISPLAY_PRIVATE_BRAILLE_DISPLAY_PRIVATE_API_H_

#include "chromecast/common/extensions_api/braille_display_private.h"
#include "extensions/browser/api/async_api_function.h"

namespace extensions {
namespace cast {
namespace api {

class BrailleDisplayPrivateGetDisplayStateFunction : public AsyncApiFunction {
  DECLARE_EXTENSION_FUNCTION("brailleDisplayPrivate.getDisplayState",
                             BRAILLEDISPLAYPRIVATE_GETDISPLAYSTATE)
 protected:
  ~BrailleDisplayPrivateGetDisplayStateFunction() override {}
  bool Prepare() override;
  void Work() override;
  bool Respond() override;
};

class BrailleDisplayPrivateWriteDotsFunction : public AsyncApiFunction {
  DECLARE_EXTENSION_FUNCTION("brailleDisplayPrivate.writeDots",
                             BRAILLEDISPLAYPRIVATE_WRITEDOTS);

 public:
  BrailleDisplayPrivateWriteDotsFunction();

 protected:
  ~BrailleDisplayPrivateWriteDotsFunction() override;
  bool Prepare() override;
  void Work() override;
  bool Respond() override;

 private:
  std::unique_ptr<braille_display_private::WriteDots::Params> params_;
};

}  // namespace api
}  // namespace cast
}  // namespace extensions

#endif  // CHROMECAST_BROWSER_EXTENSIONS_API_BRAILLE_DISPLAY_PRIVATE_BRAILLE_DISPLAY_PRIVATE_API_H_
