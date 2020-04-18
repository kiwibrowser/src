/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Exercise NaClDescIoDescFromHandleFactory and make sure it works.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "native_client/src/include/build_config.h"

#if NACL_WINDOWS
# include <io.h>
# include <windows.h>
#endif


#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

#define TEST_FILE_BYTES (1<<16)
#define TEST_IO_BYTES 10

#if !defined(NACL_POSIX)
# define NACL_POSIX  (NACL_LINUX || NACL_OSX)
#endif

struct TestParams {
  char const *test_info;
  int flags;
  int expected_read_error;
  int expected_write_error;
  int expected_mmap_error;
  int mmap_prot;
  int mmap_flags;
};

static char test_data[4096];

static void InitializeTestData(void) {
  size_t ix;
  for (ix = 0; ix < sizeof test_data; ++ix) {
    test_data[ix] = (char) ix;
  }
}

void WriteTestData(NaClHandle hFile, size_t file_size) {
  size_t to_write;
  size_t write_request;
#if NACL_WINDOWS
  DWORD written;
#elif NACL_POSIX
  ssize_t written;
#else
# error "What platform?"
#endif

  for (to_write = file_size; to_write > 0; to_write -= written) {
    write_request = sizeof test_data;
    if (write_request > to_write) {
      write_request = to_write;
    }
#if NACL_WINDOWS
    written = 0;
    CHECK(WriteFile(hFile, test_data, (DWORD) write_request, &written, NULL));
#elif NACL_POSIX
    written = write(hFile, test_data, write_request);
    CHECK(0 < written);
#else
# error "What platform?"
#endif
    CHECK((size_t) written <= write_request);
  }
}

int VerifyTestData(char const *data, size_t skip, size_t file_size) {
  size_t to_check;
  size_t check_size;
  size_t data_offset = 0;
  size_t ix;
  size_t start_ix;

  for (to_check = file_size; to_check > 0; to_check -= check_size) {
    check_size = sizeof test_data;
    if (check_size > to_check) {
      check_size = to_check;
    }
    if (data_offset < skip) {
      start_ix = skip;
    } else {
      start_ix = 0;
    }
    for (ix = start_ix; ix < check_size; ++ix) {
      if (data[data_offset + ix] != test_data[ix]) {
        NaClLog(2, "VerifyTestData: differed at byte %"NACL_PRIuS"\n",
                data_offset + ix);
        return 0;
      }
    }
    data_offset += check_size;
  }
  return 1;
}

NaClHandle MakeTestFile(char const *filename, int flags) {
#if NACL_WINDOWS
  HANDLE h;
  DWORD desired_access;

  switch (flags) {
    case NACL_ABI_O_RDONLY:
      desired_access = GENERIC_READ;
      break;
    case NACL_ABI_O_WRONLY:
      desired_access = GENERIC_WRITE;
      break;
    case NACL_ABI_O_RDWR:
      desired_access = GENERIC_READ | GENERIC_WRITE;
      break;
    default:
      CHECK(0);
  }
  h = CreateFileA(filename,
                  GENERIC_WRITE,
                  FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL,
                  CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL);
  CHECK((HANDLE) INVALID_HANDLE_VALUE == (HANDLE) -1);
  if ((HANDLE) INVALID_HANDLE_VALUE == h) {
    DWORD err = GetLastError();
    NaClLog(2, "MakeTestFile for %s failed, error %d\n", filename, err);
    return INVALID_HANDLE_VALUE;
  }
  WriteTestData(h, TEST_FILE_BYTES);
  if (!NaClClose(h)) {
    DWORD err = GetLastError();
    NaClLog(LOG_WARNING, "NaClClose for %s failed, error %d\n",
            filename, err);
  }
  h = CreateFileA(filename,
                  desired_access,
                  FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL,
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL);
  /* h might be INVALID_HANDLE_VALUE */
  return h;
#elif NACL_POSIX
  int fd;

  fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  CHECK(-1 != fd);
  WriteTestData(fd, TEST_FILE_BYTES);
  CHECK(-1 != close(fd));
  switch (flags) {
    case NACL_ABI_O_RDONLY:
      flags = O_RDONLY;
      break;
    case NACL_ABI_O_WRONLY:
      flags = O_WRONLY;
      break;
    case NACL_ABI_O_RDWR:
      flags = O_RDWR;
      break;
    default:
      CHECK(0);
  }
  fd = open(filename, flags);
  return fd;
#else
# error "What platform?"
#endif
}

