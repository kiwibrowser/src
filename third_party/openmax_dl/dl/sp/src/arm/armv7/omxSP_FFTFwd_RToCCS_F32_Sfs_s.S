@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of omxSP_FFTFwd_RToCCS_S32_Sfs_s.s
@//  to support float instead of SC32.
@//

@//
@// Description:
@// Compute FFT for a real signal
@//
@//


@// Include standard headers

#include "dl/api/arm/armCOMM_s.h"
#include "dl/api/arm/omxtypes_s.h"

@//        M_VARIANTS ARM1136JS

@// Import symbols required from other files
@// (For example tables)

        .extern  armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTFwd_CToC_FC32_Radix8_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTFwd_CToC_FC32_Radix4_OutOfPlace_unsafe_vfp

@// Set debugging level
@//DEBUG_ON    SETL {TRUE}



@// Guarding implementation by the processor name

@//    IF  ARM1136JS

@//Input Registers

#define pSrc            r0
#define pDst            r1
#define pFFTSpec        r2


@// Output registers
#define result          r0

@//Local Scratch Registers

@// N=1 case
#define scaleMinusOne   r2
#define rnd             r2
#define zero            r8
#define Zero            r9


#define argTwiddle      r1
#define argDst          r2
#define argScale        r4
#define pTwiddle        r4
#define pOut            r5
#define subFFTSize      r7
#define subFFTNum       r6
#define N               r6
#define order           r14
#define diff            r9
#define count           r8
#define diffMinusOne    r10
#define round           r3

#define step            r3
#define step1           r6
#define twStep          r12
#define pTwiddleTmp     r14
#define t0              r12
#define t1              r14              /*@// pTwiddleTmp*/
#define t2              r0
#define t3              r1               /*@// pSrc,argTwiddle*/
#define t4              r6
#define t5              r7               /*@// step1,subFFTSize*/

#define x0r     s0
#define x0i     s1
#define y0r     s2
#define y0i     s3
#define x1r     s4
#define x1i     s5
#define w1r     s2
#define w1i     s3
#define w0r     s6
#define w0i     s7
#define y1r     s2              /*@// w1r,w1i*/
#define y1i     s3
#define st0     s8
#define st1     s9
#define st2     s10
#define st3     s11
#define st4     s12
#define st5     s13
#define half    s15




    @// Allocate stack memory required by the function



    @// Write function header
        M_START     omxSP_FFTFwd_RToCCS_F32_Sfs_vfp,r11

