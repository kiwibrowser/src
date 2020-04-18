//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of armSP_FFT_CToC_SC32_Radix4_ls_unsafe_s.s
//  to support float instead of SC32.
//

//
// Description:
// Compute a Radix 4 FFT stage for a N point complex signal
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


// Import symbols required from other files
// (For example tables)
    //IMPORT  armAAC_constTable

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
#define grpTwStep       x13
#define stepTwiddle     x14
#define twStep          x15
#define step16          x11
#define step24          x12


// Neon Registers

#define dButterfly1Real02       v0.2s
#define dButterfly1Real028b     v0.8b
#define dButterfly1Imag02       v1.2s
#define dButterfly1Imag028b     v1.8b
#define dButterfly1Real13       v2.2s
#define dButterfly1Real138b     v2.8b
#define dButterfly1Imag13       v3.2s
#define dButterfly1Imag138b     v3.8b
#define dButterfly2Real02       v4.2s
#define dButterfly2Imag02       v5.2s
#define dButterfly2Real13       v6.2s
#define dButterfly2Imag13       v7.2s
#define dXr0                    v0.2s
#define dXi0                    v1.2s
#define dXr08b                  v0.8b
#define dXi08b                  v1.8b
#define dXr1                    v2.2s
#define dXi1                    v3.2s
#define dXr2                    v4.2s
#define dXi2                    v5.2s
#define dXr3                    v6.2s
#define dXi3                    v7.2s

#define dYr0                    v16.2s
#define dYi0                    v17.2s
#define dYr1                    v18.2s
#define dYi1                    v19.2s
#define dYr2                    v20.2s
#define dYi2                    v21.2s
#define dYr3                    v22.2s
#define dYi3                    v23.2s

#define dW1r                    v8.2s
#define dW1i                    v9.2s
#define dW2r                    v10.2s
#define dW2r8b                  v10.8b
#define dW2i                    v11.2s
#define dW3r                    v12.2s
#define dW3r8b                  v12.8b
#define dW3i                    v13.2s

#define dZr0                    v14.2s
#define dZi0                    v15.2s
#define dZr08b                  v14.8b
#define dZi08b                  v15.8b
#define dZr1                    v26.2s
#define dZi1                    v27.2s
#define dZr2                    v28.2s
#define dZi2                    v29.2s
#define dZr3                    v30.2s
#define dZi3                    v31.2s

#define dZip                    v24.2s
#define dZip8b                  v24.8b

        .macro FFTSTAGE scaled, inverse , name

        // Define stack arguments

        // Move args values into our work registers
        ldr     subFFTNum, [pSubFFTNum]
        ldr     subFFTSize, [pSubFFTSize]

        // pOut0+1 increments pOut0 by 8 bytes
        // pOut0+outPointStep == increment of 8*outPointStep bytes
        lsl     outPointStep,subFFTSize, #3

        // Update grpCount and grpSize rightaway

        ld2    {dW1r,dW1i},[pTwiddle]             // [wi|wr]
        MOV     step16,#16
        LSL     grpCount,subFFTSize,#2

        ld1    {dW2r},[pTwiddle]                  // [wi|wr]
        MOV     subFFTNum,#1                      //after the last stage

        ld1    {dW3r},[pTwiddle],step16           // [wi|wr]
        MOV     stepTwiddle,#0

        ld1    {dW2i},[pTwiddle],#8               // [wi|wr]
        SUB     grpTwStep,stepTwiddle,#8          // grpTwStep = -8 to start with

        // update subFFTSize for the next stage
        MOV     subFFTSize,grpCount
        ld1    {dW3i},[pTwiddle],grpTwStep        // [wi|wr]
        lsl     dstStep,outPointStep, #1

        // AC.r AC.i BD.r BD.i
        ld4     {dButterfly1Real02,dButterfly1Imag02,dButterfly1Real13,dButterfly1Imag13},[pSrc], #32
        ADD     dstStep,dstStep,outPointStep      // dstStep = 3*outPointStep

        rsb     dstStep,dstStep,#16               // dstStep = - 3*outPointStep+16
        MOV     step24,#24

        // AC.r AC.i BD.r BD.i
        ld4     {dButterfly2Real02,dButterfly2Imag02,dButterfly2Real13,dButterfly2Imag13},[pSrc], #32


        // Process two groups at a time

