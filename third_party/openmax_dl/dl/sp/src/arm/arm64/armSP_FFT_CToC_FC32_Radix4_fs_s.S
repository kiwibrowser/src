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
//  This is a modification of armSP_FFT_CToC_SC32_Radix4_fs_unsafe_s.s
//  to support float instead of SC32.
//

//
// Description:
// Compute a first stage Radix 4 FFT stage for a N point complex signal
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
#define step3           x11

// Neon Registers

#define dXr0    v0.2s
#define dXi0    v1.2s
#define dXr1    v2.2s
#define dXi1    v3.2s
#define dXr2    v4.2s
#define dXi2    v5.2s
#define dXr3    v6.2s
#define dXi3    v7.2s
#define dYr0    v8.2s
#define dYi0    v9.2s
#define dYr1    v10.2s
#define dYi1    v11.2s
#define dYr2    v12.2s
#define dYi2    v13.2s
#define dYr3    v14.2s
#define dYi3    v15.2s
#define dZr0    v16.2s
#define dZi0    v17.2s
#define dZr1    v18.2s
#define dZi1    v19.2s
#define dZr2    v20.2s
#define dZi2    v21.2s
#define dZr3    v22.2s
#define dZi3    v23.2s


        .macro FFTSTAGE scaled, inverse, name

        // Define stack arguments

        // Move args values into our work registers
        ldr     subFFTNum, [pSubFFTNum]
        ldr     subFFTSize, [pSubFFTSize]
        
        // pT0+1 increments pT0 by 8 bytes
        // pT0+pointStep = increment of 8*pointStep bytes = 2*grpSize bytes
        // Note: outPointStep = pointStep for firststage

        lsl     pointStep, subFFTNum, #1

        // Update pSubFFTSize and pSubFFTNum regs
        ld2     {dXr0,dXi0}, [pSrc], pointStep          // data[0]

        // subFFTSize = 1 for the first stage
        MOV     subFFTSize,#4

        // Note: setCount = subFFTNum/4 (reuse the grpSize reg for setCount)
        LSR     grpSize,subFFTNum,#2
        ld2     {dXr1,dXi1}, [pSrc], pointStep          //  data[1]
        MOV     subFFTNum,grpSize


        // Calculate the step of input data for the next set
        //MOV     setStep,pointStep,LSL #1
        lsl     setStep, grpSize, #4
        ld2     {dXr2,dXi2}, [pSrc], pointStep          //  data[2]

        // setStep = 3*pointStep
        ADD     setStep,setStep,pointStep
        // setStep = - 3*pointStep+16

        rsb     setStep,setStep,#16
        //  data[3] & update pSrc for the next set
        ld2     {dXr3,dXi3}, [pSrc], setStep

        // step1 = 2*pointStep
        lsl     step1, pointStep, #1

        // fadd qY0, qX0, qX2
        fadd    dYr0, dXr0, dXr2        
        fadd    dYi0, dXi0, dXi2
        // step3 = -pointStep
        neg     step3, pointStep

        // grp = 0 a special case since all the twiddle factors are 1
        // Loop on the sets : 2 sets at a time

radix4fsGrpZeroSetLoop\name :



        // Decrement setcount
        SUBS    setCount,setCount,#2


        // finish first stage of 4 point FFT


        // fsub qy2,qx0,qx2
        fsub    dYr2, dXr0, dXr2
        fsub    dYi2, dXi0, dXi2

        ld2     {dXr0,dXi0}, [pSrc], step1              //  data[0]
        // fadd qy1,qx1,qx3     
        fadd    dYr1, dXr1, dXr3                    
        fadd    dYi1, dXi1, dXi3
        ld2     {dXr2,dXi2}, [pSrc], step3              //  data[2]
        // fsub qy3,qx1,qx3     
        fsub    dYr3, dXr1, dXr3                    
        fsub    dYi3, dXi1, dXi3


        // finish second stage of 4 point FFT

        .ifeqs "\inverse", "TRUE"

            ld2     {dXr1,dXi1}, [pSrc], step1          //  data[1]
            // fadd  qz0,qy0,qy1
            fadd    dZr0, dYr0, dYr1                    
            fadd    dZi0, dYi0, dYi1

            //  data[3] & update pSrc for the next set, but not if it's the
            //  last iteration so that we don't read past the end of the 
            //  input array.
            BEQ     radix4SkipLastUpdateInv\name
            ld2     {dXr3,dXi3}, [pSrc], setStep

radix4SkipLastUpdateInv\name:
            FSUB    dZr3,dYr2,dYi3

            st2    {dZr0,dZi0},[pDst],outPointStep
            FADD    dZi3,dYi2,dYr3

            // fsub qZ1,qY0,qY1 
            FSUB    dZr1, dYr0, dYr1
            FSUB    dZi1, dYi0, dYi1
            st2    {dZr3,dZi3},[pDst],outPointStep

            FADD    dZr2,dYr2,dYi3
            st2    {dZr1,dZi1},[pDst],outPointStep
            FSUB    dZi2,dYi2,dYr3

            //  fadd qY0, qX0, qX2
            FADD    dYr0, dXr0, dXr2                    // u0 for next iteration
            FADD    dYi0, dXi0, dXi2
            st2    {dZr2,dZi2},[pDst],setStep


        .else

            ld2     {dXr1,dXi1}, [pSrc], step1          //  data[1]
            // fadd qZ0,qY0,qY1
            fadd    dZr0, dYr0, dYr1
            fadd    dZi0, dYi0, dYi1

            //  data[3] & update pSrc for the next set, but not if it's the
            //  last iteration so that we don't read past the end of the 
            //  input array.
            BEQ     radix4SkipLastUpdateFwd\name
            ld2     {dXr3,dXi3}, [pSrc], setStep

radix4SkipLastUpdateFwd\name:
            FADD    dZr2,dYr2,dYi3

            st2    {dZr0,dZi0},[pDst],outPointStep
            FSUB    dZi2,dYi2,dYr3

            // fsub  qz1,qy0,qy1
            fsub    dZr1, dYr0, dYr1
            fsub    dZi1, dYi0, dYi1
            st2    {dZr2,dZi2},[pDst],outPointStep

            FSUB    dZr3,dYr2,dYi3
            st2    {dZr1,dZi1},[pDst],outPointStep
            FADD    dZi3,dYi2,dYr3

            //  fadd  qy0,qx0,qx2
            fadd    dYr0, dXr0, dXr2                    // u0 for next iteration
            fadd    dYi0, dXi0, dXi2

            st2    {dZr3,dZi3},[pDst],setStep

        .endif

        BGT     radix4fsGrpZeroSetLoop\name

        // Save subFFTNum and subFFTSize for next stage
        str     subFFTNum, [pSubFFTNum]
        str     subFFTSize, [pSubFFTSize]
        
        .endm



        M_START armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace,,d15
        FFTSTAGE "FALSE","FALSE",fwd
        M_END



        M_START armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace,,d15
        FFTSTAGE "FALSE","TRUE",inv
        M_END


        .end
