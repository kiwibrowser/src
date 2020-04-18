/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"
#include "native_client/src/include/nacl_macros.h"


#define EXAMPLE_DESC 10
#define BAD_DESC 100


void test_sendmsg_data_only(void) {
  char *data = "test_sending_data_only";
  struct NaClAbiNaClImcMsgIoVec iov = { data, strlen(data) };
  struct NaClAbiNaClImcMsgHdr msg = { &iov, 1, NULL, 0, 0 };
  int rc = imc_sendmsg(EXAMPLE_DESC, &msg, 0);
  assert(rc == 101);
}

void test_sendmsg_with_descs(void) {
  char *data = "test_sending_descs";
  struct NaClAbiNaClImcMsgIoVec iov = { data, strlen(data) };
  int descs[] = { EXAMPLE_DESC, EXAMPLE_DESC };
  struct NaClAbiNaClImcMsgHdr msg = {
    &iov, 1, descs, NACL_ARRAY_SIZE(descs), 0
  };
  int rc = imc_sendmsg(EXAMPLE_DESC, &msg, 0);
  assert(rc == 102);
}

void test_sendmsg_bad_destination_desc(void) {
  char *data = "blah";
  struct NaClAbiNaClImcMsgIoVec iov = { data, strlen(data) };
  struct NaClAbiNaClImcMsgHdr msg = { &iov, 1, NULL, 0, 0 };
  int rc = imc_sendmsg(BAD_DESC, &msg, 0);
  assert(rc == -1);
  assert(errno == EBADF);
}

void test_sendmsg_bad_desc_args(void) {
  char *data = "blah";
  struct NaClAbiNaClImcMsgIoVec iov = { data, strlen(data) };
  /*
   * Including EXAMPLE_DESC in the array before the invalid descriptor
   * number means that we test for refcount leaks on the error path.
   */
  int descs[] = { EXAMPLE_DESC, BAD_DESC, EXAMPLE_DESC };
  struct NaClAbiNaClImcMsgHdr msg = {
    &iov, 1, descs, NACL_ARRAY_SIZE(descs), 0
  };
  int rc = imc_sendmsg(EXAMPLE_DESC, &msg, 0);
  assert(rc == -1);
  assert(errno == EBADF);
}


/*
 * In order to test receiving multiple kinds of message on the same
 * descriptor, we first send a message to say which test message we
 * want to receive.
 */
void send_simple_request(char *data) {
  struct NaClAbiNaClImcMsgIoVec iov = { data, strlen(data) };
  struct NaClAbiNaClImcMsgHdr msg = { &iov, 1, NULL, 0, 0 };
  int rc = imc_sendmsg(EXAMPLE_DESC, &msg, 0);
  assert(rc == 200);
}

void test_recvmsg_data_only(void) {
  send_simple_request("request_receiving_data_only");

  char buf[100];
  struct NaClAbiNaClImcMsgIoVec iov = { buf, sizeof(buf) };
  struct NaClAbiNaClImcMsgHdr msg = { &iov, 1, NULL, 0, 0 };
  int rc = imc_recvmsg(EXAMPLE_DESC, &msg, 0);
  char *expected = "test_receiving_data_only";
  assert(rc == strlen(expected));
  assert(memcmp(buf, expected, strlen(expected)) == 0);
  assert(msg.flags == 0);
  assert(msg.desc_length == 0);
}

void test_recvmsg_with_descs(void) {
  send_simple_request("request_receiving_descs");

  char buf[100];
  struct NaClAbiNaClImcMsgIoVec iov = { buf, sizeof(buf) };
  int descs[8];
  struct NaClAbiNaClImcMsgHdr msg = {
    &iov, 1, descs, NACL_ARRAY_SIZE(descs), 0
  };
  int rc = imc_recvmsg(EXAMPLE_DESC, &msg, 0);
  char *expected = "test_receiving_descs";
  assert(rc == strlen(expected));
  assert(memcmp(buf, expected, strlen(expected)) == 0);
  assert(msg.flags == 0);

  assert(msg.desc_length == 2);
  int index;
  for (index = 0; index < msg.desc_length; index++) {
    rc = close(msg.descv[index]);
    assert(rc == 0);
  }
}

void test_recvmsg_truncated_descs(void) {
  send_simple_request("request_receiving_descs");

  char buf[100];
  struct NaClAbiNaClImcMsgIoVec iov = { buf, sizeof(buf) };
  int descs[1];
  struct NaClAbiNaClImcMsgHdr msg = {
    &iov, 1, descs, NACL_ARRAY_SIZE(descs), 0
  };
  int rc = imc_recvmsg(EXAMPLE_DESC, &msg, 0);
  char *expected = "test_receiving_descs";
  assert(rc == strlen(expected));
  assert(memcmp(buf, expected, strlen(expected)) == 0);
  assert(msg.flags == NACL_ABI_RECVMSG_DESC_TRUNCATED);

  assert(msg.desc_length == 1);
  rc = close(msg.descv[0]);
  assert(rc == 0);
}


void run_test(const char *test_name, void (*test_func)(void)) {
  printf("Running %s...\n", test_name);
  test_func();
}

#define RUN_TEST(test_func) (run_test(#test_func, test_func))

int main(void) {
  RUN_TEST(test_sendmsg_data_only);
  RUN_TEST(test_sendmsg_with_descs);
  RUN_TEST(test_sendmsg_bad_destination_desc);
  RUN_TEST(test_sendmsg_bad_desc_args);

  RUN_TEST(test_recvmsg_data_only);
  RUN_TEST(test_recvmsg_with_descs);
  RUN_TEST(test_recvmsg_truncated_descs);

  /* Close so that we can check for leaks in the test host. */
  printf("Clean up for leak check...\n");
  int rc = close(EXAMPLE_DESC);
  assert(rc == 0);

  return 0;
}
