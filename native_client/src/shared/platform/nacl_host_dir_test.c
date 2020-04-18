/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Exercise the NaClHostDir NaClHostDirGetdents interface.
 *
 * We use testdata input pairs: a file containing an expected
 * directory listing, and a sample directory.  The contents are just
 * the d_name portion of the directory entry, since none of the other
 * data will be preserved across SCM checkouts, etc.  We check, as a
 * heurstic, that the d_ino part is identical to that which results
 * from a NaClHostDescStat call (platform library) after
 * NaClAbiStatHostDescStatXlate processing (desc library),
 *
 * NB: the interface being tested is not uniform/cross platform.  In
 * particular, unless UNC paths are used on Windows (which we are not
 * doing), the total path name for a file is MAX_PATH, or 260
 * characters.  Since this maximum depends on where in the directory
 * tree is a file located, it is impossible to know a priori whether
 * creating a file will succeed or not without constructing the full
 * path.  Anyway, since directory descriptors are not made available
 * except with the -d flag, and the likely use of this code, if at
 * all, will be in non-browser sandboxing contexts where we won't run
 * on that many host OSes, we're not going to try to abstract away the
 * host OS differences.  For WebFS, there will be a separate directory
 * abstraction that is provided by (and is the responsibility of) the
 * host browser, and different code paths will be used, hopefully
 * providing a host-OS platform independent view.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability_io.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_host_dir.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/include/sys/dirent.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"

#define MAXLINE 1024  /* 256 = NAME_MAX+1 should suffice */
#define JOINED_MAX 4096

struct string_array {
  char    **strings;
  size_t  nelts;
  size_t  allocated_elts;
};

#if NACL_WINDOWS
static char const path_sep = '\\';
#else
static char const path_sep = '/';
#endif

int verbosity = 0;

void StringArrayCtor(struct string_array *self) {
  self->strings = NULL;
  self->nelts = 0;
  self->allocated_elts = 0;
}

void StringArrayGrow(struct string_array *self) {
  size_t desired = 2 * self->allocated_elts;
  char   **new_strings;

  if (0 == desired) desired = 16;
  new_strings = (char **) realloc(self->strings,
                                  desired * sizeof *self->strings);
  CHECK(NULL != new_strings);
  self->strings = new_strings;
  self->allocated_elts = desired;
}

void StringArrayAdd(struct string_array *self,
                    char                *entry) {
  CHECK(self->nelts <= self->allocated_elts);
  if (self->nelts == self->allocated_elts) {
    StringArrayGrow(self);
  }
  self->strings[self->nelts++] = entry;
}

static int strcmp_wrapper(const void *left, const void *right) {
  return strcmp(*(const char **) left, *(const char **) right);
}

void StringArraySort(struct string_array *self) {
  qsort(self->strings, self->nelts, sizeof(*self->strings), strcmp_wrapper);
}

uint32_t ExtractNames(struct string_array *dest_array,
                      void                *buffer,
                      size_t              buffer_size,
                      char const          *dir_path) {
  struct nacl_abi_dirent  *nadp;
  char                    path[4096];
  nacl_host_stat_t        host_stat;
  int                     rv;
  int32_t                 rv32;
  struct nacl_abi_stat    nabi_stat;
  uint32_t                error_count = 0;

  while (buffer_size > 0) {
    nadp = (struct nacl_abi_dirent *) buffer;
    StringArrayAdd(dest_array, strdup(nadp->nacl_abi_d_name));

    rv = SNPRINTF(path, sizeof path, "%s%c%s",
                  dir_path, path_sep, nadp->nacl_abi_d_name);
    if (rv < 0) {
      fprintf(stderr, "snprintf failed?!?  name %s\n", nadp->nacl_abi_d_name);
      ++error_count;
      goto next_entry;
    }
    if ((size_t) rv >= sizeof path) {
      fprintf(stderr, "path too long\n");
      ++error_count;
      goto next_entry;
    }
    if (0 != (rv = NaClHostDescStat(path, &host_stat))) {
      fprintf(stderr, "could not stat %s: %d\n", path, rv);
      ++error_count;
      goto next_entry;
    }
    rv32 = NaClAbiStatHostDescStatXlateCtor(&nabi_stat, &host_stat);
    if (0 != rv32) {
      fprintf(stderr, "NaClAbiStatHostDescStatXlateCtor failed: %d\n", rv32);
      ++error_count;
      goto next_entry;
    }
    /*
     * the inode number may be masked, but both
     * NaClAbiStatHostDescStatXlateCtor and NaClHostDirGetdents should
     * mask identically.
     */
    if (nabi_stat.nacl_abi_st_ino != nadp->nacl_abi_d_ino) {
      fprintf(stderr,
              "inode values differ: expected 0x%"NACL_PRIx64
              ", got 0x%"NACL_PRIx64"\n",
              nabi_stat.nacl_abi_st_ino, nadp->nacl_abi_d_ino);
      ++error_count;
    }
 next_entry:
    buffer = (void *) ((char *) buffer + nadp->nacl_abi_d_reclen);
    CHECK(buffer_size >= nadp->nacl_abi_d_reclen);
    buffer_size -= nadp->nacl_abi_d_reclen;
  }
  return error_count;
}

