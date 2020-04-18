@
@  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@
@  Use of this source code is governed by a BSD-style license
@  that can be found in the LICENSE file in the root of the source
@  tree. An additional intellectual property rights grant can be found
@  in the file PATENTS.  All contributing project authors may
@  be found in the AUTHORS file in the root of the source tree.
@
@ Some code in this file was originally from file
@ armSP_FFTInv_CCSToR_S32_preTwiddleRadix2_unsafe_s.S which was licensed as
@ follows. It has been relicensed with permission from the copyright holders.
@

@
@ OpenMAX DL: v1.0.2
@ Last Modified Revision:   7485
@ Last Modified Date:       Fri, 21 Sep 2007
@ 
@ (c) Copyright 2007-2008 ARM Limited. All Rights Reserved.
@

@
@ Description:
@ Compute the "preTwiddleRadix2" stage prior to the call to the complexFFT.
@ It does a Z(k) = Feven(k) + jW^(-k) FOdd(k); k=0,1,2,...N/2-1 computation.
@ It implements both "scaled"(by 1/2) and "unscaled" versions of the above
@ formula.
@ 
        
#include "dl/api/arm/armCOMM_s.h"
#include "dl/api/arm/omxtypes_s.h"
        
@//Input Registers
#define pSrc            r0
#define pDst            r1
#define pFFTSpec        r2
#define scale           r3

@ Output registers
#define result          r0

@//Local Scratch Registers
#define argTwiddle      r1
#define argDst          r2
#define argScale        r4
#define tmpOrder        r4
#define pTwiddle        r4
#define pOut            r5
#define subFFTSize      r7     
#define subFFTNum       r6
#define N               r6
#define order           r14
#define diff            r9
@ Total num of radix stages to comple the FFT.
#define count           r8
#define x0r             r4    
#define x0i             r5
#define diffMinusOne    r2
#define round           r3
#define pOut1           r2
#define size            r7
#define step            r8            
#define step1           r9
#define step2           r10
#define twStep          r10
#define pTwiddleTmp     r11
#define argTwiddle1     r12
#define zero            r14

