//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of armSP_FFT_CToC_SC32_Radix2_ls_unsafe_s.S
//  to support float instead of SC32.
//

//
// Description:
// Compute the last stage of a Radix 2 DIT in-order out-of-place FFT
// stage for a N point complex signal.
//
//


// Include standard headers

#include "dl/api/arm/arm64COMM_s.h"
#include "dl/api/arm/omxtypes_s.h"


// Import symbols required from other files
// (For example tables)




// Set debugging level
//DEBUG_ON    SETL {TRUE}


// Guarding implementation by the processor name


//Input Registers

#define pSrc            x0
#define pDst            x1
#define pTwiddle        x2
#define	pSubFFTNum	x3
#define pSubFFTSize	x4	


//Output Registers


//Local Scratch Registers


#define subFFTNum       x5
#define subFFTSize      x6
#define outPointStep    x8
#define grpCount        x9
#define dstStep         x10

// Neon Registers

#define dWr     v0.2s
#define dWi     v1.2s
#define dXr0    v2.2s
#define dXi0    v3.2s
#define dXr1    v4.2s
#define dXi1    v5.2s
#define dYr0    v6.2s
#define dYi0    v7.2s
#define dYr1    v8.2s
#define dYi1    v9.2s
#define qT0     v10.2s
#define qT1     v12.2s

        .macro FFTSTAGE scaled, inverse, name

        // Move parameters into our work registers
        ldr     subFFTSize, [pSubFFTSize]

        lsl     outPointStep, subFFTSize, #3

        // Update grpCount and grpSize rightaway

        MOV     subFFTNum,#1                          //after the last stage
        LSL     grpCount,subFFTSize,#1

        // update subFFTSize for the next stage
        MOV     subFFTSize,grpCount

        rsb     dstStep,outPointStep,#16

        // Loop on 2 grps at a time for the last stage

radix2lsGrpLoop\name :
        // dWr = [pTwiddle[0].Re, pTwiddle[1].Re]
        // dWi = [pTwiddle[0].Im, pTwiddle[1].Im]
        ld2     {dWr,dWi},[pTwiddle], #16

        // dXr0 = [pSrc[0].Re, pSrc[2].Re]
        // dXi0 = [pSrc[0].Im, pSrc[2].Im]
        // dXr1 = [pSrc[1].Re, pSrc[3].Re]
        // dXi1 = [pSrc[1].Im, pSrc[3].Im]
        ld4     {dXr0,dXi0,dXr1,dXi1}, [pSrc], #32

        SUBS    grpCount,grpCount,#4                  // grpCount is multiplied by 2

        .ifeqs  "\inverse", "TRUE"
            fmul   qT0,dWr,dXr1
            fmla   qT0,dWi,dXi1                       // real part
            fmul   qT1,dWr,dXi1
            fmls   qT1,dWi,dXr1                       // imag part

        .else

            fmul   qT0,dWr,dXr1
            fmls   qT0,dWi,dXi1                       // real part
            fmul   qT1,dWr,dXi1
            fmla   qT1,dWi,dXr1                       // imag part

        .endif

        fsub    dYr0,dXr0,qT0
        fsub    dYi0,dXi0,qT1
        fadd    dYr1,dXr0,qT0
        fadd    dYi1,dXi0,qT1

        st2     {dYr0,dYi0},[pDst],outPointStep
        st2     {dYr1,dYi1},[pDst],dstStep            // dstStep =  step = -outPointStep + 16

        BGT     radix2lsGrpLoop\name


        .endm



        M_START armSP_FFTFwd_CToC_FC32_Radix2_ls_OutOfPlace,,d12
        FFTSTAGE "FALSE","FALSE",fwd
        M_END



        M_START armSP_FFTInv_CToC_FC32_Radix2_ls_OutOfPlace,,d12
        FFTSTAGE "FALSE","TRUE",inv
        M_END

        .end
