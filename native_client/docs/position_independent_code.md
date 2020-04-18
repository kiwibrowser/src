# Position independent code

## i386

Normal i386 PIC PLT entries look like this: `somefunc@plt: jmp
*somefunc_pltgot_index(%ebx) somefunc_slow_path: push $somefunc_pltgot_index jmp
zeroth_plt_entry
` This fits neatly into 16 bytes.

Under NaCl this takes up 64 bytes: `somefunc@plt: movl
somefunc_pltgot_index(%ebx), %ecx andl %ecx, NACLMASK jmp *%ecx hlt; hlt; ... //
pad to 32 bytes somefunc_slow_path: push $somefunc_pltgot_index jmp
zeroth_plt_entry hlt; hlt; ... // pad to 32 bytes
` See [issue 230](http://code.google.com/p/nativeclient/issues/detail?id=230)
for an idea about how to make this smaller.

## ARM

Normal ARM PLT entries look like this: `somefunc@plt: add ip, pc, #0x0XX00000
add ip, ip, #0x000YY000 ldr pc, [ip, #0xZZZ]!
` This is the same for PIC and non-PIC code. This leaves the address of the GOT
entry in `ip`, so there is no need for a separate slow-path code sequence for
filling out the GOT entry address. The displacement of the GOT entry from the
PLT entry is 0x0XXYYZZZ, so the maximum displacement is 256MB.

Under NaCl this would become something like this: `somefunc@plt: add ip,
pc, #0x0XX00000 add ip, ip, #0x000YY000 ldr %r7, [ip, #0xZZZ]! // Assuming no
read sandboxing bic pc, %r7, #NACL_ARM_MASK
` (or whatever register is typically used for this, probably not %r7 (on Google
Code))

Or if the GOT and PLT are more than 256MB apart, it would have to become:
`somefunc@plt: add ip, pc, #0xWW000000 add ip, ip, #0x00XX0000 add ip,
ip, #0x0000YY00 ldr %r7, [ip, #0xZZ]! // Assuming no read sandboxing bic pc,
%r7, #NACL_ARM_MASK nop nop nop
`
