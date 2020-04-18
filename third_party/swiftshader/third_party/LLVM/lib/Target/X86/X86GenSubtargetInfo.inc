//===- TableGen'erated file -------------------------------------*- C++ -*-===//
//
// Subtarget Enumeration Source Fragment
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//


#ifdef GET_SUBTARGETINFO_ENUM
#undef GET_SUBTARGETINFO_ENUM
namespace llvm {
namespace X86 {
enum {
  Feature3DNow =  1ULL << 0,
  Feature3DNowA =  1ULL << 1,
  Feature64Bit =  1ULL << 2,
  FeatureAES =  1ULL << 3,
  FeatureAVX =  1ULL << 4,
  FeatureBMI =  1ULL << 5,
  FeatureCLMUL =  1ULL << 6,
  FeatureCMOV =  1ULL << 7,
  FeatureCMPXCHG16B =  1ULL << 8,
  FeatureF16C =  1ULL << 9,
  FeatureFMA3 =  1ULL << 10,
  FeatureFMA4 =  1ULL << 11,
  FeatureFastUAMem =  1ULL << 12,
  FeatureLZCNT =  1ULL << 13,
  FeatureMMX =  1ULL << 14,
  FeatureMOVBE =  1ULL << 15,
  FeaturePOPCNT =  1ULL << 16,
  FeatureRDRAND =  1ULL << 17,
  FeatureSSE1 =  1ULL << 18,
  FeatureSSE2 =  1ULL << 19,
  FeatureSSE3 =  1ULL << 20,
  FeatureSSE4A =  1ULL << 21,
  FeatureSSE41 =  1ULL << 22,
  FeatureSSE42 =  1ULL << 23,
  FeatureSSSE3 =  1ULL << 24,
  FeatureSlowBTMem =  1ULL << 25,
  FeatureVectorUAMem =  1ULL << 26,
  Mode64Bit =  1ULL << 27,
  ModeNaCl =  1ULL << 28
};
}
} // End llvm namespace 
#endif // GET_SUBTARGETINFO_ENUM


