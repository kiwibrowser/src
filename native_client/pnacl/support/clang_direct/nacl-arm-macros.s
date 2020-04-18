@ Copyright (c) 2014 The Native Client Authors. All rights reserved.
@ Use of this source code is governed by a BSD-style license that can be
@ found in the LICENSE file.

@ GAS assembler macros for NaCl ARM
@ NB: GCC emits code using only a few of these, noted below.
@ TODO(mcgrathr): not really clear if this should live with gcc or elsewhere

@ This turns on instruction bundling in the assembler, with 16-byte bundles.
.bundle_align_mode 4

.macro _sfi_cmask reg, cond=
	bic\cond \reg, \reg, #0xC000000F
.endm

.macro _sfi_dmask reg, cond=
	bic\cond \reg, \reg, #0xC0000000
.endm

.macro _sfi_for_each_cond macro, args:vararg
	\macro eq, \args
	\macro ne, \args
	\macro cs, \args
	\macro hs, \args
	\macro cc, \args
	\macro lo, \args
	\macro mi, \args
	\macro pl, \args
	\macro vs, \args
	\macro vc, \args
	\macro hi, \args
	\macro ls, \args
	\macro ge, \args
	\macro lt, \args
	\macro gt, \args
	\macro le, \args
	\macro al, \args
	\macro nv, \args
.endm

@ Used in place of bx (indirect jump), no difference but the mnemonic.
@ NB: GCC output uses this.
.macro sfi_bx reg, cond=
	.bundle_lock
	_sfi_cmask \reg, \cond
	bx\cond \reg
	.bundle_unlock
.endm

.macro _sfi_bl label, target, cond=
	adr\cond lr, \label
	b\cond \target
	.p2align 4
\label\():
.endm

@ Used in place of bl (direct call), no difference but the mnemonic.
@ NB: GCC output uses this.
.macro sfi_bl target, cond=
	_sfi_bl .Lsfi.\@, \target, \cond
.endm

.macro _sfi_blx label, reg, cond
	adr lr, \label
	.bundle_lock
	_sfi_cmask \reg, \cond
	bx\cond \reg
	.bundle_unlock
	.p2align 4
\label\():
.endm

@ Used in place of blx (indirect call), no difference but the mnemonic.
@ NB: GCC output uses this.
.macro sfi_blx target, cond=
	_sfi_blx .Lsfi.\@, \target, \cond
.endm

.macro _sfi_condify_1 cond, macro
	.macro \macro\cond args:vararg
	\macro \args, \cond
	.endm
.endm
.macro _sfi_condify macro
	_sfi_for_each_cond _sfi_condify_1, \macro
.endm

@ Conditionalized variants of the branching macros.
@ NB: GCC output uses (some of) these.
_sfi_condify sfi_bl
_sfi_condify sfi_blx
_sfi_condify sfi_bx

.purgem _sfi_condify_1
.purgem _sfi_condify

@ ldr/str syntax cheat sheet:
@ [Rn, offset] => *(Rn + offset)
@ [Rn, offset]! => *(Rn += offset)
@ [Rn], offset => *Rn, Rn += offset

@ sfi_mem* macros have the feature that they specify each parameter
@ only once.  They all have the form:
@	sfi_mem... opcode value-register, base-register [, ...]
@ opcode		is the normal mnemonic (ldr, str, ldrb, etc.)
@ value-register	is the destination for a load and source for a store
@ 			(i.e. the first operand in normal ARM syntax);
@ base-register		is the base register in the address computation;
@			this is what's touched by the SFI masking instruction

@ For simple register or register+offset cases, e.g.:
@	sfi_mem ldr r0, r1		@ ldr r0, [r1]
@	sfi_mem str r1, r2, #4		@ str r1, [r2, #4]
.macro sfi_mem op, dest, basereg, offset=#0
	.bundle_lock
	_sfi_dmask \basereg
	\op \dest, [\basereg, \offset]
	.bundle_unlock
.endm

@ For "increment before" cases, e.g.:
@	sfi_memib ldr r0, r1, #4	@ ldr r0, [r1, #4]!
.macro sfi_memib op, dest, basereg, offset
	.bundle_lock
	_sfi_dmask \basereg
	\op \dest, [\basereg, \offset]!
	.bundle_unlock
.endm

@ For "increment after" cases, e.g.:
@	sfi_memia ldr r0, r1, #4	@ ldr r0, [r1], #4
.macro sfi_memia op, dest, basereg, offset
	.bundle_lock
	_sfi_dmask \basereg
	\op \dest, [\basereg], \offset
	.bundle_unlock
.endm

