/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_BASE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_BASE_H_

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/public/nacl_desc.h"

/* For NaClHandle */
#include "native_client/src/shared/imc/nacl_imc_c.h"

/* for nacl_off64_t */
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_sync.h"

#include "native_client/src/trusted/desc/desc_metadata_types.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"

EXTERN_C_BEGIN

struct NaClDesc;
struct nacl_abi_stat;
struct nacl_abi_timespec;
struct NaClDescEffector;
struct NaClImcTypedMsgHdr;
struct NaClMessageHeader;

/*
 * Externalization / internalization state, used by
 * Externalize/Internalize functions.  Externalize convert the
 * descriptor represented by the self (this) object to an entry in the
 * handles table in a NaClMessageHeader, and the Internalize function
 * is a factory that takes a dgram and NaClDescXferState and
 * constructs a vector of NaClDesc objects.
 *
 * This is essentially a pair of input/output iterators.  The *_end
 * values are not needed during externalization, since the SendMsg
 * code will have queried ExternalizeSize to ensure that there is
 * enough space.  During internalization, however, we try to be more
 * paranoid and check that we do not overrun our buffers.
 *
 * NB: we must assume that the NaClHandle values passed are the right
 * type; if not, it is possible to violate invariant properties
 * required by the various subclasses of NaClDesc.
 */
struct NaClDescXferState {
  /*
   * In/out value, used for both serialization and deserialization.
   * The Externalize method read/write type tags that are part of the
   * message header as well as data-based capabilities in a
   * self-describing format.
   */
  char        *next_byte;
  char        *byte_buffer_end;

  /*
   * In/out value.  Next handle to work on.
   */
  NaClHandle  *next_handle;
  NaClHandle  *handle_buffer_end;
};

enum NaClDescTypeTag {
  NACL_DESC_INVALID,
  NACL_DESC_DIR,
  NACL_DESC_HOST_IO,
  NACL_DESC_CONN_CAP,
  NACL_DESC_CONN_CAP_FD,
  NACL_DESC_BOUND_SOCKET,
  NACL_DESC_CONNECTED_SOCKET,
  NACL_DESC_SHM,
  NACL_DESC_SHM_MACH,
  NACL_DESC_MUTEX,
  NACL_DESC_CONDVAR,
  NACL_DESC_SEMAPHORE,
  NACL_DESC_SYNC_SOCKET,
  NACL_DESC_TRANSFERABLE_DATA_SOCKET,
  NACL_DESC_IMC_SOCKET,
  NACL_DESC_QUOTA,
  NACL_DESC_CUSTOM,
  NACL_DESC_NULL
  /*
   * Add new NaClDesc subclasses here.
   *
   * NB: when we add new tag types, NaClDescInternalize[] **MUST**
   * also be updated to add new internalization functions.
   */
};
#define NACL_DESC_TYPE_MAX      (NACL_DESC_NULL + 1)
#define NACL_DESC_TYPE_END_TAG  (0xff)

struct NaClInternalRealHeader {
  uint32_t  xfer_protocol_version;
  uint32_t  descriptor_data_bytes;
};

struct NaClInternalHeader {
  struct NaClInternalRealHeader h;
  /*
   * We add 0x10 here because pad must have at least one element.
   * This unfortunately also means that if NaClInternalRealHeader is
   * already a multiple of 16 in size, we will add in an unnecessary
   * 16-byte pad.  The preprocessor does not have access to sizeof
   * information, so we cannot just get rid of the pad.
   */
  char      pad[((sizeof(struct NaClInternalRealHeader) + 0x10) & ~0xf)
                - sizeof(struct NaClInternalRealHeader)];
  /* total size is a multiple of 16 bytes */
};

#define NACL_HANDLE_TRANSFER_PROTOCOL 0xd3c0de01
/* incr from here */

