/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <io.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

/* bool */
int TryToMap(struct NaClHostDesc *hd, size_t map_bytes, int prot, int flags,
             int expected_errno) {
  uintptr_t addr;

  addr = NaClHostDescMap(hd,
                         NaClDescEffectorTrustedMem(),
                         NULL,
                         map_bytes,
                         prot,
                         flags,
                         0);
  if (0 == expected_errno) {
    if ((uintptr_t) -4095 < addr) {
      NaClLog(LOG_ERROR, "NaClHostDescMap returned errno %d\n", -(int) addr);
      return 0;
    }
    NaClHostDescUnmapUnsafe((void *) addr, map_bytes);
    return 1;
  } else {
    if ((uintptr_t) -4095 < addr) {
      if (expected_errno != -(int) addr) {
        NaClLog(LOG_ERROR, "NaClHostDescMap returned errno %d, expected %d\n",
                -(int) addr, expected_errno);
      }
    } else {
      NaClLog(LOG_ERROR, "NaClHostDescMap succeeded, expected errno %d\n",
              expected_errno);
      NaClHostDescUnmapUnsafe((void *) addr, map_bytes);
    }
    return expected_errno == -(int) addr;
  }
}

void WriteTestData(HANDLE hFile, size_t file_size) {
  char buffer[4096];
  size_t to_write;
  size_t write_request;
  DWORD written;

  memset(buffer, 0, sizeof buffer);
  for (to_write = file_size; to_write > 0; to_write -= written) {
    write_request = sizeof buffer;
    if (write_request > to_write) {
      write_request = to_write;
    }
    written = 0;
    CHECK(WriteFile(hFile, buffer, (DWORD) write_request, &written, NULL));
  }
}

struct FileCreationArgs {
  char const *file_type_description;
  DWORD desired_access;
  DWORD share_mode;
  DWORD flags_and_attributes;
  size_t file_size;
};

/* bool */
HANDLE CreateTestFile(char const *path, struct FileCreationArgs *args) {
  HANDLE h;

  NaClLog(1,
          "CreateFile(%s, %d, %d, NULL, OPEN_EXISTING, %d, NULL),"
          " size 0x%"NACL_PRIxS"\n",
          path, args->desired_access, args->share_mode,
          args->flags_and_attributes,
          args->file_size);
  h = CreateFileA(path,
                  GENERIC_WRITE,
                  0,
                  NULL,
                  CREATE_ALWAYS,
                  args->flags_and_attributes,
                  NULL);
  if (INVALID_HANDLE_VALUE == h) {
    DWORD err = GetLastError();
    NaClLog(2, "CreateTestFile for %s failed, error %d\n", path, err);
    return INVALID_HANDLE_VALUE;
  }
  WriteTestData(h, args->file_size);
  if (!CloseHandle(h)) {
    DWORD err = GetLastError();
    NaClLog(LOG_WARNING, "CloseHandle for %s failed, error %d\n", path, err);
  }
  h = CreateFileA(path,
                  args->desired_access,
                  args->share_mode,
                  NULL,
                  OPEN_EXISTING,
                  args->flags_and_attributes,
                  NULL);
  /* h might be INVALID_HANDLE_VALUE */
  return h;
}

void AttemptToDeleteTestFile(char const *path) {
  DWORD attr;

  attr = GetFileAttributesA(path);
  CHECK(INVALID_FILE_ATTRIBUTES != attr);
  if (0 != (attr & FILE_ATTRIBUTE_READONLY)) {
    attr &= ~FILE_ATTRIBUTE_READONLY;
    CHECK(SetFileAttributesA(path, attr));
  }
  if (!DeleteFileA(path)) {
    DWORD err = GetLastError();
    NaClLog(LOG_WARNING, "DeleteFileA failed, error %d\n", err);
  }
}

