/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This alters instructions for the IRT so that access to the TLS
 * point to the IRT's TLS.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf64.h"
#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator_ragel/validator.h"

static Bool g_verbose = FALSE;
static void *g_contents;
static size_t g_contents_size;
static uint8_t *g_code_start;
static uint32_t g_code_addr;
static int g_errors;
static int g_changes;

static uint32_t GetInsnAddr(const void *insn) {
  return (uint32_t) ((uint8_t *) insn - g_code_start) + g_code_addr;
}

static void ReportError(const void *insn, const char *message) {
  uint32_t insn_addr = GetInsnAddr(insn);

  fprintf(stderr, "%#x: %s\n", (unsigned int) insn_addr, message);
  ++g_errors;
}

static void EditArmCode(void *code, size_t code_length) {
  Elf32_Word *const words = code;
  Elf32_Word *const end = (void *) ((uint8_t *) code + code_length);
  Elf32_Word *insn;

  for (insn = words; insn < end; ++insn) {
    if (*insn == NACL_INSTR_ARM_LITERAL_POOL_HEAD) {
      if ((insn - words) % 4 != 0) {
        ReportError(insn, "Roadblock not at start of bundle");
      } else {
        /*
         * Ignore the rest of this bundle.
         */
        insn += 3;
      }
    } else if ((*insn & 0x0FFF0FFF) == 0x05990000) {
      /*
       * This is 'ldr REG, [r9, #0]'.
       * Turn it into 'ldr REG, [r9, #4]'.
       */
      *insn |= 4;
      ++g_changes;
    }
  }
}

static void EditMipsCode(void *code, size_t code_length) {
  Elf32_Word *const words = code;
  Elf32_Word *const end = (void *) ((uint8_t *) code + code_length);
  Elf32_Word *insn;

  for (insn = words; insn < end; ++insn) {
    if ((*insn & 0xFFE0FFFF) == 0x8f000000) {
      /*
       * This is 'lw $REG, 0($t8)'.
       * Turn it into 'lw $REG, 4($t8)'.
       */
      *insn |= 4;
      ++g_changes;
    }
  }
}

static Bool ConsiderOneInsn(const uint8_t *insn_begin, const uint8_t *insn_end,
                            uint32_t validation_info, void *data) {
  UNREFERENCED_PARAMETER(data);
  if (insn_begin[0] == 0x65) {  /* GS prefix */
    if (insn_end - insn_begin < 6) {
      ReportError(insn_begin, "Unexpected GS prefix");
    } else if (insn_end[-1] == 0 && insn_end[-2] == 0 &&
               insn_end[-3] == 0 && insn_end[-4] == 0) {
      /*
       * This is 'something %gs:0'.
       * Turn it into 'something %gs:4'.
       */
       ((uint8_t *) insn_end)[-4] = 4;
      ++g_changes;
    } else if (insn_end[-1] == 0 && insn_end[-2] == 0 &&
               insn_end[-3] == 0 && insn_end[-4] == 4) {
      /*
       * The instruction is already offset to the right location.
       * TODO(dyen): This case should be removed eventually once the IRT
       * is being properly compiled without any special TLS flags.
       */
      uint32_t insn_addr = GetInsnAddr(insn_begin);
      if (g_verbose) {
        printf("%#x: %%gs address already pointing to correct offset (4)\n",
               insn_addr);
      }
    } else {
      ReportError(insn_begin, "Unexpected %gs address");
    }
  }
  return (validation_info & (VALIDATION_ERRORS_MASK | BAD_JUMP_TARGET)) == 0;
}

static void EditX86_32Code(void *code, size_t code_length) {
  const NaClCPUFeaturesX86 *cpu_features = &kFullCPUIDFeatures;
  if (!ValidateChunkIA32(code, code_length,
                         CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                         cpu_features, &ConsiderOneInsn, NULL))
    ReportError(code, "Validation failed");
}

