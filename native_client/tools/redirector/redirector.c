/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <wchar.h>
#include <windows.h>

#include "native_client/tools/redirector/redirector.h"

/*
 * The toolchain redirector reads it's name and invokes the appropriate binary
 * from a location with cygwin1.dll nearby so that all cygwin-oriented
 * toolchain programs can start (this works for Cygwin version >= 1.7).
 *
 * Supports arbitrary unicode names in it's base directory name.
 *
 * The executable must be placed in nacl/bin or nacl64/bin or just bin (in the
 * latter case the name of the binary should be prefixed with nacl- or nacl64-)
 *
 * The binary should be built by visual studio (not cygwin) since when it starts
 * cygwin has not been located yet.
 *
 * Use the following commands:
 *   cl /c /O2 /GS- redirector.c /DCYGWIN_REDIRECTORS=1
 *   link /subsystem:console /MERGE:.rdata=.text /NODEFAULTLIB
 *        redirector.obj kernel32.lib user32.lib
 */

static int debug_enabled = 0;


/*
 * Wraps GetModuleFileNameW to always succeed in finding a buffer of
 * appropriate size.
 */
static
int get_module_name_safely(wchar_t **oldpath) {
  int length = 128;
  int real_size;
  wchar_t *result;
  *oldpath = NULL;
  while (1) {
    result = HeapAlloc(
      GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(wchar_t) * length);
    if (result == NULL) return 0;
    real_size = GetModuleFileNameW(NULL, result, length);
    if (real_size == 0) {
      HeapFree(GetProcessHeap(), 0, result);
      return 0;
    }
    if (real_size < length - 1) {
      /* Success */
      *oldpath = result;
      return real_size;
    }
    length <<= 1;
    HeapFree(GetProcessHeap(), 0, result);
  }
}


static
const wchar_t* find_last_slash_or_backslash_delimiter(const wchar_t *start) {
  const wchar_t *delimiter = start + lstrlenW(start);
  while (delimiter > start && *delimiter != L'/' && *delimiter != L'\\')
    --delimiter;
  return delimiter;
}

static
wchar_t* find_program_arguments() {
  wchar_t *arguments;

  arguments = GetCommandLineW();
  if (!arguments) return NULL;
  if (arguments[0] == L'\"') {
    arguments++;
    while (arguments[0] && arguments[0] != L'\"') {
      if (arguments[0] == L'\\' &&
          (arguments[1] == L'\"' || arguments[1] == L'\\'))
        arguments++;
      arguments++;
    }
    arguments++;
  }
  while (arguments[0] && arguments[0] != L' ') arguments++;
  if (arguments[0]) while (arguments[1] == ' ') arguments++;
  return arguments;
}

static
wchar_t reduce_wchar(wchar_t c) {
  if (c == L'/')
    return L'\\';
  return (wchar_t)(intptr_t)CharLowerW((LPWSTR)(intptr_t)c);
}

/*
 * Returns whether path ends with redirect
 */
static
int check_path(const wchar_t *path, const wchar_t *redirect) {
  int path_len = lstrlenW(path);
  int redirect_len = lstrlenW(redirect);
  int path_offset;
  int i;
  if (path_len < redirect_len)
    return 0;
  path_offset = path_len - redirect_len;
  for (i = 0; i < redirect_len; i++) {
    if (reduce_wchar(path[i+path_offset]) != reduce_wchar(redirect[i]))
      return 0;
  }
  return 1;
}

static
int is_driver(const wchar_t *option) {
  return lstrcmpW(option, L"-m32") == 0 || lstrcmpW(option, L"-m64") == 0;
}

static
void debug_print(const char* message) {
  HANDLE handle;
  DWORD wrote;
  if (!debug_enabled)
    return;
  handle = GetStdHandle(STD_ERROR_HANDLE);
  WriteFile(handle, message, lstrlen(message), &wrote, NULL);
}

static
void debug_printW(const wchar_t* message) {
  char message_utf8[256];
  if (!debug_enabled)
    return;
  WideCharToMultiByte(CP_UTF8, 0, message, -1, message_utf8,
                      sizeof(message_utf8), NULL, NULL);
  debug_print(message_utf8);
}

static
void println_redirect(const redirect_t *redirect) {
  const wchar_t *str;
  DWORD tmp;
  HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  for (str = redirect->from; *str; ++str)
    WriteFile(output, str, 1, &tmp, NULL);
  WriteFile(output, L"|", 1, &tmp, NULL);
  for (str = redirect->to; *str; ++str)
    WriteFile(output, str, 1, &tmp, NULL);
  WriteFile(output, L"|", 1, &tmp, NULL);
  for (str = redirect->args; *str; ++str)
    WriteFile(output, str, 1, &tmp, NULL);
  WriteFile(output, L"\n", 1, &tmp, NULL);
}

