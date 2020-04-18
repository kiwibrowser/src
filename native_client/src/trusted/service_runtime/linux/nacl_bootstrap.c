/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This is a standalone program that loads and runs the dynamic linker.
 * This program itself must be linked statically.  To keep it small, it's
 * written to avoid all dependencies on libc and standard startup code.
 * Hence, this should be linked using -nostartfiles.  It must be compiled
 * with -fno-stack-protector to ensure the compiler won't emit code that
 * presumes some special setup has been done.
 *
 * On ARM, the compiler will emit calls to some libc functions, so we
 * cannot link with -nostdlib.  The functions it does use (memset and
 * __aeabi_* functions for integer division) are sufficiently small and
 * self-contained in ARM's libc.a that we don't have any problem using
 * the libc definitions though we aren't using the rest of libc or doing
 * any of the setup it might expect.
 */
#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <link.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#include "native_client/src/include/build_config.h"

/*
 * Get inline functions for system calls.
 */
static int my_errno;
#define SYS_ERRNO my_errno
#include "third_party/lss/linux_syscall_support.h"

#define MAX_PHNUM               12

typedef uintptr_t __attribute__((may_alias)) stack_val_t;

/*
 * These exact magic argument strings are recognized in check_r_debug_arg
 * and check_reserved_at_zero_arg, below. Requiring the arguments to have
 * those Xs as a template both simplifies our argument matching code and saves
 * us from having to reformat the whole stack to find space for a string longer
 * than the original argument.
 */
#define TEMPLATE_DIGITS "XXXXXXXXXXXXXXXX"
#define R_DEBUG_TEMPLATE_PREFIX "--r_debug=0x"
static const char kRDebugTemplate[] = R_DEBUG_TEMPLATE_PREFIX TEMPLATE_DIGITS;
static const size_t kRDebugPrefixLen = sizeof(R_DEBUG_TEMPLATE_PREFIX) - 1;

#define RESERVED_AT_ZERO_TEMPLATE_PREFIX "--reserved_at_zero=0x"
static const char kReservedAtZeroTemplate[] =
    RESERVED_AT_ZERO_TEMPLATE_PREFIX TEMPLATE_DIGITS;
static const size_t kReservedAtZeroPrefixLen =
    sizeof(RESERVED_AT_ZERO_TEMPLATE_PREFIX) - 1;
extern char RESERVE_TOP[];

extern char TEXT_START[];

/*
 * We're not using <string.h> functions here, to avoid dependencies.
 * In the x86 libc, even "simple" functions like memset and strlen can
 * depend on complex startup code, because in newer libc
 * implementations they are defined using STT_GNU_IFUNC.
 */

/*
 * Some GCC versions are so clever that they recognize these simple loops
 * as having the semantics of standard library functions and replace them
 * with calls.  That defeats the whole purpose, which is to avoid requiring
 * any C library at all.  Fortunately, this optimization can be disabled
 * for all (following) functions in the file via #pragma.
 */
