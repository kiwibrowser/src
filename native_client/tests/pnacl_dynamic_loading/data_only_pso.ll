; Copyright (c) 2014 The Native Client Authors. All rights reserved.
; Use of this source code is governed by a BSD-style license that can be
; found in the LICENSE file.

; This is an example PSO that contains no code, only data, and requires no
; relocations.  This is for testing corner cases in the dynamic ELF loader.

target datalayout = "p:32:32:32"

@__pnacl_pso_root = global [8 x i8] c"\11\22\33\44\AA\BB\CC\DD", align 4
