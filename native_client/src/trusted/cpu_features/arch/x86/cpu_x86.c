/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"

#include "native_client/src/include/build_config.h"
/*
 * cpu_x86.c
 * Retrieve and decode CPU model specific feature mask.
 */
#if NACL_WINDOWS
#include <intrin.h>  /* __cpuid intrinsic */
#endif  /* NACL_WINDOWS  */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_log.h"

/*
 * TODO(bradchen): consolidate to use one debug print mechanism.
 */

#define CPUID_EDX_x87        0x00000001  /* x87 FPU support */
#define CPUID_EDX_VME        0x00000002  /* Virtual 8086 Mode Enhancement */
#define CPUID_EDX_DEB        0x00000004  /* Debugging Extensions */
#define CPUID_EDX_PSE        0x00000008  /* Page Size Extensions */
#define CPUID_EDX_TSC        0x00000010  /* Time Stamp Counter */
#define CPUID_EDX_MSR        0x00000020  /* RDMSR and WRMSR */
#define CPUID_EDX_PAE        0x00000040  /* Physical Address Extensions */
#define CPUID_EDX_MCE        0x00000080  /* Machine Check Exception */
#define CPUID_EDX_CX8        0x00000100  /* CMPXCHG8B Instruction */
#define CPUID_EDX_APIC       0x00000200  /* APIC on chip */
/* 0x00000400 reserved */
#define CPUID_EDX_SEP        0x00000800  /* SYSENTER and SYSEXIT */
#define CPUID_EDX_MTRR       0x00001000  /* Memory Type Range Registers */
#define CPUID_EDX_PGE        0x00002000  /* PTE Global Bit */
#define CPUID_EDX_MCA        0x00004000  /* Machine Check Architecture */
#define CPUID_EDX_CMOV       0x00008000  /* CMOV instruction */
#define CPUID_EDX_PAT        0x00010000  /* Page Attribute Table */
#define CPUID_EDX_PSE36      0x00020000  /* Page Size Extension bis */
#define CPUID_EDX_PSN        0x00040000  /* Processor Serial Number */
#define CPUID_EDX_CLFLUSH    0x00080000  /* CLFLUSH instruction */
/* 0x00100000 reserved */
#define CPUID_EDX_DS         0x00200000  /* Debug Store */
#define CPUID_EDX_ACPI       0x00400000  /* Thermal Monitor and Clock Ctrl */
#define CPUID_EDX_MMX        0x00800000  /* MMX extensions */
#define CPUID_EDX_FXSR       0x01000000  /* FXSAVE/FXRSTOR instructions */
#define CPUID_EDX_SSE        0x02000000  /* SSE extensions */
#define CPUID_EDX_SSE2       0x04000000  /* SSE2 extensions */
#define CPUID_EDX_SS         0x08000000  /* Self snoop */
#define CPUID_EDX_HTT        0x10000000  /* Hyper-threading */
#define CPUID_EDX_TM         0x20000000  /* Thermal monitor */
/* 0x40000000 reserved */
#define CPUID_EDX_PBE        0x80000000  /* Pending Break Enable */