@ Neon registers
#define dX0             D0.S16
#define dX0S32          D0.S32
#define dShift          D1.S16
#define dX1             D1.S16
#define dX1S32          D1.S32
#define dY0             D2.S16
#define dY1             D3.S16
#define dX0r            D0.S16            
#define dX0rS32         D0.S32
#define dX0i            D1.S16
#define dX1r            D2.S16
#define dX1i            D3.S16
#define qX1             Q1.S16
#define dW0r            D4.S16
#define dW0i            D5.S16
#define dW1r            D6.S16
#define dW1i            D7.S16
#define dW0rS32         D4.S32
#define dW0iS32         D5.S32
#define dW1rS32         D6.S32
#define dW1iS32         D7.S32
#define dT0             D8.S16
#define dT1             D9.S16
#define dT2             D10.S16
#define dT3             D11.S16
#define qT0             Q6.S32
#define qT1             Q7.S32
#define qT2             Q8.S32
#define qT3             Q9.S32
#define dY0r            D4.S16
#define dY0i            D5.S16
#define dY1r            D6.S16
#define dY1i            D7.S16
#define qY1             Q3.S16
#define dY2             D4.S16
#define dY3             D5.S16
#define dW0             D6.S16
#define dW1             D7.S16
#define dW0Tmp          D10.S16
#define dW1Neg          D11.S16

        @ Structure offsets for the FFTSpec             
        .set    ARMsFFTSpec_N, 0
        .set    ARMsFFTSpec_pBitRev, 4
        .set    ARMsFFTSpec_pTwiddle, 8
        .set    ARMsFFTSpec_pBuf, 12

        .macro FFTSTAGE scaled, inverse, name
        
        @ Read the size from structure and take log
        LDR     N, [pFFTSpec, #ARMsFFTSpec_N]
        
        @ Read other structure parameters
        LDR     pTwiddle, [pFFTSpec, #ARMsFFTSpec_pTwiddle]
        LDR     pOut, [pFFTSpec, #ARMsFFTSpec_pBuf]
        
        MOV     size,N,ASR #1        @ preserve the contents of N
        MOV     step,N,LSL #1        @ step = N/2 * 4 bytes
        
        @ Process different FFT sizes with different loops.
        CMP    size,#4
        BLE    smallFFTSize\name
        
        @ Z(k) = 1/2 {[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]}
        @ Note: W^(k) is stored as negated value and also need to
        @ conjugate the values from the table.
        
        @ Z(0) : no need of twiddle multiply
        @ Z(0) = 1/2 { [F(0) +  F'(N/2)] +j [F(0) -  F'(N/2)] }
        
        VLD1    dX0S32[0],[pSrc],step
        ADD     pOut1,pOut,step      @ pOut1 = pOut+ N/2*4 bytes 
                
        VLD1    dX1S32[0],[pSrc]!
        SUB     twStep,step,size     @ twStep = 3N/8 * 4 bytes pointing to W^1
        
        MOV     step1,size,LSL #1    @ step1 = N/4 * 4 = N/2*2 bytes
        SUB     step1,step1,#4       @ (N/4-1)*4 bytes
        
        VHADD    dY0,dX0,dX1         @ [b+d | a+c]
        VHSUB    dY1,dX0,dX1         @ [b-d | a-c] 
        VTRN    dY0,dY1              @ dY0= [a-c | a+c] ;dY1= [b-d | b+d] 
        
        .ifeqs  "\scaled", "TRUE"
            VHSUB   dX0,dY0,dY1
            SUBS    size,size,#2
            VHADD   dX1,dY0,dY1
        .else
            VSUB   dX0,dY0,dY1
            SUBS    size,size,#2
            VADD   dX1,dY0,dY1
        .endif
                    
        SUB     pSrc,pSrc,step
        VST1    dX0[0],[pOut1]!
        ADD     pTwiddleTmp,pTwiddle,#4                @ W^2
        VST1    dX1[1],[pOut1]!
        ADD     argTwiddle1,pTwiddle,twStep            @ W^1 
        
        BLT     decrementScale\name
        BEQ     lastElement\name
                        
        SUB     step,step,#20
        SUB     step1,step1,#4                         @ (N/4-1)*8 bytes
        SUB     step2, step1, #4
                        
        @ Z(k) = 1/2[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]
        @ Note: W^k is stored as negative values in the table and also need to
        @ conjugate the values from the table.
        @ Process 4 elements at a time. E.g: Z(1),Z(2) and Z(N/2-2),Z(N/2-1)
        @ since both of them require F(1),F(2) and F(N/2-2),F(N/2-1).

evenOddButterflyLoop\name:     
        VLD2    {dX0r,dX0i},[pSrc],step
        VLD2    {dX1r,dX1i},[pSrc]!
        SUB     pSrc, pSrc, step

        VLD1    dW0r,[argTwiddle1],step1
        VREV64  qX1,qX1
        VLD1    dW1r,[argTwiddle1]!
        VHSUB   dT2,dX0r,dX1r                          @ a-c
        SUB     argTwiddle1, argTwiddle1, step1
        SUB     step1,step1,#16

        VLD1    dW0i,[pTwiddleTmp],step2
        VHADD   dT3,dX0i,dX1i                          @ b+d
        VLD1    dW1i,[pTwiddleTmp]!
        VHADD   dT0,dX0r,dX1r                          @ a+c
        VHSUB   dT1,dX0i,dX1i                          @ b-d
        SUB     pTwiddleTmp, pTwiddleTmp, step2
        SUB     step2,step2,#16

        SUBS    size,size,#8
        
        VZIP    dW1r,dW1i
        VTRN    dW0r,dW0i
        VZIP    dW1iS32, dW1rS32
                                
        VMULL   qT0,dW1i,dT2
        VMLSL   qT0,dW1r,dT3
        VMULL   qT1,dW1i,dT3
        VMLAL   qT1,dW1r,dT2
        VMULL   qT2,dW0r,dT2
        VMLAL   qT2,dW0i,dT3
        VMULL   qT3,dW0r,dT3
        VMLSL   qT3,dW0i,dT2
        
        VRSHRN  dX1r,qT0,#15
        VRSHRN  dX1i,qT1,#15
        VRSHRN  dX0r,qT2,#15
        VRSHRN  dX0i,qT3,#15
        
        .ifeqs  "\scaled", "TRUE"
            VHADD    dY1r,dT0,dX1i                     @ F(N/2 -1)
            VHSUB    dY1i,dX1r,dT1
        .else
            VADD    dY1r,dT0,dX1i                      @ F(N/2 -1)
            VSUB    dY1i,dX1r,dT1
        .endif
        
        .ifeqs  "\scaled", "TRUE"
            VHADD    dY0r,dT0,dX0i                     @ F(1)
            VHSUB    dY0i,dT1,dX0r
        .else
            VADD    dY0r,dT0,dX0i                      @ F(1)
            VSUB    dY0i,dT1,dX0r
        .endif
        
        VREV64  qY1,qY1

        VST2    {dY0r,dY0i},[pOut1],step
        VST2    {dY1r,dY1i},[pOut1]
        ADD     pOut1,pOut1,#16
        SUB     pOut1, pOut1, step
        SUB     step,step,#32
       
        BGT     evenOddButterflyLoop\name

        SUB     pSrc,pSrc,#4           @ set both the ptrs to the last element
        SUB     pOut1,pOut1,#4
        B       lastElement\name
        
smallFFTSize\name:
        @ Z(k) = 1/2 {[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]}
        @ Note: W^(k) is stored as negated value and also need to
        @ conjugate the values from the table.
        
        @ Z(0) : no need of twiddle multiply
        @ Z(0) = 1/2 { [F(0) +  F'(N/2)] +j [F(0) -  F'(N/2)] }
        
        VLD1    dX0S32[0],[pSrc],step
        ADD     pOut1,pOut,step      @ pOut1 = pOut+ N/2*4 bytes 
                
        VLD1    dX1S32[0],[pSrc]!
        SUB     twStep,step,size     @ twStep = 3N/8 * 4 bytes pointing to W^1
        
        MOV     step1,size,LSL #1    @ step1 = N/4 * 4 = N/2*2 bytes
        SUB     step1,step1,#4       @ (N/4-1)*4 bytes
        
        VHADD    dY0,dX0,dX1         @ [b+d | a+c]
        VHSUB    dY1,dX0,dX1         @ [b-d | a-c] 
        VTRN    dY0,dY1              @ dY0= [a-c | a+c] ;dY1= [b-d | b+d] 
        
        .ifeqs  "\scaled", "TRUE"
            VHSUB   dX0,dY0,dY1
            SUBS    size,size,#2
            VHADD   dX1,dY0,dY1
        .else
            VSUB   dX0,dY0,dY1
            SUBS    size,size,#2
            VADD   dX1,dY0,dY1
        .endif
                    
        SUB     pSrc,pSrc,step
        VST1    dX0[0],[pOut1]!
        ADD     pTwiddleTmp,pTwiddle,#4                @ W^2
        VST1    dX1[1],[pOut1]!
        ADD     argTwiddle1,pTwiddle,twStep            @ W^1 
        
        BLT     decrementScale\name
        BEQ     lastElement\name
                        
        @ Z(k) = 1/2[F(k) +  F'(N/2-k)] +j*W^(-k) [F(k) -  F'(N/2-k)]
        @ Note: W^k is stored as negative values in the table and also need to
        @ conjugate the values from the table.
        @ Process 4 elements at a time. E.g: Z(1),Z(2) and Z(N/2-2),Z(N/2-1)
        @ since both of them require F(1),F(2) and F(N/2-2),F(N/2-1).

        SUB     step,step,#12

evenOddButterflyLoopSize4\name:     
        VLD1    dW0rS32[0],[argTwiddle1],step1
        VLD1    dW1rS32[0],[argTwiddle1]!
        
        VLD2    {dX0r[0],dX0i[0]},[pSrc]!
        VLD2    {dX0r[1],dX0i[1]},[pSrc],step
        SUB     pSrc,pSrc,#4
        SUB     argTwiddle1,argTwiddle1,step1
        VLD2    {dX1r[0],dX1i[0]},[pSrc]!
        VLD2    {dX1r[1],dX1i[1]},[pSrc]!
        
        SUB     step1,step1,#4                         @ (N/4-2)*4 bytes
        VLD1    dW0iS32[0],[pTwiddleTmp],step1
        VLD1    dW1iS32[0],[pTwiddleTmp]!
        SUB     pSrc,pSrc,step
        
        SUB     pTwiddleTmp,pTwiddleTmp,step1
        VREV32  dX1r,dX1r
        VREV32  dX1i,dX1i
        SUBS    size,size,#4
                        
        VHSUB   dT2,dX0r,dX1r                          @ a-c
        VHADD   dT3,dX0i,dX1i                          @ b+d
        SUB     step1,step1,#4
        VHADD   dT0,dX0r,dX1r                          @ a+c
        VHSUB   dT1,dX0i,dX1i                          @ b-d
        
        VTRN    dW1r,dW1i
        VTRN    dW0r,dW0i
                                
        VMULL   qT0,dW1r,dT2
        VMLSL   qT0,dW1i,dT3
        VMULL   qT1,dW1r,dT3
        VMLAL   qT1,dW1i,dT2
        VMULL   qT2,dW0r,dT2
        VMLAL   qT2,dW0i,dT3
        VMULL   qT3,dW0r,dT3
        VMLSL   qT3,dW0i,dT2
        
        VRSHRN  dX1r,qT0,#15
        VRSHRN  dX1i,qT1,#15
        
        .ifeqs  "\scaled", "TRUE"
            VHADD    dY1r,dT0,dX1i                     @ F(N/2 -1)
            VHSUB    dY1i,dX1r,dT1
        .else
            VADD    dY1r,dT0,dX1i                      @ F(N/2 -1)
            VSUB    dY1i,dX1r,dT1
        .endif
        
        VREV32  dY1r,dY1r
        VREV32  dY1i,dY1i
                            
        VRSHRN  dX0r,qT2,#15
        VRSHRN  dX0i,qT3,#15
        
        .ifeqs  "\scaled", "TRUE"
            VHADD    dY0r,dT0,dX0i                     @ F(1)
            VHSUB    dY0i,dT1,dX0r
        .else
            VADD    dY0r,dT0,dX0i                      @ F(1)
            VSUB    dY0i,dT1,dX0r
        .endif
        
        VST2    {dY0r[0],dY0i[0]},[pOut1]!
        VST2    {dY0r[1],dY0i[1]},[pOut1],step
        SUB     pOut1, #4
        VST2    {dY1r[0],dY1i[0]},[pOut1]!
        VST2    {dY1r[1],dY1i[1]},[pOut1]!
        SUB     pOut1,pOut1,step
        SUB     pSrc,pSrc,#4           @ set both the ptrs to the last element
        SUB     pOut1,pOut1,#4
        
        @ Last element can be expanded as follows
        @ 1/2[Z(k) + Z'(k)] - j w^-k [Z(k) - Z'(k)] (W^k is stored as -ve)
        @ 1/2[(a+jb) + (a-jb)] - j w^-k [(a+jb) - (a-jb)]
        @ 1/2[2a+j0] - j (c-jd) [0+j2b]
        @ (a+bc, -bd)
        @ Since (c,d) = (0,1) for the last element, result is just (a,-b)
        
lastElement\name:      
        VLD1    dX0rS32[0],[pSrc]
        
        .ifeqs  "\scaled", "TRUE"
            VSHR    dX0r,dX0r,#1
        .endif
        
        VST1    dX0r[0],[pOut1]!
        VNEG    dX0r,dX0r
        VST1    dX0r[1],[pOut1]

decrementScale\name:          
        .ifeqs  "\scaled", "TRUE"
            SUB scale,scale,#1
        .endif
        
        .endm
        
        M_START armSP_FFTInv_CCSToR_S16_preTwiddleRadix2_unsafe,r4
        FFTSTAGE "FALSE","TRUE",Inv
        M_END
        
        M_START armSP_FFTInv_CCSToR_S16_Sfs_preTwiddleRadix2_unsafe,r4
        FFTSTAGE "TRUE","TRUE",InvSfs
        M_END

        
        .end
