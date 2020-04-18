/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * vcpuid.c
 *
 * Verify correctness of CPUID implementation.
 *
 * This uses shell status code to indicate its result; non-zero return
 * code indicates the CPUID instruction is not implemented or not
 * implemented correctly.
 *
 * The asm_ tests use inline assembler, which is not supported on
 * 64-bit Windows. We use #if to prevent these routines from being
 * provided on 64-bit Windows.
 *
 * TODO(bradchen): test on a machine with SSE4.
 */
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/platform_qualify/nacl_cpuwhitelist.h"

/* MAGIC_CONST is a 32 bit constant, somewhat arbitrarily choosen. */
/* The value should be a valid (i.e. not denormal single-precision */
/* float; otherwise unexpected FP exceptions are possible.         */
const int kMagicConst = 0xc01df00d;
const int kMagicConst_ROUNDSS = 0xc0000000;
const int kMagicConst_POPCNT = 13;
const int kMagicConst_CRC32  = 0xb906c3ea;

#if !(NACL_WINDOWS && (NACL_BUILD_SUBARCH == 64))
static int asm_HasMMX(void) {
  volatile int before, after;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax         \n\t"
                   "xor %%ecx, %%ecx      \n\t"
                   "movd %%eax, %%mm0     \n\t" /* copy eax into mm0 (MMX) */
                   "movd %%mm0, %%ecx     \n\t" /* copy mm0 into ecx (MMX) */
                   "mov %%ecx, %0         \n\t"
                   : "=g" (after)
                   : "m" (before)
                   : "eax", "ecx", "mm0" );
#elif NACL_WINDOWS
  __asm {
      mov eax, before
      xor ecx, ecx
      movd mm0, eax
      movd ecx, mm0
      mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst);
}

/* TODO(brad): Test this routine on a machine with 3DNow */
static int asm_Has3DNow(void) {
  volatile int before, after;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax         \n\t"
                   "movd %%eax, %%mm0     \n\t" /* copy eax into mm0 (MMX) */
                   "pfadd %%mm0, %%mm0    \n\t" /* 3DNow! */
                   "movd %%mm0, %%ecx     \n\t" /* copy mm0 into ecx (MMX) */
                   "mov %%ecx, %0         \n\t"
                   "emms                  \n\t"
                   : "=g" (after)
                   : "m" (before)
                   : "%eax", "%ecx");
#elif NACL_WINDOWS
  __asm {
      mov eax, before
      movd mm0, eax
      pfadd mm0, mm0
      movd ecx, mm0
      mov after, ecx
      emms
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst + 0x800000);
}


static int asm_HasSSE(void) {
  volatile int before, after;
  before = kMagicConst;
  after = 0;
#if defined(__GNUC__)
  __asm__ volatile("movss  %1, %%xmm0     \n\t"
                   /* copy before into xmm0 (SSE2) */
                   "movss %%xmm0, %0      \n\t" /* copy xmm0 into after (SSE) */
                   : "=g" (after)
                   : "m" (before)
                   : "xmm0" );
#elif NACL_WINDOWS
  __asm {
    movss xmm0, before
    movss after, xmm0
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst);
}

static int asm_HasSSE2(void) {
  volatile int before, after;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax         \n\t"
                   "xor %%ecx, %%ecx      \n\t"
                   "movd %%eax, %%xmm0    \n\t" /* copy eax into xmm0 (SSE2) */
                   "movd %%xmm0, %%ecx    \n\t" /* copy xmm0 into ecx (SSE2) */
                   "mov %%ecx, %0         \n\t"
                   : "=g" (after)
                   : "m" (before)
                   : "eax", "ecx", "xmm0");
#elif NACL_WINDOWS
  __asm {
    mov eax, before
    xor ecx, ecx
    movd xmm0, eax
    movd ecx, xmm0
    mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst);
}

static int asm_HasSSE3(void) {
  volatile int before, after;
  after = 0;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax             \n\t"
                   "movd %%eax, %%xmm0        \n\t"
                   "movddup %%xmm0, %%xmm1    \n\t"
                   "movd %%xmm1, %%ecx        \n\t"
                   "mov %%ecx, %0             \n\t"
                   : "=g" (after)
                   : "m" (before)
                   : "eax", "ecx", "xmm0", "xmm1" );