#define CPUID_ECX_SSE3       0x00000001  /* SSE3 extensions */
#define CPUID_ECX_CLMUL      0x00000002  /* PCLMULQDQ instruction */
#define CPUID_ECX_DTES64     0x00000004  /* 64-bit DS Area */
#define CPUID_ECX_MON        0x00000008  /* MONITOR/MWAIT instructions */
#define CPUID_ECX_DSCPL      0x00000010  /* CPL Qualified Debug Store */
#define CPUID_ECX_VMX        0x00000020  /* Virtual Machine Extensions */
#define CPUID_ECX_SMX        0x00000040  /* Safer Mode Extensions */
#define CPUID_ECX_EST        0x00000080  /* Enahcned SpeedStep */
#define CPUID_ECX_TM2        0x00000100  /* Thermal Monitor 2 */
#define CPUID_ECX_SSSE3      0x00000200  /* SS_S_E3 extensions */
#define CPUID_ECX_CXID       0x00000400  /* L1 context ID */
/* 0x00000800 reserved */
#define CPUID_ECX_FMA        0x00001000  /* FMA instructions */
#define CPUID_ECX_CX16       0x00002000  /* CMPXCHG16B instruction */
#define CPUID_ECX_XTPR       0x00004000  /* xTPR update control */
#define CPUID_ECX_PDCM       0x00008000  /* Perf/Debug Capability MSR */
/* 0x00010000 reserved */
#define CPUID_ECX_PCID       0x00020000  /* Process-context identifiers */
#define CPUID_ECX_DCA        0x00040000  /* Direct Cache Access */
#define CPUID_ECX_SSE41      0x00080000  /* SSE4.1 extensions */
#define CPUID_ECX_SSE42      0x00100000  /* SSE4.2 extensions */
#define CPUID_ECX_x2APIC     0x00800000  /* x2APIC feature */
#define CPUID_ECX_MOVBE      0x00400000  /* MOVBE instruction */
#define CPUID_ECX_POPCNT     0x00800000  /* POPCNT instruction */
#define CPUID_ECX_TSCDDLN    0xx1000000  /* TSC-Deadline */
#define CPUID_ECX_AES        0x02000000  /* AES instructions */
#define CPUID_ECX_XSAVE      0x04000000  /* XSAVE/XRSTOR/XSETBV/XGETBV */
#define CPUID_ECX_OSXSAVE    0x08000000  /* XSAVE et al enabled by OS */
#define CPUID_ECX_AVX        0x10000000  /* AVX instructions */
#define CPUID_ECX_F16C       0x20000000  /* 16bit floating-point instructions */
#define CPUID_ECX_RDRAND     0x40000000  /* RDRAND instruction */
/* 0x80000000 reserved */

/* Function 07h main leaf (ecx = 0) */
#define CPUID_EBX_FSGSBASE   0x00000001  /* RD/WR FS/GS BASE instructions */
/* 0x00000002 reserved */
/* 0x00000004 reserved */
#define CPUID_EBX_BMI1       0x00000008  /* BMI1 instructions */
#define CPUID_EBX_HLE        0x00000010  /* HLE instructions */
#define CPUID_EBX_AVX2       0x00000020  /* AVX2 instructions */
#define CPUID_EBX_SMEP       0x00000040  /* Supervisor Mode Exec-Protection */
/* 0x00000080 reserved */
#define CPUID_EBX_BMI2       0x00000100  /* BMI2 instructions */
#define CPUID_EBX_ERMS       0x00000200  /* Enhanced REP MOVSB/STOSB */
#define CPUID_EBX_INVPCID    0x00000400  /* Invalidate Processor Context ID */
#define CPUID_EBX_RTM        0x00000800  /* Restricted Transactional Memory */
/* 0xFFFFF000 reserved */

