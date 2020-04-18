// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <windows.h>

#include "base/files/file_path.h"
#include "base/scoped_native_library.h"

namespace {

//------------------------------------------------------------------------------
// PRIVATE: Control Flow Guard test things
// - MS binary indirect call tests.
//------------------------------------------------------------------------------

// These structures hold chunks of binary.  No padding wanted.
#pragma pack(push, 1)

#if defined(_WIN64)

const USHORT kCmpRsi = 0x3B48;
const BYTE kRax = 0xF0;
const USHORT kJzLoc = 0x0074;
const ULONG kMovRcxRsiCall = 0xFFCE8B48;
const BYTE kPadding = 0x00;
const ULONG kMovEcxEbxCallRsi = 0xD6FFCB8B;

const ULONG kMovRcxRsiAdd = 0x48CE8B48;
const ULONG kRcx10hNop = 0x9010C183;

// Sequence of bytes in GetSystemMetrics.
struct Signature {
  // This struct contains roughly the following code:
  // cmp        rsi, rax
  // jz         jmp_addr
  // mov        rcx, rsi
  // call       ULONG_addr <-- CFG check function
  // mov        ecx, ebx
  // call       rsi <-- Actual call to GetSystemMetrics

  // Patch will start here and be 8 bytes.
  USHORT cmp_rsi;                // = 48 3B
  BYTE rax;                      // = F0
  USHORT jz_loc;                 // = 74 XX
  ULONG mov_rcx_rsi_call;        // = 48 8B CE FF
  ULONG guard_check_icall_fptr;  // = XX XX XX XX
  BYTE padding;                  // = 00
  ULONG mov_ecx_ebx_call_rsi;    // = 8B CB FF D6
};

struct Patch {
  // Just add 16 to the existing function address.
  // This ensures a 16-byte aligned address that is
  // definitely not a valid function address.
  // mov        rcx, rsi = 48 4B CE
  // add        rcx, 10h = 48 83 C1 10
  // nop                 = 90
  ULONG first_four_bytes;   // = 48 4B CE 48
  ULONG second_four_bytes;  // = 83 C1 10 90
};

#else  // x86

const USHORT kCmpEbx = 0x0081;
const USHORT kJzLoc = 0x0074;
const ULONG kMovEcxEbxCall = 0x15FFCB8B;
const USHORT kCallEbx = 0xD3FF;

const ULONG kMovEcxEbxAddEcx = 0xC183CB8B;
const ULONG k16Nops = 0x90909010;
const USHORT kTwoNops = 0x9090;

// Sequence of bytes in GetSystemMetrics, x86.
struct Signature {
  // This struct contains roughly the following code:
  // cmp        ebx, offset
  // jz         jmp_addr
  // mov        ecx, ebx
  // call       ULONG_addr <-- CFG check function
  // call       ebx <-- Actual call to GetSystemMetrics

