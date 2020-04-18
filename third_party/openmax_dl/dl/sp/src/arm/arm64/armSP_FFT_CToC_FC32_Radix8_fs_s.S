//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of armSP_FFT_CToC_FC32_Radix8_fs_unsafe_s.s
//  to support float instead of SC32.
//

//
// Description:
// Compute a first stage Radix 8 FFT stage for a N point complex signal
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
#define grpSize         x7
// Reuse grpSize as setCount
#define setCount        x7
#define pointStep       x8
#define outPointStep    x8
#define setStep         x9
#define step1           x10
#define step2           x11
#define t0              w12


// Neon Registers

#define dXr0    v0.2s
#define dXi0    v1.2s
#define dXr1    v2.2s
#define dXi1    v3.2s
#define dXr2    v4.2s
#define dXi2    v5.2s
#define dXr3    v6.2s
#define dXi3    v7.2s
#define dXr4    v8.2s
#define dXi4    v9.2s
#define dXr5    v10.2s
#define dXi5    v11.2s
#define dXr6    v12.2s
#define dXi6    v13.2s
#define dXr7    v14.2s
#define dXi7    v15.2s
#define qX0     v0.4s
#define qX1     v1.4s
#define qX2     v2.4s
#define qX3     v3.4s
#define qX4     v4.4s
#define qX5     v5.4s
#define qX6     v6.4s
#define qX7     v7.4s

#define dUr0    v16.2s
#define dUi0    v17.2s
#define dUr2    v18.2s
#define dUi2    v19.2s
#define dUr4    v20.2s
#define dUi4    v21.2s
#define dUr6    v22.2s
#define dUi6    v23.2s
#define dUr1    v24.2s
#define dUi1    v25.2s
#define dUr3    v26.2s
#define dUi3    v27.2s
#define dUr5    v28.2s
#define dUi5    v29.2s
// reuse dXr7 and dXi7
#define dUr7    v30.2s
#define dUi7    v31.2s
#define qU0     v8.4s
#define qU1     v12.4s
#define qU2     v9.4s
#define qU3     v13.4s
#define qU4     v10.4s
#define qU5     v14.4s
#define qU6     v11.4s
#define qU7     v15.4s


#define dVr0    v24.2s
#define dVi0    v25.2s
#define dVr2    v26.2s
#define dVi2    v27.2s
#define dVr4    v28.2s
#define dVi4    v29.2s
#define dVr6    v30.2s
#define dVi6    v31.2s
#define dVr1    v16.2s
#define dVi1    v17.2s
#define dVr3    v18.2s
#define dVi3    v19.2s
#define dVr5    v20.2s
#define dVi5    v21.2s
#define dVr7    v22.2s
#define dVi7    v23.2s
#define qV0     v12.4s
#define qV1     v8.4s
#define qV2     v13.4s
#define qV3     v9.4s
#define qV4     v14.4s
#define qV5     v10.4s
#define qV6     v15.4s
#define qV7     v11.4s

#define dYr0    v16.2s
#define dYi0    v17.2s
#define dYr2    v18.2s
#define dYi2    v19.2s
#define dYr4    v20.2s
#define dYi4    v21.2s
#define dYr6    v22.2s
#define dYi6    v23.2s
#define dYr1    v24.2s
#define dYi1    v25.2s
#define dYr3    v26.2s
#define dYi3    v27.2s
#define dYr5    v28.2s
#define dYi5    v29.2s
#define dYr7    v30.2s
#define dYi7    v31.2s
#define qY0     v8.4s
#define qY1     v12.4s
#define qY2     v9.4s
#define qY3     v13.4s
#define qY4     v10.4s
#define qY5     v14.4s
#define qY6     v11.4s
#define qY7     v15.4s

#define dT0     v14.2s
#define dT0s    v14.s
#define dT1     v15.2s

        .macro FFTSTAGE scaled, inverse, name

        // Define stack arguments

        // Move args values into our work registers
        ldr     subFFTNum, [pSubFFTNum]
        ldr     subFFTSize, [pSubFFTSize]

        // Update pSubFFTSize and pSubFFTNum regs
        // subFFTSize = 1 for the first stage

        movz    t0, 0x3f35, lsl #16               // High half word of sqrt(1/2).
        movk    t0, 0x04f3                        // Low half word of sqrt(1/2).
        MOV     subFFTSize,#8

        // Note: setCount = subFFTNum/8 (reuse the grpSize reg for setCount)
        LSR     grpSize,subFFTNum,#3
        MOV     subFFTNum,grpSize


        // pT0+1 increments pT0 by 8 bytes
        // pT0+pointStep = increment of 8*pointStep bytes = grpSize bytes
        // Note: outPointStep = pointStep for firststage

        lsl     pointStep,grpSize, #3


        // Calculate the step of input data for the next set
        //MOV     step1,pointStep,LSL #1             // step1 = 2*pointStep
        ld2     {dXr0,dXi0},[pSrc],pointStep         //  data[0]
        lsl     step1,grpSize, #4
        lsl     step2,pointStep, #3

        ld2     {dXr1,dXi1},[pSrc],pointStep         //  data[1]
        SUB     step2,step2,pointStep                // step2 = 7*pointStep
        // setStep = - 7*pointStep+16
        rsb     setStep,step2,#16

        ld2     {dXr2,dXi2},[pSrc],pointStep         //  data[2]
        ld2     {dXr3,dXi3},[pSrc],pointStep         //  data[3]
        ld2     {dXr4,dXi4},[pSrc],pointStep         //  data[4]
        ld2     {dXr5,dXi5},[pSrc],pointStep         //  data[5]
        ld2     {dXr6,dXi6},[pSrc],pointStep         //  data[6]
        //  data[7] & update pSrc for the next set
        //  setStep = -7*pointStep + 16
        ld2     {dXr7,dXi7},[pSrc],setStep
        // grp = 0 a special case since all the twiddle factors are 1
        // Loop on the sets

