/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <link.h>
#include <stdlib.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

/*
 * If we are started by the bootstrap program rather than in the
 * usual way, the debugger cannot figure out where our executable
 * or the dynamic linker or the shared libraries are in memory,
 * so it won't find any symbols.  But we can fake it out to find us.
 *
 * The launcher passes --r_debug=0xXXXXXXXXXXXXXXXX.
 * nacl_helper_bootstrap replaces the Xs with the address of its _r_debug
 * structure.  The debugger will look for that symbol by name to
 * discover the addresses of key dynamic linker data structures.
 * Since all it knows about is the original main executable, which
 * is the bootstrap program, it finds the symbol defined there.  The
 * dynamic linker's structure is somewhere else, but it is filled in
 * after initialization.  The parts that really matter to the
 * debugger never change.  So we just copy the contents of the
 * dynamic linker's structure into the address provided by the option.
 * Hereafter, if someone attaches a debugger (or examines a core dump),
 * the debugger will find all the symbols in the normal way.
 */

void NaClHandleRDebug(const char *switch_value, char *argv0) {
#if NACL_ANDROID
  UNREFERENCED_PARAMETER(switch_value);
  UNREFERENCED_PARAMETER(argv0);
#else
  char *endp = NULL;
  uintptr_t r_debug_addr = strtoul(switch_value, &endp, 0);
  if (r_debug_addr != 0 && *endp == '\0') {
    struct link_map *l;
    struct r_debug *bootstrap_r_debug = (struct r_debug *) r_debug_addr;
    *bootstrap_r_debug = _r_debug;

    /*
     * Since the main executable (the bootstrap program) does not
     * have a dynamic section, the debugger will not skip the
     * first element of the link_map list as it usually would for
     * an executable or PIE that was loaded normally.  But the
     * dynamic linker has set l_name for the PIE to "" as is
     * normal for the main executable.  So the debugger doesn't
     * know which file it is.  Fill in the actual file name, which
     * came in as our argv[0].
     */
    l = _r_debug.r_map;
    if (l->l_name[0] == '\0')
      l->l_name = argv0;
  }
#endif
}