int main() {
  wchar_t *newpath = NULL, *oldpath = NULL;
  const wchar_t *cmdline, *arguments, *selector;
  const char* error_message = "Can not find filename to execute!";
  const char* error_prefix = "redirector: ";
  DWORD length;
  DWORD exitcode;
  int done;
  int redirect_index;
  int length_from;
  int length_to;
  HANDLE output;
  int n_redirects = sizeof(redirects)/sizeof(redirect_t);
  static STARTUPINFOW si = {sizeof(STARTUPINFOW), 0};
  static PROCESS_INFORMATION pi;

  char var[10];
  if (GetEnvironmentVariable("NACL_REDIRECTOR_DEBUG", var, 10)) {
    if (lstrcmp(var, "1") == 0)
      debug_enabled = 1;
  }

  length = get_module_name_safely(&oldpath);
  if (length == 0) {
    error_message = "Unable to determine executable name.";
    goto ShowErrorMessage;
  }
  debug_print("module name: ");
  debug_printW(oldpath);
  debug_print("\n");

  /* If redirector is called as "redirector.exe", dump redirector table.  */
  if (check_path(oldpath, L"redirector.exe")) {
    debug_print("dumping redirect table:\n");
    output = GetStdHandle(STD_OUTPUT_HANDLE);
    for (redirect_index = 0;
         redirect_index < n_redirects;
         redirect_index++) {
      println_redirect(redirects + redirect_index);
    }
    ExitProcess(0);
  }

  /* Lookup executable name in redirect table */
  for (redirect_index = 0;
       redirect_index < n_redirects;
       redirect_index++) {
    if (check_path(oldpath, redirects[redirect_index].from))
      break;
  }
  if (redirect_index >= n_redirects) {
    error_message = "Executable not found in redirect table.";
    goto ShowErrorMessage;
  }

  newpath = HeapAlloc(GetProcessHeap(), 0, sizeof(wchar_t)*(length + 64));
  if (!newpath) {
    error_message = "HeapAlloc failed";
    goto ShowErrorMessage;
  }

  length_from = lstrlenW(redirects[redirect_index].from);
  length_to = lstrlenW(redirects[redirect_index].to);
  lstrcpynW(newpath, oldpath, length - length_from + 1);
  lstrcpyW(newpath + length - length_from, redirects[redirect_index].to);
  HeapFree(GetProcessHeap(), 0, oldpath);
  oldpath = NULL;
  debug_print("target: ");
  debug_printW(newpath);
  debug_print("\n");

  selector = redirects[redirect_index].args;
  debug_print("selector: ");
  debug_printW(selector);
  debug_print("\n");

  /*
   * The gcc (cygwin) redirectors truncate argv0 here to make error messages
   * less confusing.  For nacl-clang it seems that we need to preserve the
   * full argv0.
   * TODO(sbc): Remove this once we are sure the cygwin behaviour is
   * non-essential.
   */
#ifdef CYGWIN_REDIRECTORS
  cmdline = redirects[redirect_index].to;
  if (!is_driver(selector)) {
    cmdline = find_last_slash_or_backslash_delimiter(cmdline);
  } else {
    cmdline = newpath;
  }
#else
  cmdline = newpath;
#endif

  debug_print("cmdline: ");
  debug_printW(cmdline);
  debug_print("\n");

  arguments = find_program_arguments();
  if (!arguments) {
    error_message = "GetCommandLine failed";
    goto ShowErrorMessage;
  }
  length = lstrlenW(cmdline);
  done = lstrlenW(selector);
  oldpath = HeapAlloc(GetProcessHeap(), 0,
    (lstrlenW(arguments) + length + done + 2) * sizeof(wchar_t));
  if (!oldpath) {
    error_message = "HeapAlloc failed";
    goto ShowErrorMessage;
  }
  lstrcpyW(oldpath, cmdline);

  /*
   * cmdline == newpath means it's some kind of gcc driver and we need to
   * handle this case specially: we need to put the -V option to be the first.
   */
  if (is_driver(selector) && arguments[1] == L'-' && arguments[2] == L'V') {
    oldpath[length++] = (arguments++)[0];
    oldpath[length++] = (arguments++)[0];
    oldpath[length++] = (arguments++)[0];
    if (arguments[0] == L' ')
      while (arguments[1] == ' ')
        arguments++;
    if (arguments[0] != 0) {
      oldpath[length++] = (arguments++)[0];
      while (arguments[0] && arguments[0] != L' ')
        oldpath[length++] = (arguments++)[0];
    } else {
      selector = L"";
      done = 0;
    }
  }

  lstrcpyW(oldpath + length, L" ");
  lstrcpyW(oldpath + length + 1, selector);
  lstrcpyW(oldpath + length + done + 1, arguments);
  if (!CreateProcessW(newpath, oldpath, NULL, NULL, TRUE,
                     CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi) != 0) {
    error_message = "CreateProcess failed";
    goto ShowErrorMessage;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  HeapFree(GetProcessHeap(), 0, newpath);
  HeapFree(GetProcessHeap(), 0, oldpath);
  if (!GetExitCodeProcess(pi.hProcess, &exitcode)) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    error_message = "GetExitCodeProcess failed";
    goto ShowErrorMessage;
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  ExitProcess(exitcode);
  return 0;

ShowErrorMessage:
  if (newpath) HeapFree(GetProcessHeap(), 0, newpath);
  if (oldpath) HeapFree(GetProcessHeap(), 0, oldpath);

  output = GetStdHandle(STD_ERROR_HANDLE);
  WriteFile(output, error_prefix, lstrlen(error_prefix), &length, NULL);
  WriteFile(output, error_message, lstrlen(error_message), &length, NULL);
  WriteFile(output, "\r\n", 2, &length, NULL);
  ExitProcess(1);

  return 1;
}