radix8fsGrpZeroSetLoop\name :

        // Decrement setcount
        SUBS    setCount,setCount,#2


        // finish first stage of 8 point FFT

        // fadd    qU0,qX0,qX4
        // fadd    qU2,qX1,qX5
        // fadd    qU4,qX2,qX6
        // fadd    qU6,qX3,qX7
        fadd    dUr0,dXr0,dXr4
        fadd    dUr2,dXr1,dXr5
        fadd    dUr4,dXr2,dXr6
        fadd    dUr6,dXr3,dXr7
        fadd    dUi0,dXi0,dXi4
        fadd    dUi2,dXi1,dXi5
        fadd    dUi4,dXi2,dXi6
        fadd    dUi6,dXi3,dXi7

        // finish second stage of 8 point FFT

        // fadd    qV0,qU0,qU4
        // fsub    qV2,qU0,qU4
        // fadd    qV4,qU2,qU6
        // fsub    qV6,qU2,qU6
        fadd    dVr0,dUr0,dUr4
        fsub    dVr2,dUr0,dUr4
        fadd    dVr4,dUr2,dUr6
        fsub    dVr6,dUr2,dUr6
        fadd    dVi0,dUi0,dUi4
        fsub    dVi2,dUi0,dUi4
        fadd    dVi4,dUi2,dUi6
        fsub    dVi6,dUi2,dUi6

        // finish third stage of 8 point FFT

        // fadd    qY0,qV0,qV4
        // fsub    qY4,qV0,qV4
        fadd    dYr0,dVr0,dVr4
        fsub    dYr4,dVr0,dVr4
        fadd    dYi0,dVi0,dVi4
        fsub    dYi4,dVi0,dVi4

        st2     {dYr0,dYi0},[pDst],step1         // store y0

        .ifeqs  "\inverse", "TRUE"

            fsub    dYr2,dVr2,dVi6
            fadd    dYi2,dVi2,dVr6

            fadd    dYr6,dVr2,dVi6
            st2     {dYr2,dYi2},[pDst],step1     // store y2
            fsub    dYi6,dVi2,dVr6

            // fsub    qU1,qX0,qX4
            fsub    dUr1,dXr0,dXr4
            fsub    dUi1,dXi0,dXi4

            st2     {dYr4,dYi4},[pDst],step1     // store y4

            // fsub    qU3,qX1,qX5
            // fsub    qU5,qX2,qX6
            fsub    dUr3,dXr1,dXr5
            fsub    dUr5,dXr2,dXr6
            fsub    dUi3,dXi1,dXi5
            fsub    dUi5,dXi2,dXi6

            st2     {dYr6,dYi6},[pDst],step1     // store y6

        .else

            fadd    dYr6,dVr2,dVi6
            fsub    dYi6,dVi2,dVr6

            fsub    dYr2,dVr2,dVi6
            st2     {dYr6,dYi6},[pDst],step1     // store y2
            fadd    dYi2,dVi2,dVr6


            // fsub    qU1,qX0,qX4
            fsub    dUr1,dXr0,dXr4
            fsub    dUi1,dXi0,dXi4

            st2     {dYr4,dYi4},[pDst],step1     // store y4

            // fsub    qU3,qX1,qX5
            // fsub    qU5,qX2,qX6
            fsub    dUr3,dXr1,dXr5
            fsub    dUr5,dXr2,dXr6
            fsub    dUi3,dXi1,dXi5
            fsub    dUi5,dXi2,dXi6

            st2     {dYr2,dYi2},[pDst],step1     // store y6


        .endif

        // finish first stage of 8 point FFT

        // fsub    qU7,qX3,qX7
        fsub    dUr7,dXr3,dXr7
        fsub    dUi7,dXi3,dXi7

        mov     dT0s[0], t0

        // finish second stage of 8 point FFT

        fsub    dVr1,dUr1,dUi5
        //  data[0] for next iteration
        ld2     {dXr0,dXi0},[pSrc],pointStep
        fadd    dVi1,dUi1,dUr5
        fadd    dVr3,dUr1,dUi5
        ld2     {dXr1,dXi1},[pSrc],pointStep     //  data[1]
        fsub    dVi3,dUi1,dUr5

        fsub    dVr5,dUr3,dUi7
        ld2     {dXr2,dXi2},[pSrc],pointStep     //  data[2]
        fadd    dVi5,dUi3,dUr7
        fadd    dVr7,dUr3,dUi7
        ld2     {dXr3,dXi3},[pSrc],pointStep     //  data[3]
        fsub    dVi7,dUi3,dUr7

        // finish third stage of 8 point FFT

        .ifeqs  "\inverse", "TRUE"

            // calculate a*v5
            fmul    dT1,dVr5,dT0s[0]              // use dVi0 for dT1

            ld2     {dXr4,dXi4},[pSrc],pointStep //  data[4]
            fmul    dVi5,dVi5,dT0s[0]

            ld2     {dXr5,dXi5},[pSrc],pointStep //  data[5]
            fsub    dVr5,dT1,dVi5                // a * V5
            fadd    dVi5,dT1,dVi5

            ld2     {dXr6,dXi6},[pSrc],pointStep //  data[6]

            // calculate  b*v7
            fmul    dT1,dVr7,dT0s[0]
            fmul    dVi7,dVi7,dT0s[0]

            // fadd    qY1,qV1,qV5
            // fsub    qY5,qV1,qV5
            fadd    dYr1,dVr1,dVr5
            fsub    dYr5,dVr1,dVr5
            fadd    dYi1,dVi1,dVi5
            fsub    dYi5,dVi1,dVi5

            fadd    dVr7,dT1,dVi7                // b * V7
            fsub    dVi7,dVi7,dT1
            SUB     pDst, pDst, step2            // set pDst to y1

            // On the last iteration,  this will read past the end of pSrc, 
            // so skip this read.
            BEQ     radix8SkipLastUpdateInv\name
            ld2     {dXr7,dXi7},[pSrc],setStep   //  data[7]
