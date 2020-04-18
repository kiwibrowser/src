// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of NtMapViewOfSection intercept for 32 bit builds.
//
// TODO(robertshield): Implement the 64 bit intercept.

#include "chrome_elf/blacklist/blacklist_interceptions.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

// Note that only #includes from base that are either header-only or built into
// base_static (see base/base.gyp) are allowed here.
#include "base/win/pe_image.h"
#include "chrome_elf/blacklist/blacklist.h"
#include "chrome_elf/crash/crash_helper.h"
#include "sandbox/win/src/internal_types.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sandbox_types.h"

namespace {

NtQuerySectionFunction g_nt_query_section_func = NULL;
NtQueryVirtualMemoryFunction g_nt_query_virtual_memory_func = NULL;
NtUnmapViewOfSectionFunction g_nt_unmap_view_of_section_func = NULL;

// TODO(robertshield): Merge with ntdll exports cache.
FARPROC GetNtDllExportByName(const char* export_name) {
  HMODULE ntdll = ::GetModuleHandle(sandbox::kNtdllName);
  return ::GetProcAddress(ntdll, export_name);
}

// TODO(robertshield): Some of the helper functions below overlap somewhat with
// code in sandbox_nt_util.cc. See if they can be unified.

// Native reimplementation of PSAPIs GetMappedFileName.
std::wstring GetBackingModuleFilePath(PVOID address) {
  DCHECK_NT(g_nt_query_virtual_memory_func);

  // We'll start with something close to max_path characters for the name.
  SIZE_T buffer_bytes = MAX_PATH * 2;
  std::vector<BYTE> buffer_data(buffer_bytes);

  for (;;) {
    MEMORY_SECTION_NAME* section_name =
        reinterpret_cast<MEMORY_SECTION_NAME*>(&buffer_data[0]);

    if (!section_name)
      break;

    SIZE_T returned_bytes;
    NTSTATUS ret = g_nt_query_virtual_memory_func(
        NtCurrentProcess, address, MemorySectionName, section_name,
        buffer_bytes, &returned_bytes);

    if (STATUS_BUFFER_OVERFLOW == ret) {
      // Retry the call with the given buffer size.
      buffer_bytes = returned_bytes + 1;
      buffer_data.resize(buffer_bytes);
      section_name = NULL;
      continue;
    }
    if (!NT_SUCCESS(ret))
      break;

    UNICODE_STRING* section_string =
        reinterpret_cast<UNICODE_STRING*>(section_name);
    return std::wstring(section_string->Buffer,
                        section_string->Length / sizeof(wchar_t));
  }

  return std::wstring();
}

bool IsModuleValidImageSection(HANDLE section,
                               PVOID* base,
                               PLARGE_INTEGER offset,
                               PSIZE_T view_size) {
  DCHECK_NT(g_nt_query_section_func);

  if (!section || !base || !view_size || offset)
    return false;

  SECTION_BASIC_INFORMATION basic_info;
  SIZE_T bytes_returned;
  NTSTATUS ret = g_nt_query_section_func(section, SectionBasicInformation,
                                         &basic_info, sizeof(basic_info),
                                         &bytes_returned);

  if (!NT_SUCCESS(ret) || sizeof(basic_info) != bytes_returned)
    return false;

  if (!(basic_info.Attributes & SEC_IMAGE))
    return false;

  return true;
}

std::wstring ExtractLoadedModuleName(const std::wstring& module_path) {
  if (module_path.empty() || module_path.back() == L'\\')
    return std::wstring();

  size_t sep = module_path.find_last_of(L'\\');
  if (sep == std::wstring::npos)
    return module_path;
  return module_path.substr(sep + 1);
}

// Fills |out_name| with the image name from the given |pe| image and |flags|
// with additional info about the image.
void SafeGetImageInfo(const base::win::PEImage& pe,
                      std::string* out_name,
                      uint32_t* flags) {
  out_name->clear();
  out_name->reserve(MAX_PATH);
  *flags = 0;
  __try {
    if (pe.VerifyMagic()) {
      *flags |= sandbox::MODULE_IS_PE_IMAGE;

      PIMAGE_EXPORT_DIRECTORY exports = pe.GetExportDirectory();
      if (exports) {
        const char* image_name = reinterpret_cast<const char*>(
            pe.RVAToAddr(exports->Name));
        size_t i = 0;
        for (; i < MAX_PATH && *image_name; ++i, ++image_name)
          out_name->push_back(*image_name);
      }

      PIMAGE_NT_HEADERS headers = pe.GetNTHeaders();
      if (headers) {
        if (headers->OptionalHeader.AddressOfEntryPoint)
          *flags |= sandbox::MODULE_HAS_ENTRY_POINT;
        if (headers->OptionalHeader.SizeOfCode)
          *flags |= sandbox::MODULE_HAS_CODE;
      }
    }
  } __except((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
              GetExceptionCode() == EXCEPTION_GUARD_PAGE ||
              GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    out_name->clear();
  }
}

std::wstring GetImageInfoFromLoadedModule(HMODULE module, uint32_t* flags) {
  std::string out_name;
  base::win::PEImage pe(module);
  SafeGetImageInfo(pe, &out_name, flags);
  return std::wstring(out_name.begin(), out_name.end());
}

bool IsSameAsCurrentProcess(HANDLE process) {
  return  (NtCurrentProcess == process) ||
          (::GetProcessId(process) == ::GetCurrentProcessId());
}

NTSTATUS BlNtMapViewOfSectionImpl(
    NtMapViewOfSectionFunction orig_MapViewOfSection,
    HANDLE section,
    HANDLE process,
    PVOID* base,
    ULONG_PTR zero_bits,
    SIZE_T commit_size,
    PLARGE_INTEGER offset,
    PSIZE_T view_size,
    SECTION_INHERIT inherit,
    ULONG allocation_type,
    ULONG protect) {
  NTSTATUS ret = orig_MapViewOfSection(section, process, base, zero_bits,
                                       commit_size, offset, view_size, inherit,
                                       allocation_type, protect);

  if (!NT_SUCCESS(ret) || !IsSameAsCurrentProcess(process) ||
      !IsModuleValidImageSection(section, base, offset, view_size)) {
    return ret;
  }

  HMODULE module = reinterpret_cast<HMODULE>(*base);
  if (module) {
    UINT image_flags;

    std::wstring module_name_from_image(GetImageInfoFromLoadedModule(
        reinterpret_cast<HMODULE>(*base), &image_flags));

    int blocked_index = blacklist::DllMatch(module_name_from_image);

    // If the module name isn't blacklisted, see if the file name is different
    // and blacklisted.
    if (blocked_index == -1) {
      std::wstring file_name(GetBackingModuleFilePath(*base));
      std::wstring module_name_from_file = ExtractLoadedModuleName(file_name);

      if (module_name_from_image != module_name_from_file)
        blocked_index = blacklist::DllMatch(module_name_from_file);
    }

    if (blocked_index != -1) {
      DCHECK_NT(g_nt_unmap_view_of_section_func);
      g_nt_unmap_view_of_section_func(process, *base);
      ret = STATUS_UNSUCCESSFUL;

      blacklist::BlockedDll(blocked_index);
    }
  }

  return ret;
}

}  // namespace

namespace blacklist {

bool InitializeInterceptImports() {
  g_nt_query_section_func =
      reinterpret_cast<NtQuerySectionFunction>(
          GetNtDllExportByName("NtQuerySection"));
  g_nt_query_virtual_memory_func =
      reinterpret_cast<NtQueryVirtualMemoryFunction>(
          GetNtDllExportByName("NtQueryVirtualMemory"));
  g_nt_unmap_view_of_section_func =
      reinterpret_cast<NtUnmapViewOfSectionFunction>(
          GetNtDllExportByName("NtUnmapViewOfSection"));

  return (g_nt_query_section_func && g_nt_query_virtual_memory_func &&
          g_nt_unmap_view_of_section_func);
}

SANDBOX_INTERCEPT NTSTATUS WINAPI
BlNtMapViewOfSection(NtMapViewOfSectionFunction orig_MapViewOfSection,
                     HANDLE section,
                     HANDLE process,
                     PVOID* base,
                     ULONG_PTR zero_bits,
                     SIZE_T commit_size,
                     PLARGE_INTEGER offset,
                     PSIZE_T view_size,
                     SECTION_INHERIT inherit,
                     ULONG allocation_type,
                     ULONG protect) {
  NTSTATUS ret = STATUS_UNSUCCESSFUL;

  __try {
    ret = BlNtMapViewOfSectionImpl(orig_MapViewOfSection, section, process,
                                   base, zero_bits, commit_size, offset,
                                   view_size, inherit, allocation_type,
                                   protect);
  } __except (elf_crash::GenerateCrashDump(GetExceptionInformation())) {
  }

  return ret;
}

#if defined(_WIN64)
NTSTATUS WINAPI BlNtMapViewOfSection64(
    HANDLE section, HANDLE process, PVOID *base, ULONG_PTR zero_bits,
    SIZE_T commit_size, PLARGE_INTEGER offset, PSIZE_T view_size,
    SECTION_INHERIT inherit, ULONG allocation_type, ULONG protect) {
  return BlNtMapViewOfSection(g_nt_map_view_of_section_func, section, process,
                              base, zero_bits, commit_size, offset, view_size,
                              inherit, allocation_type, protect);
}
#endif
}  // namespace blacklist
