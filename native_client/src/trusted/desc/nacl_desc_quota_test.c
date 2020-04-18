/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_semaphore.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/nacl_time.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_quota.h"
#include "native_client/src/trusted/desc/nacl_desc_quota_interface.h"

#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

#if NACL_WINDOWS
# define UNLINK(f)  _unlink(f)
#else
# define UNLINK(f)  unlink(f)
#endif

char *gProgram = NULL;

uint64_t gNumBytes;

struct NaClDescFake {
  struct NaClDesc base NACL_IS_REFCOUNT_SUBCLASS;
  uint64_t bytes_written;
};

/*
 * A fake descriptor quota interface.
 */

static void FakeDtor(struct NaClRefCount *nrcp) {
  nrcp->vtbl = (struct NaClRefCountVtbl *)(&kNaClDescQuotaInterfaceVtbl);
  (*nrcp->vtbl->Dtor)(nrcp);
}

static int64_t FakeWriteRequest(struct NaClDescQuotaInterface *quota_interface,
                                uint8_t const                 *file_id,
                                int64_t                       offset,
                                int64_t                       length) {
  UNREFERENCED_PARAMETER(quota_interface);
  UNREFERENCED_PARAMETER(file_id);
  UNREFERENCED_PARAMETER(offset);

  NaClLog(1,
          ("NaClSrpcPepperWriteRequest(dummy): requesting length %"NACL_PRId64
           ", (0x%"NACL_PRIx64")\n"),
          length, length);
  if (length < 0) {
    NaClLog(LOG_FATAL, "Negative length: %"NACL_PRId64"\n", length);
  }
  if ((uint64_t) length > gNumBytes) {
    NaClLog(1, "NaClSrpcPepperWriteRequest(dummy): clamping!\n");
    length = (int64_t) gNumBytes;
  }
  /* post: length <= gNumBytes */
  NaClLog(1,
          ("NaClSrpcPepperWriteRequest(dummy): allowing length %"NACL_PRId64
           ", (0x%"NACL_PRIx64")\n"),
          length, length);
  gNumBytes -= length;
  return length;
}

static int64_t FakeFtruncateRequest(
    struct NaClDescQuotaInterface *quota_interface,
    uint8_t const                 *file_id,
    int64_t                       length) {
  UNREFERENCED_PARAMETER(quota_interface);
  UNREFERENCED_PARAMETER(file_id);

  NaClLog(LOG_FATAL, "FtruncateRequest invoked!?!\n");
  return length;
}

struct NaClDescQuotaInterfaceVtbl const kFakeQuotaInterfaceVtbl = {
  {
    FakeDtor
  },
  FakeWriteRequest,
  FakeFtruncateRequest
};

struct FakeQuotaInterface {
  struct NaClDescQuotaInterface base NACL_IS_REFCOUNT_SUBCLASS;
};

int FakeQuotaInterfaceCtor(struct FakeQuotaInterface *self) {
  struct NaClRefCount *nrcp = (struct NaClRefCount *) self;
  if (!NaClDescQuotaInterfaceCtor(&(self->base))) {
    return 0;
  }
  nrcp->vtbl = (struct NaClRefCountVtbl *)(&kFakeQuotaInterfaceVtbl);
  return 1;
}

