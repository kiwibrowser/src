// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_PRINTING_HANDLER_H_
#define CHROME_UTILITY_PRINTING_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "build/build_config.h"
#include "printing/buildflags/buildflags.h"

#if !defined(OS_WIN) || !BUILDFLAG(ENABLE_PRINT_PREVIEW)
#error "Windows printing and print preview must be enabled"
#endif

namespace IPC {
class Message;
}

namespace printing {

// Dispatches IPCs for printing.
class PrintingHandler {
 public:
  PrintingHandler();
  ~PrintingHandler();

  bool OnMessageReceived(const IPC::Message& message);

 private:
  // IPC message handlers.
  void OnGetPrinterCapsAndDefaults(const std::string& printer_name);
  void OnGetPrinterSemanticCapsAndDefaults(const std::string& printer_name);

  DISALLOW_COPY_AND_ASSIGN(PrintingHandler);
};

}  // namespace printing

#endif  // CHROME_UTILITY_PRINTING_HANDLER_H_
