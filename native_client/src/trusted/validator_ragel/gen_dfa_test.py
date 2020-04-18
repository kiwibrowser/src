#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import unittest

import def_format
import gen_dfa


class TestLegacyPrefixes(unittest.TestCase):

  def test_empty(self):
    self.assertEquals(
        gen_dfa.GenerateLegacyPrefixes(64, [], []),
        [()])

  def test_one(self):
    self.assertEquals(
        gen_dfa.GenerateLegacyPrefixes(64, ['req'], []),
        [('req',)])
    self.assertEquals(
        set(gen_dfa.GenerateLegacyPrefixes(64, [], ['opt'])),
        set([
            (),
            ('opt',)]))

  def test_many(self):
    self.assertEquals(
        set(gen_dfa.GenerateLegacyPrefixes(64, ['req'], ['opt1', 'opt2'])),
        set([
            ('req',),
            ('opt1', 'req'),
            ('opt2', 'req'),
            ('req', 'opt1'),
            ('req', 'opt2'),
            ('opt1', 'opt2', 'req'),
            ('opt1', 'req', 'opt2'),
            ('opt2', 'opt1', 'req'),
            ('opt2', 'req', 'opt1'),
            ('req', 'opt1', 'opt2'),
            ('req', 'opt2', 'opt1')]))


class TestOperand(unittest.TestCase):

  def test_default(self):
    op = gen_dfa.Operand.Parse(
        'r',
        default_rw=def_format.OperandReadWriteMode.READ)

    assert op.Readable() and not op.Writable()
    self.assertEquals(op.arg_type, def_format.OperandType.REGISTER_IN_OPCODE)
    self.assertEquals(op.size, '')
    assert not op.implicit

    self.assertEquals(str(op), '=r')


class TestInstruction(unittest.TestCase):

  def test_name_and_operands(self):
    instr = gen_dfa.Instruction()
    instr.ParseNameAndOperands('add G E')
    self.assertEquals(instr.name, 'add')
    self.assertEquals(len(instr.operands), 2)

    op1, op2 = instr.operands
    assert op1.Readable() and not op1.Writable()
    assert op2.Readable() and op2.Writable()

    self.assertEquals(str(instr), 'add =G &E,')

  def test_modrm_presence(self):
    instr = gen_dfa.Instruction.Parse(
        'add G E, 0x00, lock nacl-amd64-zero-extends')
    assert instr.HasModRM()

    instr = gen_dfa.Instruction.Parse(
        'mov O !a, 0xa0, ia32')
    assert not instr.HasModRM()

    # Technically, mfence has ModRM byte (0xf0), but for our purposes
    # we treat it as part of opcode.
    instr = gen_dfa.Instruction.Parse(
        'mfence, 0x0f 0xae 0xf0, CPUFeature_SSE2')
    assert not instr.HasModRM()

  def test_collect_prefixes(self):
    instr = gen_dfa.Instruction.Parse('pause, 0xf3 0x90, norex')

    instr.CollectPrefixes()

    self.assertEquals(instr.required_prefixes, ['0xf3'])
    self.assertEquals(instr.opcodes, ['0x90'])


