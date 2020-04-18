/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

#define MAX_TEST_DESCS 8

char *exe_path = NULL;

struct TestParam {
  uint32_t flags;
  int32_t metadata_type;
  char const *metadata_string;
};

struct TestCase {
  nacl_abi_size_t num_desc;
  struct TestParam descs[MAX_TEST_DESCS];
};

struct TestCase test_case[] = {
  { 1, {
      { 0x1, 1, "" } } },
  { 1, {
      { 0x2, 2,
        "Goofe' Dean. Well, let's see, we have on the bags,"
        " Who's on first, What's on second, I Don't Know is on third..."},
    } },
  { 2, {
      { 0x4, 3,
        "That's what I want to find out."},
      { 0x8, 4,
        "I say Who's on first, What's on second, I Don't Know's on third."},
    } },
  { 3, {
      { 0x10, 5,
        "Are you the manager?"},
      { 0x20, 6,
        "Yes."},
      { 0x40, 7,
        "You gonna be the coach too?"} } },
  { 4, {
      { 0x80, 8,
        "Yes."},
      { 0x100, 9,
        "And you don't know the fellows' names?"},
      { 0x200, 10,
        "Well I should."},
      { 0x400, 11, "Well then who's on first?"},
    } },
  { 8, {
      { 0x800, 12, "Yes." },
      { 0x1000, 13, "I mean the fellow's name." },
      { 0x2000, 14, "Who." },
      { 0x4000, 15, "The guy on first." },

      { 0x8000, 16, "Who." },
      { 0xffff, 17, "The first baseman." },
      { 0xf0f0, 18, "Who." },
      { 0x0f0f, 19, "The guy playing..." },
    } },
  { 7, {
      { 0x5555, 20, "Who is on first!" },
      { 0xaaaa, 21, "I'm asking YOU who's on first!" },
      { 0x8888, 22, "That's the man's name." },
      { 0x4444, 23, "That's who's name?" },

      { 0x2222, 24, "Yes." },
      { 0x1111, 25, "Well go ahead and tell me." },
      { 0x8181, 26, "That's it." },
    } },
};

struct NaClDesc* bound_and_conn_cap[2];

struct desc_list {
  struct desc_list *next;
  struct NaClDesc *desc;
};

struct desc_list *sender_discards = NULL;
struct desc_list *receiver_discards = NULL;

void desc_list_discard(struct desc_list *p) {
  while (NULL != p) {
    struct desc_list *q;
    NaClDescUnref(p->desc);
    q = p->next;
    free(p);
    p = q;
  }
}
void desc_list_add(struct desc_list **dl, struct NaClDesc *d) {
  struct desc_list *entry = malloc(sizeof *entry);
  if (NULL == entry) {
    fprintf(stderr, "desc_list_add: NO MEMORY\n");
    exit(1);
  }
  entry->next = *dl;
  entry->desc = d;
  *dl = entry;
}

void sender_thread(void) {
  int err;
  struct NaClDesc *conn;
  size_t ix;
  struct NaClImcTypedMsgHdr header;
  struct NaClDesc *desc_buf[MAX_TEST_DESCS];
  size_t dix;
  ssize_t bytes_sent;

  printf("[ sender_thread ]\n");
  if (0 != (err = (*NACL_VTBL(NaClDesc, bound_and_conn_cap[1])->ConnectAddr)(
                bound_and_conn_cap[1], &conn))) {
    fprintf(stderr, "Cound not connect: NaCl errno %d\n", -err);
    exit(1);
  }
  for (ix = 0; ix < NACL_ARRAY_SIZE(test_case); ++ix) {
    printf("[ sender: Test %"NACL_PRIdS" ]\n", ix);
    header.iov = NULL;
    header.iov_length = 0;
    header.ndescv = desc_buf;
    header.ndesc_length = test_case[ix].num_desc;
    for (dix = 0; dix < header.ndesc_length; ++dix) {
      desc_buf[dix] = (struct NaClDesc *)
          NaClDescIoDescOpen(exe_path, NACL_ABI_O_RDONLY, 0);
      err = ((*NACL_VTBL(NaClDesc, desc_buf[dix])->SetMetadata)
             (desc_buf[dix], test_case[ix].descs[dix].metadata_type,
              (uint32_t) strlen(test_case[ix].descs[dix].metadata_string),
              (uint8_t const *) test_case[ix].descs[dix].metadata_string));
      if (0 != err) {
        fprintf(stderr,
                "ix %"NACL_PRIdS" dix %"NACL_PRIdS
                " SetMetadata failed NaCl errno %d\n",
                ix, dix, -err);
        exit(1);
      }
    }
    bytes_sent = ((*NACL_VTBL(NaClDesc, conn)->SendMsg)
           (conn, &header, 0));
    if (0 != bytes_sent) {
      fprintf(stderr,
              "ix %"NACL_PRIdS" SendMsg failed NaCl errno %"NACL_PRIdS"\n",
              ix, -bytes_sent);
      exit(1);
    }
    for (dix = 0; dix < header.ndesc_length; ++dix) {
      desc_list_add(&sender_discards, desc_buf[dix]);
    }
  }
  return;
}