/*
 * Array of function pointers, indexed by NaClDescTypeTag, any one of
 * which will extract an externalized representation of the NaClDesc
 * subclass object from the message, as referenced via
 * NaClDescXferState, and when successful return the internalized
 * representation -- a newl created NaClDesc subclass object -- to the
 * caller via an out parameter.  Returns 0 on success, negative errno
 * value on failure.
 *
 * NB: we should have atomic failures.  The caller is expected to
 * allocate an array of NaClDesc pointers, and insert into the open
 * file table of the receiving NaClApp (via the NaClDescEffector
 * interface) only when all internalizations succeed.  Since even the
 * insertion can fail, the caller must keep track of the descriptor
 * numbers in case it has to back out and report that the message is
 * dropped.
 *
 * Also, when the NaClDesc object is constructed, the NaClHandle
 * consumed (from the NaClDescXferState) MUST BE replaced with
 * NACL_INVALID_HANDLE.
 */
extern int
(*NaClDescInternalize[NACL_DESC_TYPE_MAX])(struct NaClDesc **,
                                           struct NaClDescXferState *);

extern char const *NaClDescTypeString(enum NaClDescTypeTag type_tag);

/*
 * The virtual function table for NaClDesc and its subclasses.
 *
 * This interface will change when non-blocking I/O and epoll is
 * added.
 */

struct NaClDescVtbl {
  struct NaClRefCountVtbl vbase;

  /*
   * Essentially mmap.  Note that untrusted code should always use
   * NACL_ABI_MAP_FIXED, sice NaClDesc object have no idea where the
   * untrusted NaCl module's address space is located.  When non-fixed
   * mapping is used (by trusted code), the Map virtual function uses
   * an address space hole algorithm that may be subject to race
   * between two threads, and may thus fail.  In all cases, if
   * successful, the memory mapping may be unmapped at
   * NACL_MAP_PAGESIZE granularities.  (Trusted code should use
   * UnmapUnsafe, since refilling the unmapped address space with
   * inaccessible memory is probably not desirable.)
   */
  uintptr_t (*Map)(struct NaClDesc          *vself,
                   struct NaClDescEffector  *effp,
                   void                     *start_addr,
                   size_t                   len,
                   int                      prot,
                   int                      flags,
                   nacl_off64_t             offset) NACL_WUR;

  ssize_t (*Read)(struct NaClDesc *vself,
                  void            *buf,
                  size_t          len) NACL_WUR;

  ssize_t (*Write)(struct NaClDesc  *vself,
                   void const       *buf,
                   size_t           len) NACL_WUR;

  nacl_off64_t (*Seek)(struct NaClDesc  *vself,
                       nacl_off64_t     offset,
                       int              whence) NACL_WUR;

  ssize_t (*PRead)(struct NaClDesc *vself,
                   void *buf,
                   size_t len,
                   nacl_off64_t offset) NACL_WUR;

  ssize_t (*PWrite)(struct NaClDesc *vself,
                    void const *buf,
                    size_t len,
                    nacl_off64_t offset) NACL_WUR;

  int (*Fstat)(struct NaClDesc      *vself,
               struct nacl_abi_stat *statbuf);

  int (*Fchdir)(struct NaClDesc *vself) NACL_WUR;

  int (*Fchmod)(struct NaClDesc *vself,
                int             mode) NACL_WUR;

  int (*Fsync)(struct NaClDesc *vself) NACL_WUR;

  int (*Fdatasync)(struct NaClDesc *vself) NACL_WUR;

  int (*Ftruncate)(struct NaClDesc  *vself,
                   nacl_abi_off_t   length) NACL_WUR;

  /*
   * Directory access support.  Directories require support for getdents.
   */
  ssize_t (*Getdents)(struct NaClDesc *vself,
                      void            *dirp,
                      size_t          count) NACL_WUR;

