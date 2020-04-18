/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/include/build_config.h"

#if !NACL_WINDOWS
# include <sys/mman.h>
#endif

#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

static const size_t kNumFileBytes = 2 * 0x10000;


/*
 * mmap PROT_EXEC test
 *
 * Use a file containing mmappable code.  We don't want to
 * re-implement a dynamic loader here, nor do we want to duplicate a
 * lot of ELF parsing.
 */

int prot_exec_test(struct NaClHostDesc *d, void *test_specifics) {
  struct NaClDescEffector *null_eff = NaClDescEffectorTrustedMem();
  uintptr_t addr;
  int (*func)(int param);
  int param;
  int value;

  UNREFERENCED_PARAMETER(test_specifics);

  if ((uintptr_t) -4095 <
      (addr = NaClHostDescMap(d,
                              null_eff,
                              NULL,
                              kNumFileBytes,
                              NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC,
                              NACL_ABI_MAP_SHARED,
                              /* offset */ 0))) {
    fprintf(stderr, "prot_exec_test: map failed, errno %d\n", -(int) addr);
    return 1;
  }

  func = (int (*)(int)) addr;
  for (param = 0; param < 16; ++param) {
    printf("%d -> ", param);
    fflush(stdout);
    value = (*func)(param);
    printf("%d\n", value);
    CHECK(value == param+1);
  }

  NaClHostDescUnmapUnsafe((void *) addr, kNumFileBytes);

  return 0;
}

/*
 * mmap MAP_SHARED test
 *
 * Make sure two views of the same file see the changes made from one
 * view in the other.
 */
int map_shared_test(struct NaClHostDesc *d, void *test_specifics) {
  struct NaClDescEffector *null_eff = NaClDescEffectorTrustedMem();
  uintptr_t view1;
  uintptr_t view2;
  char *v1ptr;
  char *v2ptr;

  UNREFERENCED_PARAMETER(test_specifics);

  if ((uintptr_t) -4095 <
      (view1 = NaClHostDescMap(d,
                               null_eff,
                               NULL,
                               kNumFileBytes,
                               NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                               NACL_ABI_MAP_SHARED,
                               /* offset */ 0))) {
    fprintf(stderr, "map_shared_test: view1 map failed, errno %d\n",
            -(int) view1);
    return 1;
  }

  if ((uintptr_t) -4095 <
      (view2 = NaClHostDescMap(d,
                               null_eff,
                               NULL,
                               kNumFileBytes,
                               NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                               NACL_ABI_MAP_SHARED,
                               /* offset */ 0))) {
    fprintf(stderr, "map_shared_test: view2 map failed, errno %d\n",
            -(int) view2);
    return 1;
  }

  v1ptr = (char *) view1;
  v2ptr = (char *) view2;

  CHECK(v1ptr[0] == '\0');
  CHECK(v2ptr[0] == '\0');
  v1ptr[0] = 'x';
  CHECK(v2ptr[0] == 'x');
  v2ptr[0x400] = 'y';
  CHECK(v1ptr[0x400] == 'y');

  NaClHostDescUnmapUnsafe((void *) view1, kNumFileBytes);
  NaClHostDescUnmapUnsafe((void *) view2, kNumFileBytes);

  return 0;
}

struct MapPrivateSpecifics {
  int shm_not_write;
};

/*
 * mmap MAP_PRIVATE test
 *
 * Make sure that a MAP_PRIVATE view initially sees the changes made
 * in a MAP_SHARED view, but after touching the private view further
 * changes become invisible.
 */
