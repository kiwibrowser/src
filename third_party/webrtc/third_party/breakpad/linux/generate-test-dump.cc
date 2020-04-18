// Copyright (c) 2009, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This code exists to generate minidump files for testing.

#include <stdlib.h>

#include <unistd.h>

#include "third_party/breakpad/breakpad/src/client/linux/handler/exception_handler.h"
#include "third_party/breakpad/breakpad/src/common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"

static bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                         void* context,
                         bool success) {
  if (!success) {
    static const char msg[] = "Failed to write minidump\n";
    sys_write(2, msg, sizeof(msg) - 1);
    return false;
  }

  static const char msg[] = "Wrote minidump: ";
  sys_write(2, msg, sizeof(msg) - 1);
  sys_write(2, descriptor.path(), strlen(descriptor.path()));
  sys_write(2, "\n", 1);

  return true;
}

static void DoSomethingWhichCrashes() {
  int local_var = 1;
  *reinterpret_cast<volatile char*>(NULL) = 1;
}

int main() {
  google_breakpad::MinidumpDescriptor minidump(".");
  google_breakpad::ExceptionHandler breakpad(minidump, NULL, DumpCallback, NULL,
                                             true, -1);
  DoSomethingWhichCrashes();
  return 0;
}