  /*
   * Externalization queries this for how many data bytes and how many
   * handles are needed to transfer the "this" or "self" descriptor
   * via IMC.  If the descriptor is not transferrable, this should
   * return -NACL_ABI_EINVAL.  Success is indicated by 0, and other
   * kinds of failure should be the usual negative errno.  Should
   * never have to put the calling thread to sleep or otherwise
   * manipulate thread or process state.
   *
   * The nbytes returned do not include any kind of type tag.  The
   * type tag overhead is computed by the MsgSend code, since tagging
   * format need not be known by the per-descriptor externalization
   * code.
   */
  int (*ExternalizeSize)(struct NaClDesc  *vself,
                         size_t           *nbytes,
                         size_t           *nhandles) NACL_WUR;

  /*
   * Externalize the "this" or "self" descriptor: this will take an
   * IMC datagram object to which the Nrd will be appended, either as
   * special control data or as a descriptor/handle to be passed to
   * the recipient.  Should never have to put the calling thread to
   * sleep or otherwise manipulate thread or process state.
   */
  int (*Externalize)(struct NaClDesc          *vself,
                     struct NaClDescXferState *xfer) NACL_WUR;

  /*
   * Lock and similar syscalls cannot just indefintely block,
   * since address space move will require that all other threads are
   * stopped and in a known
   */
  int (*Lock)(struct NaClDesc *vself) NACL_WUR;

  int (*TryLock)(struct NaClDesc  *vself) NACL_WUR;

  int (*Unlock)(struct NaClDesc *vself) NACL_WUR;

  int (*Wait)(struct NaClDesc *vself,
              struct NaClDesc *mutex) NACL_WUR;

  int (*TimedWaitAbs)(struct NaClDesc                *vself,
                      struct NaClDesc                *mutex,
                      struct nacl_abi_timespec const *ts) NACL_WUR;

  int (*Signal)(struct NaClDesc *vself) NACL_WUR;

  int (*Broadcast)(struct NaClDesc  *vself) NACL_WUR;

  ssize_t (*SendMsg)(struct NaClDesc                 *vself,
                     const struct NaClImcTypedMsgHdr *nitmhp,
                     int                             flags) NACL_WUR;

  ssize_t (*RecvMsg)(struct NaClDesc               *vself,
                     struct NaClImcTypedMsgHdr     *nitmhp,
                     int                           flags) NACL_WUR;

  ssize_t (*LowLevelSendMsg)(struct NaClDesc                *vself,
                             struct NaClMessageHeader const *dgram,
                             int                            flags) NACL_WUR;

  ssize_t (*LowLevelRecvMsg)(struct NaClDesc          *vself,
                             struct NaClMessageHeader *dgram,
                             int                      flags) NACL_WUR;

  /*
   * ConnectAddr() and AcceptConn():
   * On success, returns 0 and a descriptor via *result.
   * On error, returns a negative errno value.
   */
  int (*ConnectAddr)(struct NaClDesc  *vself,
                     struct NaClDesc  **result) NACL_WUR;

  int (*AcceptConn)(struct NaClDesc *vself,
                    struct NaClDesc **result) NACL_WUR;

  int (*Post)(struct NaClDesc *vself) NACL_WUR;

  int (*SemWait)(struct NaClDesc  *vself) NACL_WUR;

  int (*GetValue)(struct NaClDesc *vself) NACL_WUR;

  /*
   * Descriptor attributes setters and getters.  These are virtual
   * functions because the NaClDescQuota subclass wraps other NaClDesc
   * subclasses, and the quota descriptors should not have attributes
   * separate from that of the wrapped descriptor -- the
   * setters/getters of the NaClDescQuota just use the corresponding
   * methods of the wrapped descriptor.
   */

  /*
   * Save a copy of the |metadata_size| bytes located at |metadata|
   * with the descriptor |self|.  The |metadata_type| argument should
   * be a non-negative integer and should be unique for each use of
   * the metadata interface.  Only one set of metadata may be saved
   * with a descriptor, and it is an error to try to set metadata on a
   * descriptor which already has metadata.
   *
   * Syscall-style function: returns 0 for success and negated errno
   * (-NACL_ABI_ENOMEM if memory allocation for making a copy of the
   * metadata fails, -NACL_ABI_EPERM if metadata already set,
   * -NACL_ABI_EINVAL of metadata_type is negative).
   */
  int (*SetMetadata)(struct NaClDesc *self,
                     int32_t metadata_type,
                     uint32_t metadata_num_bytes,
                     uint8_t const *metadata_bytes) NACL_WUR;