#ifdef GET_SUBTARGETINFO_MC_DESC
#undef GET_SUBTARGETINFO_MC_DESC
namespace llvm {
// Sorted (by key) array of values for CPU features.
llvm::SubtargetFeatureKV X86FeatureKV[] = {
  { "3dnow", "Enable 3DNow! instructions", X86::Feature3DNow, X86::FeatureMMX },
  { "3dnowa", "Enable 3DNow! Athlon instructions", X86::Feature3DNowA, X86::Feature3DNow },
  { "64bit", "Support 64-bit instructions", X86::Feature64Bit, X86::FeatureCMOV },
  { "64bit-mode", "64-bit mode (x86_64)", X86::Mode64Bit, 0ULL },
  { "aes", "Enable AES instructions", X86::FeatureAES, 0ULL },
  { "avx", "Enable AVX instructions", X86::FeatureAVX, 0ULL },
  { "bmi", "Support BMI instructions", X86::FeatureBMI, 0ULL },
  { "clmul", "Enable carry-less multiplication instructions", X86::FeatureCLMUL, 0ULL },
  { "cmov", "Enable conditional move instructions", X86::FeatureCMOV, 0ULL },
  { "cmpxchg16b", "64-bit with cmpxchg16b", X86::FeatureCMPXCHG16B, X86::Feature64Bit },
  { "f16c", "Support 16-bit floating point conversion instructions", X86::FeatureF16C, 0ULL },
  { "fast-unaligned-mem", "Fast unaligned memory access", X86::FeatureFastUAMem, 0ULL },
  { "fma3", "Enable three-operand fused multiple-add", X86::FeatureFMA3, 0ULL },
  { "fma4", "Enable four-operand fused multiple-add", X86::FeatureFMA4, 0ULL },
  { "lzcnt", "Support LZCNT instruction", X86::FeatureLZCNT, 0ULL },
  { "mmx", "Enable MMX instructions", X86::FeatureMMX, 0ULL },
  { "movbe", "Support MOVBE instruction", X86::FeatureMOVBE, 0ULL },
  { "nacl-mode", "Native Client mode", X86::ModeNaCl, 0ULL },
  { "popcnt", "Support POPCNT instruction", X86::FeaturePOPCNT, 0ULL },
  { "rdrand", "Support RDRAND instruction", X86::FeatureRDRAND, 0ULL },
  { "slow-bt-mem", "Bit testing of memory is slow", X86::FeatureSlowBTMem, 0ULL },
  { "sse", "Enable SSE instructions", X86::FeatureSSE1, X86::FeatureMMX | X86::FeatureCMOV },
  { "sse2", "Enable SSE2 instructions", X86::FeatureSSE2, X86::FeatureSSE1 },
  { "sse3", "Enable SSE3 instructions", X86::FeatureSSE3, X86::FeatureSSE2 },
  { "sse41", "Enable SSE 4.1 instructions", X86::FeatureSSE41, X86::FeatureSSSE3 },
  { "sse42", "Enable SSE 4.2 instructions", X86::FeatureSSE42, X86::FeatureSSE41 | X86::FeaturePOPCNT },
  { "sse4a", "Support SSE 4a instructions", X86::FeatureSSE4A, X86::FeaturePOPCNT },
  { "ssse3", "Enable SSSE3 instructions", X86::FeatureSSSE3, X86::FeatureSSE3 },
  { "vector-unaligned-mem", "Allow unaligned memory operands on vector/SIMD instructions", X86::FeatureVectorUAMem, 0ULL }
};

// Sorted (by key) array of values for CPU subtype.
llvm::SubtargetFeatureKV X86SubTypeKV[] = {
  { "amdfam10", "Select the amdfam10 processor", X86::FeatureSSE3 | X86::FeatureSSE4A | X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "athlon", "Select the athlon processor", X86::Feature3DNowA | X86::FeatureSlowBTMem, 0ULL },
  { "athlon-4", "Select the athlon-4 processor", X86::FeatureSSE1 | X86::Feature3DNowA | X86::FeatureSlowBTMem, 0ULL },
  { "athlon-fx", "Select the athlon-fx processor", X86::FeatureSSE2 | X86::Feature3DNowA | X86::Feature64Bit | X86::FeatureSlowBTMem, 0ULL },
  { "athlon-mp", "Select the athlon-mp processor", X86::FeatureSSE1 | X86::Feature3DNowA | X86::FeatureSlowBTMem, 0ULL },
  { "athlon-tbird", "Select the athlon-tbird processor", X86::Feature3DNowA | X86::FeatureSlowBTMem, 0ULL },
  { "athlon-xp", "Select the athlon-xp processor", X86::FeatureSSE1 | X86::Feature3DNowA | X86::FeatureSlowBTMem, 0ULL },
  { "athlon64", "Select the athlon64 processor", X86::FeatureSSE2 | X86::Feature3DNowA | X86::Feature64Bit | X86::FeatureSlowBTMem, 0ULL },
  { "athlon64-sse3", "Select the athlon64-sse3 processor", X86::FeatureSSE3 | X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "atom", "Select the atom processor", X86::FeatureSSE3 | X86::FeatureCMPXCHG16B | X86::FeatureMOVBE | X86::FeatureSlowBTMem, 0ULL },
  { "barcelona", "Select the barcelona processor", X86::FeatureSSE3 | X86::FeatureSSE4A | X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "c3", "Select the c3 processor", X86::Feature3DNow, 0ULL },
  { "c3-2", "Select the c3-2 processor", X86::FeatureSSE1, 0ULL },
  { "core-avx-i", "Select the core-avx-i processor", X86::FeatureSSE42 | X86::FeatureCMPXCHG16B | X86::FeatureAES | X86::FeatureCLMUL | X86::FeatureRDRAND | X86::FeatureF16C, 0ULL },
  { "core-avx2", "Select the core-avx2 processor", X86::FeatureSSE42 | X86::FeatureCMPXCHG16B | X86::FeatureAES | X86::FeatureCLMUL | X86::FeatureRDRAND | X86::FeatureF16C | X86::FeatureFMA3 | X86::FeatureMOVBE | X86::FeatureLZCNT | X86::FeatureBMI, 0ULL },
  { "core2", "Select the core2 processor", X86::FeatureSSSE3 | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "corei7", "Select the corei7 processor", X86::FeatureSSE42 | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem | X86::FeatureFastUAMem | X86::FeatureAES, 0ULL },
  { "corei7-avx", "Select the corei7-avx processor", X86::FeatureSSE42 | X86::FeatureCMPXCHG16B | X86::FeatureAES | X86::FeatureCLMUL, 0ULL },
  { "generic", "Select the generic processor", 0ULL, 0ULL },
  { "i386", "Select the i386 processor", 0ULL, 0ULL },
  { "i486", "Select the i486 processor", 0ULL, 0ULL },
  { "i586", "Select the i586 processor", 0ULL, 0ULL },
  { "i686", "Select the i686 processor", 0ULL, 0ULL },
  { "istanbul", "Select the istanbul processor", X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSSE4A | X86::Feature3DNowA, 0ULL },
  { "k6", "Select the k6 processor", X86::FeatureMMX, 0ULL },
  { "k6-2", "Select the k6-2 processor", X86::Feature3DNow, 0ULL },
  { "k6-3", "Select the k6-3 processor", X86::Feature3DNow, 0ULL },
  { "k8", "Select the k8 processor", X86::FeatureSSE2 | X86::Feature3DNowA | X86::Feature64Bit | X86::FeatureSlowBTMem, 0ULL },
  { "k8-sse3", "Select the k8-sse3 processor", X86::FeatureSSE3 | X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "nehalem", "Select the nehalem processor", X86::FeatureSSE42 | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem | X86::FeatureFastUAMem, 0ULL },
  { "nocona", "Select the nocona processor", X86::FeatureSSE3 | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "opteron", "Select the opteron processor", X86::FeatureSSE2 | X86::Feature3DNowA | X86::Feature64Bit | X86::FeatureSlowBTMem, 0ULL },
  { "opteron-sse3", "Select the opteron-sse3 processor", X86::FeatureSSE3 | X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "penryn", "Select the penryn processor", X86::FeatureSSE41 | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem, 0ULL },
  { "pentium", "Select the pentium processor", 0ULL, 0ULL },
  { "pentium-m", "Select the pentium-m processor", X86::FeatureSSE2 | X86::FeatureSlowBTMem, 0ULL },
  { "pentium-mmx", "Select the pentium-mmx processor", X86::FeatureMMX, 0ULL },
  { "pentium2", "Select the pentium2 processor", X86::FeatureMMX | X86::FeatureCMOV, 0ULL },
  { "pentium3", "Select the pentium3 processor", X86::FeatureSSE1, 0ULL },
  { "pentium3m", "Select the pentium3m processor", X86::FeatureSSE1 | X86::FeatureSlowBTMem, 0ULL },
  { "pentium4", "Select the pentium4 processor", X86::FeatureSSE2, 0ULL },
  { "pentium4m", "Select the pentium4m processor", X86::FeatureSSE2 | X86::FeatureSlowBTMem, 0ULL },
  { "pentiumpro", "Select the pentiumpro processor", X86::FeatureCMOV, 0ULL },
  { "prescott", "Select the prescott processor", X86::FeatureSSE3 | X86::FeatureSlowBTMem, 0ULL },
  { "shanghai", "Select the shanghai processor", X86::Feature3DNowA | X86::FeatureCMPXCHG16B | X86::FeatureSSE4A | X86::Feature3DNowA, 0ULL },
  { "westmere", "Select the westmere processor", X86::FeatureSSE42 | X86::FeatureCMPXCHG16B | X86::FeatureSlowBTMem | X86::FeatureFastUAMem | X86::FeatureAES | X86::FeatureCLMUL, 0ULL },
  { "winchip-c6", "Select the winchip-c6 processor", X86::FeatureMMX, 0ULL },
  { "winchip2", "Select the winchip2 processor", X86::Feature3DNow, 0ULL },
  { "x86-64", "Select the x86-64 processor", X86::FeatureSSE2 | X86::Feature64Bit | X86::FeatureSlowBTMem, 0ULL },
  { "yonah", "Select the yonah processor", X86::FeatureSSE3 | X86::FeatureSlowBTMem, 0ULL }
};


static inline void InitX86MCSubtargetInfo(MCSubtargetInfo *II, StringRef TT, StringRef CPU, StringRef FS) {
  II->InitMCSubtargetInfo(TT, CPU, FS, X86FeatureKV, X86SubTypeKV, 0, 0, 0, 0, 29, 50);
}

} // End llvm namespace 
#endif // GET_SUBTARGETINFO_MC_DESC