class TestPrinterParts(unittest.TestCase):

  def test_nop_signature(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        '"nopw   0x0(%eax,%eax,1)", 0x66 0x0f 0x1f 0x44 0x00 0x00, ia32 norex')

    printer._PrintSignature(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        @instruction_nopw___0x0__eax__eax_1_
        @operands_count_is_0
        """.split())

  def test_simple_mov_signature(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse('mov =Ob !ab, 0xa0, ia32')
    instr = printer._SetOperandIndices(instr)

    printer._PrintSignature(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        @instruction_mov
        @operands_count_is_2
        @operand0_8bit
        @operand1_8bit
        """.split())

  def test_nop_opcode(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'nop, 0x90, norex')

    printer._PrintOpcode(instr)

    self.assertEquals(printer.GetContent(), '0x90')

  def test_register_in_opcode(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'bswap ry, 0x0f 0xc8')
    instr = printer._SetOperandIndices(instr)

    printer._PrintOpcode(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x0f (0xc8|0xc9|0xca|0xcb|0xcc|0xcd|0xce|0xcf)
        @operand0_from_opcode
        """.split())

  def test_opcode_in_modrm(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'add I E, 0x80 /0, lock nacl-amd64-zero-extends')

    printer._PrintOpcode(instr)

    self.assertEquals(printer.GetContent(), '0x80')


class TestInstructionPrinter(unittest.TestCase):

  def test_no_modrm(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'mov Ob !ab, 0xa0, ia32')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0xa0
        @instruction_mov
        @operands_count_is_2
        @operand0_8bit
        @operand1_8bit
        @operand1_rax
        @operand0_absolute_disp
        disp32
        """.split())

  def test_relative(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse('jmp Jb, 0xeb')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0xeb
        @instruction_jmp
        @operands_count_is_1
        @operand0_8bit
        rel8
        @operand0_jmp_to
        """.split())

  def test_immediates(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse('adc =Ib &ab, 0x14')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x14
        @instruction_adc
        @operands_count_is_2
        @operand0_8bit
        @operand1_8bit
        @operand1_rax
        imm8
        @operand0_immediate
        """.split())

  def test_2bit_immediate(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'vpermil2pd I2 Lpdx Wpdx Hpdx Vpdx, '
        '0xc4 RXB.00011 0.src.L.01 0x49, '
        'CPUFeature_XOP')

    instr = printer._SetOperandIndices(instr)
    printer._PrintImmediates(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        @operand0_immediate
        b_0xxx_00xx @imm2_operand
        @operand1_from_is4
        """.split())

  def test_no_modrm_with_rex(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'movabs Od !ad, 0xa0, amd64 nacl-forbidden')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0xa0
        @instruction_movabs
        @operands_count_is_2
        @operand0_32bit
        @operand1_32bit
        @set_spurious_rex_b
        @set_spurious_rex_x
        @set_spurious_rex_r
        @operand1_rax
        @operand0_absolute_disp
        disp64
        """.split())

  def test_nop_with_rex(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'nop, 0x40 0x90, amd64 norex')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x40 0x90
        @instruction_nop
        @operands_count_is_0
        @set_spurious_rex_b
        @set_spurious_rex_x
        @set_spurious_rex_r
        """.split())

  def test_rex_b_for_opcode_in_register(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'bswap rd, 0x0f 0xc8')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0x0f
        (0xc8|0xc9|0xca|0xcb|0xcc|0xcd|0xce|0xcf) @operand0_from_opcode
        @instruction_bswap
        @operands_count_is_1
        @operand0_32bit
        @set_spurious_rex_x
        @set_spurious_rex_r
        """.split())

  def test_with_prefixes(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.VALIDATOR, 32)
    instr = gen_dfa.Instruction.Parse(
        'movsb =Xb &Yb, 0xa4, rep')

    instr.CollectPrefixes()

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        (rep)?
        0xa4
        """.split())

  def test_modrm_reg(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'mov Gd !Rd, 0x89')

    printer.PrintInstructionWithModRMReg(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0x89
        modrm_registers
        @operand0_from_modrm_reg
        @operand1_from_modrm_rm
        @instruction_mov
        @operands_count_is_2
        @operand0_32bit
        @operand1_32bit
        @set_spurious_rex_x
        """.split())

  def test_modrm_memory(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'mov Gd !Md, 0x89')

    printer.PrintInstructionWithModRMMemory(
        instr,
        gen_dfa.AddressMode('operand_sib_pure_index',
                            x_matters=True,
                            b_matters=False))

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0x89
        (any
         @instruction_mov
         @operands_count_is_2
         @operand0_32bit
         @operand1_memory
         @operand0_from_modrm_reg
         @operand1_rm
         any* &
         operand_sib_pure_index)
        @set_spurious_rex_b
        """.split())

  def test_opcode_in_modrm(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'adc =Ib &Rb, 0x80 /2')

    printer.PrintInstructionWithModRMReg(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x80
        (modrm_registers & opcode_2)
        @operand1_from_modrm_rm
        @instruction_adc
        @operands_count_is_2
        @operand0_8bit
        @operand1_8bit
        imm8
        @operand0_immediate
        """.split())

  def test_segment_register(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'mov Sw !Rw, 0x8c, nacl-forbidden')

    printer.PrintInstructionWithModRMReg(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x8c
        (modrm_registers & opcode_s)
        @operand0_from_modrm_reg_norex
        @operand1_from_modrm_rm
        @instruction_mov
        @operands_count_is_2
        @operand0_segreg
        @operand1_16bit
        """.split())

  def test_cpu_features(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'emms, 0x0f 0x77, CPUFeature_MMX')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x0f 0x77
        @instruction_emms
        @operands_count_is_0
        @CPUFeature_MMX
        """.split())

  def test_opcode_instead_of_immediate(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'pavgusb Nq Pq, 0x0f 0x0f / 0xbf, CPUFeature_3DNOW')

    printer.PrintInstructionWithModRMReg(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x0f 0x0f
        modrm_registers
        @operand0_from_modrm_rm_norex
        @operand1_from_modrm_reg_norex
        0xbf
        @instruction_pavgusb
        @operands_count_is_2
        @operand0_mmx
        @operand1_mmx
        @CPUFeature_3DNOW
        """.split())

    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'pavgusb Mq Pq, 0x0f 0x0f / 0xbf, CPUFeature_3DNOW')

    printer.PrintInstructionWithModRMMemory(
        instr,
        gen_dfa.AddressMode('single_register_memory',
                            x_matters=False,
                            b_matters=True))

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x0f 0x0f
        (any @operand0_rm @operand1_from_modrm_reg_norex any* &
         single_register_memory)
        0xbf
        @instruction_pavgusb
        @operands_count_is_2
        @operand0_memory
        @operand1_mmx
        @CPUFeature_3DNOW
        """.split())

  def test_vex_prefix(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'vcvtsd2si Wsd Gy, 0xc4 RXB.00001 W.1111.x.11 0x2d, CPUFeature_AVX')
    instr.rex.r_matters = True
    instr.rex.x_matters = False
    instr.rex.b_matters = True

    instr.rex.w_matters = True
    instr.rex.w_set = False

    printer._PrintVexOrXopPrefix(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        (0xc4 (VEX_RB & VEX_map00001)  b_0_1111_0_11 @vex_prefix3 |
         0xc5 b_X_1111_0_11 @vex_prefix_short)
        """.split())

  def test_name_suffix(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'ret, 0xc3, rep amd64 att-show-name-suffix-q nacl-forbidden')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0xc3
        @instruction_ret
        @att_show_name_suffix_q
        @operands_count_is_0
        @set_spurious_rex_b
        @set_spurious_rex_x
        @set_spurious_rex_r
        """.split())

  def test_cr_and_lock(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'mov Rr Cr, 0xf0 0x0f 0x22, CPUFeature_ALTMOVCR8 nacl-forbidden')
    instr.rex.w_matters = False
    instr.CollectPrefixes()

    printer.PrintInstructionWithModRMReg(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        (0xf0)
        0x0f
        0x22
        modrm_registers
        @operand0_from_modrm_rm
        @operand1_from_modrm_reg
        @instruction_mov
        @operands_count_is_2
        @lock_extends_cr_operand1
        @operand0_32bit
        @operand1_creg
        @CPUFeature_ALTMOVCR8
        """.split())

  def test_validator_actions(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.VALIDATOR, 64)
    instr = gen_dfa.Instruction.Parse(
        'mov Md !Gd, 0x8a, nacl-amd64-modifiable nacl-amd64-zero-extends')

    printer.PrintInstructionWithModRMMemory(
        instr,
        gen_dfa.AddressMode('single_register_memory',
                            x_matters=False,
                            b_matters=True))

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0x8a
        (any @operand0_32bit
             @modifiable_instruction
             @operand0_from_modrm_reg
         any* &
         single_register_memory @check_memory_access)
        @process_1_operand_zero_extends
        """.split())


