/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * NaCl inter-module communication primitives.
 * Primitive NaCl socket and shared memory functions which provide a portable
 * inter-module communication mechanism between processes on Windows, Mac OS X,
 * and Unix variants. On Unix variants, these functions are simple wrapper
 * functions for the AF_UNIX domain socket API.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_IMC_NACL_IMC_C_H_
#define NATIVE_CLIENT_SRC_SHARED_IMC_NACL_IMC_C_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#ifndef __native_client__
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"
#endif  /* __native_client__ */


struct NaClDescEffector;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * NaCl resource descriptor type NaCl resource descriptors can be
 * directly used with epoll on Linux, or with WaitForMultipleObject on
 * Windows.  TODO(sehr): work out one place for this definition.
 */

#ifndef __nacl_handle_defined
#define __nacl_handle_defined
#if NACL_WINDOWS
typedef HANDLE NaClHandle;
#else
typedef int NaClHandle;
#endif
#endif

#define NACL_INVALID_HANDLE ((NaClHandle) -1)

NaClHandle NaClDuplicateNaClHandle(NaClHandle handle);

/* The maximum length of the zero-terminated pathname for SocketAddress */
#define NACL_PATH_MAX 28            /* TBD */

/*
 * A NaCl socket address is defined as a pathname. The pathname must
 * be a zero- terminated character string starting from a character
 * within the ranges A - Z or a - z. The pathname is not case
 * sensitive.
*/

typedef struct NaClSocketAddress {
  char path[NACL_PATH_MAX];
} NaClSocketAddress;

/*
 * I/O vector for the scatter/gather operation used by
 * NaClSendDatagram() and NaClReceiveDatagram()
*/

typedef struct NaClIOVec {
  void*  base;
  size_t length;
} NaClIOVec;

/* The maximum number of handles to be passed by NaClSendDatagram() */
#define NACL_HANDLE_COUNT_MAX 8     /* TBD */
/*
 * If you are going to touch OSX code, read this!
 *
 * CMSG_SPACE() is supposed to be a constant expression, and most
 * BSD-ish code uses it to determine the size of buffers used to
 * send/receive descriptors in the control message for
 * sendmsg/recvmsg.  Such buffers are often automatic variables.
 *
 * In Darwin, CMSG_SPACE uses __DARWIN_ALIGN.  And __DARWIN_ALIGN(p)
 * expands to
 *
 * ((__darwin_size_t)((char *)(__darwin_size_t)(p) + __DARNWIN_ALIGNBYTES)
 *  &~ __DARWIN_ALIGNBYTES)
 *
 * which Clang (to which many Apple employees contribute!) complains
 * about when invoked with -pedantic -- and with our -Werror, causes
 * Clang-based builds to fail.  Clang says this is not a constant
 * expression:
 *
 * error: variable length array folded to constant array as an
 * extension [-Werror,-pedantic]
 *
 * Possibly true per standard definition, since that cast to (char *)
 * is ugly, but actually Clang was able to convert to a compile-time
 * constant... it just had to work harder.
 *
 * As a ugly workaround, we define the following constant for use in
 * automatic array declarations, and in all cases have an assert to
 * verify that the value is of sufficient size.  (The assert should
 * constant propagate and be dead-code eliminated in normal compiles
 * and should cause a quick death if ever violated, since NaCl startup
 * code involves the use of descriptor passing through the affected
 * code.)
 */
#define CMSG_SPACE_KHANDLE_COUNT_MAX_INTS (8 * 4 + 16)

/*
 * NaClMessageHeader flags set by NaClReceiveDatagram()
 */
/* The trailing portion of a message was discarded. */
#define NACL_MESSAGE_TRUNCATED 0x1
/* Not all the handles were received. */
#define NACL_HANDLES_TRUNCATED 0x2

/* Message header used by NaClSendDatagram() and NaClReceiveDatagram() */
typedef struct NaClMessageHeader {
  NaClIOVec*  iov;            /* scatter/gather array */
  uint32_t    iov_length;     /* number of elements in iov */
  NaClHandle* handles;        /* array of handles to be transferred */
  uint32_t    handle_count;   /* number of handles in handles */
  int         flags;
} NaClMessageHeader;

/*
 * Creates a NaCl socket associated with the local address.
 *
 * NaClBoundSocket() returns a handle of the newly created
 * bound socket on success, and NACL_INVALID_HANDLE on failure.
 */
NaClHandle NaClBoundSocket(const NaClSocketAddress* address);

/*
 * Creates an unnamed pair of connected sockets.  NaClSocketPair()
 * return 0 on success, and -1 on failure.
 */
int NaClSocketPair(NaClHandle pair[2]);

