@//
@//  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
@//
@//  Use of this source code is governed by a BSD-style license
@//  that can be found in the LICENSE file in the root of the source
@//  tree. An additional intellectual property rights grant can be found
@//  in the file PATENTS.  All contributing project authors may
@//  be found in the AUTHORS file in the root of the source tree.
@//
@//  This is a modification of omxSP_FFTInv_CToC_SC32_Sfs_s.s
@//  to support float instead of SC32.
@//

@//
@// Description:
@// Compute an inverse FFT for a complex signal
@//
@//


@// Include standard headers

#include "dl/api/arm/armCOMM_s.h"
#include "dl/api/arm/omxtypes_s.h"

@//        M_VARIANTS ARM1136JS

@// Import symbols required from other files
@// (For example tables)

        .extern  armSP_FFTInv_CToC_FC32_Sfs_Radix2_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTInv_CToC_FC32_Radix8_fs_OutOfPlace_unsafe_vfp
        .extern  armSP_FFTInv_CToC_FC32_Radix4_OutOfPlace_unsafe_vfp

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
#define diffMinusOne    r2
#define round           r3

#define x0r     s0
#define x0i     s1
#define fone    s2
#define fscale  s3


    @// Allocate stack memory required by the function

    @// Write function header
        M_START     omxSP_FFTInv_CToC_FC32_Sfs_vfp,r11

@ Structure offsets for FFTSpec
        .set    ARMsFFTSpec_N, 0
        .set    ARMsFFTSpec_pBitRev, 4
        .set    ARMsFFTSpec_pTwiddle, 8
        .set    ARMsFFTSpec_pBuf, 12

        @// Define stack arguments

        @// Read the size from structure and take log
        LDR     N, [pFFTSpec, #ARMsFFTSpec_N]

        @// Read other structure parameters
        LDR     pTwiddle, [pFFTSpec, #ARMsFFTSpec_pTwiddle]
        LDR     pOut, [pFFTSpec, #ARMsFFTSpec_pBuf]

        CLZ     order,N                 @// N = 2^order
        RSB     order,order,#31
        MOV     subFFTSize,#1
        @//MOV     subFFTNum,N

        CMP     order,#1
        BGT     orderGreaterthan1       @// order > 1
        @// Order = 0 or 1
        vldmlt.f32 pSrc, {x0r, x0i}
        vstmlt.f32 pDst, {x0r, x0i}

        MOVLT   pSrc,pDst
        BLT     FFTEnd

        @// Handle order = 1
        MOV     argDst,pDst
        MOV     argTwiddle,pTwiddle

        BL      armSP_FFTInv_CToC_FC32_Radix2_fs_OutOfPlace_unsafe_vfp
        B       FFTEnd

orderGreaterthan1:

        TST     order, #2               @// Set input args to fft stages
        MOVNE   argDst,pDst
        MOVEQ   argDst,pOut
        MOVEQ   pOut,pDst               @// Pass the first stage dest in RN5
        MOV     argTwiddle,pTwiddle


        @//check for even or odd order
        @// NOTE: The following combination of BL's would work fine
        @// eventhough the first BL would corrupt the flags. This is
        @// because the end of the "grpZeroSetLoop" loop inside
        @// armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp sets
        @// the Z flag to EQ

        TST     order,#0x00000001
        BLEQ    armSP_FFTInv_CToC_FC32_Radix4_fs_OutOfPlace_unsafe_vfp
        BLNE    armSP_FFTInv_CToC_FC32_Radix8_fs_OutOfPlace_unsafe_vfp

unscaledRadix4Loop:
        CMP        subFFTNum,#1
         BEQ        FFTEnd
         BL        armSP_FFTInv_CToC_FC32_Radix4_OutOfPlace_unsafe_vfp
         B        unscaledRadix4Loop


FFTEnd:

        vldm.f32 pSrc, {x0r, x0i}

        vmov.f32     fscale, subFFTSize
        vcvt.f32.s32 fscale, fscale             @// fscale = N as a float
        movw         round, #0
        movt         round, #0x3f80             @// round = 1.0
        vmov.f32     fone, round
        vdiv.f32     fscale, fone, fscale       @// fscale = 1/N
scaleFFTData:                                   @// N = subFFTSize
        SUBS    subFFTSize,subFFTSize,#1
        vmul.f32 x0r, x0r, fscale
        vmul.f32 x0i, x0i, fscale
        vstm.f32 pSrc, {x0r, x0i}
        add      pSrc, #8
        vldmgt.f32 pSrc, {x0r, x0i}

        bgt     scaleFFTData


        @// Set return value
        MOV     result, #OMX_Sts_NoErr

        @// Write function tail
        M_END

@//    ENDIF                                           @//ARM1136JS


      @// Guarding implementation by the processor name



    .end