  /*
   * GetMetadata writes at most |*metadata_buffer_num_bytes_in_out|
   * bytes of the metadata associated with the descriptor using
   * SetMetaData to the buffer located at |metadata_buffer|.  It
   * returns the metadata_type associated with the descriptor.  On
   * return, the value at |*metadata_buffer_num_bytes_in_out| contains
   * the total number of bytes that would be written if there were no
   * size limit, i.e., the actual number of bytes written if the
   * buffer was large enough, or the number of bytes that would have
   * been written if the buffer were not.  Thus, on return, if the
   * value is less than or equal to the original value at
   * |*metadata_buffer_num_bytes_in_out|, the contents of
   * |metadata_buffer| contains all of the metadata.
   *
   * To query the size of the metadata without doing any copying, call
   * GetMetadata with |*metadata_buffer_num_bytes_in_out| equal to
   * zero, in which case |metadata_buffer| can be NULL.
   *
   * Callers should always check the return value of GetMetadata to
   * ensure that the metadata_type is the expected value.  If the type
   * is wrong, do not process the metadata -- treat the descriptor as
   * if it has no metadata.  To do otherwise would expose us to type
   * confusion attacks.
   *
   * Returns NACL_DESC_METADATA_NONE_TYPE if no metadata is associated
   * with the descriptor.
   */
  int32_t (*GetMetadata)(struct NaClDesc *self,
                         uint32_t *metadata_buffer_num_bytes_in_out,
                         uint8_t *metadata_buffer) NACL_WUR;


  void (*SetFlags)(struct NaClDesc *self,
                   uint32_t flags);

  uint32_t (*GetFlags)(struct NaClDesc *self);

  int32_t (*Isatty)(struct NaClDesc *self);

  /*
   * Inappropriate methods for the subclass will just return
   * -NACL_ABI_EINVAL.
   */

  /*
   * typeTag is one of the enumeration values from NaClDescTypeTag.
   *
   * This is not a class variable, since one must access it through an
   * instance.  Having a value in the vtable is not allowed in C++;
   * instead, we would implement this as a const virtual function that
   * returns the type tag, or RTTI which would typically be done via
   * examining the vtable pointer.  This is potentially cheaper, since
   * one could choose bit patterns for the type tags that make
   * subclass relationships easier to compute (we don't do this, since
   * we only ever need exact type checks).
   *
   * We put this at the end of the vtable so that when we add new
   * virtual functions above it, we are guaranteed to get a type
   * mismatch if any subclass implemention did not have its vtable
   * properly extended -- even when -Wmissing-field-initializers was
   * omitted.
   */
  enum NaClDescTypeTag typeTag;
};

struct NaClDesc {
  struct NaClRefCount base NACL_IS_REFCOUNT_SUBCLASS;
  uint32_t flags;

  /* "public" flags -- settable by users of NaClDesc interface */

  /*
   * It is okay to try to use this descriptor with PROT_EXEC in mmap.
   * This is just a hint to the service runtime to try direct mmap --
   * the validator cache can still disallow the operation.
   */
#define NACL_DESC_FLAGS_MMAP_EXEC_OK 0x1000

  /* private flags -- used internally by NaClDesc */
#define NACL_DESC_FLAGS_PUBLIC_MASK 0xffff

#define NACL_DESC_FLAGS_HAS_METADATA 0x10000
  /*
   * We could have used two uint16_t variables too, but that just
   * makes the serialization (externalization) and deserialization
   * (internalization) more complex; here, only the SetFlags and
   * GetFlags need to be messy.
   *
   * We do not encode the presence of metadata using metadata type, so
   * we do not have to transfer the 4 type bytes when metadata is
   * absent.  Since the interface only allows setting the metadata
   * once, we don't worry about
   */
  int32_t metadata_type;
  uint32_t metadata_num_bytes;
  uint8_t *metadata;
};

