//
//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
//
//  Use of this source code is governed by a BSD-style license
//  that can be found in the LICENSE file in the root of the source
//  tree. An additional intellectual property rights grant can be found
//  in the file PATENTS.  All contributing project authors may
//  be found in the AUTHORS file in the root of the source tree.
//
//  This is a modification of omxSP_FFTFwd_RToCCS_S32_Sfs_s.s
//  to support float instead of SC32.
//

//
// Description:
// Compute FFT for a real signal
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


//Input Registers

#define pSrc            x0
#define pDst            x1
#define pTwiddle        x2
#define	pOut		x3	
#define	subFFTNum	x4

// Output registers

//Local Scratch Registers

#define argTwiddle      x5
#define argDst          x6
#define subFFTSize      x7
#define N               subFFTNum
#define order           x14
#define step            x8
#define step1           pTwiddle
#define twStep          x9
#define zero            w10
#define pTwiddleTmp     pOut

// Neon registers

#define dX0       v0.2s
#define dX0s      v0.s
#define dX0r      v2.2s
#define dX0rs     v2.s
#define dX0i      v3.2s
#define dX0is     v3.s
#define dX1r      v4.2s
#define dX1i      v5.2s
#define dT0       v6.2s
#define dT1       v7.2s
#define dT2       v8.2s
#define dT3       v9.2s
#define qT0       v10.2s
#define qT1       v12.2s
#define dW0r      v14.2s
#define dW0r8b    v14.8b
#define dW0i      v15.2s
#define dW1r      v16.2s
#define dW1r8b    v16.8b
#define dW1i      v17.2s
#define dY0r      v14.2s
#define dY0i      v15.2s
#define dY1r      v16.2s
#define dY1i      v17.2s
#define qT2       v18.2s
#define qT3       v20.2s

#define half      v0.2s
#define halfs     v0.s
#define dZip      v21.2s
#define dZip8b    v21.8b
        
    // Allocate stack memory required by the function

    // Write function header
        M_START     ComplexToRealFixup,,d15

        asr     N, N, #1

        clz     order, subFFTNum                    // N = 2^order

        rsb     order,order,#63
        MOV     subFFTSize,subFFTNum            // subFFTSize = N/2
        //MOV     subFFTNum,N
        mov     argDst, pDst
        mov     argTwiddle, pTwiddle

        // F(0) = 1/2[Z(0) + Z'(0)] - j [Z(0) - Z'(0)]
        // 1/2[(a+jb) + (a-jb)] - j  [(a+jb) - (a-jb)]
        // 1/2[2a+j0] - j [0+j2b]
        // (a+b, 0)

        // F(N/2) = 1/2[Z(0) + Z'(0)] + j [Z(0) - Z'(0)]
        // 1/2[(a+jb) + (a-jb)] + j  [(a+jb) - (a-jb)]
        // 1/2[2a+j0] + j [0+j2b]
        // (a-b, 0)

        // F(0) and F(N/2)
        ld2     {dX0rs,dX0is}[0],[pSrc], #8
        MOV     zero,#0
        mov    dX0rs[1],zero
        lsl     step,subFFTSize, #3               // step = N/2 * 8 bytes
        mov    dX0is[1],zero
        // twStep = 3N/8 * 8 bytes pointing to W^1
        SUB     twStep,step,subFFTSize,LSL #1

        fadd    dY0r,dX0r,dX0i                    // F(0) = ((Z0.r+Z0.i) , 0)
        lsl     step1,subFFTSize, #2              // step1 = N/2 * 4 bytes
        fsub    dY0i,dX0r,dX0i                    // F(N/2) = ((Z0.r-Z0.i) , 0)
        SUBS    subFFTSize,subFFTSize,#2

        st1     {dY0r},[argDst],step
        ADD     pTwiddleTmp,argTwiddle,#8         // W^2
        st1     {dY0i},[argDst], #8
        ADD     argTwiddle,argTwiddle,twStep      // W^1

