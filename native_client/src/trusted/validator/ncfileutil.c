/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncfileutil.c - open an executable file. FOR TESTING ONLY.
 */

#include "native_client/src/trusted/validator/ncfileutil.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "native_client/src/include/portability_io.h"

/* This module is intended for testing use only, not for production use */
/* in sel_ldr. To prevent unintended production usage, define a symbol  */
/* that will cause a load-time error for sel_ldr.                       */
int gNaClValidateImage_foo = 0;
void NaClValidateImage(void) { gNaClValidateImage_foo += 1; }

static void NcLoadFilePrintError(const char* format, ...)
    ATTRIBUTE_FORMAT_PRINTF(1, 2);

/* Define the default print error function to use for this module. */
static void NcLoadFilePrintError(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/***********************************************************************/
/* THIS ROUTINE IS FOR DEBUGGING/TESTING ONLY, NOT FOR SECURE RUNTIME  */
/* ALL PAGES ARE LEFT WRITEABLE BY THIS LOADER.                        */
/***********************************************************************/
/* Loading a NC executable from a host file */
static off_t readat(ncfile* ncf, const int fd,
                    void *buf, const off_t sz, const off_t at) {
  /* TODO(karl) fix types for off_t and size_t so that the work for 64-bits */
  int sofar = 0;
  int nread;
  char *cbuf = (char *) buf;

  if (0 > lseek(fd, (long) at, SEEK_SET)) {
    ncf->error_fn("readat: lseek failed\n");
    return -1;
  }

  /* TODO(robertm) Figure out if O_BINARY flag fixes this. */
  /* Strangely this loop is needed on Windows. It seems the read()   */
  /* implementation doesn't always return as many bytes as requested */
  /* so you have to keep on trying.                                  */
  do {
    nread = read(fd, &cbuf[sofar], sz - sofar);
    if (nread <= 0) {
      ncf->error_fn("readat: read failed\n");
      return -1;
    }
    sofar += nread;
  } while (sz != sofar);
  return nread;
}

static const char* GetEiClassName(unsigned char c) {
  if (c == ELFCLASS32) {
    return "(32 bit executable)";
  } else if (c == ELFCLASS64) {
    return "(64 bit executable)";
  } else {
    return "(invalid class)";
  }
}

static int nc_load(ncfile *ncf, int fd) {
  union {
    Elf32_Ehdr h32;
#if NACL_BUILD_SUBARCH == 64
    Elf64_Ehdr h64;
#endif
  } h;
  Elf_Half phnum;
  Elf_Half shnum;
  Elf_Off phoff;
  Elf_Off shoff;
  ssize_t nread;
  Elf_Addr vmemlo, vmemhi;
  size_t shsize, phsize;
  int i;

  /* Read and check the ELF header */
  nread = readat(ncf, fd, &h, sizeof(h), 0);
  if (nread < 0 || (size_t) nread < sizeof(h)) {
    ncf->error_fn("nc_load(%s): could not read ELF header", ncf->fname);
    return -1;
  }

  /* do a bunch of sanity checks */
  if (memcmp(h.h32.e_ident, ELFMAG, SELFMAG)) {
    ncf->error_fn("nc_load(%s): bad magic number", ncf->fname);
    return -1;
  }

#if NACL_BUILD_SUBARCH == 64
  if (h.h32.e_ident[EI_CLASS] == ELFCLASS64) {
    if (h.h64.e_phoff > 0xffffffffU) {
      ncf->error_fn("nc_load(%s): e_phoff overflows 32 bits\n", ncf->fname);
      return -1;
    }
    if (h.h64.e_shoff > 0xffffffffU) {
      ncf->error_fn("nc_load(%s): e_shoff overflows 32 bits\n", ncf->fname);
      return -1;
    }
    phoff = (Elf32_Off) h.h64.e_phoff;
    shoff = (Elf32_Off) h.h64.e_shoff;
    phnum = h.h64.e_phnum;
    shnum = h.h64.e_shnum;
  } else
#endif
  {
    if (h.h32.e_ident[EI_CLASS] == ELFCLASS32) {
      phoff = h.h32.e_phoff;
      shoff = h.h32.e_shoff;
      phnum = h.h32.e_phnum;
      shnum = h.h32.e_shnum;
    } else {
      ncf->error_fn("nc_load(%s): bad EI CLASS %d %s\n", ncf->fname,
                    h.h32.e_ident[EI_CLASS],
                    GetEiClassName(h.h32.e_ident[EI_CLASS]));
      return -1;
    }
  }

  /* Read the program header table */
  if (phnum <= 0 || phnum > kMaxPhnum) {
    ncf->error_fn("nc_load(%s): e_phnum %d > kMaxPhnum %d\n",
                  ncf->fname, phnum, kMaxPhnum);
    return -1;
  }
  ncf->phnum = phnum;
  ncf->pheaders = (Elf_Phdr *)calloc(phnum, sizeof(Elf_Phdr));
  if (NULL == ncf->pheaders) {
    ncf->error_fn("nc_load(%s): calloc(%d, %"NACL_PRIuS") failed\n",
                  ncf->fname, phnum, sizeof(Elf_Phdr));
    return -1;
  }
  phsize = phnum * sizeof(*ncf->pheaders);
#if NACL_BUILD_SUBARCH == 64
  if (h.h32.e_ident[EI_CLASS] == ELFCLASS64) {
    /*
     * Read 64-bit program headers and convert them.
     */
    Elf64_Phdr phdr64[kMaxPhnum];
    nread = readat(ncf, fd, phdr64, (off_t) (phnum * sizeof(phdr64[0])),
                   (off_t) phoff);
    if (nread < 0 || (size_t) nread < phsize) return -1;
    for (i = 0; i < phnum; ++i) {
      if (phdr64[i].p_offset > 0xffffffffU ||
          phdr64[i].p_vaddr > 0xffffffffU ||
          phdr64[i].p_paddr > 0xffffffffU ||
          phdr64[i].p_filesz > 0xffffffffU ||
          phdr64[i].p_memsz > 0xffffffffU ||
          phdr64[i].p_align > 0xffffffffU) {
        ncf->error_fn("nc_load(%s): phdr[%d] fields overflow 32 bits\n",
                      ncf->fname, i);
        return -1;
      }
      ncf->pheaders[i].p_type = phdr64[i].p_type;
      ncf->pheaders[i].p_flags = phdr64[i].p_flags;
      ncf->pheaders[i].p_offset = (Elf32_Off) phdr64[i].p_offset;
      ncf->pheaders[i].p_vaddr = (Elf32_Addr) phdr64[i].p_vaddr;
      ncf->pheaders[i].p_paddr = (Elf32_Addr) phdr64[i].p_paddr;
      ncf->pheaders[i].p_filesz = (Elf32_Word) phdr64[i].p_filesz;
      ncf->pheaders[i].p_memsz = (Elf32_Word) phdr64[i].p_memsz;
      ncf->pheaders[i].p_align = (Elf32_Word) phdr64[i].p_align;
    }
  } else
#endif
  {
    /* TODO(karl) Remove the cast to size_t, or verify size. */
    nread = readat(ncf, fd, ncf->pheaders, (off_t) phsize, (off_t) phoff);
    if (nread < 0 || (size_t) nread < phsize) return -1;
  }

  /* Iterate through the program headers to find the virtual */
  /* size of loaded text.                                    */
  vmemlo = MAX_ELF_ADDR;
  vmemhi = MIN_ELF_ADDR;
  for (i = 0; i < phnum; i++) {
    if (ncf->pheaders[i].p_type != PT_LOAD) continue;
    if (0 == (ncf->pheaders[i].p_flags & PF_X)) continue;
    /* This is executable text. Check low and high addrs */
    if (vmemlo > ncf->pheaders[i].p_vaddr) vmemlo = ncf->pheaders[i].p_vaddr;
    if (vmemhi < ncf->pheaders[i].p_vaddr + ncf->pheaders[i].p_memsz) {
      vmemhi = ncf->pheaders[i].p_vaddr + ncf->pheaders[i].p_memsz;
    }
  }
  ncf->size = vmemhi - vmemlo;
  ncf->vbase = vmemlo;
  /* TODO(karl) Remove the cast to size_t, or verify size. */
  ncf->data = (uint8_t *)calloc(1, (size_t) ncf->size);
  if (NULL == ncf->data) {
    ncf->error_fn("nc_load(%s): calloc(1, %d) failed\n",
                  ncf->fname, (int)ncf->size);
    return -1;
  }

  /* Load program text segments */
  for (i = 0; i < phnum; i++) {
    const Elf_Phdr *p = &ncf->pheaders[i];
    if (p->p_type != PT_LOAD) continue;
    if (0 == (ncf->pheaders[i].p_flags & PF_X)) continue;

    /* TODO(karl) Remove the cast to off_t, or verify value in range. */
    nread = readat(ncf, fd, &(ncf->data[p->p_vaddr - ncf->vbase]),
                   (off_t) p->p_filesz, (off_t) p->p_offset);
    if (nread < 0 || (size_t) nread < p->p_filesz) {
      ncf->error_fn(
          "nc_load(%s): could not read segment %d (%d < %"
          NACL_PRIuElf_Xword")\n",
          ncf->fname, i, (int)nread, p->p_filesz);
      return -1;
    }
  }

  /* load the section headers */
  ncf->shnum = shnum;
  shsize = ncf->shnum * sizeof(*ncf->sheaders);
  ncf->sheaders = (Elf_Shdr *)calloc(1, shsize);
  if (NULL == ncf->sheaders) {
    ncf->error_fn("nc_load(%s): calloc(1, %"NACL_PRIuS") failed\n",
                  ncf->fname, shsize);
    return -1;
  }
#if NACL_BUILD_SUBARCH == 64
  if (h.h32.e_ident[EI_CLASS] == ELFCLASS64) {
    /*
     * Read 64-bit section headers and convert them.
     */
    Elf64_Shdr *shdr64 = (Elf64_Shdr *)calloc(shnum, sizeof(shdr64[0]));
    if (NULL == shdr64) {
      ncf->error_fn(
          "nc_load(%s): calloc(%"NACL_PRIdS", %"NACL_PRIdS") failed\n",
          ncf->fname, (size_t) shnum, sizeof(shdr64[0]));
      return -1;
    }
    shsize = ncf->shnum * sizeof(shdr64[0]);
    nread = readat(ncf, fd, shdr64, (off_t) shsize, (off_t) shoff);
    if (nread < 0 || (size_t) nread < shsize) {
      ncf->error_fn("nc_load(%s): could not read section headers\n",
                    ncf->fname);
      return -1;
    }
    for (i = 0; i < shnum; ++i) {
      if (shdr64[i].sh_flags > 0xffffffffU ||
          shdr64[i].sh_size > 0xffffffffU ||
          shdr64[i].sh_addralign > 0xffffffffU ||
          shdr64[i].sh_entsize > 0xffffffffU) {
        ncf->error_fn("nc_load(%s): shdr[%d] fields overflow 32 bits\n",
                      ncf->fname, i);
        return -1;
      }
      ncf->sheaders[i].sh_name = shdr64[i].sh_name;
      ncf->sheaders[i].sh_type = shdr64[i].sh_type;
      ncf->sheaders[i].sh_flags = (Elf32_Word) shdr64[i].sh_flags;
      ncf->sheaders[i].sh_addr = (Elf32_Addr) shdr64[i].sh_addr;
      ncf->sheaders[i].sh_offset = (Elf32_Off) shdr64[i].sh_offset;
      ncf->sheaders[i].sh_size = (Elf32_Word) shdr64[i].sh_size;
      ncf->sheaders[i].sh_link = shdr64[i].sh_link;
      ncf->sheaders[i].sh_info = shdr64[i].sh_info;
      ncf->sheaders[i].sh_addralign = (Elf32_Word) shdr64[i].sh_addralign;
      ncf->sheaders[i].sh_entsize = (Elf32_Word) shdr64[i].sh_entsize;
    }
    free(shdr64);
  } else
#endif
  {
    /* TODO(karl) Remove the cast to size_t, or verify value in range. */
    nread = readat(ncf, fd, ncf->sheaders, (off_t) shsize, (off_t) shoff);
    if (nread < 0 || (size_t) nread < shsize) {
      ncf->error_fn("nc_load(%s): could not read section headers\n",
                    ncf->fname);
      return -1;
    }
  }

  /* success! */
  return 0;
}

ncfile *nc_loadfile_depending(const char *filename,
                              nc_loadfile_error_fn error_fn) {
  ncfile *ncf;
  int fd;
  int rdflags = O_RDONLY | _O_BINARY;
  fd = OPEN(filename, rdflags);
  if (fd < 0) return NULL;

  /* Allocate the ncfile structure */
  ncf = calloc(1, sizeof(ncfile));
  if (ncf == NULL) return NULL;
  ncf->size = 0;
  ncf->data = NULL;
  ncf->fname = filename;
  if (error_fn == NULL) {
    ncf->error_fn = NcLoadFilePrintError;
  } else {
    ncf->error_fn = error_fn;
  }

  if (nc_load(ncf, fd) < 0) {
    close(fd);
    free(ncf);
    return NULL;
  }
  close(fd);
  return ncf;
}

ncfile *nc_loadfile(const char *filename) {
  return nc_loadfile_depending(filename, NULL);
}

ncfile *nc_loadfile_with_error_fn(const char *filename,
                                  nc_loadfile_error_fn error_fn) {
  return nc_loadfile_depending(filename, error_fn);
}


void nc_freefile(ncfile *ncf) {
  if (ncf->data != NULL) free(ncf->data);
  free(ncf);
}

/***********************************************************************/

void GetVBaseAndLimit(ncfile *ncf, NaClPcAddress *vbase,
                      NaClPcAddress *vlimit) {
  int ii;
  /* TODO(karl) - Define so constant applies to 64-bit pc address. */
  NaClPcAddress base = 0xffffffff;
  NaClPcAddress limit = 0;

  for (ii = 0; ii < ncf->shnum; ii++) {
    if ((ncf->sheaders[ii].sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR) {
      if (ncf->sheaders[ii].sh_addr < base) base = ncf->sheaders[ii].sh_addr;
      if (ncf->sheaders[ii].sh_addr + ncf->sheaders[ii].sh_size > limit)
        limit = ncf->sheaders[ii].sh_addr + ncf->sheaders[ii].sh_size;
    }
  }
  *vbase = base;
  *vlimit = limit;
}