/*
 * Placement new style ctor; creates w/ ref_count of 1.
 *
 * The subclasses' ctor must call this base class ctor during their
 * contruction.
 */
int NaClDescCtor(struct NaClDesc *ndp) NACL_WUR;

extern struct NaClDescVtbl const kNaClDescVtbl;

/*
 * NaClDescSafeUnref is just like NaCDescUnref, except that ndp may be
 * NULL (in which case this is a noop).
 *
 * Used in failure cleanup of initialization code, esp in Ctors that
 * can fail.
 */
void NaClDescSafeUnref(struct NaClDesc *ndp);

/*
 * USE THE VIRTUAL FUNCTION.  THIS DECLARATION IS FOR SUBCLASSES.
 */
int NaClDescSetMetadata(struct NaClDesc *self,
                        int32_t metadata_type,
                        uint32_t metadata_num_bytes,
                        uint8_t const *metadata_bytes) NACL_WUR;

/*
 * USE THE VIRTUAL FUNCTION.  THIS DECLARATION IS FOR SUBCLASSES.
 */
int32_t NaClDescGetMetadata(struct NaClDesc *self,
                            uint32_t *metadata_buffer_num_bytes_in_out,
                            uint8_t *metadata_buffer) NACL_WUR;

/*
 * USE THE VIRTUAL FUNCTION.  THIS DECLARATION IS FOR SUBCLASSES.
 */
void NaClDescSetFlags(struct NaClDesc *self,
                      uint32_t flags);

/*
 * USE THE VIRTUAL FUNCTION.  THIS DECLARATION IS FOR SUBCLASSES.
 */
uint32_t NaClDescGetFlags(struct NaClDesc *self);

int32_t NaClDescIsattyNotImplemented(struct NaClDesc *vself);

/*
 * Base class externalize functions; all subclass externalize
 * functions should invoke these, up the class hierarchy, and add to
 * the sizes or the NaClDescXferState.
 */
int NaClDescExternalizeSize(struct NaClDesc *self,
                            size_t *nbytes,
                            size_t *nhandles);

int NaClDescExternalize(struct NaClDesc *self,
                        struct NaClDescXferState *xfer);

/*
 * The top level internalize interface are factories: they allocate
 * space, then run the internalize-in-place ctor code.  Ideally, these
 * would be two separate functions -- the memory allocation could, in
 * most cases, be simplified to be simply an attribute containing the
 * desired memory size and a generic allocator used, though of course
 * we would like to permit subclasses that contains variable size
 * arrays (at end of struct), etc.  For base (super) classes, the
 * memory allocation is not necessary, since the subclass internalize
 * function will have handled it.
 */
int NaClDescInternalizeCtor(struct NaClDesc *vself,
                            struct NaClDescXferState *xfer);


/*
 * subclasses are in their own header files.
 */


/* utility routines */

int32_t NaClAbiStatHostDescStatXlateCtor(struct nacl_abi_stat    *dst,
                                         nacl_host_stat_t const  *src);

/*
 * The following two functions are not part of the exported public
 * API.
 *
 * Read/write to a NaClHandle, much like how read/write syscalls work.
 */
ssize_t NaClDescReadFromHandle(NaClHandle handle,
                               void       *buf,
                               size_t     length);

ssize_t NaClDescWriteToHandle(NaClHandle handle,
                              void const *buf,
                              size_t     length);

/*
 * Default functions for the vtable for when the functionality is
 * inappropriate for the descriptor type -- they just return
 * -NACL_ABI_EINVAL
 */
void NaClDescDtorNotImplemented(struct NaClRefCount  *vself);

uintptr_t NaClDescMapNotImplemented(struct NaClDesc         *vself,
                                    struct NaClDescEffector *effp,
                                    void                    *start_addr,
                                    size_t                  len,
                                    int                     prot,
                                    int                     flags,
                                    nacl_off64_t            offset);

