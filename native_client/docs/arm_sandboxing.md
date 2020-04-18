## Function return

C code: `void foo() {}
`

Disassembly fragment: `0: e3cee2ff bic lr, lr, #-268435441 ; 0xf000000f 4:
e12fff1e bx lr
`

## Memory read

C code: `int get_word(int *x) { return *x; }
`

Disassembly fragment: `0: e5900000 ldr r0, [r0]
`

Read sandboxing is off by default, so this is the usual ARM instruction.

## Memory write

C code: `int put_word(int *x) { *x = 0x100; }
`

Disassembly fragment: `4: e3c00103 bic r0, r0, #-1073741824 ; 0xc0000000 8:
e5801000 str r1, [r0]
`
