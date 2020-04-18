/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

/*
 * An open_resource implementation used for testing PNaCl's sandboxed
 * translator (the linker), outside of Chromium.
 *
 * This code will prepend the |base_dir| to |pathname|. That is useful if,
 * say, |pathname| is the basename of a library, and |base_dir| is a directory.
 * holding library files. The |base_dir| is assumed to already have a
 * separator char at the end.
 *
 * PNaCl's sandboxed linker also has hardcoded the name of a shim library.
 * That will open the Chromium PPAPI shim when used with Chromium, but when
 * testing outside of Chromium we want to open a dummy shim library instead.
 * |file_remap| is used to redirect the hardcoded shim library name to the
 * dummy shim. This is specified as an environment variable, e.g.:
 * "NACL_IRT_OPEN_RESOURCE_REMAPPED=lib_orig.a:lib_dummy.a"
 */
static int nacl_irt_open_resource_remapped(const char *pathname,
                                           const char *base_dir,
                                           const char *file_remap) {
  if (file_remap != NULL) {
    const char *delim_ptr = strchr(file_remap, ':');
    size_t remap_from_len = delim_ptr - file_remap;
    if (delim_ptr != NULL) {
      if (strncmp(pathname, file_remap, remap_from_len) == 0 &&
          strlen(pathname) == remap_from_len) {
        pathname = delim_ptr + 1;
      }
    } else {
      fprintf(
          stderr,
          "NACL_IRT_OPEN_RESOURCE_REMAP expects <from>:<to> instead of %s\n",
          file_remap);
      abort();
    }
  }
  size_t base_len = strlen(base_dir);
  size_t pathname_len = strlen(pathname);
  char *merged_path = (char *) malloc(
      sizeof(char) * (base_len + pathname_len + 1));
  if (merged_path == NULL) {
    errno = ENOMEM;
    return -1;
  }
  strcpy(merged_path, base_dir);
  strcat(merged_path, pathname);
  int rv = NACL_GC_WRAP_SYSCALL(NACL_SYSCALL(open)(merged_path, O_RDONLY, 0));
  free(merged_path);
  return rv;
}

static int nacl_irt_open_resource(const char *pathname, int *newfd) {
  char *base_dir = getenv("NACL_IRT_OPEN_RESOURCE_BASE");
  char *file_remap = getenv("NACL_IRT_OPEN_RESOURCE_REMAP");
  int rv;
  if (base_dir == NULL) {
    rv = NACL_GC_WRAP_SYSCALL(NACL_SYSCALL(open)(pathname, O_RDONLY, 0));
  } else {
    rv = nacl_irt_open_resource_remapped(pathname, base_dir, file_remap);
  }
  if (rv < 0)
    return -rv;
  *newfd = rv;
  return 0;
}

const struct nacl_irt_resource_open nacl_irt_resource_open = {
  nacl_irt_open_resource,
};
