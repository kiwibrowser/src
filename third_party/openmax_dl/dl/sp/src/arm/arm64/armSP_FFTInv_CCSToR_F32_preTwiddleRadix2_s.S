//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of
//  armSP_FFTInv_CCSToR_S32_preTwiddleRadix2_unsafe_s.s to support float
//  instead of SC32.
//

//
// Description:
// Compute the "preTwiddleRadix2" stage prior to the call to the complexFFT
// It does a Z(k) = Feven(k) + jW^(-k) FOdd(k); k=0,1,2,...N/2-1 computation
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
#define pTwiddle        x1
#define	pOut		x2	
#define	subFFTNum	x3

// Output registers

//Local Scratch Registers

#define argTwiddle      x5
#define argDst          x6
#define subFFTSize      x7
#define N               subFFTNum

#define pOut1           x13
	
#define size            x7
#define step            x8
#define step1           x9
#define twStep          x10
#define pTwiddleTmp     x11
#define argTwiddle1     x12

// Neon registers

#define dX0     v0.2s
#define dX0s    v0.s
#define dShift  v1.2s
#define dX1     v1.2s
#define dX1s    v1.s
#define dY0     v2.2s
#define dY08b   v2.8b
#define dY1     v3.2s
#define dX0r    v0.2s
#define dX0rs   v0.s
#define dX0i    v1.2s
#define dX1r    v2.2s
#define dX1i    v3.2s
#define dW0r    v4.2s
#define dW0r8b  v4.8b
#define dW0i    v5.2s
#define dW1r    v6.2s
#define dW1r8b  v6.8b
#define dW1i    v7.2s
#define dT0     v8.2s
#define dT1     v9.2s
#define dT2     v10.2s
#define dT3     v11.2s
#define qT0     v12.2s
#define qT1     v14.2s
#define qT2     v16.2s
#define qT3     v18.2s
#define dY0r    v4.2s
#define dY0i    v5.2s
#define dY1r    v6.2s
#define dY1i    v7.2s

#define dY2     v4.2s
#define dY3     v5.2s
#define dW0     v6.2s
#define dW1     v7.2s
#define dW0Tmp  v10.2s
#define dW1Neg  v11.2s

#define dZip    v19.2s
#define dZip8b  v19.8b
#define half    v13.2s
#define halfs   v13.s

        .macro FFTSTAGE scaled, inverse, name

        fmov    half, 0.5

        asr     size, subFFTNum, #1           // preserve the contents of N = subFFTNum
        lsl     step, subFFTNum, #2           // step = N/2 * 8 bytes


        // Z(k) = 1/2 {[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]}
        // Note: W^(k) is stored as negated value and also need to
        // conjugate the values from the table

        // Z(0) : no need of twiddle multiply
        // Z(0) = 1/2 { [F(0) +  F'(N/2)] +j [F(0) -  F'(N/2)] }

        ld1     {dX0},[pSrc],step
        ADD     pOut1,pOut,step               // pOut1 = pOut+ N/2*8 bytes

        ld1     {dX1},[pSrc], #8
        // twStep = 3N/8 * 8 bytes pointing to W^1
        SUB     twStep,step,size,LSL #1

        lsl     step1,size, #2                // step1 = N/4 * 8 = N/2*4 bytes
        SUB     step1,step1,#8                // (N/4-1)*8 bytes

        fadd    dY0,dX0,dX1                   // [b+d | a+c]
        fsub    dY1,dX0,dX1                   // [b-d | a-c]
        fmul    dY0, dY0, halfs[0]
        fmul    dY1, dY1, halfs[0]

        // dY0= [a-c | a+c] ;dY1= [b-d | b+d]
        // VZIP    dY0,dY1
        zip1    dZip,dY0,dY1
        zip2    dY1,dY0,dY1
        mov     dY08b, dZip8b

        fsub   dX0,dY0,dY1
        SUBS   size,size,#2
        fadd   dX1,dY0,dY1

        SUB     pSrc,pSrc,step

        st1     {dX0s}[0],[pOut1], #4
        ADD     pTwiddleTmp,pTwiddle,#8       // W^2
        st1     {dX1s}[1],[pOut1], #4
        ADD     argTwiddle1,pTwiddle,twStep   // W^1


        BLT     decrementScale\name
        BEQ     lastElement\name


        // Z(k) = 1/2[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]
        // Note: W^k is stored as negative values in the table and also
        // need to conjugate the values from the table.
        //
        // Process 4 elements at a time. E.g: Z(1),Z(2) and Z(N/2-2),Z(N/2-1)
        // since both of them require F(1),F(2) and F(N/2-2),F(N/2-1)


        SUB     step,step,#24
