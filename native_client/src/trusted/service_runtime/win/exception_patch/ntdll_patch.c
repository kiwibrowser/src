/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <windows.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"
#include "native_client/src/trusted/service_runtime/win/exception_patch/ntdll_patch.h"


/*
 * Unlike Unix, Windows does much of its dispatching to fault handling
 * routines in user space, rather than in the kernel.  Hence
 * AddVectoredExceptionHandler() adds a function to a list maintained
 * in user space.  When a fault occurs, the kernel passes control to
 * an internal userland routine in NTDLL.DLL called
 * KiUserExceptionDispatcher, which in turn runs handlers registered
 * by AddVectoredExceptionHandler().  Furthermore,
 * KiUserExceptionDispatcher gets run even if no handlers have been
 * registered.  This is unlike Unix's signal() function, which
 * modifies a table of functions maintained in the kernel.  At the
 * same time, Windows has no equivalent of Unix's sigaltstack(), so
 * KiUserExceptionDispatcher runs on the stack that %esp/%rsp pointed
 * to at the time the fault occurred.
 *
 * This is really bad for x86-64 NaCl, because %rsp points into
 * untrusted address space.  KiUserExceptionDispatcher calls functions
 * internally, and it would be possible for untrusted code to hijack
 * these function returns by writing to the stack from another thread.
 *
 * To avoid this problem, we patch KiUserExceptionDispatcher to safely
 * terminate the process.  Of course, we are relying on an
 * implementation detail of Windows here.
 *
 * We make a number of assumptions:
 *
 *  * We assume that there are no jumps to instructions that we might
 *    be overwriting, except for the first instruction.  There is not
 *    really any way to verify this.  We minimise this risk by keeping
 *    our patch sequence short.  It is unlikely that there are any
 *    jumps to KiUserExceptionDispatcher+i for 0 < i < 5.
 *
 *  * We assume that no-one else is patching this routine in the same
 *    process.
 *
 *  * We assume that no-one catches and skips the HLT instruction.
 *
 *  * We assume that KiUserExceptionDispatcher really is the first
 *    address that the kernel passes control to in user space after a
 *    fault.
 *
 *  * The kernel still writes fault information to the untrusted stack.
 *    We assume that nothing sensitive is included in this stack
 *    frame, because untrusted code will probably be able to read this
 *    information.
 *
 * An alternative approach might be to use Windows' debugging
 * interfaces (DebugActiveProcess() etc.) to catch the fault before
 * any fault handler inside the sel_ldr process has run.  This would
 * have the advantage that the kernel would not write a stack frame to
 * the untrusted stack.  However, it would involve having an extra
 * helper process running.
 */

/*
 * _tls_index is the module index of the executable/DLL we are linked
 * into.  It is part of the Windows x86-64 TLS ABI.  TLS variable
 * accesses normally read it using %rip-relative addressing, but since
 * we are copying our assembly-code template routine we might as well
 * patch in the value of _tls_index rather than relocating a
 * %rip-relative instruction.
 */
extern uint32_t _tls_index;

