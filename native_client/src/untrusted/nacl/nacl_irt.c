/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

static int stub_close(int fd) {
  return ENOSYS;
}

static int stub_dup(int fd, int *newfd) {
  return ENOSYS;
}

static int stub_dup2(int fd, int newfd) {
  return ENOSYS;
}

static int stub_read(int fd, void *buf, size_t count, size_t *nread) {
  return ENOSYS;
}

static int stub_write(int fd, const void *buf, size_t count, size_t *nwrote) {
  return ENOSYS;
}

static int stub_seek(int fd, off_t offset, int whence, off_t *new_offset) {
  return ENOSYS;
}

static int stub_fstat(int fd, nacl_irt_stat_t *st) {
  return ENOSYS;
}

static int stub_getdents(int fd, struct dirent *dirent, size_t count,
                         size_t *nread) {
  return ENOSYS;
}

struct nacl_irt_basic __libnacl_irt_basic;
struct nacl_irt_memory __libnacl_irt_memory;
struct nacl_irt_tls __libnacl_irt_tls;
struct nacl_irt_clock __libnacl_irt_clock;
struct nacl_irt_dev_getpid __libnacl_irt_dev_getpid;

struct nacl_irt_dev_filename __libnacl_irt_dev_filename;
struct nacl_irt_dev_fdio __libnacl_irt_dev_fdio;

struct nacl_irt_fdio __libnacl_irt_fdio = {
  stub_close,
  stub_dup,
  stub_dup2,
  stub_read,
  stub_write,
  stub_seek,
  stub_fstat,
  stub_getdents,
};

TYPE_nacl_irt_query __nacl_irt_query;


/*
 * Avoid a dependency on libc's strlen function.
 */
static size_t my_strlen(const char *s) {
  size_t len = 0;
  while (*s++) ++len;
  return len;
}

/*
 * TODO(robertm): make the helper below globally accessible.
 */
static void __libnacl_message(const char *message) {
   /*
    * Skip write if fdio is not available.
    */
  if (__libnacl_irt_fdio.write == NULL) {
    return;
  }
  size_t dummy_bytes_written;
  size_t len = my_strlen(message);
  __libnacl_irt_fdio.write(STDERR_FILENO, message, len, &dummy_bytes_written);
}

/*
 * TODO(robertm): make the helper below globally accessible.
 */
static void __libnacl_fatal(const char* message) {
  __libnacl_message(message);
  __builtin_trap();
}


int __libnacl_irt_query(const char *interface,
                        void *table, size_t table_size) {
  if (NULL == __nacl_irt_query) {
    __libnacl_fatal("No IRT interface query routine!\n");
  }
  if (__nacl_irt_query(interface, table, table_size) != table_size) {
    return 0;
  }
  return 1;
}

void __libnacl_mandatory_irt_query(const char *interface,
                                   void *table, size_t table_size) {
  if (!__libnacl_irt_query(interface, table, table_size)) {
    __libnacl_fatal("IRT interface query failed for essential interface\n");
  }
}

int __libnacl_irt_init_fn(void *interface_field, void (*init)(void)) {
  if (*((void **) interface_field) == NULL) {
    init();
    if (*((void **) interface_field) == NULL) {
      errno = ENOSYS;
      return 0;
    }
  }
  return 1;
}
