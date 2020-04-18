/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/** @file
 *
 * Measure the performace of the Nrd Xfer protocol implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

/* NOTE(robertm): no errno magic here, why? */
#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"

int                         debug = 0;

void report(struct timeval  *time_start,
            struct timeval  *time_end,
            int             num_rep,
            int             transfer_count) {
  struct timeval  time_elapsed;
  double          time_per_call;

  time_elapsed.tv_sec = time_end->tv_sec - time_start->tv_sec;
  time_elapsed.tv_usec = time_end->tv_usec - time_start->tv_usec;
  if (time_elapsed.tv_usec < 0) {
    --time_elapsed.tv_sec;
    time_elapsed.tv_usec += 1000000;
  }
  printf("Number of nrd xfers:      %d\n", num_rep);
  printf("Number of descs per xfer: %d\n", transfer_count);
  printf("Elapsed time:             %d.%06dS\n",
         (int) time_elapsed.tv_sec,
         (int) time_elapsed.tv_usec);
  time_per_call = ((time_elapsed.tv_sec + time_elapsed.tv_usec / 1.0e6)
                   / num_rep);
  printf("Time per call:            %gS or %fnS\n",
         time_per_call,
         1.0e9 * time_per_call);
  printf("Calls per sec:            %d\n", (int) (1.0 / time_per_call));
}

void server_side(int channel,
                 int num_rep) {
  int                     rep;
  int                     rv;
  int                     i;
  int                     nrds_per_call = -1;
  struct timeval          time_start;
  struct timeval          time_end;

  struct NaClAbiNaClImcMsgHdr msg_hdr;
  struct NaClAbiNaClImcMsgIoVec iov[1];
  char                    data_buffer[4096];
  int                     desc_buffer[NACL_ABI_IMC_USER_DESC_MAX];

  gettimeofday(&time_start, (void *) NULL);

  for (rep = 0; rep < num_rep; ++rep) {
    iov[0].base = data_buffer;
    iov[0].length = sizeof data_buffer;

    msg_hdr.iov = iov;
    msg_hdr.iov_length = 1;
    msg_hdr.descv = desc_buffer;
    msg_hdr.desc_length = sizeof desc_buffer / sizeof desc_buffer[0];

    rv = imc_recvmsg(channel, &msg_hdr, 0);
    if (debug) {
      printf("Received %d data bytes\n", rv);
    }

    if (rv >= 0) {
      if (debug) {
        printf("Received %d descriptors\n", msg_hdr.desc_length);
      }
      if (nrds_per_call == -1) {
        nrds_per_call = msg_hdr.desc_length;
      } else {
        if (nrds_per_call != msg_hdr.desc_length) {
          fprintf(stderr, "inconsistent number of nrds per call\n");
          fprintf(stderr, "earlier was %d, now %d\n",
                  nrds_per_call,
                  msg_hdr.desc_length);
          exit(105);
        }
      }
      for (i = 0; i < msg_hdr.desc_length; ++i) {
        int   rcvd_desc;

        rcvd_desc = msg_hdr.descv[i];

        if (-1 == close(rcvd_desc)) {
          fprintf(stderr, "close of transferred descriptor %d failed\n",
                  rcvd_desc);
        }
      }
    }
  }
  gettimeofday(&time_end, (void *) NULL);

  report(&time_start, &time_end, num_rep, nrds_per_call);
}