ssize_t NaClDescReadNotImplemented(struct NaClDesc  *vself,
                                   void             *buf,
                                   size_t           len);

ssize_t NaClDescWriteNotImplemented(struct NaClDesc         *vself,
                                    void const              *buf,
                                    size_t                  len);

nacl_off64_t NaClDescSeekNotImplemented(struct NaClDesc *vself,
                                        nacl_off64_t    offset,
                                        int             whence);

ssize_t NaClDescPReadNotImplemented(struct NaClDesc *vself,
                                    void *buf,
                                    size_t len,
                                    nacl_off64_t offset);

ssize_t NaClDescPWriteNotImplemented(struct NaClDesc *vself,
                                     void const *buf,
                                     size_t len,
                                     nacl_off64_t offset);

int NaClDescFstatNotImplemented(struct NaClDesc       *vself,
                                struct nacl_abi_stat  *statbuf);

int NaClDescFchdirNotImplemented(struct NaClDesc *vself);

int NaClDescFchmodNotImplemented(struct NaClDesc *vself,
                                 int             mode);

int NaClDescFsyncNotImplemented(struct NaClDesc *vself);

int NaClDescFdatasyncNotImplemented(struct NaClDesc *vself);

int NaClDescFtruncateNotImplemented(struct NaClDesc  *vself,
                                    nacl_abi_off_t   length);

ssize_t NaClDescGetdentsNotImplemented(struct NaClDesc  *vself,
                                       void             *dirp,
                                       size_t           count);

int NaClDescExternalizeSizeNotImplemented(struct NaClDesc *vself,
                                          size_t          *nbytes,
                                          size_t          *nhandles);

int NaClDescExternalizeNotImplemented(struct NaClDesc          *vself,
                                      struct NaClDescXferState *xfer);

int NaClDescLockNotImplemented(struct NaClDesc  *vself);

int NaClDescTryLockNotImplemented(struct NaClDesc *vself);

int NaClDescUnlockNotImplemented(struct NaClDesc  *vself);

int NaClDescWaitNotImplemented(struct NaClDesc  *vself,
                               struct NaClDesc  *mutex);

int NaClDescTimedWaitAbsNotImplemented(struct NaClDesc                *vself,
                                       struct NaClDesc                *mutex,
                                       struct nacl_abi_timespec const *ts);

int NaClDescSignalNotImplemented(struct NaClDesc  *vself);

int NaClDescBroadcastNotImplemented(struct NaClDesc *vself);

ssize_t NaClDescSendMsgNotImplemented(
    struct NaClDesc                 *vself,
    const struct NaClImcTypedMsgHdr *nitmhp,
    int                             flags);

ssize_t NaClDescRecvMsgNotImplemented(
    struct NaClDesc               *vself,
    struct NaClImcTypedMsgHdr     *nitmhp,
    int                           flags);

ssize_t NaClDescLowLevelSendMsgNotImplemented(
    struct NaClDesc                *vself,
    struct NaClMessageHeader const *dgram,
    int                            flags);

ssize_t NaClDescLowLevelRecvMsgNotImplemented(
    struct NaClDesc           *vself,
    struct NaClMessageHeader  *dgram,
    int                       flags);

int NaClDescConnectAddrNotImplemented(struct NaClDesc *vself,
                                      struct NaClDesc **out_desc);

int NaClDescAcceptConnNotImplemented(struct NaClDesc  *vself,
                                     struct NaClDesc  **out_desc);

int NaClDescPostNotImplemented(struct NaClDesc  *vself);

int NaClDescSemWaitNotImplemented(struct NaClDesc *vself);

int NaClDescGetValueNotImplemented(struct NaClDesc  *vself);

int NaClDescInternalizeNotImplemented(
    struct NaClDesc                **out_desc,
    struct NaClDescXferState       *xfer);


int NaClSafeCloseNaClHandle(NaClHandle h);

int NaClDescIsSafeForMmap(struct NaClDesc *self);

void NaClDescMarkSafeForMmap(struct NaClDesc *self);

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_BASE_H_
