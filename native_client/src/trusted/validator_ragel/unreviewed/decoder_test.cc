/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf64.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator_ragel/decoder.h"


/* TODO(shcherbina): Code that handles line breaks when printing an instruction
 * is now dead and should be eliminated eventually.
 */
const int INSN_WIDTH = 15; /* Counterpart of '--insn-width' objdump option. */


/* This is a copy of NaClLog from shared/platform/nacl_log.c to avoid
 * linking in code in NaCl shared code in the unreviewed/Makefile and be able to
 *  use CHECK().

 * TODO(khim): remove the copy of NaClLog implementation as soon as
 * unreviewed/Makefile is eliminated.
 */
void NaClLog(int detail_level, char const  *fmt, ...) {
  va_list ap;

  UNREFERENCED_PARAMETER(detail_level);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  exit(1);
}

static void CheckBounds(unsigned char *data, size_t data_size,
                        void *ptr, size_t inside_size) {
  CHECK(data <= (unsigned char *) ptr);
  CHECK((unsigned char *) ptr + inside_size <= data + data_size);
}

void ReadImage(const char *filename, uint8_t **result, size_t *result_size) {
  FILE *fp;
  uint8_t *data;
  size_t file_size;
  size_t got;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open input file: %s\n", filename);
    exit(1);
  }
  /* Find the file size. */
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  data = static_cast<uint8_t*>(malloc(file_size));
  if (data == NULL) {
    fprintf(stderr, "Unable to create memory image of input file: %s\n",
            filename);
    exit(1);
  }
  fseek(fp, 0, SEEK_SET);
  got = fread(data, 1, file_size, fp);
  if (got != file_size) {
    fprintf(stderr, "Unable to read data from input file: %s\n",
            filename);
    exit(1);
  }
  fclose(fp);

  *result = data;
  *result_size = file_size;
}

struct DecodeState {
  std::ostream *out_stream;
  uint8_t width;
  const uint8_t *offset;
  int ia32_mode;
};

const char *RegisterNameAsString(
    enum OperandName name,
    enum OperandFormat format,
    Bool rex) {
  /* There are not 16, but 20 8-bit registers: handle %ah/%ch/%dh/%bh case.  */
  if (!rex && format == OPERAND_FORMAT_8_BIT &&
      name >= NC_REG_RSP && name <= NC_REG_RDI) {
    static const char *kRegisterNames[NC_REG_RDI - NC_REG_RSP + 1] = {
      "ah", "ch", "dh", "bh"
    };
    return kRegisterNames[name - NC_REG_RSP];
  } else if (name <= NC_REG_R15 && format <= OPERAND_FORMATS_REGISTER_MAX) {
    static const char
        *kRegisterNames[NC_REG_R15 + 1][OPERAND_FORMATS_REGISTER_MAX] =
    {
      {   "al",   "ax",  "eax",  "rax", "st(0)", "mm0",  "xmm0",  "ymm0",
          "es",  "cr0",  "db0",  "tr0" },
      {   "cl",   "cx",  "ecx",  "rcx", "st(1)", "mm1",  "xmm1",  "ymm1",
          "cs",  "cr1",  "db1",  "tr1" },
      {   "dl",   "dx",  "edx",  "rdx", "st(2)", "mm2",  "xmm2",  "ymm2",
          "ss",  "cr2",  "db2",  "tr2" },
      {   "bl",   "bx",  "ebx",  "rbx", "st(3)", "mm3",  "xmm3",  "ymm3",
          "ds",  "cr3",  "db3",  "tr3" },
      {  "spl",   "sp",  "esp",  "rsp", "st(4)", "mm4",  "xmm4",  "ymm4",
          "fs",  "cr4",  "db4",  "tr4" },
      {  "bpl",   "bp",  "ebp",  "rbp", "st(5)", "mm5",  "xmm5",  "ymm5",
          "gs",  "cr5",  "db5",  "tr5" },
      {  "sil",   "si",  "esi",  "rsi", "st(6)", "mm6",  "xmm6",  "ymm6",
          NULL,  "cr6",  "db6",  "tr6" },
      {  "dil",   "di",  "edi",  "rdi", "st(7)", "mm7",  "xmm7",  "ymm7",
          NULL,  "cr7",  "db7",  "tr7" },
      {  "r8b",  "r8w",  "r8d",   "r8",    NULL,  NULL,  "xmm8",  "ymm8",
          NULL,  "cr8",  "db8",  "tr8" },
      {  "r9b",  "r9w",  "r9d",   "r9",    NULL,  NULL,  "xmm9",  "ymm9",
          NULL,  "cr9",  "db9",  "tr9" },
      { "r10b", "r10w", "r10d",  "r10",    NULL,  NULL, "xmm10", "ymm10",
          NULL, "cr10", "db10", "tr10" },
      { "r11b", "r11w", "r11d",  "r11",    NULL,  NULL, "xmm11", "ymm11",
          NULL, "cr11", "db11", "tr11" },
      { "r12b", "r12w", "r12d",  "r12",    NULL,  NULL, "xmm12", "ymm12",
          NULL, "cr12", "db12", "tr12" },
      { "r13b", "r13w", "r13d",  "r13",    NULL,  NULL, "xmm13", "ymm13",
          NULL, "cr13", "db13", "tr13" },
      { "r14b", "r14w", "r14d",  "r14",    NULL,  NULL, "xmm14", "ymm14",
          NULL, "cr14", "db14", "tr14" },
      { "r15b", "r15w", "r15d",  "r15",    NULL,  NULL, "xmm15", "ymm15",
          NULL, "cr15", "db15", "tr15" }
    };
    assert(kRegisterNames[name][format]);
    return kRegisterNames[name][format];
  } else {
    assert(FALSE);
    return NULL;
  }
}