int ExerciseQuotaObject(struct NaClDescQuota  *test_obj,
                        struct NaClSecureRng  *rngp,
                        char                  *buffer,
                        size_t                max_write_size,
                        uint64_t              num_bytes) {
  nacl_off64_t file_size;

  gNumBytes = num_bytes;

  NaClLog(LOG_INFO,
          "ExerciseQuotaObject: allow total %"NACL_PRId64" bytes\n",
          num_bytes);
  NaClLog(LOG_INFO,
          "ExerciseQuotaObject: random write sizes, up to %"
          NACL_PRIdS" bytes\n",
          max_write_size);

  while (gNumBytes > 0) {
    size_t    write_size;
    uint32_t  limit;
    ssize_t   result;
    uint64_t  old_size;

    NACL_COMPILE_TIME_ASSERT(sizeof(uint32_t) <= sizeof(size_t));

    /* following is always false on ILP32 */
    if (max_write_size > NACL_UMAX_VAL(uint32_t)) {
      limit = NACL_UMAX_VAL(uint32_t);
    } else {
      limit = (uint32_t) max_write_size;
    }

    write_size = (*rngp->base.vtbl->Uniform)(&rngp->base, limit);

    NaClLog(LOG_INFO, "Random size: %"NACL_PRIdS"\n", write_size);

    old_size = gNumBytes;

    result = (*NACL_VTBL(NaClDesc, test_obj)->
              Write)((struct NaClDesc *) test_obj, buffer, write_size);

    if (result < 0) {
      NaClLog(LOG_INFO, "Write result %"NACL_PRIdS"\n", result);
      return 1;
    }
    /*
     * OS short write?  Clamped write amount?
     */
    if ((size_t) result < write_size && (uint64_t) result != gNumBytes) {
      NaClLog(LOG_INFO,
              ("Short write: asked for %"NACL_PRIdS" (0x%"NACL_PRIxS") bytes,"
               " got %"NACL_PRIdS" (0x%"NACL_PRIxS") bytes.\n"),
              write_size, write_size,
              result, result);
    }
    if ((size_t) result > write_size) {
      NaClLog(LOG_FATAL,
              ("ERROR: LONG write: asked for %"NACL_PRIdS
               " (0x%"NACL_PRIxS") bytes,"
               " got %"NACL_PRIdS" (0x%"NACL_PRIxS") bytes.\n"),
              write_size, write_size,
              result, result);
      return 1;
    }

    if (old_size - gNumBytes != (uint64_t) result) {
      NaClLog(LOG_FATAL,
              "Write tracking failure.\n");
    }
  }

  NaClLog(LOG_INFO, "... checking file size\n");

  file_size = (*NACL_VTBL(NaClDesc, test_obj)->
               Seek)((struct NaClDesc *) test_obj,
                     0,
                     SEEK_END);
  CHECK((uint64_t) file_size == num_bytes);

  NaClLog(LOG_INFO, "OK.\n");

  return 0;
}

void Usage(void) {
  fprintf(stderr, "Usage: %s -f file-to-write [-n num_bytes]\n", gProgram);
}