void client_side(int channel,
                 int num_rep,
                 int transfer_count,
                 int xfer_fd) {
  int                     i;
  int                     rv;
  struct timeval          time_start;
  struct timeval          time_end;

  struct NaClAbiNaClImcMsgHdr msg_hdr;
  struct NaClAbiNaClImcMsgIoVec iov[1];
  char                    data_buffer[4096];
  int                     desc_buffer[NACL_ABI_IMC_USER_DESC_MAX];

  iov[0].base = data_buffer;
  iov[0].length = 0;
  msg_hdr.iov = iov;
  msg_hdr.iov_length = 0;

  msg_hdr.descv = desc_buffer;
  for (i = 0; i < transfer_count; ++i) {
    desc_buffer[i] = xfer_fd;
  }
  msg_hdr.desc_length = transfer_count;

  gettimeofday(&time_start, (void *) NULL);

  for (i = 0; i < num_rep; ++i) {
    rv = imc_sendmsg(channel, &msg_hdr, 0);
    if (-1 == rv) {
      fprintf(stderr,
              "imc_sendmsg returned %d, errno %d\n",
              rv, errno);
      exit(105);
    }
  }

  gettimeofday(&time_end, (void *) NULL);
  report(&time_start, &time_end, num_rep, transfer_count);
}

int main(int  ac,
         char **av) {
  int                         opt;
  int                         server = 0;
  int                         client_desc = -1;
  int                         channel;
  size_t                      i;
  char                        *transfer_file = "/dev/null";
  unsigned long               num_rep = 1;

  /* default transfer_file inappropriate for windows! */

  while (EOF != (opt = getopt(ac, av, "c:dn:st:T:"))) {
    switch (opt) {
      case 'c':
        client_desc = strtol(optarg, (char **) 0, 0);
        /* descriptor holds a connection capability for the server */
        break;
      case 'd':
        ++debug;
        break;
      case 'n':
        num_rep = strtoul(optarg, (char **) 0, 0);
        break;
      case 's':
        server = 1;
        break;
      case 't':
        transfer_file = optarg;
        break;
      default:
        fprintf(stderr,
                "Usage: sel_ldr [sel_ldr_args] -- "  /* no newline */
                "nrd_xfer_test.nexe [-c server-addr-desc] [-s]\n"
                "    [-n num_rep]\n"
                "    [-t file-to-open-and-transfer]\n"
                "\n"
                "Typically, server is run using a command such as\n"
                "\n"
                "    sel_ldr -X -1 -D 1 -- nrd_xfer_perf.nexe -s\n"
                "\n"
                "so the socket address is printed to standard output,\n"
                "and then the client is run with a command such as\n"
                "\n"
                "    sel_ldr -d -X -1 -a 6:<addr-from-server> " /* no \n */
                "-- nrd_xfer_perf.nexe -c 6\n"
                "\n"
                "to have descriptor 6 be used to represent the socket\n"
                "address for connecting to the server\n"
                "\n"
                "For the client side, -t specifies the file to open\n"
                "to pass to the server --\n"
                "remember that the -d (debug) flag is needed for\n"
                "sel_ldr to enable file opens.\n");
        return 1;
    }
  }

  printf("\nStarting in %s mode\n", server ? "server" : "client");
  printf("Debug is %d\n", debug);

  if (server) {
    channel = imc_accept(3);

    for (i = 0; i < NACL_ABI_IMC_USER_DESC_MAX; ++i) {
      server_side(channel, num_rep);
    }

    if (-1 == close(channel)) {
      fprintf(stderr, "close of channel %d failed\n", channel);
    }
  } else {
    int xfer_fd = -1;

    if (-1 == client_desc) {
      fprintf(stderr,
              "Client needs server socket address to which to connect\n");
      return 100;
    }

    channel = imc_connect(client_desc);

    if (-1 == channel) {
      fprintf(stderr, "Client could not connect\n");
      return 102;
    }

    xfer_fd = open(transfer_file, O_CREAT | O_RDONLY, 0777);
    if (-1 == xfer_fd) {
      fprintf(stderr,
              "Could not open file \"%s\" to transfer descriptor.\n",
              transfer_file);
      return 104;
    }

    for (i = 1; i <= NACL_ABI_IMC_USER_DESC_MAX; ++i) {
      client_side(channel, num_rep, i, xfer_fd);
    }
  }
  return 0;
}