static void ReadInput(const char *filename) {
  struct stat st;
  size_t read_bytes;

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "fopen: %s: %s\n",
            filename, strerror(errno));
    exit(1);
  }

  if (fstat(fileno(fp), &st) < 0) {
    fprintf(stderr, "fstat: %s: %s\n",
            filename, strerror(errno));
    exit(1);
  }

  g_contents_size = st.st_size;
  g_contents = malloc(g_contents_size);
  if (g_contents == NULL) {
    fprintf(stderr, "Cannot allocate %u bytes: %s\n",
            (unsigned int) st.st_size, strerror(errno));
    exit(1);
  }

  read_bytes = fread(g_contents, 1, g_contents_size, fp);
  if (read_bytes != g_contents_size) {
    if (ferror(fp)) {
      fprintf(stderr, "fread: %s: %s\n",
              filename, strerror(errno));
    } else if (feof(fp)) {
      fprintf(stderr, "fread: %s: premature EOF\n",
              filename);
    } else {
      fprintf(stderr, "fread: %s: unexpected read count - %u != %u\n",
              filename, (unsigned int) read_bytes,
              (unsigned int) g_contents_size);
    }
    exit(1);
  }

  fclose(fp);
}

static void WriteOutput(const char *filename) {
  size_t nwrote;

  FILE *fp = fopen(filename, "wb+");
  if (fp == NULL) {
    fprintf(stderr, "fopen: %s: %s\n", filename, strerror(errno));
    exit(1);
  }

  nwrote = fwrite(g_contents, 1, g_contents_size, fp);
  if (nwrote != g_contents_size) {
    fprintf(stderr, "fwrite: %s: %s\n", filename, strerror(errno));
    exit(1);
  }

  fclose(fp);
}

static Bool EditTLSCode(const char *infile, uint32_t code_addr,
                        uint8_t *code_start, size_t code_length,
                        uint16_t e_machine) {
  g_code_addr = code_addr;
  g_code_start = code_start;

  switch (e_machine) {
    case EM_ARM:
      EditArmCode(g_code_start, code_length);
      break;
    case EM_386:
      EditX86_32Code(g_code_start, code_length);
      break;
    case EM_X86_64:
      if (g_verbose) {
        printf("%s: x86-64 ELF detected, no instructions changed\n",
               infile);
      }
      return TRUE;
    case EM_MIPS:
      EditMipsCode(g_code_start, code_length);
      return TRUE;
    default:
      fprintf(stderr, "%s: Unsupported e_machine %d\n",
              infile, e_machine);
      return FALSE;
  }

  if (g_verbose) {
    if (g_changes == 0) {
      printf("%s: Found no changes to make\n",
             infile);
    } else {
      printf("%s: %d instructions changed\n",
             infile, g_changes);
    }
  }

  return TRUE;
}

static Bool Process32BitFile(const char *infile) {
  const Elf32_Ehdr *ehdr;
  const Elf32_Phdr *phdr;
  int i;

  ehdr = g_contents;
  if (ehdr->e_phoff > g_contents_size) {
    fprintf(stderr, "%s: bogus e_phoff\n",
            infile);
    return FALSE;
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
    fprintf(stderr, "%s: not an ELFCLASS32 file\n",
            infile);
    return FALSE;
  }
  if (ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
    fprintf(stderr, "%s: wrong e_phentsize: %u\n",
            infile, ehdr->e_phentsize);
    return FALSE;
  }
  if (g_contents_size - ehdr->e_phoff < ehdr->e_phnum * sizeof(Elf32_Phdr)) {
    fprintf(stderr, "%s: bogus elf32 e_phnum\n",
            infile);
    return FALSE;
  }

  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    fprintf(stderr, "%s: not an ELFDATA2LSB file\n",
            infile);
    return FALSE;
  }

  phdr = (const void *) ((uint8_t *) g_contents + ehdr->e_phoff);
  for (i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type == PT_LOAD && (phdr[i].p_flags & PF_X) != 0)
      break;
    }
  if (i == ehdr->e_phnum) {
    fprintf(stderr, "%s: Could not find executable load segment!\n",
            infile);
    return FALSE;
  }

  if (phdr[i].p_offset > g_contents_size ||
      g_contents_size - phdr[i].p_offset < phdr[i].p_filesz) {
    fprintf(stderr, "%s: Program header %d has invalid offset or size!\n",
            infile, i);
    return FALSE;
  }

  return EditTLSCode(infile, phdr[i].p_vaddr,
                     (uint8_t *) g_contents + phdr[i].p_offset,
                     phdr[i].p_filesz, ehdr->e_machine);
}

