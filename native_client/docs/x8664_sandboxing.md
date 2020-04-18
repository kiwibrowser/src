## Function return

C code: `void foo() {}
`

Assembler fragment: `nacljmp %r11d,%r15
`

Disassembly fragment: `7: 41 83 e3 e0 and $0xffffffffffffffe0,%r11d b: 4d 01 fb
add %r15,%r11 e: 41 ff e3 jmpq *%r11
`

## Memory read

C code: `int get_word(int *x) { return *x; }
`

Assembler fragment: `movl %nacl:(%r15,%rax), %eax
`

Disassembly fragment: `2a: 89 c0 mov %eax,%eax 2c: 41 8b 04 07 mov
(%r15,%rax,1),%eax
`

The instruction `mov %eax,%eax` is not a no-op; it truncates `%rax` to 32 bits.

Sandboxing of reads is optional. Without it, we get integrity but not
necessarily confidentiality, depending on what else is mapped into address
space. sel\_ldr would need to be careful not to map sensitive information. We
may need to worry about the presence of other DLLs (see
[WindowsDLLInjectionProblem](windows_dll_injection_problem.md)). We would want
sandboxing of reads in cases where we want to enforce DeterministicExecution.

## Memory write

C code: `int put_word(int *x) { *x = 0x100; }
`

Assembler fragment: `movl $256, %nacl:(%r15,%rax)
`

Disassembly fragment: `4a: 89 c0 mov %eax,%eax 4c: 41 c7 04 07 00 01 00 movl
$0x100,(%r15,%rax,1)
`

## Data segment addressing

C code: `static int var; void set_var() { var = 0x100; }
`

Assembler fragment: `movl $256, var(%rip)
`

Disassembly fragment: `104: c7 05 00 00 00 00 00 movl $0x100,0x0(%rip) 10b: 01
00 00 106: R_X86_64_PC32 .bss-0x8
`

RIP-relative addressing is always safe. You can only use 2GB offset from %rip
(x86-64 limitation) and so can not address anything outside of protected area,
which has 40GB unmapped gaps before and after.