radix8SkipLastUpdateInv\name:

            fsub    dYr3,dVr3,dVr7
            fsub    dYi3,dVi3,dVi7
            st2     {dYr1,dYi1},[pDst],step1     // store y1
            fadd    dYr7,dVr3,dVr7
            fadd    dYi7,dVi3,dVi7


            st2     {dYr3,dYi3},[pDst],step1     // store y3
            st2     {dYr5,dYi5},[pDst],step1     // store y5
            st2     {dYr7,dYi7},[pDst]           // store y7
            ADD pDst, pDst, #16

        .else

            // calculate  b*v7
            fmul    dT1,dVr7,dT0s[0]
            ld2     {dXr4,dXi4},[pSrc],pointStep //  data[4]
            fmul    dVi7,dVi7,dT0s[0]

            ld2     {dXr5,dXi5},[pSrc],pointStep //  data[5]
            fadd    dVr7,dT1,dVi7                     // b * V7
            fsub    dVi7,dVi7,dT1

            ld2     {dXr6,dXi6},[pSrc],pointStep //  data[6]

            // calculate a*v5
            fmul    dT1,dVr5,dT0s[0]              // use dVi0 for dT1
            fmul    dVi5,dVi5,dT0s[0]

            fadd    dYr7,dVr3,dVr7
            fadd    dYi7,dVi3,dVi7
            SUB     pDst, pDst, step2            // set pDst to y1

            fsub    dVr5,dT1,dVi5                // a * V5
            fadd    dVi5,dT1,dVi5

            // On the last iteration,  this will read past the end of pSrc, 
            // so skip this read.
            BEQ     radix8SkipLastUpdateFwd\name
            ld2     {dXr7,dXi7},[pSrc],setStep   //  data[7]
radix8SkipLastUpdateFwd\name:

            // fsub    qY5,qV1,qV5
            fsub    dYr5,dVr1,dVr5
            fsub    dYi5,dVi1,dVi5

            fsub    dYr3,dVr3,dVr7
            st2     {dYr7,dYi7},[pDst],step1     // store y1
            fsub    dYi3,dVi3,dVi7

            // fadd    qY1,qV1,qV5
            fadd    dYr1,dVr1,dVr5
            fadd    dYi1,dVi1,dVi5

            st2     {dYr5,dYi5},[pDst],step1     // store y3
            st2     {dYr3,dYi3},[pDst],step1     // store y5
            st2     {dYr1,dYi1},[pDst],#16       // store y7

        .endif


        // update pDst for the next set
        SUB     pDst, pDst, step2
        BGT     radix8fsGrpZeroSetLoop\name

        // Save subFFTNum and subFFTSize for next stage
        str     subFFTNum, [pSubFFTNum]
        str     subFFTSize, [pSubFFTSize]
        
        .endm


        // Allocate stack memory required by the function


        M_START armSP_FFTFwd_CToC_FC32_Radix8_fs_OutOfPlace,,d15
            FFTSTAGE "FALSE","FALSE",FWD
        M_END


        M_START armSP_FFTInv_CToC_FC32_Radix8_fs_OutOfPlace,,d15
            FFTSTAGE "FALSE","TRUE",INV
        M_END



        .end
