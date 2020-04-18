/* Get Dwarf Frame state for target live PID process.
   Copyright (C) 2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "libdwflP.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

struct pid_arg
{
  DIR *dir;
  /* It is 0 if not used.  */
  pid_t tid_attached;
  /* Valid only if TID_ATTACHED is not zero.  */
  bool tid_was_stopped;
  /* True if threads are ptrace stopped by caller.  */
  bool assume_ptrace_stopped;
};

static bool
linux_proc_pid_is_stopped (pid_t pid)
{
  char buffer[64];
  FILE *procfile;
  bool retval, have_state;

  snprintf (buffer, sizeof (buffer), "/proc/%ld/status", (long) pid);
  procfile = fopen (buffer, "r");
  if (procfile == NULL)
    return false;

  have_state = false;
  while (fgets (buffer, sizeof (buffer), procfile) != NULL)
    if (strncmp (buffer, "State:", 6) == 0)
      {
	have_state = true;
	break;
      }
  retval = (have_state && strstr (buffer, "T (stopped)") != NULL);
  fclose (procfile);
  return retval;
}

static bool
ptrace_attach (pid_t tid, bool *tid_was_stoppedp)
{
  if (ptrace (PTRACE_ATTACH, tid, NULL, NULL) != 0)
    {
      __libdwfl_seterrno (DWFL_E_ERRNO);
      return false;
    }
  *tid_was_stoppedp = linux_proc_pid_is_stopped (tid);
  if (*tid_was_stoppedp)
    {
      /* Make sure there is a SIGSTOP signal pending even when the process is
	 already State: T (stopped).  Older kernels might fail to generate
	 a SIGSTOP notification in that case in response to our PTRACE_ATTACH
	 above.  Which would make the waitpid below wait forever.  So emulate
	 it.  Since there can only be one SIGSTOP notification pending this is
	 safe.  See also gdb/linux-nat.c linux_nat_post_attach_wait.  */
      syscall (__NR_tkill, tid, SIGSTOP);
      ptrace (PTRACE_CONT, tid, NULL, NULL);
    }
  for (;;)
    {
      int status;
      if (waitpid (tid, &status, __WALL) != tid || !WIFSTOPPED (status))
	{
	  int saved_errno = errno;
	  ptrace (PTRACE_DETACH, tid, NULL, NULL);
	  errno = saved_errno;
	  __libdwfl_seterrno (DWFL_E_ERRNO);
	  return false;
	}
      if (WSTOPSIG (status) == SIGSTOP)
	break;
      if (ptrace (PTRACE_CONT, tid, NULL,
		  (void *) (uintptr_t) WSTOPSIG (status)) != 0)
	{
	  int saved_errno = errno;
	  ptrace (PTRACE_DETACH, tid, NULL, NULL);
	  errno = saved_errno;
	  __libdwfl_seterrno (DWFL_E_ERRNO);
	  return false;
	}
    }
  return true;
}

static bool
pid_memory_read (Dwfl *dwfl, Dwarf_Addr addr, Dwarf_Word *result, void *arg)
{
  struct pid_arg *pid_arg = arg;
  pid_t tid = pid_arg->tid_attached;
  assert (tid > 0);
  Dwfl_Process *process = dwfl->process;
  if (ebl_get_elfclass (process->ebl) == ELFCLASS64)
    {
#if SIZEOF_LONG == 8
      errno = 0;
      *result = ptrace (PTRACE_PEEKDATA, tid, (void *) (uintptr_t) addr, NULL);
      return errno == 0;
#else /* SIZEOF_LONG != 8 */
      /* This should not happen.  */
      return false;
#endif /* SIZEOF_LONG != 8 */
    }
#if SIZEOF_LONG == 8
  /* We do not care about reads unaliged to 4 bytes boundary.
     But 0x...ffc read of 8 bytes could overrun a page.  */
  bool lowered = (addr & 4) != 0;
  if (lowered)
    addr -= 4;
#endif /* SIZEOF_LONG == 8 */
  errno = 0;
  *result = ptrace (PTRACE_PEEKDATA, tid, (void *) (uintptr_t) addr, NULL);
  if (errno != 0)
    return false;
#if SIZEOF_LONG == 8
# if BYTE_ORDER == BIG_ENDIAN
  if (! lowered)
    *result >>= 32;
# else
  if (lowered)
    *result >>= 32;
# endif
#endif /* SIZEOF_LONG == 8 */
  *result &= 0xffffffff;
  return true;
}

static pid_t
pid_next_thread (Dwfl *dwfl __attribute__ ((unused)), void *dwfl_arg,
		 void **thread_argp)
{
  struct pid_arg *pid_arg = dwfl_arg;
  struct dirent *dirent;
  /* Start fresh on first traversal. */
  if (*thread_argp == NULL)
    rewinddir (pid_arg->dir);
  do
    {
      errno = 0;
      dirent = readdir (pid_arg->dir);
      if (dirent == NULL)
	{
	  if (errno != 0)
	    {
	      __libdwfl_seterrno (DWFL_E_ERRNO);
	      return -1;
	    }
	  return 0;
	}
    }
  while (strcmp (dirent->d_name, ".") == 0
	 || strcmp (dirent->d_name, "..") == 0);
  char *end;
  errno = 0;
  long tidl = strtol (dirent->d_name, &end, 10);
  if (errno != 0)
    {
      __libdwfl_seterrno (DWFL_E_ERRNO);
      return -1;
    }
  pid_t tid = tidl;
  if (tidl <= 0 || (end && *end) || tid != tidl)
    {
      __libdwfl_seterrno (DWFL_E_PARSE_PROC);
      return -1;
    }
  *thread_argp = dwfl_arg;
  return tid;
}

