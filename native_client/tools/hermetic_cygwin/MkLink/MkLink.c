/*
 * Copyright 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include <commctrl.h>
#include <nsis/pluginapi.h>
#include <winternl.h>

enum { kLargeBuf = 1024, kSmallBuf = 256 } ;

#if defined(_MSC_VER)
/* Ensure these are treated as functions and not inlined as intrinsics, or disable /Oi */
#pragma warning(disable:4164)  /* intrinsic function not declared */
#pragma function(memcpy, memset, memcmp)
#endif

HMODULE hNrDll = NULL;
NTSTATUS (NTAPI *fNtCreateFile) (PHANDLE FileHandle,
  ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
  PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize,
  ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition,
  ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
NTSTATUS (NTAPI *fNtClose) (HANDLE Handle);

HMODULE hKernel32 = NULL;
BOOL (WINAPI *fCreateHardLink) (TCHAR * linkFileName,
  TCHAR * existingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL (WINAPI *fCreateSymbolicLink) (TCHAR * linkFileName,
  TCHAR * existingFileName, DWORD flags);

#undef CreateHardLink
#undef CreateSymbolicLink
#ifdef UNICODE
#define CreateHardLink "CreateHardLinkW"
#define CreateSymbolicLink "CreateSymbolicLinkW"
#else
#define CreateHardLink "CreateHardLinkA"
#define CreateSymbolicLink "CreateSymbolicLinkA"
#endif

BOOL MakeHardLink(TCHAR *linkFileName, TCHAR *existingFileName) {
  if (!hKernel32)
    hKernel32 = LoadLibrary(_T("KERNEL32.DLL"));
  if (hKernel32) {
    if (!fCreateHardLink)
      fCreateHardLink = GetProcAddress(hKernel32, CreateHardLink);
    if (fCreateHardLink)
      return fCreateHardLink(linkFileName, existingFileName, NULL);
  }
  return FALSE;
}

BOOL MakeSymLink(TCHAR *linkFileName, TCHAR *existingFileName, BOOL dirLink) {
  TCHAR *f1 = HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*kLargeBuf);
  TCHAR *f2 = HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*kLargeBuf);
  SECURITY_ATTRIBUTES sec_attr = { sizeof (SECURITY_ATTRIBUTES), NULL, FALSE};
  OBJECT_ATTRIBUTES obj_attr;
  IO_STATUS_BLOCK io_block;
  TCHAR *p, *q;
  HANDLE f;
  BOOL status;
  if (!f1 || !f2)
    return FALSE;
  lstrcpy(f1, linkFileName);
  for (p = f1; p[0]; p++)
    if (p[0] == _T('/'))
      p[0] = _T('\\');
  q = f1;
  while (q[0]) {
    p = q;
    do {
      q++;
    } while (q[0] && q[0] != '\\');
  }
  if (p[0] = '\\') {
    TCHAR c = p[1];
    p[1] = 0;
    status = GetVolumeInformation(f1, NULL, 0, NULL, NULL, NULL,
                                  f2, sizeof(f2));
    p[1] = c;
  } else {
    status = GetVolumeInformation(NULL, NULL, 0, NULL, NULL, NULL,
                                  f2, sizeof(f2));
  }
  /* If it's NFS then we can create real symbolic link. */
  if (!lstrcmpi(f2, _T("NFS"))) {
    lstrcpy(f2, existingFileName);
    for (p = f2; p[0]; p++)
      if (p[0] == _T('\\'))
        p[0] = _T('/');
    if (!hNrDll)
      hNrDll = LoadLibrary(_T("NTDLL.DLL"));
    if (hNrDll) {
      if (!fNtCreateFile)
        fNtCreateFile = GetProcAddress(hNrDll, "NtCreateFile");
      if (!fNtClose)
        fNtClose = GetProcAddress(hNrDll, "NtClose");
      if (fNtCreateFile && fNtClose) {
        struct {
          ULONG offset;
          UCHAR flags;
          UCHAR nameLength;
          USHORT valueLength;
          CHAR name[21];
          /* To prevent troubles with alignment */
          CHAR value[kLargeBuf*sizeof(WCHAR)];
        } *ea_info = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(*ea_info));
        WCHAR *fn = HeapAlloc(GetProcessHeap(),
                              0,
                              sizeof(TCHAR)*kLargeBuf);
        UNICODE_STRING n = { lstrlen(f1), kLargeBuf, fn };
        ea_info->nameLength = 20;
        lstrcpy(ea_info->name, "NfsSymlinkTargetName");
#ifdef UNICODE
        lstrcpy(fn, f1);
        lstrcpy((LPWSTR)ea_info->value, existingFileName);
#else
        MultiByteToWideChar(CP_ACP, 0, f1, -1, fn, kLargeBuf);
        MultiByteToWideChar(CP_ACP, 0, existingFileName, -1,
                            (LPWSTR)ea_info->value,
                            sizeof(ea_info->value)/sizeof(WCHAR));
#endif
        ea_info->valueLength =
            lstrlenW((LPWSTR)ea_info->value)*sizeof(WCHAR);
        InitializeObjectAttributes(&obj_attr, &n, OBJ_CASE_INSENSITIVE,
                                   NULL, NULL);
        status = fNtCreateFile(
            &f, FILE_WRITE_DATA | FILE_WRITE_EA | SYNCHRONIZE, &obj_attr,
            &io_block, NULL, FILE_ATTRIBUTE_SYSTEM, FILE_SHARE_READ |
            FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_CREATE,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
            &ea_info, 1024 * sizeof (WCHAR));
        if (NT_SUCCESS(status)) {
          fNtClose(f);
          HeapFree(GetProcessHeap(), 0, fn);
          HeapFree(GetProcessHeap(), 0, ea_info);
          HeapFree(GetProcessHeap(), 0, f2);
          HeapFree(GetProcessHeap(), 0, f1);
          return TRUE;
        }
        HeapFree(GetProcessHeap(), 0, fn);
        HeapFree(GetProcessHeap(), 0, ea_info);
      }
    }
  }
  lstrcpy(f2, existingFileName);
  for (p = f2; p[0]; p++)
    if (p[0] == _T('/'))
      p[0] = _T('\\');
  if (!hKernel32)
    hKernel32 = LoadLibrary(_T("KERNEL32.DLL"));
  if (hKernel32) {
    if (!fCreateSymbolicLink)
      fCreateSymbolicLink = GetProcAddress(hKernel32, CreateSymbolicLink);
    if (fCreateSymbolicLink) {
      if (fCreateSymbolicLink(f1, f2,
                              dirLink ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0)) {
        HeapFree(GetProcessHeap(), 0, f2);
        HeapFree(GetProcessHeap(), 0, f1);
        return TRUE;
      }
    }
  }
  if (dirLink) {
    /* Ignore errors - file may already exist */
    CreateDirectory(f1, NULL);
    f = CreateFile(f1, GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                   NULL );
    if (f != INVALID_HANDLE_VALUE) {
      struct rp_info {
        DWORD tag;
        DWORD dataLength;
        WORD reserved1;
        WORD targetLength;
        WORD targetMaxLength;
        WORD reserved2;
        WCHAR target[kLargeBuf+4];
      } *rp_info = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           sizeof(*rp_info));
      DWORD len;
      WCHAR *startlink, *endlink;
      rp_info->tag = IO_REPARSE_TAG_MOUNT_POINT;
      rp_info->target[0] = L'\\';
      rp_info->target[1] = L'?';
      rp_info->target[2] = L'?';
      rp_info->target[3] = L'\\';
      if (((f2[0] == _T('\\')) && (f2[1] == _T('\\'))) ||
          ((f2[1] == _T(':')) && (f2[2] == _T('\\')))) {
#ifdef UNICODE
        lstrcpy(rp_info->target+4, f2);
#else
        MultiByteToWideChar(CP_ACP, 0, f2, -1,
                            rp_info->target+4, kLargeBuf);
#endif
      } else {
#ifdef UNICODE
        GetFullPathNameW(f1, 1024, rp_info->target+4, &startlink);
        lstrcpy(startlink, f2);
#else
        MultiByteToWideChar(CP_ACP, 0, f1, -1,
                            (LPWSTR)f1, kLargeBuf/sizeof(WCHAR));
        GetFullPathNameW(f1, 1024, rp_info->target+4, &startlink);
        MultiByteToWideChar(CP_ACP, 0, f2, -1,
                            startlink, kLargeBuf+4-(startlink-rp_info->target));
#endif
      }
      /* Remove "XXX/../" and replace "/" with "\" */
      for (startlink = endlink = rp_info->target+4;
                                           endlink[0]; startlink++, endlink++) {
        startlink[0] = endlink[0];
        if ((startlink[0] == L'\\') &&
            (startlink[-1] == L'.') &&
            (startlink[-2] == L'.')) {
          for (startlink--; startlink > rp_info->target+4 &&
                                             startlink[0] != L'\\'; startlink--)
            { }
          for (startlink--; startlink > rp_info->target+4 &&
                                             startlink[0] != L'\\'; startlink--)
            { }
          if (startlink < rp_info->target+4)
            startlink = rp_info->target+4;
        }
      }
      startlink[0] = endlink[0];
      rp_info->targetLength = lstrlenW(rp_info->target)*sizeof(WCHAR);
      rp_info->targetMaxLength = rp_info->targetLength+sizeof(WCHAR);
      rp_info->dataLength = rp_info->targetMaxLength
                            +FIELD_OFFSET(struct rp_info, target)
                            -FIELD_OFFSET(struct rp_info, reserved1)
                            +sizeof(WCHAR);
      if (DeviceIoControl(f, 0x900A4, rp_info,
                          rp_info->dataLength
                                       +FIELD_OFFSET(struct rp_info, reserved1),
                          NULL, 0, &len, NULL)) {
        CloseHandle(f);
        HeapFree(GetProcessHeap(), 0, rp_info);
        HeapFree(GetProcessHeap(), 0, f2);
        HeapFree(GetProcessHeap(), 0, f1);
        return TRUE;
      }
      CloseHandle(f);
      RemoveDirectory(f1);
      HeapFree(GetProcessHeap(), 0, rp_info);
    }
  }
  for (p = f2; p[0]; p++)
    if (p[0] == _T('\\'))
      p[0] = _T('/');
  f = CreateFile(f1, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                 CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM, NULL);
  if (f != INVALID_HANDLE_VALUE) {
    struct {
      WCHAR sig[4];
      WCHAR value[kLargeBuf];
    } *link_info = HeapAlloc(GetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             sizeof(*link_info));
    DWORD towrite, written;
    link_info->sig[0] = 0x6e49;
    link_info->sig[1] = 0x7874;
    link_info->sig[2] = 0x4e4c;
    link_info->sig[3] = 0x014b;
#ifdef UNICODE
    lstrcpy(link_info->value, f2);
#else
    MultiByteToWideChar(CP_ACP, 0, f2, -1, link_info->value, kLargeBuf);
#endif
    towrite = lstrlenW(link_info->value)*sizeof(WCHAR)+sizeof(link_info->sig);
    WriteFile(f, link_info, towrite, &written, NULL);
    CloseHandle(f);
    if (written == towrite) {
      HeapFree(GetProcessHeap(), 0, link_info);
      HeapFree(GetProcessHeap(), 0, f2);
      HeapFree(GetProcessHeap(), 0, f1);
      return TRUE;
    }
    HeapFree(GetProcessHeap(), 0, link_info);
  }
  HeapFree(GetProcessHeap(), 0, f2);
  HeapFree(GetProcessHeap(), 0, f1);
  return FALSE;
}

