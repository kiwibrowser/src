all:
	cl \
    sidestep/ia32_modrm_map.cc \
    sidestep/ia32_opcode_map.cc \
    sidestep/mini_disassembler.cc \
    main.cc \
    /Fetraceline.exe \
    /D_WIN32_WINNT=0x0500 \
    /D_HAS_EXCEPTIONS=0 \
    "/I." \
    /Zi \
  /link \
    kernel32.lib user32.lib dbghelp.lib powrprof.lib

tests:
	cl \
    assembler_unittest.cc \
    /Fassembler_unittest.exe \
    /D_WIN32_WINNT=0x0500 \
    /D_HAS_EXCEPTIONS=0 \
    /Zi \
  /link \
    kernel32.lib user32.lib dbghelp.lib powrprof.lib

stubs : stubs.asm
	nasm -o stubs stubs.asm
	ndisasm -u stubs

clean:
	rm -f *.obj {cpuinfo,traceline,assembler_unittest,vc80}.{exe,ilk,pdb} stubs