Bool IsNameInList(const char *name, ...) {
  va_list value_list;

  va_start(value_list, name);

  for (;;) {
    const char *element = va_arg(value_list, const char*);
    if (element) {
      if (strcmp(name, element) == 0) {
        va_end(value_list);
        return TRUE;
      }
    } else {
      va_end(value_list);
      return FALSE;
    }
  }
}

int stream_printf(std::ostream& out_stream, const char *format, ...) {
  va_list ap;
  va_start(ap, format);

  char buf[1024];
  int result = vsprintf(buf, format, ap);
  out_stream << buf;
  return result;
}

#define printf(...) \
      stream_printf(*((struct DecodeState *)userdata)->out_stream, __VA_ARGS__)

void ProcessInstruction(const uint8_t *begin, const uint8_t *end,
                        struct Instruction *instruction, void *userdata) {
  const char *instruction_name = instruction->name;
  unsigned char operands_count = instruction->operands_count;
  unsigned char rex_prefix = instruction->prefix.rex;
  enum OperandName rm_index = instruction->rm.index;
  enum OperandName rm_base = instruction->rm.base;
  Bool data16_prefix = instruction->prefix.data16;
  const uint8_t *p;
  char delimeter = ' ';
  Bool print_rip = FALSE;
  Bool empty_rex_prefix_ok = FALSE;
  Bool spurious_rex_prefix = FALSE;
#define print_name(x) (printf((x)), shown_name += strlen((x)))
  size_t shown_name = 0;
  int i, operand_format;

  /*
   * objdump's logic WRT fwait is extremely convoluted.
   *
   * At first it looks like fwait is "eaten up" by the next instruction:
   *    0:9b 9b df 28             fildll (%rax)
   * but only if next instruction is x87 instruction:
   *    0:9b                      fwait
   *    1:9b                      fwait
   *    2:48 8b 00                mov    (%rax),%rax
   * and only if there are up to two fwait prefixes:
   *    0:9b                      fwait
   *    1:9b 9b df 28             fildll (%rax)
   * but if rex prefix is used it's separated:
   *    0:40                      rex
   *    1:9b df 28                fildll (%rax)
   * and then fwait prefix without rex is named "rex" (sic!):
   *    0:9b                      rex
   *    1:40                      rex
   *    2:9b df 28                fildll (%rax)
   * but not if there are two fwait prefixes before rex:
   *    0:9b                      fwait
   *    1:9b                      rex
   *    2:40                      rex
   *    3:9b df 28                fildll (%rax)
   *
   * It looks too much work to try to reproduce all that (especially since some
   * of these outputs are clearly incorrect) and instead we just do the minimal
   * change required to handle one case which is actually used in our tests.
   */
  if ((end == begin + 2) && ((begin[0] & 0xf0) == 0x40) && (begin[1] == 0x9b)) {
    printf("%*lx:\t%*x\trex%s%s%s%s%s\n",
           ((struct DecodeState *)userdata)->width,
           (long)(begin - (((struct DecodeState *)userdata)->offset)),
           INSN_WIDTH * -3,
           begin[0],
           begin[0] == 0x40 ? "" : ".",
           begin[0] & 0x08 ? "W" : "",
           begin[0] & 0x04 ? "R" : "",
           begin[0] & 0x02 ? "X" : "",
           begin[0] & 0x01 ? "B" : "");
    printf("%*lx:\t%*x\tfwait\n",
           ((struct DecodeState *)userdata)->width,
           (long)(begin + 1 - (((struct DecodeState *)userdata)->offset)),
           INSN_WIDTH * -3,
           begin[1]);
    return;
  }

  if ((data16_prefix) && (begin[0] == 0x66) && (!(rex_prefix & 0x08)) &&
      (IsNameInList(instruction_name,
                    "fbld", "fbstp", "fild", "fistp", "fld", "fstp", NULL))) {
    printf("%*lx:\t%*x\tdata16\n",
           ((struct DecodeState *)userdata)->width,
           (long)(begin - (((struct DecodeState *)userdata)->offset)),
           INSN_WIDTH * -3,
           begin[0]);
    data16_prefix = FALSE;
    ++begin;
  }
  printf("%*lx:\t", ((struct DecodeState *)userdata)->width,
                    (long)(begin - (((struct DecodeState *)userdata)->offset)));
  for (p = begin; p < begin + INSN_WIDTH; ++p) {
    if (p >= end)
      printf("   ");
    else
      printf("%02x ", *p);
  }
  printf("\t");
  /*
   * "pclmulqdq" has two-operand mnemonic names for "imm8" equal to 0x01, 0x01,
   * 0x10, and 0x11.  Objdump incorrectly mixes them up with 0x2 and 0x03.
   */
  if (!strcmp(instruction_name, "pclmulqdq")) {
    if (instruction->imm[0] < 0x04) {
      switch (instruction->imm[0]) {
        case 0x02: instruction_name = "pclmullqhqdq"; break;
        case 0x03: instruction_name = "pclmulhqhqdq"; break;
      }
      --operands_count;
    }
  }
  /*
   * "vpclmulqdq" has two-operand mnemonic names for "imm8" equal to 0x01, 0x01,
   * 0x10, and 0x11.  Objdump mixes them with 0x2 and 0x03.
   */
  if (!strcmp(instruction_name, "vpclmulqdq")) {
    if (instruction->imm[0] < 0x04) {
      switch (instruction->imm[0]) {
        case 0x02: instruction_name = "vpclmullqhqdq"; break;
        case 0x03: instruction_name = "vpclmulhqhqdq"; break;
      }
      --operands_count;
    }
  }
  if (rex_prefix &&
      (instruction->prefix.rex_b_spurious ||
       instruction->prefix.rex_x_spurious ||
       instruction->prefix.rex_r_spurious ||
       instruction->prefix.rex_w_spurious))
    spurious_rex_prefix = TRUE;
  if (operands_count > 0) {
    if (!((struct DecodeState *)userdata)->ia32_mode)
      for (i=0; i<operands_count; ++i)
        /*
         * Objdump mistakenly allows "lock" with "mov %crX,%rXX" only in ia32
         * mode.  It's perfectly valid in amd64, too, so instead of changing
         * the decoder we fix it here.
         */
        if (instruction->operands[i].format ==
            OPERAND_FORMAT_CONTROL_REGISTER &&
            *begin == 0xf0 &&
            !instruction->prefix.lock) {
          print_name("lock ");
          if (rex_prefix & 0x04) {
            if (!instruction->prefix.rex_b_spurious &&
                instruction->prefix.rex_r_spurious &&
                !instruction->prefix.rex_x_spurious &&
                !instruction->prefix.rex_w_spurious)
              spurious_rex_prefix = FALSE;
          } else {
            instruction->operands[i].name = static_cast<OperandName>(
                instruction->operands[i].name - 8);
          }
        }
  }
  /* Only few rare instructions show spurious REX.B in objdump.  */
  if (!spurious_rex_prefix && instruction->prefix.rex_b_spurious)
    if (IsNameInList(instruction_name,
                     "ja", "jae", "jbe", "jb", "je", "jg", "jge", "jle",
                     "jl", "jne", "jno", "jnp", "jns", "jo", "jp", "js",
                     "jecxz", "jrcxz", "loop", "loope", "loopne", NULL))
      spurious_rex_prefix = TRUE;
  /* Some instructions don't show spurious REX.B in objdump.  */
  if (spurious_rex_prefix &&
      instruction->prefix.rex_b_spurious &&
      !instruction->prefix.rex_r_spurious &&
      !instruction->prefix.rex_x_spurious &&
      !instruction->prefix.rex_w_spurious) {
    if (operands_count > 0)
      for (i=0; i<operands_count; ++i)
        if (instruction->operands[i].name == NC_REG_RM &&
            instruction->rm.disp_type != DISP64) {
          spurious_rex_prefix = FALSE;
          break;
        }
  }
  /* Some instructions don't show spurious REX.W in objdump.  */
  if (spurious_rex_prefix &&
      !instruction->prefix.rex_b_spurious &&
      !instruction->prefix.rex_r_spurious &&
      !instruction->prefix.rex_x_spurious &&
      instruction->prefix.rex_w_spurious) {
    if (IsNameInList(instruction_name,
                     "ja", "jae", "jbe", "jb", "je", "jg", "jge", "jle",
                     "jl", "jne", "jno", "jnp", "jns", "jo", "jp", "js",
                     "jecxz", "jrcxz", "loop", "loope", "loopne",
                     "call", "jmp", NULL) &&
        instruction->operands[0].name == NC_JMP_TO &&
        instruction->operands[0].format != OPERAND_FORMAT_8_BIT)
      spurious_rex_prefix = FALSE;
    /*
     * Both AMD manual and Intel manual agree that mov from general purpose
     * register to segment register has signature "mov Ew Sw", but objdump
     * insists on 32bit/64 bit.  This is clearly an error in objdump so we fix
     * it here and not in decoder.
     */
    if ((begin[0] >= 0x48) && (begin[0] <= 0x4f) && (begin[1] == 0x8e) &&
        operands_count >=2 && instruction->operands[1].name != NC_REG_RM)
      spurious_rex_prefix = FALSE;
  }

  /*
   * If REX.W is used to owerride data16 prefix you can not really claim
   * it's superfluous, but objdump shows it as such for the instructions
   * which usually don't react to REX.W ("in", "out", "push", etc).
   */
  if (!spurious_rex_prefix && (rex_prefix & 0x08) &&
      ((IsNameInList(instruction_name, "call", "jmp", "lcall", "ljmp", NULL) &&
        instruction->operands[0].name != NC_JMP_TO) ||
       IsNameInList(instruction_name,
                    "fldenvs", "fnstenvs", "fnsaves",
                    "frstors", "fsaves", "fstenvs",
                    "in", "ins", "out", "outs",
                    "popf", "push", "pushf", NULL)))
    spurious_rex_prefix = TRUE;

  if (instruction->prefix.lock && (begin[0] == 0xf0))
    print_name("lock ");
  if (instruction->prefix.repnz && (begin[0] == 0xf2))
    print_name("repnz ");
  if (instruction->prefix.repz && (begin[0] == 0xf3)) {
    /*
     * This prefix is "rep" for "ins", "lods", "movs", "outs", "stos". For other
     * instructions print "repz".
     */
    if (IsNameInList(instruction_name,
                     "ins", "lods", "movs", "outs", "stos", NULL))
      print_name("rep ");
    else
      print_name("repz ");
  }

  if (((data16_prefix) && (rex_prefix & 0x08)) &&
      !IsNameInList(instruction_name,
                    "bsf", "bsr", "fldenvs", "fnstenvs", "fnsaves", "frstors",
                    "fsaves", "fstenvs", "movbe", NULL)) {
    if ((end - begin) != 3 ||
        (begin[0] != 0x66) || ((begin[1] & 0x48) != 0x48) || (begin[2] != 0x90))
      print_name("data32 ");
  }

  if (instruction->prefix.lock && (begin[0] != 0xf0)) {
    print_name("lock ");
  }
  if (instruction->prefix.repnz && (begin[0] != 0xf2)) {
    print_name("repnz ");
  }
  if (instruction->prefix.repz && (begin[0] != 0xf3)) {
    /*
     * This prefix is "rep" for "ins", "lods", "movs", "outs", "stos". For other
     * instructions print "repz".
     */
    if (IsNameInList(instruction_name,
                     "ins", "lods", "movs", "outs", "stos", NULL))
      print_name("rep ");
    else
      print_name("repz ");
  }

  if (rex_prefix == 0x40) {
    if (operands_count > 0)
      for (i=0; i<operands_count; ++i)
        /*
         * "Empty" rex prefix (0x40) is used to select "sil"/"dil"/"spl"/"bpl".
         */
        if (instruction->operands[i].format == OPERAND_FORMAT_8_BIT &&
            instruction->operands[i].name <= NC_REG_RDI) {
          empty_rex_prefix_ok = TRUE;
        }
    if (!empty_rex_prefix_ok)
      print_name("rex ");
  } else if (spurious_rex_prefix) {
    print_name("rex.");
    if (rex_prefix & 0x08) {
      print_name("W");
    }
    if (rex_prefix & 0x04) {
      print_name("R");
    }
    if (rex_prefix & 0x02) {
      print_name("X");
    }
    if (rex_prefix & 0x01) {
      print_name("B");
    }
    print_name(" ");
  }

  printf("%s", instruction_name);
  shown_name += strlen(instruction_name);

  if (instruction->att_instruction_suffix) {
    if (!IsNameInList(instruction_name,
                      "nopw   0x0(%eax,%eax,1)",
                      "nopw   0x0(%rax,%rax,1)",
                      NULL)) {
      printf("%s", instruction->att_instruction_suffix);
      shown_name += strlen(instruction->att_instruction_suffix);
    }
  }

  if (strcmp(instruction_name, "mov") == 0 &&
      instruction->operands[1].name == NC_REG_IMM &&
      instruction->operands[1].format == OPERAND_FORMAT_64_BIT)
    print_name("abs");

  if (IsNameInList(instruction_name,
                   "ja", "jae", "jbe", "jb", "je", "jg", "jge", "jle",
                   "jl", "jne", "jno", "jnp", "jns", "jo", "jp", "js",
                   "jecxz", "jrcxz", "loop", "loope", "loopne", NULL)) {
    if (instruction->prefix.branch_not_taken)
      print_name(",pn");
    else if (instruction->prefix.branch_taken)
      print_name(",pt");
  }

#undef print_name
  if ((strcmp(instruction_name, "nop") != 0 || operands_count != 0) &&
      !IsNameInList(
        instruction_name,
        "fwait",
        "nopw   0x0(%eax,%eax,1)",
        "nopw   0x0(%rax,%rax,1)",
        "nopw %cs:0x0(%eax,%eax,1)",
        "nopw %cs:0x0(%rax,%rax,1)",
        "nopw   %cs:0x0(%eax,%eax,1)",
        "nopw   %cs:0x0(%rax,%rax,1)",
        "data32 nopw %cs:0x0(%eax,%eax,1)",
        "data32 nopw %cs:0x0(%rax,%rax,1)",
        "data32 data32 nopw %cs:0x0(%eax,%eax,1)",
        "data32 data32 nopw %cs:0x0(%rax,%rax,1)",
        "data32 data32 data32 nopw %cs:0x0(%eax,%eax,1)",
        "data32 data32 data32 nopw %cs:0x0(%rax,%rax,1)",
        "data32 data32 data32 data32 nopw %cs:0x0(%eax,%eax,1)",
        "data32 data32 data32 data32 nopw %cs:0x0(%rax,%rax,1)",
        "data32 data32 data32 data32 data32 nopw %cs:0x0(%eax,%eax,1)",
        "data32 data32 data32 data32 data32 nopw %cs:0x0(%rax,%rax,1)",
        NULL)) {
    while (shown_name < 6) {
      printf(" ");
      ++shown_name;
    }
    if (operands_count == 0)
      printf(" ");
  }
  for (i=operands_count-1; i>=0; --i) {
    printf("%c", delimeter);
    if (IsNameInList(instruction_name, "call", "jmp", "lcall", "ljmp", NULL) &&
        instruction->operands[i].name != NC_JMP_TO)
      printf("*");
    /*
     * Both AMD manual and Intel manual agree that mov from general purpose
     * register to segment register has signature "mov Ew Sw", but objdump
     * insists on 32bit/64 bit.  This is clearly an error in objdump so we fix
     * it here and not in decoder.
     */
    if ((begin[0] >= 0x48) && (begin[0] <= 0x4f) && (begin[1] == 0x8e) &&
        (instruction->operands[i].format == OPERAND_FORMAT_16_BIT)) {
      operand_format = OPERAND_FORMAT_64_BIT;
    } else if (((begin[0] == 0x8e) ||
       ((begin[0] >= 0x40) && (begin[0] <= 0x4f) && (begin[1] == 0x8e))) &&
        (instruction->operands[i].format == OPERAND_FORMAT_16_BIT)) {
      operand_format = OPERAND_FORMAT_32_BIT;
    } else {
      operand_format = instruction->operands[i].format;
    }
    switch (instruction->operands[i].name) {
      case NC_REG_RAX:
      case NC_REG_RCX:
      case NC_REG_RDX:
      case NC_REG_RBX:
      case NC_REG_RSP:
      case NC_REG_RBP:
      case NC_REG_RSI:
      case NC_REG_RDI:
      case  NC_REG_R8:
      case  NC_REG_R9:
      case NC_REG_R10:
      case NC_REG_R11:
      case NC_REG_R12:
      case NC_REG_R13:
      case NC_REG_R14:
      case NC_REG_R15:
        printf("%%%s", RegisterNameAsString(
            instruction->operands[i].name,
            static_cast<OperandFormat>(operand_format),
            static_cast<Bool>(rex_prefix != 0)));
        break;
      case NC_REG_ST:
        assert(operand_format == OPERAND_FORMAT_ST);
        printf("%%st");
        break;
      case NC_REG_RM:
        if (instruction->rm.disp_type != DISPNONE) {
          if ((instruction->rm.disp_type == DISP64) ||
              (instruction->rm.offset >= 0))
            printf("0x%"NACL_PRIx64, instruction->rm.offset);
          else
            printf("-0x%"NACL_PRIx64, -instruction->rm.offset);
        }
        if (((struct DecodeState *)userdata)->ia32_mode) {
          if ((rm_base != NC_NO_REG) ||
              (rm_index != NC_NO_REG) ||
              (instruction->rm.scale != 0))
            printf("(");
          if (rm_base != NC_NO_REG)
            printf("%%%s",
                   RegisterNameAsString(rm_base, OPERAND_FORMAT_32_BIT, FALSE));
          if (rm_index == NC_REG_RIZ) {
            if ((rm_base != NC_REG_RSP) || (instruction->rm.scale != 0))
              printf(",%%eiz,%d", 1 << instruction->rm.scale);
          } else if (rm_index != NC_NO_REG) {
            printf(",%%%s,%d",
                   RegisterNameAsString(rm_index, OPERAND_FORMAT_32_BIT, FALSE),
                   1 << instruction->rm.scale);
          }
          if ((rm_base != NC_NO_REG) ||
              (rm_index != NC_NO_REG) ||
              (instruction->rm.scale != 0))
            printf(")");
        } else {
          if ((rm_base != NC_NO_REG) ||
              (rm_index != NC_REG_RIZ) ||
              (instruction->rm.scale != 0))
            printf("(");
          if (rm_base == NC_REG_RIP) {
            printf("%%rip");
            print_rip = TRUE;
          } else if (rm_base != NC_NO_REG) {
            printf("%%%s",
                   RegisterNameAsString(rm_base, OPERAND_FORMAT_64_BIT, FALSE));
          }
          if (rm_index == NC_REG_RIZ) {
            if ((rm_base != NC_NO_REG &&
                 rm_base != NC_REG_RSP &&
                 rm_base != NC_REG_R12) ||
                instruction->rm.scale != 0)
              printf(",%%riz,%d",1 << instruction->rm.scale);
          } else if (rm_index != NC_NO_REG) {
            printf(",%%%s,%d",
                   RegisterNameAsString(rm_index, OPERAND_FORMAT_64_BIT, FALSE),
                   1 << instruction->rm.scale);
          }
          if ((rm_base != NC_NO_REG) ||
              (rm_index != NC_REG_RIZ) ||
              (instruction->rm.scale != 0))
            printf(")");
        }
        break;
      case NC_REG_IMM:
        printf("$0x%"NACL_PRIx64,instruction->imm[0]);
        break;
      case NC_REG_IMM2:
        printf("$0x%"NACL_PRIx64,instruction->imm[1]);
        break;
      case NC_REG_PORT_DX:
        printf("(%%dx)");
        break;
      case NC_REG_DS_RBX:
        if (((struct DecodeState *)userdata)->ia32_mode)
          printf("%%ds:(%%ebx)");
        else
          printf("%%ds:(%%rbx)");
        break;
      case NC_REG_ES_RDI:
        if (((struct DecodeState *)userdata)->ia32_mode)
          printf("%%es:(%%edi)");
        else
          printf("%%es:(%%rdi)");
        break;
      case NC_REG_DS_RSI:
        if (((struct DecodeState *)userdata)->ia32_mode)
          printf("%%ds:(%%esi)");
        else
          printf("%%ds:(%%rsi)");
        break;
      case NC_JMP_TO:
        if (instruction->operands[0].format == OPERAND_FORMAT_16_BIT)
          printf("0x%lx", (long)((end + instruction->rm.offset -
                         (((struct DecodeState *)userdata)->offset)) & 0xffff));
        else
          printf("0x%lx", (long)(end + instruction->rm.offset -
                                   (((struct DecodeState *)userdata)->offset)));
        break;
      case NC_REG_RIP:
      case NC_REG_RIZ:
      case NC_NO_REG:
        assert(FALSE);
    }
    delimeter = ',';
  }
  if (print_rip) {
    printf("        # 0x%8"NACL_PRIx64,
           (uint64_t) (end + instruction->rm.offset -
               (((struct DecodeState *)userdata)->offset)));
  }
  printf("\n");
  begin += INSN_WIDTH;
  while (begin < end) {
    printf("%*"NACL_PRIx64":\t", ((struct DecodeState *)userdata)->width,
           (uint64_t) (begin - (((struct DecodeState *)userdata)->offset)));
    for (p = begin; p < begin + INSN_WIDTH; ++p) {
      if (p >= end) {
        printf("\n");
        return;
      } else {
        printf("%02x ", *p);
      }
    }
    printf("\n");
    if (p >= end)
      return;
    begin += INSN_WIDTH;
  }
}