/* AMD-specific masks - most are the same as in eax == 1 subfunction */
/* 0x00000001 duplicates CPUID_EDX_x87 */
/* 0x00000002 duplicates CPUID_EDX_VME */
/* 0x00000004 duplicates CPUID_EDX_DEB */
/* 0x00000008 duplicates CPUID_EDX_PSE */
/* 0x00000010 duplicates CPUID_EDX_TSC */
/* 0x00000020 duplicates CPUID_EDX_MSR */
/* 0x00000040 duplicates CPUID_EDX_PAE */
/* 0x00000080 duplicates CPUID_EDX_MCE */
/* 0x00000100 duplicates CPUID_EDX_CX8 */
/* 0000000200 duplicates CPUID_EDX_APIC */
/* 0x00000400 reserved */
#define CPUID_EDX_SYSCALL    0x00000800  /* SYSCALL/SYSRET instructions */
/* 0x00001000 duplicates CPUID_EDX_MTRR */
/* 0x00002000 duplicates CPUID_EDX_PGE */
/* 0x00004000 duplicates CPUID_EDX_MCA */
/* 0x00008000 duplicates CPUID_EDX_CMOV */
/* 0x00010000 duplicates CPUID_EDX_PAT */
/* 0x00020000 duplicates CPUID_EDX_PSE36 */
/* 0x00040000 reserved */
/* 0x00080000 reserved */
#define CPUID_EDX_NX         0x00100000  /* Execute Disable Bit available */
/* 0x00200000 reserved */
#define CPUID_EDX_EMMX       0x00400000  /* Extensions to MMX instructions */
/* 0x00800000 duplicates CPUID_EDX_MMX */
/* 0x01000000 duplicates CPUID_EDX_FXSR */
#define CPUID_EDX_FFFXSR     0x02000000  /* FXSAVE/FXRSTOR optimizations */
#define CPUID_EDX_1GBPAGES   0x04000000  /* 1-GB large page support */
#define CPUID_EDX_TSCP       0x08000000  /* RDTSCP instruction */
/* 0x10000000 reserved */
#define CPUID_EDX_LM         0x20000000  /* Longmode (AKA x86-64 mode) */
#define CPUID_EDX_E3DN       0x40000000  /* Extensions to 3DNow! instructions */
#define CPUID_EDX_3DN        0x80000000  /* 3DNow! instructions */

#define CPUID_ECX_LAHF       0x00000001  /* LAHF/SAHF in x86-64 mode */
#define CPUID_ECX_CMPLEGACY  0x00000002  /* Core Multi-Processing legacy mode */
#define CPUID_ECX_SVM        0x00000004  /* Secure Virtual Machine */
#define CPUID_ECX_EXTAPIC    0x00000008  /* Extended APIC space */
#define CPUID_ECX_ALTMOVCR8  0x00000010  /* LOCK MOV CR0 means MOV CR8 */
#define CPUID_ECX_ABM        0x00000020  /* LZCNT instruction */
#define CPUID_ECX_SSE4A      0x00000040  /* SSRE4A instructions */
#define CPUID_ECX_MISALGNSSE 0x00000080  /* Misalign SSE mode */
#define CPUID_ECX_PRE        0x00000100  /* 3DNow! prefetch */
#define CPUID_ECX_OSVW       0x00000200  /* OS visible workaround */
#define CPUID_ECX_IBS        0x00000400  /* Instruction Based Sampling */
#define CPUID_ECX_XOP        0x00000800  /* XOP instructions */
#define CPUID_ECX_SKINIT     0x00001000  /* SKINIT/STGI are always supported */
#define CPUID_ECX_WDT        0x00002000  /* Watchdog timer support */
/* 0x00004000 reserved */
#define CPUID_ECX_LWP        0x00008000  /* Lightweight profiling support */
#define CPUID_ECX_FMA4       0x00010000  /* FMA4 instructions */
/* 0x00020000 reserved */
/* 0x00040000 reserved */
#define CPUID_ECX_NODEID     0x00080000  /* MSRC001_100C[NodeId, NodesPerCPU] */
/* 0x00100000 reserved */
#define CPUID_ECX_TBM        0x00200000  /* Trailing bit manipulations */
#define CPUID_ECX_TOPOLOGY   0x00400000  /* Topology extensions support */
/* 0xFF800000 reserved */
#define CPUID_NONE           0x00000000 /* bogus value */