int map_private_test(struct NaClHostDesc *d, void *test_specifics) {
  struct MapPrivateSpecifics *params =
      (struct MapPrivateSpecifics *) test_specifics;
  struct NaClDescEffector *null_eff = NaClDescEffectorTrustedMem();
  uintptr_t view1;
  uintptr_t view2;
  nacl_off64_t off;
  ssize_t bytes_written;
  char *v1ptr;
  char *v2ptr;

  if ((uintptr_t) -4095 <
      (view1 = NaClHostDescMap(d,
                               null_eff,
                               NULL,
                               kNumFileBytes,
                               NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                               NACL_ABI_MAP_SHARED,
                               /* offset */ 0))) {
    fprintf(stderr, "map_private_test: view1 map failed, errno %d\n",
            -(int) view1);
    return 1;
  }

  NaClLog(2, "map_private_test: view1 = 0x%"NACL_PRIxPTR"\n", view1);

  if ((uintptr_t) -4095 <
      (view2 = NaClHostDescMap(d,
                               null_eff,
                               NULL,
                               kNumFileBytes,
                               NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                               NACL_ABI_MAP_PRIVATE,
                               /* offset */ 0))) {
    fprintf(stderr, "map_private_test: view2 map failed, errno %d\n",
            -(int) view2);
    return 1;
  }

  NaClLog(2, "map_private_test: view2 = 0x%"NACL_PRIxPTR"\n", view2);

  v1ptr = (char *) view1;
  v2ptr = (char *) view2;

  CHECK(v1ptr[0] == '\0');
  CHECK(v2ptr[0] == '\0');
  if (params->shm_not_write) {
    NaClLog(2, "map_private_test: changing via shm view\n");
    v1ptr[0] = 'x';  /* write through shared view */
  } else {
    NaClLog(2, "map_private_test: changing via write interface\n");
    off = NaClHostDescSeek(d, 0, 0);
    if (off < 0) {
      fprintf(stderr, "Could not seek: NaCl errno %d\n", (int) -off);
      return 1;
    }
    bytes_written = NaClHostDescWrite(d, "x", 1);
    if (1 != bytes_written) {
      fprintf(stderr, "Could not write: NaCl errno %d\n", (int) -bytes_written);
      return 1;
    }
  }
#if NACL_LINUX || NACL_WINDOWS
  /*
   * Most OSes have this behavior: a PRIVATE mapping is copy-on-write,
   * but the COW occurs when the fault occurs on that mapping, not
   * other mappings; otherwise, the page tables just point the system
   * to the buffer cache (or, if evicted, a stub entry that permits
   * faulting in the page).  So, a write through a writable file
   * descriptor or a SHARED mapping would modify the buffer cache, and
   * the PRIVATE mapping would see such changes until a fault occurs.
   */
  CHECK(v2ptr[0] == 'x');  /* visible! */
#elif NACL_OSX
  /*
   * On OSX, however, the underlying Mach primitives provide
   * bidirectional COW.
   */
  CHECK(v2ptr[0] == '\0');  /* NOT visible! */
#else
# error "Unsupported OS"
#endif

  v2ptr[0] = 'z';  /* COW fault */
  v1ptr[0] = 'y';
  CHECK(v2ptr[0] == 'z'); /* private! */

  CHECK(v1ptr[0x400] == '\0');
  v2ptr[0x400] = 'y';
  CHECK(v1ptr[0x400] == '\0');

  NaClHostDescUnmapUnsafe((void *) view1, kNumFileBytes);
  NaClHostDescUnmapUnsafe((void *) view2, kNumFileBytes);

  return 0;
}

/*
 * Write out kNumFileBytes (a multiple of 64K) bytes of data.
 */
int CreateTestData(struct NaClHostDesc *d) {
  size_t nbytes;
  size_t written = 0;
  ssize_t result;
  static char buffer[4096];

  memset(buffer, 0, sizeof buffer);
  while (written < kNumFileBytes) {
    nbytes = kNumFileBytes - written;
    if (nbytes > sizeof buffer) {
      nbytes = sizeof buffer;
    }
    result = NaClHostDescWrite(d, buffer, nbytes);
    if (result < 0) {
      return (int) result;
    }
    written += result;
  }
  return 0;
}

struct TestParams {
  char const *test_name;
  int (*test_func)(struct NaClHostDesc *, void *test_specifics);
  int open_flags;
  int file_perms;

  unsigned char const *test_data_start;
  size_t test_data_size;

  void *test_specifics;
};

void CreateTestFile(struct NaClHostDesc *d_out,
                    char const *pathname,
                    struct TestParams *param) {
  struct NaClHostDesc hd;
  int err;
  nacl_off64_t off;
  size_t desired_write;
  ssize_t bytes_written;

  printf("pathname = %s, perms 0%o\n", pathname, param->file_perms);
  if (0 != (err = NaClHostDescOpen(&hd,
                                   pathname,
                                   NACL_ABI_O_WRONLY |
                                   NACL_ABI_O_CREAT |
                                   NACL_ABI_O_TRUNC,
                                   param->file_perms))) {
    fprintf(stderr, "Could not open test scratch file: NaCl errno %d\n", -err);
    exit(1);
  }
  if (0 != (err = CreateTestData(&hd))) {
    fprintf(stderr,
            "Could not write test data into test scratch file: NaCl errno %d\n",
            -err);
    exit(1);
  }
  if (NULL != param->test_data_start) {
    off = NaClHostDescSeek(&hd, 0, 0);
    if (off < 0) {
      fprintf(stderr,
              "Could not seek to create test data: NaCl errno %d\n",
              (int) -off);
      exit(1);
    }
    desired_write = param->test_data_size;
    bytes_written = NaClHostDescWrite(&hd,
                                      param->test_data_start,
                                      desired_write);
    if (bytes_written < 0) {
      fprintf(stderr,
              "Could not write specialized test data: NaCl errno %d\n",
              (int) -bytes_written);
      exit(1);
    }
    if ((size_t) bytes_written != desired_write) {
      fprintf(stderr,
              "Error while writing specialized test data:"
              " tried to write %d, actual %d\n",
              (int) desired_write, (int) bytes_written);
      exit(1);
    }
  }
  if (0 != (err = NaClHostDescClose(&hd))) {
    fprintf(stderr,
            "Error while closing test data file, errno %d\n", -err);
    exit(1);
  }
  if (0 != (err = NaClHostDescOpen(d_out,
                                   pathname,
                                   param->open_flags,
                                   param->file_perms))) {
    fprintf(stderr, "Could not open test scratch file: NaCl errno %d\n", -err);
    exit(1);
  }
}

