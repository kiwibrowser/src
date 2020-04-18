//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//
//  This is a modification of armSP_FFT_CToC_SC32_Radix4_unsafe_s.s
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




// Guarding implementation by the processor name


// Import symbols required from other files
// (For example tables)


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
#define grpCount        x7
#define grpCount32      w7
#define pointStep       x8
#define pointStep32     w8
#define outPointStep    x9
#define stepTwiddle     x10
#define setCount        x11
#define srcStep         x12
#define setStep         x13
#define dstStep         x14
#define twStep          x15

// Neon Registers

#define dW1     v0.2s
#define dW1s    v0.s
#define dW2     v1.2s
#define dW2s    v1.s
#define dW3     v2.2s
#define dW3s    v2.s

#define dXr0    v4.2s
#define dXi0    v5.2s
#define dXr1    v6.2s
#define dXi1    v7.2s
#define dXr2    v8.2s
#define dXi2    v9.2s
#define dXr3    v10.2s
#define dXi3    v11.2s
#define dYr0    v12.2s
#define dYi0    v13.2s
#define dYr1    v14.2s
#define dYi1    v15.2s
#define dYr2    v16.2s
#define dYi2    v17.2s
#define dYr3    v18.2s
#define dYi3    v19.2s
#define dZr0    v20.2s
#define dZi0    v21.2s
#define dZr1    v22.2s
#define dZi1    v23.2s
#define dZr2    v24.2s
#define dZi2    v25.2s
#define dZr3    v26.2s
#define dZi3    v27.2s

        .macro FFTSTAGE scaled, inverse , name

        // Define stack arguments

        // Move args values into our work registers
        ldr     subFFTNum, [pSubFFTNum]
        ldr     subFFTSize, [pSubFFTSize]

        // Update grpCount and grpSize rightaway inorder to reuse
        // pGrpCount and pGrpSize regs

        LSL     grpCount,subFFTSize,#2
        LSR     subFFTNum,subFFTNum,#2
        MOV     subFFTSize,grpCount

        ld1      {dW1},[pTwiddle]                    //[wi | wr]
        // pT0+1 increments pT0 by 8 bytes
        // pT0+pointStep = increment of 8*pointStep bytes = 2*grpSize bytes
        lsl     pointStep,subFFTNum, #1

        // pOut0+1 increments pOut0 by 8 bytes
        // pOut0+outPointStep == increment of 8*outPointStep bytes
        //   = 2*size bytes

        MOV     stepTwiddle,#0
        ld1      {dW2},[pTwiddle]                    //[wi | wr]
        smull   outPointStep,grpCount32,pointStep32

        LSL     pointStep,pointStep,#2             // 2*grpSize

        ld1      {dW3},[pTwiddle]                  //[wi | wr]
        lsl     srcStep,pointStep, #1              // srcStep = 2*pointStep

        ADD     setStep,srcStep,pointStep          // setStep = 3*pointStep

        rsb     setStep,setStep,#0                 // setStep = - 3*pointStep
        SUB     srcStep,srcStep,#16                // srcStep = 2*pointStep-16

        lsl     dstStep,outPointStep, #1

        ADD     dstStep,dstStep,outPointStep       // dstStep = 3*outPointStep
        // dstStep = - 3*outPointStep+16
        rsb     dstStep,dstStep,#16


radix4GrpLoop\name :

        ld2     {dXr0,dXi0},[pSrc],pointStep       //  data[0]
        ADD      stepTwiddle,stepTwiddle,pointStep
        ld2     {dXr1,dXi1},[pSrc],pointStep       //  data[1]
        // set pTwiddle to the first point
        ADD      pTwiddle,pTwiddle,stepTwiddle
        ld2     {dXr2,dXi2},[pSrc],pointStep       //  data[2]
        lsl      twStep,stepTwiddle, #2

        //  data[3] & update pSrc for the next set
        ld2     {dXr3,dXi3},[pSrc],setStep
        SUB      twStep,stepTwiddle,twStep         // twStep = -3*stepTwiddle

        lsr      setCount,pointStep, #3

        // set pSrc to data[0] of the next set
        ADD     pSrc,pSrc,#16
        // increment to data[1] of the next set
        ADD     pSrc,pSrc,pointStep


        // Loop on the sets

