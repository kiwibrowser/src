/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "main/macros.h"
#include "program/program.h"
#include "program/prog_print.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"

const struct brw_instruction_info brw_opcodes[128] = {
    [BRW_OPCODE_MOV] = { .name = "mov", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_FRC] = { .name = "frc", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_RNDU] = { .name = "rndu", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_RNDD] = { .name = "rndd", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_RNDE] = { .name = "rnde", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_RNDZ] = { .name = "rndz", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_NOT] = { .name = "not", .nsrc = 1, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_LZD] = { .name = "lzd", .nsrc = 1, .ndst = 1 },

    [BRW_OPCODE_MUL] = { .name = "mul", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_MAC] = { .name = "mac", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_MACH] = { .name = "mach", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_LINE] = { .name = "line", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_PLN] = { .name = "pln", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SAD2] = { .name = "sad2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SADA2] = { .name = "sada2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP4] = { .name = "dp4", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DPH] = { .name = "dph", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP3] = { .name = "dp3", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP2] = { .name = "dp2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_MATH] = { .name = "math", .nsrc = 2, .ndst = 1 },

    [BRW_OPCODE_AVG] = { .name = "avg", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_ADD] = { .name = "add", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_SEL] = { .name = "sel", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_AND] = { .name = "and", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_OR] = { .name = "or", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_XOR] = { .name = "xor", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_SHR] = { .name = "shr", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_SHL] = { .name = "shl", .nsrc = 2, .ndst = 1, .is_arith = 1 },
    [BRW_OPCODE_ASR] = { .name = "asr", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_CMP] = { .name = "cmp", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_CMPN] = { .name = "cmpn", .nsrc = 2, .ndst = 1 },

    [BRW_OPCODE_SEND] = { .name = "send", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_NOP] = { .name = "nop", .nsrc = 0, .ndst = 0 },
    [BRW_OPCODE_JMPI] = { .name = "jmpi", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_IF] = { .name = "if", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_IFF] = { .name = "iff", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_WHILE] = { .name = "while", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_ELSE] = { .name = "else", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_BREAK] = { .name = "break", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_CONTINUE] = { .name = "cont", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_HALT] = { .name = "halt", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_MSAVE] = { .name = "msave", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_PUSH] = { .name = "push", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_MRESTORE] = { .name = "mrest", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_POP] = { .name = "pop", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_WAIT] = { .name = "wait", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_DO] = { .name = "do", .nsrc = 0, .ndst = 0 },
    [BRW_OPCODE_ENDIF] = { .name = "endif", .nsrc = 2, .ndst = 0 },
};

static INLINE
bool brw_is_arithmetic_inst(const struct brw_instruction *inst)
{
   return brw_opcodes[inst->header.opcode].is_arith;
}

static const GLuint inst_stride[7] = {
    [0] = 0,
    [1] = 1,
    [2] = 2,
    [3] = 4,
    [4] = 8,
    [5] = 16,
    [6] = 32
};

static const GLuint inst_type_size[8] = {
    [BRW_REGISTER_TYPE_UD] = 4,
    [BRW_REGISTER_TYPE_D] = 4,
    [BRW_REGISTER_TYPE_UW] = 2,
    [BRW_REGISTER_TYPE_W] = 2,
    [BRW_REGISTER_TYPE_UB] = 1,
    [BRW_REGISTER_TYPE_B] = 1,
    [BRW_REGISTER_TYPE_F] = 4
};

static INLINE bool
brw_is_grf_written(const struct brw_instruction *inst,
                   int reg_index, int size,
                   int gen)
{
   if (brw_opcodes[inst->header.opcode].ndst == 0)
      return false;

   if (inst->bits1.da1.dest_address_mode != BRW_ADDRESS_DIRECT)
      if (inst->bits1.ia1.dest_reg_file == BRW_GENERAL_REGISTER_FILE)
         return true;

   if (inst->bits1.da1.dest_reg_file != BRW_GENERAL_REGISTER_FILE)
      return false;

   const int reg_start = reg_index * REG_SIZE;
   const int reg_end = reg_start + size;

   const int type_size = inst_type_size[inst->bits1.da1.dest_reg_type];
   const int write_start = inst->bits1.da1.dest_reg_nr*REG_SIZE
                         + inst->bits1.da1.dest_subreg_nr;
   int length, write_end;

   /* SEND is specific */
   if (inst->header.opcode == BRW_OPCODE_SEND) {
      if (gen >= 5)
         length = inst->bits3.generic_gen5.response_length*REG_SIZE;
      else 
         length = inst->bits3.generic.response_length*REG_SIZE;
   }
   else {
      length = 1 << inst->header.execution_size;
      length *= type_size;
      length *= inst->bits1.da1.dest_horiz_stride;
   }

   /* If the two intervals intersect, we overwrite the register */
   write_end = write_start + length;
   const int left = MAX2(write_start, reg_start);
   const int right = MIN2(write_end, reg_end);

   return left < right;
}

static bool
brw_is_mrf_written_alu(const struct brw_instruction *inst,
		       int reg_index, int size)
{
   if (brw_opcodes[inst->header.opcode].ndst == 0)
      return false;

   if (inst->bits1.da1.dest_reg_file != BRW_MESSAGE_REGISTER_FILE)
      return false;

   if (inst->bits1.da1.dest_address_mode != BRW_ADDRESS_DIRECT)
      return true;

   const int reg_start = reg_index * REG_SIZE;
   const int reg_end = reg_start + size;

   const int mrf_index = inst->bits1.da1.dest_reg_nr & 0x0f;
   const int is_compr4 = inst->bits1.da1.dest_reg_nr & BRW_MRF_COMPR4;
   const int type_size = inst_type_size[inst->bits1.da1.dest_reg_type];

   /* We use compr4 with a size != 16 elements. Strange, we conservatively
    * consider that we are writing the register.
    */
   if (is_compr4 && inst->header.execution_size != BRW_EXECUTE_16)
      return true;

   /* Here we write mrf_{i} and mrf_{i+4}. So we read two times 8 elements */
   if (is_compr4) {
      const int length = 8 * type_size * inst->bits1.da1.dest_horiz_stride;

      /* First 8-way register */
      const int write_start0 = mrf_index*REG_SIZE
                             + inst->bits1.da1.dest_subreg_nr;
      const int write_end0 = write_start0 + length;

      /* Second 8-way register */
      const int write_start1 = (mrf_index+4)*REG_SIZE
                             + inst->bits1.da1.dest_subreg_nr;
      const int write_end1 = write_start1 + length;

      /* If the two intervals intersect, we overwrite the register */
      const int left0 = MAX2(write_start0, reg_start);
      const int right0 = MIN2(write_end0, reg_end);
      const int left1 = MAX2(write_start1, reg_start);
      const int right1 = MIN2(write_end1, reg_end);

      if (left0 < right0 || left1 < right1)
	 return true;
   }
   else {
      int length;
      length = 1 << inst->header.execution_size;
      length *= type_size;
      length *= inst->bits1.da1.dest_horiz_stride;

      /* If the two intervals intersect, we write into the register */
      const int write_start = inst->bits1.da1.dest_reg_nr*REG_SIZE
                            + inst->bits1.da1.dest_subreg_nr;
      const int write_end = write_start + length;
      const int left = MAX2(write_start, reg_start);
      const int right = MIN2(write_end, reg_end);

      if (left < right)
	 return true;
   }

   return false;
}

/* SEND may perform an implicit mov to a mrf register */
static bool
brw_is_mrf_written_send(const struct brw_instruction *inst,
			int reg_index, int size)
{

   const int reg_start = reg_index * REG_SIZE;
   const int reg_end = reg_start + size;
   const int mrf_start = inst->header.destreg__conditionalmod;
   const int write_start = mrf_start * REG_SIZE;
   const int write_end = write_start + REG_SIZE;
   const int left = MAX2(write_start, reg_start);
   const int right = MIN2(write_end, reg_end);

   if (inst->header.opcode != BRW_OPCODE_SEND ||
       inst->bits1.da1.src0_reg_file == 0)
      return false;

   return left < right;
}

/* Specific path for message register since we need to handle the compr4 case */
static INLINE bool
brw_is_mrf_written(const struct brw_instruction *inst, int reg_index, int size)
{
   return (brw_is_mrf_written_alu(inst, reg_index, size) ||
	   brw_is_mrf_written_send(inst, reg_index, size));
}

static INLINE bool
brw_is_mrf_read(const struct brw_instruction *inst,
                int reg_index, int size, int gen)
{
   if (inst->header.opcode != BRW_OPCODE_SEND)
      return false;
   if (inst->bits2.da1.src0_address_mode != BRW_ADDRESS_DIRECT)
      return true;

   const int reg_start = reg_index*REG_SIZE;
   const int reg_end = reg_start + size;

   int length, read_start, read_end;
   if (gen >= 5)
      length = inst->bits3.generic_gen5.msg_length*REG_SIZE;
   else 
      length = inst->bits3.generic.msg_length*REG_SIZE;

   /* Look if SEND uses an implicit mov. In that case, we read one less register
    * (but we write it)
    */
   if (inst->bits1.da1.src0_reg_file != 0)
      read_start = inst->header.destreg__conditionalmod;
   else {
      length--;
      read_start = inst->header.destreg__conditionalmod + 1;
   }
   read_start *= REG_SIZE;
   read_end = read_start + length;

   const int left = MAX2(read_start, reg_start);
   const int right = MIN2(read_end, reg_end);

   return left < right;
}

static INLINE bool
brw_is_grf_read(const struct brw_instruction *inst, int reg_index, int size)
{
   int i, j;
   if (brw_opcodes[inst->header.opcode].nsrc == 0)
      return false;

   /* Look at first source. We must take into account register regions to
    * monitor carefully the read. Note that we are a bit too conservative here
    * since we do not take into account the fact that some complete registers
    * may be skipped
    */
   if (brw_opcodes[inst->header.opcode].nsrc >= 1) {

      if (inst->bits2.da1.src0_address_mode != BRW_ADDRESS_DIRECT)
         if (inst->bits1.ia1.src0_reg_file == BRW_GENERAL_REGISTER_FILE)
            return true;
      if (inst->bits1.da1.src0_reg_file != BRW_GENERAL_REGISTER_FILE)
         return false;

      const int reg_start = reg_index*REG_SIZE;
      const int reg_end = reg_start + size;

      /* See if at least one of this element intersects the interval */
      const int type_size = inst_type_size[inst->bits1.da1.src0_reg_type];
      const int elem_num = 1 << inst->header.execution_size;
      const int width = 1 << inst->bits2.da1.src0_width;
      const int row_num = elem_num >> inst->bits2.da1.src0_width;
      const int hs = type_size*inst_stride[inst->bits2.da1.src0_horiz_stride];
      const int vs = type_size*inst_stride[inst->bits2.da1.src0_vert_stride];
      int row_start = inst->bits2.da1.src0_reg_nr*REG_SIZE
                    + inst->bits2.da1.src0_subreg_nr;
      for (j = 0; j < row_num; ++j) {
         int write_start = row_start;
         for (i = 0; i < width; ++i) {
            const int write_end = write_start + type_size;
            const int left = write_start > reg_start ? write_start : reg_start;
            const int right = write_end < reg_end ? write_end : reg_end;
            if (left < right)
               return true;
            write_start += hs;
         }
         row_start += vs;
      }
   }

   /* Second src register */
   if (brw_opcodes[inst->header.opcode].nsrc >= 2) {

      if (inst->bits3.da1.src1_address_mode != BRW_ADDRESS_DIRECT)
         if (inst->bits1.ia1.src1_reg_file == BRW_GENERAL_REGISTER_FILE)
            return true;
      if (inst->bits1.da1.src1_reg_file != BRW_GENERAL_REGISTER_FILE)
         return false;

      const int reg_start = reg_index*REG_SIZE;
      const int reg_end = reg_start + size;

      /* See if at least one of this element intersects the interval */
      const int type_size = inst_type_size[inst->bits1.da1.src1_reg_type];
      const int elem_num = 1 << inst->header.execution_size;
      const int width = 1 << inst->bits3.da1.src1_width;
      const int row_num = elem_num >> inst->bits3.da1.src1_width;
      const int hs = type_size*inst_stride[inst->bits3.da1.src1_horiz_stride];
      const int vs = type_size*inst_stride[inst->bits3.da1.src1_vert_stride];
      int row_start = inst->bits3.da1.src1_reg_nr*REG_SIZE
                    + inst->bits3.da1.src1_subreg_nr;
      for (j = 0; j < row_num; ++j) {
         int write_start = row_start;
         for (i = 0; i < width; ++i) {
            const int write_end = write_start + type_size;
            const int left = write_start > reg_start ? write_start : reg_start;
            const int right = write_end < reg_end ? write_end : reg_end;
            if (left < right)
               return true;
            write_start += hs;
         }
         row_start += vs;
      }
   }

   return false;
}

static INLINE bool
brw_is_control_done(const struct brw_instruction *mov) {
   return
       mov->header.dependency_control != 0 ||
       mov->header.thread_control != 0 ||
       mov->header.mask_control != 0 ||
       mov->header.saturate != 0 ||
       mov->header.debug_control != 0;
}

static INLINE bool
brw_is_predicated(const struct brw_instruction *mov) {
   return mov->header.predicate_control != 0;
}

static INLINE bool
brw_is_grf_to_mrf_mov(const struct brw_instruction *mov,
                      int *mrf_index,
                      int *grf_index,
                      bool *is_compr4)
{
   if (brw_is_predicated(mov) ||
       brw_is_control_done(mov) ||
       mov->header.debug_control != 0)
      return false;

   if (mov->bits1.da1.dest_address_mode != BRW_ADDRESS_DIRECT ||
       mov->bits1.da1.dest_reg_file != BRW_MESSAGE_REGISTER_FILE ||
       mov->bits1.da1.dest_reg_type != BRW_REGISTER_TYPE_F ||
       mov->bits1.da1.dest_horiz_stride != BRW_HORIZONTAL_STRIDE_1 ||
       mov->bits1.da1.dest_subreg_nr != 0)
      return false;

   if (mov->bits2.da1.src0_address_mode != BRW_ADDRESS_DIRECT ||
       mov->bits1.da1.src0_reg_file != BRW_GENERAL_REGISTER_FILE ||
       mov->bits1.da1.src0_reg_type != BRW_REGISTER_TYPE_F ||
       mov->bits2.da1.src0_width != BRW_WIDTH_8 ||
       mov->bits2.da1.src0_horiz_stride != BRW_HORIZONTAL_STRIDE_1 ||
       mov->bits2.da1.src0_vert_stride != BRW_VERTICAL_STRIDE_8 ||
       mov->bits2.da1.src0_subreg_nr != 0 ||
       mov->bits2.da1.src0_abs != 0 ||
       mov->bits2.da1.src0_negate != 0)
      return false;

   *grf_index = mov->bits2.da1.src0_reg_nr;
   *mrf_index = mov->bits1.da1.dest_reg_nr & 0x0f;
   *is_compr4 = (mov->bits1.da1.dest_reg_nr & BRW_MRF_COMPR4) != 0;
   return true;
}

static INLINE bool
brw_is_grf_straight_write(const struct brw_instruction *inst, int grf_index)
{
   /* remark: no problem to predicate a SEL instruction */
   if ((!brw_is_predicated(inst) || inst->header.opcode == BRW_OPCODE_SEL) &&
       brw_is_control_done(inst) == false &&
       inst->header.execution_size == 4 &&
       inst->header.access_mode == BRW_ALIGN_1 &&
       inst->bits1.da1.dest_address_mode == BRW_ADDRESS_DIRECT &&
       inst->bits1.da1.dest_reg_file == BRW_GENERAL_REGISTER_FILE &&
       inst->bits1.da1.dest_reg_type == BRW_REGISTER_TYPE_F &&
       inst->bits1.da1.dest_horiz_stride == BRW_HORIZONTAL_STRIDE_1 &&
       inst->bits1.da1.dest_reg_nr == grf_index &&
       inst->bits1.da1.dest_subreg_nr == 0 &&
       brw_is_arithmetic_inst(inst))
      return true;

   return false;
}

static INLINE bool
brw_inst_are_equal(const struct brw_instruction *src0,
                   const struct brw_instruction *src1)
{
   const GLuint *field0 = (GLuint *) src0;
   const GLuint *field1 = (GLuint *) src1;
   return field0[0] == field1[0] &&
          field0[1] == field1[1] &&
          field0[2] == field1[2] &&
          field0[3] == field1[3];
}

static INLINE void
brw_inst_copy(struct brw_instruction *dst,
              const struct brw_instruction *src)
{
   GLuint *field_dst = (GLuint *) dst;
   const GLuint *field_src = (GLuint *) src;
   field_dst[0] = field_src[0];
   field_dst[1] = field_src[1];
   field_dst[2] = field_src[2];
   field_dst[3] = field_src[3];
}

static void brw_remove_inst(struct brw_compile *p, const bool *removeInst)
{
   int i, nr_insn = 0, to = 0, from = 0;

   for (from = 0; from < p->nr_insn; ++from) {
      if (removeInst[from])
         continue;
      if(to != from)
         brw_inst_copy(p->store + to, p->store + from);
      to++;
   }

   for (i = 0; i < p->nr_insn; ++i)
      if (removeInst[i] == false)
         nr_insn++;
   p->nr_insn = nr_insn;
}

/* The gen code emitter generates a lot of duplications in the
 * grf-to-mrf moves, for example when texture sampling with the same
 * coordinates from multiple textures..  Here, we monitor same mov
 * grf-to-mrf instrutions and remove repeated ones where the operands
 * and dst ahven't changed in between.
 */
void brw_remove_duplicate_mrf_moves(struct brw_compile *p)
{
   const int gen = p->brw->intel.gen;
   int i, j;

   bool *removeInst = calloc(sizeof(bool), p->nr_insn);
   for (i = 0; i < p->nr_insn; i++) {
      if (removeInst[i])
         continue;

      const struct brw_instruction *mov = p->store + i;
      int mrf_index, grf_index;
      bool is_compr4;

      /* Only consider _straight_ grf-to-mrf moves */
      if (!brw_is_grf_to_mrf_mov(mov, &mrf_index, &grf_index, &is_compr4))
         continue;

      const int mrf_index0 = mrf_index;
      const int mrf_index1 = is_compr4 ? mrf_index0+4 : mrf_index0+1;
      const int simd16_size = 2 * REG_SIZE;

      for (j = i + 1; j < p->nr_insn; j++) {
         const struct brw_instruction *inst = p->store + j;

         if (brw_inst_are_equal(mov, inst)) {
            removeInst[j] = true;
            continue;
         }

         if (brw_is_grf_written(inst, grf_index, simd16_size, gen) ||
             brw_is_mrf_written(inst, mrf_index0, REG_SIZE) ||
             brw_is_mrf_written(inst, mrf_index1, REG_SIZE))
            break;
      }
   }

   brw_remove_inst(p, removeInst);
   free(removeInst);
}

/* Replace moves to MRFs where the value moved is the result of a
 * normal arithmetic operation with computation right into the MRF.
 */
void brw_remove_grf_to_mrf_moves(struct brw_compile *p)
{
   int i, j, prev;
   struct brw_context *brw = p->brw;
   const int gen = brw->intel.gen;
   const int simd16_size = 2*REG_SIZE;

   bool *removeInst = calloc(sizeof(bool), p->nr_insn);
   assert(removeInst);

   for (i = 0; i < p->nr_insn; i++) {
      if (removeInst[i])
         continue;

      struct brw_instruction *grf_inst = NULL;
      const struct brw_instruction *mov = p->store + i;
      int mrf_index, grf_index;
      bool is_compr4;

      /* Only consider _straight_ grf-to-mrf moves */
      if (!brw_is_grf_to_mrf_mov(mov, &mrf_index, &grf_index, &is_compr4))
         continue;

      /* Using comp4 enables a stride of 4 for this instruction */
      const int mrf_index0 = mrf_index;
      const int mrf_index1 = is_compr4 ? mrf_index+4 : mrf_index+1;

      /* Look where the register has been set */
      prev = i;
      bool potential_remove = false;
      while (prev--) {

         /* If _one_ instruction writes the grf, we try to remove the mov */
         struct brw_instruction *inst = p->store + prev;
         if (brw_is_grf_straight_write(inst, grf_index)) {
            potential_remove = true;
            grf_inst = inst;
            break;
         }

      }

      if (potential_remove == false)
         continue;
      removeInst[i] = true;

      /* Monitor first the section of code between the grf computation and the
       * mov. Here we cannot read or write both mrf and grf register
       */
      for (j = prev + 1; j < i; ++j) {
         struct brw_instruction *inst = p->store + j;
         if (removeInst[j])
            continue;
         if (brw_is_grf_written(inst, grf_index, simd16_size, gen)   ||
             brw_is_grf_read(inst, grf_index, simd16_size)           ||
             brw_is_mrf_written(inst, mrf_index0, REG_SIZE)   ||
             brw_is_mrf_written(inst, mrf_index1, REG_SIZE)   ||
             brw_is_mrf_read(inst, mrf_index0, REG_SIZE, gen) ||
             brw_is_mrf_read(inst, mrf_index1, REG_SIZE, gen)) {
            removeInst[i] = false;
            break;
         }
      }

      /* After the mov, we can read or write the mrf. If the grf is overwritten,
       * we are done
       */
      for (j = i + 1; j < p->nr_insn; ++j) {
         struct brw_instruction *inst = p->store + j;
         if (removeInst[j])
            continue;

         if (brw_is_grf_read(inst, grf_index, simd16_size)) {
            removeInst[i] = false;
            break;
         }

         if (brw_is_grf_straight_write(inst, grf_index))
            break;
      }

      /* Note that with the top down traversal, we can safely pacth the mov
       * instruction
       */
      if (removeInst[i]) {
         grf_inst->bits1.da1.dest_reg_file = mov->bits1.da1.dest_reg_file;
         grf_inst->bits1.da1.dest_reg_nr = mov->bits1.da1.dest_reg_nr;
      }
   }

   brw_remove_inst(p, removeInst);
   free(removeInst);
}

static bool
is_single_channel_dp4(struct brw_instruction *insn)
{
   if (insn->header.opcode != BRW_OPCODE_DP4 ||
       insn->header.execution_size != BRW_EXECUTE_8 ||
       insn->header.access_mode != BRW_ALIGN_16 ||
       insn->bits1.da1.dest_reg_file != BRW_GENERAL_REGISTER_FILE)
      return false;

   if (!is_power_of_two(insn->bits1.da16.dest_writemask))
      return false;

   return true;
}

/**
 * Sets the dependency control fields on DP4 instructions.
 *
 * The hardware only tracks dependencies on a register basis, so when
 * you do:
 *
 * DP4 dst.x src1 src2
 * DP4 dst.y src1 src3
 * DP4 dst.z src1 src4
 * DP4 dst.w src1 src5
 *
 * It will wait to do the DP4 dst.y until the dst.x is resolved, etc.
 * We can examine our instruction stream and set the dependency
 * control fields to tell the hardware when to do it.
 *
 * We may want to extend this to other instructions that are used to
 * fill in a channel at a time of the destination register.
 */
static void
brw_set_dp4_dependency_control(struct brw_compile *p)
{
   int i;

   for (i = 1; i < p->nr_insn; i++) {
      struct brw_instruction *insn = &p->store[i];
      struct brw_instruction *prev = &p->store[i - 1];

      if (!is_single_channel_dp4(prev))
	 continue;

      if (!is_single_channel_dp4(insn)) {
	 i++;
	 continue;
      }

      /* Only avoid hw dep control if the write masks are different
       * channels of one reg.
       */
      if (insn->bits1.da16.dest_writemask == prev->bits1.da16.dest_writemask)
	 continue;
      if (insn->bits1.da16.dest_reg_nr != prev->bits1.da16.dest_reg_nr)
	 continue;

      /* Check if the second instruction depends on the previous one
       * for a src.
       */
      if (insn->bits1.da1.src0_reg_file == BRW_GENERAL_REGISTER_FILE &&
	  (insn->bits2.da1.src0_address_mode != BRW_ADDRESS_DIRECT ||
	   insn->bits2.da1.src0_reg_nr == insn->bits1.da16.dest_reg_nr))
	  continue;
      if (insn->bits1.da1.src1_reg_file == BRW_GENERAL_REGISTER_FILE &&
	  (insn->bits3.da1.src1_address_mode != BRW_ADDRESS_DIRECT ||
	   insn->bits3.da1.src1_reg_nr == insn->bits1.da16.dest_reg_nr))
	  continue;

      prev->header.dependency_control |= BRW_DEPENDENCY_NOTCLEARED;
      insn->header.dependency_control |= BRW_DEPENDENCY_NOTCHECKED;
   }
}

void
brw_optimize(struct brw_compile *p)
{
   brw_set_dp4_dependency_control(p);
}
