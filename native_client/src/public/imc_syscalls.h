/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _NATIVE_CLIENT_SRC_PUBLIC_IMC_SYSCALLS_H_
#define _NATIVE_CLIENT_SRC_PUBLIC_IMC_SYSCALLS_H_

/*
 * IMC API for NaCl-sandboxed code.
 * The functions declared here are available via -limc_syscalls.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct NaClAbiNaClImcMsgHdr;  /* imc_types.h */

/**
 *  @nacl
 *  Creates a bound IMC socket and returns a descriptor for the socket and the
 *  socket address.
 *  @param sock_and_addr An array of two file descriptors.  On successful
 *  return, the first is set to the descriptor of the bound socket, and can be
 *  used for, e.g., imc_accept.  On successful return, the second descriptor is
 *  set to a socket address object that may be sent to other NativeClient
 *  modules or the browser plugin.
 *  @return On success, imcmakeboundsock returns zero.  On failure, it returns
 *  -1 and sets errno appropriately.
 */
extern int imc_makeboundsock(int sock_and_addr[2]);

/**
 *  @nacl
 *  Accepts a connection on a bound IMC socket, returning a connected IMC socket
 *  descriptor.
 *  @param desc The file descriptor of an IMC bound socket.
 *  @return On success, imc_accept returns a non-negative file descriptor for
 *  the connected socket.  On failure, it returns -1 and sets errno
 *  appropriately.
 */
extern int imc_accept(int desc);

/**
 *  @nacl
 *  Connects to an IMC socket address, returning a connected IMC socket
 *  descriptor.
 *  @param desc The file descriptor of an IMC socket address.
 *  @return On success, imc_connect returns a non-negative file descriptor for
 *  the connected socket.  On failure, it returns -1 and sets errno.
 *  The returned descriptor may be used to transfer data and descriptors
 *  but is itself not transferable.
 *  appropriately.
 */
extern int imc_connect(int desc);

/**
 *  @nacl
 *  Sends a message over a specified IMC socket descriptor.
 *  descriptor.
 *  @param desc The file descriptor of an IMC socket.
 *  @param nmhp The message header structure containing information to be sent.
 *  @param flags TBD
 *  @return On success, imc_sendmsg returns a non-negative number of bytes sent
 *  to the socket.  On failure, it returns -1 and sets errno appropriately.
 *  The returned descriptor may be used to transfer data and descriptors
 *  but is itself not transferable.
 */
extern int imc_sendmsg(int desc, const struct NaClAbiNaClImcMsgHdr *nmhp,
                       int flags);

/**
 *  @nacl
 *  Receives a message over a specified IMC socket descriptor.
 *  @param desc The file descriptor of an IMC socket.
 *  @param nmhp The message header structure to be populated when receiving the
 *  message.
 *  @param flags TBD
 *  @return On success, imc_recvmsg returns a non-negative number of bytes
 *  read. On failure, it returns -1 and sets errno appropriately.
 */
extern int imc_recvmsg(int desc, struct NaClAbiNaClImcMsgHdr *nmhp, int flags);

/**
 *  @nacl
 *  Creates an IMC shared memory region, returning a file descriptor.
 *  @param nbytes The number of bytes in the requested shared memory region.
 *  @return On success, imc_mem_obj_create returns a non-negative file
 *  descriptor for the shared memory region.  On failure, it returns -1 and
 *  sets errno appropriately.
 */
extern int imc_mem_obj_create(size_t nbytes);

/**
 *  @nacl
 *  Creates an IMC socket pair, returning a pair of file descriptors.
 *  These descriptors are data-only, i.e., they may be used with
 *  imc_sendmsg and imc_recvmsg, but only if the descriptor count
 *  (desc_length) is zero.
 *  @param pair An array of two file descriptors for the two ends of the
 *  socket.
 *  @return On success imc_socketpair returns zero.  On failure, it returns -1
 *  and sets errno appropriately.
 *  The returned descriptor may only be used to transfer data
 *  and is itself transferable.
 */
extern int imc_socketpair(int pair[2]);

#ifdef __cplusplus
}
#endif

#endif
