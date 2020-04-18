//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of armSP_FFT_CToC_SC32_Radix2_fs_unsafe_s.S
//  to support float instead of SC32.
//

//
// Description:
// Compute the first stage of a Radix 2 DIT in-order out-of-place FFT
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
#define pointStep       x7
#define outPointStep    x7
#define grpSize         x8
#define setCount        x8
#define step            x9
#define dstStep         x9

// Neon Registers
#define dX0     v0.2s
#define dX1     v1.2s
#define dY0     v2.2s
#define dY1     v3.2s

        .macro FFTSTAGE scaled, inverse, name

        // Define stack arguments

        // Move args values into our work registers
        ldr     subFFTNum, [pSubFFTNum]
        ldr     subFFTSize, [pSubFFTSize]

        // update subFFTSize and subFFTNum into RN6 and RN7 for the next stage


        MOV        subFFTSize,#2
        LSR        grpSize,subFFTNum,#1
        MOV        subFFTNum,grpSize


        // pT0+1 increments pT0 by 8 bytes
        // pT0+pointStep = increment of 8*pointStep bytes = 4*grpSize bytes
        // Note: outPointStep = pointStep for firststage
        // Note: setCount = grpSize/2 (reuse the updated grpSize for setCount)

        lsl     pointStep, grpSize, #3
        rsb     step, pointStep, #8

        // Loop on the sets for grp zero

grpZeroSetLoop\name :

        LD1    {dX0},[pSrc],pointStep
        LD1    {dX1},[pSrc],step                   // step = -pointStep + 8

        SUBS    setCount,setCount,#1

        fadd    dY0,dX0,dX1
        fsub    dY1,dX0,dX1

        ST1    {dY0},[pDst],outPointStep
        // dstStep =  step = -pointStep + 8
        ST1    {dY1},[pDst],dstStep

        BGT     grpZeroSetLoop\name


        // Save subFFTNum and subFFTSize for next stage
        str     subFFTNum, [pSubFFTNum]
        str     subFFTSize, [pSubFFTSize]

        .endm



        M_START armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace
        FFTSTAGE "FALSE","FALSE",fwd
        M_END



        M_START armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace
        FFTSTAGE "FALSE","TRUE",inv
        M_END

        .end