evenOddButterflyLoop\name :


        ld1     {dW0r},[argTwiddle1],step1
        ld1     {dW1r},[argTwiddle1], #8

        ld2     {dX0r,dX0i},[pSrc],step
        SUB     argTwiddle1,argTwiddle1,step1
        ld2     {dX1r,dX1i},[pSrc], #16

        SUB     step1,step1,#8                // (N/4-2)*8 bytes
        ld1     {dW0i},[pTwiddleTmp],step1
        ld1     {dW1i},[pTwiddleTmp], #8
        SUB     pSrc,pSrc,step

        SUB     pTwiddleTmp,pTwiddleTmp,step1
        rev64   dX1r,dX1r
        rev64   dX1i,dX1i
        SUBS    size,size,#4


        fsub    dT2,dX0r,dX1r                 // a-c
        fadd    dT3,dX0i,dX1i                 // b+d
        fadd    dT0,dX0r,dX1r                 // a+c
        fsub    dT1,dX0i,dX1i                 // b-d
        SUB     step1,step1,#8

        fmul    dT2, dT2, halfs[0]
        fmul    dT3, dT3, halfs[0]

        fmul    dT0, dT0, halfs[0]
        fmul    dT1, dT1, halfs[0]

        // VZIP    dW1r,dW1i
        // VZIP    dW0r,dW0i
        zip1    dZip, dW1r,dW1i
        zip2    dW1i,dW1r,dW1i
        mov     dW1r8b, dZip8b
        zip1    dZip,dW0r,dW0i
        zip2    dW0i,dW0r,dW0i
        mov     dW0r8b, dZip8b

        fmul   dX1r,dW1r,dT2
        fmul   dX1i,dW1r,dT3
        fmul   dX0r,dW0r,dT2
        fmul   dX0i,dW0r,dT3

        fmls   dX1r,dW1i,dT3
        fmla   dX1i,dW1i,dT2

        fmla   dX0r,dW0i,dT3
        fmls   dX0i,dW0i,dT2


        fadd    dY1r,dT0,dX1i                 // F(N/2 -1)
        fsub    dY1i,dX1r,dT1

        rev64   dY1r,dY1r
        rev64   dY1i,dY1i


        fadd    dY0r,dT0,dX0i                 // F(1)
        fsub    dY0i,dT1,dX0r


        st2     {dY0r,dY0i},[pOut1],step
        st2     {dY1r,dY1i},[pOut1], #16
        SUB     pOut1,pOut1,step
        SUB     step,step,#32                 // (N/2-4)*8 bytes


        BGT     evenOddButterflyLoop\name


        // set both the ptrs to the last element
        SUB     pSrc,pSrc,#8
        SUB     pOut1,pOut1,#8

        // Last element can be expanded as follows
        // 1/2[Z(k) + Z'(k)] - j w^-k [Z(k) - Z'(k)] (since W^k is stored as
        // -ve)
        // 1/2[(a+jb) + (a-jb)] - j w^-k [(a+jb) - (a-jb)]
        // 1/2[2a+j0] - j (c-jd) [0+j2b]
        // (a+bc, -bd)
        // Since (c,d) = (0,1) for the last element, result is just (a,-b)

lastElement\name :
        ld1     {dX0r},[pSrc]

        st1     {dX0rs}[0],[pOut1], #4
        fneg    dX0r,dX0r
        st1     {dX0rs}[1],[pOut1]



decrementScale\name :

        .endm

        M_START armSP_FFTInv_CCSToR_F32_preTwiddleRadix2,,d15
            FFTSTAGE "FALSE","TRUE",Inv
        M_END

        .end