int OperateOnDir(char *dir_name, struct string_array *file_list,
                 int (*op)(char const *path)) {
  char    joined[JOINED_MAX];
  size_t  ix;
  int     errors = 0;

  for (ix = 0; ix < file_list->nelts; ++ix) {
    if (0 == strcmp(file_list->strings[ix], ".") ||
        0 == strcmp(file_list->strings[ix], "..")) {
      continue;
    }
    if ((size_t) SNPRINTF(joined, sizeof joined, "%s%c%s",
                          dir_name, path_sep, file_list->strings[ix])
        >= sizeof joined) {
      fprintf(stderr, "path buffer too small for %s%c%s\n",
              dir_name, path_sep, file_list->strings[ix]);
      ++errors;
      continue;
    }
#if NACL_WINDOWS
    if (strlen(joined) > MAX_PATH) {
      /*
       * Yuck.  We remove this entry from the list, since later we are
       * going to compare against what's actually read in, and if an
       * ignored entry is left in, then the comparison will fail.  We
       * mark the entry as deleted, and then we repair the string
       * list.
       */
      free(file_list->strings[ix]);
      file_list->strings[ix] = NULL;
      continue;
    }
#endif
    errors += (*op)(joined);
  }
#if NACL_WINDOWS
  if (1) {
    size_t dest_ix;
    for (ix = dest_ix = 0; ix < file_list->nelts; ++ix) {
      if (0 != file_list->strings[ix]) {
        file_list->strings[dest_ix] = file_list->strings[ix];
        ++dest_ix;
      }
    }
    file_list->nelts = dest_ix;
  }
#endif
  return errors;
}

static int NaClCreateFile(char const *joined) {
  int fd;

  if (1 < verbosity) {
    printf("CreateFile(%s)\n", joined);
  }
  fd = OPEN(joined, O_CREAT | O_TRUNC | O_WRONLY, 0777);
  if (-1 == fd) {
    perror("nacl_host_dir_test:CreateFile");
    fprintf(stderr, "could not create file %s\n", joined);
    return 1;
  }
  (void) CLOSE(fd);
  return 0;
}

int PopulateDirectory(char *dir_name, struct string_array *file_list) {
  /*
   * Typical use is from SCons, where Python's tempfile module picks a
   * directory name and creates it, so it would pre-exist.  For ease
   * of direct, non-SCons test execution, we try to create the
   * directory here.
   */
  (void) MKDIR(dir_name, 0777);
  return OperateOnDir(dir_name, file_list, NaClCreateFile);
}

static int NaClDeleteFile(char const *joined) {
  int retval;

  if (1 < verbosity) {
    printf("DeleteFile(%s)\n", joined);
  }
  retval = UNLINK(joined);
  if (-1 == retval) {
    perror("nacl_hst_dir_test:DeleteFile");
    return 1;
  }
  return 0;
}

int CleanupDirectory(char *dirname, struct string_array *file_list) {
  int errors = 0;

  if (0 != (errors += OperateOnDir(dirname, file_list, NaClDeleteFile))) {
    return errors;
  }
  if (1 < verbosity) {
    printf("rmdir(%s)\n", dirname);
  }
  if (0 != rmdir(dirname)) {
    perror("nacl_host_dir_test: rmdir");
#if !NACL_WINDOWS
    /*
     * Spurious errors found on Windows due to other processes looking
     * cross-eyed at the directory.  A similar problem could occur, in
     * principle, on the unlink, but so far this has't been observed.
     */
    ++errors;
#endif
  }
  return errors;
}

struct dirent_buffers {
  char const  *description;
  size_t      buffer_bytes;
};