struct FileCreationArgs g_normal_rwx_file = {
  "Normal file, GENERIC_(READ|WRITE|EXECUTE), FILE_SHARE_ALL, NORMAL",
  GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  FILE_ATTRIBUTE_NORMAL,
  2 << 16,
};

struct FileCreationArgs g_normal_rw_file = {
  "Normal file, GENERIC_(READ|WRITE), FILE_SHARE_ALL, NORMAL",
  GENERIC_READ|GENERIC_WRITE,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  FILE_ATTRIBUTE_NORMAL,
  2 << 16,
};

struct FileCreationArgs g_normal_r_file = {
  "Normal file, GENERIC_READ, FILE_SHARE_ALL, NORMAL",
  GENERIC_READ,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  FILE_ATTRIBUTE_NORMAL,
  2 << 16,
};

struct FileCreationArgs g_readonly_rwx_file = {
  "Readonly file, GENERIC_(READ|WRITE|EXECUTE), FILE_SHARE_ALL, READONLY",
  GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  FILE_ATTRIBUTE_READONLY,
  2 << 16,
};
struct FileCreationArgs g_readonly_rw_file = {
  "Readonly file, GENERIC_(READ|WRITE), FILE_SHARE_ALL, READONLY",
  GENERIC_READ|GENERIC_WRITE,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  FILE_ATTRIBUTE_READONLY,
  2 << 16,
};

struct FileCreationArgs g_readonly_r_file = {
  "Readonly file, GENERIC_READ, FILE_SHARE_ALL, READONLY",
  GENERIC_READ,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  FILE_ATTRIBUTE_READONLY,
  2 << 16,
};

struct TestParam {
  char const *test_info;
  struct FileCreationArgs *file_type;
  size_t map_bytes;
  int prot;
  int map_flags;
  int oflags;
  int posix_flags;
  int expected_open_error;  /* GetLastError */
  int expected_mmap_errno;
} tests[] = {
  {
    "Mapping normal rwx file, PROT_ALL, PRIVATE",
    &g_normal_rwx_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal rw file, PROT_ALL, PRIVATE",
    &g_normal_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ NACL_ABI_EACCES,
  }, {
    "Mapping normal rw file, PROT_(READ|WRITE), PRIVATE",
    &g_normal_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal ro file, PROT_READ, PRIVATE",
    &g_normal_r_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal ro file, PROT_(READ|WRITE), PRIVATE",
    &g_normal_r_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal rwx file, PROT_ALL, SHARED",
    &g_normal_rwx_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC,
    NACL_ABI_MAP_SHARED,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal rw file, PROT_(READ|WRITE), SHARED",
    &g_normal_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal ro file, PROT_READ, SHARED",
    &g_normal_r_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_SHARED,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping normal ro file, PROT_(READ|WRITE), SHARED",
    &g_normal_r_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ NACL_ABI_EACCES,
  }, {
    "Mapping ro rwx file, PROT_ALL, PRIVATE",
    &g_readonly_rwx_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro rw file, PROT_ALL, PRIVATE",
    &g_readonly_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro rw file, PROT_(READ|WRITE), PRIVATE",
    &g_readonly_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro rw file, PROT_READ, PRIVATE",
    &g_readonly_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro ro file, PROT_READ, PRIVATE",
    &g_readonly_r_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro ro file, PROT_(READ|WRITE), PRIVATE",
    &g_readonly_r_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro rwx file, PROT_ALL, SHARED",
    &g_readonly_rwx_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC,
    NACL_ABI_MAP_SHARED,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro rw file, PROT_(READ|WRITE), SHARED",
    &g_readonly_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro ro file, PROT_READ, SHARED",
    &g_readonly_rw_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_SHARED,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_ACCESS_DENIED,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro ro file, PROT_READ, SHARED",
    &g_readonly_r_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_SHARED,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ 0,
  }, {
    "Mapping ro ro file, PROT_(READ|WRITE), SHARED",
    &g_readonly_r_file,
    1 << 16,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED,
    _O_RDONLY | _O_BINARY,
    NACL_ABI_O_RDONLY,
    /* expected_open_error= */ 0,
    /* expected_mmap_errno= */ NACL_ABI_EACCES,
  }, {
    "LIE: Mapping normal r pretend rw file, PROT_READ, PRIVATE",
    &g_normal_r_file,
    1 << 16,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE,
    _O_RDWR | _O_BINARY,
    NACL_ABI_O_RDWR,
    /* expected_open_error= */ ERROR_SUCCESS,
    /* expected_mmap_errno= */ NACL_ABI_EACCES,
  },
};