#if (defined(__GNUC__) && !defined(__clang__) && \
     (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
# pragma GCC optimize("-fno-tree-loop-distribute-patterns")
#endif

static void my_bzero(void *buf, size_t n) {
  char *p = buf;
  while (n-- > 0)
    *p++ = 0;
}

static size_t my_strlen(const char *s) {
  size_t n = 0;
  while (*s++ != '\0')
    ++n;
  return n;
}

static int my_strcmp(const char *a, const char *b) {
  while (*a == *b) {
    if (*a == '\0')
      return 0;
    ++a;
    ++b;
  }
  return (int) (unsigned char) *a - (int) (unsigned char) *b;
}


/*
 * We're avoiding libc, so no printf.  The only nontrivial thing we need
 * is rendering numbers, which is, in fact, pretty trivial.
 * bufsz of course must be enough to hold INT_MIN in decimal.
 */
static void iov_int_string(int value, struct kernel_iovec *iov,
                           char *buf, size_t bufsz) {
  char *p = &buf[bufsz];
  int negative = value < 0;
  if (negative)
    value = -value;
  do {
    --p;
    *p = "0123456789"[value % 10];
    value /= 10;
  } while (value != 0);
  if (negative)
    *--p = '-';
  iov->iov_base = p;
  iov->iov_len = &buf[bufsz] - p;
}

#define STRING_IOV(string_constant, cond) \
  { (void *) string_constant, cond ? (sizeof(string_constant) - 1) : 0 }

__attribute__((noreturn)) static void fail(const char *filename,
                                           const char *message,
                                           const char *item1, int value1,
                                           const char *item2, int value2) {
  char valbuf1[32];
  char valbuf2[32];
  struct kernel_iovec iov[] = {
    STRING_IOV("bootstrap_helper: ", 1),
    { (void *) filename, my_strlen(filename) },
    STRING_IOV(": ", 1),
    { (void *) message, my_strlen(message) },
    { (void *) item1, item1 == NULL ? 0 : my_strlen(item1) },
    STRING_IOV("=", item1 != NULL),
    { NULL, 0 },                        /* iov[6] */
    STRING_IOV(", ", item1 != NULL && item2 != NULL),
    { (void *) item2, item2 == NULL ? 0 : my_strlen(item2) },
    STRING_IOV("=", item2 != NULL),
    { NULL, 0 },                        /* iov[10] */
    { "\n", 1 },
  };
  const int niov = sizeof(iov) / sizeof(iov[0]);

  if (item1 != NULL)
    iov_int_string(value1, &iov[6], valbuf1, sizeof(valbuf1));
  if (item2 != NULL)
    iov_int_string(value2, &iov[10], valbuf2, sizeof(valbuf2));

  sys_writev(2, iov, niov);
  sys_exit_group(2);
  while (1) *(volatile int *) 0 = 0;  /* Crash.  */
}


static int my_open(const char *file, int oflag) {
  int result = sys_open(file, oflag, 0);
  if (result < 0)
    fail(file, "Cannot open ELF file!  ", "errno", my_errno, NULL, 0);
  return result;
}

static void my_pread(const char *file, const char *fail_message,
                     int fd, void *buf, size_t bufsz, uintptr_t pos) {
  ssize_t result = sys_pread64(fd, buf, bufsz, pos);
  if (result < 0)
    fail(file, fail_message, "errno", my_errno, NULL, 0);
  if ((size_t) result != bufsz)
    fail(file, fail_message, "read count", result, NULL, 0);
}

static uintptr_t my_mmap_simple(uintptr_t address, size_t size,
                                int prot, int flags, int fd, uintptr_t pos) {
  void *result = sys_mmap((void *) address, size, prot, flags, fd, pos);
  return (uintptr_t) result;
}

static uintptr_t my_mmap(const char *file,
                         const char *segment_type, unsigned int segnum,
                         uintptr_t address, size_t size,
                         int prot, int flags, int fd, uintptr_t pos) {
  uintptr_t result = my_mmap_simple(address, size, prot, flags, fd, pos);
  if ((void *) result == MAP_FAILED)
    fail(file, "Failed to map segment!  ",
         segment_type, segnum, "errno", my_errno);
  return result;
}

static void my_mprotect(const char *file, unsigned int segnum,
                        uintptr_t address, size_t size, int prot) {
  if (sys_mprotect((void *) address, size, prot) < 0)
    fail(file, "Failed to mprotect segment hole!  ",
         "segment", segnum, "errno", my_errno);
}


static int prot_from_phdr(const ElfW(Phdr) *phdr) {
  int prot = 0;
  if (phdr->p_flags & PF_R)
    prot |= PROT_READ;
  if (phdr->p_flags & PF_W)
    prot |= PROT_WRITE;
  if (phdr->p_flags & PF_X)
    prot |= PROT_EXEC;
  return prot;
}

static uintptr_t round_up(uintptr_t value, uintptr_t size) {
  return (value + size - 1) & -size;
}

static uintptr_t round_down(uintptr_t value, uintptr_t size) {
  return value & -size;
}

/*
 * Handle the "bss" portion of a segment, where the memory size
 * exceeds the file size and we zero-fill the difference.  For any
 * whole pages in this region, we over-map anonymous pages.  For the
 * sub-page remainder, we zero-fill bytes directly.
 */
static void handle_bss(const char *file,
                       unsigned int segnum, const ElfW(Phdr) *ph,
                       ElfW(Addr) load_bias, size_t pagesize) {
  if (ph->p_memsz > ph->p_filesz) {
    ElfW(Addr) file_end = ph->p_vaddr + load_bias + ph->p_filesz;
    ElfW(Addr) file_page_end = round_up(file_end, pagesize);
    ElfW(Addr) page_end = round_up(ph->p_vaddr + load_bias +
                                   ph->p_memsz, pagesize);
    if (page_end > file_page_end)
      my_mmap(file, "bss segment", segnum,
              file_page_end, page_end - file_page_end,
              prot_from_phdr(ph), MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (file_page_end > file_end && (ph->p_flags & PF_W))
      my_bzero((void *) file_end, file_page_end - file_end);
  }
}

/*
 * Open an ELF file and load it into memory.
 */
static ElfW(Addr) load_elf_file(const char *filename,
                                size_t pagesize,
                                ElfW(Addr) *out_base,
                                ElfW(Addr) *out_phdr,
                                ElfW(Addr) *out_phnum,
                                const char **out_interp) {
  int fd = my_open(filename, O_RDONLY);

  ElfW(Ehdr) ehdr;
  my_pread(filename, "Failed to read ELF header from file!  ",
           fd, &ehdr, sizeof(ehdr), 0);

  if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
      ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
      ehdr.e_ident[EI_MAG3] != ELFMAG3 ||
      ehdr.e_version != EV_CURRENT ||
      ehdr.e_ehsize != sizeof(ehdr) ||
      ehdr.e_phentsize != sizeof(ElfW(Phdr)))
    fail(filename, "File has no valid ELF header!", NULL, 0, NULL, 0);

  switch (ehdr.e_machine) {
#if defined(__i386__)
    case EM_386:
#elif defined(__x86_64__)
    case EM_X86_64:
#elif defined(__arm__)
    case EM_ARM:
#elif defined(__mips__)
    case EM_MIPS:
#else
# error "Don't know the e_machine value for this architecture!"
#endif
      break;
    default:
      fail(filename, "ELF file has wrong architecture!  ",
           "e_machine", ehdr.e_machine, NULL, 0);
      break;
  }

  ElfW(Phdr) phdr[MAX_PHNUM];
  if (ehdr.e_phnum > sizeof(phdr) / sizeof(phdr[0]) || ehdr.e_phnum < 1)
    fail(filename, "ELF file has unreasonable ",
         "e_phnum", ehdr.e_phnum, NULL, 0);

  if (ehdr.e_type != ET_DYN)
    fail(filename, "ELF file not ET_DYN!  ",
         "e_type", ehdr.e_type, NULL, 0);

  my_pread(filename, "Failed to read program headers from ELF file!  ",
           fd, phdr, sizeof(phdr[0]) * ehdr.e_phnum, ehdr.e_phoff);

  size_t i = 0;
  while (i < ehdr.e_phnum && phdr[i].p_type != PT_LOAD)
    ++i;
  if (i == ehdr.e_phnum)
    fail(filename, "ELF file has no PT_LOAD header!",
         NULL, 0, NULL, 0);

  /*
   * ELF requires that PT_LOAD segments be in ascending order of p_vaddr.
   * Find the last one to calculate the whole address span of the image.
   */
  const ElfW(Phdr) *first_load = &phdr[i];
  const ElfW(Phdr) *last_load = &phdr[ehdr.e_phnum - 1];
  while (last_load > first_load && last_load->p_type != PT_LOAD)
    --last_load;

  size_t span = last_load->p_vaddr + last_load->p_memsz - first_load->p_vaddr;

  /*
   * Map the first segment and reserve the space used for the rest and
   * for holes between segments.
   */
  const uintptr_t mapping = my_mmap(filename, "segment", first_load - phdr,
                                    round_down(first_load->p_vaddr, pagesize),
                                    span, prot_from_phdr(first_load),
                                    MAP_PRIVATE, fd,
                                    round_down(first_load->p_offset, pagesize));

  const ElfW(Addr) load_bias = mapping - round_down(first_load->p_vaddr,
                                                    pagesize);

  if (first_load->p_offset > ehdr.e_phoff ||
      first_load->p_filesz < ehdr.e_phoff + (ehdr.e_phnum * sizeof(ElfW(Phdr))))
    fail(filename, "First load segment of ELF file does not contain phdrs!",
         NULL, 0, NULL, 0);

  handle_bss(filename, first_load - phdr, first_load, load_bias, pagesize);

  ElfW(Addr) last_end = first_load->p_vaddr + load_bias + first_load->p_memsz;

  /*
   * Map the remaining segments, and protect any holes between them.
   */
  const ElfW(Phdr) *ph;
  for (ph = first_load + 1; ph <= last_load; ++ph) {
    if (ph->p_type == PT_LOAD) {
      ElfW(Addr) last_page_end = round_up(last_end, pagesize);

      last_end = ph->p_vaddr + load_bias + ph->p_memsz;
      ElfW(Addr) start = round_down(ph->p_vaddr + load_bias, pagesize);
      ElfW(Addr) end = round_up(last_end, pagesize);

      if (start > last_page_end)
        my_mprotect(filename,
                    ph - phdr, last_page_end, start - last_page_end, PROT_NONE);

      my_mmap(filename, "segment", ph - phdr,
              start, end - start,
              prot_from_phdr(ph), MAP_PRIVATE | MAP_FIXED, fd,
              round_down(ph->p_offset, pagesize));

      handle_bss(filename, ph - phdr, ph, load_bias, pagesize);
    }
  }

  if (out_interp != NULL) {
    /*
     * Find the PT_INTERP header, if there is one.
     */
    for (i = 0; i < ehdr.e_phnum; ++i) {
      if (phdr[i].p_type == PT_INTERP) {
        /*
         * The PT_INTERP isn't really required to sit inside the first
         * (or any) load segment, though it normally does.  So we can
         * easily avoid an extra read in that case.
         */
        if (phdr[i].p_offset >= first_load->p_offset &&
            phdr[i].p_filesz <= first_load->p_filesz) {
          *out_interp = (const char *) (phdr[i].p_vaddr + load_bias);
        } else {
          static char interp_buffer[PATH_MAX + 1];
          if (phdr[i].p_filesz >= sizeof(interp_buffer)) {
            fail(filename, "ELF file has unreasonable PT_INTERP size!  ",
                 "segment", i, "p_filesz", phdr[i].p_filesz);
          }
          my_pread(filename, "Cannot read PT_INTERP segment contents!",
                   fd, interp_buffer, phdr[i].p_filesz, phdr[i].p_offset);
          *out_interp = interp_buffer;
        }
        break;
      }
    }
  }

  sys_close(fd);

  if (out_base != NULL)
    *out_base = load_bias;
  if (out_phdr != NULL)
    *out_phdr = (ehdr.e_phoff - first_load->p_offset +
                 first_load->p_vaddr + load_bias);
  if (out_phnum != NULL)
    *out_phnum = ehdr.e_phnum;

  return ehdr.e_entry + load_bias;
}

/*
 * Replace template digits with a fill value.  This function places the
 * bottom num_digits of the hex representation of fill into the string
 * pointed to by start.
 */
static void fill_in_template_digits(char *start, size_t num_digits,
                                    uintptr_t fill) {
  while (num_digits-- > 0) {
    start[num_digits] = "0123456789abcdef"[fill & 0xf];
    fill >>= 4;
  }
  if (fill != 0)
    fail("fill_in_template_digits",
         "fill has significant digits beyond num_digits", NULL, 0, NULL, 0);
}

/*
 * GDB looks for this symbol name when it cannot find PT_DYNAMIC->DT_DEBUG.
 * We don't have a PT_DYNAMIC, so it will find this.  Now all we have to do
 * is arrange for this space to be filled in with the dynamic linker's
 * _r_debug contents after they're initialized.  That way, attaching GDB to
 * this process or examining its core file will find the PIE we loaded, the
 * dynamic linker, and all the shared libraries, making debugging pleasant.
 */
#if !NACL_ANDROID
/* Android does not define r_debug in a public header file. */
struct r_debug _r_debug __attribute__((nocommon, section(".r_debug")));
#endif  /* !NACL_ANDROID */

/*
 * If the argument matches the kRDebugTemplate string, then replace
 * the 16 Xs with the hexadecimal address of our _r_debug variable.
 */
static int check_r_debug_arg(char *arg) {
  if (my_strcmp(arg, kRDebugTemplate) == 0) {
#if !NACL_ANDROID
    fill_in_template_digits(arg + kRDebugPrefixLen,
                            sizeof(TEMPLATE_DIGITS) - 1,
                            (uintptr_t) &_r_debug);
    return 1;
#endif  /* !NACL_ANDROID */
  }
  return 0;
}

/*
 * If the argument matches the kReservedAtZeroTemplate string, then replace
 * the 8 Xs with the hexadecimal representation of the amount of
 * prereserved memory.
 */
static int check_reserved_at_zero_arg(char *arg) {
  if (my_strcmp(arg, kReservedAtZeroTemplate) == 0) {
    fill_in_template_digits(arg + kReservedAtZeroPrefixLen,
                            sizeof(TEMPLATE_DIGITS) - 1,
                            (uintptr_t) RESERVE_TOP);
    return 1;
  }
  return 0;
}


static void ReserveBottomPages(size_t pagesize) {
  uintptr_t page_addr;
  uintptr_t mmap_rval;

  /*
   * Attempt to protect low memory from zero to the code start address.
   *
   * It is normal for mmap() calls to fail with EPERM if the indicated
   * page is less than vm.mmap_min_addr (see /proc/sys/vm/mmap_min_addr),
   * or with EACCES under SELinux if less than CONFIG_LSM_MMAP_MIN_ADDR
   * (64k).  Sometimes, mmap() calls may fail with EINVAL if the
   * starting address is 0.  Hence, we adaptively move the bottom of the
   * region up a page at a time until we succeed in getting a reservation.
   */
  for (page_addr = 0;
       page_addr < (uintptr_t) TEXT_START;
       page_addr += pagesize) {
    mmap_rval = my_mmap_simple(page_addr,
                               (uintptr_t) TEXT_START - page_addr,
                               PROT_NONE,
                               MAP_PRIVATE | MAP_FIXED |
                               MAP_ANONYMOUS | MAP_NORESERVE,
                               -1, 0);
    if (page_addr == mmap_rval) {
      /* Success; the pages are now protected. */
      break;
    } else if (MAP_FAILED == (void *) mmap_rval &&
               (EPERM == my_errno || EACCES == my_errno ||
                EINVAL == my_errno)) {
      /*
       * Normal; this is an invalid page for this process and
       * doesn't need to be protected. Continue with next page.
       */
    } else {
      fail("ReserveBottomPages", "NULL pointer guard page ",
           "errno", my_errno, "address", (int) page_addr);
    }
  }
}


/*
 * This is the main loading code.  It's called with the starting stack pointer.
 * This points to a sequence of pointer-size words:
 *      [0]             argc
 *      [1..argc]       argv[0..argc-1]
 *      [1+argc]        NULL
 *      [2+argc..]      envp[0..]
 *                      NULL
 *                      auxv[0].a_type
 *                      auxv[1].a_un.a_val
 *                      ...
 * It returns the dynamic linker's runtime entry point address, where
 * we should jump to.  This is called by the machine-dependent _start
 * code (below).  On return, it restores the original stack pointer
 * and jumps to this entry point.
 *
 * argv[0] is the uninteresting name of this bootstrap program.  argv[1] is
 * the real program file name we'll open, and also the argv[0] for that
 * program.  We need to modify argc, move argv[1..] back to the argv[0..]
 * position, and also examine and modify the auxiliary vector on the stack.
 */
ElfW(Addr) do_load(stack_val_t *stack) {
  size_t i;
  int argn;

  /*
   * First find the end of the auxiliary vector.
   */
  int argc = stack[0];
  char **argv = (char **) &stack[1];
  const char *program = argv[1];
  char **envp = &argv[argc + 1];
  char **ep = envp;
  while (*ep != NULL)
    ++ep;
  ElfW(auxv_t) *auxv = (ElfW(auxv_t) *) (ep + 1);
  ElfW(auxv_t) *av = auxv;
  while (av->a_type != AT_NULL)
    ++av;
  size_t stack_words = (stack_val_t *) (av + 1) - &stack[1];

  if (argc < 2)
    fail("Usage", "PROGRAM ARGS...", NULL, 0, NULL, 0);

  /*
   * Now move everything back to eat our original argv[0].  When we've done
   * that, envp and auxv will start one word back from where they were.
   */
  --argc;
  --envp;
  auxv = (ElfW(auxv_t) *) ep;
  stack[0] = argc;
  for (i = 1; i < stack_words; ++i)
    stack[i] = stack[i + 1];

  /*
   * If an argument is the kRDebugTemplate or kReservedAtZeroTemplate
   * string, then we'll modify that argument string in place to specify
   * the address of our _r_debug structure (for kRDebugTemplate) or the
   * amount of prereserved address space (for kReservedAtZeroTemplate).
   * We expect that the arguments matching the templates are the first
   * arguments provided.
   */
  for (argn = 1; argn < argc; ++argn) {
    if (!check_r_debug_arg(argv[argn]) &&
        !check_reserved_at_zero_arg(argv[argn]))
     break;
  }

  /*
   * Record the auxv entries that are specific to the file loaded.
   * The incoming entries point to our own static executable.
   */
  ElfW(auxv_t) *av_base = NULL;
  ElfW(auxv_t) *av_entry = NULL;
  ElfW(auxv_t) *av_phdr = NULL;
  ElfW(auxv_t) *av_phnum = NULL;
  size_t pagesize = 0;

  for (av = auxv;
       (av_base == NULL || av_entry == NULL || av_phdr == NULL ||
        av_phnum == NULL || pagesize == 0);
       ++av) {
    switch (av->a_type) {
      case AT_NULL:
        fail("startup", "\
Failed to find AT_BASE, AT_ENTRY, AT_PHDR, AT_PHNUM, or AT_PAGESZ!",
             NULL, 0, NULL, 0);
        /*NOTREACHED*/
        break;
      case AT_BASE:
        av_base = av;
        break;
      case AT_ENTRY:
        av_entry = av;
        break;
      case AT_PAGESZ:
        pagesize = av->a_un.a_val;
        break;
      case AT_PHDR:
        av_phdr = av;
        break;
      case AT_PHNUM:
        av_phnum = av;
        break;
    }
  }

  ReserveBottomPages(pagesize);

  /* Load the program and point the auxv elements at its phdrs and entry.  */
  const char *interp = NULL;
  av_entry->a_un.a_val = load_elf_file(program,
                                       pagesize,
                                       NULL,
                                       &av_phdr->a_un.a_val,
                                       &av_phnum->a_un.a_val,
                                       &interp);

  ElfW(Addr) entry = av_entry->a_un.a_val;

  if (interp != NULL) {
    /*
     * There was a PT_INTERP, so we have a dynamic linker to load.
     */
    entry = load_elf_file(interp, pagesize, &av_base->a_un.a_val,
                          NULL, NULL, NULL);
  }

  return entry;
}

/*
 * We have to define the actual entry point code (_start) in assembly for
 * each machine.  The kernel startup protocol is not compatible with the
 * normal C function calling convention.  Here, we call do_load (above)
 * using the normal C convention as per the ABI, with the starting stack
 * pointer as its argument; restore the original starting stack; and
 * finally, jump to the dynamic linker's entry point address.
 */
#if defined(__i386__)
asm(".pushsection \".text\",\"ax\",@progbits\n"
    ".globl _start\n"
    ".type _start,@function\n"
    "_start:\n"
    "xorl %ebp, %ebp\n"
    "movl %esp, %ebx\n"         /* Save starting SP in %ebx.  */
    "andl $-16, %esp\n"         /* Align the stack as per ABI.  */
    "pushl %ebx\n"              /* Argument: stack block.  */
    "call do_load\n"
    "movl %ebx, %esp\n"         /* Restore the saved SP.  */
    "jmp *%eax\n"               /* Jump to the entry point.  */
    ".popsection"
    );
#elif defined(__x86_64__)
asm(".pushsection \".text\",\"ax\",@progbits\n"
    ".globl _start\n"
    ".type _start,@function\n"
    "_start:\n"
    "xorq %rbp, %rbp\n"
    "movq %rsp, %rbx\n"         /* Save starting SP in %rbx.  */
    "andq $-16, %rsp\n"         /* Align the stack as per ABI.  */
    "movq %rbx, %rdi\n"         /* Argument: stack block.  */
    "call do_load\n"
    "movq %rbx, %rsp\n"         /* Restore the saved SP.  */
    "jmp *%rax\n"               /* Jump to the entry point.  */
    ".popsection"
    );
#elif defined(__arm__)
asm(".pushsection \".text\",\"ax\",%progbits\n"
    ".globl _start\n"
    ".type _start,#function\n"
    "_start:\n"
#if defined(__thumb2__)
    ".thumb\n"
    ".syntax unified\n"
#endif
    "mov fp, #0\n"
    "mov lr, #0\n"
    "mov r4, sp\n"              /* Save starting SP in r4.  */
    "mov r0, sp\n"              /* Argument: stack block.  */
    "bl do_load\n"
    "mov sp, r4\n"              /* Restore the saved SP.  */
    "blx r0\n"                  /* Jump to the entry point.  */
    ".popsection"
    );
#elif defined(__mips__)
asm(".pushsection \".text\",\"ax\",@progbits\n"
    ".globl _start\n"
    ".type _start,@function\n"
    "_start:\n"
    ".set noreorder\n"
    "addiu $fp, $zero, 0\n"
    "addiu $ra, $zero, 0\n"
    "addiu $s8, $sp,   0\n"     /* Save starting SP in s8.  */
    "addiu $a0, $sp,   0\n"
    "addiu $sp, $sp, -16\n"
    "jal   do_load\n"
    "nop\n"
    "addiu $sp, $s8,  0\n"      /* Restore the saved SP.  */
    "jr    $v0\n"               /* Jump to the entry point.  */
    "nop\n"
    ".popsection"
    );
#else
# error "Need stack-preserving _start code for this architecture!"
#endif

#if defined(__arm__)
/*
 * We may bring in __aeabi_* functions from libgcc that in turn
 * want to call raise.
 */
int raise(int sig) {
  return sys_kill(sys_getpid(), sig);
}
#endif