/* bool -- 1 if test passed, 0 if test failed */
int TestIsOkayP(char const *filename, struct TestParams const *params) {
  NaClHandle h;
  struct NaClDesc *iod;
  ssize_t io_sys_ret;
  char buffer[TEST_IO_BYTES];
  uintptr_t addr;

  h = MakeTestFile(filename, params->flags);
  if ((NaClHandle) -1 == h) {
    NaClLog(LOG_ERROR, "Could not make test file %s\n", filename);
    return 0;  /* failed */
  }
  iod = NaClDescIoMakeFromHandle(h, params->flags);
  if (NULL == iod) {
    NaClLog(LOG_ERROR, "Could not create iod\n");
    return 0;
  }
  /* try to read */
  if (-1 == ((*NACL_VTBL(NaClDesc, iod)->Seek)(iod, 0, 0))) {
    NaClLog(LOG_ERROR, "Could not Seek for read\n");
    return 0;
  }
  memset(buffer, 0, sizeof buffer);
  io_sys_ret = (*NACL_VTBL(NaClDesc, iod)->Read)((struct NaClDesc *) iod,
                                                 buffer,
                                                 sizeof buffer);
  if (0 == params->expected_read_error) {
    if (sizeof buffer != io_sys_ret) {
      NaClLog(LOG_ERROR, "Unexpected short read, got %"NACL_PRIdS" bytes\n",
              io_sys_ret);
      return 0;
    }
    CHECK(0 == memcmp(buffer, test_data, sizeof buffer));
  } else {
    if (params->expected_read_error != io_sys_ret) {
      NaClLog(LOG_ERROR,
              "Successful read when expected failure,"
              " expected errno %d, got %d\n",
              -params->expected_read_error,
              -(int) io_sys_ret);
      return 0;
    }
  }

  /* try to write */
  if (-1 == ((*NACL_VTBL(NaClDesc, iod)->
              Seek)((struct NaClDesc *) iod, 0, 0))) {
    NaClLog(LOG_ERROR, "Could not Seek for write\n");
    return 0;
  }
  memset(buffer, 0, sizeof buffer);
  io_sys_ret = (*NACL_VTBL(NaClDesc, iod)->Write)(iod, buffer, sizeof buffer);
  if (0 == params->expected_write_error) {
    if (sizeof buffer != io_sys_ret) {
      NaClLog(LOG_ERROR, "Unexpected write failure\n");
      return 0;
    }
  } else {
    if (params->expected_write_error != io_sys_ret) {
      NaClLog(LOG_ERROR,
              "Unexpected write error, expected errno %d, got %d\n",
              -params->expected_write_error, -(int) io_sys_ret);
      return 0;
    }
  }

  /* try to mmap */
  addr = ((*NACL_VTBL(NaClDesc, iod)->
           Map)(iod, NaClDescEffectorTrustedMem(),
                (void *) NULL, TEST_FILE_BYTES, params->mmap_prot,
                params->mmap_flags, 0));
  if (0 == params->expected_mmap_error) {
    if ((uintptr_t) -4095 < addr) {
      NaClLog(LOG_ERROR, "Unexpected map failure, error %d\n",
              -(int) addr);
      return 0;
    }
    if (0 == params->expected_write_error) {
      if (0 != memcmp((char const *) addr, buffer, sizeof buffer)) {
        NaClLog(LOG_ERROR, "map contents (prefix) bogus\n");
        return 0;
      }
      if (!VerifyTestData((char const *) addr, sizeof buffer,
                          TEST_FILE_BYTES)) {
        NaClLog(LOG_ERROR, "map contents (remainder) bogus\n");
        return 0;
      }
    } else {
      if (!VerifyTestData((char const *) addr, 0, TEST_FILE_BYTES)) {
        NaClLog(LOG_ERROR, "map contents bogus\n");
        return 0;
      }
    }
    NaClHostDescUnmapUnsafe((void *) addr, TEST_FILE_BYTES);
  } else {
    if ((uintptr_t) -4095 < addr) {
      if (params->expected_mmap_error != (int) addr) {
        NaClLog(LOG_ERROR, "Unexpected map error, expected errno %d, got %d\n",
                -params->expected_mmap_error, -(int) addr);
        return 0;
      }
    } else {
      NaClLog(LOG_ERROR, "Unexpected map success, addr %"NACL_PRIxPTR"\n",
              addr);
      return 0;
    }
  }
  NaClDescUnref(iod);

  return 1;
}