/*
 * Closes a NaCl descriptor created by NaCl primitives.
 *
 * NaClClose() returns 0 on success, and -1 on failure. Note NaCl
 * descriptors must be explicitly closed by NaClClose(). Otherwise,
 * the resources of the underlining operating system will not be
 * released correctly.
 */
int NaClClose(NaClHandle handle);

/*
 * NaClSendDatagram()/NaClReceiveDatagram() flags
 */

#define NACL_DONT_WAIT 0x1  /* Enables non-blocking operation */

/*
 * Checks the last non-blocking operation was failed because no
 * message is available in the queue.  WouldBlock() returns non-zero
 * value if the previous non-blocking NaClSendDatagram() or
 * NaClReceiveDatagram() operation would block if NACL_DONT_WAIT was
 * not specified.
 */
int NaClWouldBlock(void);

/*
 * Sends a message on a socket.
 *
 * NaClSendDatagram() sends the message to the remote peer
 * of the connection created by SocketPair().
 *
 * NaClSendDatagramTo() sends the message to the socket specified by
 * the name.
 *
 * The send functions return the number of bytes sent, or -1 upon
 * failure.  If NACL_DONT_WAIT flag is specified with the call and the
 * other peer of the socket is unable to receive more data, the
 * function returns -1 without waiting, and the subsequent
 * NaClWouldBlock() will return non-zero value.
 *
 * Note it is not safe to send messages from the same socket handle by
 * multiple threads simultaneously unless the destination address is
 * explicitly specified by NaClSendDatagramTo().
 */
int NaClSendDatagram(NaClHandle socket, const NaClMessageHeader* message,
                     int flags);
int NaClSendDatagramTo(const NaClMessageHeader* message,
                       int flags, const NaClSocketAddress* name);

/*
 * Receives a message from a socket.
 *
 * The receive functions return the number of bytes received,
 * or -1 upon failure.
 *
 * If NACL_DONT_WAIT flag is specified with the call and no messages are
 * available in the queue, the function returns -1 and the subsequent
 * NaClWouldBlock() will return non-zero value. Internally, in this case
 * ERROR_PIPE_LISTENING is set to the last error code on Windows and EAGAIN is
 * set to errno on Linux.
 *
 * Note it is not safe to receive messages from the same socket handle
 * by multiple threads simultaneously unless the socket handle is created
 * by NaClBoundSocket().
 */

int NaClReceiveDatagram(NaClHandle socket, NaClMessageHeader* message,
                        int flags);

/*
 * Message size validator.  The ABI requires that the data size must
 * be less than 2**32 bytes.
 */
int NaClMessageSizeIsValid(const NaClMessageHeader *message);

/*
 * Type of function supplied to NaClSetCreateMemoryObjectFunc().  Such
 * a function creates a memory object of length bytes.
 * @param length The size of the memory object to create. It must be a
 *               multiple of allocation granularity given by
 *               kMapPageSize.
 * @param executable Whether the memory object needs to be mappable
 *                   with PROT_EXEC.  On Mac OS X, FDs created with
 *                   shm_open() are not mappable with PROT_EXEC, so
 *                   this flag indicates whether an alternative FD
 *                   type must be used.
 * @return A handle of the newly created memory object on success, and
 *         kInvalidHandle on failure.
 */
typedef NaClHandle (*NaClCreateMemoryObjectFunc)(size_t length, int executable);

/*
 * This allows an alternative implementation of NaClCreateMemoryObject()
 * to be provided that works in an outer sandbox.
 */
void NaClSetCreateMemoryObjectFunc(NaClCreateMemoryObjectFunc func);

/*
 * Creates a memory object of length bytes.
 *
 * NaClCreateMemoryObject() returns a handle of the newly created
 * memory object on success, and NACL_INVALID_HANDLE on failure.
 * length must be a multiple of allocation granularity given by
 * NACL_MAP_PAGESIZE in nacl_config.h.
 *
 * executable: Whether the memory object needs to be mappable as
 * executable.  (This is significant only on Mac OS X.)
 */

NaClHandle NaClCreateMemoryObject(size_t length, int executable);

/*
 * Maps the specified memory object in the process address space.
 *
 * NaClMap() returns a pointer to the mapped area, or NACL_ABI_MAP_FAILED upon
 * error.
 * For prot, the bitwise OR of the NACL_ABI_PROT_* bits must be specified.
 * For flags, either NACL_ABI_MAP_SHARED or NACL_ABI_MAP_PRIVATE must
 * be specified.
 * If NACL_ABI_MAP_FIXED is also set, NaClMap() tries to map the
 * memory object at the address specified by start.
 */
void* NaClMap(struct NaClDescEffector* effp,
              void* start, size_t length, int prot, int flags,
              NaClHandle memory, off_t offset);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* NATIVE_CLIENT_SRC_SHARED_IMC_NACL_IMC_C_H_ */