/*
 * It is the responsibility of the invoking environment to delete the
 * file passed as command-line argument.  See the build.scons file.
 */
int main(int ac, char **av) {
  char const *test_dir_name = "/tmp/nacl_host_desc_mmap_win_test";
  struct NaClHostDesc hd;
  int test_passed;
  size_t error_count;
  size_t ix;
  int opt;
  int num_runs = 1;
  int test_run;
  HANDLE h;
  int d;

  while (EOF != (opt = getopt(ac, av, "c:t:"))) {
    switch (opt) {
      case 'c':
        num_runs = atoi(optarg);
        break;
      case 't':
        test_dir_name = optarg;
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_host_desc_mmap_win_test [-c run_count]\n"
                "                                    [-t test_temp_dir]\n");
        exit(1);
    }
  }

  NaClPlatformInit();

  error_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
      _snprintf_s(test_file_name, sizeof test_file_name, _TRUNCATE,
                  "%s/f%d.%"NACL_PRIuS, test_dir_name, test_run, ix);
      printf("%s\n", tests[ix].test_info);
      printf("-- %s\n", tests[ix].file_type->file_type_description);

      h = CreateTestFile(test_file_name, tests[ix].file_type);
      if (ERROR_SUCCESS == tests[ix].expected_open_error) {
        if (INVALID_HANDLE_VALUE == h) {
          DWORD err = GetLastError();
          NaClLog(LOG_ERROR,
                  "CreateTestFile for %s failed, error %d\n",
                  test_file_name, err);
          printf("FAILED\n");
          ++error_count;
          continue;
        }

        d = _open_osfhandle((intptr_t) h, tests[ix].oflags);
        NaClHostDescPosixTake(&hd, d, tests[ix].posix_flags);
        test_passed = TryToMap(&hd, tests[ix].map_bytes, tests[ix].prot,
                               tests[ix].map_flags,
                               tests[ix].expected_mmap_errno);
        error_count += !test_passed;
        printf("%s\n", test_passed ? "PASSED" : "FAILED");

        CHECK(0 == NaClHostDescClose(&hd));
        AttemptToDeleteTestFile(test_file_name);
      } else {
        if (INVALID_HANDLE_VALUE == h) {
          DWORD err = GetLastError();
          if (err != tests[ix].expected_open_error) {
            NaClLog(LOG_ERROR,
                    "Expected CreateTestFile for %s to failed with"
                    " error %d, but got error %d\n",
                    test_file_name, err, tests[ix].expected_open_error);
            printf("FAILED\n");
            ++error_count;
          } else {
            printf("PASSED (open failed)\n");
          }
          continue;
        } else {
          NaClLog(LOG_ERROR,
                  "Expected CreateTestFile for %s to fail with error %d,"
                  " but succeeded instead!\n",
                  test_file_name,
                  tests[ix].expected_open_error);
          (void) CloseHandle(h);
          AttemptToDeleteTestFile(test_file_name);
          printf("FAILED\n");
          ++error_count;
          continue;
        }
      }
    }
  }

  printf("Total of %d error%s.\n", error_count, (error_count == 1) ? "" : "s");

  NaClPlatformFini();

  /* we ignore the 2^32 or 2^64 total errors case */
  return (error_count > 255) ? 255 : error_count;
}
