// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/nacl_irt/irt_interfaces.h"

#include <unistd.h>

#include "build/build_config.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "ppapi/nacl_irt/irt_manifest.h"
#include "ppapi/nacl_irt/public/irt_ppapi.h"
#include "ppapi/native_client/src/untrusted/pnacl_irt_shim/irt_shim_ppapi.h"

#if defined(OS_NACL_SFI)
static int ppapihook_pnacl_private_filter(void) {
  int pnacl_mode = sysconf(NACL_ABI__SC_NACL_PNACL_MODE);
  if (pnacl_mode == -1)
    return 0;
  return pnacl_mode;
}
#endif

static const nacl_irt_resource_open kIrtResourceOpen = {
  ppapi::IrtOpenResource,
};

#if defined(OS_NACL_SFI)
static int not_pnacl_filter(void) {
  int pnacl_mode = sysconf(NACL_ABI__SC_NACL_PNACL_MODE);
  if (pnacl_mode == -1)
    return 0;
  return !pnacl_mode;
}
#endif

static const struct nacl_irt_interface irt_interfaces[] = {
  { NACL_IRT_PPAPIHOOK_v0_1, &nacl_irt_ppapihook, sizeof(nacl_irt_ppapihook),
    NULL },
#if defined(OS_NACL_SFI)
  { NACL_IRT_PPAPIHOOK_PNACL_PRIVATE_v0_1,
    &nacl_irt_ppapihook_pnacl_private, sizeof(nacl_irt_ppapihook_pnacl_private),
    ppapihook_pnacl_private_filter },
#endif
  { NACL_IRT_RESOURCE_OPEN_v0_1, &kIrtResourceOpen,
    sizeof(kIrtResourceOpen),
#if defined(OS_NACL_SFI)
    not_pnacl_filter,
#else
    // If we change PNaCl to use Non-SFI Mode on the open web,
    // we should add a filter here.
    NULL,
#endif
  },
#if defined(OS_NACL_SFI)
  // TODO(mseaborn): Ideally these two PNaCl translator interfaces should
  // be hidden in processes that aren't PNaCl sandboxed translator
  // processes.  However, we haven't yet plumbed though a flag to indicate
  // when a NaCl process is a PNaCl translator process.  The risk of an app
  // accidentally depending on the presence of this interface is much lower
  // than for other non-stable IRT interfaces, because this interface is
  // not useful to apps.
  { NACL_IRT_PRIVATE_PNACL_TRANSLATOR_LINK_v0_1,
    &nacl_irt_private_pnacl_translator_link,
    sizeof(nacl_irt_private_pnacl_translator_link), NULL },
  { NACL_IRT_PRIVATE_PNACL_TRANSLATOR_COMPILE_v0_1,
    &nacl_irt_private_pnacl_translator_compile,
    sizeof(nacl_irt_private_pnacl_translator_compile), NULL },
#endif
};

size_t chrome_irt_query(const char* interface_ident,
                        void* table, size_t tablesize) {
  size_t result = nacl_irt_query_list(interface_ident,
                                      table,
                                      tablesize,
                                      irt_interfaces,
                                      sizeof(irt_interfaces));
  if (result != 0)
    return result;

  return nacl_irt_query_core(interface_ident, table, tablesize);
}