radix4lsGrpLoop\name :

        // VZIP    dW2r,dW2i
        zip1    dZip, dW2r, dW2i
        zip2    dW2i, dW2r, dW2i
        mov     dW2r8b, dZip8b

        ADD     stepTwiddle,stepTwiddle,#16

        // VZIP    dW3r,dW3i
        zip1    dZip, dW3r,dW3i
        zip2    dW3i, dW3r, dW3i
        mov     dW3r8b, dZip8b
        ADD     grpTwStep,stepTwiddle,#4

        // VUZP     dButterfly1Real13, dButterfly2Real13      // B.r D.r
        uzp1     dZip, dButterfly1Real13, dButterfly2Real13   // B.r D.r
        uzp2     dButterfly2Real13, dButterfly1Real13, dButterfly2Real13   // B.r D.r
        mov      dButterfly1Real138b, dZip8b

        SUB     twStep,stepTwiddle,#16                        // -16+stepTwiddle

        // VUZP     dButterfly1Imag13, dButterfly2Imag13      // B.i D.i
        uzp1     dZip, dButterfly1Imag13, dButterfly2Imag13   // B.i D.i
        uzp2     dButterfly2Imag13, dButterfly1Imag13, dButterfly2Imag13   // B.i D.i
        mov      dButterfly1Imag138b, dZip8b
        lsl     grpTwStep,grpTwStep,#1

        // VUZP     dButterfly1Real02, dButterfly2Real02      // A.r C.r
        uzp1     dZip, dButterfly1Real02, dButterfly2Real02   // A.r C.r
        uzp2     dButterfly2Real02, dButterfly1Real02, dButterfly2Real02   // A.r C.r
        mov      dButterfly1Real028b, dZip8b
        rsb     grpTwStep,grpTwStep,#0                        // -8-2*stepTwiddle

        // VUZP     dButterfly1Imag02, dButterfly2Imag02      // A.i C.i
        uzp1     dZip, dButterfly1Imag02, dButterfly2Imag02   // A.i C.i
        uzp2     dButterfly2Imag02, dButterfly1Imag02, dButterfly2Imag02   // A.i C.i
        mov      dButterfly1Imag028b, dZip8b


        // grpCount is multiplied by 4
        SUBS    grpCount,grpCount,#8

        .ifeqs  "\inverse", "TRUE"
            fmul   dZr1,dW1r,dXr1
            fmla   dZr1,dW1i,dXi1                       // real part
            fmul   dZi1,dW1r,dXi1
            fmls   dZi1,dW1i,dXr1                       // imag part

        .else

            fmul   dZr1,dW1r,dXr1
            fmls   dZr1,dW1i,dXi1                       // real part
            fmul   dZi1,dW1r,dXi1
            fmla   dZi1,dW1i,dXr1                       // imag part

        .endif

        ld2    {dW1r,dW1i},[pTwiddle],stepTwiddle       // [wi|wr]

        .ifeqs  "\inverse", "TRUE"
            fmul   dZr2,dW2r,dXr2
            fmla   dZr2,dW2i,dXi2                       // real part
            fmul   dZi2,dW2r,dXi2
            ld1   {dW2r},[pTwiddle],step16              // [wi|wr]
            fmls   dZi2,dW2i,dXr2                       // imag part

        .else

            fmul   dZr2,dW2r,dXr2
            fmls   dZr2,dW2i,dXi2                       // real part
            fmul   dZi2,dW2r,dXi2
            ld1    {dW2r},[pTwiddle],step16             // [wi|wr]
            fmla   dZi2,dW2i,dXr2                       // imag part

        .endif


        ld1    {dW2i},[pTwiddle],twStep                 // [wi|wr]

        // move qX0 so as to load for the next iteration
        // MOV     qZ0,qX0
        mov     dZr08b, dXr08b
        mov     dZi08b, dXi08b

        .ifeqs  "\inverse", "TRUE"
            fmul   dZr3,dW3r,dXr3
            fmla   dZr3,dW3i,dXi3                       // real part
            fmul   dZi3,dW3r,dXi3
            ld1    {dW3r},[pTwiddle],step24
            fmls   dZi3,dW3i,dXr3                       // imag part

        .else

            fmul   dZr3,dW3r,dXr3
            fmls   dZr3,dW3i,dXi3                       // real part
            fmul   dZi3,dW3r,dXi3
            ld1    {dW3r},[pTwiddle],step24
            fmla   dZi3,dW3i,dXr3                       // imag part

        .endif

        ld1    {dW3i},[pTwiddle],grpTwStep              // [wi|wr]

        // Don't do the load on the last iteration so we don't read past the end
        // of pSrc.
        bne     skipIncrement\name
        add     pSrc, pSrc, #64