int main(int ac, char **av) {
  int                        exit_status = 1;
  int                        num_errors = 0;
  int                        opt;
  char                       *file_path = NULL;
  char                       *buffer;
  size_t                     max_write_size = 5UL << 10;  /* > 1 page */
  size_t                     ix;
  uint64_t                   num_bytes = 16UL << 20;
  struct NaClDescIoDesc      *ndip = NULL;
  struct NaClDescQuota       *object_under_test = NULL;
  struct FakeQuotaInterface  *fake_interface = NULL;
  struct NaClSecureRng       rng;
  static uint8_t             file_id0[NACL_DESC_QUOTA_FILE_ID_LEN];
  static const char file_id0_cstr[NACL_DESC_QUOTA_FILE_ID_LEN] = "File ID 0";

  NaClLogModuleInit();
  NaClTimeInit();
  NaClSecureRngModuleInit();

  gProgram = strrchr(av[0], '/');
  if (NULL == gProgram) {
    gProgram = av[0];
  } else {
    ++gProgram;
  }

  memset(file_id0, 0, sizeof file_id0);
  memcpy(file_id0, file_id0_cstr, sizeof file_id0_cstr);

  while (EOF != (opt = getopt(ac, av, "f:m:n:"))) {
    switch (opt) {
      case 'f':
        file_path = optarg;
        break;
      case 'm':
        max_write_size = (size_t) STRTOULL(optarg, (char **) NULL, 0);
      case 'n':
        num_bytes = (uint64_t) STRTOULL(optarg, (char **) NULL, 0);
        break;
      default:
        Usage();
        exit_status = 1;
        goto cleanup;
    }
  }
  if (NULL == file_path) {
    fprintf(stderr, "Please provide a path name for a temporary file that\n");
    fprintf(stderr, "this test can create/overwrite.\n");
    Usage();
    exit_status = 2;
    goto cleanup;
  }

  /*
   * Invariant in cleanup code: ndip and object_under_test, if
   * non-NULL, needs to be Dtor'd (via NaClDescSafeUnref).  So:
   *
   * 1) We never have a partially-constructed object.  (Our Ctors fail
   *    atomically.)
   *
   * 2) If a Ctor takes ownership of another object, we immediately
   *    replace our reference to that object with NULL, so that the
   *    cleanup code would not try to Dtor the contained object
   *    independently of the containing object.
   */
  if (NULL == (ndip = NaClDescIoDescOpen(file_path,
                                         NACL_ABI_O_RDWR | NACL_ABI_O_CREAT,
                                         0777))) {
    fprintf(stderr, "%s could not open file %s\n", gProgram, file_path);
    exit_status = 3;
    goto cleanup_file;
  }
  if (NULL == (fake_interface = malloc(sizeof *fake_interface))) {
    perror(gProgram);
    fprintf(stderr, "No memory for fake interface.\n");
    exit_status = 4;
    goto cleanup_file;
  }
  if (!FakeQuotaInterfaceCtor(fake_interface)) {
    perror(gProgram);
    fprintf(stderr, "Ctor for fake quota interface failed.\n");
    /*
     * fake_interface is not constructed, so we must free it and
     * NULL it out to prevent the cleanup code Dtors from firing.
     */
    free(fake_interface);
    fake_interface = NULL;
    exit_status = 5;
    goto cleanup_file;
  }
  if (NULL == (object_under_test = malloc(sizeof *object_under_test))) {
    perror(gProgram);
    fprintf(stderr, "No memory for object under test.\n");
    exit_status = 4;
    goto cleanup_file;
  }
  if (!NaClDescQuotaCtor(object_under_test,
                         (struct NaClDesc *) ndip,
                         file_id0,
                         (struct NaClDescQuotaInterface *) fake_interface)) {
    perror(gProgram);
    fprintf(stderr, "Ctor for quota object failed.\n");
    /*
     * object_under_test is not constructed, so we must free it and
     * NULL it out to prevent the cleanup code Dtors from firing.
     */
    free(object_under_test);
    object_under_test = NULL;
    exit_status = 5;
    goto cleanup_file;
  }

  ndip = NULL;  /* object_under_test has ownership */

  if (!NaClSecureRngCtor(&rng)) {
    fprintf(stderr, "Ctor for SecureRng failed.\n");
    exit_status = 6;
    goto cleanup_file;
  }

  if (NULL == (buffer = malloc(max_write_size))) {
    fprintf(stderr, "No memory for write source buffer\n");
    exit_status = 7;
    goto cleanup_file;
  }

  for (ix = 0; ix < max_write_size; ++ix) {
    buffer[ix] = (char) ix;
  }

  num_errors += ExerciseQuotaObject(object_under_test,
                                    &rng,
                                    buffer,
                                    max_write_size,
                                    num_bytes);

  if (num_errors > 0) {
    printf("Total %d errors\n", num_errors);
    exit_status = 8;
  } else {
    printf("PASSED.\n");
    exit_status = 0;
  }

cleanup_file:
  NaClDescQuotaInterfaceSafeUnref(
      (struct NaClDescQuotaInterface *) fake_interface);
  NaClDescSafeUnref((struct NaClDesc *) ndip);
  NaClDescSafeUnref((struct NaClDesc *) object_under_test);

  if (-1 == UNLINK(file_path)) {
    perror("nacl_desc_quota_test");
    fprintf(stderr,
            "unlink of a temporary file failed during test cleanup, errno %d\n",
            errno);
    exit_status = 9;
  }

cleanup:
  return exit_status;
}