#elif NACL_WINDOWS
  __asm {
    mov eax, before
    movd xmm0, eax
    movddup xmm1, xmm0
    movd ecx, xmm1
    mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst);
}

static int asm_HasSSSE3(void) {
  volatile int after;
#if defined(__GNUC__)
  __asm__ volatile("mov $0x0000ffff, %%eax  \n\t"
                   "xor %%ecx, %%ecx        \n\t"
                   "movd %%eax, %%mm0       \n\t" /* copy eax into mm0 (MMX) */
                   /* pabsw will change 0x0000ffff to 0x00000001 */
                   "pabsw %%mm0, %%mm0      \n\t"
                   "movd %%mm0, %%ecx       \n\t" /* copy mm0 into ecx (MMX) */
                   "mov %%ecx, %0           \n\t"
                   "emms                    \n\t"
                   : "=g" (after)
                   :
                   : "eax", "ecx", "mm0");
#elif NACL_WINDOWS
  __asm {
    mov eax, 0x0000ffff
    xor ecx, ecx
    movd mm0, eax
    pabsw mm0, mm0
    movd ecx, mm0
    mov after, ecx
    emms
  }
#else
# error Unsupported platform
#endif
  return (after == 1);
}

static int asm_HasSSE41(void) {
  volatile int before, after;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax              \n\t"
                   "movd %%eax, %%xmm0         \n\t"
                   "roundss $0, %%xmm0, %%xmm0 \n\t"
                   "movd %%xmm0, %%ecx         \n\t"
                   "mov %%ecx, %0              \n\t"
                   : "=g" (after)
                   : "g" (before)
                   : "eax", "ecx", "xmm0" );
#elif NACL_WINDOWS
  __asm {
    mov eax, before
    movd xmm0, eax
    /*
     * NOTE: Use _emit for older MSFT compilers that don't know of SSE4
     * 66 0f 3a 0a c0 00      roundss $0, xmm0, xmm0
     */
    _emit 0x66
    _emit 0x0f
    _emit 0x3a
    _emit 0x0a
    _emit 0xc0
    _emit 0x00
    movd ecx, xmm0
    mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst_ROUNDSS);
}

static int asm_HasSSE42(void) {
  volatile int after;
#if defined(__GNUC__)
  __asm__ volatile("mov $0x0000ffff, %%eax  \n\t"
                   "xor %%ecx, %%ecx        \n\t"
                   "crc32 %%eax, %%ecx      \n\t"
                   "mov %%ecx, %0           \n\t"
                   : "=g" (after)
                   :
                   : "eax", "ecx" );
#elif NACL_WINDOWS
  __asm {
    mov eax, 0x0000ffff
    xor ecx, ecx
    /*
     * NOTE: Use _emit for older MSFT compilers that don't know of SSE4
     * f2 0f 38 f1 c8   crc32 ecx, eax
     */
    _emit 0xf2
    _emit 0x0f
    _emit 0x38
    _emit 0xf1
    _emit 0xc8
    mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst_CRC32);
}

static int asm_HasPOPCNT(void) {
  volatile int before, after;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax         \n\t"
                   "xor %%ecx, %%ecx      \n\t"
                   "popcnt %%eax, %%ecx   \n\t"
                   "mov %%ecx, %0         \n\t"
                   : "=g" (after)
                   : "m" (before)
                   : "eax", "ecx");
#elif NACL_WINDOWS
  __asm {
    mov eax, before
    xor ecx, ecx
    /*
     * NOTE: Use _emit for older MSFT compilers that don't know of SSE4
     * f3 0f b8 c8   popcnt ecx, eax
     */
    _emit 0xf3
    _emit 0x0f
    _emit 0xb8
    _emit 0xc8
    mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst_POPCNT);
}

static int asm_HasCMOV(void) {
  volatile int before, after;
  before = kMagicConst;
#if defined(__GNUC__)
  __asm__ volatile("mov %1, %%eax          \n\t"
                   "xor %%ecx, %%ecx       \n\t"
                   "add $0, %%eax          \n\t"  /* to set condition code */
                   "cmovnz %%eax, %%ecx    \n\t"
                   "mov %%ecx, %0          \n\t"
                   : "=g" (after)
                   : "m" (before)
                   : "eax", "ecx");
#elif NACL_WINDOWS
  __asm {
    mov eax, before
    xor ecx, ecx
    add eax, 0
    cmovnz ecx, eax
    mov after, ecx
  }
#else
# error Unsupported platform
#endif
  return (after == kMagicConst);
}