void *AllocatePageInRange(uint8_t *min_addr, uint8_t *max_addr) {
  MEMORY_BASIC_INFORMATION info;
  uint8_t *addr = (uint8_t *) NaClTruncAllocPage((uintptr_t) min_addr);
  while (addr < max_addr) {
    size_t result;
    result = VirtualQuery(addr, &info, sizeof(info));
    if (result == 0) {
      break;
    }
    CHECK(result == sizeof(info));
    if (info.State == MEM_FREE) {
      return VirtualAlloc(info.BaseAddress, NACL_MAP_PAGESIZE,
                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }
    /*
     * RegionSize can be a 4k multiple but not a 64k multiple, so we
     * have to round up, otherwise our subsequent attempt to
     * VirtualAlloc() a non-64k-aligned address will fail.
     */
    addr += NaClRoundAllocPage(info.RegionSize);
  }
  return NULL;
}

/*
 * Given that a chunk of code has been copied from old_base to
 * new_base, this function relocates a %rip-relative displacement
 * value (at new_base + offset) so that it still points to the same
 * address.
 */
static void RelocateRipRelative(uint8_t *new_base, uint8_t *old_base,
                                size_t offset) {
  size_t moved_by = new_base - old_base;
  int32_t *old_operand = (int32_t *) (old_base + offset);
  int32_t *new_operand = (int32_t *) (new_base + offset);

  int64_t new_displacement = *new_operand - moved_by;
  *new_operand = (int32_t) new_displacement;

  /*
   * Check that the displacement still points to the same location.
   * This could fail if the original displacement was big (e.g. if
   * NTDLL is big).
   */
  CHECK((uintptr_t) new_operand + *new_operand ==
        (uintptr_t) old_operand + *old_operand);
  /*
   * Do an additional sanity check because we were not taking into
   * account *exactly* where the displacement is measured from, which
   * depends on the size of the instruction that contains it.
   */
  CHECK(new_displacement < INT32_MAX - 0x100);
  CHECK(new_displacement > INT32_MIN + 0x100);
}

const int kJump32Size = 5;

/*
 * This function writes the following instruction at address "code":
 *   e9 XX XX XX XX    jmp jump_target
 */
static void WriteJump32(uint8_t *code, uint8_t *jump_target) {
  uint8_t *instruction_end = code + kJump32Size;
  code[0] = 0xe9;
  *(int32_t *) &code[1] = (int32_t) (jump_target - instruction_end);
  /* Check that jump_target is in range. */
  CHECK(instruction_end + *(int32_t *) &code[1] == jump_target);
}

uint8_t *NaClGetKiUserExceptionDispatcher(void) {
  uint8_t *handler;
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  if (ntdll == NULL) {
    NaClLog(LOG_FATAL,
            "NaClGetKiUserExceptionDispatcher: "
            "Failed to open ntdll.dll\n");
  }
  handler = (uint8_t *) (uintptr_t)
            GetProcAddress(ntdll, "KiUserExceptionDispatcher");
  if (handler == NULL) {
    NaClLog(LOG_FATAL,
            "NaClGetKiUserExceptionDispatcher: "
            "GetProcAddress() failed to get KiUserExceptionDispatcher\n");
  }
  return handler;
}

int NaClPatchWindowsExceptionDispatcherWithCheck(void) {
  uint8_t *ntdll_routine = NaClGetKiUserExceptionDispatcher();
  uint8_t *intercept_code;
  uint8_t *dest;
  uint32_t *reloc_addr;
  size_t template_size = (NaCl_exception_dispatcher_intercept_end -
                          NaCl_exception_dispatcher_intercept);
  DWORD old_prot;

  /*
   * We patch the start of KiUserExceptionDispatcher with a 32-bit
   * relative jump to our routine, so our routine needs to be
   * relocated to within +/-2GB of KiUserExceptionDispatcher's
   * address.  To avoid edge cases, we allocate within the smaller
   * range of +/-1.5GB.
   *
   * We use a 32-bit relative jump because it's short -- 5 bytes --
   * which minimises the amount of code we have to overwrite and
   * relocate.  If we did a 64-bit jump, we'd need to do something
   * like the following:
   *   50                              push %rax
   *   48 b8 XX XX XX XX XX XX XX XX   mov $0xXXXXXXXXXXXXXXXX, %rax
   *   ff e0                           jmpq *%rax
   * which is 13 bytes.
   */
  const size_t kMaxDistance = 1536 << 20;

  /*
   * Check for the instructions:
   *   fc                     cld
   *   48 8b 05 XX XX XX XX   mov XXXXXXXX(%rip), %rax
   *
   * Note that if you set a breakpoint on KiUserExceptionDispatcher
   * with WinDbg, this check will fail because the first byte will
   * have been replaced with int3.
   */
  size_t bytes_to_move = 8;
  size_t rip_relative_operand_offset = 4;
  if (ntdll_routine[0] == 0xfc &&
      ntdll_routine[1] == 0x48 &&
      ntdll_routine[2] == 0x8b &&
      ntdll_routine[3] == 0x05) {
    NaClLog(2, "NaClPatchWindowsExceptionDispatcherWithCheck: "
            "Got instructions expected for Windows Vista and Windows 7\n");
  } else {
    NaClLog(ERROR, "NaClPatchWindowsExceptionDispatcherWithCheck: "
            "Unexpected start instructions\n");
    return 0; /* Failure */
  }

  intercept_code = AllocatePageInRange(ntdll_routine - kMaxDistance,
                                       ntdll_routine + kMaxDistance);
  if (intercept_code == NULL) {
    NaClLog(LOG_FATAL, "NaClPatchWindowsExceptionDispatcherWithCheck: "
            "AllocatePageInRange() failed\n");
  }
  /* Fill the page with HLTs just in case. */
  NaClFillMemoryRegionWithHalt(intercept_code, NACL_MAP_PAGESIZE);
  dest = intercept_code;

  /* Copy template code and fill out a parameter in it. */
  memcpy(dest, NaCl_exception_dispatcher_intercept, template_size);
  reloc_addr =
    (uint32_t *) (dest + (NaCl_exception_dispatcher_intercept_tls_index -
                          NaCl_exception_dispatcher_intercept) + 1);
  CHECK(*reloc_addr == 0x12345678);
  *reloc_addr = _tls_index;
  dest += template_size;

  /*
   * Copy and relocate instructions from KiUserExceptionDispatcher
   * that we will be overwriting.
   */
  memcpy(dest, ntdll_routine, bytes_to_move);
  RelocateRipRelative(dest, ntdll_routine, rip_relative_operand_offset);
  dest += bytes_to_move;

  /*
   * Lastly, write a jump that returns to the unmodified portion of
   * KiUserExceptionDispatcher.
   */
  WriteJump32(dest, ntdll_routine + bytes_to_move);

  if (!VirtualProtect(intercept_code, NACL_MAP_PAGESIZE, PAGE_EXECUTE_READ,
                      &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcherWithCheck: VirtualProtect() "
            "failed to make our intercept routine executable\n");
  }

  if (!VirtualProtect(ntdll_routine, kJump32Size + 1,
                      PAGE_EXECUTE_READWRITE, &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcherWithCheck: "
            "VirtualProtect() failed to make the routine writable\n");
  }
  /*
   * We write the jump after the "cld" instruction so that, if you
   * want to set a breakpoint on KiUserExceptionDispatcher, you can
   * take out the checks for "cld" and this will still work.
   */
  CHECK(ntdll_routine[0] == 0xfc); /* "cld" instruction */
  WriteJump32(ntdll_routine + 1, intercept_code);
  if (!VirtualProtect(ntdll_routine, kJump32Size + 1, old_prot, &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcherWithCheck: "
            "VirtualProtect() failed to restore page permissions\n");
  }
  return 1; /* Success */
}

void NaClPatchWindowsExceptionDispatcherToFailFast(void) {
  DWORD old_prot;
  uint8_t *patch_bytes = NaCl_exception_dispatcher_exit_fast;
  size_t patch_size = (NaCl_exception_dispatcher_exit_fast_end -
                       NaCl_exception_dispatcher_exit_fast);
  uint8_t *ntdll_routine = NaClGetKiUserExceptionDispatcher();

  if (!VirtualProtect(ntdll_routine, patch_size,
                      PAGE_EXECUTE_READWRITE, &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcherToFailFast: "
            "VirtualProtect() failed to make the routine writable\n");
  }
  memcpy(ntdll_routine, patch_bytes, patch_size);
  if (!VirtualProtect(ntdll_routine, patch_size, old_prot, &old_prot)) {
    NaClLog(LOG_FATAL,
            "NaClPatchWindowsExceptionDispatcherToFailFast: "
            "VirtualProtect() failed to restore page permissions\n");
  }
}

void NaClPatchWindowsExceptionDispatcher(void) {
  /*
   * We first try to apply the smarter patch that can recover from
   * trusted crashes, but if NTDLL contains unfamiliar code, we fall
   * back to the simpler fail-fast patch.  We do this on the grounds
   * that only Breakpad crash reporting relies on the smarter patch,
   * and it's better to have NaCl working without Breakpad on newer
   * versions of Windows than not working at all.
   *
   * The fallback assumes that no-one is relying on catching faults
   * inside trusted code.  This includes:
   *  * MSVC's __try/__except feature.
   *  * IsBadReadPtr() et al.  (Code that uses these functions is not
   *    fail-safe, so if IsBadReadPtr() is called on an invalid
   *    pointer, it is probably good that our patch makes it fail
   *    fast.)
   *
   * TODO(mseaborn): It would be useful to report (via UMA) whether
   * this fallback has occurred.
   */
  if (!NaClPatchWindowsExceptionDispatcherWithCheck()) {
    NaClPatchWindowsExceptionDispatcherToFailFast();
  }
}