void ProcessError (const uint8_t *ptr, void *userdata) {
  printf("rejected at %"NACL_PRIx64" (byte 0x%02"NACL_PRIx32")\n",
         (uint64_t) (ptr - (((struct DecodeState *)userdata)->offset)),
         *ptr);
}

#undef printf

int DecodeFile(const char *filename, int repeat_count) {
  size_t data_size;
  uint8_t *data;
  int count;

  ReadImage(filename, &data, &data_size);
  if (data[4] == 1) {
    for (count = 0; count < repeat_count; ++count) {
      Elf32_Ehdr *header;
      int index;

      header = (Elf32_Ehdr *) data;
      CheckBounds(data, data_size, header, sizeof *header);
      assert(memcmp(header->e_ident, ELFMAG, strlen(ELFMAG)) == 0);

      for (index = 0; index < header->e_shnum; ++index) {
        Elf32_Shdr *section = (Elf32_Shdr *) (data + header->e_shoff +
                                                   header->e_shentsize * index);
        CheckBounds(data, data_size, section, sizeof *section);

        if ((section->sh_flags & SHF_EXECINSTR) != 0) {
          struct DecodeState state;
          int res;

          std::ostringstream result_stream;

          state.out_stream = &result_stream;
          state.ia32_mode = TRUE;
          state.offset = data + section->sh_offset - section->sh_addr;
          if (section->sh_size <= 0xfff) {
            state.width = 4;
          } else if (section->sh_size <= 0xfffffff) {
            state.width = 8;
          } else {
            state.width = 12;
          }
          CheckBounds(data, data_size,
                      data + section->sh_offset, section->sh_size);
          res = DecodeChunkIA32(data + section->sh_offset, section->sh_size,
                                ProcessInstruction, ProcessError, &state);
          printf("%s", result_stream.str().c_str());
          if (!res) {
            return FALSE;
          }
          return TRUE;
        }
      }
    }
  } else if (data[4] == 2) {
    for (count = 0; count < repeat_count; ++count) {
      Elf64_Ehdr *header;
      int index;

      header = (Elf64_Ehdr *) data;
      CheckBounds(data, data_size, header, sizeof *header);
      assert(memcmp(header->e_ident, ELFMAG, strlen(ELFMAG)) == 0);

      for (index = 0; index < header->e_shnum; ++index) {
        Elf64_Shdr *section = (Elf64_Shdr *) (data + header->e_shoff +
                                                   header->e_shentsize * index);
        CheckBounds(data, data_size, section, sizeof *section);

        if ((section->sh_flags & SHF_EXECINSTR) != 0) {
          struct DecodeState state;
          int res;

          std::ostringstream result_stream;

          state.out_stream = &result_stream;
          state.ia32_mode = FALSE;
          state.offset = data + section->sh_offset - section->sh_addr;
          if (section->sh_size <= 0xfff) {
            state.width = 4;
          } else if (section->sh_size <= 0xfffffff) {
            state.width = 8;
          } else if (section->sh_size <= 0xfffffffffffLL) {
            state.width = 12;
          } else {
            state.width = 16;
          }
          CheckBounds(data, data_size,
                      data + section->sh_offset, (size_t)section->sh_size);
          res = DecodeChunkAMD64(data + section->sh_offset,
                                 (size_t)section->sh_size,
                                 ProcessInstruction, ProcessError, &state);
          printf("%s", result_stream.str().c_str());
          if (!res) {
            return FALSE;
          }
          return TRUE;
        }
      }
    }
  } else {
    printf("Unknown ELF class: %s\n", filename);
    exit(1);
  }
  return 0;
}

