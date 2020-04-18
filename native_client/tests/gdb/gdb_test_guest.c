/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alloca.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"

int global_var;
volatile void *global_ptr;

void test_two_line_function(int arg) {
  global_var = arg - 1;
  global_var = arg;
}

void test_stepi_after_break(void) {
  /* Something meaningful to step through.  */
  global_var = 0;
}

void set_global_var(int arg) {
  global_var = arg;
}

void leaf_call(int arg) {
  global_var = arg;
}

void nested_calls(int arg) {
  global_var = 1;
  leaf_call(arg + 1);
}

int test_print_symbol(void) {
  int local_var = 3;
  global_var = 2 + local_var * 0; /* Use local variable to prevent warning.  */
  set_global_var(1);
  nested_calls(1);
  return global_var;
}

/* A function with non-trivial prolog. */
void test_step_from_function_start(int arg) {
  int local_var = arg - 1;
  global_var = local_var;
  /*
   * Force using frame pointer for this function by calling alloca.
   * This allows to test skipping %esp modifying instructions when they
   * are located in the middle of the function.
   */
  global_ptr = alloca(arg);
}

/*
 * Three layers of nested functions to break and continue from while confirming
 * that gdb can connect and disconnect in various ways.
 */
void test_disconnect_layer3(void) {
  global_var = 100003;
}

void test_disconnect_layer2(void) {
  global_var = 100002;
  test_disconnect_layer3();
}

void test_disconnect(void) {
  global_var = 100001;
  test_disconnect_layer2();
}

void test_kill(void) {
  /* Something to break on before killing program. */
  global_var = 0;
}

void test_detach(void) {
  /* Something to break on before detaching. */
  global_var = 0;
}

int test_call_from_gdb(int arg) {
  global_var = 2 * arg;
  return 3 * arg;
}

void test_change_variable(int arg) {
  int local_var = 2 * arg;
  global_var += local_var + arg;
}

void mmap_breakpoint(void) {
  global_var = 0;
}

void ret_sequence(void) {
#if defined(__i386__)
  __asm__(
      ".globl ret_start\n"
      ".globl ret_end\n"
      ".align 32\n"
      "ret_start:\n"
      "naclret\n"
      "ret_end:\n");
#elif defined(__x86_64__)
  __asm__(
      ".globl ret_start\n"
      ".globl ret_end\n"
      ".align 32\n"
      "ret_start:\n"
      "naclret\n"
      "ret_end:\n");
#endif
}

typedef void (*func_t)(void);

#if defined(__i386__) || defined(__x86_64__)
extern const char ret_start;
extern const char ret_end;

void fill_file_with_code(FILE *f, int file_size) {
  void *ret = (void *) &ret_start;
  int ret_size = &ret_end - &ret_start;
  char *buf = malloc(file_size);
  /* 0x90 is nop for i386 and x86-64. */
  memset(buf, 0x90, file_size);
  int ret_offset = 32 - ret_size;
  memcpy(buf + ret_offset, ret, ret_size);
  fseek(f, 0, SEEK_SET);
  size_t written = fwrite(buf, file_size, 1, f);
  ASSERT_EQ(written, 1u);
  free(buf);
  fflush(f);
}
#else
/* Should not be called. */
void fill_file_with_code(FILE *f, int file_size) {
  ASSERT(0);
}
#endif

void fill_file_with_data(FILE *f, int file_size) {
  int *buf = malloc(file_size);
  memset(buf, 0, file_size);
  buf[0] = 123;
  if (fwrite(buf, file_size, 1, f) != 1) {
    exit(1);
  }
  free(buf);
  fflush(f);
}

uintptr_t find_map_address_for_code(void) {
  func_t func_pointer = &mmap_breakpoint;
  uintptr_t addr = (uintptr_t) *(void **) &func_pointer;
  /* Find free space by skipping 4Mb ahead. */
  addr += 0x400000;
  /* Only 64Kb-aligned addresses are valid for mapping. */
  addr = (addr >> 16) << 16;
  return addr;
}

