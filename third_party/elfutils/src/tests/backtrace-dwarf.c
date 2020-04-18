/* Test program for unwinding of complicated DWARF expressions.
   Copyright (C) 2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <assert.h>
#include <signal.h>
#include <inttypes.h>
#include <stdio_ext.h>
#include <locale.h>
#include <errno.h>
#include <sys/ptrace.h>
#include ELFUTILS_HEADER(dwfl)

static void cleanup_13_abort (void);
#define main cleanup_13_main
#include "cleanup-13.c"
#undef main

static void
report_pid (Dwfl *dwfl, pid_t pid)
{
  int result = dwfl_linux_proc_report (dwfl, pid);
  if (result < 0)
    error (2, 0, "dwfl_linux_proc_report: %s", dwfl_errmsg (-1));
  else if (result > 0)
    error (2, result, "dwfl_linux_proc_report");

  if (dwfl_report_end (dwfl, NULL, NULL) != 0)
    error (2, 0, "dwfl_report_end: %s", dwfl_errmsg (-1));

  result = dwfl_linux_proc_attach (dwfl, pid, true);
  if (result < 0)
    error (2, 0, "dwfl_linux_proc_attach: %s", dwfl_errmsg (-1));
  else if (result > 0)
    error (2, result, "dwfl_linux_proc_attach");
}

static Dwfl *
pid_to_dwfl (pid_t pid)
{
  static char *debuginfo_path;
  static const Dwfl_Callbacks proc_callbacks =
    {
      .find_debuginfo = dwfl_standard_find_debuginfo,
      .debuginfo_path = &debuginfo_path,

      .find_elf = dwfl_linux_proc_find_elf,
    };
  Dwfl *dwfl = dwfl_begin (&proc_callbacks);
  if (dwfl == NULL)
    error (2, 0, "dwfl_begin: %s", dwfl_errmsg (-1));
  report_pid (dwfl, pid);
  return dwfl;
}

static int
frame_callback (Dwfl_Frame *state, void *frame_arg)
{
  Dwarf_Addr pc;
  bool isactivation;
  if (! dwfl_frame_pc (state, &pc, &isactivation))
    {
      error (0, 0, "%s", dwfl_errmsg (-1));
      return DWARF_CB_ABORT;
    }
  Dwarf_Addr pc_adjusted = pc - (isactivation ? 0 : 1);

  /* Get PC->SYMNAME.  */
  Dwfl_Thread *thread = dwfl_frame_thread (state);
  Dwfl *dwfl = dwfl_thread_dwfl (thread);
  Dwfl_Module *mod = dwfl_addrmodule (dwfl, pc_adjusted);
  const char *symname = NULL;
  if (mod)
    symname = dwfl_module_addrname (mod, pc_adjusted);

  printf ("%#" PRIx64 "\t%s\n", (uint64_t) pc, symname);

  if (symname && (strcmp (symname, "main") == 0
		  || strcmp (symname, ".main") == 0))
    {
      kill (dwfl_pid (dwfl), SIGKILL);
      exit (0);
    }

  return DWARF_CB_OK;
}

static int
thread_callback (Dwfl_Thread *thread, void *thread_arg)
{
  dwfl_thread_getframes (thread, frame_callback, NULL);
  error (1, 0, "dwfl_thread_getframes: %s", dwfl_errmsg (-1));
}

int
main (int argc __attribute__ ((unused)), char **argv)
{
  /* We use no threads here which can interfere with handling a stream.  */
  __fsetlocking (stdin, FSETLOCKING_BYCALLER);
  __fsetlocking (stdout, FSETLOCKING_BYCALLER);
  __fsetlocking (stderr, FSETLOCKING_BYCALLER);

  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  elf_version (EV_CURRENT);

  pid_t pid = fork ();
  switch (pid)
  {
    case -1:
      abort ();
    case 0:;
      long l = ptrace (PTRACE_TRACEME, 0, NULL, NULL);
      assert_perror (errno);
      assert (l == 0);
      cleanup_13_main ();
      abort ();
    default:
      break;
  }

  errno = 0;
  int status;
  pid_t got = waitpid (pid, &status, 0);
  assert_perror (errno);
  assert (got == pid);
  assert (WIFSTOPPED (status));
  assert (WSTOPSIG (status) == SIGABRT);

  Dwfl *dwfl = pid_to_dwfl (pid);
  dwfl_getthreads (dwfl, thread_callback, NULL);

  /* There is an exit (0) call if we find the "main" frame,  */
  error (1, 0, "dwfl_getthreads: %s", dwfl_errmsg (-1));
}