static int asm_HasTSC(void) {
  uint32_t _eax, _edx;
  _eax = 0;
  _edx = 0;

#if defined(__GNUC__)
  __asm__ volatile("rdtsc"
                   : "=a" (_eax), "=d" (_edx)
                   );
#elif NACL_WINDOWS
  __asm {
    rdtsc
    mov _eax, eax
    mov _edx, ecx
  }
#else
# error Unsupported platform
#endif
  return ((_eax | _edx) != 0);
}

static int asm_HasX87(void) {
#if defined(__GNUC__)
  __asm__ volatile("fld1          \n\t"
                   "fstp %st(0)    \n\t");
  return 1;
#elif NACL_WINDOWS
  __asm {
    fld1
    fstp st(0)
  }
  return 1;
#else
# error Unsupported platform
#endif
}

#if 0
/* I'm having some trouble with my cmpxchg8b instruction */
static int asm_HasCX8(void) {
  uint32_t _eax, _ebx, _ecx, _edx;
  uint64_t foo64 = 0;
  _eax = 0;
  _edx = 0;
  _ebx = 0;
  _ecx = kMagicConst;

#if defined(__GNUC__)
  __asm__ volatile("cmpxchg8b %0   \n\t"
                   : "=g" (foo64), "=b" (_ebx), "=c" (_ecx)
                   : "a" (_eax), "d" (_edx) );
#elif NACL_WINDOWS
  __asm {
  }
#else
# error Unsupported platform
#endif
  printf("ebx == %x  ecx == %x\n", _ebx, _ecx);
  return (foo64 == (uint64_t)kMagicConst);
}
#endif  /* 0 */
#endif  /* 64-bit Windows */

#if NACL_WINDOWS && (NACL_BUILD_SUBARCH == 64)
static int CheckCPUFeatureDetection(NaClCPUFeaturesX86 *cpuf) {
  /* Unfortunately the asm_ tests will not work on 64-bit Windows */
  return 0;
}
#else
#if (NACL_LINUX || NACL_OSX)
/* Linux/MacOS signal handling code, for trapping illegal instruction faults */
static int sawbadinstruction = 0;
static struct sigaction crash_detect;
static int signum;

sigjmp_buf crash_load;

void  handler_load(int  signum) {
  siglongjmp(crash_load, signum);
}

void  all_sigs(struct sigaction *new_action,
               struct sigaction *prev_action) {
  int               sig;
  struct sigaction  ign;

  for (sig = SIGHUP; sig < NSIG; ++sig) {
    switch (sig) {
      case SIGWINCH:
      case SIGCHLD:
      case SIGTSTP:
        break;
      default:
        (void) sigaction(sig, new_action, prev_action);
        break;
    }
  }
  ign.sa_handler = SIG_DFL;
  sigemptyset(&ign.sa_mask);
  ign.sa_flags = SA_ONSTACK;
  (void) sigaction(SIGWINCH, &ign, 0);
  (void) sigaction(SIGCHLD, &ign, 0);
  (void) sigaction(SIGTSTP, &ign, 0);
}

static void SignalInit(void) {
  sawbadinstruction = 0;
  crash_detect.sa_handler = handler_load;
  sigemptyset(&crash_detect.sa_mask);
  crash_detect.sa_flags = SA_RESETHAND;
  all_sigs(&crash_detect, 0);
}

static void SetSawBadInst(void) {
  sawbadinstruction = 1;
}

static int SawBadInst(void) {
  return sawbadinstruction != 0;
}

/*
 * DoTest tests for a particular CPU feature using thetest().
 * It returns 0 if the feature is present, 1 if it is not.
 */
