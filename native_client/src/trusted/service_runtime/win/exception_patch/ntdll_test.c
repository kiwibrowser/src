/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <windows.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/win/exception_patch/ntdll_patch.h"


/*
 * This code tests the behaviour of Windows rather than testing
 * NaClPatchWindowsExceptionDispatcher().
 */

static int g_counter = 0;

void CauseFault(void) {
  *(int *) 0 = 0;
}

void ReplacementHandler(void) {
  printf("In replacement handler (%i)\n", g_counter++);
  if (g_counter < 5) {
    /*
     * Cause a fault to demonstrate that ReplacementHandler() gets
     * re-run by the kernel.
     */
    CauseFault();
  } else {
    /*
     * Check that our exit_fast routine does not re-run ReplacementHandler().
     */
    void (*exit_fast)(void)
      = (void (*)(void)) NaCl_exception_dispatcher_exit_fast;
    fprintf(stderr, "** intended_exit_status=untrusted_segfault\n");
    exit_fast();
  }
}

int main(void) {
  HMODULE ntdll;
  uint8_t *handler;
  DWORD old_prot;
  int i;
  int patch_size = 12;

  /* Turn off stdout/stderr buffering. */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  ntdll = GetModuleHandleA("ntdll.dll");
  if (ntdll == NULL) {
    NaClLog(LOG_FATAL, "Failed to open ntdll.dll\n");
  }
  handler = (uint8_t *) GetProcAddress(ntdll, "KiUserExceptionDispatcher");
  if (handler == NULL) {
    NaClLog(LOG_FATAL,
            "GetProcAddress() failed to get KiUserExceptionDispatcher\n");
  }

  fprintf(stderr,
          "KiUserExceptionDispatcher starts with the following bytes:\n");
  for (i = 0; i < 20; i++) {
    /*
     * Print in a form such that this can easily be run through an
     * assembler with '.ascii "..."' and then disassembled.
     */
    fprintf(stderr, "\\x%02x", handler[i]);
  }
  fprintf(stderr, "\n");
  /*
   * Sanity check: Does the existing routine start with instructions
   * that we have seen before on known versions of x86-64 Windows?  If
   * not, there is probably no problem, but we might want to
   * investigate.  It should be OK to add new instructions to the
   * whitelist here if this test fails.
   *
   * Check for the instructions:
   *   fc                     cld
   *   48 8b 05 XX XX XX XX   mov XXXXXXXX(%rip), %rax
   */
  if (handler[0] == 0xfc &&
      handler[1] == 0x48 &&
      handler[2] == 0x8b &&
      handler[3] == 0x05) {
    fprintf(stderr,
            "Got instructions expected for Windows Vista and Windows 7\n");
  } else {
    fprintf(stderr, "Unexpected start instructions\n");
    return 1;
  }

  if (!VirtualProtect(handler, patch_size,
                      PAGE_EXECUTE_READWRITE, &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcher: "
            "VirtualProtect() failed to make the routine writable\n");
  }

  /*
   * We cannot use a direct jump because direct jumps only accept a
   * 32-bit displacement, and ReplacementHandler might be too far from
   * KiUserExceptionDispatcher.
   *
   * Patch in these instructions:
   *   48 b8 XX XX XX XX XX XX XX XX   movabs $XXXXXXXXXXXXXXXX, %rax
   *   ff e0                           jmp *%rax
   */
  handler[0] = 0x48;
  handler[1] = 0xb8;
  *(uint64_t *) &handler[2] = (uint64_t) ReplacementHandler;
  handler[10] = 0xff;
  handler[11] = 0xe0;

  if (!VirtualProtect(handler, patch_size, old_prot, &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcher: "
            "VirtualProtect() failed to restore page permissions\n");
  }

  CauseFault();

  fprintf(stderr, "Should not reach here.\n");
  return 1;
}