  // Patch will start here and be 10 bytes.
  USHORT cmp_ebx;                // = 81 XX
  ULONG addr;                    // = XX XX XX XX
  USHORT jz_loc;                 // = 74 XX
  ULONG mov_ecx_ebx_call;        // = 8B CB FF 15
  ULONG guard_check_icall_fptr;  // = XX XX XX XX
  USHORT call_ebx;               // = FF D3
};

struct Patch {
  // Just add 16 to the existing function address.
  // This ensures a 16-byte aligned address that is
  // definitely not a valid function address.
  // mov        ecx, ebx = 8B CB
  // add        ecx, 10h = 83 C1 10 90
  // nop                 = 90 90 90 90
  ULONG first_four_bytes;   // = 8B CB 83 C1
  ULONG second_four_bytes;  // = 10 90 90 90
  USHORT last_two_bytes;    // = 90 90
};

#endif  // _WIN64

#pragma pack(pop)
//------------------------------------------------------------------------------

// - Search binary starting at |address_start| for a matching chunk of
//   |binary_to_find|, of size |size_to_match|.
// - A byte of value 0 in |binary_to_find|, is a wildcard for anything.
// - Give a |max_distance| to find the chunk within, before failing.
// - If return value is true, |out_offset| will hold the offset from
//   |address_start| that the match starts.
bool FindBinary(BYTE* binary_to_find,
                DWORD size_to_match,
                BYTE* address_start,
                DWORD max_distance,
                DWORD* out_offset) {
  assert(size_to_match <= max_distance);
  assert(size_to_match > 0);

  BYTE* max_byte = address_start + max_distance - size_to_match;
  BYTE* temp_ptr = address_start;
  // Yes, it's a double while loop.
  while (temp_ptr <= max_byte) {
    size_t i = 0;
    // 0 is a wildcard match.
    while (binary_to_find[i] == 0 || temp_ptr[i] == binary_to_find[i]) {
      // Check if this is the last byte.
      if (i == size_to_match - 1) {
        *out_offset = temp_ptr - address_start;
        return true;
      }
      ++i;
    }
    ++temp_ptr;
  }

  return false;
}

// - Will write patch starting at |start_addr| with the patch for x86
//   or x64.
// - This function is only for writes within this process.
bool DoPatch(BYTE* start_addr) {
  Patch patch = {};
#if defined(_WIN64)
  patch.first_four_bytes = kMovRcxRsiAdd;
  patch.second_four_bytes = kRcx10hNop;
#else   // x86
  patch.first_four_bytes = kMovEcxEbxAddEcx;
  patch.second_four_bytes = k16Nops;
  patch.last_two_bytes = kTwoNops;
#endif  // _WIN64

  DWORD old_protection;
  // PAGE_WRITECOPY explicitly allows "copy-on-write" behaviour for
  // system DLL patches.
  if (!::VirtualProtect(start_addr, sizeof(patch), PAGE_WRITECOPY,
                        &old_protection))
    return false;

  ::memcpy(start_addr, &patch, sizeof(patch));
  ::VirtualProtect(start_addr, sizeof(patch), old_protection, &old_protection);

  return true;
}

// - Find the offset from |start| that the x86 or x64 signature starts.
bool FindSignature(BYTE* start, DWORD* offset_found) {
  Signature signature = {};
#if defined(_WIN64)
  signature.cmp_rsi = kCmpRsi;
  signature.rax = kRax;
  signature.jz_loc = kJzLoc;
  signature.mov_rcx_rsi_call = kMovRcxRsiCall;
  signature.padding = kPadding;
  signature.mov_ecx_ebx_call_rsi = kMovEcxEbxCallRsi;

  // This is far enough into GetSystemMetrics that the signature should
  // have been found.  See disassembly.
  DWORD max_area = 0xB0;
#else   // x86
  signature.cmp_ebx = kCmpEbx;
  signature.jz_loc = kJzLoc;
  signature.mov_ecx_ebx_call = kMovEcxEbxCall;
  signature.call_ebx = kCallEbx;

  // This is far enough into GetSystemMetrics x86 that the signature should
  // have been found.  See disassembly.
  DWORD max_area = 0xD9;
#endif  // _WIN64
  if (!FindBinary(reinterpret_cast<BYTE*>(&signature), sizeof(signature), start,
                  max_area, offset_found))
    return false;

  return true;
}

// - This function tests for CFG in MS system binaries, in process.
//
// A few words about the patching for this test:
//
//  - In both x86 and x64, the function FindSignature() will scan
//    GetSystemMetrics in user32.dll, to find the ideal place chosen
//    for this test.  It's a spot where a CFG check was compiled into
//    a Microsoft system DLL.  For more visualization, open user32.dll
//    in IDA to follow along (especially if planning to change this test).
//
//  - The CFG security check basically calls __guard_check_icall_fptr, with
//    the function address about to be called as the argument in EAX/RAX reg.
//    If the address is not in the process' "valid indirect call bitmap", a
//    CFG exception will be thrown.
//
//  - The DoPatch() function then overwrites with a small, custom change.  This
//    change will simply add 16 (0x10) to the real function address in EAX/RAX
//    about to be checked.  This will maintain the 16-byte alignment required in
//    a target address by CFG, but also ensure that it fails the check.
//
//    The whole purpose of this unittest is to ensure that a failed CFG check in
//    a Microsoft binary results in an exception.  If CFG is not properly
//    enabled for a process, no exception will be thrown.
//    All Chromium projects should be linked with "common_linker_setup" config
//    (build\config\win\BUILD.gn), which should result in CFG enabled on the
//    process.
//
//  - The patches (x86 or x64) were carefully constructed to be valid and not
//    mess up the executing instructions.  Need to ensure that the CFG check
//    fully happens, and that nothing else goes wrong before OR AFTER that
//    point.  The only exception expected is a very intentional one.
//    **The patches also allow the call to GetSystemMetrics to SUCCEED if CFG is
//    NOT enabled for the process!  This makes for very clear behaviour.
void TestMsIndirect() {
  base::ScopedNativeLibrary user32(base::FilePath(L"user32.dll"));
  if (!user32.is_valid())
    _exit(1);

  using GetSystemMetricsFunction = decltype(&::GetSystemMetrics);
  GetSystemMetricsFunction get_system_metrics =
      reinterpret_cast<GetSystemMetricsFunction>(
          user32.GetFunctionPointer("GetSystemMetrics"));
  if (!get_system_metrics)
    _exit(2);

  // Sanity check the function works fine pre-patch.  Tests should only be
  // running from normal boot (0).
  if (0 != get_system_metrics(SM_CLEANBOOT))
    _exit(3);

  BYTE* target = reinterpret_cast<BYTE*>(get_system_metrics);
  DWORD offset = 0;
  if (!FindSignature(target, &offset))
    _exit(4);

  // Now patch the function.  Don't bother saving original code,
  // as this process will end very soon.
  if (!DoPatch(target + offset))
    _exit(5);

  // Call the patched function!
  get_system_metrics(SM_CLEANBOOT);
}

}  // namespace

//------------------------------------------------------------------------------
// PUBLIC
//------------------------------------------------------------------------------

// Good ol' main.
// - Exe exits with non-zero return codes for unexpected errors.
// - Return code of zero indicates no issues at all.
// - Else, a CFG exception will result in the process being destroyed.
int main(int argc, char** argv) {
  if (argc != 2)
    _exit(-1);

  const char* arg = argv[1];

  int iarg = ::atoi(arg);
  if (!iarg)
    _exit(-1);

  switch (iarg) {
    // kSysDllTest
    case 1:
      TestMsIndirect();
      break;
    // Unsupported argument.
    default:
      _exit(-1);
  }

  return 0;
}
