; This is a smoke test of constant blinding and constant pooling.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    | FileCheck %s --check-prefix=BLINDINGO2
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=randomize \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    | FileCheck %s --check-prefix=BLINDINGOM1

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=pool \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    | FileCheck %s --check-prefix=POOLING
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:    -sz-seed=1 -randomize-pool-immediates=pool \
; RUN:    -randomize-pool-threshold=0x1 \
; RUN:    | FileCheck %s --check-prefix=POOLING


define internal i32 @add_arg_plus_200000(i32 %arg) {
entry:
  %res = add i32 200000, %arg
  ret i32 %res

; BLINDINGO2-LABEL: add_arg_plus_200000
; BLINDINGO2: mov [[REG:e[a-z]*]],0x669f4eea
; BLINDINGO2-NEXT: lea [[REG]],{{[[]}}[[REG]]-0x669c41aa{{[]]}}

; BLINDINGOM1-LABEL: add_arg_plus_200000
; BLINDINGOM1: mov [[REG:e[a-z]*]],0x669f4eea
; BLINDINGOM1-NEXT: lea [[REG]],{{[[]}}[[REG]]-0x669c41aa{{[]]}}

; POOLING-LABEL: add_arg_plus_200000
; POOLING: mov e{{[a-z]*}},{{(DWORD PTR )?}}ds:0x0 {{[0-9a-f]*}}: R_386_32 .L$i32$00030d40
}

define internal float @load_arg_plus_200000(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = add i32 %arg.int, 200000
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; BLINDINGO2-LABEL: load_arg_plus_200000
; BLINDINGO2: lea [[REG:e[a-z]*]],{{[[]}}{{e[a-z]*}}+0x69ed4ee7{{[]]}}

; BLINDINGOM1-LABEL: load_arg_plus_200000
; BLINDINGOM1: lea [[REG:e[a-z]*]],{{[[]}}{{e[a-z]*}}-0x69ea41a7{{[]]}}

; POOLING-LABEL: load_arg_plus_200000
; POOLING: mov e{{[a-z]*}},{{(DWORD PTR )?}}ds:0x0 {{[0-9a-f]*}}: R_386_32 .L$i32$00030d40
}

define internal i64 @add_arg_plus_64bits(i32 %arg) {
entry:
  %0 = sext i32 %arg to i64
  %res = add i64 90000000000, %0
  ret i64 %res

; BLINDINGO2-LABEL: add_arg_plus_64bits
; BLINDINGO2: sar [[RHI:e[a-z]*]],0x1f
; BLINDINGO2: mov [[RLO:e[a-z]*]],0x61a345a8
; BLINDINGO2-NEXT: lea [[RLO]],{{[[]}}[[RLO]]-0x6d3841a8{{[]]}}

; BLINDINGOM1-LABEL: add_arg_plus_64bits
; BLINDINGOM1: sar [[RHI:e[a-z]*]],0x1f
; BLINDINGOM1: mov [[RLO:e[a-z]*]],0x61a345a8
; BLINDINGOM1-NEXT: lea [[RLO]],{{[[]}}[[RLO]]-0x6d3841a8{{[]]}}

; POOLING-LABEL: add_arg_plus_64bits
; POOLING: mov e{{[a-z]*}},{{(DWORD PTR )?}}ds:0x0 {{[0-9a-f]*}}: R_386_32 .L$i32$f46b0400
}

define internal i64 @load_arg_plus_64bits(i64* %arg) {
entry:
  %arg.int = ptrtoint i64* %arg to i32
  %arg.new = add i32 %arg.int, 90000
  %arg.ptr = inttoptr i32 %arg.new to i64*
  %arg.load = load i64, i64* %arg.ptr, align 1
  ret i64 %arg.load

; BLINDINGO2-LABEL: load_arg_plus_64bits
; BLINDINGO2: lea e{{[a-z]*}},{{[[]}}e{{[a-z]*}}+0x7087a139{{[]]}}
; BLINDINGO2: mov e{{[a-z]*}},DWORD PTR {{[[]}}e{{[a-z]*}}-0x708641a9{{[]]}}

; BLINDINGOM1-LABEL: load_arg_plus_64bits
; BLINDINGOM1: mov e{{[a-z]*}},0x7087a139
; BLINDINGOM1-NEXT: lea e{{[a-z]*}},{{[[]}}e{{[a-z]*}}-0x708641a9{{[]]}}

; POOLING-LABEL: load_arg_plus_64bits
; POOLING: mov e{{[a-z]x}},{{(DWORD PTR )?}}ds:0x0 {{[0-9a-f]*}}: R_386_32 .L$i32$00000004
}

define internal i32 @add_const_8bits(i32 %a) {
entry:
  %a_8 = trunc i32 %a to i8
  %add = add i8 %a_8, 123
  %ret = zext i8 %add to i32
  ret i32 %ret

; BLINDINGO2-LABEL: add_const_8bits
; BLINDINGO2: mov e{{[a-z]*}},0x73d44225
; BLINDINGO2-NEXT: e{{[a-z]*}},{{[[]}}e{{[a-z]*}}-0x73d441aa{{[]]}}

; BLINDINGOM1-LABEL: add_const_8bits
; BLINDINGOM1: mov e{{[a-z]*}},0x73d44225
; BLINDINGOM1-NEXT: e{{[a-z]*}},{{[[]}}e{{[a-z]*}}-0x73d441aa{{[]]}}

; POOLING-LABEL: add_const_8bits
; POOLING: mov {{[a-z]l}},{{(BYTE PTR )?}}ds:0x0 {{[0-9a-f]*}}: R_386_32 .L$i8$0000007b
}

define internal i32 @add_const_16bits(i32 %a) {
entry:
  %a_16 = trunc i32 %a to i16
  %add = add i16 %a_16, 32766
  %ret = zext i16 %add to i32
  ret i32 %ret

; BLINDINGO2-LABEL: add_const_16bits
; BLINDINGO2: mov e{{[a-z]*}},0x7722c1a5
; BLINDINGO2-NEXT: e{{[a-z]*}},{{[[]}}e{{[a-z]*}}-0x772241a7{{[]]}}

; BLINDINGOM1-LABEL: add_const_16bits
; BLINDINGOM1: mov e{{[a-z]*}},0x7722c1a5
; BLINDINGOM1-NEXT: e{{[a-z]*}},{{[[]}}e{{[a-z]*}}-0x772241a7{{[]]}}

; POOLING-LABEL: add_const_16bits
; POOLING: mov {{[a-z]x}},{{(WORD PTR )?}}ds:0x0 {{[0-9a-f]*}}: R_386_32 .L$i16$00007ffe

}