extern "C"
DLLEXPORT
char *DisassembleChunk(const uint8_t *data, size_t size, int bitness) {
  static char buf[10000];
  struct DecodeState state;

  std::ostringstream result_stream;

  state.out_stream = &result_stream;
  state.ia32_mode = (bitness == 32);
  state.offset = data;

  if (bitness == 32)
    DecodeChunkIA32(data, size, ProcessInstruction, ProcessError, &state);
  else if (bitness == 64)
    DecodeChunkAMD64(data, size, ProcessInstruction, ProcessError, &state);
  else
    CHECK(false);

  CHECK(result_stream.str().length() < sizeof buf);
  strcpy(buf, result_stream.str().c_str());
  return buf;
}

int main(int argc, char **argv) {
  int index, initial_index = 1, repeat_count = 1;
  if (argc == 1) {
    printf("%s: no input files\n", argv[0]);
    exit(1);
  }
  if (!strcmp(argv[1], "--repeat"))
    repeat_count = atoi(argv[2]),
    initial_index += 2;
  for (index = initial_index; index < argc; ++index) {
    const char *filename = argv[index];
    int rc = DecodeFile(filename, repeat_count);
    if (!rc) {
      printf("file '%s' can not be fully decoded\n", filename);
      return 1;
    }
  }
  return 0;
}
