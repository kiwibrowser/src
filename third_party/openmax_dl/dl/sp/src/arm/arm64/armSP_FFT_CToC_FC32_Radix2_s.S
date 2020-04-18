//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of armSP_FFT_CToC_SC32_Radix2_unsafe_s.s
//  to support float instead of SC32.
//

// Description:
// Compute a Radix 2 DIT in-order out-of-place FFT stage for an N point
// complex signal.  This handles the general stage, not the first or last
// stage.
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
#define pointStep       x9
#define pointStep32     w9
#define grpCount        x10
#define grpCount32      w10
#define setCount        x13
#define step            x15
#define dstStep         x11

// Neon Registers

#define dW      v0.2s
#define dWs     v0.s
#define dX0     v2.2s
#define dX1     v3.2s
#define dX2     v4.2s
#define dX3     v5.2s
#define dY0     v6.2s
#define dY1     v7.2s
#define dY2     v8.2s
#define dY3     v9.2s
#define qT0     v10.2s
#define qT1     v11.2s

        .macro FFTSTAGE scaled, inverse, name

        // Define stack arguments

        // Move args values into our work registers
        ldr     subFFTNum, [pSubFFTNum]
        ldr     subFFTSize, [pSubFFTSize]

        // Update grpCount and grpSize rightaway inorder to reuse pGrpCount
        // and pGrpSize regs

        LSR     subFFTNum,subFFTNum,#1                 //grpSize
        LSL     grpCount,subFFTSize,#1


        // pT0+1 increments pT0 by 8 bytes
        // pT0+pointStep = increment of 8*pointStep bytes = 4*grpSize bytes
        lsl     pointStep, subFFTNum, #2

        // update subFFTSize for the next stage
        MOV     subFFTSize,grpCount

        // pOut0+1 increments pOut0 by 8 bytes
        // pOut0+outPointStep == increment of 8*outPointStep bytes =
        //    4*size bytes
        smull   outPointStep, grpCount32, pointStep32

        LSL     pointStep,pointStep,#1


        rsb      step,pointStep,#16
        rsb      dstStep,outPointStep,#16

        // Loop on the groups

radix2GrpLoop\name :
        lsr     setCount, pointStep, #3
        LD1     {dW},[pTwiddle],pointStep              //[wi | wr]


        // Loop on the sets


radix2SetLoop\name :


        // point0: dX0-real part dX1-img part
        LD2    {dX0,dX1},[pSrc],pointStep
        // point1: dX2-real part dX3-img part
        LD2    {dX2,dX3},[pSrc],step

        SUBS    setCount,setCount,#2

        .ifeqs  "\inverse", "TRUE"
            fmul   qT0,dX2,dWs[0]
            fmla   qT0,dX3,dWs[1]                       // real part
            fmul   qT1,dX3,dWs[0]
            fmls   qT1,dX2,dWs[1]                       // imag part

        .else

            fmul   qT0,dX2,dWs[0]
            fmls   qT0,dX3,dWs[1]                       // real part
            fmul   qT1,dX3,dWs[0]
            fmla   qT1,dX2,dWs[1]                       // imag part

        .endif

        fsub    dY0,dX0,qT0
        fsub    dY1,dX1,qT1
        fadd    dY2,dX0,qT0
        fadd    dY3,dX1,qT1

        st2    {dY0,dY1},[pDst],outPointStep
        // dstStep = -outPointStep + 16
        st2    {dY2,dY3},[pDst],dstStep

        BGT     radix2SetLoop\name

        SUBS    grpCount,grpCount,#2
        ADD     pSrc,pSrc,pointStep
        BGT     radix2GrpLoop\name


        str     subFFTNum, [pSubFFTNum]
        str     subFFTSize, [pSubFFTSize]
        .endm



        M_START armSP_FFTFwd_CToC_FC32_Radix2_OutOfPlace,,d11
        FFTSTAGE "FALSE","FALSE",FWD
        M_END



        M_START armSP_FFTInv_CToC_FC32_Radix2_OutOfPlace,,d11
        FFTSTAGE "FALSE","TRUE",INV
        M_END


        .end