/* Just checks that the thread id exists.  */
static bool
pid_getthread (Dwfl *dwfl __attribute__ ((unused)), pid_t tid,
	       void *dwfl_arg, void **thread_argp)
{
  *thread_argp = dwfl_arg;
  if (kill (tid, 0) < 0)
    {
      __libdwfl_seterrno (DWFL_E_ERRNO);
      return false;
    }
  return true;
}

/* Implement the ebl_set_initial_registers_tid setfunc callback.  */

static bool
pid_thread_state_registers_cb (int firstreg, unsigned nregs,
			       const Dwarf_Word *regs, void *arg)
{
  Dwfl_Thread *thread = (Dwfl_Thread *) arg;
  if (firstreg < 0)
    {
      assert (firstreg == -1);
      assert (nregs == 1);
      INTUSE(dwfl_thread_state_register_pc) (thread, *regs);
      return true;
    }
  assert (nregs > 0);
  return INTUSE(dwfl_thread_state_registers) (thread, firstreg, nregs, regs);
}

static bool
pid_set_initial_registers (Dwfl_Thread *thread, void *thread_arg)
{
  struct pid_arg *pid_arg = thread_arg;
  assert (pid_arg->tid_attached == 0);
  pid_t tid = INTUSE(dwfl_thread_tid) (thread);
  if (! pid_arg->assume_ptrace_stopped
      && ! ptrace_attach (tid, &pid_arg->tid_was_stopped))
    return false;
  pid_arg->tid_attached = tid;
  Dwfl_Process *process = thread->process;
  Ebl *ebl = process->ebl;
  return ebl_set_initial_registers_tid (ebl, tid,
					pid_thread_state_registers_cb, thread);
}

static void
pid_detach (Dwfl *dwfl __attribute__ ((unused)), void *dwfl_arg)
{
  struct pid_arg *pid_arg = dwfl_arg;
  closedir (pid_arg->dir);
  free (pid_arg);
}

static void
pid_thread_detach (Dwfl_Thread *thread, void *thread_arg)
{
  struct pid_arg *pid_arg = thread_arg;
  pid_t tid = INTUSE(dwfl_thread_tid) (thread);
  assert (pid_arg->tid_attached == tid);
  pid_arg->tid_attached = 0;
  if (! pid_arg->assume_ptrace_stopped)
    {
      /* This handling is needed only on older Linux kernels such as
         2.6.32-358.23.2.el6.ppc64.  Later kernels such as
         3.11.7-200.fc19.x86_64 remember the T (stopped) state
         themselves and no longer need to pass SIGSTOP during
         PTRACE_DETACH.  */
      ptrace (PTRACE_DETACH, tid, NULL,
	      (void *) (intptr_t) (pid_arg->tid_was_stopped ? SIGSTOP : 0));
    }
}

static const Dwfl_Thread_Callbacks pid_thread_callbacks =
{
  pid_next_thread,
  pid_getthread,
  pid_memory_read,
  pid_set_initial_registers,
  pid_detach,
  pid_thread_detach,
};

int
dwfl_linux_proc_attach (Dwfl *dwfl, pid_t pid, bool assume_ptrace_stopped)
{
  char buffer[36];
  FILE *procfile;

  /* Make sure to report the actual PID (thread group leader) to
     dwfl_attach_state.  */
  snprintf (buffer, sizeof (buffer), "/proc/%ld/status", (long) pid);
  procfile = fopen (buffer, "r");
  if (procfile == NULL)
    return errno;

  char *line = NULL;
  size_t linelen = 0;
  while (getline (&line, &linelen, procfile) >= 0)
    if (strncmp (line, "Tgid:", 5) == 0)
      {
	errno = 0;
	char *endptr;
	long val = strtol (&line[5], &endptr, 10);
	if ((errno == ERANGE && val == LONG_MAX)
	    || *endptr != '\n' || val < 0 || val != (pid_t) val)
	  pid = 0;
	else
	  pid = (pid_t) val;
	break;
      }
  free (line);
  fclose (procfile);

  if (pid == 0)
    return ESRCH;

  char dirname[64];
  int i = snprintf (dirname, sizeof (dirname), "/proc/%ld/task", (long) pid);
  assert (i > 0 && i < (ssize_t) sizeof (dirname) - 1);
  DIR *dir = opendir (dirname);
  if (dir == NULL)
    return errno;
  struct pid_arg *pid_arg = malloc (sizeof *pid_arg);
  if (pid_arg == NULL)
    {
      closedir (dir);
      return ENOMEM;
    }
  pid_arg->dir = dir;
  pid_arg->tid_attached = 0;
  pid_arg->assume_ptrace_stopped = assume_ptrace_stopped;
  if (! INTUSE(dwfl_attach_state) (dwfl, NULL, pid, &pid_thread_callbacks,
				   pid_arg))
    {
      closedir (dir);
      free (pid_arg);
      return -1;
    }
  return 0;
}
INTDEF (dwfl_linux_proc_attach)
