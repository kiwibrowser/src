/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/elf.h"
#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_startup.h"
#include "native_client/src/untrusted/nacl/tls.h"


/*
 * This is an arbitrary limit.  Usually there are 11 or fewer.
 */
#define MAX_PHNUM               16


#if 0
# define DEBUG_PRINTF(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
# define DEBUG_PRINTF(fmt, ...) ((void) 0)
#endif


static const char kInterpPrefixFlag[] = "--interp-prefix";

static bool g_explicit_filename;

static struct nacl_irt_resource_open resource_open;
static struct nacl_irt_code_data_alloc code_data_alloc;
static struct nacl_irt_dyncode dyncode;

static int open_program(const char *filename) {
  int fd;
  if (resource_open.open_resource == NULL)
    return open(filename, O_RDONLY);
  if (!g_explicit_filename) {
    /*
     * When we're loading everything via a manifest file, the full pathname
     * in the PT_INTERP is not really what we want.  Just its basename will
     * actually appear as a key in the manifest file.
     */
    const char *lastslash = strrchr(filename, '/');
    if (lastslash != NULL)
      filename = lastslash + 1;
  }
  int error = resource_open.open_resource(filename, &fd);
  if (error != 0) {
    errno = error;
    return -1;
  }
  return fd;
}

static void my_pread(const char *file, const char *fail_message,
                     int fd, void *buf, size_t bufsz,
                     uintptr_t pos, uintptr_t *last_pos) {
  DEBUG_PRINTF("XXX pread(%d, %#x, %#x, %#x)\n",
               fd, (uintptr_t) buf, bufsz, pos);
  if (pos != *last_pos && lseek(fd, pos, SEEK_SET) != pos) {
    fprintf(stderr, "%s: %s (lseek to %u: %s)\n",
            file, fail_message, pos, strerror(errno));
    exit(1);
  }
  ssize_t result = read(fd, buf, bufsz);
  if (result < 0) {
    fprintf(stderr, "%s: %s (read: %s)\n",
            file, fail_message, strerror(errno));
    exit(1);
  } else if ((size_t) result != bufsz) {
    fprintf(stderr, "%s: %s (short read: %u != %u)\n",
            file, fail_message, result, bufsz);
    exit(1);
  }
  *last_pos = pos + result;
}

static uintptr_t my_mmap(const char *file,
                         const char *segment_type, unsigned int segnum,
                         uintptr_t address, size_t size,
                         int prot, int flags, int fd, uintptr_t pos,
                         bool unreserved_code_address) {
  DEBUG_PRINTF("XXX %s %u mmap(%#x, %#x, %#x, %#x, %d, %#x)\n",
               segment_type, segnum, address, size, prot, flags, fd, pos);
  void *result = mmap((void *) address, size, prot, flags, fd, pos);
  if (result == MAP_FAILED) {
    /*
     * The service runtime will refuse to mmap a code segment from a file
     * descriptor that is not "blessed", e.g. if it comes from the web
     * cache rather than an installed Chrome extension.  In that case, we
     * have to read the data into a temporary buffer and then install it
     * using the dyncode_create interface.
     */
    if (errno == EINVAL && (prot & PROT_EXEC) && (flags & MAP_FIXED)) {
      DEBUG_PRINTF("XXX mmap %s %u failed, falling back to dyncode_create\n",
                   segment_type, segnum);
      void *data = mmap(NULL, size, PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE, -1, 0);
      if (data == MAP_FAILED) {
        fprintf(stderr,
                "%s: Failed to allocate buffer for %s %u (mmap: %s)\n",
                file, segment_type, segnum, strerror(errno));
        exit(1);
      }
      uintptr_t last_pos = -1;
      my_pread(file, "Failed to read code segment data", fd,
               data, size, pos, &last_pos);
      int error = dyncode.dyncode_create((void *) address, data, size);
      munmap(data, size);
      if (error) {
        fprintf(stderr, "%s: Failed to map %s %u (dyncode_create: %s)\n",
                file, segment_type, segnum, strerror(errno));
        exit(1);
      }
      if (unreserved_code_address) {
        /*
         * A successful PROT_EXEC mmap would have implicitly updated the
         * bookkeeping so that a future allocate_code_data call would
         * know that this range of the address space is already occupied.
         * That doesn't happen implicitly with dyncode_create, so it's
         * necessary to do an explicit call to update the bookkeeping.
         */
        uintptr_t allocated_address;
        error = code_data_alloc.allocate_code_data(address, size, 0, 0,
                                                   &allocated_address);
        if (error) {
          fprintf(stderr, "%s: Failed to map %s %u (allocate_code_data: %s)\n",
                  file, segment_type, segnum, strerror(errno));
          exit(1);
        }
        if (allocated_address != address) {
          fprintf(stderr,
                  "%s: Failed to map %s %u: allocate_code_data(%#" PRIxPTR
                  ", %#zx) yielded %#" PRIxPTR " instead!\n",
                  file, segment_type, segnum, address, size, allocated_address);
          exit(1);
        }
      }
      return address;
    }
    fprintf(stderr, "%s: Failed to map %s %u (mmap: %s)\n",
            file, segment_type, segnum, strerror(errno));
    exit(1);
  }
  return (uintptr_t) result;
}