@ For register pair cases (i.e. ldrd or strd), e.g.:
@	sfi_memd ldrd r2, r3, r0	@ ldrd r2, r3, [r0]
@ (Note we don't support the pre-indexed case.)
.macro sfi_memd op, dest, dest2, basereg, offset=#0
	.bundle_lock
	_sfi_dmask \basereg
	\op \dest,\dest2, [\basereg, \offset]
	.bundle_unlock
.endm

@ For "increment after" register pair cases (i.e. ldrd or strd), e.g.:
@	sfi_memdia ldrd r2, r3, r0, -r1	@ ldrd r2, r3, [r0], -r1
.macro sfi_memdia op, dest, dest2, basereg, offset
	.bundle_lock
	_sfi_dmask \basereg
	\op \dest,\dest2, [\basereg], \offset
	.bundle_unlock
.endm

@ For load/store multiple cases, e.g.:
@	sfi_memm ldm r0, {r1, r2, r3}	@ ldm r0, {r1, r2, r3}
@ Note you don't want to use this for sp, since SFI is not required.
.macro sfi_memm op basereg, reglist:vararg
	.bundle_lock
	_sfi_dmask \basereg
	\op \basereg, \reglist
	.bundle_unlock
.endm

@ For "increment after" load/store multiple cases, e.g.:
@	sfi_memm ldmia r0!, {r5-r8}	@ ldmia r0!, {r5-r8}
@ Note you don't want to use this for sp, since SFI is not required.
.macro sfi_memmia op basereg, reglist:vararg
	.bundle_lock
	_sfi_dmask \basereg
	\op \basereg!, \reglist
	.bundle_unlock
.endm


@ For popping multiple registers, including sp.
@ Note you don't want to use this if sp is not touched.
.macro sfi_popm reglist:vararg
	.bundle_lock
	pop \reglist
	_sfi_dmask sp
	.bundle_unlock
.endm


@ Alternative scheme with just one macro.  This has the feature that
@ most of the line is exactly the usual ARM instruction syntax,
@ unmodified.  If the instruction is wholly unmodified, this has the
@ downside that the base register appears twice, thus introducing the
@ possibility of errors where the base register in the first parameter
@ is not the actual base register used in the addressing mode syntax.
@ However, you can instead modify the instruction only slightly: just
@ replace the base register with \B (backslash followed by capital B).
@ Then the instruction syntax remains normal, idiosyncrasies of ARM
@ addressing mode syntaxes all supported--only the base register part of
@ the addressing mode syntax is replaced--and there is no error-prone
@ duplication of the base register.
@ NB: GCC output uses this.
.macro sfi_breg basereg, insn, operands:vararg
	.macro _sfi_breg_doit B
	\insn \operands
	.endm
	.bundle_lock
	_sfi_breg_dmask_\insn \basereg
	_sfi_breg_doit \basereg
	.bundle_unlock
	.purgem _sfi_breg_doit
.endm

.macro _sfi_breg_dmask_define insn, suffix1=, suffix2=
	_sfi_for_each_cond _sfi_breg_dmask_define_1, \insn, \suffix1, \suffix2
	_sfi_breg_dmask_define_1 \(), \insn, \suffix1, \suffix2
.endm

.macro _sfi_breg_dmask_define_1 cond, insn, suffix1=, suffix2=
	.ifb \suffix1
	.macro _sfi_breg_dmask_\insn\cond basereg
	_sfi_dmask \basereg, \cond
	.endm
	.else
	.macro _sfi_breg_dmask_\insn\cond\suffix1\suffix2 basereg
	_sfi_dmask \basereg, \cond
	.endm
	.ifnb \cond
	.macro _sfi_breg_dmask_\insn\suffix1\cond\suffix2 basereg
	_sfi_dmask \basereg, \cond
	.endm
	.ifnb \suffix2
	.macro _sfi_breg_dmask_\insn\suffix1\suffix2\cond basereg
	_sfi_dmask \basereg, \cond
	.endm
	.endif
	.endif
	.endif
.endm

@ We need to name here all the instructions that might appear in sfi_breg,
@ so as to handle all their conditionalized forms.
.macro _sfi_breg_dmask_define_ldst insn
	_sfi_breg_dmask_define \insn
	_sfi_breg_dmask_define \insn, h
	_sfi_breg_dmask_define \insn, sh
	_sfi_breg_dmask_define \insn, b
	_sfi_breg_dmask_define \insn, sb
	_sfi_breg_dmask_define \insn, d
.endm
.macro _sfi_breg_dmask_define_ldmstm insn, suffix=
	_sfi_breg_dmask_define \insn, \suffix
	_sfi_breg_dmask_define \insn, ia, \suffix
	_sfi_breg_dmask_define \insn, fd, \suffix
	_sfi_breg_dmask_define \insn, da, \suffix
	_sfi_breg_dmask_define \insn, fa, \suffix
	_sfi_breg_dmask_define \insn, db, \suffix
	_sfi_breg_dmask_define \insn, ea, \suffix
	_sfi_breg_dmask_define \insn, ib, \suffix
	_sfi_breg_dmask_define \insn, ed, \suffix
.endm
.macro _sfi_breg_dmask_define_neon insn
	_sfi_breg_dmask_define \insn, .8
	_sfi_breg_dmask_define \insn, .16
	_sfi_breg_dmask_define \insn, .32
	_sfi_breg_dmask_define \insn, .64
.endm
_sfi_breg_dmask_define_ldst ldr
_sfi_breg_dmask_define_ldst ldrex
_sfi_breg_dmask_define_ldst ldc
_sfi_breg_dmask_define_ldst str
_sfi_breg_dmask_define_ldst strex
_sfi_breg_dmask_define_ldst stc
_sfi_breg_dmask_define_ldmstm ldm
_sfi_breg_dmask_define_ldmstm stm
_sfi_breg_dmask_define pld
_sfi_breg_dmask_define pldw
_sfi_breg_dmask_define pldi
_sfi_breg_dmask_define_ldmstm vldm
_sfi_breg_dmask_define_ldmstm vldm, .32
_sfi_breg_dmask_define_ldmstm vldm, .64
_sfi_breg_dmask_define_ldmstm vstm
_sfi_breg_dmask_define_ldmstm vstm, .32
_sfi_breg_dmask_define_ldmstm vstm, .64
_sfi_breg_dmask_define fld, s
_sfi_breg_dmask_define fld, d
_sfi_breg_dmask_define fst, s
_sfi_breg_dmask_define fst, d
_sfi_breg_dmask_define_ldmstm fldm, s
_sfi_breg_dmask_define_ldmstm fldm, d
_sfi_breg_dmask_define_ldmstm fstm, s
_sfi_breg_dmask_define_ldmstm fstm, d
_sfi_breg_dmask_define vldr
_sfi_breg_dmask_define vldr, .32
_sfi_breg_dmask_define vldr, .64
_sfi_breg_dmask_define vstr
_sfi_breg_dmask_define vstr, .32
_sfi_breg_dmask_define vstr, .64
_sfi_breg_dmask_define_neon vld1
_sfi_breg_dmask_define_neon vld2
_sfi_breg_dmask_define_neon vld3
_sfi_breg_dmask_define_neon vld4
_sfi_breg_dmask_define_neon vst1
_sfi_breg_dmask_define_neon vst2
_sfi_breg_dmask_define_neon vst3
_sfi_breg_dmask_define_neon vst4
_sfi_breg_dmask_define ldcl
_sfi_breg_dmask_define stcl

.purgem _sfi_breg_dmask_define
.purgem _sfi_breg_dmask_define_1
.purgem _sfi_breg_dmask_define_ldst
.purgem _sfi_breg_dmask_define_ldmstm
.purgem _sfi_breg_dmask_define_neon

@ Macro to precede an instruction that touches sp.
@ NB: GCC output uses this.
.macro sfi_sp insn:vararg
	.bundle_lock
	\insn
	_sfi_dmask sp
	.bundle_unlock
.endm

@ Macro to start a naturally aligned 16-byte constant pool fragment.
@ NB: GCC output uses this.
.macro sfi_constant_barrier
	bkpt 0x5be0
.endm

@ Macro to do a guaranteed safely nonresumable trap.
@ There is no assembler mnemonic for the UDF instruction.
@ This word is chosen so that in ARM it is a UDF instruction
@ and in (little-endian) Thumb-2 it is a UDF followed by a branch-to-self
@ (and vice versa in big-endian Thumb-2).
@ NB: GCC output uses this.
.macro sfi_trap
	@ .inst is like .word, but doesn't emit a $d mapping symbol.
	.inst 0xe7fedef0
.endm

.purgem _sfi_for_each_cond

@ TODO(mcgrathr): examples don't belong here
.if 0
@ Example uses of all the macros (nops just to ease reading of disassembly)
	nop
Tbx:	sfi_bx r0
	nop
Tbl:	sfi_bl foobar
	nop
Tblx:	sfi_blx r1
	nop
Tret:	sfi_ret

	nop
Tloads: sfi_mem ldr r0, r1
	sfi_mem ldr r0, r1, #8
	sfi_mem str r0, r1, #8
	sfi_memib ldr r0, r1, #8
	sfi_memia ldr r0, r1, #8
	sfi_memm ldm r0, {r2, r3, r4}
	sfi_memmia ldmia r0, {r2, r3, r4}
	sfi_memm stm r0, {r2, r3, r4}
	sfi_memmia stmia r0, {r2, r3, r4}
	sfi_popm {r5, sp, lr}

Talternate:
	sfi_breg r1, ldr r0, [r1]
	sfi_breg r1, ldr r0, [r1, #8]
	sfi_breg r1, str r0, [r1, #8]
	sfi_breg r1, ldr r0, [r1, #8]!
	sfi_breg r1, ldr r0, [r1], #8
	sfi_breg r0, ldm r0, {r2, r3, r4}
	sfi_breg r0, ldmia r0!, {r2, r3, r4}
	sfi_breg r0, stm r0, {r2, r3, r4}
	sfi_breg r0, stmia r0!, {r2, r3, r4}
.endif

.pushsection .note.NaCl.ABI.arm, "aG", %note, .note.NaCl.ABI.arm, comdat
	.int 1f - 0f, 3f - 2f, 1
	.balign 4
0:	.string "NaCl"
1:	.balign 4
2:	.string "arm"
3:	.balign 4
.popsection
