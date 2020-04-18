/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/public/linux_syscalls/sched.h"

namespace {

int simple_clone_fn(void *arg) {
  return (int) arg;
}

void test_simple_clone() {
  puts("test_simple_clone");
  const int kStackSize = 4096;
  void *stack = mmap(0, kStackSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, stack);
  int pid = clone(&simple_clone_fn,
                  (void *)((uintptr_t) stack + kStackSize),
                  LINUX_SIGCHLD, NULL);
  ASSERT_GT(pid, 0);  // Success, and in the parent process.
  int status;
  int rc = waitpid(pid, &status, 0);
  ASSERT_EQ(rc, pid);
  ASSERT(WIFEXITED(status));
  ASSERT_EQ(WEXITSTATUS(status), 0);

  // Release stack on parent process.
  rc = munmap(stack, kStackSize);
  ASSERT_EQ(rc, 0);
}

void test_arg_return_value() {
  puts("test_arg_return_value");
  const int kStackSize = 4096;
  void *stack = mmap(0, kStackSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, stack);
  int pid = clone(&simple_clone_fn,
                  (void *) ((uintptr_t) stack + kStackSize),
                  LINUX_SIGCHLD, (void *) 10);
  ASSERT_GT(pid, 0);  // Success, and in the parent process.
  int status;
  int rc = waitpid(pid, &status, 0);
  ASSERT_EQ(rc, pid);
  ASSERT(WIFEXITED(status));
  ASSERT_EQ(WEXITSTATUS(status), 10);

  // Release stack on parent process.
  rc = munmap(stack, kStackSize);
  ASSERT_EQ(rc, 0);
}

void test_null_child_stack() {
  puts("test_null_child_stack");
  errno = 0;
  int pid = clone(&simple_clone_fn, /* child_stack = */ NULL,
                  LINUX_SIGCHLD, NULL);
  ASSERT_EQ(pid, -1);
  ASSERT_EQ(errno, EINVAL);
}

void test_ptid() {
  puts("test_ptid");

  const int kStackSize = 4096;
  void *stack = mmap(0, kStackSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, stack);
  pid_t ptid = 0;
  int pid = clone(&simple_clone_fn,
                  (void *) ((uintptr_t) stack + kStackSize),
                  CLONE_PARENT_SETTID | LINUX_SIGCHLD, NULL, &ptid);
  ASSERT_GT(pid, 0);  // Success, and in the parent process.
  ASSERT_GT(ptid, 0);  // |ptid| is properly set.
  int status;
  int rc = waitpid(pid, &status, 0);
  ASSERT_EQ(rc, pid);
  ASSERT(WIFEXITED(status));
  ASSERT_EQ(WEXITSTATUS(status), 0);

  // Release stack on parent process.
  rc = munmap(stack, kStackSize);
  ASSERT_EQ(rc, 0);
}

int verify_ctid_fn(void *arg) {
  ASSERT_NE(NULL, arg);
  pid_t ctid = *(pid_t *) arg;
  return ctid == 0;  // Return SUCCESS (= 0) if ctid != 0.
}

void test_ctid() {
  puts("test_ctid");

  const int kStackSize = 4096;
  void *stack = mmap(0, kStackSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, stack);
  pid_t *ctid_ptr = (pid_t *) ((uintptr_t) stack + kStackSize / 2);
  int pid = clone(&verify_ctid_fn,
                  (void *) ((uintptr_t) stack + kStackSize),
                  CLONE_CHILD_SETTID | LINUX_SIGCHLD, ctid_ptr, NULL, NULL,
                  ctid_ptr);
  ASSERT_GT(pid, 0);  // Success, and in the parent process.
  int status;
  int rc = waitpid(pid, &status, 0);
  ASSERT_EQ(rc, pid);
  ASSERT(WIFEXITED(status));
  ASSERT_EQ(WEXITSTATUS(status), 0);  // Make sure ctid verification result.

  // Release stack on parent process.
  rc = munmap(stack, kStackSize);
  ASSERT_EQ(rc, 0);
}

}  // namespace

int main(int argc, char *argv[]) {
  test_simple_clone();
  test_arg_return_value();
  test_null_child_stack();
  // Under QEMU, CLONE_PARENT_SETTID does not seem to work.
  if (getenv("UNDER_QEMU_ARM") == NULL) {
    test_ptid();
  }
  test_ctid();
  puts("PASSED");
}