typedef enum {
  CFReg_EAX_I=0,  /* eax == 1 */
  CFReg_EBX_I,    /* eax == 1 */
  CFReg_ECX_I,    /* eax == 1 */
  CFReg_EDX_I,    /* eax == 1 */
  CFReg_EAX_7,    /* eax == 7; ecx == 0 */
  CFReg_EBX_7,    /* eax == 7; ecx == 0 */
  CFReg_ECX_7,    /* eax == 7; ecx == 0 */
  CFReg_EDX_7,    /* eax == 7; ecx == 0 */
  CFReg_EAX_A,    /* eax == 0x80000001 */
  CFReg_EBX_A,    /* eax == 0x80000001 */
  CFReg_ECX_A,    /* eax == 0x80000001 */
  CFReg_EDX_A,    /* eax == 0x80000001 */
  CFReg_NONE      /* bogus value */
} CPUFeatureReg;

typedef struct cpufeature {
  uint8_t reg;
  uint32_t mask;
  const char *name;
} CPUFeature;

static const CPUFeature CPUFeatureDescriptions[(int)NaClCPUFeatureX86_Max] = {
#define NACL_X86_CPU_FEATURE(id, reg, idx, ven, str)            \
  {                                                             \
    NACL_CONCAT(CFReg_, reg),                                   \
    NACL_CONCAT(CPUID_, idx),                                   \
    str,                                                        \
  },
/* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86_features.h"
#undef NACL_X86_CPU_FEATURE
};

#define /* static const int */ kVendorIDLength 13
static const char Intel_CPUID0[kVendorIDLength]   = "GenuineIntel";
static const char AMD_CPUID0[kVendorIDLength]     = "AuthenticAMD";
#ifdef NOTYET
static const char UMC_CPUID0[kVendorIDLength]     = "UMC UMC UMC ";
static const char Cyrix_CPUID0[kVendorIDLength]   = "CyrixInstead";
static const char NexGen_CPUID0[kVendorIDLength]  = "NexGenDriven";
static const char Cantaur_CPUID0[kVendorIDLength] = "CentaurHauls";
static const char Rise_CPUID0[kVendorIDLength]    = "RiseRiseRise";
static const char SiS_CPUID0[kVendorIDLength]     = "SiS SiS SiS ";
static const char TM_CPUID0[kVendorIDLength]      = "GenuineTMx86";
static const char NSC_CPUID0[kVendorIDLength]     = "Geode by NSC";
#endif

static int asm_HasCPUID(void) {
#if NACL_BUILD_SUBARCH == 64
  /* Note: If we are running in x86-64, then cpuid must be defined,
   * since CPUID dates from DX2 486, and x86-64 was added after this.
   */
  return 1;
#else
  volatile int before, after, result;
/* TODO(bradchen): split into separate Windows, etc., files */
# if defined(__GNUC__)
  __asm__ volatile("pushfl                \n\t" /* save EFLAGS to eax */
                   "pop %%eax             \n\t"
                   "movl %%eax, %0        \n\t" /* remember EFLAGS in %0 */
                   "xor $0x00200000, %%eax\n\t" /* toggle bit 21 */
                   "push %%eax            \n\t" /* write eax to EFLAGS */
                   "popfl                 \n\t"
                   "pushfl                \n\t" /* save EFLAGS to %1 */
                   "pop %1                \n\t"
                   /*
                    * We use "r" constraints here, forcing registers,
                    * because a memory reference using the stack
                    * pointer wouldn't be safe since we're moving the
                    * stack pointer around in between the
                    * instructions.  We need to inform the compiler
                    * that we're clobbering %eax as a scratch register.
                    */
                   : "=r" (before), "=r" (after) : : "eax");
# elif NACL_WINDOWS
  __asm {
    pushfd
    pop eax
    mov before, eax
    xor eax, 0x00200000
    push eax
    popfd
    pushfd
    pop after
  }
# else
#  error Unsupported platform
# endif
  result = (before ^ after) & 0x0200000;
  return result;
#endif
}

static void asm_CPUID(uint32_t op, volatile uint32_t reg[4]) {
#if defined(__GNUC__)
#if NACL_BUILD_SUBARCH == 64
 __asm__ volatile("push %%rbx       \n\t" /* save %ebx */
#else
 __asm__ volatile("pushl %%ebx      \n\t"
#endif
                   "cpuid            \n\t"
                   "movl %%ebx, %1   \n\t"
                   /* save what cpuid just put in %ebx */
#if NACL_BUILD_SUBARCH == 64
                   "pop %%rbx        \n\t"
#else
                   "popl %%ebx       \n\t" /* restore the old %ebx */
#endif
                   : "=a"(reg[0]), "=S"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                   : "a"(op)
                   : "cc");
#elif NACL_WINDOWS
  __cpuid((int*)reg, op);
#else
# error Unsupported platform
#endif
}

/*
 * Historically CPUID only used eax to select "CPUID function". Function 07h
 * broke this tradition: it's now required to specify "leaf" in ECX register.
 *
 * We can specify leaf in all cases (older "CPUID functions" will just ignore
 * it), but there is a catch: MSVC 2005 or below don't include __cpuidex
 * intrinsic required to call CPUID with leaf support!
 *
 * Thus we have two functions: asm_CPUID (for "CPUID functions" without leaves)
 * and asm_CPUIDx (for "CPUID functions" with leaves). If code is compiled using
 * MSVC 2005 or MSVC 2008 then features detected using function 07h will not be
 * available.
 *
 * Note: MSVC 2008 is particularly problematic: MSVC 2008 does not support
 * __cpuidex while MSVC 2008 SP1 does. Unfortunatelly there are no easy way
 * to distinguish MSVC 2008 SP1 from MSVC 2008 using ifdef's thus we disable
 * __cpuidex for MSVC 2008 unconditionally.
 */
static void asm_CPUIDx(uint32_t op, volatile uint32_t reg[4], uint32_t ecx) {
#if defined(__GNUC__)
#if NACL_BUILD_SUBARCH == 64
 __asm__ volatile("push %%rbx       \n\t" /* save %ebx */
#else
 __asm__ volatile("pushl %%ebx      \n\t"
#endif
                   "cpuid            \n\t"
                   "movl %%ebx, %1   \n\t"
                   /* save what cpuid just put in %ebx */
#if NACL_BUILD_SUBARCH == 64
                   "pop %%rbx        \n\t"
#else
                   "popl %%ebx       \n\t" /* restore the old %ebx */
#endif
                   : "=a"(reg[0]), "=S"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                   : "a"(op), "c"(ecx)
                   : "cc");
#elif NACL_WINDOWS
#ifdef _MSC_VER
#if _MSC_VER < 1600
  reg[0] = 0;
  reg[1] = 0;
  reg[2] = 0;
  reg[3] = 0;
#else
  __cpuidex((int*)reg, op, ecx);
#endif
#else /* NACL_WINDOWS, but _MSC_VER is not defined */
/* This is Windows but not MSVC: who knows if __cpuidex is available?  */
# error Unsupported compiler
#endif
#else
# error Unsupported platform
#endif
}

static void CacheCPUVersionID(NaClCPUData* data) {
  uint32_t reg[4] = {0, 0, 0, 0 };
  asm_CPUID(0, reg);
  data->_vidwords[0] = reg[1];
  data->_vidwords[1] = reg[3];
  data->_vidwords[2] = reg[2];
  data->_vidwords[3] = 0;
}

/* Defines the (cached) cpu version id */
#define CPUVersionID(data) ((char*) (data)->_vidwords)


/* Cache feature vector as array of uint32_t [ecx, edx] */
static void CacheCPUFeatureVector(NaClCPUData* data) {
  int i;
  for (i = 0; i < kMaxCPUFeatureReg; ++i) {
    data->_featurev[i] = 0;
  }
  asm_CPUID(1, data->_featurev);
  /* This is for for model-specific instructions from AMD and Intel after
     AMD's Bulldozer (introduced BMI1 set) and Intel's Haswell (introduced
     AVX2, BMI2, HLE, RTM, and RD/WR FS/GS BASE instructions).  */
  asm_CPUIDx(7, &data->_featurev[CFReg_EAX_7], 0);
  /* this is for AMD CPUs */
  asm_CPUID(0x80000001, &data->_featurev[CFReg_EAX_A]);
#if 0
  /* print feature vector */
  printf("CPUID:  %08x  %08x  %08x  %08x\n",
         data->_featurev[0],
         data->_featurev[1],
         data->_featurev[2],
         data->_featurev[3]);
  printf("CPUID:  %08x  %08x  %08x  %08x\n",
         data->_featurev[4],
         data->_featurev[5],
         data->_featurev[6],
         data->_featurev[7]);
#endif
}

/* CacheGetCPUIDString creates an ASCII string that identfies this CPU's */
/* vendor ID, family, model, and stepping, as per the CPUID instruction */
static void CacheGetCPUIDString(NaClCPUData* data) {
  char *cpuversionid = CPUVersionID(data);
  uint32_t *fv = data->_featurev;
  char* wlid = data->_wlid;
  /* Subtract 1 in this assert to avoid counting two null characters. */
  assert(9 + kVendorIDLength - 1 == kCPUIDStringLength);
  memcpy(wlid, cpuversionid, kVendorIDLength-1);
  SNPRINTF(&(wlid[kVendorIDLength-1]), 9, "%08x", (int)fv[CFReg_EAX_I]);
}

char *GetCPUIDString(NaClCPUData* data) {
  return data->_wlid;
}

/* Returns true if the given feature is defined by the CPUID. */
static int CheckCPUFeature(NaClCPUData* data, NaClCPUFeatureX86ID fid) {
  const CPUFeature *f = &CPUFeatureDescriptions[fid];
  uint32_t *fv = data->_featurev;
  if ((fid == NaClCPUFeatureX86_CPUIDSupported) ||
      (fid == NaClCPUFeatureX86_CPUSupported)) {
    /* CPUIDSupported and CPUSupported aren't actually in CPUID,
       CPUFeatureDescriptions therefore doesn't contain actual reg/mask for
       them. */
    return 1;
  }
  if (fv[f->reg] & f->mask) {
    return 1;
  } else {
    return 0;
  }
}

uint64_t NaClXGETBV(uint32_t);

/* Cache XCR vector */
static void CacheCPUXCRVector(NaClCPUData* data) {
  if (CheckCPUFeature(data, NaClCPUFeatureX86_OSXSAVE)) {
    int i;
    for (i = 0; i < kMaxCPUXCRReg; ++i) {
      data->_xcrv[i] = NaClXGETBV(i);
    }
  } else {
    int i;
    for (i = 0; i < kMaxCPUXCRReg; ++i) {
      data->_xcrv[i] = 0;
    }
  }
}

/* Check that we have a supported 386 architecture. NOTE:
 * As as side effect, the given cpu features is cleared before
 * setting the appropriate fields.
 */
static void CheckNaClArchFeatures(NaClCPUData *data,
                                  NaClCPUFeaturesX86 *features) {
  const size_t kCPUID0Length = 12;
  char *cpuversionid;
  if (data->_has_CPUID) {
    NaClSetCPUFeatureX86(features, NaClCPUFeatureX86_CPUIDSupported, 1);
  }
  cpuversionid = CPUVersionID(data);
  if (strncmp(cpuversionid, Intel_CPUID0, kCPUID0Length) == 0) {
    NaClSetCPUFeatureX86(features, NaClCPUFeatureX86_CPUSupported, 1);
  } else if (strncmp(cpuversionid, AMD_CPUID0, kCPUID0Length) == 0) {
    NaClSetCPUFeatureX86(features, NaClCPUFeatureX86_CPUSupported, 1);
  }
}

void NaClSetAllCPUFeaturesX86(NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast in this interface. */
  NaClCPUFeaturesX86 *features = (NaClCPUFeaturesX86 *) f;
  /* Be a little more pedantic than using memset because we don't know exactly
   * how the structure is laid out.  If we use memset, fields may be initialized
   * to 0xff instead of 1 ... this isn't the end of the world but it can
   * create a skew if the structure is hashed, etc.
   */
  int id;
  /* Ensure any padding is zeroed. */
  NaClClearCPUFeaturesX86(features);
  for (id = 0; id < NaClCPUFeatureX86_Max; ++id) {
    NaClSetCPUFeatureX86(features, id, 1);
  }
}

/* WARNING: This routine and subroutines it uses are not threadsafe.
 * However, if races occur, they are short lived, and at worst, will
 * result in defining fewer features than are actually supported by
 * the hardware. Hence, if a race occurs, the validator may reject
 * some features that should not be rejected.
 */
static void GetCPUFeatures(NaClCPUData* data, NaClCPUFeaturesX86 *cpuf) {
  int id;
  NaClClearCPUFeaturesX86(cpuf);
  CheckNaClArchFeatures(data, cpuf);
  if (!NaClGetCPUFeatureX86(cpuf, NaClCPUFeatureX86_CPUIDSupported)) {
    return;
  }

  for (id = 0; id < NaClCPUFeatureX86_Max; ++id) {
    NaClSetCPUFeatureX86(cpuf, id, CheckCPUFeature(data, id));
  }

  /*
   * If the operating system doesn't maintain the YMM state,
   * pretend we don't have the instructions available at all.
   */
  if (!(NaClGetCPUFeatureX86(cpuf, NaClCPUFeatureX86_OSXSAVE)
        && ((data->_xcrv[0] & 6) == 6))) {
    NaClSetCPUFeatureX86(cpuf, NaClCPUFeatureX86_AVX, 0);
    NaClSetCPUFeatureX86(cpuf, NaClCPUFeatureX86_AVX2, 0);
    NaClSetCPUFeatureX86(cpuf, NaClCPUFeatureX86_F16C, 0);
    NaClSetCPUFeatureX86(cpuf, NaClCPUFeatureX86_FMA, 0);
    NaClSetCPUFeatureX86(cpuf, NaClCPUFeatureX86_FMA4, 0);
  }
}

void NaClCPUDataGet(NaClCPUData* data) {
  data->_has_CPUID = asm_HasCPUID();
  CacheCPUVersionID(data);
  CacheCPUFeatureVector(data);
  CacheCPUXCRVector(data);
  CacheGetCPUIDString(data);
}

void NaClGetCurrentCPUFeaturesX86(NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast in this interface. */
  NaClCPUFeaturesX86 *features = (NaClCPUFeaturesX86 *) f;
  NaClCPUData cpu_data;
  NaClCPUDataGet(&cpu_data);
  GetCPUFeatures(&cpu_data, features);
}

const char* NaClGetCPUFeatureX86Name(NaClCPUFeatureX86ID id) {
  return CPUFeatureDescriptions[id].name;
}

void NaClSetCPUFeatureX86(NaClCPUFeaturesX86 *f, NaClCPUFeatureX86ID id,
                          int state) {
  f->data[id] = (char) state;
}

void NaClClearCPUFeaturesX86(NaClCPUFeaturesX86 *features) {
  memset(features, 0, sizeof(*features));
}

void NaClCopyCPUFeaturesX86(NaClCPUFeaturesX86 *target,
                            const NaClCPUFeaturesX86 *source) {
  memcpy(target, source, sizeof(*target));
}

int NaClArchSupportedX86(const NaClCPUFeaturesX86 *f) {
  return (NaClGetCPUFeatureX86(f, NaClCPUFeatureX86_CPUIDSupported) &&
          NaClGetCPUFeatureX86(f, NaClCPUFeatureX86_CPUSupported));
}
