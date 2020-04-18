/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"

/* TODO(mseaborn): This should really be in an IMC header file. */
static const int kKnownInvalidDescNumber = -1;

void checked_close(int fd) {
  int rc = close(fd);
  assert(rc == 0);
}

struct accept_thread_args {
  int boundsock_fd;
  int result_fd;
};

void *accept_thread(void *arg) {
  struct accept_thread_args *args = arg;
  args->result_fd = imc_accept(args->boundsock_fd);
  return NULL;
}

void connect_and_accept(int boundsock_fd, int sockaddr_fd,
                        int *result_fd1, int *result_fd2) {
  struct accept_thread_args args = { -1, -1 };
  pthread_t tid;

  /* TODO(mseaborn): Use of a thread here is necessary on Windows, but
     not on Linux or Mac OS X.  See
     http://code.google.com/p/nativeclient/issues/detail?id=692 */
  args.boundsock_fd = boundsock_fd;
  pthread_create(&tid, NULL, accept_thread, &args);

  *result_fd1 = imc_connect(sockaddr_fd);
  assert(*result_fd1 >= 0);

  pthread_join(tid, NULL);
  *result_fd2 = args.result_fd;
  assert(*result_fd2 >= 0);
}

/* Making a pair of ConnectedSockets is relatively convoluted, since
   we have to create them via a BoundSocket/SocketAddress pair. */
void make_socket_pair(int pair[2]) {
  int bound_pair[2];
  int rc;

  rc = imc_makeboundsock(bound_pair);
  assert(rc == 0);

  connect_and_accept(bound_pair[0], bound_pair[1], &pair[0], &pair[1]);
  checked_close(bound_pair[0]);
  checked_close(bound_pair[1]);
}

int send_message(int sock_fd, char *data, size_t data_size,
                 int *fds, int fds_size) {
  struct NaClAbiNaClImcMsgIoVec iov;
  struct NaClAbiNaClImcMsgHdr msg;
  iov.base = data;
  iov.length = data_size;
  msg.iov = &iov;
  msg.iov_length = 1;
  msg.descv = fds;
  msg.desc_length = fds_size;
  return imc_sendmsg(sock_fd, &msg, 0);
}

int receive_message(int sock_fd, char *data, size_t data_size,
                    int *fds, int fds_size, int *fds_got) {
  struct NaClAbiNaClImcMsgIoVec iov;
  struct NaClAbiNaClImcMsgHdr msg;
  int received;
  iov.base = data;
  iov.length = data_size;
  msg.iov = &iov;
  msg.iov_length = 1;
  msg.descv = fds;
  msg.desc_length = fds_size;
  received = imc_recvmsg(sock_fd, &msg, 0);
  if (fds_got != NULL)
    *fds_got = msg.desc_length;
  return received;
}

char *test_message = "Peter Piper picked a peck of pickled peppers";

/* Check that we can send and receive a data-only message between a
   pair of sockets. */
void check_send_and_receive_data(int sock_fd1, int sock_fd2) {
  int rc;
  char buf[100];
  int fds_got;

  rc = send_message(sock_fd1, test_message, strlen(test_message), NULL, 0);
  assert(rc == strlen(test_message));
  rc = receive_message(sock_fd2, buf, sizeof(buf), NULL, 0, &fds_got);
  assert(rc == strlen(test_message));
  assert(memcmp(buf, test_message, strlen(test_message)) == 0);
  assert(fds_got == 0);
}

void test_socket_transfer(void) {
  int sock_pair[2];
  int bound_pair[2];
  int rc;
  int fd1;
  int fd2;
  char buf[100];
  int received_fds[8];
  int received_fd_count;
  int received_bytes;

  make_socket_pair(sock_pair);

  printf("Test data-only messages...\n");
  check_send_and_receive_data(sock_pair[0], sock_pair[1]);
  check_send_and_receive_data(sock_pair[1], sock_pair[0]);

  rc = imc_makeboundsock(bound_pair);
  assert(rc == 0);

  printf("Test sending a BoundSocket (this is rejected)...\n");
  rc = send_message(sock_pair[0], test_message, strlen(test_message),
                    &bound_pair[0], 1);
  assert(rc == -1);
  assert(errno == EIO);

  printf("Test sending a ConnectedSocket (this is rejected)...\n");
  send_message(sock_pair[0], test_message, strlen(test_message),
               &sock_pair[0], 1);
  assert(rc == -1);
  assert(errno == EIO);

  printf("Test sending a SocketAddress...\n");
  rc = send_message(sock_pair[0], test_message, strlen(test_message),
                    &bound_pair[1], 1);
  assert(rc == strlen(test_message));

  printf("Check that we can receive the SocketAddress we sent...\n");
  received_bytes = receive_message(sock_pair[1], buf, sizeof(buf),
                                   received_fds, 8, &received_fd_count);
  assert(received_bytes == strlen(test_message));
  assert(memcmp(buf, test_message, strlen(test_message)) == 0);
  assert(received_fd_count == 1);

  printf("Check that the received SocketAddress works...\n");
  connect_and_accept(bound_pair[0], received_fds[0], &fd1, &fd2);
  checked_close(received_fds[0]);
  check_send_and_receive_data(fd1, fd2);
  check_send_and_receive_data(fd2, fd1);
  checked_close(fd1);
  checked_close(fd2);

  checked_close(sock_pair[0]);
  checked_close(sock_pair[1]);
  checked_close(bound_pair[0]);
  checked_close(bound_pair[1]);
}

