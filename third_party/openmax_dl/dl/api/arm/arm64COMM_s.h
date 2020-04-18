// -*- Mode: asm; -*-
//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This file was originally licensed as follows. It has been
//  relicensed with permission from the copyright holders.
//
	
// 
// File Name:  armCOMM_s.h
// OpenMAX DL: v1.0.2
// Last Modified Revision:   13871
// Last Modified Date:       Fri, 09 May 2008
// 
// (c) Copyright 2007-2008 ARM Limited. All Rights Reserved.
// 
// 
//
// ARM optimized OpenMAX common header file
//

	.set	_SBytes, 0	// Number of scratch bytes on stack
	.set	_Workspace, 0	// Stack offset of scratch workspace

	.set	_RRegList, 0	// R saved register list (last register number)
	.set	_DRegList, 0	// D saved register list (last register number)

	// Work out list of D saved registers, like for R registers.
	.macro	_M_GETDREGLIST dreg
	.ifeqs "\dreg", ""
	.set	_DRegList, 0
	.exitm
	.endif

	.ifeqs "\dreg", "d8"
	.set	_DRegList, 8
	.exitm
	.endif

	.ifeqs "\dreg", "d9"
	.set	_DRegList, 9
	.exitm
	.endif

	.ifeqs "\dreg", "d10"
	.set	_DRegList, 10
	.exitm
	.endif

	.ifeqs "\dreg", "d11"
	.set	_DRegList, 11
	.exitm
	.endif

	.ifeqs "\dreg", "d12"
	.set	_DRegList, 12
	.exitm
	.endif

	.ifeqs "\dreg", "d13"
	.set	_DRegList, 13
	.exitm
	.endif

	.ifeqs "\dreg", "d14"
	.set	_DRegList, 14
	.exitm
	.endif

	.ifeqs "\dreg", "d15"
	.set	_DRegList, 15
	.exitm
	.endif

	.warning "Unrecognized saved d register limit: \rreg"
	.endm

//////////////////////////////////////////////////////////
// Function header and footer macros
//////////////////////////////////////////////////////////      
	
        // Function Header Macro    
        // Generates the function prologue
        // Note that functions should all be "stack-moves-once"
        // The FNSTART and FNEND macros should be the only places
        // where the stack moves.
        //    
        //  name  = function name
        //  rreg  = ""   don't stack any registers
        //          "lr" stack "lr" only
        //          "rN" stack registers "r4-rN,lr"
        //  dreg  = ""   don't stack any D registers
        //          "dN" stack registers "d8-dN"
        //
        // Note: ARM Archicture procedure call standard AAPCS
        // states that r4-r11, sp, d8-d15 must be preserved by
        // a compliant function.
	.macro	M_START name, rreg, dreg
	.set	_Workspace, 0

	// Define the function and make it external.
	.global	\name
	.hidden \name
	.section	.text.\name,"ax",%progbits
	.align	4
\name :		
//.fnstart
	// Save specified R registers
	_M_PUSH_RREG

	// Save specified D registers
        _M_GETDREGLIST  \dreg
	_M_PUSH_DREG

	// Ensure size claimed on stack is 16-byte aligned for ARM64
	.if (_SBytes & 15) != 0
	.set	_SBytes, _SBytes + (16 - (_SBytes & 15))
	.endif
	.if _SBytes != 0
		sub	sp, sp, #_SBytes
	.endif	
	.endm

        // Function Footer Macro        
        // Generates the function epilogue
	.macro M_END
	// Restore the stack pointer to its original value on function entry
	.if _SBytes != 0
		add	sp, sp, #_SBytes
	.endif
	// Restore any saved R or D registers.
	_M_RET
	//.fnend	
        // Reset the global stack tracking variables back to their
	// initial values.
	.set _SBytes, 0
	.endm

	// Based on the value of _DRegList, push the specified set of registers 
	// to the stack.
	// The ARM64 ABI says only v8-v15 needs to be saved across calls and only 
	// the lower 64 bits need to be saved.
	.macro _M_PUSH_DREG
	.if _DRegList >= 8
		sub	sp, sp, (_DRegList - 7) * 16	// 16-byte alignment
		str	q8, [sp]
	.endif
	
	.if _DRegList >= 9
		str	q9, [sp, #16]
	.endif
	
	.if _DRegList >= 10
		str	q10, [sp, #32]
	.endif
	
	.if _DRegList >= 11
		str	q11, [sp, #48]
	.endif
	
	.if _DRegList >= 12
		str	q12, [sp, #64]
	.endif
	
	.if _DRegList >= 13
		str	q13, [sp, #80]
	.endif
	
	.if _DRegList >= 14
		str	q14, [sp, #96]
	.endif
	
	.if _DRegList >= 15
		str	q15, [sp, #112]
	.endif
	
	.exitm
	.endm

	// Based on the value of _RRegList, push the specified set of registers 
	// to the stack.
	// The ARM64 ABI says registers r19-r29 needs to be saved across calls.
	// But for the FFT routines, we don't need to save anything, so just
	// preserve the SP and LR.
	.macro _M_PUSH_RREG
	sub	sp, sp, #16
	str	x30, [sp]
	str	x29, [sp, #8]
	.exitm
	.endm

	// The opposite of _M_PUSH_DREG
	.macro  _M_POP_DREG
	.if _DRegList >= 8
		ldr	q8, [sp]
	.endif
	
	.if _DRegList >= 9
		ldr	q9, [sp, #16]
	.endif
	
	.if _DRegList >= 10
		ldr	q10, [sp, #32]
	.endif
	
	.if _DRegList >= 11
		ldr	q11, [sp, #48]
	.endif
	
	.if _DRegList >= 12
		ldr	q12, [sp, #64]
	.endif
	
	.if _DRegList >= 13
		ldr	q13, [sp, #80]
	.endif
	
	.if _DRegList >= 14
		ldr	q14, [sp, #96]
	.endif
	
	.if _DRegList >= 15
		ldr	q15, [sp, #112]
	.endif

	.if _DRegList >= 8
		add	sp, sp, (_DRegList - 7) * 16	// 16-byte alignment
	.endif
	.exitm
	.endm

	// The opposite of _M_PUSH_RREG
	.macro _M_POP_RREG cc
	ldr	x29, [sp, #8]
	ldr	x30, [sp]
	add	sp, sp, #16
	.exitm
	.endm
	
        // Produce function return instructions
	.macro	_M_RET cc
	_M_POP_DREG \cc
	_M_POP_RREG \cc
	ret
	.endm	
	// rsb - reverse subtract
	// compute dst = src2 - src1, useful when src2 is an immediate value
	.macro	rsb	dst, src1, src2
	sub	\dst, \src1, \src2
	neg	\dst, \dst
	.endm