skipIncrement\name:     
        beq     radix4lsSkipRead\name
        // AC.r AC.i BD.r BD.i
        ld4     {dButterfly1Real02,dButterfly1Imag02,dButterfly1Real13,dButterfly1Imag13},[pSrc], #32

        // AC.r AC.i BD.r BD.i
        ld4     {dButterfly2Real02,dButterfly2Imag02,dButterfly2Real13,dButterfly2Imag13},[pSrc], #32
radix4lsSkipRead\name:

        // finish first stage of 4 point FFT

        // fadd    qY0,qZ0,qZ2
        fadd    dYr0,dZr0,dZr2
        fadd    dYi0,dZi0,dZi2
        // fsub    qY2,qZ0,qZ2
        fsub    dYr2,dZr0,dZr2
        fsub    dYi2,dZi0,dZi2
        // fadd    qY1,qZ1,qZ3
        fadd    dYr1,dZr1,dZr3
        fadd    dYi1,dZi1,dZi3
        // fsub    qY3,qZ1,qZ3
        fsub    dYr3,dZr1,dZr3
        fsub    dYi3,dZi1,dZi3


        // finish second stage of 4 point FFT

        .ifeqs  "\inverse", "TRUE"

            // fsub    qZ0,qY2,qY1
            fsub    dZr0,dYr2,dYr1
            fsub    dZi0,dYi2,dYi1
            fadd    dZr3,dYr0,dYi3
            st2    {dZr0,dZi0},[pDst],outPointStep
            fsub    dZi3,dYi0,dYr3

            // fadd    qZ2,qY2,qY1
            fadd    dZr2,dYr2,dYr1
            fadd    dZi2,dYi2,dYi1

            st2    {dZr3,dZi3},[pDst],outPointStep

            fsub    dZr1,dYr0,dYi3
            st2    {dZr2,dZi2},[pDst],outPointStep
            fadd    dZi1,dYi0,dYr3

            // dstStep = -outPointStep + 16
            st2    {dZr1,dZi1},[pDst],dstStep


        .else

            // fsub    qZ0,qY2,qY1
            fsub    dZr0,dYr2,dYr1
            fsub    dZi0,dYi2,dYi1

            fsub    dZr1,dYr0,dYi3
            st2    {dZr0,dZi0},[pDst],outPointStep
            fadd    dZi1,dYi0,dYr3

            // fadd    qZ2,qY2,qY1
            fadd    dZr2,dYr2,dYr1
            fadd    dZi2,dYi2,dYi1

            st2    {dZr1,dZi1},[pDst],outPointStep

            fadd    dZr3,dYr0,dYi3
            st2    {dZr2,dZi2},[pDst],outPointStep
            fsub    dZi3,dYi0,dYr3

            // dstStep = -outPointStep + 16
            st2    {dZr3,dZi3},[pDst],dstStep


        .endif

        BGT     radix4lsGrpLoop\name

        .endm


        M_START armSP_FFTFwd_CToC_FC32_Radix4_ls_OutOfPlace,,d15
        FFTSTAGE "FALSE","FALSE",fwd
        M_END


        M_START armSP_FFTInv_CToC_FC32_Radix4_ls_OutOfPlace,,d15
        FFTSTAGE "FALSE","TRUE",inv
        M_END


        .end