void test_imc_accept_end_of_stream(void) {
  int bound_pair[2];
  int rc;
  rc = imc_makeboundsock(bound_pair);
  assert(rc == 0);

  printf("Test imc_accept() when SocketAddress has been dropped...\n");
  checked_close(bound_pair[1]);

  rc = imc_accept(bound_pair[0]);
  assert(rc == -1);
  assert(errno == EIO);
  checked_close(bound_pair[0]);
}

void test_imc_connect_with_no_acceptor(void) {
  int bound_pair[2];
  int rc;
  rc = imc_makeboundsock(bound_pair);
  assert(rc == 0);

  printf("Test imc_connect() when BoundSocket has been dropped...\n");
  checked_close(bound_pair[0]);

  rc = imc_connect(bound_pair[1]);
  assert(rc == -1);
  assert(errno == EIO);
  checked_close(bound_pair[1]);
}

void test_special_invalid_fd(void) {
  int sock_pair[2];
  int sent;
  int received;
  int fd_to_send;
  char data_buf[10];
  int fds_buf[NACL_ABI_IMC_DESC_MAX];
  int fds_got;

  printf("Test sending and receiving the special 'invalid' descriptor...\n");
  make_socket_pair(sock_pair);

  fd_to_send = kKnownInvalidDescNumber;
  sent = send_message(sock_pair[0], "", 0, &fd_to_send, 1);
  assert(sent == 0);
  received = receive_message(sock_pair[1], data_buf, sizeof(data_buf),
                             fds_buf, NACL_ABI_IMC_DESC_MAX, &fds_got);
  assert(received == 0);
  assert(fds_got == 1);
  assert(fds_buf[0] == kKnownInvalidDescNumber);

  /* Other invalid FD numbers are not accepted. */
  fd_to_send = 1234;
  sent = send_message(sock_pair[0], "", 0, &fd_to_send, 1);
  assert(sent == -1);
  assert(errno == EBADF);

  /* We don't accept other negative numbers. */
  fd_to_send = -2;
  sent = send_message(sock_pair[0], "", 0, &fd_to_send, 1);
  assert(sent == -1);
  assert(errno == EBADF);

  checked_close(sock_pair[0]);
  checked_close(sock_pair[1]);
}

void test_sending_and_receiving_max_fd_count(void) {
  int sock_pair[2];
  int sent;
  int received;
  char data_buf[10];
  int fds_send_buf[NACL_ABI_IMC_DESC_MAX + 1];
  int fds_receive_buf[NACL_ABI_IMC_DESC_MAX];
  int fds_got;
  int i;

  printf("Test sending and receiving many FDs...\n");
  make_socket_pair(sock_pair);

  /* Test exactly the maximum. */
  for (i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
    fds_send_buf[i] = kKnownInvalidDescNumber;
  }
  sent = send_message(sock_pair[0], "", 0, fds_send_buf, NACL_ABI_IMC_DESC_MAX);
  assert(sent == 0);
  received = receive_message(sock_pair[1], data_buf, sizeof(data_buf),
                             fds_receive_buf, NACL_ABI_IMC_DESC_MAX, &fds_got);
  assert(received == 0);
  assert(fds_got == NACL_ABI_IMC_DESC_MAX);
  for (i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
    assert(fds_receive_buf[i] == kKnownInvalidDescNumber);
  }

  /* Test above the maximum. */
  for (i = 0; i < NACL_ABI_IMC_DESC_MAX + 1; i++) {
    fds_send_buf[i] = kKnownInvalidDescNumber;
  }
  sent = send_message(sock_pair[0], "", 0, fds_send_buf,
                      NACL_ABI_IMC_DESC_MAX + 1);
  assert(sent == -1);
  assert(errno == EINVAL);

  checked_close(sock_pair[0]);
  checked_close(sock_pair[1]);
}

int main(int argc, char **argv) {
  /* TODO(mseaborn): It would be better to have a way to pass
     environment variables through sel_ldr into the NaCl process. */
  int i;
  for(i = 1; i < argc; i++) {
    putenv(argv[i]);
  }

  /* Turn off stdout buffering to aid debugging in case of a crash. */
  setvbuf(stdout, NULL, _IONBF, 0);

  test_socket_transfer();

  if (getenv("DISABLE_IMC_ACCEPT_EOF_TEST") == NULL) {
    test_imc_accept_end_of_stream();
  }

  test_imc_connect_with_no_acceptor();

  test_special_invalid_fd();

  test_sending_and_receiving_max_fd_count();

  return 0;
}