void receiver_thread(void) {
  int err;
  struct NaClDesc *conn;
  size_t ix;
  struct NaClImcTypedMsgHdr header;
  struct NaClDesc *desc_buf[MAX_TEST_DESCS];
  size_t dix;
  ssize_t bytes_received;
  int32_t metadata_type;
  uint8_t metadata_buffer[1024];
  uint32_t metadata_buffer_num_bytes;

  printf("[ receiver_thread ]\n");
  if (0 != (err = (*NACL_VTBL(NaClDesc, bound_and_conn_cap[0])->AcceptConn)(
                bound_and_conn_cap[0], &conn))) {
    fprintf(stderr, "Cound not accept connection: NaCl errno %d\n", -err);
    exit(1);
  }
  for (ix = 0; ix < NACL_ARRAY_SIZE(test_case); ++ix) {
    printf("[ receiver: Test %"NACL_PRIdS" ]\n", ix);
    header.iov = NULL;
    header.iov_length = 0;
    header.ndescv = desc_buf;
    header.ndesc_length = NACL_ARRAY_SIZE(desc_buf);
    bytes_received = (*NACL_VTBL(NaClDesc, conn)->RecvMsg)(conn, &header, 0);
    if (0 != bytes_received) {
      fprintf(stderr,
              "ix %"NACL_PRIdS" RecvMsg failed NaCl errno %"NACL_PRIdS"\n",
              ix, -bytes_received);
      exit(1);
    }
    if (header.ndesc_length != test_case[ix].num_desc) {
      fprintf(stderr,
              "Test case %"NACL_PRIdS", expected %u descriptors, got %u\n",
              ix, test_case[ix].num_desc, header.ndesc_length);
      exit(1);
    }
    for (dix = 0; dix < header.ndesc_length; ++dix) {
      metadata_buffer_num_bytes = sizeof metadata_buffer;
      metadata_type = ((*NACL_VTBL(NaClDesc, desc_buf[dix])->GetMetadata)
                       (desc_buf[dix],
                        &metadata_buffer_num_bytes, metadata_buffer));
      if (test_case[ix].descs[dix].metadata_type != metadata_type) {
        fprintf(stderr,
                "ix %"NACL_PRIdS" dix %"NACL_PRIdS
                " GetMetadata failed expected metadata %d, got %d\n",
                ix, dix, test_case[ix].descs[dix].metadata_type, metadata_type);
        exit(1);
      }
      if (strlen(test_case[ix].descs[dix].metadata_string) !=
          metadata_buffer_num_bytes) {
        fprintf(stderr,
                "ix %"NACL_PRIdS" dix %"NACL_PRIdS
                " GetMetadata failed expected %"NACL_PRIdS" bytes, got %u\n",
                ix, dix,
                strlen(test_case[ix].descs[dix].metadata_string),
                metadata_buffer_num_bytes);
        exit(1);
      }

      metadata_buffer[metadata_buffer_num_bytes] = '\0';
      /* these are not ASCII NUL terminated strings */

      if (0 != strcmp(test_case[ix].descs[dix].metadata_string,
                      (char *) metadata_buffer)) {
        fprintf(stderr,
                "ix %"NACL_PRIdS" dix %"NACL_PRIdS
                " GetMetadata failed expected string %s, got %s\n",
                ix, dix,
                test_case[ix].descs[dix].metadata_string,
                (char *) metadata_buffer);
        exit(1);
      }
      printf("%s\n", metadata_buffer);
    }
    /* could unref right away */
    for (dix = 0; dix < header.ndesc_length; ++dix) {
      desc_list_add(&receiver_discards, desc_buf[dix]);
    }
  }
  return;
}

void WINAPI child_thread_fn(void *child_thread_state) {
  /*
   * create and send a desc with flags, and then with metadata string,
   * and then with both.
   */
  UNREFERENCED_PARAMETER(child_thread_state);
  sender_thread();
  return;
}

int main(int ac, char **av) {
  int err;
  struct NaClThread child_thread;

  if (ac < 2) {
    fprintf(stderr,
            "metadata_test: the first argument should be path to a file\n");
    exit(2);
  }
  NaClPlatformInit();
  exe_path = av[1];
  if (0 != (err = NaClCommonDescMakeBoundSock(bound_and_conn_cap))) {
    fprintf(stderr,
            "NaClCommonDescMakeBoundSock failed: NaCl errno %d\n", -err);
    exit(3);
  }
  if (!NaClThreadCreateJoinable(&child_thread,
                                child_thread_fn,
                                (void *) NULL,
                                65536)) {
    fprintf(stderr, "could not create worker thread\n");
    exit(4);
  }

  receiver_thread();

  NaClThreadJoin(&child_thread);

  desc_list_discard(sender_discards);
  desc_list_discard(receiver_discards);

  NaClPlatformFini();
  printf("OK\n");
  return 0;
}
