/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_scoped_ptr.h"
#include "native_client/src/public/linux_syscalls/poll.h"
#include "native_client/src/public/linux_syscalls/sys/socket.h"

namespace {

char kMessage[] = "foo";
size_t kMessageLen = sizeof(kMessage);

class ScopedSocketPair {
 public:
  explicit ScopedSocketPair(int type) {
    int rc = socketpair(AF_UNIX, type, 0, fds_);
    ASSERT_EQ(rc, 0);
  }

  ~ScopedSocketPair() {
    int rc = close(fds_[0]);
    ASSERT_EQ(rc, 0);
    rc = close(fds_[1]);
    ASSERT_EQ(rc, 0);
  }

  int operator[](int i) const {
    ASSERT(i == 0 || i == 1);
    return fds_[i];
  }

 private:
  int fds_[2];

  DISALLOW_COPY_AND_ASSIGN(ScopedSocketPair);
};

void SendPacket(const ScopedSocketPair &fds) {
  struct msghdr sent_msg;
  memset(&sent_msg, 0, sizeof(sent_msg));
  struct iovec sent_iov = { kMessage, kMessageLen };
  sent_msg.msg_iov = &sent_iov;
  sent_msg.msg_iovlen = 1;
  int rc = sendmsg(fds[1], &sent_msg, 0);
  ASSERT_EQ(rc, kMessageLen);
}

char *RecvPacket(const ScopedSocketPair &fds) {
  struct msghdr received_msg;
  memset(&received_msg, 0, sizeof(received_msg));
  nacl::scoped_array<char> buf(new char[kMessageLen]);
  struct iovec received_iov = { buf.get(), kMessageLen };
  received_msg.msg_iov = &received_iov;
  received_msg.msg_iovlen = 1;

  int rc = recvmsg(fds[0], &received_msg, 0);
  ASSERT_EQ(rc, kMessageLen);
  return buf.release();
}

void TestDgramSocketpair(const char *type_str, int type) {
  printf("TestDgramSocketpair (%s)", type_str);
  ScopedSocketPair fds(type);

  SendPacket(fds);
  nacl::scoped_array<char> received(RecvPacket(fds));
  ASSERT_EQ(0, strcmp(received.get(), kMessage));
}

void TestStreamSocketpair() {
  printf("TestStreamSocketpair");
  ScopedSocketPair fds(SOCK_STREAM);

  size_t written_len = write(fds[1], kMessage, kMessageLen);
  ASSERT_EQ(written_len, kMessageLen);

  nacl::scoped_array<char> buf(new char[kMessageLen]);
  size_t read_len = read(fds[0], buf.get(), kMessageLen);
  ASSERT_EQ(read_len, kMessageLen);

  ASSERT_EQ(0, strcmp(buf.get(), kMessage));
}

void PreparePollFd(const ScopedSocketPair &fds, struct pollfd pfd[2]) {
  memset(pfd, 0, sizeof(*pfd) * 2);
  pfd[0].fd = fds[0];
  pfd[0].events = POLLIN;
  pfd[1].fd = fds[1];
  pfd[1].events = POLLOUT;
}

void TestPoll() {
  printf("TestPoll");
  ScopedSocketPair fds(SOCK_DGRAM);

  struct pollfd pfd[2];
  PreparePollFd(fds, pfd);

  int rc = poll(pfd, 2, 0);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(pfd[0].revents, 0);
  ASSERT_EQ(pfd[1].revents, POLLOUT);

  SendPacket(fds);

  PreparePollFd(fds, pfd);

  rc = poll(pfd, 2, 0);
  ASSERT_EQ(rc, 2);
  ASSERT_EQ(pfd[0].revents, POLLIN);
  ASSERT_EQ(pfd[1].revents, POLLOUT);

  nacl::scoped_array<char> buf(RecvPacket(fds));

  rc = shutdown(fds[1], SHUT_RDWR);
  ASSERT_EQ(rc, 0);

  PreparePollFd(fds, pfd);
  rc = poll(pfd, 2, 0);
  ASSERT_EQ(rc, 1);
  ASSERT_EQ(pfd[0].revents, 0);
  ASSERT_EQ(pfd[1].revents, POLLOUT | POLLHUP);
}

void TestCmsg() {
  printf("TestCmsg");
  // We assume here sizeof(int) (= the CMSG's alignment) is 4 bytes.
  ASSERT_EQ(CMSG_ALIGN(1), 4);
  ASSERT_EQ(CMSG_ALIGN(2), 4);
  ASSERT_EQ(CMSG_ALIGN(3), 4);
  ASSERT_EQ(CMSG_ALIGN(4), 4);
  ASSERT_EQ(CMSG_ALIGN(5), 8);

  ASSERT_EQ(sizeof(cmsghdr), 12);

  // CMSG_SPACE considers the alignment for the data size.
  ASSERT_EQ(CMSG_SPACE(1), 16);
  ASSERT_EQ(CMSG_SPACE(2), 16);
  ASSERT_EQ(CMSG_SPACE(3), 16);
  ASSERT_EQ(CMSG_SPACE(4), 16);
  ASSERT_EQ(CMSG_SPACE(5), 20);

  // CMSG_LEN does not include the alignment of the data.
  ASSERT_EQ(CMSG_LEN(1), 13);
  ASSERT_EQ(CMSG_LEN(2), 14);
  ASSERT_EQ(CMSG_LEN(3), 15);
  ASSERT_EQ(CMSG_LEN(4), 16);
  ASSERT_EQ(CMSG_LEN(5), 17);

  // The data should follow the header.
  {
    struct cmsghdr cmsg[2];
    ASSERT_EQ(CMSG_DATA(cmsg), (unsigned char *) (&cmsg[1]));
  }

  // Test for CMSG_FIRSTHDR and CMSG_NXTHDR.
  // Set up msghdr with two CMSG, whose data size are 1 and 2.
  struct msghdr mhdr;
  char msg_control[CMSG_SPACE(1) + CMSG_SPACE(2)];
  {
    struct cmsghdr *cmsg_ptr = (struct cmsghdr *) msg_control;
    cmsg_ptr->cmsg_len = CMSG_LEN(1);
  }
  {
    struct cmsghdr *cmsg_ptr = (struct cmsghdr *) (msg_control + CMSG_SPACE(1));
    cmsg_ptr->cmsg_len = CMSG_LEN(2);
  }
  mhdr.msg_control = msg_control;
  mhdr.msg_controllen = sizeof(msg_control);

  struct cmsghdr *cmsg_ptr = CMSG_FIRSTHDR(&mhdr);
  ASSERT_EQ((uintptr_t) msg_control, (uintptr_t) cmsg_ptr);
  ASSERT_EQ(cmsg_ptr->cmsg_len, CMSG_LEN(1));
  cmsg_ptr = CMSG_NXTHDR(&mhdr, cmsg_ptr);
  ASSERT_EQ((uintptr_t) msg_control + CMSG_SPACE(1), (uintptr_t) cmsg_ptr);
  ASSERT_EQ(cmsg_ptr->cmsg_len, CMSG_LEN(2));
  cmsg_ptr = CMSG_NXTHDR(&mhdr, cmsg_ptr);
  ASSERT_EQ(cmsg_ptr, NULL);
}

}  // namespace

int main(int argc, char *argv[]) {
  TestDgramSocketpair("DGRAM", SOCK_DGRAM);
  TestDgramSocketpair("SEQPACKET", SOCK_SEQPACKET);
  TestStreamSocketpair();
  TestPoll();
  TestCmsg();
  return 0;
}