static int prot_from_phdr(const Elf32_Phdr *phdr) {
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

static bool segment_contains(const Elf32_Phdr *phdr,
                             Elf32_Off offset, size_t filesz) {
  return (phdr->p_type == PT_LOAD &&
          offset >= phdr->p_offset &&
          offset - phdr->p_offset < phdr->p_filesz &&
          filesz <= phdr->p_filesz - (offset - phdr->p_offset));
}

static Elf32_Addr choose_load_bias(const char *filename, size_t pagesize,
                                   const Elf32_Phdr *first_load,
                                   const Elf32_Phdr *last_load,
                                   bool anywhere) {
  if (!anywhere)
    return 0;

  if (!(first_load->p_flags & PF_X)) {
    fprintf(stderr, "%s: First PT_LOAD segment is not executable!\n",
            filename);
    exit(1);
  }

  Elf32_Addr code_begin = round_down(first_load->p_vaddr, pagesize);
  Elf32_Addr code_end = round_up(first_load->p_vaddr + first_load->p_memsz,
                                 pagesize);

  do
    ++first_load;
  while (first_load < last_load && first_load->p_type != PT_LOAD);

  Elf32_Addr data_begin = round_down(first_load->p_vaddr, pagesize);
  Elf32_Addr data_end = round_up(last_load->p_vaddr + last_load->p_memsz,
                                 pagesize);

  size_t code_size = code_end - code_begin;
  uintptr_t data_offset = data_begin - code_begin;
  size_t data_size = data_end - data_begin;

  uintptr_t where;
  int error = code_data_alloc.allocate_code_data(code_begin, code_size,
                                                 data_offset, data_size,
                                                 &where);
  if (error) {
    fprintf(stderr,
            "%s: Cannot allocate address space (%#x, %#x, %#x, %#x): %s\n",
            filename, code_begin, code_size, data_offset, data_size,
            strerror(errno));
    exit(1);
  }

  DEBUG_PRINTF("XXX allocate_code_data(%#x, %#x, %#x, %#x) -> %#x\n",
               code_begin, code_size, data_offset, data_size, where);

  return where - code_begin;
}

/*
 * Open an ELF file and load it into memory.
 */
static Elf32_Addr load_elf_file(const char *filename,
                                size_t pagesize,
                                Elf32_Addr *out_base,
                                Elf32_Addr *out_phdr,
                                Elf32_Addr *out_phnum,
                                const char **out_interp) {
  int fd = open_program(filename);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %s\n", filename, strerror(errno));
    exit(2);
  }

  uintptr_t pread_pos = 0;
  Elf32_Ehdr ehdr;
  my_pread(filename, "Failed to read ELF header from file!  ",
           fd, &ehdr, sizeof(ehdr), 0, &pread_pos);

  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr.e_version != EV_CURRENT ||
      ehdr.e_ehsize != sizeof(ehdr) ||
      ehdr.e_phentsize != sizeof(Elf32_Phdr)) {
    fprintf(stderr, "%s has no valid ELF header!\n", filename);
    exit(1);
  }

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
      fprintf(stderr,
              "%s: ELF file has wrong architecture (e_machine=%u)\n",
              filename, ehdr.e_machine);
      exit(1);
  }

  Elf32_Phdr phdr[MAX_PHNUM];
  if (ehdr.e_phnum > sizeof(phdr) / sizeof(phdr[0]) || ehdr.e_phnum < 1) {
    fprintf(stderr, "%s: ELF file has unreasonable e_phnum=%u\n",
            filename, ehdr.e_phnum);
    exit(1);
  }

  bool anywhere;
  switch (ehdr.e_type) {
    case ET_EXEC:
      anywhere = false;
      break;
    case ET_DYN:
      anywhere = true;
      break;
    default:
      fprintf(stderr, "%s: ELF file has unexpected e_type=%u\n",
              filename, ehdr.e_type);
      exit(1);
  }

  my_pread(filename, "Failed to read program headers from ELF file!  ",
           fd, phdr, sizeof(phdr[0]) * ehdr.e_phnum, ehdr.e_phoff, &pread_pos);

  size_t i = 0;
  while (i < ehdr.e_phnum && phdr[i].p_type != PT_LOAD)
    ++i;
  if (i == ehdr.e_phnum) {
    fprintf(stderr, "%s: ELF file has no PT_LOAD header!", filename);
    exit(1);
  }

  /*
   * ELF requires that PT_LOAD segments be in ascending order of p_vaddr.
   * Find the last one to calculate the whole address span of the image.
   */
  const Elf32_Phdr *first_load = &phdr[i];
  const Elf32_Phdr *last_load = &phdr[ehdr.e_phnum - 1];
  while (last_load > first_load && last_load->p_type != PT_LOAD)
    --last_load;

  /*
   * For NaCl, the first load segment must always be the code segment.
   */
  if (first_load->p_flags != (PF_R | PF_X)) {
    fprintf(stderr, "%s: First PT_LOAD has p_flags=%#x (expecting RX=%#x)\n",
            filename, first_load->p_flags, PF_R | PF_X);
    exit(1);
  }
  if (first_load->p_filesz != first_load->p_memsz) {
    fprintf(stderr, "%s: Code segment has p_filesz %u != p_memsz %u\n",
            filename, first_load->p_filesz, first_load->p_memsz);
    exit(1);
  }

  /*
   * Decide where to load the image and reserve the portions of the address
   * space where it will reside.
   */
  Elf32_Addr load_bias = choose_load_bias(filename, pagesize,
                                          first_load, last_load, anywhere);
  DEBUG_PRINTF("XXX load_bias (%s) %#x\n",
               anywhere ? "anywhere" : "fixed",
               load_bias);

  /*
   * Map the code segment in.
   */
  my_mmap(filename, "code segment", first_load - phdr,
          load_bias + round_down(first_load->p_vaddr, pagesize),
          first_load->p_memsz, prot_from_phdr(first_load),
          MAP_PRIVATE | MAP_FIXED, fd,
          round_down(first_load->p_offset, pagesize),
          !anywhere);

  Elf32_Addr last_end = first_load->p_vaddr + load_bias + first_load->p_memsz;
  Elf32_Addr last_page_end = round_up(last_end, pagesize);

  /*
   * Map the remaining segments, and protect any holes between them.
   * The large hole after the code segment does not need to be
   * protected (and cannot be).  It covers the whole large tail of the
   * dynamic text area, which cannot be touched by mprotect.
   */
  const Elf32_Phdr *ph;
  for (ph = first_load + 1; ph <= last_load; ++ph) {
    if (ph->p_type == PT_LOAD) {
      Elf32_Addr start = round_down(ph->p_vaddr + load_bias, pagesize);

      if (start > last_page_end && ph > first_load + 1) {
        if (mprotect((void *) last_page_end, start - last_page_end,
                     PROT_NONE) != 0) {
          fprintf(stderr, "%s: Failed to mprotect segment %u hole! (%s)\n",
                  filename, ph - phdr, strerror(errno));
          exit(1);
        }
      }

      last_end = ph->p_vaddr + load_bias + ph->p_memsz;
      last_page_end = round_up(last_end, pagesize);
      Elf32_Addr map_end = last_page_end;

      /*
       * Unlike POSIX mmap, NaCl's mmap does not reliably handle COW
       * faults in the remainder of the final partial page.  So to get
       * the expected behavior for the unaligned boundary between data
       * and bss, it's necessary to allocate the final partial page of
       * data as anonymous memory rather than mapping it from the file.
       */
      Elf32_Addr file_end = ph->p_vaddr + load_bias + ph->p_filesz;
      if (ph->p_memsz > ph->p_filesz)
        map_end = round_down(file_end, pagesize);

      if (map_end > start) {
        my_mmap(filename, "segment", ph - phdr,
                start, map_end - start,
                prot_from_phdr(ph), MAP_PRIVATE | MAP_FIXED, fd,
                round_down(ph->p_offset, pagesize), false);
      }

      if (map_end < last_page_end) {
        /*
         * Handle the "bss" portion of a segment, where the memory size
         * exceeds the file size and we zero-fill the difference.  We map
         * anonymous pages for all the pages containing bss space.  Then,
         * if there is any partial-page tail of the file data, we read that
         * into the first such page.
         *
         * This scenario is invalid for an unwritable segment.
         */

        if ((ph->p_flags & PF_W) == 0) {
          fprintf(stderr,
                  "%s: Segment %u has p_memsz %u > p_filesz %u but no PF_W!\n",
                  filename, ph - phdr, ph->p_memsz, ph->p_filesz);
          exit(1);
        }

        my_mmap(filename, "bss segment", ph - phdr,
                map_end, last_page_end - map_end, prot_from_phdr(ph),
                MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0, false);

        if (file_end > map_end) {
          /*
           * There is a partial page of data to read in.
           */
          my_pread(filename, "Failed to read final partial page of data!  ",
                   fd, (void *) map_end, file_end - map_end,
                   round_down(ph->p_offset + ph->p_filesz, pagesize),
                   &pread_pos);
        }
      }
    }
  }

  /*
   * We've finished with the file now.
   */
  close(fd);

  /*
   * Find the PT_INTERP header, if there is one.
   */
  const Elf32_Phdr *interp = NULL;
  if (out_interp != NULL) {
    for (i = 0; i < ehdr.e_phnum; ++i) {
      if (phdr[i].p_type == PT_INTERP) {
        interp = &phdr[i];
        break;
      }
    }
  }

  /*
   * Find the PT_LOAD segments containing the PT_INTERP data and the phdrs.
   */
  for (ph = first_load;
       ph <= last_load && (interp != NULL || out_phdr != NULL);
       ++ph) {
    if (interp != NULL &&
        segment_contains(ph, interp->p_offset, interp->p_filesz)) {
      *out_interp = (const char *) (interp->p_vaddr + load_bias);
      interp = NULL;
    }
    if (out_phdr != NULL &&
        segment_contains(ph, ehdr.e_phoff, ehdr.e_phnum * sizeof(phdr[0]))) {
      *out_phdr = ehdr.e_phoff - ph->p_offset + ph->p_vaddr + load_bias;
      out_phdr = NULL;
    }
  }

  if (interp != NULL) {
    fprintf(stderr, "%s: PT_INTERP not within any PT_LOAD segment\n",
            filename);
    exit(1);
  }

  if (out_phdr != NULL) {
    *out_phdr = 0;
    fprintf(stderr,
            "Warning: %s: ELF program headers not within any PT_LOAD segment\n",
            filename);
  }

  if (out_phnum != NULL)
    *out_phnum = ehdr.e_phnum;

  if (out_base != NULL)
    *out_base = load_bias;

  return ehdr.e_entry + load_bias;
}

