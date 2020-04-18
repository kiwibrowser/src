/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <iostream>
#include <limits>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/public/irt_core.h"
#include "native_client/src/public/nonsfi/elf_loader.h"
#include "native_client/tests/nonsfi/example_interface.h"

namespace {

// The implementation of our interface function.
int add_one(uint32_t number, uint32_t *result) {
  // Error handling in case of overflow
  if (number == std::numeric_limits<uint32_t>::max())
    return EOVERFLOW;
  *result = number + 1;
  return 0;
}

// Our instatiation of the interface.
const struct nacl_irt_example k_irt_example = {
    add_one,
    // More functions can go here.
};

// It is possible to have multiple interfaces here. For simplicity, we are only
// showing one.
const struct nacl_irt_interface irt_interfaces[] = {
    {NACL_IRT_EXAMPLE_v1_0, &k_irt_example, sizeof(k_irt_example), NULL}};

size_t example_irt_nonsfi_query(const char *interface_ident, void *table,
                                size_t tablesize) {
  // First, try to access our custom interface.
  size_t result = nacl_irt_query_list(interface_ident, table, tablesize,
                                      irt_interfaces, sizeof(irt_interfaces));
  if (result != 0)
    return result;
  // Fall back to the nonsfi default core.
  return nacl_irt_query_core(interface_ident, table, tablesize);
}

}  // namespace

// Our example loader. The important part is that we call "NaClLoadElfFile" to
// get our entrypoint to the nexe, and pass that to "nacl_irt_nonsfi_entry"
// along with the appropriate querying function.
int main(int argc, char **argv, char **environ) {
  nacl_irt_nonsfi_allow_dev_interfaces();
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <executable> <args...>\n";
    return 1;
  }
  const char *nexe_filename = argv[1];
  int fd = open(nexe_filename, O_RDONLY);
  if (fd < 0) {
    std::cerr << "Failed to open " << nexe_filename << "\n";
    return 1;
  }
  // Takes ownership of file descriptor -- we don't need to close fd.
  uintptr_t entry = NaClLoadElfFile(fd);
  return nacl_irt_nonsfi_entry(argc - 1, argv + 1, environ,
                               reinterpret_cast<nacl_entry_func_t>(entry),
                               example_irt_nonsfi_query);
}
