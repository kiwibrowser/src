/*
 * This is a dummy linker script used as crt1.o.
 * The actual startup code is just the _start function defined in a library.
 * We provide this file for two purposes:
 * 1. To keep with the traditional linking sequence that puts crt1.o first.
 * 2. To generate references to the main and exit symbols like the real
 *    startup code would, so that they will be brought in from libraries
 *    before -lc is encountered in the link.  Otherwise a main defined in
 *    a library wouldn't be referenced until after that library had already
 *    been examined, and _start's call would get an undefined reference.
 */

EXTERN ( main exit _exit )
