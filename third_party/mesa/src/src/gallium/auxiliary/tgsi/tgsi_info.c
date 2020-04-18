/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "tgsi_info.h"

#define NONE TGSI_OUTPUT_NONE
#define COMP TGSI_OUTPUT_COMPONENTWISE
#define REPL TGSI_OUTPUT_REPLICATE
#define CHAN TGSI_OUTPUT_CHAN_DEPENDENT
#define OTHR TGSI_OUTPUT_OTHER

static const struct tgsi_opcode_info opcode_info[TGSI_OPCODE_LAST] =
{
   { 1, 1, 0, 0, 0, 0, COMP, "ARL", TGSI_OPCODE_ARL },
   { 1, 1, 0, 0, 0, 0, COMP, "MOV", TGSI_OPCODE_MOV },
   { 1, 1, 0, 0, 0, 0, CHAN, "LIT", TGSI_OPCODE_LIT },
   { 1, 1, 0, 0, 0, 0, REPL, "RCP", TGSI_OPCODE_RCP },
   { 1, 1, 0, 0, 0, 0, REPL, "RSQ", TGSI_OPCODE_RSQ },
   { 1, 1, 0, 0, 0, 0, CHAN, "EXP", TGSI_OPCODE_EXP },
   { 1, 1, 0, 0, 0, 0, CHAN, "LOG", TGSI_OPCODE_LOG },
   { 1, 2, 0, 0, 0, 0, COMP, "MUL", TGSI_OPCODE_MUL },
   { 1, 2, 0, 0, 0, 0, COMP, "ADD", TGSI_OPCODE_ADD },
   { 1, 2, 0, 0, 0, 0, REPL, "DP3", TGSI_OPCODE_DP3 },
   { 1, 2, 0, 0, 0, 0, REPL, "DP4", TGSI_OPCODE_DP4 },
   { 1, 2, 0, 0, 0, 0, CHAN, "DST", TGSI_OPCODE_DST },
   { 1, 2, 0, 0, 0, 0, COMP, "MIN", TGSI_OPCODE_MIN },
   { 1, 2, 0, 0, 0, 0, COMP, "MAX", TGSI_OPCODE_MAX },
   { 1, 2, 0, 0, 0, 0, COMP, "SLT", TGSI_OPCODE_SLT },
   { 1, 2, 0, 0, 0, 0, COMP, "SGE", TGSI_OPCODE_SGE },
   { 1, 3, 0, 0, 0, 0, COMP, "MAD", TGSI_OPCODE_MAD },
   { 1, 2, 0, 0, 0, 0, COMP, "SUB", TGSI_OPCODE_SUB },
   { 1, 3, 0, 0, 0, 0, COMP, "LRP", TGSI_OPCODE_LRP },
   { 1, 3, 0, 0, 0, 0, COMP, "CND", TGSI_OPCODE_CND },
   { 0, 0, 0, 0, 0, 0, NONE, "", 20 },      /* removed */
   { 1, 3, 0, 0, 0, 0, REPL, "DP2A", TGSI_OPCODE_DP2A },
   { 0, 0, 0, 0, 0, 0, NONE, "", 22 },      /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "", 23 },      /* removed */
   { 1, 1, 0, 0, 0, 0, COMP, "FRC", TGSI_OPCODE_FRC },
   { 1, 3, 0, 0, 0, 0, COMP, "CLAMP", TGSI_OPCODE_CLAMP },
   { 1, 1, 0, 0, 0, 0, COMP, "FLR", TGSI_OPCODE_FLR },
   { 1, 1, 0, 0, 0, 0, COMP, "ROUND", TGSI_OPCODE_ROUND },
   { 1, 1, 0, 0, 0, 0, REPL, "EX2", TGSI_OPCODE_EX2 },
   { 1, 1, 0, 0, 0, 0, REPL, "LG2", TGSI_OPCODE_LG2 },
   { 1, 2, 0, 0, 0, 0, REPL, "POW", TGSI_OPCODE_POW },
   { 1, 2, 0, 0, 0, 0, COMP, "XPD", TGSI_OPCODE_XPD },
   { 0, 0, 0, 0, 0, 0, NONE, "", 32 },      /* removed */
   { 1, 1, 0, 0, 0, 0, COMP, "ABS", TGSI_OPCODE_ABS },
   { 1, 1, 0, 0, 0, 0, REPL, "RCC", TGSI_OPCODE_RCC },
   { 1, 2, 0, 0, 0, 0, REPL, "DPH", TGSI_OPCODE_DPH },
   { 1, 1, 0, 0, 0, 0, REPL, "COS", TGSI_OPCODE_COS },
   { 1, 1, 0, 0, 0, 0, COMP, "DDX", TGSI_OPCODE_DDX },
   { 1, 1, 0, 0, 0, 0, COMP, "DDY", TGSI_OPCODE_DDY },
   { 0, 0, 0, 0, 0, 0, NONE, "KILP", TGSI_OPCODE_KILP },
   { 1, 1, 0, 0, 0, 0, COMP, "PK2H", TGSI_OPCODE_PK2H },
   { 1, 1, 0, 0, 0, 0, COMP, "PK2US", TGSI_OPCODE_PK2US },
   { 1, 1, 0, 0, 0, 0, COMP, "PK4B", TGSI_OPCODE_PK4B },
   { 1, 1, 0, 0, 0, 0, COMP, "PK4UB", TGSI_OPCODE_PK4UB },
   { 1, 2, 0, 0, 0, 0, COMP, "RFL", TGSI_OPCODE_RFL },
   { 1, 2, 0, 0, 0, 0, COMP, "SEQ", TGSI_OPCODE_SEQ },
   { 1, 2, 0, 0, 0, 0, REPL, "SFL", TGSI_OPCODE_SFL },
   { 1, 2, 0, 0, 0, 0, COMP, "SGT", TGSI_OPCODE_SGT },
   { 1, 1, 0, 0, 0, 0, REPL, "SIN", TGSI_OPCODE_SIN },
   { 1, 2, 0, 0, 0, 0, COMP, "SLE", TGSI_OPCODE_SLE },
   { 1, 2, 0, 0, 0, 0, COMP, "SNE", TGSI_OPCODE_SNE },
   { 1, 2, 0, 0, 0, 0, REPL, "STR", TGSI_OPCODE_STR },
   { 1, 2, 1, 0, 0, 0, OTHR, "TEX", TGSI_OPCODE_TEX },
   { 1, 4, 1, 0, 0, 0, OTHR, "TXD", TGSI_OPCODE_TXD },
   { 1, 2, 1, 0, 0, 0, OTHR, "TXP", TGSI_OPCODE_TXP },
   { 1, 1, 0, 0, 0, 0, COMP, "UP2H", TGSI_OPCODE_UP2H },
   { 1, 1, 0, 0, 0, 0, COMP, "UP2US", TGSI_OPCODE_UP2US },
   { 1, 1, 0, 0, 0, 0, COMP, "UP4B", TGSI_OPCODE_UP4B },
   { 1, 1, 0, 0, 0, 0, COMP, "UP4UB", TGSI_OPCODE_UP4UB },
   { 1, 3, 0, 0, 0, 0, COMP, "X2D", TGSI_OPCODE_X2D },
   { 1, 1, 0, 0, 0, 0, COMP, "ARA", TGSI_OPCODE_ARA },
   { 1, 1, 0, 0, 0, 0, COMP, "ARR", TGSI_OPCODE_ARR },
   { 0, 1, 0, 0, 0, 0, NONE, "BRA", TGSI_OPCODE_BRA },
   { 0, 0, 0, 1, 0, 0, NONE, "CAL", TGSI_OPCODE_CAL },
   { 0, 0, 0, 0, 0, 0, NONE, "RET", TGSI_OPCODE_RET },
   { 1, 1, 0, 0, 0, 0, COMP, "SSG", TGSI_OPCODE_SSG },
   { 1, 3, 0, 0, 0, 0, COMP, "CMP", TGSI_OPCODE_CMP },
   { 1, 1, 0, 0, 0, 0, CHAN, "SCS", TGSI_OPCODE_SCS },
   { 1, 2, 1, 0, 0, 0, OTHR, "TXB", TGSI_OPCODE_TXB },
   { 1, 1, 0, 0, 0, 0, COMP, "NRM", TGSI_OPCODE_NRM },
   { 1, 2, 0, 0, 0, 0, COMP, "DIV", TGSI_OPCODE_DIV },
   { 1, 2, 0, 0, 0, 0, REPL, "DP2", TGSI_OPCODE_DP2 },
   { 1, 2, 1, 0, 0, 0, OTHR, "TXL", TGSI_OPCODE_TXL },
   { 0, 0, 0, 0, 0, 0, NONE, "BRK", TGSI_OPCODE_BRK },
   { 0, 1, 0, 1, 0, 1, NONE, "IF", TGSI_OPCODE_IF },
   { 1, 1, 0, 0, 0, 1, NONE, "", 75 },      /* removed */
   { 0, 1, 0, 0, 0, 1, NONE, "", 76 },      /* removed */
   { 0, 0, 0, 1, 1, 1, NONE, "ELSE", TGSI_OPCODE_ELSE },
   { 0, 0, 0, 0, 1, 0, NONE, "ENDIF", TGSI_OPCODE_ENDIF },
   { 1, 0, 0, 0, 1, 0, NONE, "", 79 },      /* removed */
   { 0, 0, 0, 0, 1, 0, NONE, "", 80 },      /* removed */
   { 0, 1, 0, 0, 0, 0, NONE, "PUSHA", TGSI_OPCODE_PUSHA },
   { 1, 0, 0, 0, 0, 0, NONE, "POPA", TGSI_OPCODE_POPA },
   { 1, 1, 0, 0, 0, 0, COMP, "CEIL", TGSI_OPCODE_CEIL },
   { 1, 1, 0, 0, 0, 0, COMP, "I2F", TGSI_OPCODE_I2F },
   { 1, 1, 0, 0, 0, 0, COMP, "NOT", TGSI_OPCODE_NOT },
   { 1, 1, 0, 0, 0, 0, COMP, "TRUNC", TGSI_OPCODE_TRUNC },
   { 1, 2, 0, 0, 0, 0, COMP, "SHL", TGSI_OPCODE_SHL },
   { 0, 0, 0, 0, 0, 0, NONE, "", 88 },      /* removed */
   { 1, 2, 0, 0, 0, 0, COMP, "AND", TGSI_OPCODE_AND },
   { 1, 2, 0, 0, 0, 0, COMP, "OR", TGSI_OPCODE_OR },
   { 1, 2, 0, 0, 0, 0, COMP, "MOD", TGSI_OPCODE_MOD },
   { 1, 2, 0, 0, 0, 0, COMP, "XOR", TGSI_OPCODE_XOR },
   { 1, 3, 0, 0, 0, 0, COMP, "SAD", TGSI_OPCODE_SAD },
   { 1, 2, 1, 0, 0, 0, OTHR, "TXF", TGSI_OPCODE_TXF },
   { 1, 2, 1, 0, 0, 0, OTHR, "TXQ", TGSI_OPCODE_TXQ },
   { 0, 0, 0, 0, 0, 0, NONE, "CONT", TGSI_OPCODE_CONT },
   { 0, 0, 0, 0, 0, 0, NONE, "EMIT", TGSI_OPCODE_EMIT },
   { 0, 0, 0, 0, 0, 0, NONE, "ENDPRIM", TGSI_OPCODE_ENDPRIM },
   { 0, 0, 0, 1, 0, 1, NONE, "BGNLOOP", TGSI_OPCODE_BGNLOOP },
   { 0, 0, 0, 0, 0, 1, NONE, "BGNSUB", TGSI_OPCODE_BGNSUB },
   { 0, 0, 0, 1, 1, 0, NONE, "ENDLOOP", TGSI_OPCODE_ENDLOOP },
   { 0, 0, 0, 0, 1, 0, NONE, "ENDSUB", TGSI_OPCODE_ENDSUB },
   { 1, 1, 1, 0, 0, 0, OTHR, "TXQ_LZ", TGSI_OPCODE_TXQ_LZ },
   { 0, 0, 0, 0, 0, 0, NONE, "", 104 },     /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "", 105 },     /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "", 106 },     /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "NOP", TGSI_OPCODE_NOP },
   { 0, 0, 0, 0, 0, 0, NONE, "", 108 },     /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "", 109 },     /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "", 110 },     /* removed */
   { 0, 0, 0, 0, 0, 0, NONE, "", 111 },     /* removed */
   { 1, 1, 0, 0, 0, 0, REPL, "NRM4", TGSI_OPCODE_NRM4 },
   { 0, 1, 0, 0, 0, 0, NONE, "CALLNZ", TGSI_OPCODE_CALLNZ },
   { 0, 1, 0, 0, 0, 0, NONE, "IFC", TGSI_OPCODE_IFC },
   { 0, 1, 0, 0, 0, 0, NONE, "BREAKC", TGSI_OPCODE_BREAKC },
   { 0, 1, 0, 0, 0, 0, NONE, "KIL", TGSI_OPCODE_KIL },
   { 0, 0, 0, 0, 0, 0, NONE, "END", TGSI_OPCODE_END },
   { 0, 0, 0, 0, 0, 0, NONE, "", 118 },     /* removed */
   { 1, 1, 0, 0, 0, 0, COMP, "F2I", TGSI_OPCODE_F2I },
   { 1, 2, 0, 0, 0, 0, COMP, "IDIV", TGSI_OPCODE_IDIV },
   { 1, 2, 0, 0, 0, 0, COMP, "IMAX", TGSI_OPCODE_IMAX },
   { 1, 2, 0, 0, 0, 0, COMP, "IMIN", TGSI_OPCODE_IMIN },
   { 1, 1, 0, 0, 0, 0, COMP, "INEG", TGSI_OPCODE_INEG },
   { 1, 2, 0, 0, 0, 0, COMP, "ISGE", TGSI_OPCODE_ISGE },
   { 1, 2, 0, 0, 0, 0, COMP, "ISHR", TGSI_OPCODE_ISHR },
   { 1, 2, 0, 0, 0, 0, COMP, "ISLT", TGSI_OPCODE_ISLT },
   { 1, 1, 0, 0, 0, 0, COMP, "F2U", TGSI_OPCODE_F2U },
   { 1, 1, 0, 0, 0, 0, COMP, "U2F", TGSI_OPCODE_U2F },
   { 1, 2, 0, 0, 0, 0, COMP, "UADD", TGSI_OPCODE_UADD },
   { 1, 2, 0, 0, 0, 0, COMP, "UDIV", TGSI_OPCODE_UDIV },
   { 1, 3, 0, 0, 0, 0, COMP, "UMAD", TGSI_OPCODE_UMAD },
   { 1, 2, 0, 0, 0, 0, COMP, "UMAX", TGSI_OPCODE_UMAX },
   { 1, 2, 0, 0, 0, 0, COMP, "UMIN", TGSI_OPCODE_UMIN },
   { 1, 2, 0, 0, 0, 0, COMP, "UMOD", TGSI_OPCODE_UMOD },
   { 1, 2, 0, 0, 0, 0, COMP, "UMUL", TGSI_OPCODE_UMUL },
   { 1, 2, 0, 0, 0, 0, COMP, "USEQ", TGSI_OPCODE_USEQ },
   { 1, 2, 0, 0, 0, 0, COMP, "USGE", TGSI_OPCODE_USGE },
   { 1, 2, 0, 0, 0, 0, COMP, "USHR", TGSI_OPCODE_USHR },
   { 1, 2, 0, 0, 0, 0, COMP, "USLT", TGSI_OPCODE_USLT },
   { 1, 2, 0, 0, 0, 0, COMP, "USNE", TGSI_OPCODE_USNE },
   { 0, 1, 0, 0, 0, 0, NONE, "SWITCH", TGSI_OPCODE_SWITCH },
   { 0, 1, 0, 0, 0, 0, NONE, "CASE", TGSI_OPCODE_CASE },
   { 0, 0, 0, 0, 0, 0, NONE, "DEFAULT", TGSI_OPCODE_DEFAULT },
   { 0, 0, 0, 0, 0, 0, NONE, "ENDSWITCH", TGSI_OPCODE_ENDSWITCH },

   { 1, 3, 0, 0, 0, 0, OTHR, "SAMPLE",      TGSI_OPCODE_SAMPLE },
   { 1, 2, 0, 0, 0, 0, OTHR, "SAMPLE_I",    TGSI_OPCODE_SAMPLE_I },
   { 1, 2, 0, 0, 0, 0, OTHR, "SAMPLE_I_MS", TGSI_OPCODE_SAMPLE_I_MS },
   { 1, 4, 0, 0, 0, 0, OTHR, "SAMPLE_B",    TGSI_OPCODE_SAMPLE_B },
   { 1, 4, 0, 0, 0, 0, OTHR, "SAMPLE_C",    TGSI_OPCODE_SAMPLE_C },
   { 1, 4, 0, 0, 0, 0, OTHR, "SAMPLE_C_LZ", TGSI_OPCODE_SAMPLE_C_LZ },
   { 1, 5, 0, 0, 0, 0, OTHR, "SAMPLE_D",    TGSI_OPCODE_SAMPLE_D },
   { 1, 3, 0, 0, 0, 0, OTHR, "SAMPLE_L",    TGSI_OPCODE_SAMPLE_L },
   { 1, 3, 0, 0, 0, 0, OTHR, "GATHER4",     TGSI_OPCODE_GATHER4 },
   { 1, 2, 0, 0, 0, 0, OTHR, "SVIEWINFO",   TGSI_OPCODE_SVIEWINFO },
   { 1, 2, 0, 0, 0, 0, OTHR, "SAMPLE_POS",  TGSI_OPCODE_SAMPLE_POS },
   { 1, 2, 0, 0, 0, 0, OTHR, "SAMPLE_INFO", TGSI_OPCODE_SAMPLE_INFO },
   { 1, 1, 0, 0, 0, 0, COMP, "UARL", TGSI_OPCODE_UARL },
   { 1, 3, 0, 0, 0, 0, COMP, "UCMP", TGSI_OPCODE_UCMP },
   { 1, 1, 0, 0, 0, 0, COMP, "IABS", TGSI_OPCODE_IABS },
   { 1, 1, 0, 0, 0, 0, COMP, "ISSG", TGSI_OPCODE_ISSG },
   { 1, 2, 0, 0, 0, 0, OTHR, "LOAD", TGSI_OPCODE_LOAD },
   { 1, 2, 0, 0, 0, 0, OTHR, "STORE", TGSI_OPCODE_STORE },
   { 1, 0, 0, 0, 0, 0, OTHR, "MFENCE", TGSI_OPCODE_MFENCE },
   { 1, 0, 0, 0, 0, 0, OTHR, "LFENCE", TGSI_OPCODE_LFENCE },
   { 1, 0, 0, 0, 0, 0, OTHR, "SFENCE", TGSI_OPCODE_SFENCE },
   { 0, 0, 0, 0, 0, 0, OTHR, "BARRIER", TGSI_OPCODE_BARRIER },

   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMUADD", TGSI_OPCODE_ATOMUADD },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMXCHG", TGSI_OPCODE_ATOMXCHG },
   { 1, 4, 0, 0, 0, 0, OTHR, "ATOMCAS", TGSI_OPCODE_ATOMCAS },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMAND", TGSI_OPCODE_ATOMAND },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMOR", TGSI_OPCODE_ATOMOR },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMXOR", TGSI_OPCODE_ATOMXOR },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMUMIN", TGSI_OPCODE_ATOMUMIN },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMUMAX", TGSI_OPCODE_ATOMUMAX },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMIMIN", TGSI_OPCODE_ATOMIMIN },
   { 1, 3, 0, 0, 0, 0, OTHR, "ATOMIMAX", TGSI_OPCODE_ATOMIMAX }
};