class TestSplit(unittest.TestCase):

  def test_no_split_rm(self):
    instr = gen_dfa.Instruction.Parse('mov =G !E, 0x88')
    self.assertEquals(
        map(str, gen_dfa.SplitRM(instr)),
        ['mov =G !R, 0x88',
         'mov =G !M, 0x88'])

  def test_split_rm(self):
    instr = gen_dfa.Instruction.Parse('mov =O !a, 0xa0, ia32')
    self.assertEquals(
        map(str, gen_dfa.SplitRM(instr)),
        [str(instr)])

  def test_split_byte(self):
    instr = gen_dfa.Instruction.Parse('mov =I !E, 0xc6 /0')
    self.assertEquals(
        map(str, gen_dfa.SplitByteNonByte(instr)),
        ['mov =Ib !Eb, 0xc6 /0',
         'mov =Iz !Ev, 0xc7 /0'])

  def test_split_vyz(self):
    instr = gen_dfa.Instruction.Parse('mov =Iz !Ev, 0xc7 /0')

    instr1, instr2, instr3 = gen_dfa.SplitVYZ(64, instr)

    self.assertEquals(str(instr1), 'mov =Iw !Ew, 0xc7 /0')
    assert not instr1.rex.w_set
    self.assertEquals(instr1.required_prefixes, ['data16'])

    self.assertEquals(str(instr2), 'mov =Id !Ed, 0xc7 /0')
    assert not instr2.rex.w_set
    self.assertEquals(instr2.required_prefixes, [])

    self.assertEquals(str(instr3), 'mov =Id !Eq, 0xc7 /0')
    assert instr3.rex.w_set
    self.assertEquals(instr3.required_prefixes, [])

  def test_split_L(self):
    instr = gen_dfa.Instruction.Parse(
        'vaddpd =Wpdx =Hpdx !Vpdx, '
        '0xc4 RXB.00001 x.src.L.01 0x58, '
        'CPUFeature_AVX')
    self.assertEquals(
        map(str, gen_dfa.SplitL(instr)),
        [('vaddpd =Wpd =Hpd !Vpd, '
          '0xc4 RXB.00001 x.src.0.01 0x58, '
          'CPUFeature_AVX'),
         ('vaddpd =Wpd-ymm =Hpd-ymm !Vpd-ymm, '
          '0xc4 RXB.00001 x.src.1.01 0x58, '
          'CPUFeature_AVX')])

  def test_name_suffix(self):
    instr = gen_dfa.Instruction.Parse(
        'movs X Y, 0xa4, rep nacl-amd64-forbidden')
    self.assertEquals(
        map(str, gen_dfa.SplitByteNonByte(instr)),
        ['movs =Xb &Yb, 0xa4, rep nacl-amd64-forbidden att-show-name-suffix-b',
         'movs =Xv &Yv, 0xa5, rep nacl-amd64-forbidden'])


class TestParser(unittest.TestCase):

  def test_instruction_definitions(self):
    def_filenames = sys.argv[1:]
    assert len(def_filenames) > 0
    for filename in def_filenames:
      list(gen_dfa.ParseDefFile(filename))


if __name__ == '__main__':
  unittest.main(argv=[sys.argv[0], '--verbose'])