static struct TestParams const tests[] = {
  {
    "O_RDONLY, PROT_READ, MAP_SHARED",
    NACL_ABI_O_RDONLY,
    0, -NACL_ABI_EBADF, 0,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_SHARED
  }, {
    "O_RDONLY, PROT_READ|PROT_WRITE, MAP_SHARED",
    NACL_ABI_O_RDONLY,
    0, -NACL_ABI_EBADF, -NACL_ABI_EACCES,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED
  }, {
    "O_WRONLY, PROT_READ, MAP_SHARED",
    NACL_ABI_O_WRONLY,
    -NACL_ABI_EBADF, 0, -NACL_ABI_EACCES,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_SHARED
  }, {
    "O_WRONLY, PROT_READ|PROT_WRITE, MAP_SHARED",
    NACL_ABI_O_WRONLY,
    -NACL_ABI_EBADF, 0, -NACL_ABI_EACCES,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED
  }, {
    "O_RDWR, PROT_READ, MAP_SHARED",
    NACL_ABI_O_RDWR,
    0, 0, 0,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_SHARED
  }, {
    "O_RDWR, PROT_READ|PROT_WRITE, MAP_SHARED",
    NACL_ABI_O_RDWR,
    0, 0, 0,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_SHARED
  }, {
    "O_RDONLY, PROT_READ, MAP_PRIVATE",
    NACL_ABI_O_RDONLY,
    0, -NACL_ABI_EBADF, 0,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE
  }, {
    "O_RDONLY, PROT_READ|PROT_WRITE, MAP_PRIVATE",
    NACL_ABI_O_RDONLY,
    0, -NACL_ABI_EBADF, 0,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE
  }, {
    "O_WRONLY, PROT_READ, MAP_PRIVATE",
    NACL_ABI_O_WRONLY,
    -NACL_ABI_EBADF, 0, -NACL_ABI_EACCES,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE
  }, {
    "O_WRONLY, PROT_READ|PROT_WRITE, MAP_PRIVATE",
    NACL_ABI_O_WRONLY,
    -NACL_ABI_EBADF, 0, -NACL_ABI_EACCES,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE
  }, {
    "O_RDWR, PROT_READ, MAP_PRIVATE",
    NACL_ABI_O_RDWR,
    0, 0, 0,
    NACL_ABI_PROT_READ,
    NACL_ABI_MAP_PRIVATE
  }, {
    "O_RDWR, PROT_READ|PROT_WRITE, MAP_PRIVATE",
    NACL_ABI_O_RDWR,
    0, 0, 0,
    NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
    NACL_ABI_MAP_PRIVATE
  }
};

/*
 * It is the responsibility of the invoking environment to delete the
 * directory passed as command-line argument.  See the build.scons file.
 */
int main(int ac, char **av) {
  char const *test_dir_name = "/tmp/nacl_desc_io_factory_test";
  int test_passed;
  size_t error_count;
  size_t ix;
  int opt;
  int num_runs = 1;
  int test_run;

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
  InitializeTestData();

  error_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
#if NACL_WINDOWS
      _snprintf_s(test_file_name, sizeof test_file_name, _TRUNCATE,
                  "%s/f%d.%"NACL_PRIuS, test_dir_name, test_run, ix);
#elif NACL_POSIX
      snprintf(test_file_name, sizeof test_file_name,
                  "%s/f%d.%"NACL_PRIuS, test_dir_name, test_run, ix);
#endif
      printf("%s\n", tests[ix].test_info);

      test_passed = TestIsOkayP(test_file_name, &tests[ix]);
      error_count += !test_passed;
      printf("%s\n", test_passed ? "PASSED" : "FAILED");
    }
  }

  printf("Total of %"NACL_PRIuS" error%s.\n",
         error_count, (error_count == 1) ? "" : "s");

  NaClPlatformFini();

  /* we ignore the 2^32 or 2^64 total errors case */
  return (error_count > 255) ? 255 : error_count;
}