int main(int argc, char **argv) {
  int                 opt;
  char                *test_dir = NULL;
  char                *test_file = NULL;
  FILE                *test_iop;
  struct string_array expected;
  char                line_buf[MAXLINE];
  size_t              ix;
  ssize_t             nbytes;
  int                 retval;
  struct NaClHostDir  nhd;
  struct string_array actual;

  union {
    struct nacl_abi_dirent  nad;
    char                    buffer[1024 + sizeof(uint64_t)];
  }                   actual_buffer;
  void                *aligned_buffer =
      (void *) (((uintptr_t) &actual_buffer.buffer + sizeof(uint64_t) - 1)
                & ~(sizeof(uint64_t) - 1));
  /*
   * our ABI requires -malign-double in untrusted code but our trusted
   * build does not (and cannot, since this affects the interface to
   * system libraries), so we have to manually align.
   */

  struct dirent_buffers buffers[] = {
    { "small", 20, },
    { "medium", 24, },
    { "large", 300, },
    { "extra large", 1024, },
  };

  size_t              min_elts;
  uint32_t            error_count = 0;

  NaClPlatformInit();

  while (EOF != (opt = getopt(argc, argv, "f:d:v"))) {
    switch (opt) {
      case 'f':
        test_file = optarg;
        break;
      case 'd':
        test_dir = optarg;
        break;
      case 'v':
        ++verbosity;
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_host_dir_test [-v] [-f file] [-d dir]\n"
                "\n"
                "       -v increases verbosity level\n"
                "       -f <file> specifes a file containing the expected\n"
                "          directory contents,\n"
                "       -d <dir> is the test directory to be scanned\n"
                "          using the NaClHostDir routines.\n");
        return -1;
    }
  }
  if (NULL == test_file) {
    fprintf(stderr,
            "nacl_host_dir_test: expected directory content file"
            " (-f) required\n");
    return 1;
  }
  if (NULL == test_dir) {
    fprintf(stderr,
            "nacl_host_dir_test: test directory"
            " (-d) required\n");
    return 2;
  }
  test_iop = fopen(test_file, "r");
  if (NULL == test_iop) {
    fprintf(stderr,
            "nacl_host_dir_test: could not open expected directory"
            " content file %s\n",
            test_file);
    return 3;
  }
  StringArrayCtor(&expected);
  while (NULL != fgets(line_buf, sizeof line_buf, test_iop)) {
    /*
     * We do not have newlines in the file names, though they are
     * permitted on Linux -- actually, any POSIX-compliant system.
     * Ideally, the test data would use NUL as filename terminators.
     *
     * On Windows, we do not support names greater than NAME_MAX
     * characters...
     */
    size_t len = strlen(line_buf);

    CHECK(0 < len);
    CHECK('\n' == line_buf[len-1]);
    line_buf[len-1] = '\0';
    StringArrayAdd(&expected, strdup(line_buf));
  }
  StringArraySort(&expected);

  printf("\n\nAttempting to create directory contents:\n\n");
  for (ix = 0; ix < expected.nelts; ++ix) {
    printf("%s\n", expected.strings[ix]);
  }

  if (0 != PopulateDirectory(test_dir, &expected)) {
    retval = 4;
    goto cleanup;
  }

  printf("\n\nExpected actual directory contents"
         " (some may get omitted on Windows):\n\n");
  for (ix = 0; ix < expected.nelts; ++ix) {
    printf("%s\n", expected.strings[ix]);
  }


  StringArrayCtor(&actual);

  retval = NaClHostDirOpen(&nhd, test_dir);
  if (0 != retval) {
    fprintf(stderr, "Could not open directory %s\n", test_dir);
    fprintf(stderr, "Error code %d\n", retval);
    goto cleanup;
  }
  for (;;) {
    for (ix = 0; ix < sizeof buffers/sizeof buffers[0]; ++ix) {
      if (0 < verbosity) {
        printf("Using %s buffer\n", buffers[ix].description);
      }
      nbytes = NaClHostDirGetdents(&nhd,
                                   aligned_buffer,
                                   buffers[ix].buffer_bytes);
      if (0 == nbytes) {
        if (0 != ix) {
          fprintf(stderr, "EOF on larger buffer but not smallest?!?\n");
          retval = 5;
          goto cleanup;
        }
        goto double_break;
      } else if (0 < nbytes) {
        error_count += ExtractNames(&actual, aligned_buffer, nbytes, test_dir);
        break;
      }
      /* nbytes < 0 must hold */
      if (-NACL_ABI_EINVAL != nbytes) {
        fprintf(stderr, "Unexpected error return %d\n", retval);
        retval = 6;
        goto cleanup;
      }
      if (ix + 1 == sizeof buffers/sizeof buffers[0]) {
        fprintf(stderr, "Largest buffer insufficient!?!\n");
        retval = 7;
        goto cleanup;
      }
    }
  }
double_break:
  StringArraySort(&actual);

  printf("\n\nActual directory contents:\n\n");
  for (ix = 0; ix < actual.nelts; ++ix) {
    printf("%s\n", actual.strings[ix]);
  }

  printf("Comparing...\n");
  retval = 0;
  if (expected.nelts != actual.nelts) {
    fprintf(stderr,
            "Number of directory entries differ"
            " (%"NACL_PRIuS" expected, vs %"NACL_PRIuS" actual)\n",
            expected.nelts, actual.nelts);
    retval = 8;
  }

  min_elts = (expected.nelts > actual.nelts) ? actual.nelts : expected.nelts;

  for (ix = 0; ix < min_elts; ++ix) {
    if (0 != strcmp(expected.strings[ix], actual.strings[ix])) {
      fprintf(stderr,
              "Entry %"NACL_PRIuS" differs: expected %s, actual %s\n",
              ix, expected.strings[ix], actual.strings[ix]);
      retval = 9;
    }
  }
  if (0 == retval && 0 != error_count) {
    retval = 10;
  }
cleanup:
  if (0 != CleanupDirectory(test_dir, &expected)) {
    if (0 == retval) {
      retval = 11;
    }
  }

  /*
   * Negative tests:  open file as dir, expect consistent error.
   */

  printf(0 == retval ? "PASS\n" : "FAIL\n");

  return retval;
}
