// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file implements BSD-style setproctitle() for Linux.
// It is written such that it can easily be compiled outside Chromium.
//
// The Linux kernel sets up two locations in memory to pass arguments and
// environment variables to processes. First, there are two char* arrays stored
// one after another: argv and environ. A pointer to argv is passed to main(),
// while glibc sets the global variable |environ| to point at the latter. Both
// of these arrays are terminated by a NULL pointer; the environment array is
// also followed by some empty space to allow additional variables to be added.
//
// These arrays contain pointers to a second location in memory, where the
// strings themselves are stored one after another: first all the arguments,
// then the environment variables. The kernel will allocate a single page of
// memory for this purpose, so the end of the page containing argv[0] is the
// end of the storage potentially available to store the process title.
//
// When the kernel reads the command line arguments for a process, it looks at
// the range of memory within this page that it initially used for the argument
// list. If the terminating '\0' character is still where it expects, nothing
// further is done. If it has been overwritten, the kernel will scan up to the
// size of a page looking for another. (Note, however, that in general not that
// much space is actually mapped, since argv[0] is rarely page-aligned and only
// one page is mapped.)
//
// Thus to change the process title, we must move any environment variables out
// of the way to make room for a potentially longer title, and then overwrite
// the memory pointed to by argv[0] with a single replacement string, making
// sure its size does not exceed the available space.
//
// It is perhaps worth noting that patches to add a system call to Linux for
// this, like in BSD, have never made it in: this is the "official" way to do
// this on Linux. Presumably it is not in glibc due to some disagreement over
// this position within the glibc project, leaving applications caught in the
// middle. (Also, only a very few applications need or want this anyway.)

#include "services/service_manager/embedder/set_process_title_linux.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern char** environ;

static char** g_main_argv = NULL;
static char* g_orig_argv0 = NULL;

void setproctitle(const char* fmt, ...) {
  va_list ap;
  size_t i, avail_size;
  uintptr_t page_size, page, page_end;
  // Sanity check before we try and set the process title.
  // The BSD version allows fmt == NULL to restore the original title.
  if (!g_main_argv || !environ || !fmt)
    return;
  if (!g_orig_argv0) {
    // Save the original argv[0].
    g_orig_argv0 = strdup(g_main_argv[0]);
    if (!g_orig_argv0)
      return;
  }
  page_size = sysconf(_SC_PAGESIZE);
  // Get the page on which the argument list and environment live.
  page = (uintptr_t)g_main_argv[0];
  page -= page % page_size;
  page_end = page + page_size;
  // Move the environment out of the way. Note that we are moving the values,
  // not the environment array itself (which may not be on the page we need
  // to overwrite anyway).
  for (i = 0; environ[i]; ++i) {
    uintptr_t env_i = (uintptr_t)environ[i];
    // Only move the value if it's actually in the way. This avoids
    // leaking copies of the values if this function is called again.
    if (page <= env_i && env_i < page_end) {
      char* copy = strdup(environ[i]);
      // Be paranoid. Check for allocation failure and bail out.
      if (!copy)
        return;
      environ[i] = copy;
    }
  }
  // Put the title in argv[0]. We have to zero out the space first since the
  // kernel doesn't actually look for a null terminator unless we make the
  // argument list longer than it started.
  avail_size = page_end - (uintptr_t)g_main_argv[0];
  memset(g_main_argv[0], 0, avail_size);
  va_start(ap, fmt);
  if (fmt[0] == '-') {
    vsnprintf(g_main_argv[0], avail_size, &fmt[1], ap);
  } else {
    size_t size = snprintf(g_main_argv[0], avail_size, "%s ", g_orig_argv0);
    if (size < avail_size)
      vsnprintf(g_main_argv[0] + size, avail_size - size, fmt, ap);
  }
  va_end(ap);
  g_main_argv[1] = NULL;
}

// A version of this built into glibc would not need this function, since
// it could stash the argv pointer in __libc_start_main(). But we need it.
void setproctitle_init(const char** main_argv) {
  if (g_main_argv)
    return;

  uintptr_t page_size = sysconf(_SC_PAGESIZE);
  // Check that the argv array is in fact on the same page of memory
  // as the environment array just as an added measure of protection.
  if (((uintptr_t)environ) / page_size == ((uintptr_t)main_argv) / page_size)
    g_main_argv = const_cast<char**>(main_argv);
}