void CloseTestFile(struct NaClHostDesc *d) {
  CHECK(0 == NaClHostDescClose(d));
}

struct MapPrivateSpecifics test0 = { 0 };
struct MapPrivateSpecifics test1 = { 1 };

/*
 * This is the function the machine instructions for which is below.
 * Instead of making assumptions about taking addresses of functions,
 * we open-code the machine instructions.  Since porting this test now
 * requires a little more effort, here is the procedure: Put in
 * "garbage" data in the test_machine_code for your architecture.
 * Build the Test.  The test will fail -- don't bother to run it.
 * However, you should now be able to use a debugger to disassemble
 * the x_plus_1 function, and update the test_machine_code contents.
 */
int x_plus_1(int x) {
  return x+1;
}

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
# if NACL_BUILD_SUBARCH == 32
unsigned char const test_machine_code[] = {
  0x8b, 0x44, 0x24, 0x04, 0x83, 0xc0, 0x01, 0xc3,
};
# elif NACL_BUILD_SUBARCH == 64
#  if NACL_WINDOWS
unsigned char const test_machine_code[] = {
  0x8d, 0x41, 0x01, 0xc3,  /* lea 0x1(%rcx), %eax; ret */
};
#  else
unsigned char const test_machine_code[] = {
  0x8d, 0x47, 0x01, 0xc3,  /* lea 0x1(%rdi), %eax; ret */
};
#  endif
# else
#  error "Who leaked the secret x86-128 project?"
# endif
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
unsigned char const test_machine_code[] = {
  0x01, 0x00, 0x80, 0xe2, 0x1e, 0xff, 0x2f, 0xe1
};
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
unsigned char const test_machine_code[] = {
  0x01, 0x00, 0x82, 0x20,  /* addi v0, a0, 1 */
  0x08, 0x00, 0xe0, 0x03,  /* jr ra */
  0x00, 0x00, 0x00, 0x00,  /* nop */
};
#else
# error "What architecture?"
#endif

struct TestParams tests[] = {
  {
    "Shared Mapping Test",
    map_shared_test,
    (NACL_ABI_O_RDWR | NACL_ABI_O_CREAT), 0666,
    NULL, 0,
    NULL,
  }, {
    "Private Mapping Test, modify by write",
    map_private_test,
    (NACL_ABI_O_RDWR | NACL_ABI_O_CREAT), 0666,
    NULL, 0,
    &test0,
  }, {
    "Private Mapping Test, modify by shm",
    map_private_test,
    (NACL_ABI_O_RDWR | NACL_ABI_O_CREAT), 0666,
    NULL, 0,
    &test1,
  }, {
    "PROT_EXEC Mapping Test",
    prot_exec_test,
    (NACL_ABI_O_RDWR | NACL_ABI_O_CREAT), 0777,
    test_machine_code,
    sizeof test_machine_code,
    NULL,
  },
};

/*
 * It is the responsibility of the invoking environment to delete the
 * file passed as command-line argument.  See the build.scons file.
 */
int main(int ac, char **av) {
  char const *test_dir_name = "/tmp/nacl_host_desc_test";
  struct NaClHostDesc hd;
  size_t err_count;
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
                "Usage: nacl_host_desc_mmap_test [-c run_count]\n"
                "                                [-t test_temp_dir]\n");
        exit(1);
    }
  }

  NaClPlatformInit();

  err_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
      SNPRINTF(test_file_name, sizeof test_file_name,
               "%s/f%d.%"NACL_PRIuS, test_dir_name, test_run, ix);
      printf("%s\n", tests[ix].test_name);
      CreateTestFile(&hd, test_file_name, &tests[ix]);
      err_count += (*tests[ix].test_func)(&hd, tests[ix].test_specifics);
      CloseTestFile(&hd);
    }
  }

  NaClPlatformFini();

  /* we ignore the 2^32 or 2^64 total errors case */
  return (err_count > 255) ? 255 : err_count;
}