static int DoTest(int (*thetest)(void), const char *s) {
  SignalInit();
  if (0 != (signum = sigsetjmp(crash_load, 1))) {
    SetSawBadInst();
    if (SIGILL == signum) {
      fprintf(stderr, "%s: illegal instruction\n", s);
    } else {
      fprintf(stderr, "%s: signal %d received\n", s, signum);
    }
  }
  if (! SawBadInst()) {
    int hasfeature = thetest();
    if (hasfeature && (! SawBadInst())) {
      printf("[Has %s]\n", s);
      return 0;
    }
  }
  printf("no %s\n", s);
  return 1;
}
#elif NACL_WINDOWS
/* Windows signal handling code, for trapping illegal instruction faults */
/*
 * DoTest tests for a particular CPU feature using thetest().
 * It returns 0 if the feature is present, 1 if it is not.
 */
static int DoTest(int (*thetest)(void), const char *s) {
  int hasfeature = 0;

  __try {
    hasfeature = thetest();
  } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    printf("Saw exception\n");
    hasfeature = 0;
  }
  if (hasfeature) {
    printf("[Has %s]\n", s);
    return 0;
  } else {
    printf("no %s\n", s);
    return 1;
  }
}
#else
# error Please specify platform as NACL_LINUX, NACL_OSX or NACL_WINDOWS
#endif

static int DoCPUFeatureTest(NaClCPUFeaturesX86 *features,
                            NaClCPUFeatureX86ID id,
                            int (*thetest)(void)) {
  if (NaClGetCPUFeatureX86(features, id))
    return DoTest(thetest, NaClGetCPUFeatureX86Name(id));
  else
    return 0;
}

/*
 * Check if CPU feature detection is self-consistent.
 * Returns 0 on success and non-0 else.
 */
static int CheckCPUFeatureDetection(NaClCPUFeaturesX86 *cpuf) {
  int rcode = 0;
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_x87, asm_HasX87);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_MMX, asm_HasMMX);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_SSE, asm_HasSSE);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_SSE2, asm_HasSSE2);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_3DNOW, asm_Has3DNow);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_SSE3, asm_HasSSE3);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_SSSE3, asm_HasSSSE3);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_SSE41, asm_HasSSE41);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_SSE42, asm_HasSSE42);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_POPCNT, asm_HasPOPCNT);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_CMOV, asm_HasCMOV);
  rcode |= DoCPUFeatureTest(cpuf, NaClCPUFeatureX86_TSC, asm_HasTSC);

#define TEST_NEGATIVE_CASE 0
#if TEST_NEGATIVE_CASE
  printf("TESTING: simulating invalid CPUID implementation\n");
  rcode |= DoTest(asm_HasSSSE3, "SSSE3");
  rcode |= DoTest(asm_HasSSE41, "SSE41");
  rcode |= DoTest(asm_HasSSE42, "SSE42");
  rcode |= DoTest(asm_HasPOPCNT, "POPCNT");
  rcode |= DoTest(asm_HasCMOV, "CMOV");
#endif
  /*
   * TODO(brad): implement the rest of these tests
   * rcode |= DoCPUFeatureTest(&cpuf, NaClCPUFeatureX86_CX8, asm_HasCX8);
   * rcode |= DoCPUFeatureTest(&cpuf, NaClCPUFeatureX86_CX16, asm_HasCX16);
   * DoTest(asm_HasSSE4a, "SSE4a");
   * DoTest(asm_HasEMMX, "EMMX");
   * DoTest(asm_HasE3DNow, "E3DNow");
   * what about LZCNT?
   */
  return rcode;
}
#endif  /* 64-bit Windows */

static void PrintFail(const char *why) {
  fprintf(stderr, "ERROR: %s.\n", why);
  fprintf(stderr, "Google Native Client cannot continue.\n");
}

int CPUIDImplIsValid(void) {
  NaClCPUFeaturesX86 cpuf;
  NaClGetCurrentCPUFeaturesX86((NaClCPUFeatures *) &cpuf);

  if (!NaClGetCPUFeatureX86(&cpuf, NaClCPUFeatureX86_CPUIDSupported)) {
    PrintFail("CPUID not implemented");
    return 0;
  }
  if (!NaClGetCPUFeatureX86(&cpuf, NaClCPUFeatureX86_CPUSupported)) {
    PrintFail("CPU not supported");
    return 0;
  }
  if (CheckCPUFeatureDetection(&cpuf) != 0) {
    PrintFail("CPUID not implemented correctly.");
    return 0;
  }

  printf("[CPUID implementation looks okay]\n");
  return 1;
}