#ifdef GET_SUBTARGETINFO_TARGET_DESC
#undef GET_SUBTARGETINFO_TARGET_DESC
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
// ParseSubtargetFeatures - Parses features string setting specified
// subtarget options.
void llvm::X86Subtarget::ParseSubtargetFeatures(StringRef CPU, StringRef FS) {
  DEBUG(dbgs() << "\nFeatures:" << FS);
  DEBUG(dbgs() << "\nCPU:" << CPU);
  uint64_t Bits = ReInitMCSubtargetInfo(CPU, FS);
  if ((Bits & X86::Feature3DNow) != 0 && X863DNowLevel < ThreeDNow) X863DNowLevel = ThreeDNow;
  if ((Bits & X86::Feature3DNowA) != 0 && X863DNowLevel < ThreeDNowA) X863DNowLevel = ThreeDNowA;
  if ((Bits & X86::Feature64Bit) != 0) HasX86_64 = true;
  if ((Bits & X86::FeatureAES) != 0) HasAES = true;
  if ((Bits & X86::FeatureAVX) != 0) HasAVX = true;
  if ((Bits & X86::FeatureBMI) != 0) HasBMI = true;
  if ((Bits & X86::FeatureCLMUL) != 0) HasCLMUL = true;
  if ((Bits & X86::FeatureCMOV) != 0) HasCMov = true;
  if ((Bits & X86::FeatureCMPXCHG16B) != 0) HasCmpxchg16b = true;
  if ((Bits & X86::FeatureF16C) != 0) HasF16C = true;
  if ((Bits & X86::FeatureFMA3) != 0) HasFMA3 = true;
  if ((Bits & X86::FeatureFMA4) != 0) HasFMA4 = true;
  if ((Bits & X86::FeatureFastUAMem) != 0) IsUAMemFast = true;
  if ((Bits & X86::FeatureLZCNT) != 0) HasLZCNT = true;
  if ((Bits & X86::FeatureMMX) != 0 && X86SSELevel < MMX) X86SSELevel = MMX;
  if ((Bits & X86::FeatureMOVBE) != 0) HasMOVBE = true;
  if ((Bits & X86::FeaturePOPCNT) != 0) HasPOPCNT = true;
  if ((Bits & X86::FeatureRDRAND) != 0) HasRDRAND = true;
  if ((Bits & X86::FeatureSSE1) != 0 && X86SSELevel < SSE1) X86SSELevel = SSE1;
  if ((Bits & X86::FeatureSSE2) != 0 && X86SSELevel < SSE2) X86SSELevel = SSE2;
  if ((Bits & X86::FeatureSSE3) != 0 && X86SSELevel < SSE3) X86SSELevel = SSE3;
  if ((Bits & X86::FeatureSSE4A) != 0) HasSSE4A = true;
  if ((Bits & X86::FeatureSSE41) != 0 && X86SSELevel < SSE41) X86SSELevel = SSE41;
  if ((Bits & X86::FeatureSSE42) != 0 && X86SSELevel < SSE42) X86SSELevel = SSE42;
  if ((Bits & X86::FeatureSSSE3) != 0 && X86SSELevel < SSSE3) X86SSELevel = SSSE3;
  if ((Bits & X86::FeatureSlowBTMem) != 0) IsBTMemSlow = true;
  if ((Bits & X86::FeatureVectorUAMem) != 0) HasVectorUAMem = true;
  if ((Bits & X86::Mode64Bit) != 0) In64BitMode = true;
  if ((Bits & X86::ModeNaCl) != 0) InNaClMode = true;
}
#endif // GET_SUBTARGETINFO_TARGET_DESC


#ifdef GET_SUBTARGETINFO_HEADER
#undef GET_SUBTARGETINFO_HEADER
namespace llvm {
struct X86GenSubtargetInfo : public TargetSubtargetInfo {
  explicit X86GenSubtargetInfo(StringRef TT, StringRef CPU, StringRef FS);
};
} // End llvm namespace 
#endif // GET_SUBTARGETINFO_HEADER


#ifdef GET_SUBTARGETINFO_CTOR
#undef GET_SUBTARGETINFO_CTOR
namespace llvm {
extern llvm::SubtargetFeatureKV X86FeatureKV[];
extern llvm::SubtargetFeatureKV X86SubTypeKV[];
X86GenSubtargetInfo::X86GenSubtargetInfo(StringRef TT, StringRef CPU, StringRef FS)
  : TargetSubtargetInfo() {
  InitMCSubtargetInfo(TT, CPU, FS, X86FeatureKV, X86SubTypeKV, 0, 0, 0, 0, 29, 50);
}

} // End llvm namespace 
#endif // GET_SUBTARGETINFO_CTOR

