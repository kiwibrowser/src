/*
 * Copyright (C) 2013 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AARCH64_ELF_MACHDEP_H_
#define _AARCH64_ELF_MACHDEP_H_

/* Null relocations */
#define R_ARM_NONE                      0
#define R_AARCH64_NONE                  256

/* Static Data relocations */
#define R_AARCH64_ABS64                 257
#define R_AARCH64_ABS32                 258
#define R_AARCH64_ABS16                 259
#define R_AARCH64_PREL64                260
#define R_AARCH64_PREL32                261
#define R_AARCH64_PREL16                262

#define R_AARCH64_MOVW_UABS_G0          263
#define R_AARCH64_MOVW_UABS_G0_NC       264
#define R_AARCH64_MOVW_UABS_G1          265
#define R_AARCH64_MOVW_UABS_G1_NC       266
#define R_AARCH64_MOVW_UABS_G2          267
#define R_AARCH64_MOVW_UABS_G2_NC       268
#define R_AARCH64_MOVW_UABS_G3          269
#define R_AARCH64_MOVW_SABS_G0          270
#define R_AARCH64_MOVW_SABS_G1          271
#define R_AARCH64_MOVW_SABS_G2          272

/* PC-relative addresses */
#define R_AARCH64_LD_PREL_LO19          273
#define R_AARCH64_ADR_PREL_LO21         274
#define R_AARCH64_ADR_PREL_PG_HI21      275
#define R_AARCH64_ADR_PREL_PG_HI21_NC   276
#define R_AARCH64_ADD_ABS_LO12_NC       277
#define R_AARCH64_LDST8_ABS_LO12_NC     278

/* Control-flow relocations */
#define R_AARCH64_TSTBR14               279
#define R_AARCH64_CONDBR19              280
#define R_AARCH64_JUMP26                282
#define R_AARCH64_CALL26                283
#define R_AARCH64_LDST16_ABS_LO12_NC    284
#define R_AARCH64_LDST32_ABS_LO12_NC    285
#define R_AARCH64_LDST64_ABS_LO12_NC    286
#define R_AARCH64_LDST128_ABS_LO12_NC   299

#define R_AARCH64_MOVW_PREL_G0          287
#define R_AARCH64_MOVW_PREL_G0_NC       288
#define R_AARCH64_MOVW_PREL_G1          289
#define R_AARCH64_MOVW_PREL_G1_NC       290
#define R_AARCH64_MOVW_PREL_G2          291
#define R_AARCH64_MOVW_PREL_G2_NC       292
#define R_AARCH64_MOVW_PREL_G3          293

/* Dynamic relocations */
#define R_AARCH64_COPY                  1024
#define R_AARCH64_GLOB_DAT              1025    /* Create GOT entry.  */
#define R_AARCH64_JUMP_SLOT             1026    /* Create PLT entry.  */
#define R_AARCH64_RELATIVE              1027    /* Adjust by program base.  */
#define R_AARCH64_TLS_TPREL64           1030
#define R_AARCH64_TLS_DTPREL32          1031
#define R_AARCH64_IRELATIVE             1032

#endif /* _AARCH64_ELF_MACHDEP_H_ */