@ Structure offsets for FFTSpec
        .set    ARMsFFTSpec_N, 0
        .set    ARMsFFTSpec_pBitRev, 4
        .set    ARMsFFTSpec_pTwiddle, 8
        .set    ARMsFFTSpec_pBuf, 12

        @// Define stack arguments

        @// Setup half value
        movw    N, #0                   @// Use N as a temp.
        movt    N, #0x3f00
        vmov.f32 half, N

        @// Read the size from structure and take log
        LDR     N, [pFFTSpec, #ARMsFFTSpec_N]

        @// Read other structure parameters
        LDR     pTwiddle, [pFFTSpec, #ARMsFFTSpec_pTwiddle]
        LDR     pOut, [pFFTSpec, #ARMsFFTSpec_pBuf]

        @//  N=1 Treat seperately
        CMP     N,#1
        BGT     sizeGreaterThanOne
        // N<=1 is not supported
        @// Set return value
        MOV     result, #OMX_Sts_NoErr
        B       FunctionEnd

sizeGreaterThanOne:
        @// Do a N/2 point complex FFT including the scaling

        MOV     N,N,ASR #1              @// N/2 point complex FFT
        CLZ     order,N                 @// N = 2^order
        RSB     order,order,#31
        MOV     subFFTSize,#1
        @//MOV     subFFTNum,N


        CMP     order,#1
        BGT     orderGreaterthan1       @// order > 1
        vldmlt.f32 pSrc, {x0r, x0i}
        vstmlt.f32 pOut, {x0r, x0i}
        MOVLT   pSrc,pOut
        MOVLT   argDst,pDst
        BLT     FFTEnd

        MOV     argDst,pOut             @// Set input args to fft stages
        MOV     pOut,pDst               @// Set input args to fft stages
        MOV     argTwiddle,pTwiddle

        BL    armSP_FFTFwd_CToC_FC32_Radix2_fs_OutOfPlace_unsafe_vfp
        B     finalComplexToRealFixup

orderGreaterthan1:

        TST     order, #2               @// Set input args to fft stages
        MOVEQ   argDst,pDst
        MOVNE   argDst,pOut
        MOVNE   pOut,pDst               @// Pass the first stage dest in RN5
        MOV     argTwiddle,pTwiddle

        @//check for even or odd order

        @// NOTE: The following combination of BL's would work fine
        @// eventhough the first BL would corrupt the flags. This is
        @// because the end of the "grpZeroSetLoop" loop inside
        @// armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp sets
        @// the Z flag to EQ

        TST     order,#0x00000001
        BLEQ    armSP_FFTFwd_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp
        BLNE    armSP_FFTFwd_CToC_FC32_Radix8_fs_OutOfPlace_unsafe_vfp

unscaledRadix4Loop:
        CMP        subFFTNum,#1
         BEQ        FFTEnd
         BL        armSP_FFTFwd_CToC_FC32_Radix4_OutOfPlace_unsafe_vfp
         B        unscaledRadix4Loop

FFTEnd:
finalComplexToRealFixup:

        @// step = N/2 * 8 bytes
        MOV     step,subFFTSize,LSL #3
        @// twStep = 3N/8 * 8 bytes pointing to W^1
        SUB     twStep,step,subFFTSize,LSL #1
        @// step1 = N/4 * 8 = N/2*4 bytes
        MOV     step1,subFFTSize,LSL #2
        @// (N/4-1)*8 bytes
        SUB     step1,step1,#8

        @// F(0) = 1/2 [Z(0) + Z'(0)] - j [Z(0) - Z'(0)]
        @// 1/2 [(a+jb) + (a-jb)] - j  [(a+jb) - (a-jb)]
        @// 1/2 [2a+j0] - j [0+j2b]
        @// (a+b, 0)

        @// F(N/2) =1/2 [Z(0) + Z'(0)] + j [Z(0) - Z'(0)]
        @// 1/2 [(a+jb) + (a-jb)] + j  [(a+jb) - (a-jb)]
        @// 1/2 [2a+j0] + j [0+j2b]
        @// (a-b, 0)

        @// F(0) and F(N/2)
        vldm.f32 pSrc!, {x0r, x0i}
        vadd.f32 y0r,x0r,x0i            @// F(0) = (2(Z0.r+Z0.i) , 0)
        vsub.f32 x0r,x0r,x0i            @// F(N/2) = (2(Z0.r-Z0.i) , 0)
        vsub.f32 y0i, y0i               @ y0i and x0i set to 0.0
        vsub.f32 x0i, x0i

        add      argDst, step
        vstm.f32 argDst, {x0r, x0i}     @// {x0r,x0i}->[argDst, step]
        sub      argDst, step
        vstm.f32 argDst!, {y0r, y0i}

        SUBS    subFFTSize,subFFTSize,#2

        ADD     pTwiddleTmp,argTwiddle,#8       @// W^2
        ADD     argTwiddle,argTwiddle,twStep    @// W^1
        BLT     End
        BEQ     lastElement


        @// F(k) = 1/2 [Z(k) +  Z'(N/2-k)] -j*W^(k) [Z(k) -  Z'(N/2-k)]
        @// Process 2 elements at a time. E.g: F(1) and F(N/2-1) since
        @// both of them require Z(1) and Z(N/2-1)

        ASR     subFFTSize,subFFTSize,#1
evenOddButterflyLoop:

        SUB     step,step,#16           @// (N/2-2)*8 bytes

        add      pSrc, step
        vldm.f32 pSrc, {x1r, x1i}       @// {x1r, x1i} = [pSrc, step]
        sub      pSrc, step
        vldm.f32 pSrc!, {x0r, x0i}
        add      argTwiddle, step1
        vldm.f32 argTwiddle, {w1r, w1i}  @// {w1r, w1i} = [argTwiddle, step1]
        sub      argTwiddle, step1
        vldm.f32 argTwiddle!, {w0r, w0i} @// {w0r, w0i} = [argTwiddle], #8

        SUB     step1,step1,#8
        SUBS    subFFTSize,subFFTSize,#1

        vsub.f32 st2,x0r,x1r            @// a-c
        vadd.f32 st3,x0i,x1i            @// b+d
        vadd.f32 st0,x0r,x1r            @// a+c
        vsub.f32 st1,x0i,x1i            @// b-d

        vmul.f32 x1r,w1r,st2
        vmul.f32 x1i,w1r,st3
        vmla.f32 x1r,w1i,st3            @// x1r = w1r*st2 + w1i*st3
        @//RSB     x1r,x1r,#0
        vmls.f32 x1i,w1i,st2            @// x1i = w1r*st3 - wli*st2

        vsub.f32 y1r, st0, x1i
        vadd.f32 y1i, x1r, st1
        vneg.f32 y1i, y1i

        vmul.f32  x0r,w0r,st2
        vmul.f32  x0i,w0r,st3
        vmls.f32  x0r,w0i,st3           @// x0r = w0r*st2 - w0i*st3
        vmla.f32  x0i,w0i,st2           @// x0i = w0r*st3 + x0i*st1

        vsub.f32   st4,st0,x0i          @// F(1)
        vadd.f32   st5,x0r,st1


        vmul.f32 y1r, half
        vmul.f32 y1i, half
        vmul.f32 st4, half
        vmul.f32 st5, half

        add      argDst, step
        vstm.f32 argDst, {y1r, y1i}     @// {y1r,y1i} -> [argDst,step]
        sub      argDst, step
        vstm.f32 argDst!, {st4, st5}


        MOV     t0,argTwiddle           @// swap ptr for even and odd twiddles
        MOV     argTwiddle,pTwiddleTmp
        MOV     pTwiddleTmp,t0

        BGT     evenOddButterflyLoop

        @// Last element can be expanded as follows
        @// 1/2[Z(k) + Z'(k)] + j w^k [Z(k) - Z'(k)]
        @// 1/2[(a+jb) + (a-jb)] + j w^k [(a+jb) - (a-jb)]
        @// 1/2[2a+j0] + j (c+jd) [0+j2b]
        @// (a-bc, -bd)

lastElement:
        vldm.f32 pSrc, {x0r, x0i}
        vneg.f32 x0i, x0i
        vstm.f32 argDst, {x0r, x0i}

End:
        @// Set return value
        MOV     result, #OMX_Sts_NoErr

FunctionEnd:
        @// Write function tail
        M_END

@//    ENDIF                                           @//ARM1136JS


    @// Guarding implementation by the processor name



    .end