const struct tgsi_opcode_info *
tgsi_get_opcode_info( uint opcode )
{
   static boolean firsttime = 1;

   if (firsttime) {
      unsigned i;
      firsttime = 0;
      for (i = 0; i < Elements(opcode_info); i++)
         assert(opcode_info[i].opcode == i);
   }
   
   if (opcode < TGSI_OPCODE_LAST)
      return &opcode_info[opcode];

   assert( 0 );
   return NULL;
}


const char *
tgsi_get_opcode_name( uint opcode )
{
   const struct tgsi_opcode_info *info = tgsi_get_opcode_info(opcode);
   return info->mnemonic;
}


const char *
tgsi_get_processor_name( uint processor )
{
   switch (processor) {
   case TGSI_PROCESSOR_VERTEX:
      return "vertex shader";
   case TGSI_PROCESSOR_FRAGMENT:
      return "fragment shader";
   case TGSI_PROCESSOR_GEOMETRY:
      return "geometry shader";
   default:
      return "unknown shader type!";
   }
}

/*
 * infer the source type of a TGSI opcode.
 * MOV is special so return VOID
 */
enum tgsi_opcode_type
tgsi_opcode_infer_src_type( uint opcode )
{
   switch (opcode) {
   case TGSI_OPCODE_MOV:
      return TGSI_TYPE_UNTYPED;
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
   case TGSI_OPCODE_U2F:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_USHR:
   case TGSI_OPCODE_SHL:
   case TGSI_OPCODE_TXQ:
      return TGSI_TYPE_UNSIGNED;
   case TGSI_OPCODE_MOD:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_IDIV:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_IABS:
   case TGSI_OPCODE_ISSG:
   case TGSI_OPCODE_UARL:
      return TGSI_TYPE_SIGNED;
   default:
      return TGSI_TYPE_FLOAT;
   }
}

/*
 * infer the destination type of a TGSI opcode.
 * MOV is special so return VOID
 */
enum tgsi_opcode_type
tgsi_opcode_infer_dst_type( uint opcode )
{
   switch (opcode) {
   case TGSI_OPCODE_MOV:
      return TGSI_TYPE_UNTYPED;
   case TGSI_OPCODE_F2U:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_USHR:
   case TGSI_OPCODE_SHL:
   case TGSI_OPCODE_TXQ:
   case TGSI_OPCODE_TXQ_LZ:
      return TGSI_TYPE_UNSIGNED;
   case TGSI_OPCODE_F2I:
   case TGSI_OPCODE_IDIV:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_MOD:
   case TGSI_OPCODE_UARL:
   case TGSI_OPCODE_ARL:
   case TGSI_OPCODE_ARR:
   case TGSI_OPCODE_IABS:
   case TGSI_OPCODE_ISSG:
      return TGSI_TYPE_SIGNED;
   default:
      return TGSI_TYPE_FLOAT;
   }
}