radix4SetLoop\name :



        .ifeqs  "\inverse", "TRUE"
            fmul   dZr1,dXr1,dW1s[0]
            fmul   dZi1,dXi1,dW1s[0]
            fmul   dZr2,dXr2,dW2s[0]
            fmul   dZi2,dXi2,dW2s[0]
            fmul   dZr3,dXr3,dW3s[0]
            fmul   dZi3,dXi3,dW3s[0]

            fmla   dZr1,dXi1,dW1s[1]                // real part
            fmls   dZi1,dXr1,dW1s[1]                // imag part

            //  data[1] for next iteration
            ld2     {dXr1,dXi1},[pSrc],pointStep

            fmla   dZr2,dXi2,dW2s[1]                // real part
            fmls   dZi2,dXr2,dW2s[1]                // imag part

            //  data[2] for next iteration
            ld2     {dXr2,dXi2},[pSrc],pointStep

            fmla   dZr3,dXi3,dW3s[1]                // real part
            fmls   dZi3,dXr3,dW3s[1]                // imag part
        .else
            fmul   dZr1,dXr1,dW1s[0]
            fmul   dZi1,dXi1,dW1s[0]
            fmul   dZr2,dXr2,dW2s[0]
            fmul   dZi2,dXi2,dW2s[0]
            fmul   dZr3,dXr3,dW3s[0]
            fmul   dZi3,dXi3,dW3s[0]

            fmls   dZr1,dXi1,dW1s[1]                // real part
            fmla   dZi1,dXr1,dW1s[1]                // imag part

            //  data[1] for next iteration
            ld2     {dXr1,dXi1},[pSrc],pointStep

            fmls   dZr2,dXi2,dW2s[1]                // real part
            fmla   dZi2,dXr2,dW2s[1]                // imag part

            //  data[2] for next iteration
            ld2     {dXr2,dXi2},[pSrc],pointStep

            fmls   dZr3,dXi3,dW3s[1]                // real part
            fmla   dZi3,dXr3,dW3s[1]                // imag part
        .endif

        //  data[3] & update pSrc to data[0]
        // But don't read on the very last iteration because that reads past 
        // the end of pSrc. The last iteration is grpCount = 4, setCount = 2.
        cmp     grpCount, #4
        
        b.ne    skipUpdate\name
        cmp     setCount, #2
        b.ne    skipUpdate\name
        add     pSrc, pSrc, setStep
        beq     radix4SkipRead\name
skipUpdate\name:
        ld2     {dXr3,dXi3},[pSrc],setStep
radix4SkipRead\name:

        SUBS    setCount,setCount,#2

        // finish first stage of 4 point FFT
        // fadd    qY0,qX0,qZ2
        // fsub    qY2,qX0,qZ2
        fadd    dYr0,dXr0,dZr2
        fsub    dYr2,dXr0,dZr2
        fadd    dYi0,dXi0,dZi2
        fsub    dYi2,dXi0,dZi2

        //  data[0] for next iteration
        ld2     {dXr0,dXi0},[pSrc], #16
        // fadd    qY1,qZ1,qZ3
        // fsub    qY3,qZ1,qZ3
        fadd    dYr1,dZr1,dZr3
        fsub    dYr3,dZr1,dZr3
        fadd    dYi1,dZi1,dZi3
        fsub    dYi3,dZi1,dZi3

        // finish second stage of 4 point FFT

        // fsub    qZ0,qY2,qY1
        fsub    dZr0,dYr2,dYr1
        fsub    dZi0,dYi2,dYi1

        .ifeqs  "\inverse", "TRUE"

            fadd    dZr3,dYr0,dYi3
            st2     {dZr0,dZi0},[pDst],outPointStep
            fsub    dZi3,dYi0,dYr3

            // fadd    qZ2,qY2,qY1
            fadd    dZr2,dYr2,dYr1
            fadd    dZi2,dYi2,dYi1

            st2     {dZr3,dZi3},[pDst],outPointStep

            fsub    dZr1,dYr0,dYi3
            st2     {dZr2,dZi2},[pDst],outPointStep
            fadd    dZi1,dYi0,dYr3

            st2     {dZr1,dZi1},[pDst],dstStep


        .else

            fsub    dZr1,dYr0,dYi3
            st2     {dZr0,dZi0},[pDst],outPointStep
            fadd    dZi1,dYi0,dYr3

            // fadd    qZ2,qY2,qY1
            fadd    dZr2,dYr2,dYr1
            fadd    dZi2,dYi2,dYi1

            st2     {dZr1,dZi1},[pDst],outPointStep

            fadd    dZr3,dYr0,dYi3
            st2     {dZr2,dZi2},[pDst],outPointStep
            fsub    dZi3,dYi0,dYr3

            st2     {dZr3,dZi3},[pDst],dstStep


        .endif

        // increment to data[1] of the next set
        ADD     pSrc,pSrc,pointStep
        BGT     radix4SetLoop\name


        ld1      {dW1},[pTwiddle],stepTwiddle    //[wi | wr]
        // subtract 4 since grpCount multiplied by 4
        SUBS    grpCount,grpCount,#4
        ld1      {dW2},[pTwiddle],stepTwiddle    //[wi | wr]
        // increment pSrc for the next grp
        ADD     pSrc,pSrc,srcStep
        ld1      {dW3},[pTwiddle],twStep         //[wi | wr]
        BGT     radix4GrpLoop\name

        str     subFFTNum, [pSubFFTNum]
        str     subFFTSize, [pSubFFTSize]

        .endm


        M_START armSP_FFTFwd_CToC_FC32_Radix4_OutOfPlace,,d15
            FFTSTAGE "FALSE","FALSE",FWD
        M_END


        M_START armSP_FFTInv_CToC_FC32_Radix4_OutOfPlace,,d15
            FFTSTAGE "FALSE","TRUE",INV
        M_END


        .end