enum AuxvIndex {
  kPhdr,
  kPhent,
  kPhnum,
  kBase,
  kEntry,
  kSysinfo,
  kNull,
  kAuxvCount
};

__attribute__((noreturn))
static void chainload(const char *program, const char *interp_prefix,
                      int argc, char **argv, int envc, char **envp) {
  if (nacl_interface_query(NACL_IRT_RESOURCE_OPEN_v0_1, &resource_open,
                           sizeof(resource_open)) != sizeof(resource_open))
    resource_open.open_resource = NULL;
  if (nacl_interface_query(NACL_IRT_CODE_DATA_ALLOC_v0_1, &code_data_alloc,
                           sizeof(code_data_alloc)) !=
      sizeof(code_data_alloc)) {
    fprintf(stderr, "Failed to find necessary IRT interface %s!\n",
            NACL_IRT_CODE_DATA_ALLOC_v0_1);
    exit(1);
  }
  if (nacl_interface_query(NACL_IRT_DYNCODE_v0_1, &dyncode,
                           sizeof(dyncode)) != sizeof(dyncode)) {
    fprintf(stderr, "Failed to find necessary IRT interface %s!\n",
            NACL_IRT_DYNCODE_v0_1);
    exit(1);
  }

  const size_t pagesize = NACL_MAP_PAGESIZE;
  const TYPE_nacl_irt_query irt_query = __nacl_irt_query;

  /*
   * Populate our own info array on the stack.  We do not assume that the
   * argv and envp arrays are in their usual layout, since the caller could
   * be passing different values.
   */
  uint32_t info[NACL_STARTUP_ARGV + argc + 1 + envc + 1 + (kAuxvCount * 2)];
  info[NACL_STARTUP_FINI] = 0;
  info[NACL_STARTUP_ENVC] = envc;
  info[NACL_STARTUP_ARGC] = argc;
  memcpy(nacl_startup_argv(info), argv, (argc + 1) * sizeof(argv[0]));
  memcpy(nacl_startup_envp(info), envp, (envc + 1) * sizeof(envp[0]));
  Elf32_auxv_t *auxv = nacl_startup_auxv(info);

  /*
   * Populate the auxiliary vector with the values dynamic linkers expect.
   */
  auxv[kPhdr].a_type = AT_PHDR;
  auxv[kPhent].a_type = AT_PHENT;
  auxv[kPhent].a_un.a_val = sizeof(Elf32_Phdr);
  auxv[kPhnum].a_type = AT_PHNUM;
  auxv[kBase].a_type = AT_BASE;
  auxv[kEntry].a_type = AT_ENTRY;
  auxv[kSysinfo].a_type = AT_SYSINFO;
  auxv[kSysinfo].a_un.a_val = (uint32_t) irt_query;
  auxv[kNull].a_type = AT_NULL;
  auxv[kNull].a_un.a_val = 0;

  /*
   * Load the program and point the auxv elements at its phdrs and entry.
   */
  const char *interp = NULL;
  uint32_t entry = load_elf_file(program,
                                 pagesize,
                                 &auxv[kBase].a_un.a_val,
                                 &auxv[kPhdr].a_un.a_val,
                                 &auxv[kPhnum].a_un.a_val,
                                 &interp);
  auxv[kEntry].a_un.a_val = entry;

  if (auxv[kPhdr].a_un.a_val == 0)
    auxv[kPhdr].a_type = AT_IGNORE;

  DEBUG_PRINTF("XXX loaded %s, entry %#x, interp %s\n", program, entry, interp);

  if (interp != NULL) {
    /*
     * There was a PT_INTERP, so we have a dynamic linker to load.
     */

    char interp_buf[PATH_MAX];
    if (interp_prefix != NULL) {
      /*
       * Apply the command-line-specified prefix to the embedded file name.
       */
      snprintf(interp_buf, sizeof(interp_buf), "%s%s", interp_prefix, interp);
      interp = interp_buf;
    }

    entry = load_elf_file(interp, pagesize, NULL, NULL, NULL, NULL);

    DEBUG_PRINTF("XXX loaded PT_INTERP %s, entry %#x\n", interp, entry);
  }

  for (uint32_t *p = info; p < (uint32_t *) &auxv[kNull + 1]; ++p)
    DEBUG_PRINTF("XXX info[%d] = %#x\n", p - info, *p);

  /*
   * Off to the races!
   * The application's entry point should not return.  Crash if it does.
   */
  DEBUG_PRINTF("XXX user entry point: %#x(%#x)\n", entry, (uintptr_t) info);
  (*(void (*)(uint32_t[])) entry)(info);
  while (1)
    __builtin_trap();
}

int main(int argc, char **argv, char **envp) {
  const char *interp_prefix = NULL;
  const char *program = "main.nexe";

  if (argc > 2 && !strcmp(argv[1], kInterpPrefixFlag)) {
    interp_prefix = argv[2];
    argc -= 2;
    argv += 2;
  }

  if (argc > 1) {
    program = argv[1];
    g_explicit_filename = true;
    --argc;
    ++argv;
  }

  int envc = 0;
  for (char **ep = envp; *ep != NULL; ++ep)
    ++envc;

  chainload(program, interp_prefix, argc, argv, envc, envp);
  /*NOTREACHED*/
}
