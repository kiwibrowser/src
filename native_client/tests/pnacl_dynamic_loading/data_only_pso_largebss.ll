; Copyright (c) 2014 The Native Client Authors. All rights reserved.
; Use of this source code is governed by a BSD-style license that can be
; found in the LICENSE file.

; This is an example PSO that contains no code and a large BSS.  This
; is for testing corner cases in the dynamic ELF loader.

target datalayout = "p:32:32:32"

@bss_var = internal global [1000000 x i8] zeroinitializer

@__pnacl_pso_root = global i32 ptrtoint ([1000000 x i8]* @bss_var to i32),
    align 4