//        dup     dzero,zero
        SUB     argDst,argDst,step

        BLT     End
        BEQ     lastElement
        SUB     step,step,#24
        SUB     step1,step1,#8                    // (N/4-1)*8 bytes

        // F(k) = 1/2[Z(k) +  Z'(N/2-k)] -j*W^(k) [Z(k) -  Z'(N/2-k)]
        // Note: W^k is stored as negative values in the table
        // Process 4 elements at a time. E.g: F(1),F(2) and F(N/2-2),F(N/2-1)
        // since both of them require Z(1),Z(2) and Z(N/2-2),Z(N/2-1)

        fmov     half, #0.5

evenOddButterflyLoop:


        ld1     {dW0r},[argTwiddle],step1
        ld1     {dW1r},[argTwiddle], #8

        ld2     {dX0r,dX0i},[pSrc],step
        SUB     argTwiddle,argTwiddle,step1
        ld2     {dX1r,dX1i},[pSrc], #16



        SUB     step1,step1,#8                    // (N/4-2)*8 bytes
        ld1     {dW0i},[pTwiddleTmp],step1
        ld1     {dW1i},[pTwiddleTmp], #8
        SUB     pSrc,pSrc,step

        SUB     pTwiddleTmp,pTwiddleTmp,step1
        rev64   dX1r,dX1r
        rev64   dX1i,dX1i
        SUBS    subFFTSize,subFFTSize,#4



        fsub    dT2,dX0r,dX1r                     // a-c
        SUB     step1,step1,#8
        fadd    dT0,dX0r,dX1r                     // a+c
        fsub    dT1,dX0i,dX1i                     // b-d
        fadd    dT3,dX0i,dX1i                     // b+d
        fmul   dT0,dT0,halfs[0]
        fmul   dT1,dT1,halfs[0]
        // VZIP    dW1r,dW1i
        // VZIP    dW0r,dW0i
        zip1    dZip, dW1r, dW1i
        zip2    dW1i, dW1r, dW1i
        mov     dW1r8b, dZip8b
        zip1    dZip, dW0r, dW0i
        zip2    dW0i, dW0r, dW0i
        mov     dW0r8b, dZip8b

        fmul   qT0,dW1r,dT2
        fmul   qT1,dW1r,dT3
        fmul   qT2,dW0r,dT2
        fmul   qT3,dW0r,dT3

        fmla   qT0,dW1i,dT3
        fmls   qT1,dW1i,dT2

        fmls   qT2,dW0i,dT3
        fmla   qT3,dW0i,dT2


        fmul  dX1r,qT0,halfs[0]
        fmul  dX1i,qT1,halfs[0]

        fsub    dY1r,dT0,dX1i                     // F(N/2 -1)
        fadd    dY1i,dT1,dX1r
        fneg    dY1i,dY1i

        rev64   dY1r,dY1r
        rev64   dY1i,dY1i


        fmul  dX0r,qT2,halfs[0]
        fmul  dX0i,qT3,halfs[0]

        fsub    dY0r,dT0,dX0i                     // F(1)
        fadd    dY0i,dT1,dX0r


        st2     {dY0r,dY0i},[argDst],step
        st2     {dY1r,dY1i},[argDst], #16
        SUB     argDst,argDst,step
        SUB     step,step,#32                     // (N/2-4)*8 bytes


        BGT     evenOddButterflyLoop

        // set both the ptrs to the last element
        SUB     pSrc,pSrc,#8
        SUB     argDst,argDst,#8



        // Last element can be expanded as follows
        // 1/2[Z(k) + Z'(k)] + j w^k [Z(k) - Z'(k)]
        // 1/2[(a+jb) + (a-jb)] + j w^k [(a+jb) - (a-jb)]
        // 1/2[2a+j0] + j (c+jd) [0+j2b]
        // (a-bc, -bd)
        // Since (c,d) = (0,1) for the last element, result is just (a,-b)

lastElement:
        ld1     {dX0r},[pSrc]

        st1     {dX0rs}[0],[argDst], #4
        fneg    dX0r,dX0r
        st1     {dX0rs}[1],[argDst], #4
End:

        // Write function tail
        M_END

        .end