static Bool Process64BitFile(const char *infile) {
  const Elf64_Ehdr *ehdr;
  const Elf64_Phdr *phdr;
  int i;

  ehdr = g_contents;
  if (ehdr->e_phoff > g_contents_size) {
    fprintf(stderr, "%s: bogus e_phoff\n",
            infile);
    return FALSE;
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
    fprintf(stderr, "%s: not an ELFCLASS64 file\n",
            infile);
    return FALSE;
  }
  if (ehdr->e_phentsize != sizeof(Elf64_Phdr)) {
    fprintf(stderr, "%s: wrong e_phentsize: %u\n",
            infile, ehdr->e_phentsize);
    return FALSE;
  }
  if (g_contents_size - ehdr->e_phoff < ehdr->e_phnum * sizeof(Elf64_Phdr)) {
    fprintf(stderr, "%s: bogus elf64 e_phnum\n",
            infile);
    return FALSE;
  }

  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    fprintf(stderr, "%s: not an ELFDATA2LSB file\n",
            infile);
    return FALSE;
  }

  phdr = (const void *) ((uint8_t *) g_contents + ehdr->e_phoff);
  for (i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type == PT_LOAD && (phdr[i].p_flags & PF_X) != 0)
      break;
    }
  if (i == ehdr->e_phnum) {
    fprintf(stderr, "%s: Could not find executable load segment!\n",
            infile);
    return FALSE;
  }

  if (phdr[i].p_offset > g_contents_size ||
      g_contents_size - phdr[i].p_offset < phdr[i].p_filesz) {
    fprintf(stderr, "%s: Program header %d has invalid offset or size!\n",
            infile, i);
    return FALSE;
  }

  return EditTLSCode(infile, (uint32_t) phdr[i].p_vaddr,
                     (uint8_t *) g_contents + phdr[i].p_offset,
                     (size_t) phdr[i].p_filesz, ehdr->e_machine);
}

int main(int argc, char **argv) {
  const char *infile;
  const char *outfile;
  int arg_iter;

  for (arg_iter = 1; arg_iter < argc; ++arg_iter) {
    if (argv[arg_iter][0] != '-')
      break;

    if (0 == strcmp(argv[arg_iter], "--verbose")) {
      g_verbose = TRUE;
    } else {
      fprintf(stderr, "Invalid option: %s", argv[arg_iter]);
      return 1;
    }
  }

  if (argc - arg_iter != 2) {
    fprintf(stderr, "Usage: %s [OPTIONS] INFILE OUTFILE\n", argv[0]);
    fprintf(stderr, "\nOPTIONS:\n");
    fprintf(stderr, "\t--verbose: Display verbose output messages\n");
    return 1;
  }

  infile = argv[arg_iter];
  outfile = argv[arg_iter + 1];

  ReadInput(infile);

  if (g_contents_size < SELFMAG) {
    fprintf(stderr, "%s: too short to be an ELF file\n",
            infile);
    return 1;
  }
  if (memcmp(g_contents, ELFMAG, SELFMAG) != 0) {
    fprintf(stderr, "%s: not an ELF file\n",
            infile);
    return 1;
  }

  /*
  * We will examine the header to figure out whether we are dealing
  * with a 32 bit or 64 bit executable file.
  */
  if (g_contents_size >= sizeof(Elf32_Ehdr) &&
      ((Elf32_Ehdr *) g_contents)->e_ident[EI_CLASS] == ELFCLASS32) {
    if (!Process32BitFile(infile)) {
      fprintf(stderr, "%s: Could not process 32 bit ELF file\n",
              infile);
      return 1;
    }
  } else if (g_contents_size >= sizeof(Elf64_Ehdr) &&
             ((Elf64_Ehdr *) g_contents)->e_ident[EI_CLASS] == ELFCLASS64) {
    if (!Process64BitFile(infile)) {
      fprintf(stderr, "%s: Could not process 64 bit ELF file\n",
              infile);
      return 1;
    }
  } else {
    fprintf(stderr, "%s: Invalid ELF file!\n",
            infile);
    return 1;
  }

  if (g_errors != 0)
    return 1;

  WriteOutput(outfile);
  return 0;
}