HINSTANCE instance;

HWND parent, list;

/*
 * Show message in NSIS details window.
 */
void NSISprint(const TCHAR *str) {
  if (list && *str) {
    LVITEM item = {
      /* mask */ LVIF_TEXT,
      /* iItem */ SendMessage(list, LVM_GETITEMCOUNT, 0, 0),
      /* iSubItem */ 0, /* state */ 0, /* stateMask */ 0,
      /* pszText */ (TCHAR *) str, /* cchTextMax */ 0,
      /* iImage */ 0, /* lParam */ 0, /* iIndent */ 0,
      /* iGroupId */ 0, /* cColumns */ 0, /* puColumns */ NULL,
      /* piColFmt */ NULL, /* iGroup */ 0};
    ListView_InsertItem(list, &item);
    ListView_EnsureVisible(list, item.iItem, 0);
  }
}

enum linktype { HARDLINK, SOFTLINKD, SOFTLINKF };

void makelink(HWND hwndParent, int string_size, TCHAR *variables,
              stack_t **stacktop, extra_parameters *extra, enum linktype type) {
  TCHAR *msg = HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*kLargeBuf);
  TCHAR *from = HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*kSmallBuf);
  TCHAR *to = HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*kSmallBuf);
  TCHAR *msgFormat =
      type == HARDLINK ? _T("Link: \"%s\" to \"%s\"%s") :
      type == SOFTLINKD ? _T("Symbolic Directory Link: \"%s\" to \"%s\"%s") :
      _T("Symbolic Link: \"%s\" to \"%s\"%s");
  BOOL res;
  parent = hwndParent;
  list = FindWindowEx(FindWindowEx(parent, NULL, _T("#32770"), NULL),
                      NULL, _T("SysListView32"), NULL);

  EXDLL_INIT();

  if (!msg || !from || !to) {
    MessageBox(parent, _T("Fatal error: no memory for MkLink"), 0, MB_OK);
  }

  if (popstringn(from, kSmallBuf)) {
    MessageBox(parent,
      _T("Usage: MkLink::Hard \"to_file\" \"from_file\" "), 0, MB_OK);
  }

  if (popstringn(to, kSmallBuf)) {
    MessageBox(parent,
      _T("Usage: MkLink::Hard \"fo_file\" \"from_file\" "),0,MB_OK);
  }

  switch (type) {
    case HARDLINK:
      res = MakeHardLink(from, to);
      break;
    case SOFTLINKD:
      res = MakeSymLink(from, to, TRUE);
      break;
    case SOFTLINKF:
      res = MakeSymLink(from, to, FALSE);
      break;
  }
  wsprintf(msg, msgFormat, to, from, res ? _T("") : _T(" - fail..."));
  NSISprint(msg);

  HeapFree(GetProcessHeap(), 0, to);
  HeapFree(GetProcessHeap(), 0, from);
  HeapFree(GetProcessHeap(), 0, msg);
}

void __declspec(dllexport) Hard(HWND hwndParent, int string_size,
                                TCHAR *variables, stack_t **stacktop,
                                extra_parameters *extra) {
  makelink(hwndParent, string_size, variables, stacktop, extra, HARDLINK);
}

void __declspec(dllexport) SoftD(HWND hwndParent, int string_size,
                                 TCHAR *variables, stack_t **stacktop,
                                 extra_parameters *extra) {
  makelink(hwndParent, string_size, variables, stacktop, extra, SOFTLINKD);
}

void __declspec(dllexport) SoftF(HWND hwndParent, int string_size,
                                 TCHAR *variables, stack_t **stacktop,
                                 extra_parameters *extra) {
  makelink(hwndParent, string_size, variables, stacktop, extra, SOFTLINKF);
}

BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved) {
  instance = hInst;
  return TRUE;
}