const char *mmap_file_name = "gdb_test_mmap";

void test_mmap(void) {
  int rc;
  func_t func_pointer;
  const int file_size = 0x10000;
  FILE *f = fopen(mmap_file_name, "wb+");
  ASSERT_NE(f, NULL);
  int fd = fileno(f);

  /* Check that debugger can read read-only mapped file. */
  fill_file_with_data(f, file_size);
  int *file_mapping = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(file_mapping, MAP_FAILED);
  mmap_breakpoint();
  rc = munmap(file_mapping, file_size);
  ASSERT_EQ(rc, 0);

  /* Check that debugger can set breakpoint to mapped code. */
  /* We use NaCl fault injection to bless all file descriptors. */
  fill_file_with_code(f, file_size);
  uintptr_t addr = find_map_address_for_code();

  /* Map code from readonly file descriptor */
  int fd2 = open(mmap_file_name, O_RDONLY);
  file_mapping = mmap((void *) addr, file_size, PROT_READ | PROT_EXEC,
              MAP_FIXED | MAP_PRIVATE, fd2, 0);
  ASSERT_NE(file_mapping, MAP_FAILED);
  mmap_breakpoint();
  func_pointer = *(func_t *) &file_mapping;
  func_pointer();
  rc = close(fd2);
  ASSERT_EQ(rc, 0);

  /* Map code from readwrite file descriptor */
  addr += file_size;
  file_mapping = mmap((void *) addr, file_size, PROT_READ | PROT_EXEC,
              MAP_FIXED | MAP_PRIVATE, fd, 0);
  ASSERT_NE(file_mapping, MAP_FAILED);
  mmap_breakpoint();
  func_pointer = *(func_t *) &file_mapping;
  func_pointer();
  rc = fclose(f);
  ASSERT_EQ(rc, 0);

  /*
   * We can't delete the file since it is mapped to memory and
   * unmapping code from memory is not allowed.
   */
}

int main(int argc, char **argv) {
  int opt;
  char const *test_command;

  while (-1 != (opt = getopt(argc, argv, "f:"))) {
    switch (opt) {
      case 'f':
        /*
         * The test framework supplies the test file name, and is
         * responsible for deleting the file after the test exits.
         * See nacl.scons.
         */
        mmap_file_name = optarg;
        break;
      default:
        fprintf(stderr, "Usage: gdb_test_guest [-f mmap_file] test-type\n");
        return 1;
    }
  }
  assert(argc - optind >= 1);
  test_command = argv[optind];

  if (strcmp(test_command, "break_inside_function") == 0) {
    test_two_line_function(1);
    return 0;
  }
  if (strcmp(test_command, "stepi_after_break") == 0) {
    test_stepi_after_break();
    return 0;
  }
  if (strcmp(test_command, "print_symbol") == 0) {
    return test_print_symbol();
  }
  if (strcmp(test_command, "stack_trace") == 0) {
    nested_calls(1);
    return 0;
  }
  if (strcmp(test_command, "step_from_func_start") == 0) {
    global_var = 0;
    test_step_from_function_start(2);
    return 0;
  }
  if (strcmp(test_command, "detach") == 0) {
    test_detach();
    return 0;
  }
  if (strcmp(test_command, "disconnect") == 0) {
    global_var = 0;
    test_disconnect();
    return 0;
  }
  if (strcmp(test_command, "kill") == 0) {
    test_kill();
    return 0;
  }
  if (strcmp(test_command, "complete") == 0) {
    return 123;
  }
  if (strcmp(test_command, "call_from_gdb") == 0) {
    /* Call function so that it doesn't get optimized away. */
    return test_call_from_gdb(0);
  }
  if (strcmp(test_command, "change_variable") == 0) {
    global_var = 0;
    test_change_variable(1);
    return 0;
  }
  if (strcmp(test_command, "invalid_memory") == 0) {
    return 0;
  }
  if (strcmp(test_command, "mmap") == 0) {
    test_mmap();
    return 0;
  }
  return 1;
}
