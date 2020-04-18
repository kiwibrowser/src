//===- TableGen'erated file -------------------------------------*- C++ -*-===//
//
// Target Register Enum Values
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//


#ifdef GET_REGINFO_ENUM
#undef GET_REGINFO_ENUM
namespace llvm {

class MCRegisterClass;
extern MCRegisterClass X86MCRegisterClasses[];

namespace X86 {
enum {
  NoRegister,
  AH = 1,
  AL = 2,
  AX = 3,
  BH = 4,
  BL = 5,
  BP = 6,
  BPL = 7,
  BX = 8,
  CH = 9,
  CL = 10,
  CR0 = 11,
  CR1 = 12,
  CR2 = 13,
  CR3 = 14,
  CR4 = 15,
  CR5 = 16,
  CR6 = 17,
  CR7 = 18,
  CR8 = 19,
  CR9 = 20,
  CR10 = 21,
  CR11 = 22,
  CR12 = 23,
  CR13 = 24,
  CR14 = 25,
  CR15 = 26,
  CS = 27,
  CX = 28,
  DH = 29,
  DI = 30,
  DIL = 31,
  DL = 32,
  DR0 = 33,
  DR1 = 34,
  DR2 = 35,
  DR3 = 36,
  DR4 = 37,
  DR5 = 38,
  DR6 = 39,
  DR7 = 40,
  DS = 41,
  DX = 42,
  EAX = 43,
  EBP = 44,
  EBX = 45,
  ECX = 46,
  EDI = 47,
  EDX = 48,
  EFLAGS = 49,
  EIP = 50,
  EIZ = 51,
  ES = 52,
  ESI = 53,
  ESP = 54,
  FP0 = 55,
  FP1 = 56,
  FP2 = 57,
  FP3 = 58,
  FP4 = 59,
  FP5 = 60,
  FP6 = 61,
  FS = 62,
  GS = 63,
  IP = 64,
  MM0 = 65,
  MM1 = 66,
  MM2 = 67,
  MM3 = 68,
  MM4 = 69,
  MM5 = 70,
  MM6 = 71,
  MM7 = 72,
  R8 = 73,
  R8B = 74,
  R8D = 75,
  R8W = 76,
  R9 = 77,
  R9B = 78,
  R9D = 79,
  R9W = 80,
  R10 = 81,
  R10B = 82,
  R10D = 83,
  R10W = 84,
  R11 = 85,
  R11B = 86,
  R11D = 87,
  R11W = 88,
  R12 = 89,
  R12B = 90,
  R12D = 91,
  R12W = 92,
  R13 = 93,
  R13B = 94,
  R13D = 95,
  R13W = 96,
  R14 = 97,
  R14B = 98,
  R14D = 99,
  R14W = 100,
  R15 = 101,
  R15B = 102,
  R15D = 103,
  R15W = 104,
  RAX = 105,
  RBP = 106,
  RBX = 107,
  RCX = 108,
  RDI = 109,
  RDX = 110,
  RIP = 111,
  RIZ = 112,
  RSI = 113,
  RSP = 114,
  SI = 115,
  SIL = 116,
  SP = 117,
  SPL = 118,
  SS = 119,
  ST0 = 120,
  ST1 = 121,
  ST2 = 122,
  ST3 = 123,
  ST4 = 124,
  ST5 = 125,
  ST6 = 126,
  ST7 = 127,
  XMM0 = 128,
  XMM1 = 129,
  XMM2 = 130,
  XMM3 = 131,
  XMM4 = 132,
  XMM5 = 133,
  XMM6 = 134,
  XMM7 = 135,
  XMM8 = 136,
  XMM9 = 137,
  XMM10 = 138,
  XMM11 = 139,
  XMM12 = 140,
  XMM13 = 141,
  XMM14 = 142,
  XMM15 = 143,
  YMM0 = 144,
  YMM1 = 145,
  YMM2 = 146,
  YMM3 = 147,
  YMM4 = 148,
  YMM5 = 149,
  YMM6 = 150,
  YMM7 = 151,
  YMM8 = 152,
  YMM9 = 153,
  YMM10 = 154,
  YMM11 = 155,
  YMM12 = 156,
  YMM13 = 157,
  YMM14 = 158,
  YMM15 = 159,
  NUM_TARGET_REGS 	// 160
};
}

// Register classes
namespace X86 {
enum {
  GR8RegClassID = 0,
  GR64RegClassID = 1,
  GR16RegClassID = 2,
  GR32RegClassID = 3,
  FR32RegClassID = 4,
  GR64_with_sub_8bitRegClassID = 5,
  FR64RegClassID = 6,
  CONTROL_REGRegClassID = 7,
  VR128RegClassID = 8,
  VR256RegClassID = 9,
  GR32_NOSPRegClassID = 10,
  GR32_NOAXRegClassID = 11,
  GR64_NOSPRegClassID = 12,
  GR64_TCRegClassID = 13,
  GR64_NOREXRegClassID = 14,
  GR8_NOREXRegClassID = 15,
  GR16_NOREXRegClassID = 16,
  GR32_NOREXRegClassID = 17,
  DEBUG_REGRegClassID = 18,
  VR64RegClassID = 19,
  GR64_TC_with_sub_8bitRegClassID = 20,
  GR64_NOREX_with_sub_8bitRegClassID = 21,
  RSTRegClassID = 22,
  RFP32RegClassID = 23,
  GR32_NOREX_NOSPRegClassID = 24,
  RFP64RegClassID = 25,
  GR64_NOREX_NOSPRegClassID = 26,
  RFP80RegClassID = 27,
  SEGMENT_REGRegClassID = 28,
  GR64_TCW64RegClassID = 29,
  GR8_ABCD_LRegClassID = 30,
  GR8_ABCD_HRegClassID = 31,
  GR16_ABCDRegClassID = 32,
  GR32_ABCDRegClassID = 33,
  GR64_ABCDRegClassID = 34,
  GR32_TCRegClassID = 35,
  GR32_NOAX_with_sub_8bit_hiRegClassID = 36,
  GR64_TC_with_sub_8bit_hiRegClassID = 37,
  GR32_ADRegClassID = 38,
  CCRRegClassID = 39
  };
}
} // End llvm namespace 
#endif // GET_REGINFO_ENUM

//===- TableGen'erated file -------------------------------------*- C++ -*-===//
//
// MC Register Information
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//


#ifdef GET_REGINFO_MC_DESC
#undef GET_REGINFO_MC_DESC
namespace llvm {

struct X86GenMCRegisterInfo : public MCRegisterInfo {
  explicit X86GenMCRegisterInfo(const MCRegisterDesc *D);
};

namespace {
  const unsigned AH_Overlaps[] = { X86::AH, X86::AX, X86::EAX, X86::RAX, 0 };
  const unsigned AL_Overlaps[] = { X86::AL, X86::AX, X86::EAX, X86::RAX, 0 };
  const unsigned AX_Overlaps[] = { X86::AX, X86::AH, X86::AL, X86::EAX, X86::RAX, 0 };
  const unsigned BH_Overlaps[] = { X86::BH, X86::BX, X86::EBX, X86::RBX, 0 };
  const unsigned BL_Overlaps[] = { X86::BL, X86::BX, X86::EBX, X86::RBX, 0 };
  const unsigned BP_Overlaps[] = { X86::BP, X86::BPL, X86::EBP, X86::RBP, 0 };
  const unsigned BPL_Overlaps[] = { X86::BPL, X86::BP, X86::EBP, X86::RBP, 0 };
  const unsigned BX_Overlaps[] = { X86::BX, X86::BH, X86::BL, X86::EBX, X86::RBX, 0 };
  const unsigned CH_Overlaps[] = { X86::CH, X86::CX, X86::ECX, X86::RCX, 0 };
  const unsigned CL_Overlaps[] = { X86::CL, X86::CX, X86::ECX, X86::RCX, 0 };
  const unsigned CR0_Overlaps[] = { X86::CR0, 0 };
  const unsigned CR1_Overlaps[] = { X86::CR1, 0 };
  const unsigned CR2_Overlaps[] = { X86::CR2, 0 };
  const unsigned CR3_Overlaps[] = { X86::CR3, 0 };
  const unsigned CR4_Overlaps[] = { X86::CR4, 0 };
  const unsigned CR5_Overlaps[] = { X86::CR5, 0 };
  const unsigned CR6_Overlaps[] = { X86::CR6, 0 };
  const unsigned CR7_Overlaps[] = { X86::CR7, 0 };
  const unsigned CR8_Overlaps[] = { X86::CR8, 0 };
  const unsigned CR9_Overlaps[] = { X86::CR9, 0 };
  const unsigned CR10_Overlaps[] = { X86::CR10, 0 };
  const unsigned CR11_Overlaps[] = { X86::CR11, 0 };
  const unsigned CR12_Overlaps[] = { X86::CR12, 0 };
  const unsigned CR13_Overlaps[] = { X86::CR13, 0 };
  const unsigned CR14_Overlaps[] = { X86::CR14, 0 };
  const unsigned CR15_Overlaps[] = { X86::CR15, 0 };
  const unsigned CS_Overlaps[] = { X86::CS, 0 };
  const unsigned CX_Overlaps[] = { X86::CX, X86::CH, X86::CL, X86::ECX, X86::RCX, 0 };
  const unsigned DH_Overlaps[] = { X86::DH, X86::DX, X86::EDX, X86::RDX, 0 };
  const unsigned DI_Overlaps[] = { X86::DI, X86::DIL, X86::EDI, X86::RDI, 0 };
  const unsigned DIL_Overlaps[] = { X86::DIL, X86::DI, X86::EDI, X86::RDI, 0 };
  const unsigned DL_Overlaps[] = { X86::DL, X86::DX, X86::EDX, X86::RDX, 0 };
  const unsigned DR0_Overlaps[] = { X86::DR0, 0 };
  const unsigned DR1_Overlaps[] = { X86::DR1, 0 };
  const unsigned DR2_Overlaps[] = { X86::DR2, 0 };
  const unsigned DR3_Overlaps[] = { X86::DR3, 0 };
  const unsigned DR4_Overlaps[] = { X86::DR4, 0 };
  const unsigned DR5_Overlaps[] = { X86::DR5, 0 };
  const unsigned DR6_Overlaps[] = { X86::DR6, 0 };
  const unsigned DR7_Overlaps[] = { X86::DR7, 0 };
  const unsigned DS_Overlaps[] = { X86::DS, 0 };
  const unsigned DX_Overlaps[] = { X86::DX, X86::DH, X86::DL, X86::EDX, X86::RDX, 0 };
  const unsigned EAX_Overlaps[] = { X86::EAX, X86::AH, X86::AL, X86::AX, X86::RAX, 0 };
  const unsigned EBP_Overlaps[] = { X86::EBP, X86::BP, X86::BPL, X86::RBP, 0 };
  const unsigned EBX_Overlaps[] = { X86::EBX, X86::BH, X86::BL, X86::BX, X86::RBX, 0 };
  const unsigned ECX_Overlaps[] = { X86::ECX, X86::CH, X86::CL, X86::CX, X86::RCX, 0 };
  const unsigned EDI_Overlaps[] = { X86::EDI, X86::DI, X86::DIL, X86::RDI, 0 };
  const unsigned EDX_Overlaps[] = { X86::EDX, X86::DH, X86::DL, X86::DX, X86::RDX, 0 };
  const unsigned EFLAGS_Overlaps[] = { X86::EFLAGS, 0 };
  const unsigned EIP_Overlaps[] = { X86::EIP, X86::IP, X86::RIP, 0 };
  const unsigned EIZ_Overlaps[] = { X86::EIZ, 0 };
  const unsigned ES_Overlaps[] = { X86::ES, 0 };
  const unsigned ESI_Overlaps[] = { X86::ESI, X86::RSI, X86::SI, X86::SIL, 0 };
  const unsigned ESP_Overlaps[] = { X86::ESP, X86::RSP, X86::SP, X86::SPL, 0 };
  const unsigned FP0_Overlaps[] = { X86::FP0, X86::ST7, 0 };
  const unsigned FP1_Overlaps[] = { X86::FP1, X86::ST6, 0 };
  const unsigned FP2_Overlaps[] = { X86::FP2, X86::ST5, 0 };
  const unsigned FP3_Overlaps[] = { X86::FP3, X86::ST4, 0 };
  const unsigned FP4_Overlaps[] = { X86::FP4, X86::ST3, 0 };
  const unsigned FP5_Overlaps[] = { X86::FP5, X86::ST2, 0 };
  const unsigned FP6_Overlaps[] = { X86::FP6, X86::ST1, 0 };
  const unsigned FS_Overlaps[] = { X86::FS, 0 };
  const unsigned GS_Overlaps[] = { X86::GS, 0 };
  const unsigned IP_Overlaps[] = { X86::IP, X86::EIP, X86::RIP, 0 };
  const unsigned MM0_Overlaps[] = { X86::MM0, 0 };
  const unsigned MM1_Overlaps[] = { X86::MM1, 0 };
  const unsigned MM2_Overlaps[] = { X86::MM2, 0 };
  const unsigned MM3_Overlaps[] = { X86::MM3, 0 };
  const unsigned MM4_Overlaps[] = { X86::MM4, 0 };
  const unsigned MM5_Overlaps[] = { X86::MM5, 0 };
  const unsigned MM6_Overlaps[] = { X86::MM6, 0 };
  const unsigned MM7_Overlaps[] = { X86::MM7, 0 };
  const unsigned R8_Overlaps[] = { X86::R8, X86::R8B, X86::R8D, X86::R8W, 0 };
  const unsigned R8B_Overlaps[] = { X86::R8B, X86::R8, X86::R8D, X86::R8W, 0 };
  const unsigned R8D_Overlaps[] = { X86::R8D, X86::R8, X86::R8B, X86::R8W, 0 };
  const unsigned R8W_Overlaps[] = { X86::R8W, X86::R8, X86::R8B, X86::R8D, 0 };
  const unsigned R9_Overlaps[] = { X86::R9, X86::R9B, X86::R9D, X86::R9W, 0 };
  const unsigned R9B_Overlaps[] = { X86::R9B, X86::R9, X86::R9D, X86::R9W, 0 };
  const unsigned R9D_Overlaps[] = { X86::R9D, X86::R9, X86::R9B, X86::R9W, 0 };
  const unsigned R9W_Overlaps[] = { X86::R9W, X86::R9, X86::R9B, X86::R9D, 0 };
  const unsigned R10_Overlaps[] = { X86::R10, X86::R10B, X86::R10D, X86::R10W, 0 };
  const unsigned R10B_Overlaps[] = { X86::R10B, X86::R10, X86::R10D, X86::R10W, 0 };
  const unsigned R10D_Overlaps[] = { X86::R10D, X86::R10, X86::R10B, X86::R10W, 0 };
  const unsigned R10W_Overlaps[] = { X86::R10W, X86::R10, X86::R10B, X86::R10D, 0 };
  const unsigned R11_Overlaps[] = { X86::R11, X86::R11B, X86::R11D, X86::R11W, 0 };
  const unsigned R11B_Overlaps[] = { X86::R11B, X86::R11, X86::R11D, X86::R11W, 0 };
  const unsigned R11D_Overlaps[] = { X86::R11D, X86::R11, X86::R11B, X86::R11W, 0 };
  const unsigned R11W_Overlaps[] = { X86::R11W, X86::R11, X86::R11B, X86::R11D, 0 };
  const unsigned R12_Overlaps[] = { X86::R12, X86::R12B, X86::R12D, X86::R12W, 0 };
  const unsigned R12B_Overlaps[] = { X86::R12B, X86::R12, X86::R12D, X86::R12W, 0 };
  const unsigned R12D_Overlaps[] = { X86::R12D, X86::R12, X86::R12B, X86::R12W, 0 };
  const unsigned R12W_Overlaps[] = { X86::R12W, X86::R12, X86::R12B, X86::R12D, 0 };
  const unsigned R13_Overlaps[] = { X86::R13, X86::R13B, X86::R13D, X86::R13W, 0 };
  const unsigned R13B_Overlaps[] = { X86::R13B, X86::R13, X86::R13D, X86::R13W, 0 };
  const unsigned R13D_Overlaps[] = { X86::R13D, X86::R13, X86::R13B, X86::R13W, 0 };
  const unsigned R13W_Overlaps[] = { X86::R13W, X86::R13, X86::R13B, X86::R13D, 0 };
  const unsigned R14_Overlaps[] = { X86::R14, X86::R14B, X86::R14D, X86::R14W, 0 };
  const unsigned R14B_Overlaps[] = { X86::R14B, X86::R14, X86::R14D, X86::R14W, 0 };
  const unsigned R14D_Overlaps[] = { X86::R14D, X86::R14, X86::R14B, X86::R14W, 0 };
  const unsigned R14W_Overlaps[] = { X86::R14W, X86::R14, X86::R14B, X86::R14D, 0 };
  const unsigned R15_Overlaps[] = { X86::R15, X86::R15B, X86::R15D, X86::R15W, 0 };
  const unsigned R15B_Overlaps[] = { X86::R15B, X86::R15, X86::R15D, X86::R15W, 0 };
  const unsigned R15D_Overlaps[] = { X86::R15D, X86::R15, X86::R15B, X86::R15W, 0 };
  const unsigned R15W_Overlaps[] = { X86::R15W, X86::R15, X86::R15B, X86::R15D, 0 };
  const unsigned RAX_Overlaps[] = { X86::RAX, X86::AH, X86::AL, X86::AX, X86::EAX, 0 };
  const unsigned RBP_Overlaps[] = { X86::RBP, X86::BP, X86::BPL, X86::EBP, 0 };
  const unsigned RBX_Overlaps[] = { X86::RBX, X86::BH, X86::BL, X86::BX, X86::EBX, 0 };
  const unsigned RCX_Overlaps[] = { X86::RCX, X86::CH, X86::CL, X86::CX, X86::ECX, 0 };
  const unsigned RDI_Overlaps[] = { X86::RDI, X86::DI, X86::DIL, X86::EDI, 0 };
  const unsigned RDX_Overlaps[] = { X86::RDX, X86::DH, X86::DL, X86::DX, X86::EDX, 0 };
  const unsigned RIP_Overlaps[] = { X86::RIP, X86::EIP, X86::IP, 0 };
  const unsigned RIZ_Overlaps[] = { X86::RIZ, 0 };
  const unsigned RSI_Overlaps[] = { X86::RSI, X86::ESI, X86::SI, X86::SIL, 0 };
  const unsigned RSP_Overlaps[] = { X86::RSP, X86::ESP, X86::SP, X86::SPL, 0 };
  const unsigned SI_Overlaps[] = { X86::SI, X86::ESI, X86::RSI, X86::SIL, 0 };
  const unsigned SIL_Overlaps[] = { X86::SIL, X86::ESI, X86::RSI, X86::SI, 0 };
  const unsigned SP_Overlaps[] = { X86::SP, X86::ESP, X86::RSP, X86::SPL, 0 };
  const unsigned SPL_Overlaps[] = { X86::SPL, X86::ESP, X86::RSP, X86::SP, 0 };
  const unsigned SS_Overlaps[] = { X86::SS, 0 };
  const unsigned ST0_Overlaps[] = { X86::ST0, 0 };
  const unsigned ST1_Overlaps[] = { X86::ST1, X86::FP6, 0 };
  const unsigned ST2_Overlaps[] = { X86::ST2, X86::FP5, 0 };
  const unsigned ST3_Overlaps[] = { X86::ST3, X86::FP4, 0 };
  const unsigned ST4_Overlaps[] = { X86::ST4, X86::FP3, 0 };
  const unsigned ST5_Overlaps[] = { X86::ST5, X86::FP2, 0 };
  const unsigned ST6_Overlaps[] = { X86::ST6, X86::FP1, 0 };
  const unsigned ST7_Overlaps[] = { X86::ST7, X86::FP0, 0 };
  const unsigned XMM0_Overlaps[] = { X86::XMM0, X86::YMM0, 0 };
  const unsigned XMM1_Overlaps[] = { X86::XMM1, X86::YMM1, 0 };
  const unsigned XMM2_Overlaps[] = { X86::XMM2, X86::YMM2, 0 };
  const unsigned XMM3_Overlaps[] = { X86::XMM3, X86::YMM3, 0 };
  const unsigned XMM4_Overlaps[] = { X86::XMM4, X86::YMM4, 0 };
  const unsigned XMM5_Overlaps[] = { X86::XMM5, X86::YMM5, 0 };
  const unsigned XMM6_Overlaps[] = { X86::XMM6, X86::YMM6, 0 };
  const unsigned XMM7_Overlaps[] = { X86::XMM7, X86::YMM7, 0 };
  const unsigned XMM8_Overlaps[] = { X86::XMM8, X86::YMM8, 0 };
  const unsigned XMM9_Overlaps[] = { X86::XMM9, X86::YMM9, 0 };
  const unsigned XMM10_Overlaps[] = { X86::XMM10, X86::YMM10, 0 };
  const unsigned XMM11_Overlaps[] = { X86::XMM11, X86::YMM11, 0 };
  const unsigned XMM12_Overlaps[] = { X86::XMM12, X86::YMM12, 0 };
  const unsigned XMM13_Overlaps[] = { X86::XMM13, X86::YMM13, 0 };
  const unsigned XMM14_Overlaps[] = { X86::XMM14, X86::YMM14, 0 };
  const unsigned XMM15_Overlaps[] = { X86::XMM15, X86::YMM15, 0 };
  const unsigned YMM0_Overlaps[] = { X86::YMM0, X86::XMM0, 0 };
  const unsigned YMM1_Overlaps[] = { X86::YMM1, X86::XMM1, 0 };
  const unsigned YMM2_Overlaps[] = { X86::YMM2, X86::XMM2, 0 };
  const unsigned YMM3_Overlaps[] = { X86::YMM3, X86::XMM3, 0 };
  const unsigned YMM4_Overlaps[] = { X86::YMM4, X86::XMM4, 0 };
  const unsigned YMM5_Overlaps[] = { X86::YMM5, X86::XMM5, 0 };
  const unsigned YMM6_Overlaps[] = { X86::YMM6, X86::XMM6, 0 };
  const unsigned YMM7_Overlaps[] = { X86::YMM7, X86::XMM7, 0 };
  const unsigned YMM8_Overlaps[] = { X86::YMM8, X86::XMM8, 0 };
  const unsigned YMM9_Overlaps[] = { X86::YMM9, X86::XMM9, 0 };
  const unsigned YMM10_Overlaps[] = { X86::YMM10, X86::XMM10, 0 };
  const unsigned YMM11_Overlaps[] = { X86::YMM11, X86::XMM11, 0 };
  const unsigned YMM12_Overlaps[] = { X86::YMM12, X86::XMM12, 0 };
  const unsigned YMM13_Overlaps[] = { X86::YMM13, X86::XMM13, 0 };
  const unsigned YMM14_Overlaps[] = { X86::YMM14, X86::XMM14, 0 };
  const unsigned YMM15_Overlaps[] = { X86::YMM15, X86::XMM15, 0 };
  const unsigned Empty_SubRegsSet[] = { 0 };
  const unsigned AX_SubRegsSet[] = { X86::AL, X86::AH, 0 };
  const unsigned BP_SubRegsSet[] = { X86::BPL, 0 };
  const unsigned BX_SubRegsSet[] = { X86::BL, X86::BH, 0 };
  const unsigned CX_SubRegsSet[] = { X86::CL, X86::CH, 0 };
  const unsigned DI_SubRegsSet[] = { X86::DIL, 0 };
  const unsigned DX_SubRegsSet[] = { X86::DL, X86::DH, 0 };
  const unsigned EAX_SubRegsSet[] = { X86::AX, X86::AL, X86::AH, 0 };
  const unsigned EBP_SubRegsSet[] = { X86::BP, X86::BPL, 0 };
  const unsigned EBX_SubRegsSet[] = { X86::BX, X86::BL, X86::BH, 0 };
  const unsigned ECX_SubRegsSet[] = { X86::CX, X86::CL, X86::CH, 0 };
  const unsigned EDI_SubRegsSet[] = { X86::DI, X86::DIL, 0 };
  const unsigned EDX_SubRegsSet[] = { X86::DX, X86::DL, X86::DH, 0 };
  const unsigned EIP_SubRegsSet[] = { X86::IP, 0 };
  const unsigned ESI_SubRegsSet[] = { X86::SI, X86::SIL, 0 };
  const unsigned ESP_SubRegsSet[] = { X86::SP, X86::SPL, 0 };
  const unsigned R8_SubRegsSet[] = { X86::R8D, X86::R8W, X86::R8B, 0 };
  const unsigned R8D_SubRegsSet[] = { X86::R8W, X86::R8B, 0 };
  const unsigned R8W_SubRegsSet[] = { X86::R8B, 0 };
  const unsigned R9_SubRegsSet[] = { X86::R9D, X86::R9W, X86::R9B, 0 };
  const unsigned R9D_SubRegsSet[] = { X86::R9W, X86::R9B, 0 };
  const unsigned R9W_SubRegsSet[] = { X86::R9B, 0 };
  const unsigned R10_SubRegsSet[] = { X86::R10D, X86::R10W, X86::R10B, 0 };
  const unsigned R10D_SubRegsSet[] = { X86::R10W, X86::R10B, 0 };
  const unsigned R10W_SubRegsSet[] = { X86::R10B, 0 };
  const unsigned R11_SubRegsSet[] = { X86::R11D, X86::R11W, X86::R11B, 0 };
  const unsigned R11D_SubRegsSet[] = { X86::R11W, X86::R11B, 0 };
  const unsigned R11W_SubRegsSet[] = { X86::R11B, 0 };
  const unsigned R12_SubRegsSet[] = { X86::R12D, X86::R12W, X86::R12B, 0 };
  const unsigned R12D_SubRegsSet[] = { X86::R12W, X86::R12B, 0 };
  const unsigned R12W_SubRegsSet[] = { X86::R12B, 0 };
  const unsigned R13_SubRegsSet[] = { X86::R13D, X86::R13W, X86::R13B, 0 };
  const unsigned R13D_SubRegsSet[] = { X86::R13W, X86::R13B, 0 };
  const unsigned R13W_SubRegsSet[] = { X86::R13B, 0 };
  const unsigned R14_SubRegsSet[] = { X86::R14D, X86::R14W, X86::R14B, 0 };
  const unsigned R14D_SubRegsSet[] = { X86::R14W, X86::R14B, 0 };
  const unsigned R14W_SubRegsSet[] = { X86::R14B, 0 };
  const unsigned R15_SubRegsSet[] = { X86::R15D, X86::R15W, X86::R15B, 0 };
  const unsigned R15D_SubRegsSet[] = { X86::R15W, X86::R15B, 0 };
  const unsigned R15W_SubRegsSet[] = { X86::R15B, 0 };
  const unsigned RAX_SubRegsSet[] = { X86::EAX, X86::AX, X86::AL, X86::AH, 0 };
  const unsigned RBP_SubRegsSet[] = { X86::EBP, X86::BP, X86::BPL, 0 };
  const unsigned RBX_SubRegsSet[] = { X86::EBX, X86::BX, X86::BL, X86::BH, 0 };
  const unsigned RCX_SubRegsSet[] = { X86::ECX, X86::CX, X86::CL, X86::CH, 0 };
  const unsigned RDI_SubRegsSet[] = { X86::EDI, X86::DI, X86::DIL, 0 };
  const unsigned RDX_SubRegsSet[] = { X86::EDX, X86::DX, X86::DL, X86::DH, 0 };
  const unsigned RIP_SubRegsSet[] = { X86::EIP, X86::IP, 0 };
  const unsigned RSI_SubRegsSet[] = { X86::ESI, X86::SI, X86::SIL, 0 };
  const unsigned RSP_SubRegsSet[] = { X86::ESP, X86::SP, X86::SPL, 0 };
  const unsigned SI_SubRegsSet[] = { X86::SIL, 0 };
  const unsigned SP_SubRegsSet[] = { X86::SPL, 0 };
  const unsigned XMM0_SubRegsSet[] = { 0 };
  const unsigned XMM1_SubRegsSet[] = { 0 };
  const unsigned XMM2_SubRegsSet[] = { 0 };
  const unsigned XMM3_SubRegsSet[] = { 0 };
  const unsigned XMM4_SubRegsSet[] = { 0 };
  const unsigned XMM5_SubRegsSet[] = { 0 };
  const unsigned XMM6_SubRegsSet[] = { 0 };
  const unsigned XMM7_SubRegsSet[] = { 0 };
  const unsigned XMM8_SubRegsSet[] = { 0 };
  const unsigned XMM9_SubRegsSet[] = { 0 };
  const unsigned XMM10_SubRegsSet[] = { 0 };
  const unsigned XMM11_SubRegsSet[] = { 0 };
  const unsigned XMM12_SubRegsSet[] = { 0 };
  const unsigned XMM13_SubRegsSet[] = { 0 };
  const unsigned XMM14_SubRegsSet[] = { 0 };
  const unsigned XMM15_SubRegsSet[] = { 0 };
  const unsigned YMM0_SubRegsSet[] = { X86::XMM0, 0 };
  const unsigned YMM1_SubRegsSet[] = { X86::XMM1, 0 };
  const unsigned YMM2_SubRegsSet[] = { X86::XMM2, 0 };
  const unsigned YMM3_SubRegsSet[] = { X86::XMM3, 0 };
  const unsigned YMM4_SubRegsSet[] = { X86::XMM4, 0 };
  const unsigned YMM5_SubRegsSet[] = { X86::XMM5, 0 };
  const unsigned YMM6_SubRegsSet[] = { X86::XMM6, 0 };
  const unsigned YMM7_SubRegsSet[] = { X86::XMM7, 0 };
  const unsigned YMM8_SubRegsSet[] = { X86::XMM8, 0 };
  const unsigned YMM9_SubRegsSet[] = { X86::XMM9, 0 };
  const unsigned YMM10_SubRegsSet[] = { X86::XMM10, 0 };
  const unsigned YMM11_SubRegsSet[] = { X86::XMM11, 0 };
  const unsigned YMM12_SubRegsSet[] = { X86::XMM12, 0 };
  const unsigned YMM13_SubRegsSet[] = { X86::XMM13, 0 };
  const unsigned YMM14_SubRegsSet[] = { X86::XMM14, 0 };
  const unsigned YMM15_SubRegsSet[] = { X86::XMM15, 0 };
  const unsigned Empty_SuperRegsSet[] = { 0 };
  const unsigned AH_SuperRegsSet[] = { X86::AX, X86::EAX, X86::RAX, 0 };
  const unsigned AL_SuperRegsSet[] = { X86::AX, X86::EAX, X86::RAX, 0 };
  const unsigned AX_SuperRegsSet[] = { X86::EAX, X86::RAX, 0 };
  const unsigned BH_SuperRegsSet[] = { X86::BX, X86::EBX, X86::RBX, 0 };
  const unsigned BL_SuperRegsSet[] = { X86::BX, X86::EBX, X86::RBX, 0 };
  const unsigned BP_SuperRegsSet[] = { X86::EBP, X86::RBP, 0 };
  const unsigned BPL_SuperRegsSet[] = { X86::BP, X86::EBP, X86::RBP, 0 };
  const unsigned BX_SuperRegsSet[] = { X86::EBX, X86::RBX, 0 };
  const unsigned CH_SuperRegsSet[] = { X86::CX, X86::ECX, X86::RCX, 0 };
  const unsigned CL_SuperRegsSet[] = { X86::CX, X86::ECX, X86::RCX, 0 };
  const unsigned CX_SuperRegsSet[] = { X86::ECX, X86::RCX, 0 };
  const unsigned DH_SuperRegsSet[] = { X86::DX, X86::EDX, X86::RDX, 0 };
  const unsigned DI_SuperRegsSet[] = { X86::EDI, X86::RDI, 0 };
  const unsigned DIL_SuperRegsSet[] = { X86::DI, X86::EDI, X86::RDI, 0 };
  const unsigned DL_SuperRegsSet[] = { X86::DX, X86::EDX, X86::RDX, 0 };
  const unsigned DX_SuperRegsSet[] = { X86::EDX, X86::RDX, 0 };
  const unsigned EAX_SuperRegsSet[] = { X86::RAX, 0 };
  const unsigned EBP_SuperRegsSet[] = { X86::RBP, 0 };
  const unsigned EBX_SuperRegsSet[] = { X86::RBX, 0 };
  const unsigned ECX_SuperRegsSet[] = { X86::RCX, 0 };
  const unsigned EDI_SuperRegsSet[] = { X86::RDI, 0 };
  const unsigned EDX_SuperRegsSet[] = { X86::RDX, 0 };
  const unsigned EIP_SuperRegsSet[] = { X86::RIP, 0 };
  const unsigned ESI_SuperRegsSet[] = { X86::RSI, 0 };
  const unsigned ESP_SuperRegsSet[] = { X86::RSP, 0 };
  const unsigned IP_SuperRegsSet[] = { X86::EIP, X86::RIP, 0 };
  const unsigned R8B_SuperRegsSet[] = { X86::R8W, X86::R8D, X86::R8, 0 };
  const unsigned R8D_SuperRegsSet[] = { X86::R8, 0 };
  const unsigned R8W_SuperRegsSet[] = { X86::R8D, X86::R8, 0 };
  const unsigned R9B_SuperRegsSet[] = { X86::R9W, X86::R9D, X86::R9, 0 };
  const unsigned R9D_SuperRegsSet[] = { X86::R9, 0 };
  const unsigned R9W_SuperRegsSet[] = { X86::R9D, X86::R9, 0 };
  const unsigned R10B_SuperRegsSet[] = { X86::R10W, X86::R10D, X86::R10, 0 };
  const unsigned R10D_SuperRegsSet[] = { X86::R10, 0 };
  const unsigned R10W_SuperRegsSet[] = { X86::R10D, X86::R10, 0 };
  const unsigned R11B_SuperRegsSet[] = { X86::R11W, X86::R11D, X86::R11, 0 };
  const unsigned R11D_SuperRegsSet[] = { X86::R11, 0 };
  const unsigned R11W_SuperRegsSet[] = { X86::R11D, X86::R11, 0 };
  const unsigned R12B_SuperRegsSet[] = { X86::R12W, X86::R12D, X86::R12, 0 };
  const unsigned R12D_SuperRegsSet[] = { X86::R12, 0 };
  const unsigned R12W_SuperRegsSet[] = { X86::R12D, X86::R12, 0 };
  const unsigned R13B_SuperRegsSet[] = { X86::R13W, X86::R13D, X86::R13, 0 };
  const unsigned R13D_SuperRegsSet[] = { X86::R13, 0 };
  const unsigned R13W_SuperRegsSet[] = { X86::R13D, X86::R13, 0 };
  const unsigned R14B_SuperRegsSet[] = { X86::R14W, X86::R14D, X86::R14, 0 };
  const unsigned R14D_SuperRegsSet[] = { X86::R14, 0 };
  const unsigned R14W_SuperRegsSet[] = { X86::R14D, X86::R14, 0 };
  const unsigned R15B_SuperRegsSet[] = { X86::R15W, X86::R15D, X86::R15, 0 };
  const unsigned R15D_SuperRegsSet[] = { X86::R15, 0 };
  const unsigned R15W_SuperRegsSet[] = { X86::R15D, X86::R15, 0 };
  const unsigned SI_SuperRegsSet[] = { X86::ESI, X86::RSI, 0 };
  const unsigned SIL_SuperRegsSet[] = { X86::SI, X86::ESI, X86::RSI, 0 };
  const unsigned SP_SuperRegsSet[] = { X86::ESP, X86::RSP, 0 };
  const unsigned SPL_SuperRegsSet[] = { X86::SP, X86::ESP, X86::RSP, 0 };
  const unsigned XMM0_SuperRegsSet[] = { X86::YMM0, 0 };
  const unsigned XMM1_SuperRegsSet[] = { X86::YMM1, 0 };
  const unsigned XMM2_SuperRegsSet[] = { X86::YMM2, 0 };
  const unsigned XMM3_SuperRegsSet[] = { X86::YMM3, 0 };
  const unsigned XMM4_SuperRegsSet[] = { X86::YMM4, 0 };
  const unsigned XMM5_SuperRegsSet[] = { X86::YMM5, 0 };
  const unsigned XMM6_SuperRegsSet[] = { X86::YMM6, 0 };
  const unsigned XMM7_SuperRegsSet[] = { X86::YMM7, 0 };
  const unsigned XMM8_SuperRegsSet[] = { X86::YMM8, 0 };
  const unsigned XMM9_SuperRegsSet[] = { X86::YMM9, 0 };
  const unsigned XMM10_SuperRegsSet[] = { X86::YMM10, 0 };
  const unsigned XMM11_SuperRegsSet[] = { X86::YMM11, 0 };
  const unsigned XMM12_SuperRegsSet[] = { X86::YMM12, 0 };
  const unsigned XMM13_SuperRegsSet[] = { X86::YMM13, 0 };
  const unsigned XMM14_SuperRegsSet[] = { X86::YMM14, 0 };
  const unsigned XMM15_SuperRegsSet[] = { X86::YMM15, 0 };
}

MCRegisterDesc X86RegDesc[] = { // Descriptors
  { "NOREG",	0,	0,	0 },
  { "AH",	AH_Overlaps,	Empty_SubRegsSet,	AH_SuperRegsSet },
  { "AL",	AL_Overlaps,	Empty_SubRegsSet,	AL_SuperRegsSet },
  { "AX",	AX_Overlaps,	AX_SubRegsSet,	AX_SuperRegsSet },
  { "BH",	BH_Overlaps,	Empty_SubRegsSet,	BH_SuperRegsSet },
  { "BL",	BL_Overlaps,	Empty_SubRegsSet,	BL_SuperRegsSet },
  { "BP",	BP_Overlaps,	BP_SubRegsSet,	BP_SuperRegsSet },
  { "BPL",	BPL_Overlaps,	Empty_SubRegsSet,	BPL_SuperRegsSet },
  { "BX",	BX_Overlaps,	BX_SubRegsSet,	BX_SuperRegsSet },
  { "CH",	CH_Overlaps,	Empty_SubRegsSet,	CH_SuperRegsSet },
  { "CL",	CL_Overlaps,	Empty_SubRegsSet,	CL_SuperRegsSet },
  { "CR0",	CR0_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR1",	CR1_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR2",	CR2_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR3",	CR3_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR4",	CR4_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR5",	CR5_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR6",	CR6_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR7",	CR7_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR8",	CR8_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR9",	CR9_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR10",	CR10_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR11",	CR11_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR12",	CR12_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR13",	CR13_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR14",	CR14_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CR15",	CR15_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CS",	CS_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "CX",	CX_Overlaps,	CX_SubRegsSet,	CX_SuperRegsSet },
  { "DH",	DH_Overlaps,	Empty_SubRegsSet,	DH_SuperRegsSet },
  { "DI",	DI_Overlaps,	DI_SubRegsSet,	DI_SuperRegsSet },
  { "DIL",	DIL_Overlaps,	Empty_SubRegsSet,	DIL_SuperRegsSet },
  { "DL",	DL_Overlaps,	Empty_SubRegsSet,	DL_SuperRegsSet },
  { "DR0",	DR0_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR1",	DR1_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR2",	DR2_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR3",	DR3_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR4",	DR4_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR5",	DR5_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR6",	DR6_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DR7",	DR7_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DS",	DS_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "DX",	DX_Overlaps,	DX_SubRegsSet,	DX_SuperRegsSet },
  { "EAX",	EAX_Overlaps,	EAX_SubRegsSet,	EAX_SuperRegsSet },
  { "EBP",	EBP_Overlaps,	EBP_SubRegsSet,	EBP_SuperRegsSet },
  { "EBX",	EBX_Overlaps,	EBX_SubRegsSet,	EBX_SuperRegsSet },
  { "ECX",	ECX_Overlaps,	ECX_SubRegsSet,	ECX_SuperRegsSet },
  { "EDI",	EDI_Overlaps,	EDI_SubRegsSet,	EDI_SuperRegsSet },
  { "EDX",	EDX_Overlaps,	EDX_SubRegsSet,	EDX_SuperRegsSet },
  { "EFLAGS",	EFLAGS_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "EIP",	EIP_Overlaps,	EIP_SubRegsSet,	EIP_SuperRegsSet },
  { "EIZ",	EIZ_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ES",	ES_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ESI",	ESI_Overlaps,	ESI_SubRegsSet,	ESI_SuperRegsSet },
  { "ESP",	ESP_Overlaps,	ESP_SubRegsSet,	ESP_SuperRegsSet },
  { "FP0",	FP0_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FP1",	FP1_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FP2",	FP2_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FP3",	FP3_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FP4",	FP4_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FP5",	FP5_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FP6",	FP6_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "FS",	FS_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "GS",	GS_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "IP",	IP_Overlaps,	Empty_SubRegsSet,	IP_SuperRegsSet },
  { "MM0",	MM0_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM1",	MM1_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM2",	MM2_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM3",	MM3_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM4",	MM4_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM5",	MM5_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM6",	MM6_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "MM7",	MM7_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "R8",	R8_Overlaps,	R8_SubRegsSet,	Empty_SuperRegsSet },
  { "R8B",	R8B_Overlaps,	Empty_SubRegsSet,	R8B_SuperRegsSet },
  { "R8D",	R8D_Overlaps,	R8D_SubRegsSet,	R8D_SuperRegsSet },
  { "R8W",	R8W_Overlaps,	R8W_SubRegsSet,	R8W_SuperRegsSet },
  { "R9",	R9_Overlaps,	R9_SubRegsSet,	Empty_SuperRegsSet },
  { "R9B",	R9B_Overlaps,	Empty_SubRegsSet,	R9B_SuperRegsSet },
  { "R9D",	R9D_Overlaps,	R9D_SubRegsSet,	R9D_SuperRegsSet },
  { "R9W",	R9W_Overlaps,	R9W_SubRegsSet,	R9W_SuperRegsSet },
  { "R10",	R10_Overlaps,	R10_SubRegsSet,	Empty_SuperRegsSet },
  { "R10B",	R10B_Overlaps,	Empty_SubRegsSet,	R10B_SuperRegsSet },
  { "R10D",	R10D_Overlaps,	R10D_SubRegsSet,	R10D_SuperRegsSet },
  { "R10W",	R10W_Overlaps,	R10W_SubRegsSet,	R10W_SuperRegsSet },
  { "R11",	R11_Overlaps,	R11_SubRegsSet,	Empty_SuperRegsSet },
  { "R11B",	R11B_Overlaps,	Empty_SubRegsSet,	R11B_SuperRegsSet },
  { "R11D",	R11D_Overlaps,	R11D_SubRegsSet,	R11D_SuperRegsSet },
  { "R11W",	R11W_Overlaps,	R11W_SubRegsSet,	R11W_SuperRegsSet },
  { "R12",	R12_Overlaps,	R12_SubRegsSet,	Empty_SuperRegsSet },
  { "R12B",	R12B_Overlaps,	Empty_SubRegsSet,	R12B_SuperRegsSet },
  { "R12D",	R12D_Overlaps,	R12D_SubRegsSet,	R12D_SuperRegsSet },
  { "R12W",	R12W_Overlaps,	R12W_SubRegsSet,	R12W_SuperRegsSet },
  { "R13",	R13_Overlaps,	R13_SubRegsSet,	Empty_SuperRegsSet },
  { "R13B",	R13B_Overlaps,	Empty_SubRegsSet,	R13B_SuperRegsSet },
  { "R13D",	R13D_Overlaps,	R13D_SubRegsSet,	R13D_SuperRegsSet },
  { "R13W",	R13W_Overlaps,	R13W_SubRegsSet,	R13W_SuperRegsSet },
  { "R14",	R14_Overlaps,	R14_SubRegsSet,	Empty_SuperRegsSet },
  { "R14B",	R14B_Overlaps,	Empty_SubRegsSet,	R14B_SuperRegsSet },
  { "R14D",	R14D_Overlaps,	R14D_SubRegsSet,	R14D_SuperRegsSet },
  { "R14W",	R14W_Overlaps,	R14W_SubRegsSet,	R14W_SuperRegsSet },
  { "R15",	R15_Overlaps,	R15_SubRegsSet,	Empty_SuperRegsSet },
  { "R15B",	R15B_Overlaps,	Empty_SubRegsSet,	R15B_SuperRegsSet },
  { "R15D",	R15D_Overlaps,	R15D_SubRegsSet,	R15D_SuperRegsSet },
  { "R15W",	R15W_Overlaps,	R15W_SubRegsSet,	R15W_SuperRegsSet },
  { "RAX",	RAX_Overlaps,	RAX_SubRegsSet,	Empty_SuperRegsSet },
  { "RBP",	RBP_Overlaps,	RBP_SubRegsSet,	Empty_SuperRegsSet },
  { "RBX",	RBX_Overlaps,	RBX_SubRegsSet,	Empty_SuperRegsSet },
  { "RCX",	RCX_Overlaps,	RCX_SubRegsSet,	Empty_SuperRegsSet },
  { "RDI",	RDI_Overlaps,	RDI_SubRegsSet,	Empty_SuperRegsSet },
  { "RDX",	RDX_Overlaps,	RDX_SubRegsSet,	Empty_SuperRegsSet },
  { "RIP",	RIP_Overlaps,	RIP_SubRegsSet,	Empty_SuperRegsSet },
  { "RIZ",	RIZ_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "RSI",	RSI_Overlaps,	RSI_SubRegsSet,	Empty_SuperRegsSet },
  { "RSP",	RSP_Overlaps,	RSP_SubRegsSet,	Empty_SuperRegsSet },
  { "SI",	SI_Overlaps,	SI_SubRegsSet,	SI_SuperRegsSet },
  { "SIL",	SIL_Overlaps,	Empty_SubRegsSet,	SIL_SuperRegsSet },
  { "SP",	SP_Overlaps,	SP_SubRegsSet,	SP_SuperRegsSet },
  { "SPL",	SPL_Overlaps,	Empty_SubRegsSet,	SPL_SuperRegsSet },
  { "SS",	SS_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST0",	ST0_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST1",	ST1_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST2",	ST2_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST3",	ST3_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST4",	ST4_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST5",	ST5_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST6",	ST6_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "ST7",	ST7_Overlaps,	Empty_SubRegsSet,	Empty_SuperRegsSet },
  { "XMM0",	XMM0_Overlaps,	XMM0_SubRegsSet,	XMM0_SuperRegsSet },
  { "XMM1",	XMM1_Overlaps,	XMM1_SubRegsSet,	XMM1_SuperRegsSet },
  { "XMM2",	XMM2_Overlaps,	XMM2_SubRegsSet,	XMM2_SuperRegsSet },
  { "XMM3",	XMM3_Overlaps,	XMM3_SubRegsSet,	XMM3_SuperRegsSet },
  { "XMM4",	XMM4_Overlaps,	XMM4_SubRegsSet,	XMM4_SuperRegsSet },
  { "XMM5",	XMM5_Overlaps,	XMM5_SubRegsSet,	XMM5_SuperRegsSet },
  { "XMM6",	XMM6_Overlaps,	XMM6_SubRegsSet,	XMM6_SuperRegsSet },
  { "XMM7",	XMM7_Overlaps,	XMM7_SubRegsSet,	XMM7_SuperRegsSet },
  { "XMM8",	XMM8_Overlaps,	XMM8_SubRegsSet,	XMM8_SuperRegsSet },
  { "XMM9",	XMM9_Overlaps,	XMM9_SubRegsSet,	XMM9_SuperRegsSet },
  { "XMM10",	XMM10_Overlaps,	XMM10_SubRegsSet,	XMM10_SuperRegsSet },
  { "XMM11",	XMM11_Overlaps,	XMM11_SubRegsSet,	XMM11_SuperRegsSet },
  { "XMM12",	XMM12_Overlaps,	XMM12_SubRegsSet,	XMM12_SuperRegsSet },
  { "XMM13",	XMM13_Overlaps,	XMM13_SubRegsSet,	XMM13_SuperRegsSet },
  { "XMM14",	XMM14_Overlaps,	XMM14_SubRegsSet,	XMM14_SuperRegsSet },
  { "XMM15",	XMM15_Overlaps,	XMM15_SubRegsSet,	XMM15_SuperRegsSet },
  { "YMM0",	YMM0_Overlaps,	YMM0_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM1",	YMM1_Overlaps,	YMM1_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM2",	YMM2_Overlaps,	YMM2_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM3",	YMM3_Overlaps,	YMM3_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM4",	YMM4_Overlaps,	YMM4_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM5",	YMM5_Overlaps,	YMM5_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM6",	YMM6_Overlaps,	YMM6_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM7",	YMM7_Overlaps,	YMM7_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM8",	YMM8_Overlaps,	YMM8_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM9",	YMM9_Overlaps,	YMM9_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM10",	YMM10_Overlaps,	YMM10_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM11",	YMM11_Overlaps,	YMM11_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM12",	YMM12_Overlaps,	YMM12_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM13",	YMM13_Overlaps,	YMM13_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM14",	YMM14_Overlaps,	YMM14_SubRegsSet,	Empty_SuperRegsSet },
  { "YMM15",	YMM15_Overlaps,	YMM15_SubRegsSet,	Empty_SuperRegsSet },
};

namespace {     // Register classes...
  // GR8 Register Class...
  static const unsigned GR8[] = {
    X86::AL, X86::CL, X86::DL, X86::AH, X86::CH, X86::DH, X86::BL, X86::BH, X86::SIL, X86::DIL, X86::BPL, X86::SPL, X86::R8B, X86::R9B, X86::R10B, X86::R11B, X86::R14B, X86::R15B, X86::R12B, X86::R13B, 
  };

  // GR8 Bit set.
  static const unsigned char GR8Bits[] = {
    0xb6, 0x06, 0x00, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x00, 0x50, 
  };

  // GR64 Register Class...
  static const unsigned GR64[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::R8, X86::R9, X86::R10, X86::R11, X86::RBX, X86::R14, X86::R15, X86::R12, X86::R13, X86::RBP, X86::RSP, X86::RIP, 
  };

  // GR64 Bit set.
  static const unsigned char GR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0xfe, 0x06, 
  };

  // GR16 Register Class...
  static const unsigned GR16[] = {
    X86::AX, X86::CX, X86::DX, X86::SI, X86::DI, X86::BX, X86::BP, X86::SP, X86::R8W, X86::R9W, X86::R10W, X86::R11W, X86::R14W, X86::R15W, X86::R12W, X86::R13W, 
  };

  // GR16 Bit set.
  static const unsigned char GR16Bits[] = {
    0x48, 0x01, 0x00, 0x50, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x11, 0x11, 0x11, 0x01, 0x28, 
  };

  // GR32 Register Class...
  static const unsigned GR32[] = {
    X86::EAX, X86::ECX, X86::EDX, X86::ESI, X86::EDI, X86::EBX, X86::EBP, X86::ESP, X86::R8D, X86::R9D, X86::R10D, X86::R11D, X86::R14D, X86::R15D, X86::R12D, X86::R13D, 
  };

  // GR32 Bit set.
  static const unsigned char GR32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x61, 0x00, 0x00, 0x88, 0x88, 0x88, 0x88, 
  };

  // FR32 Register Class...
  static const unsigned FR32[] = {
    X86::XMM0, X86::XMM1, X86::XMM2, X86::XMM3, X86::XMM4, X86::XMM5, X86::XMM6, X86::XMM7, X86::XMM8, X86::XMM9, X86::XMM10, X86::XMM11, X86::XMM12, X86::XMM13, X86::XMM14, X86::XMM15, 
  };

  // FR32 Bit set.
  static const unsigned char FR32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
  };

  // GR64_with_sub_8bit Register Class...
  static const unsigned GR64_with_sub_8bit[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::R8, X86::R9, X86::R10, X86::R11, X86::RBX, X86::R14, X86::R15, X86::R12, X86::R13, X86::RBP, X86::RSP, 
  };

  // GR64_with_sub_8bit Bit set.
  static const unsigned char GR64_with_sub_8bitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x7e, 0x06, 
  };

  // FR64 Register Class...
  static const unsigned FR64[] = {
    X86::XMM0, X86::XMM1, X86::XMM2, X86::XMM3, X86::XMM4, X86::XMM5, X86::XMM6, X86::XMM7, X86::XMM8, X86::XMM9, X86::XMM10, X86::XMM11, X86::XMM12, X86::XMM13, X86::XMM14, X86::XMM15, 
  };

  // FR64 Bit set.
  static const unsigned char FR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
  };

  // CONTROL_REG Register Class...
  static const unsigned CONTROL_REG[] = {
    X86::CR0, X86::CR1, X86::CR2, X86::CR3, X86::CR4, X86::CR5, X86::CR6, X86::CR7, X86::CR8, X86::CR9, X86::CR10, X86::CR11, X86::CR12, X86::CR13, X86::CR14, X86::CR15, 
  };

  // CONTROL_REG Bit set.
  static const unsigned char CONTROL_REGBits[] = {
    0x00, 0xf8, 0xff, 0x07, 
  };

  // VR128 Register Class...
  static const unsigned VR128[] = {
    X86::XMM0, X86::XMM1, X86::XMM2, X86::XMM3, X86::XMM4, X86::XMM5, X86::XMM6, X86::XMM7, X86::XMM8, X86::XMM9, X86::XMM10, X86::XMM11, X86::XMM12, X86::XMM13, X86::XMM14, X86::XMM15, 
  };

  // VR128 Bit set.
  static const unsigned char VR128Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
  };

  // VR256 Register Class...
  static const unsigned VR256[] = {
    X86::YMM0, X86::YMM1, X86::YMM2, X86::YMM3, X86::YMM4, X86::YMM5, X86::YMM6, X86::YMM7, X86::YMM8, X86::YMM9, X86::YMM10, X86::YMM11, X86::YMM12, X86::YMM13, X86::YMM14, X86::YMM15, 
  };

  // VR256 Bit set.
  static const unsigned char VR256Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
  };

  // GR32_NOSP Register Class...
  static const unsigned GR32_NOSP[] = {
    X86::EAX, X86::ECX, X86::EDX, X86::ESI, X86::EDI, X86::EBX, X86::EBP, X86::R8D, X86::R9D, X86::R10D, X86::R11D, X86::R14D, X86::R15D, X86::R12D, X86::R13D, 
  };

  // GR32_NOSP Bit set.
  static const unsigned char GR32_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x21, 0x00, 0x00, 0x88, 0x88, 0x88, 0x88, 
  };

  // GR32_NOAX Register Class...
  static const unsigned GR32_NOAX[] = {
    X86::ECX, X86::EDX, X86::ESI, X86::EDI, X86::EBX, X86::EBP, X86::ESP, X86::R8D, X86::R9D, X86::R10D, X86::R11D, X86::R14D, X86::R15D, X86::R12D, X86::R13D, 
  };

  // GR32_NOAX Bit set.
  static const unsigned char GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x61, 0x00, 0x00, 0x88, 0x88, 0x88, 0x88, 
  };

  // GR64_NOSP Register Class...
  static const unsigned GR64_NOSP[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::R8, X86::R9, X86::R10, X86::R11, X86::RBX, X86::R14, X86::R15, X86::R12, X86::R13, X86::RBP, 
  };

  // GR64_NOSP Bit set.
  static const unsigned char GR64_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x7e, 0x02, 
  };

  // GR64_TC Register Class...
  static const unsigned GR64_TC[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::R8, X86::R9, X86::R11, X86::RIP, 
  };

  // GR64_TC Bit set.
  static const unsigned char GR64_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0xf2, 0x02, 
  };

  // GR64_NOREX Register Class...
  static const unsigned GR64_NOREX[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::RBX, X86::RBP, X86::RSP, X86::RIP, 
  };

  // GR64_NOREX Bit set.
  static const unsigned char GR64_NOREXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x06, 
  };

  // GR8_NOREX Register Class...
  static const unsigned GR8_NOREX[] = {
    X86::AL, X86::CL, X86::DL, X86::AH, X86::CH, X86::DH, X86::BL, X86::BH, 
  };

  // GR8_NOREX Bit set.
  static const unsigned char GR8_NOREXBits[] = {
    0x36, 0x06, 0x00, 0x20, 0x01, 
  };

  // GR16_NOREX Register Class...
  static const unsigned GR16_NOREX[] = {
    X86::AX, X86::CX, X86::DX, X86::SI, X86::DI, X86::BX, X86::BP, X86::SP, 
  };

  // GR16_NOREX Bit set.
  static const unsigned char GR16_NOREXBits[] = {
    0x48, 0x01, 0x00, 0x50, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 
  };

  // GR32_NOREX Register Class...
  static const unsigned GR32_NOREX[] = {
    X86::EAX, X86::ECX, X86::EDX, X86::ESI, X86::EDI, X86::EBX, X86::EBP, X86::ESP, 
  };

  // GR32_NOREX Bit set.
  static const unsigned char GR32_NOREXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x61, 
  };

  // DEBUG_REG Register Class...
  static const unsigned DEBUG_REG[] = {
    X86::DR0, X86::DR1, X86::DR2, X86::DR3, X86::DR4, X86::DR5, X86::DR6, X86::DR7, 
  };

  // DEBUG_REG Bit set.
  static const unsigned char DEBUG_REGBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xfe, 0x01, 
  };

  // VR64 Register Class...
  static const unsigned VR64[] = {
    X86::MM0, X86::MM1, X86::MM2, X86::MM3, X86::MM4, X86::MM5, X86::MM6, X86::MM7, 
  };

  // VR64 Bit set.
  static const unsigned char VR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x01, 
  };

  // GR64_TC_with_sub_8bit Register Class...
  static const unsigned GR64_TC_with_sub_8bit[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::R8, X86::R9, X86::R11, 
  };

  // GR64_TC_with_sub_8bit Bit set.
  static const unsigned char GR64_TC_with_sub_8bitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0x72, 0x02, 
  };

  // GR64_NOREX_with_sub_8bit Register Class...
  static const unsigned GR64_NOREX_with_sub_8bit[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::RBX, X86::RBP, X86::RSP, 
  };

  // GR64_NOREX_with_sub_8bit Bit set.
  static const unsigned char GR64_NOREX_with_sub_8bitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x06, 
  };

  // RST Register Class...
  static const unsigned RST[] = {
    X86::ST0, X86::ST1, X86::ST2, X86::ST3, X86::ST4, X86::ST5, X86::ST6, X86::ST7, 
  };

  // RST Bit set.
  static const unsigned char RSTBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
  };

  // RFP32 Register Class...
  static const unsigned RFP32[] = {
    X86::FP0, X86::FP1, X86::FP2, X86::FP3, X86::FP4, X86::FP5, X86::FP6, 
  };

  // RFP32 Bit set.
  static const unsigned char RFP32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 
  };

  // GR32_NOREX_NOSP Register Class...
  static const unsigned GR32_NOREX_NOSP[] = {
    X86::EAX, X86::ECX, X86::EDX, X86::ESI, X86::EDI, X86::EBX, X86::EBP, 
  };

  // GR32_NOREX_NOSP Bit set.
  static const unsigned char GR32_NOREX_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x21, 
  };

  // RFP64 Register Class...
  static const unsigned RFP64[] = {
    X86::FP0, X86::FP1, X86::FP2, X86::FP3, X86::FP4, X86::FP5, X86::FP6, 
  };

  // RFP64 Bit set.
  static const unsigned char RFP64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 
  };

  // GR64_NOREX_NOSP Register Class...
  static const unsigned GR64_NOREX_NOSP[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RSI, X86::RDI, X86::RBX, X86::RBP, 
  };

  // GR64_NOREX_NOSP Bit set.
  static const unsigned char GR64_NOREX_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 
  };

  // RFP80 Register Class...
  static const unsigned RFP80[] = {
    X86::FP0, X86::FP1, X86::FP2, X86::FP3, X86::FP4, X86::FP5, X86::FP6, 
  };

  // RFP80 Bit set.
  static const unsigned char RFP80Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 
  };

  // SEGMENT_REG Register Class...
  static const unsigned SEGMENT_REG[] = {
    X86::CS, X86::DS, X86::SS, X86::ES, X86::FS, X86::GS, 
  };

  // SEGMENT_REG Bit set.
  static const unsigned char SEGMENT_REGBits[] = {
    0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x10, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
  };

  // GR64_TCW64 Register Class...
  static const unsigned GR64_TCW64[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::R8, X86::R9, X86::R11, 
  };

  // GR64_TCW64 Bit set.
  static const unsigned char GR64_TCW64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0x52, 
  };

  // GR8_ABCD_L Register Class...
  static const unsigned GR8_ABCD_L[] = {
    X86::AL, X86::CL, X86::DL, X86::BL, 
  };

  // GR8_ABCD_L Bit set.
  static const unsigned char GR8_ABCD_LBits[] = {
    0x24, 0x04, 0x00, 0x00, 0x01, 
  };

  // GR8_ABCD_H Register Class...
  static const unsigned GR8_ABCD_H[] = {
    X86::AH, X86::CH, X86::DH, X86::BH, 
  };

  // GR8_ABCD_H Bit set.
  static const unsigned char GR8_ABCD_HBits[] = {
    0x12, 0x02, 0x00, 0x20, 
  };

  // GR16_ABCD Register Class...
  static const unsigned GR16_ABCD[] = {
    X86::AX, X86::CX, X86::DX, X86::BX, 
  };

  // GR16_ABCD Bit set.
  static const unsigned char GR16_ABCDBits[] = {
    0x08, 0x01, 0x00, 0x10, 0x00, 0x04, 
  };

  // GR32_ABCD Register Class...
  static const unsigned GR32_ABCD[] = {
    X86::EAX, X86::ECX, X86::EDX, X86::EBX, 
  };

  // GR32_ABCD Bit set.
  static const unsigned char GR32_ABCDBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x01, 
  };

  // GR64_ABCD Register Class...
  static const unsigned GR64_ABCD[] = {
    X86::RAX, X86::RCX, X86::RDX, X86::RBX, 
  };

  // GR64_ABCD Bit set.
  static const unsigned char GR64_ABCDBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a, 
  };

  // GR32_TC Register Class...
  static const unsigned GR32_TC[] = {
    X86::EAX, X86::ECX, X86::EDX, 
  };

  // GR32_TC Bit set.
  static const unsigned char GR32_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x01, 
  };

  // GR32_NOAX_with_sub_8bit_hi Register Class...
  static const unsigned GR32_NOAX_with_sub_8bit_hi[] = {
    X86::ECX, X86::EDX, X86::EBX, 
  };

  // GR32_NOAX_with_sub_8bit_hi Bit set.
  static const unsigned char GR32_NOAX_with_sub_8bit_hiBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 
  };

  // GR64_TC_with_sub_8bit_hi Register Class...
  static const unsigned GR64_TC_with_sub_8bit_hi[] = {
    X86::RAX, X86::RCX, X86::RDX, 
  };

  // GR64_TC_with_sub_8bit_hi Bit set.
  static const unsigned char GR64_TC_with_sub_8bit_hiBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 
  };

  // GR32_AD Register Class...
  static const unsigned GR32_AD[] = {
    X86::EAX, X86::EDX, 
  };

  // GR32_AD Bit set.
  static const unsigned char GR32_ADBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x01, 
  };

  // CCR Register Class...
  static const unsigned CCR[] = {
    X86::EFLAGS, 
  };

  // CCR Bit set.
  static const unsigned char CCRBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 
  };

}

MCRegisterClass X86MCRegisterClasses[] = {
  MCRegisterClass(X86::GR8RegClassID, "GR8", 1, 1, 1, 1, GR8, GR8 + 20, GR8Bits, sizeof(GR8Bits)),
  MCRegisterClass(X86::GR64RegClassID, "GR64", 8, 8, 1, 1, GR64, GR64 + 17, GR64Bits, sizeof(GR64Bits)),
  MCRegisterClass(X86::GR16RegClassID, "GR16", 2, 2, 1, 1, GR16, GR16 + 16, GR16Bits, sizeof(GR16Bits)),
  MCRegisterClass(X86::GR32RegClassID, "GR32", 4, 4, 1, 1, GR32, GR32 + 16, GR32Bits, sizeof(GR32Bits)),
  MCRegisterClass(X86::FR32RegClassID, "FR32", 4, 4, 1, 1, FR32, FR32 + 16, FR32Bits, sizeof(FR32Bits)),
  MCRegisterClass(X86::GR64_with_sub_8bitRegClassID, "GR64_with_sub_8bit", 8, 8, 1, 1, GR64_with_sub_8bit, GR64_with_sub_8bit + 16, GR64_with_sub_8bitBits, sizeof(GR64_with_sub_8bitBits)),
  MCRegisterClass(X86::FR64RegClassID, "FR64", 8, 8, 1, 1, FR64, FR64 + 16, FR64Bits, sizeof(FR64Bits)),
  MCRegisterClass(X86::CONTROL_REGRegClassID, "CONTROL_REG", 8, 8, 1, 1, CONTROL_REG, CONTROL_REG + 16, CONTROL_REGBits, sizeof(CONTROL_REGBits)),
  MCRegisterClass(X86::VR128RegClassID, "VR128", 16, 16, 1, 1, VR128, VR128 + 16, VR128Bits, sizeof(VR128Bits)),
  MCRegisterClass(X86::VR256RegClassID, "VR256", 32, 32, 1, 1, VR256, VR256 + 16, VR256Bits, sizeof(VR256Bits)),
  MCRegisterClass(X86::GR32_NOSPRegClassID, "GR32_NOSP", 4, 4, 1, 1, GR32_NOSP, GR32_NOSP + 15, GR32_NOSPBits, sizeof(GR32_NOSPBits)),
  MCRegisterClass(X86::GR32_NOAXRegClassID, "GR32_NOAX", 4, 4, 1, 1, GR32_NOAX, GR32_NOAX + 15, GR32_NOAXBits, sizeof(GR32_NOAXBits)),
  MCRegisterClass(X86::GR64_NOSPRegClassID, "GR64_NOSP", 8, 8, 1, 1, GR64_NOSP, GR64_NOSP + 15, GR64_NOSPBits, sizeof(GR64_NOSPBits)),
  MCRegisterClass(X86::GR64_TCRegClassID, "GR64_TC", 8, 8, 1, 1, GR64_TC, GR64_TC + 9, GR64_TCBits, sizeof(GR64_TCBits)),
  MCRegisterClass(X86::GR64_NOREXRegClassID, "GR64_NOREX", 8, 8, 1, 1, GR64_NOREX, GR64_NOREX + 9, GR64_NOREXBits, sizeof(GR64_NOREXBits)),
  MCRegisterClass(X86::GR8_NOREXRegClassID, "GR8_NOREX", 1, 1, 1, 1, GR8_NOREX, GR8_NOREX + 8, GR8_NOREXBits, sizeof(GR8_NOREXBits)),
  MCRegisterClass(X86::GR16_NOREXRegClassID, "GR16_NOREX", 2, 2, 1, 1, GR16_NOREX, GR16_NOREX + 8, GR16_NOREXBits, sizeof(GR16_NOREXBits)),
  MCRegisterClass(X86::GR32_NOREXRegClassID, "GR32_NOREX", 4, 4, 1, 1, GR32_NOREX, GR32_NOREX + 8, GR32_NOREXBits, sizeof(GR32_NOREXBits)),
  MCRegisterClass(X86::DEBUG_REGRegClassID, "DEBUG_REG", 4, 4, 1, 1, DEBUG_REG, DEBUG_REG + 8, DEBUG_REGBits, sizeof(DEBUG_REGBits)),
  MCRegisterClass(X86::VR64RegClassID, "VR64", 8, 8, 1, 1, VR64, VR64 + 8, VR64Bits, sizeof(VR64Bits)),
  MCRegisterClass(X86::GR64_TC_with_sub_8bitRegClassID, "GR64_TC_with_sub_8bit", 8, 8, 1, 1, GR64_TC_with_sub_8bit, GR64_TC_with_sub_8bit + 8, GR64_TC_with_sub_8bitBits, sizeof(GR64_TC_with_sub_8bitBits)),
  MCRegisterClass(X86::GR64_NOREX_with_sub_8bitRegClassID, "GR64_NOREX_with_sub_8bit", 8, 8, 1, 1, GR64_NOREX_with_sub_8bit, GR64_NOREX_with_sub_8bit + 8, GR64_NOREX_with_sub_8bitBits, sizeof(GR64_NOREX_with_sub_8bitBits)),
  MCRegisterClass(X86::RSTRegClassID, "RST", 10, 4, 1, 0, RST, RST + 8, RSTBits, sizeof(RSTBits)),
  MCRegisterClass(X86::RFP32RegClassID, "RFP32", 4, 4, 1, 1, RFP32, RFP32 + 7, RFP32Bits, sizeof(RFP32Bits)),
  MCRegisterClass(X86::GR32_NOREX_NOSPRegClassID, "GR32_NOREX_NOSP", 4, 4, 1, 1, GR32_NOREX_NOSP, GR32_NOREX_NOSP + 7, GR32_NOREX_NOSPBits, sizeof(GR32_NOREX_NOSPBits)),
  MCRegisterClass(X86::RFP64RegClassID, "RFP64", 8, 4, 1, 1, RFP64, RFP64 + 7, RFP64Bits, sizeof(RFP64Bits)),
  MCRegisterClass(X86::GR64_NOREX_NOSPRegClassID, "GR64_NOREX_NOSP", 8, 8, 1, 1, GR64_NOREX_NOSP, GR64_NOREX_NOSP + 7, GR64_NOREX_NOSPBits, sizeof(GR64_NOREX_NOSPBits)),
  MCRegisterClass(X86::RFP80RegClassID, "RFP80", 10, 4, 1, 1, RFP80, RFP80 + 7, RFP80Bits, sizeof(RFP80Bits)),
  MCRegisterClass(X86::SEGMENT_REGRegClassID, "SEGMENT_REG", 2, 2, 1, 1, SEGMENT_REG, SEGMENT_REG + 6, SEGMENT_REGBits, sizeof(SEGMENT_REGBits)),
  MCRegisterClass(X86::GR64_TCW64RegClassID, "GR64_TCW64", 8, 8, 1, 1, GR64_TCW64, GR64_TCW64 + 6, GR64_TCW64Bits, sizeof(GR64_TCW64Bits)),
  MCRegisterClass(X86::GR8_ABCD_LRegClassID, "GR8_ABCD_L", 1, 1, 1, 1, GR8_ABCD_L, GR8_ABCD_L + 4, GR8_ABCD_LBits, sizeof(GR8_ABCD_LBits)),
  MCRegisterClass(X86::GR8_ABCD_HRegClassID, "GR8_ABCD_H", 1, 1, 1, 1, GR8_ABCD_H, GR8_ABCD_H + 4, GR8_ABCD_HBits, sizeof(GR8_ABCD_HBits)),
  MCRegisterClass(X86::GR16_ABCDRegClassID, "GR16_ABCD", 2, 2, 1, 1, GR16_ABCD, GR16_ABCD + 4, GR16_ABCDBits, sizeof(GR16_ABCDBits)),
  MCRegisterClass(X86::GR32_ABCDRegClassID, "GR32_ABCD", 4, 4, 1, 1, GR32_ABCD, GR32_ABCD + 4, GR32_ABCDBits, sizeof(GR32_ABCDBits)),
  MCRegisterClass(X86::GR64_ABCDRegClassID, "GR64_ABCD", 8, 8, 1, 1, GR64_ABCD, GR64_ABCD + 4, GR64_ABCDBits, sizeof(GR64_ABCDBits)),
  MCRegisterClass(X86::GR32_TCRegClassID, "GR32_TC", 4, 4, 1, 1, GR32_TC, GR32_TC + 3, GR32_TCBits, sizeof(GR32_TCBits)),
  MCRegisterClass(X86::GR32_NOAX_with_sub_8bit_hiRegClassID, "GR32_NOAX_with_sub_8bit_hi", 4, 4, 1, 1, GR32_NOAX_with_sub_8bit_hi, GR32_NOAX_with_sub_8bit_hi + 3, GR32_NOAX_with_sub_8bit_hiBits, sizeof(GR32_NOAX_with_sub_8bit_hiBits)),
  MCRegisterClass(X86::GR64_TC_with_sub_8bit_hiRegClassID, "GR64_TC_with_sub_8bit_hi", 8, 8, 1, 1, GR64_TC_with_sub_8bit_hi, GR64_TC_with_sub_8bit_hi + 3, GR64_TC_with_sub_8bit_hiBits, sizeof(GR64_TC_with_sub_8bit_hiBits)),
  MCRegisterClass(X86::GR32_ADRegClassID, "GR32_AD", 4, 4, 1, 1, GR32_AD, GR32_AD + 2, GR32_ADBits, sizeof(GR32_ADBits)),
  MCRegisterClass(X86::CCRRegClassID, "CCR", 4, 4, -1, 0, CCR, CCR + 1, CCRBits, sizeof(CCRBits)),
};

static inline void InitX86MCRegisterInfo(MCRegisterInfo *RI, unsigned RA, unsigned DwarfFlavour = 0, unsigned EHFlavour = 0) {
  RI->InitMCRegisterInfo(X86RegDesc, 160, RA, X86MCRegisterClasses, 40);

  switch (DwarfFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    RI->mapDwarfRegToLLVMReg(41, X86::MM0, false );
    RI->mapDwarfRegToLLVMReg(42, X86::MM1, false );
    RI->mapDwarfRegToLLVMReg(43, X86::MM2, false );
    RI->mapDwarfRegToLLVMReg(44, X86::MM3, false );
    RI->mapDwarfRegToLLVMReg(45, X86::MM4, false );
    RI->mapDwarfRegToLLVMReg(46, X86::MM5, false );
    RI->mapDwarfRegToLLVMReg(47, X86::MM6, false );
    RI->mapDwarfRegToLLVMReg(48, X86::MM7, false );
    RI->mapDwarfRegToLLVMReg(8, X86::R8, false );
    RI->mapDwarfRegToLLVMReg(9, X86::R9, false );
    RI->mapDwarfRegToLLVMReg(10, X86::R10, false );
    RI->mapDwarfRegToLLVMReg(11, X86::R11, false );
    RI->mapDwarfRegToLLVMReg(12, X86::R12, false );
    RI->mapDwarfRegToLLVMReg(13, X86::R13, false );
    RI->mapDwarfRegToLLVMReg(14, X86::R14, false );
    RI->mapDwarfRegToLLVMReg(15, X86::R15, false );
    RI->mapDwarfRegToLLVMReg(0, X86::RAX, false );
    RI->mapDwarfRegToLLVMReg(6, X86::RBP, false );
    RI->mapDwarfRegToLLVMReg(3, X86::RBX, false );
    RI->mapDwarfRegToLLVMReg(2, X86::RCX, false );
    RI->mapDwarfRegToLLVMReg(5, X86::RDI, false );
    RI->mapDwarfRegToLLVMReg(1, X86::RDX, false );
    RI->mapDwarfRegToLLVMReg(16, X86::RIP, false );
    RI->mapDwarfRegToLLVMReg(4, X86::RSI, false );
    RI->mapDwarfRegToLLVMReg(7, X86::RSP, false );
    RI->mapDwarfRegToLLVMReg(33, X86::ST0, false );
    RI->mapDwarfRegToLLVMReg(34, X86::ST1, false );
    RI->mapDwarfRegToLLVMReg(35, X86::ST2, false );
    RI->mapDwarfRegToLLVMReg(36, X86::ST3, false );
    RI->mapDwarfRegToLLVMReg(37, X86::ST4, false );
    RI->mapDwarfRegToLLVMReg(38, X86::ST5, false );
    RI->mapDwarfRegToLLVMReg(39, X86::ST6, false );
    RI->mapDwarfRegToLLVMReg(40, X86::ST7, false );
    RI->mapDwarfRegToLLVMReg(17, X86::XMM0, false );
    RI->mapDwarfRegToLLVMReg(18, X86::XMM1, false );
    RI->mapDwarfRegToLLVMReg(19, X86::XMM2, false );
    RI->mapDwarfRegToLLVMReg(20, X86::XMM3, false );
    RI->mapDwarfRegToLLVMReg(21, X86::XMM4, false );
    RI->mapDwarfRegToLLVMReg(22, X86::XMM5, false );
    RI->mapDwarfRegToLLVMReg(23, X86::XMM6, false );
    RI->mapDwarfRegToLLVMReg(24, X86::XMM7, false );
    RI->mapDwarfRegToLLVMReg(25, X86::XMM8, false );
    RI->mapDwarfRegToLLVMReg(26, X86::XMM9, false );
    RI->mapDwarfRegToLLVMReg(27, X86::XMM10, false );
    RI->mapDwarfRegToLLVMReg(28, X86::XMM11, false );
    RI->mapDwarfRegToLLVMReg(29, X86::XMM12, false );
    RI->mapDwarfRegToLLVMReg(30, X86::XMM13, false );
    RI->mapDwarfRegToLLVMReg(31, X86::XMM14, false );
    RI->mapDwarfRegToLLVMReg(32, X86::XMM15, false );
    break;
  case 1:
    RI->mapDwarfRegToLLVMReg(0, X86::EAX, false );
    RI->mapDwarfRegToLLVMReg(4, X86::EBP, false );
    RI->mapDwarfRegToLLVMReg(3, X86::EBX, false );
    RI->mapDwarfRegToLLVMReg(1, X86::ECX, false );
    RI->mapDwarfRegToLLVMReg(7, X86::EDI, false );
    RI->mapDwarfRegToLLVMReg(2, X86::EDX, false );
    RI->mapDwarfRegToLLVMReg(8, X86::EIP, false );
    RI->mapDwarfRegToLLVMReg(6, X86::ESI, false );
    RI->mapDwarfRegToLLVMReg(5, X86::ESP, false );
    RI->mapDwarfRegToLLVMReg(29, X86::MM0, false );
    RI->mapDwarfRegToLLVMReg(30, X86::MM1, false );
    RI->mapDwarfRegToLLVMReg(31, X86::MM2, false );
    RI->mapDwarfRegToLLVMReg(32, X86::MM3, false );
    RI->mapDwarfRegToLLVMReg(33, X86::MM4, false );
    RI->mapDwarfRegToLLVMReg(34, X86::MM5, false );
    RI->mapDwarfRegToLLVMReg(35, X86::MM6, false );
    RI->mapDwarfRegToLLVMReg(36, X86::MM7, false );
    RI->mapDwarfRegToLLVMReg(12, X86::ST0, false );
    RI->mapDwarfRegToLLVMReg(13, X86::ST1, false );
    RI->mapDwarfRegToLLVMReg(14, X86::ST2, false );
    RI->mapDwarfRegToLLVMReg(15, X86::ST3, false );
    RI->mapDwarfRegToLLVMReg(16, X86::ST4, false );
    RI->mapDwarfRegToLLVMReg(17, X86::ST5, false );
    RI->mapDwarfRegToLLVMReg(18, X86::ST6, false );
    RI->mapDwarfRegToLLVMReg(19, X86::ST7, false );
    RI->mapDwarfRegToLLVMReg(21, X86::XMM0, false );
    RI->mapDwarfRegToLLVMReg(22, X86::XMM1, false );
    RI->mapDwarfRegToLLVMReg(23, X86::XMM2, false );
    RI->mapDwarfRegToLLVMReg(24, X86::XMM3, false );
    RI->mapDwarfRegToLLVMReg(25, X86::XMM4, false );
    RI->mapDwarfRegToLLVMReg(26, X86::XMM5, false );
    RI->mapDwarfRegToLLVMReg(27, X86::XMM6, false );
    RI->mapDwarfRegToLLVMReg(28, X86::XMM7, false );
    break;
  case 2:
    RI->mapDwarfRegToLLVMReg(0, X86::EAX, false );
    RI->mapDwarfRegToLLVMReg(5, X86::EBP, false );
    RI->mapDwarfRegToLLVMReg(3, X86::EBX, false );
    RI->mapDwarfRegToLLVMReg(1, X86::ECX, false );
    RI->mapDwarfRegToLLVMReg(7, X86::EDI, false );
    RI->mapDwarfRegToLLVMReg(2, X86::EDX, false );
    RI->mapDwarfRegToLLVMReg(8, X86::EIP, false );
    RI->mapDwarfRegToLLVMReg(6, X86::ESI, false );
    RI->mapDwarfRegToLLVMReg(4, X86::ESP, false );
    RI->mapDwarfRegToLLVMReg(29, X86::MM0, false );
    RI->mapDwarfRegToLLVMReg(30, X86::MM1, false );
    RI->mapDwarfRegToLLVMReg(31, X86::MM2, false );
    RI->mapDwarfRegToLLVMReg(32, X86::MM3, false );
    RI->mapDwarfRegToLLVMReg(33, X86::MM4, false );
    RI->mapDwarfRegToLLVMReg(34, X86::MM5, false );
    RI->mapDwarfRegToLLVMReg(35, X86::MM6, false );
    RI->mapDwarfRegToLLVMReg(36, X86::MM7, false );
    RI->mapDwarfRegToLLVMReg(11, X86::ST0, false );
    RI->mapDwarfRegToLLVMReg(12, X86::ST1, false );
    RI->mapDwarfRegToLLVMReg(13, X86::ST2, false );
    RI->mapDwarfRegToLLVMReg(14, X86::ST3, false );
    RI->mapDwarfRegToLLVMReg(15, X86::ST4, false );
    RI->mapDwarfRegToLLVMReg(16, X86::ST5, false );
    RI->mapDwarfRegToLLVMReg(17, X86::ST6, false );
    RI->mapDwarfRegToLLVMReg(18, X86::ST7, false );
    RI->mapDwarfRegToLLVMReg(21, X86::XMM0, false );
    RI->mapDwarfRegToLLVMReg(22, X86::XMM1, false );
    RI->mapDwarfRegToLLVMReg(23, X86::XMM2, false );
    RI->mapDwarfRegToLLVMReg(24, X86::XMM3, false );
    RI->mapDwarfRegToLLVMReg(25, X86::XMM4, false );
    RI->mapDwarfRegToLLVMReg(26, X86::XMM5, false );
    RI->mapDwarfRegToLLVMReg(27, X86::XMM6, false );
    RI->mapDwarfRegToLLVMReg(28, X86::XMM7, false );
    break;
  }
  switch (EHFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    RI->mapDwarfRegToLLVMReg(41, X86::MM0, true );
    RI->mapDwarfRegToLLVMReg(42, X86::MM1, true );
    RI->mapDwarfRegToLLVMReg(43, X86::MM2, true );
    RI->mapDwarfRegToLLVMReg(44, X86::MM3, true );
    RI->mapDwarfRegToLLVMReg(45, X86::MM4, true );
    RI->mapDwarfRegToLLVMReg(46, X86::MM5, true );
    RI->mapDwarfRegToLLVMReg(47, X86::MM6, true );
    RI->mapDwarfRegToLLVMReg(48, X86::MM7, true );
    RI->mapDwarfRegToLLVMReg(8, X86::R8, true );
    RI->mapDwarfRegToLLVMReg(9, X86::R9, true );
    RI->mapDwarfRegToLLVMReg(10, X86::R10, true );
    RI->mapDwarfRegToLLVMReg(11, X86::R11, true );
    RI->mapDwarfRegToLLVMReg(12, X86::R12, true );
    RI->mapDwarfRegToLLVMReg(13, X86::R13, true );
    RI->mapDwarfRegToLLVMReg(14, X86::R14, true );
    RI->mapDwarfRegToLLVMReg(15, X86::R15, true );
    RI->mapDwarfRegToLLVMReg(0, X86::RAX, true );
    RI->mapDwarfRegToLLVMReg(6, X86::RBP, true );
    RI->mapDwarfRegToLLVMReg(3, X86::RBX, true );
    RI->mapDwarfRegToLLVMReg(2, X86::RCX, true );
    RI->mapDwarfRegToLLVMReg(5, X86::RDI, true );
    RI->mapDwarfRegToLLVMReg(1, X86::RDX, true );
    RI->mapDwarfRegToLLVMReg(16, X86::RIP, true );
    RI->mapDwarfRegToLLVMReg(4, X86::RSI, true );
    RI->mapDwarfRegToLLVMReg(7, X86::RSP, true );
    RI->mapDwarfRegToLLVMReg(33, X86::ST0, true );
    RI->mapDwarfRegToLLVMReg(34, X86::ST1, true );
    RI->mapDwarfRegToLLVMReg(35, X86::ST2, true );
    RI->mapDwarfRegToLLVMReg(36, X86::ST3, true );
    RI->mapDwarfRegToLLVMReg(37, X86::ST4, true );
    RI->mapDwarfRegToLLVMReg(38, X86::ST5, true );
    RI->mapDwarfRegToLLVMReg(39, X86::ST6, true );
    RI->mapDwarfRegToLLVMReg(40, X86::ST7, true );
    RI->mapDwarfRegToLLVMReg(17, X86::XMM0, true );
    RI->mapDwarfRegToLLVMReg(18, X86::XMM1, true );
    RI->mapDwarfRegToLLVMReg(19, X86::XMM2, true );
    RI->mapDwarfRegToLLVMReg(20, X86::XMM3, true );
    RI->mapDwarfRegToLLVMReg(21, X86::XMM4, true );
    RI->mapDwarfRegToLLVMReg(22, X86::XMM5, true );
    RI->mapDwarfRegToLLVMReg(23, X86::XMM6, true );
    RI->mapDwarfRegToLLVMReg(24, X86::XMM7, true );
    RI->mapDwarfRegToLLVMReg(25, X86::XMM8, true );
    RI->mapDwarfRegToLLVMReg(26, X86::XMM9, true );
    RI->mapDwarfRegToLLVMReg(27, X86::XMM10, true );
    RI->mapDwarfRegToLLVMReg(28, X86::XMM11, true );
    RI->mapDwarfRegToLLVMReg(29, X86::XMM12, true );
    RI->mapDwarfRegToLLVMReg(30, X86::XMM13, true );
    RI->mapDwarfRegToLLVMReg(31, X86::XMM14, true );
    RI->mapDwarfRegToLLVMReg(32, X86::XMM15, true );
    break;
  case 1:
    RI->mapDwarfRegToLLVMReg(0, X86::EAX, true );
    RI->mapDwarfRegToLLVMReg(4, X86::EBP, true );
    RI->mapDwarfRegToLLVMReg(3, X86::EBX, true );
    RI->mapDwarfRegToLLVMReg(1, X86::ECX, true );
    RI->mapDwarfRegToLLVMReg(7, X86::EDI, true );
    RI->mapDwarfRegToLLVMReg(2, X86::EDX, true );
    RI->mapDwarfRegToLLVMReg(8, X86::EIP, true );
    RI->mapDwarfRegToLLVMReg(6, X86::ESI, true );
    RI->mapDwarfRegToLLVMReg(5, X86::ESP, true );
    RI->mapDwarfRegToLLVMReg(29, X86::MM0, true );
    RI->mapDwarfRegToLLVMReg(30, X86::MM1, true );
    RI->mapDwarfRegToLLVMReg(31, X86::MM2, true );
    RI->mapDwarfRegToLLVMReg(32, X86::MM3, true );
    RI->mapDwarfRegToLLVMReg(33, X86::MM4, true );
    RI->mapDwarfRegToLLVMReg(34, X86::MM5, true );
    RI->mapDwarfRegToLLVMReg(35, X86::MM6, true );
    RI->mapDwarfRegToLLVMReg(36, X86::MM7, true );
    RI->mapDwarfRegToLLVMReg(12, X86::ST0, true );
    RI->mapDwarfRegToLLVMReg(13, X86::ST1, true );
    RI->mapDwarfRegToLLVMReg(14, X86::ST2, true );
    RI->mapDwarfRegToLLVMReg(15, X86::ST3, true );
    RI->mapDwarfRegToLLVMReg(16, X86::ST4, true );
    RI->mapDwarfRegToLLVMReg(17, X86::ST5, true );
    RI->mapDwarfRegToLLVMReg(18, X86::ST6, true );
    RI->mapDwarfRegToLLVMReg(19, X86::ST7, true );
    RI->mapDwarfRegToLLVMReg(21, X86::XMM0, true );
    RI->mapDwarfRegToLLVMReg(22, X86::XMM1, true );
    RI->mapDwarfRegToLLVMReg(23, X86::XMM2, true );
    RI->mapDwarfRegToLLVMReg(24, X86::XMM3, true );
    RI->mapDwarfRegToLLVMReg(25, X86::XMM4, true );
    RI->mapDwarfRegToLLVMReg(26, X86::XMM5, true );
    RI->mapDwarfRegToLLVMReg(27, X86::XMM6, true );
    RI->mapDwarfRegToLLVMReg(28, X86::XMM7, true );
    break;
  case 2:
    RI->mapDwarfRegToLLVMReg(0, X86::EAX, true );
    RI->mapDwarfRegToLLVMReg(5, X86::EBP, true );
    RI->mapDwarfRegToLLVMReg(3, X86::EBX, true );
    RI->mapDwarfRegToLLVMReg(1, X86::ECX, true );
    RI->mapDwarfRegToLLVMReg(7, X86::EDI, true );
    RI->mapDwarfRegToLLVMReg(2, X86::EDX, true );
    RI->mapDwarfRegToLLVMReg(8, X86::EIP, true );
    RI->mapDwarfRegToLLVMReg(6, X86::ESI, true );
    RI->mapDwarfRegToLLVMReg(4, X86::ESP, true );
    RI->mapDwarfRegToLLVMReg(29, X86::MM0, true );
    RI->mapDwarfRegToLLVMReg(30, X86::MM1, true );
    RI->mapDwarfRegToLLVMReg(31, X86::MM2, true );
    RI->mapDwarfRegToLLVMReg(32, X86::MM3, true );
    RI->mapDwarfRegToLLVMReg(33, X86::MM4, true );
    RI->mapDwarfRegToLLVMReg(34, X86::MM5, true );
    RI->mapDwarfRegToLLVMReg(35, X86::MM6, true );
    RI->mapDwarfRegToLLVMReg(36, X86::MM7, true );
    RI->mapDwarfRegToLLVMReg(11, X86::ST0, true );
    RI->mapDwarfRegToLLVMReg(12, X86::ST1, true );
    RI->mapDwarfRegToLLVMReg(13, X86::ST2, true );
    RI->mapDwarfRegToLLVMReg(14, X86::ST3, true );
    RI->mapDwarfRegToLLVMReg(15, X86::ST4, true );
    RI->mapDwarfRegToLLVMReg(16, X86::ST5, true );
    RI->mapDwarfRegToLLVMReg(17, X86::ST6, true );
    RI->mapDwarfRegToLLVMReg(18, X86::ST7, true );
    RI->mapDwarfRegToLLVMReg(21, X86::XMM0, true );
    RI->mapDwarfRegToLLVMReg(22, X86::XMM1, true );
    RI->mapDwarfRegToLLVMReg(23, X86::XMM2, true );
    RI->mapDwarfRegToLLVMReg(24, X86::XMM3, true );
    RI->mapDwarfRegToLLVMReg(25, X86::XMM4, true );
    RI->mapDwarfRegToLLVMReg(26, X86::XMM5, true );
    RI->mapDwarfRegToLLVMReg(27, X86::XMM6, true );
    RI->mapDwarfRegToLLVMReg(28, X86::XMM7, true );
    break;
  }
  switch (DwarfFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    RI->mapLLVMRegToDwarfReg(X86::AH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::AL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::AX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BPL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR7, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR8, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR9, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR10, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR11, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR12, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR13, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR14, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR15, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DI, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DIL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR7, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::EAX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::EBP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::EBX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::ECX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::EDI, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::EDX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::EFLAGS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::EIP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::EIZ, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ES, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ESI, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::ESP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::FP0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::GS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::IP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::MM0, 41, false );
    RI->mapLLVMRegToDwarfReg(X86::MM1, 42, false );
    RI->mapLLVMRegToDwarfReg(X86::MM2, 43, false );
    RI->mapLLVMRegToDwarfReg(X86::MM3, 44, false );
    RI->mapLLVMRegToDwarfReg(X86::MM4, 45, false );
    RI->mapLLVMRegToDwarfReg(X86::MM5, 46, false );
    RI->mapLLVMRegToDwarfReg(X86::MM6, 47, false );
    RI->mapLLVMRegToDwarfReg(X86::MM7, 48, false );
    RI->mapLLVMRegToDwarfReg(X86::R8, 8, false );
    RI->mapLLVMRegToDwarfReg(X86::R8B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R8D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R8W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9, 9, false );
    RI->mapLLVMRegToDwarfReg(X86::R9B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10, 10, false );
    RI->mapLLVMRegToDwarfReg(X86::R10B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11, 11, false );
    RI->mapLLVMRegToDwarfReg(X86::R11B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12, 12, false );
    RI->mapLLVMRegToDwarfReg(X86::R12B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13, 13, false );
    RI->mapLLVMRegToDwarfReg(X86::R13B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14, 14, false );
    RI->mapLLVMRegToDwarfReg(X86::R14B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15, 15, false );
    RI->mapLLVMRegToDwarfReg(X86::R15B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::RAX, 0, false );
    RI->mapLLVMRegToDwarfReg(X86::RBP, 6, false );
    RI->mapLLVMRegToDwarfReg(X86::RBX, 3, false );
    RI->mapLLVMRegToDwarfReg(X86::RCX, 2, false );
    RI->mapLLVMRegToDwarfReg(X86::RDI, 5, false );
    RI->mapLLVMRegToDwarfReg(X86::RDX, 1, false );
    RI->mapLLVMRegToDwarfReg(X86::RIP, 16, false );
    RI->mapLLVMRegToDwarfReg(X86::RIZ, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::RSI, 4, false );
    RI->mapLLVMRegToDwarfReg(X86::RSP, 7, false );
    RI->mapLLVMRegToDwarfReg(X86::SI, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SIL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SPL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ST0, 33, false );
    RI->mapLLVMRegToDwarfReg(X86::ST1, 34, false );
    RI->mapLLVMRegToDwarfReg(X86::ST2, 35, false );
    RI->mapLLVMRegToDwarfReg(X86::ST3, 36, false );
    RI->mapLLVMRegToDwarfReg(X86::ST4, 37, false );
    RI->mapLLVMRegToDwarfReg(X86::ST5, 38, false );
    RI->mapLLVMRegToDwarfReg(X86::ST6, 39, false );
    RI->mapLLVMRegToDwarfReg(X86::ST7, 40, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM0, 17, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM1, 18, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM2, 19, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM3, 20, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM4, 21, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM5, 22, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM6, 23, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM7, 24, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM8, 25, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM9, 26, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM10, 27, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM11, 28, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM12, 29, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM13, 30, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM14, 31, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM15, 32, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM0, 17, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM1, 18, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM2, 19, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM3, 20, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM4, 21, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM5, 22, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM6, 23, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM7, 24, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM8, 25, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM9, 26, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM10, 27, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM11, 28, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM12, 29, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM13, 30, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM14, 31, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM15, 32, false );
    break;
  case 1:
    RI->mapLLVMRegToDwarfReg(X86::AH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::AL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::AX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BPL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR7, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR8, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR9, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR10, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR11, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR12, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR13, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR14, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR15, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DI, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DIL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR7, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::EAX, 0, false );
    RI->mapLLVMRegToDwarfReg(X86::EBP, 4, false );
    RI->mapLLVMRegToDwarfReg(X86::EBX, 3, false );
    RI->mapLLVMRegToDwarfReg(X86::ECX, 1, false );
    RI->mapLLVMRegToDwarfReg(X86::EDI, 7, false );
    RI->mapLLVMRegToDwarfReg(X86::EDX, 2, false );
    RI->mapLLVMRegToDwarfReg(X86::EFLAGS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::EIP, 8, false );
    RI->mapLLVMRegToDwarfReg(X86::EIZ, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ES, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ESI, 6, false );
    RI->mapLLVMRegToDwarfReg(X86::ESP, 5, false );
    RI->mapLLVMRegToDwarfReg(X86::FP0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::GS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::IP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::MM0, 29, false );
    RI->mapLLVMRegToDwarfReg(X86::MM1, 30, false );
    RI->mapLLVMRegToDwarfReg(X86::MM2, 31, false );
    RI->mapLLVMRegToDwarfReg(X86::MM3, 32, false );
    RI->mapLLVMRegToDwarfReg(X86::MM4, 33, false );
    RI->mapLLVMRegToDwarfReg(X86::MM5, 34, false );
    RI->mapLLVMRegToDwarfReg(X86::MM6, 35, false );
    RI->mapLLVMRegToDwarfReg(X86::MM7, 36, false );
    RI->mapLLVMRegToDwarfReg(X86::R8, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R8B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R8D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R8W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R9B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R10B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R11B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R12B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R13B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R14B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R15B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::RAX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RBP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RBX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RCX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RDI, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RDX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RIP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RIZ, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::RSI, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RSP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::SI, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SIL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SPL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ST0, 12, false );
    RI->mapLLVMRegToDwarfReg(X86::ST1, 13, false );
    RI->mapLLVMRegToDwarfReg(X86::ST2, 14, false );
    RI->mapLLVMRegToDwarfReg(X86::ST3, 15, false );
    RI->mapLLVMRegToDwarfReg(X86::ST4, 16, false );
    RI->mapLLVMRegToDwarfReg(X86::ST5, 17, false );
    RI->mapLLVMRegToDwarfReg(X86::ST6, 18, false );
    RI->mapLLVMRegToDwarfReg(X86::ST7, 19, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM0, 21, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM1, 22, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM2, 23, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM3, 24, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM4, 25, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM5, 26, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM6, 27, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM7, 28, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM8, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM9, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM10, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM11, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM12, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM13, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM14, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM15, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM0, 21, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM1, 22, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM2, 23, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM3, 24, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM4, 25, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM5, 26, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM6, 27, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM7, 28, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM8, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM9, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM10, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM11, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM12, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM13, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM14, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM15, -2, false );
    break;
  case 2:
    RI->mapLLVMRegToDwarfReg(X86::AH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::AL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::AX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BPL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::BX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR7, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR8, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR9, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR10, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR11, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR12, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR13, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR14, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CR15, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::CX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DH, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DI, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DIL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DR7, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::DX, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::EAX, 0, false );
    RI->mapLLVMRegToDwarfReg(X86::EBP, 5, false );
    RI->mapLLVMRegToDwarfReg(X86::EBX, 3, false );
    RI->mapLLVMRegToDwarfReg(X86::ECX, 1, false );
    RI->mapLLVMRegToDwarfReg(X86::EDI, 7, false );
    RI->mapLLVMRegToDwarfReg(X86::EDX, 2, false );
    RI->mapLLVMRegToDwarfReg(X86::EFLAGS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::EIP, 8, false );
    RI->mapLLVMRegToDwarfReg(X86::EIZ, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ES, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ESI, 6, false );
    RI->mapLLVMRegToDwarfReg(X86::ESP, 4, false );
    RI->mapLLVMRegToDwarfReg(X86::FP0, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP1, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP2, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP3, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP4, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP5, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FP6, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::FS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::GS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::IP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::MM0, 29, false );
    RI->mapLLVMRegToDwarfReg(X86::MM1, 30, false );
    RI->mapLLVMRegToDwarfReg(X86::MM2, 31, false );
    RI->mapLLVMRegToDwarfReg(X86::MM3, 32, false );
    RI->mapLLVMRegToDwarfReg(X86::MM4, 33, false );
    RI->mapLLVMRegToDwarfReg(X86::MM5, 34, false );
    RI->mapLLVMRegToDwarfReg(X86::MM6, 35, false );
    RI->mapLLVMRegToDwarfReg(X86::MM7, 36, false );
    RI->mapLLVMRegToDwarfReg(X86::R8, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R8B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R8D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R8W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R9B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R9W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R10B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R10W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R11B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R11W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R12B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R12W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R13B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R13W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R14B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R14W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::R15B, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15D, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::R15W, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::RAX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RBP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RBX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RCX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RDI, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RDX, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RIP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RIZ, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::RSI, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::RSP, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::SI, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SIL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SP, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SPL, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::SS, -1, false );
    RI->mapLLVMRegToDwarfReg(X86::ST0, 11, false );
    RI->mapLLVMRegToDwarfReg(X86::ST1, 12, false );
    RI->mapLLVMRegToDwarfReg(X86::ST2, 13, false );
    RI->mapLLVMRegToDwarfReg(X86::ST3, 14, false );
    RI->mapLLVMRegToDwarfReg(X86::ST4, 15, false );
    RI->mapLLVMRegToDwarfReg(X86::ST5, 16, false );
    RI->mapLLVMRegToDwarfReg(X86::ST6, 17, false );
    RI->mapLLVMRegToDwarfReg(X86::ST7, 18, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM0, 21, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM1, 22, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM2, 23, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM3, 24, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM4, 25, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM5, 26, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM6, 27, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM7, 28, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM8, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM9, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM10, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM11, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM12, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM13, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM14, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::XMM15, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM0, 21, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM1, 22, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM2, 23, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM3, 24, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM4, 25, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM5, 26, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM6, 27, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM7, 28, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM8, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM9, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM10, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM11, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM12, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM13, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM14, -2, false );
    RI->mapLLVMRegToDwarfReg(X86::YMM15, -2, false );
    break;
  }
  switch (EHFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    RI->mapLLVMRegToDwarfReg(X86::AH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::AL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::AX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BPL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR7, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR8, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR9, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR10, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR11, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR12, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR13, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR14, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR15, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DI, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DIL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR7, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::EAX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::EBP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::EBX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::ECX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::EDI, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::EDX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::EFLAGS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::EIP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::EIZ, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ES, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ESI, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::ESP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::FP0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::GS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::IP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::MM0, 41, true );
    RI->mapLLVMRegToDwarfReg(X86::MM1, 42, true );
    RI->mapLLVMRegToDwarfReg(X86::MM2, 43, true );
    RI->mapLLVMRegToDwarfReg(X86::MM3, 44, true );
    RI->mapLLVMRegToDwarfReg(X86::MM4, 45, true );
    RI->mapLLVMRegToDwarfReg(X86::MM5, 46, true );
    RI->mapLLVMRegToDwarfReg(X86::MM6, 47, true );
    RI->mapLLVMRegToDwarfReg(X86::MM7, 48, true );
    RI->mapLLVMRegToDwarfReg(X86::R8, 8, true );
    RI->mapLLVMRegToDwarfReg(X86::R8B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R8D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R8W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9, 9, true );
    RI->mapLLVMRegToDwarfReg(X86::R9B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10, 10, true );
    RI->mapLLVMRegToDwarfReg(X86::R10B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11, 11, true );
    RI->mapLLVMRegToDwarfReg(X86::R11B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12, 12, true );
    RI->mapLLVMRegToDwarfReg(X86::R12B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13, 13, true );
    RI->mapLLVMRegToDwarfReg(X86::R13B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14, 14, true );
    RI->mapLLVMRegToDwarfReg(X86::R14B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15, 15, true );
    RI->mapLLVMRegToDwarfReg(X86::R15B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::RAX, 0, true );
    RI->mapLLVMRegToDwarfReg(X86::RBP, 6, true );
    RI->mapLLVMRegToDwarfReg(X86::RBX, 3, true );
    RI->mapLLVMRegToDwarfReg(X86::RCX, 2, true );
    RI->mapLLVMRegToDwarfReg(X86::RDI, 5, true );
    RI->mapLLVMRegToDwarfReg(X86::RDX, 1, true );
    RI->mapLLVMRegToDwarfReg(X86::RIP, 16, true );
    RI->mapLLVMRegToDwarfReg(X86::RIZ, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::RSI, 4, true );
    RI->mapLLVMRegToDwarfReg(X86::RSP, 7, true );
    RI->mapLLVMRegToDwarfReg(X86::SI, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SIL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SPL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ST0, 33, true );
    RI->mapLLVMRegToDwarfReg(X86::ST1, 34, true );
    RI->mapLLVMRegToDwarfReg(X86::ST2, 35, true );
    RI->mapLLVMRegToDwarfReg(X86::ST3, 36, true );
    RI->mapLLVMRegToDwarfReg(X86::ST4, 37, true );
    RI->mapLLVMRegToDwarfReg(X86::ST5, 38, true );
    RI->mapLLVMRegToDwarfReg(X86::ST6, 39, true );
    RI->mapLLVMRegToDwarfReg(X86::ST7, 40, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM0, 17, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM1, 18, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM2, 19, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM3, 20, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM4, 21, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM5, 22, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM6, 23, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM7, 24, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM8, 25, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM9, 26, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM10, 27, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM11, 28, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM12, 29, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM13, 30, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM14, 31, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM15, 32, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM0, 17, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM1, 18, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM2, 19, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM3, 20, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM4, 21, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM5, 22, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM6, 23, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM7, 24, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM8, 25, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM9, 26, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM10, 27, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM11, 28, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM12, 29, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM13, 30, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM14, 31, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM15, 32, true );
    break;
  case 1:
    RI->mapLLVMRegToDwarfReg(X86::AH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::AL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::AX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BPL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR7, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR8, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR9, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR10, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR11, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR12, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR13, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR14, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR15, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DI, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DIL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR7, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::EAX, 0, true );
    RI->mapLLVMRegToDwarfReg(X86::EBP, 4, true );
    RI->mapLLVMRegToDwarfReg(X86::EBX, 3, true );
    RI->mapLLVMRegToDwarfReg(X86::ECX, 1, true );
    RI->mapLLVMRegToDwarfReg(X86::EDI, 7, true );
    RI->mapLLVMRegToDwarfReg(X86::EDX, 2, true );
    RI->mapLLVMRegToDwarfReg(X86::EFLAGS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::EIP, 8, true );
    RI->mapLLVMRegToDwarfReg(X86::EIZ, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ES, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ESI, 6, true );
    RI->mapLLVMRegToDwarfReg(X86::ESP, 5, true );
    RI->mapLLVMRegToDwarfReg(X86::FP0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::GS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::IP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::MM0, 29, true );
    RI->mapLLVMRegToDwarfReg(X86::MM1, 30, true );
    RI->mapLLVMRegToDwarfReg(X86::MM2, 31, true );
    RI->mapLLVMRegToDwarfReg(X86::MM3, 32, true );
    RI->mapLLVMRegToDwarfReg(X86::MM4, 33, true );
    RI->mapLLVMRegToDwarfReg(X86::MM5, 34, true );
    RI->mapLLVMRegToDwarfReg(X86::MM6, 35, true );
    RI->mapLLVMRegToDwarfReg(X86::MM7, 36, true );
    RI->mapLLVMRegToDwarfReg(X86::R8, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R8B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R8D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R8W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R9B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R10B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R11B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R12B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R13B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R14B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R15B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::RAX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RBP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RBX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RCX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RDI, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RDX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RIP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RIZ, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::RSI, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RSP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::SI, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SIL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SPL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ST0, 12, true );
    RI->mapLLVMRegToDwarfReg(X86::ST1, 13, true );
    RI->mapLLVMRegToDwarfReg(X86::ST2, 14, true );
    RI->mapLLVMRegToDwarfReg(X86::ST3, 15, true );
    RI->mapLLVMRegToDwarfReg(X86::ST4, 16, true );
    RI->mapLLVMRegToDwarfReg(X86::ST5, 17, true );
    RI->mapLLVMRegToDwarfReg(X86::ST6, 18, true );
    RI->mapLLVMRegToDwarfReg(X86::ST7, 19, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM0, 21, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM1, 22, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM2, 23, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM3, 24, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM4, 25, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM5, 26, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM6, 27, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM7, 28, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM8, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM9, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM10, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM11, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM12, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM13, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM14, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM15, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM0, 21, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM1, 22, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM2, 23, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM3, 24, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM4, 25, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM5, 26, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM6, 27, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM7, 28, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM8, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM9, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM10, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM11, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM12, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM13, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM14, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM15, -2, true );
    break;
  case 2:
    RI->mapLLVMRegToDwarfReg(X86::AH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::AL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::AX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BPL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::BX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR7, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR8, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR9, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR10, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR11, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR12, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR13, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR14, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CR15, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::CX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DH, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DI, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DIL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DR7, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::DX, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::EAX, 0, true );
    RI->mapLLVMRegToDwarfReg(X86::EBP, 5, true );
    RI->mapLLVMRegToDwarfReg(X86::EBX, 3, true );
    RI->mapLLVMRegToDwarfReg(X86::ECX, 1, true );
    RI->mapLLVMRegToDwarfReg(X86::EDI, 7, true );
    RI->mapLLVMRegToDwarfReg(X86::EDX, 2, true );
    RI->mapLLVMRegToDwarfReg(X86::EFLAGS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::EIP, 8, true );
    RI->mapLLVMRegToDwarfReg(X86::EIZ, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ES, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ESI, 6, true );
    RI->mapLLVMRegToDwarfReg(X86::ESP, 4, true );
    RI->mapLLVMRegToDwarfReg(X86::FP0, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP1, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP2, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP3, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP4, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP5, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FP6, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::FS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::GS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::IP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::MM0, 29, true );
    RI->mapLLVMRegToDwarfReg(X86::MM1, 30, true );
    RI->mapLLVMRegToDwarfReg(X86::MM2, 31, true );
    RI->mapLLVMRegToDwarfReg(X86::MM3, 32, true );
    RI->mapLLVMRegToDwarfReg(X86::MM4, 33, true );
    RI->mapLLVMRegToDwarfReg(X86::MM5, 34, true );
    RI->mapLLVMRegToDwarfReg(X86::MM6, 35, true );
    RI->mapLLVMRegToDwarfReg(X86::MM7, 36, true );
    RI->mapLLVMRegToDwarfReg(X86::R8, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R8B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R8D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R8W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R9B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R9W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R10B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R10W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R11B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R11W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R12B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R12W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R13B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R13W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R14B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R14W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::R15B, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15D, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::R15W, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::RAX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RBP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RBX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RCX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RDI, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RDX, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RIP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RIZ, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::RSI, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::RSP, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::SI, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SIL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SP, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SPL, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::SS, -1, true );
    RI->mapLLVMRegToDwarfReg(X86::ST0, 11, true );
    RI->mapLLVMRegToDwarfReg(X86::ST1, 12, true );
    RI->mapLLVMRegToDwarfReg(X86::ST2, 13, true );
    RI->mapLLVMRegToDwarfReg(X86::ST3, 14, true );
    RI->mapLLVMRegToDwarfReg(X86::ST4, 15, true );
    RI->mapLLVMRegToDwarfReg(X86::ST5, 16, true );
    RI->mapLLVMRegToDwarfReg(X86::ST6, 17, true );
    RI->mapLLVMRegToDwarfReg(X86::ST7, 18, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM0, 21, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM1, 22, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM2, 23, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM3, 24, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM4, 25, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM5, 26, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM6, 27, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM7, 28, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM8, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM9, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM10, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM11, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM12, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM13, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM14, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::XMM15, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM0, 21, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM1, 22, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM2, 23, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM3, 24, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM4, 25, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM5, 26, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM6, 27, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM7, 28, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM8, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM9, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM10, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM11, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM12, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM13, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM14, -2, true );
    RI->mapLLVMRegToDwarfReg(X86::YMM15, -2, true );
    break;
  }
}

} // End llvm namespace 
#endif // GET_REGINFO_MC_DESC

//===- TableGen'erated file -------------------------------------*- C++ -*-===//
//
// Register Information Header Fragment
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//


#ifdef GET_REGINFO_HEADER
#undef GET_REGINFO_HEADER
#include "llvm/Target/TargetRegisterInfo.h"
#include <string>

namespace llvm {

struct X86GenRegisterInfo : public TargetRegisterInfo {
  explicit X86GenRegisterInfo(unsigned RA, unsigned D = 0, unsigned E = 0);
  virtual bool needsStackRealignment(const MachineFunction &) const
     { return false; }
  unsigned getSubReg(unsigned RegNo, unsigned Index) const;
  unsigned getSubRegIndex(unsigned RegNo, unsigned SubRegNo) const;
  unsigned composeSubRegIndices(unsigned, unsigned) const;
  const TargetRegisterClass *getSubClassWithSubReg(const TargetRegisterClass*, unsigned) const;
};


// Subregister indices
namespace X86 {
enum {
  NoSubRegister,
  sub_8bit,	// 1
  sub_8bit_hi,	// 2
  sub_16bit,	// 3
  sub_32bit,	// 4
  sub_sd,	// 5
  sub_ss,	// 6
  sub_xmm,	// 7
  NUM_TARGET_NAMED_SUBREGS = 8
};
}
namespace X86 { // Register classes
  struct GR8Class : public TargetRegisterClass {
    GR8Class();
    ArrayRef<unsigned> getRawAllocationOrder(const MachineFunction&) const;
  };
  extern GR8Class	GR8RegClass;
  static TargetRegisterClass * const GR8RegisterClass = &GR8RegClass;
  struct GR64Class : public TargetRegisterClass {
    GR64Class();
  };
  extern GR64Class	GR64RegClass;
  static TargetRegisterClass * const GR64RegisterClass = &GR64RegClass;
  struct GR16Class : public TargetRegisterClass {
    GR16Class();
  };
  extern GR16Class	GR16RegClass;
  static TargetRegisterClass * const GR16RegisterClass = &GR16RegClass;
  struct GR32Class : public TargetRegisterClass {
    GR32Class();
  };
  extern GR32Class	GR32RegClass;
  static TargetRegisterClass * const GR32RegisterClass = &GR32RegClass;
  struct FR32Class : public TargetRegisterClass {
    FR32Class();
  };
  extern FR32Class	FR32RegClass;
  static TargetRegisterClass * const FR32RegisterClass = &FR32RegClass;
  struct GR64_with_sub_8bitClass : public TargetRegisterClass {
    GR64_with_sub_8bitClass();
  };
  extern GR64_with_sub_8bitClass	GR64_with_sub_8bitRegClass;
  static TargetRegisterClass * const GR64_with_sub_8bitRegisterClass = &GR64_with_sub_8bitRegClass;
  struct FR64Class : public TargetRegisterClass {
    FR64Class();
  };
  extern FR64Class	FR64RegClass;
  static TargetRegisterClass * const FR64RegisterClass = &FR64RegClass;
  struct CONTROL_REGClass : public TargetRegisterClass {
    CONTROL_REGClass();
  };
  extern CONTROL_REGClass	CONTROL_REGRegClass;
  static TargetRegisterClass * const CONTROL_REGRegisterClass = &CONTROL_REGRegClass;
  struct VR128Class : public TargetRegisterClass {
    VR128Class();
  };
  extern VR128Class	VR128RegClass;
  static TargetRegisterClass * const VR128RegisterClass = &VR128RegClass;
  struct VR256Class : public TargetRegisterClass {
    VR256Class();
  };
  extern VR256Class	VR256RegClass;
  static TargetRegisterClass * const VR256RegisterClass = &VR256RegClass;
  struct GR32_NOSPClass : public TargetRegisterClass {
    GR32_NOSPClass();
  };
  extern GR32_NOSPClass	GR32_NOSPRegClass;
  static TargetRegisterClass * const GR32_NOSPRegisterClass = &GR32_NOSPRegClass;
  struct GR32_NOAXClass : public TargetRegisterClass {
    GR32_NOAXClass();
  };
  extern GR32_NOAXClass	GR32_NOAXRegClass;
  static TargetRegisterClass * const GR32_NOAXRegisterClass = &GR32_NOAXRegClass;
  struct GR64_NOSPClass : public TargetRegisterClass {
    GR64_NOSPClass();
  };
  extern GR64_NOSPClass	GR64_NOSPRegClass;
  static TargetRegisterClass * const GR64_NOSPRegisterClass = &GR64_NOSPRegClass;
  struct GR64_TCClass : public TargetRegisterClass {
    GR64_TCClass();
  };
  extern GR64_TCClass	GR64_TCRegClass;
  static TargetRegisterClass * const GR64_TCRegisterClass = &GR64_TCRegClass;
  struct GR64_NOREXClass : public TargetRegisterClass {
    GR64_NOREXClass();
  };
  extern GR64_NOREXClass	GR64_NOREXRegClass;
  static TargetRegisterClass * const GR64_NOREXRegisterClass = &GR64_NOREXRegClass;
  struct GR8_NOREXClass : public TargetRegisterClass {
    GR8_NOREXClass();
    ArrayRef<unsigned> getRawAllocationOrder(const MachineFunction&) const;
  };
  extern GR8_NOREXClass	GR8_NOREXRegClass;
  static TargetRegisterClass * const GR8_NOREXRegisterClass = &GR8_NOREXRegClass;
  struct GR16_NOREXClass : public TargetRegisterClass {
    GR16_NOREXClass();
  };
  extern GR16_NOREXClass	GR16_NOREXRegClass;
  static TargetRegisterClass * const GR16_NOREXRegisterClass = &GR16_NOREXRegClass;
  struct GR32_NOREXClass : public TargetRegisterClass {
    GR32_NOREXClass();
  };
  extern GR32_NOREXClass	GR32_NOREXRegClass;
  static TargetRegisterClass * const GR32_NOREXRegisterClass = &GR32_NOREXRegClass;
  struct DEBUG_REGClass : public TargetRegisterClass {
    DEBUG_REGClass();
  };
  extern DEBUG_REGClass	DEBUG_REGRegClass;
  static TargetRegisterClass * const DEBUG_REGRegisterClass = &DEBUG_REGRegClass;
  struct VR64Class : public TargetRegisterClass {
    VR64Class();
  };
  extern VR64Class	VR64RegClass;
  static TargetRegisterClass * const VR64RegisterClass = &VR64RegClass;
  struct GR64_TC_with_sub_8bitClass : public TargetRegisterClass {
    GR64_TC_with_sub_8bitClass();
  };
  extern GR64_TC_with_sub_8bitClass	GR64_TC_with_sub_8bitRegClass;
  static TargetRegisterClass * const GR64_TC_with_sub_8bitRegisterClass = &GR64_TC_with_sub_8bitRegClass;
  struct GR64_NOREX_with_sub_8bitClass : public TargetRegisterClass {
    GR64_NOREX_with_sub_8bitClass();
  };
  extern GR64_NOREX_with_sub_8bitClass	GR64_NOREX_with_sub_8bitRegClass;
  static TargetRegisterClass * const GR64_NOREX_with_sub_8bitRegisterClass = &GR64_NOREX_with_sub_8bitRegClass;
  struct RSTClass : public TargetRegisterClass {
    RSTClass();
  };
  extern RSTClass	RSTRegClass;
  static TargetRegisterClass * const RSTRegisterClass = &RSTRegClass;
  struct RFP32Class : public TargetRegisterClass {
    RFP32Class();
  };
  extern RFP32Class	RFP32RegClass;
  static TargetRegisterClass * const RFP32RegisterClass = &RFP32RegClass;
  struct GR32_NOREX_NOSPClass : public TargetRegisterClass {
    GR32_NOREX_NOSPClass();
  };
  extern GR32_NOREX_NOSPClass	GR32_NOREX_NOSPRegClass;
  static TargetRegisterClass * const GR32_NOREX_NOSPRegisterClass = &GR32_NOREX_NOSPRegClass;
  struct RFP64Class : public TargetRegisterClass {
    RFP64Class();
  };
  extern RFP64Class	RFP64RegClass;
  static TargetRegisterClass * const RFP64RegisterClass = &RFP64RegClass;
  struct GR64_NOREX_NOSPClass : public TargetRegisterClass {
    GR64_NOREX_NOSPClass();
  };
  extern GR64_NOREX_NOSPClass	GR64_NOREX_NOSPRegClass;
  static TargetRegisterClass * const GR64_NOREX_NOSPRegisterClass = &GR64_NOREX_NOSPRegClass;
  struct RFP80Class : public TargetRegisterClass {
    RFP80Class();
  };
  extern RFP80Class	RFP80RegClass;
  static TargetRegisterClass * const RFP80RegisterClass = &RFP80RegClass;
  struct SEGMENT_REGClass : public TargetRegisterClass {
    SEGMENT_REGClass();
  };
  extern SEGMENT_REGClass	SEGMENT_REGRegClass;
  static TargetRegisterClass * const SEGMENT_REGRegisterClass = &SEGMENT_REGRegClass;
  struct GR64_TCW64Class : public TargetRegisterClass {
    GR64_TCW64Class();
  };
  extern GR64_TCW64Class	GR64_TCW64RegClass;
  static TargetRegisterClass * const GR64_TCW64RegisterClass = &GR64_TCW64RegClass;
  struct GR8_ABCD_LClass : public TargetRegisterClass {
    GR8_ABCD_LClass();
  };
  extern GR8_ABCD_LClass	GR8_ABCD_LRegClass;
  static TargetRegisterClass * const GR8_ABCD_LRegisterClass = &GR8_ABCD_LRegClass;
  struct GR8_ABCD_HClass : public TargetRegisterClass {
    GR8_ABCD_HClass();
  };
  extern GR8_ABCD_HClass	GR8_ABCD_HRegClass;
  static TargetRegisterClass * const GR8_ABCD_HRegisterClass = &GR8_ABCD_HRegClass;
  struct GR16_ABCDClass : public TargetRegisterClass {
    GR16_ABCDClass();
  };
  extern GR16_ABCDClass	GR16_ABCDRegClass;
  static TargetRegisterClass * const GR16_ABCDRegisterClass = &GR16_ABCDRegClass;
  struct GR32_ABCDClass : public TargetRegisterClass {
    GR32_ABCDClass();
  };
  extern GR32_ABCDClass	GR32_ABCDRegClass;
  static TargetRegisterClass * const GR32_ABCDRegisterClass = &GR32_ABCDRegClass;
  struct GR64_ABCDClass : public TargetRegisterClass {
    GR64_ABCDClass();
  };
  extern GR64_ABCDClass	GR64_ABCDRegClass;
  static TargetRegisterClass * const GR64_ABCDRegisterClass = &GR64_ABCDRegClass;
  struct GR32_TCClass : public TargetRegisterClass {
    GR32_TCClass();
  };
  extern GR32_TCClass	GR32_TCRegClass;
  static TargetRegisterClass * const GR32_TCRegisterClass = &GR32_TCRegClass;
  struct GR32_NOAX_with_sub_8bit_hiClass : public TargetRegisterClass {
    GR32_NOAX_with_sub_8bit_hiClass();
  };
  extern GR32_NOAX_with_sub_8bit_hiClass	GR32_NOAX_with_sub_8bit_hiRegClass;
  static TargetRegisterClass * const GR32_NOAX_with_sub_8bit_hiRegisterClass = &GR32_NOAX_with_sub_8bit_hiRegClass;
  struct GR64_TC_with_sub_8bit_hiClass : public TargetRegisterClass {
    GR64_TC_with_sub_8bit_hiClass();
  };
  extern GR64_TC_with_sub_8bit_hiClass	GR64_TC_with_sub_8bit_hiRegClass;
  static TargetRegisterClass * const GR64_TC_with_sub_8bit_hiRegisterClass = &GR64_TC_with_sub_8bit_hiRegClass;
  struct GR32_ADClass : public TargetRegisterClass {
    GR32_ADClass();
  };
  extern GR32_ADClass	GR32_ADRegClass;
  static TargetRegisterClass * const GR32_ADRegisterClass = &GR32_ADRegClass;
  struct CCRClass : public TargetRegisterClass {
    CCRClass();
  };
  extern CCRClass	CCRRegClass;
  static TargetRegisterClass * const CCRRegisterClass = &CCRRegClass;
} // end of namespace X86

} // End llvm namespace 
#endif // GET_REGINFO_HEADER

//===- TableGen'erated file -------------------------------------*- C++ -*-===//
//
// Target Register and Register Classes Information
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//


#ifdef GET_REGINFO_TARGET_DESC
#undef GET_REGINFO_TARGET_DESC
namespace llvm {

extern MCRegisterClass X86MCRegisterClasses[];
namespace {     // Register classes...
  // GR8VTs Register Class Value Types...
  static const EVT GR8VTs[] = {
    MVT::i8, MVT::Other
  };

  // GR64VTs Register Class Value Types...
  static const EVT GR64VTs[] = {
    MVT::i64, MVT::Other
  };

  // GR16VTs Register Class Value Types...
  static const EVT GR16VTs[] = {
    MVT::i16, MVT::Other
  };

  // GR32VTs Register Class Value Types...
  static const EVT GR32VTs[] = {
    MVT::i32, MVT::Other
  };

  // FR32VTs Register Class Value Types...
  static const EVT FR32VTs[] = {
    MVT::f32, MVT::Other
  };

  // GR64_with_sub_8bitVTs Register Class Value Types...
  static const EVT GR64_with_sub_8bitVTs[] = {
    MVT::i64, MVT::Other
  };

  // FR64VTs Register Class Value Types...
  static const EVT FR64VTs[] = {
    MVT::f64, MVT::Other
  };

  // CONTROL_REGVTs Register Class Value Types...
  static const EVT CONTROL_REGVTs[] = {
    MVT::i64, MVT::Other
  };

  // VR128VTs Register Class Value Types...
  static const EVT VR128VTs[] = {
    MVT::v16i8, MVT::v8i16, MVT::v4i32, MVT::v2i64, MVT::v4f32, MVT::v2f64, MVT::Other
  };

  // VR256VTs Register Class Value Types...
  static const EVT VR256VTs[] = {
    MVT::v32i8, MVT::v16i16, MVT::v8i32, MVT::v4i64, MVT::v8f32, MVT::v4f64, MVT::Other
  };

  // GR32_NOSPVTs Register Class Value Types...
  static const EVT GR32_NOSPVTs[] = {
    MVT::i32, MVT::Other
  };

  // GR32_NOAXVTs Register Class Value Types...
  static const EVT GR32_NOAXVTs[] = {
    MVT::i32, MVT::Other
  };

  // GR64_NOSPVTs Register Class Value Types...
  static const EVT GR64_NOSPVTs[] = {
    MVT::i64, MVT::Other
  };

  // GR64_TCVTs Register Class Value Types...
  static const EVT GR64_TCVTs[] = {
    MVT::i64, MVT::Other
  };

  // GR64_NOREXVTs Register Class Value Types...
  static const EVT GR64_NOREXVTs[] = {
    MVT::i64, MVT::Other
  };

  // GR8_NOREXVTs Register Class Value Types...
  static const EVT GR8_NOREXVTs[] = {
    MVT::i8, MVT::Other
  };

  // GR16_NOREXVTs Register Class Value Types...
  static const EVT GR16_NOREXVTs[] = {
    MVT::i16, MVT::Other
  };

  // GR32_NOREXVTs Register Class Value Types...
  static const EVT GR32_NOREXVTs[] = {
    MVT::i32, MVT::Other
  };

  // DEBUG_REGVTs Register Class Value Types...
  static const EVT DEBUG_REGVTs[] = {
    MVT::i32, MVT::Other
  };

  // VR64VTs Register Class Value Types...
  static const EVT VR64VTs[] = {
    MVT::x86mmx, MVT::Other
  };

  // GR64_TC_with_sub_8bitVTs Register Class Value Types...
  static const EVT GR64_TC_with_sub_8bitVTs[] = {
    MVT::i64, MVT::Other
  };

  // GR64_NOREX_with_sub_8bitVTs Register Class Value Types...
  static const EVT GR64_NOREX_with_sub_8bitVTs[] = {
    MVT::i64, MVT::Other
  };

  // RSTVTs Register Class Value Types...
  static const EVT RSTVTs[] = {
    MVT::f80, MVT::f64, MVT::f32, MVT::Other
  };

  // RFP32VTs Register Class Value Types...
  static const EVT RFP32VTs[] = {
    MVT::f32, MVT::Other
  };

  // GR32_NOREX_NOSPVTs Register Class Value Types...
  static const EVT GR32_NOREX_NOSPVTs[] = {
    MVT::i32, MVT::Other
  };

  // RFP64VTs Register Class Value Types...
  static const EVT RFP64VTs[] = {
    MVT::f64, MVT::Other
  };

  // GR64_NOREX_NOSPVTs Register Class Value Types...
  static const EVT GR64_NOREX_NOSPVTs[] = {
    MVT::i64, MVT::Other
  };

  // RFP80VTs Register Class Value Types...
  static const EVT RFP80VTs[] = {
    MVT::f80, MVT::Other
  };

  // SEGMENT_REGVTs Register Class Value Types...
  static const EVT SEGMENT_REGVTs[] = {
    MVT::i16, MVT::Other
  };

  // GR64_TCW64VTs Register Class Value Types...
  static const EVT GR64_TCW64VTs[] = {
    MVT::i64, MVT::Other
  };

  // GR8_ABCD_LVTs Register Class Value Types...
  static const EVT GR8_ABCD_LVTs[] = {
    MVT::i8, MVT::Other
  };

  // GR8_ABCD_HVTs Register Class Value Types...
  static const EVT GR8_ABCD_HVTs[] = {
    MVT::i8, MVT::Other
  };

  // GR16_ABCDVTs Register Class Value Types...
  static const EVT GR16_ABCDVTs[] = {
    MVT::i16, MVT::Other
  };

  // GR32_ABCDVTs Register Class Value Types...
  static const EVT GR32_ABCDVTs[] = {
    MVT::i32, MVT::Other
  };

  // GR64_ABCDVTs Register Class Value Types...
  static const EVT GR64_ABCDVTs[] = {
    MVT::i64, MVT::Other
  };

  // GR32_TCVTs Register Class Value Types...
  static const EVT GR32_TCVTs[] = {
    MVT::i32, MVT::Other
  };

  // GR32_NOAX_with_sub_8bit_hiVTs Register Class Value Types...
  static const EVT GR32_NOAX_with_sub_8bit_hiVTs[] = {
    MVT::i32, MVT::Other
  };

  // GR64_TC_with_sub_8bit_hiVTs Register Class Value Types...
  static const EVT GR64_TC_with_sub_8bit_hiVTs[] = {
    MVT::i64, MVT::Other
  };

  // GR32_ADVTs Register Class Value Types...
  static const EVT GR32_ADVTs[] = {
    MVT::i32, MVT::Other
  };

  // CCRVTs Register Class Value Types...
  static const EVT CCRVTs[] = {
    MVT::i32, MVT::Other
  };

}  // end anonymous namespace

namespace X86 {   // Register class instances
  GR8Class	GR8RegClass;
  GR64Class	GR64RegClass;
  GR16Class	GR16RegClass;
  GR32Class	GR32RegClass;
  FR32Class	FR32RegClass;
  GR64_with_sub_8bitClass	GR64_with_sub_8bitRegClass;
  FR64Class	FR64RegClass;
  CONTROL_REGClass	CONTROL_REGRegClass;
  VR128Class	VR128RegClass;
  VR256Class	VR256RegClass;
  GR32_NOSPClass	GR32_NOSPRegClass;
  GR32_NOAXClass	GR32_NOAXRegClass;
  GR64_NOSPClass	GR64_NOSPRegClass;
  GR64_TCClass	GR64_TCRegClass;
  GR64_NOREXClass	GR64_NOREXRegClass;
  GR8_NOREXClass	GR8_NOREXRegClass;
  GR16_NOREXClass	GR16_NOREXRegClass;
  GR32_NOREXClass	GR32_NOREXRegClass;
  DEBUG_REGClass	DEBUG_REGRegClass;
  VR64Class	VR64RegClass;
  GR64_TC_with_sub_8bitClass	GR64_TC_with_sub_8bitRegClass;
  GR64_NOREX_with_sub_8bitClass	GR64_NOREX_with_sub_8bitRegClass;
  RSTClass	RSTRegClass;
  RFP32Class	RFP32RegClass;
  GR32_NOREX_NOSPClass	GR32_NOREX_NOSPRegClass;
  RFP64Class	RFP64RegClass;
  GR64_NOREX_NOSPClass	GR64_NOREX_NOSPRegClass;
  RFP80Class	RFP80RegClass;
  SEGMENT_REGClass	SEGMENT_REGRegClass;
  GR64_TCW64Class	GR64_TCW64RegClass;
  GR8_ABCD_LClass	GR8_ABCD_LRegClass;
  GR8_ABCD_HClass	GR8_ABCD_HRegClass;
  GR16_ABCDClass	GR16_ABCDRegClass;
  GR32_ABCDClass	GR32_ABCDRegClass;
  GR64_ABCDClass	GR64_ABCDRegClass;
  GR32_TCClass	GR32_TCRegClass;
  GR32_NOAX_with_sub_8bit_hiClass	GR32_NOAX_with_sub_8bit_hiRegClass;
  GR64_TC_with_sub_8bit_hiClass	GR64_TC_with_sub_8bit_hiRegClass;
  GR32_ADClass	GR32_ADRegClass;
  CCRClass	CCRRegClass;

  static const TargetRegisterClass* const NullRegClasses[] = { NULL };

  // GR8 Super-register Classes...
  static const TargetRegisterClass* const GR8SuperRegClasses[] = {
    &X86::GR64RegClass, &X86::GR16RegClass, &X86::GR32RegClass, &X86::GR32_NOSPRegClass, &X86::GR32_NOAXRegClass, &X86::GR64_NOSPRegClass, &X86::GR64_TCRegClass, &X86::GR32_TCRegClass, NULL
  };

  // GR64 Super-register Classes...
  static const TargetRegisterClass* const GR64SuperRegClasses[] = {
    NULL
  };

  // GR16 Super-register Classes...
  static const TargetRegisterClass* const GR16SuperRegClasses[] = {
    &X86::GR64RegClass, &X86::GR32RegClass, &X86::GR32_NOSPRegClass, &X86::GR32_NOAXRegClass, &X86::GR64_NOSPRegClass, &X86::GR64_TCRegClass, &X86::GR32_TCRegClass, NULL
  };

  // GR32 Super-register Classes...
  static const TargetRegisterClass* const GR32SuperRegClasses[] = {
    &X86::GR64RegClass, NULL
  };

  // FR32 Super-register Classes...
  static const TargetRegisterClass* const FR32SuperRegClasses[] = {
    &X86::VR128RegClass, &X86::VR256RegClass, NULL
  };

  // GR64_with_sub_8bit Super-register Classes...
  static const TargetRegisterClass* const GR64_with_sub_8bitSuperRegClasses[] = {
    NULL
  };

  // FR64 Super-register Classes...
  static const TargetRegisterClass* const FR64SuperRegClasses[] = {
    &X86::VR128RegClass, &X86::VR256RegClass, NULL
  };

  // CONTROL_REG Super-register Classes...
  static const TargetRegisterClass* const CONTROL_REGSuperRegClasses[] = {
    NULL
  };

  // VR128 Super-register Classes...
  static const TargetRegisterClass* const VR128SuperRegClasses[] = {
    &X86::VR256RegClass, NULL
  };

  // VR256 Super-register Classes...
  static const TargetRegisterClass* const VR256SuperRegClasses[] = {
    NULL
  };

  // GR32_NOSP Super-register Classes...
  static const TargetRegisterClass* const GR32_NOSPSuperRegClasses[] = {
    &X86::GR64_NOSPRegClass, NULL
  };

  // GR32_NOAX Super-register Classes...
  static const TargetRegisterClass* const GR32_NOAXSuperRegClasses[] = {
    NULL
  };

  // GR64_NOSP Super-register Classes...
  static const TargetRegisterClass* const GR64_NOSPSuperRegClasses[] = {
    NULL
  };

  // GR64_TC Super-register Classes...
  static const TargetRegisterClass* const GR64_TCSuperRegClasses[] = {
    NULL
  };

  // GR64_NOREX Super-register Classes...
  static const TargetRegisterClass* const GR64_NOREXSuperRegClasses[] = {
    NULL
  };

  // GR8_NOREX Super-register Classes...
  static const TargetRegisterClass* const GR8_NOREXSuperRegClasses[] = {
    &X86::GR64_NOREXRegClass, &X86::GR16_NOREXRegClass, &X86::GR32_NOREXRegClass, &X86::GR32_NOREX_NOSPRegClass, &X86::GR64_NOREX_NOSPRegClass, NULL
  };

  // GR16_NOREX Super-register Classes...
  static const TargetRegisterClass* const GR16_NOREXSuperRegClasses[] = {
    &X86::GR64_NOREXRegClass, &X86::GR32_NOREXRegClass, &X86::GR32_NOREX_NOSPRegClass, &X86::GR64_NOREX_NOSPRegClass, NULL
  };

  // GR32_NOREX Super-register Classes...
  static const TargetRegisterClass* const GR32_NOREXSuperRegClasses[] = {
    &X86::GR64_NOREXRegClass, NULL
  };

  // DEBUG_REG Super-register Classes...
  static const TargetRegisterClass* const DEBUG_REGSuperRegClasses[] = {
    NULL
  };

  // VR64 Super-register Classes...
  static const TargetRegisterClass* const VR64SuperRegClasses[] = {
    NULL
  };

  // GR64_TC_with_sub_8bit Super-register Classes...
  static const TargetRegisterClass* const GR64_TC_with_sub_8bitSuperRegClasses[] = {
    NULL
  };

  // GR64_NOREX_with_sub_8bit Super-register Classes...
  static const TargetRegisterClass* const GR64_NOREX_with_sub_8bitSuperRegClasses[] = {
    NULL
  };

  // RST Super-register Classes...
  static const TargetRegisterClass* const RSTSuperRegClasses[] = {
    NULL
  };

  // RFP32 Super-register Classes...
  static const TargetRegisterClass* const RFP32SuperRegClasses[] = {
    NULL
  };

  // GR32_NOREX_NOSP Super-register Classes...
  static const TargetRegisterClass* const GR32_NOREX_NOSPSuperRegClasses[] = {
    &X86::GR64_NOREX_NOSPRegClass, NULL
  };

  // RFP64 Super-register Classes...
  static const TargetRegisterClass* const RFP64SuperRegClasses[] = {
    NULL
  };

  // GR64_NOREX_NOSP Super-register Classes...
  static const TargetRegisterClass* const GR64_NOREX_NOSPSuperRegClasses[] = {
    NULL
  };

  // RFP80 Super-register Classes...
  static const TargetRegisterClass* const RFP80SuperRegClasses[] = {
    NULL
  };

  // SEGMENT_REG Super-register Classes...
  static const TargetRegisterClass* const SEGMENT_REGSuperRegClasses[] = {
    NULL
  };

  // GR64_TCW64 Super-register Classes...
  static const TargetRegisterClass* const GR64_TCW64SuperRegClasses[] = {
    NULL
  };

  // GR8_ABCD_L Super-register Classes...
  static const TargetRegisterClass* const GR8_ABCD_LSuperRegClasses[] = {
    &X86::GR16_ABCDRegClass, &X86::GR32_ABCDRegClass, &X86::GR64_ABCDRegClass, &X86::GR32_ADRegClass, NULL
  };

  // GR8_ABCD_H Super-register Classes...
  static const TargetRegisterClass* const GR8_ABCD_HSuperRegClasses[] = {
    &X86::GR16_ABCDRegClass, &X86::GR32_ABCDRegClass, &X86::GR64_ABCDRegClass, &X86::GR32_ADRegClass, NULL
  };

  // GR16_ABCD Super-register Classes...
  static const TargetRegisterClass* const GR16_ABCDSuperRegClasses[] = {
    &X86::GR32_ABCDRegClass, &X86::GR64_ABCDRegClass, &X86::GR32_ADRegClass, NULL
  };

  // GR32_ABCD Super-register Classes...
  static const TargetRegisterClass* const GR32_ABCDSuperRegClasses[] = {
    &X86::GR64_ABCDRegClass, NULL
  };

  // GR64_ABCD Super-register Classes...
  static const TargetRegisterClass* const GR64_ABCDSuperRegClasses[] = {
    NULL
  };

  // GR32_TC Super-register Classes...
  static const TargetRegisterClass* const GR32_TCSuperRegClasses[] = {
    &X86::GR64_TCRegClass, NULL
  };

  // GR32_NOAX_with_sub_8bit_hi Super-register Classes...
  static const TargetRegisterClass* const GR32_NOAX_with_sub_8bit_hiSuperRegClasses[] = {
    NULL
  };

  // GR64_TC_with_sub_8bit_hi Super-register Classes...
  static const TargetRegisterClass* const GR64_TC_with_sub_8bit_hiSuperRegClasses[] = {
    NULL
  };

  // GR32_AD Super-register Classes...
  static const TargetRegisterClass* const GR32_ADSuperRegClasses[] = {
    NULL
  };

  // CCR Super-register Classes...
  static const TargetRegisterClass* const CCRSuperRegClasses[] = {
    NULL
  };

  static const unsigned GR8SubclassMask[] = { 0xc0008001, 0x00000000, };

  static const unsigned GR64SubclassMask[] = { 0x24307022, 0x00000024, };

  static const unsigned GR16SubclassMask[] = { 0x00010004, 0x00000001, };

  static const unsigned GR32SubclassMask[] = { 0x01020c08, 0x0000005a, };

  static const unsigned FR32SubclassMask[] = { 0x00000150, 0x00000000, };

  static const unsigned GR64_with_sub_8bitSubclassMask[] = { 0x24301020, 0x00000024, };

  static const unsigned FR64SubclassMask[] = { 0x00000140, 0x00000000, };

  static const unsigned CONTROL_REGSubclassMask[] = { 0x00000080, 0x00000000, };

  static const unsigned VR128SubclassMask[] = { 0x00000100, 0x00000000, };

  static const unsigned VR256SubclassMask[] = { 0x00000200, 0x00000000, };

  static const unsigned GR32_NOSPSubclassMask[] = { 0x01000400, 0x0000005a, };

  static const unsigned GR32_NOAXSubclassMask[] = { 0x00000800, 0x00000010, };

  static const unsigned GR64_NOSPSubclassMask[] = { 0x24101000, 0x00000024, };

  static const unsigned GR64_TCSubclassMask[] = { 0x20102000, 0x00000020, };

  static const unsigned GR64_NOREXSubclassMask[] = { 0x04204000, 0x00000024, };

  static const unsigned GR8_NOREXSubclassMask[] = { 0xc0008000, 0x00000000, };

  static const unsigned GR16_NOREXSubclassMask[] = { 0x00010000, 0x00000001, };

  static const unsigned GR32_NOREXSubclassMask[] = { 0x01020000, 0x0000005a, };

  static const unsigned DEBUG_REGSubclassMask[] = { 0x00040000, 0x00000000, };

  static const unsigned VR64SubclassMask[] = { 0x00080000, 0x00000000, };

  static const unsigned GR64_TC_with_sub_8bitSubclassMask[] = { 0x20100000, 0x00000020, };

  static const unsigned GR64_NOREX_with_sub_8bitSubclassMask[] = { 0x04200000, 0x00000024, };

  static const unsigned RSTSubclassMask[] = { 0x00400000, 0x00000000, };

  static const unsigned RFP32SubclassMask[] = { 0x0a800000, 0x00000000, };

  static const unsigned GR32_NOREX_NOSPSubclassMask[] = { 0x01000000, 0x0000005a, };

  static const unsigned RFP64SubclassMask[] = { 0x0a000000, 0x00000000, };

  static const unsigned GR64_NOREX_NOSPSubclassMask[] = { 0x04000000, 0x00000024, };

  static const unsigned RFP80SubclassMask[] = { 0x08000000, 0x00000000, };

  static const unsigned SEGMENT_REGSubclassMask[] = { 0x10000000, 0x00000000, };

  static const unsigned GR64_TCW64SubclassMask[] = { 0x20000000, 0x00000020, };

  static const unsigned GR8_ABCD_LSubclassMask[] = { 0x40000000, 0x00000000, };

  static const unsigned GR8_ABCD_HSubclassMask[] = { 0x80000000, 0x00000000, };

  static const unsigned GR16_ABCDSubclassMask[] = { 0x00000000, 0x00000001, };

  static const unsigned GR32_ABCDSubclassMask[] = { 0x00000000, 0x0000005a, };

  static const unsigned GR64_ABCDSubclassMask[] = { 0x00000000, 0x00000024, };

  static const unsigned GR32_TCSubclassMask[] = { 0x00000000, 0x00000048, };

  static const unsigned GR32_NOAX_with_sub_8bit_hiSubclassMask[] = { 0x00000000, 0x00000010, };

  static const unsigned GR64_TC_with_sub_8bit_hiSubclassMask[] = { 0x00000000, 0x00000020, };

  static const unsigned GR32_ADSubclassMask[] = { 0x00000000, 0x00000040, };

  static const unsigned CCRSubclassMask[] = { 0x00000000, 0x00000080, };

  static const TargetRegisterClass* const GR64_with_sub_8bitSuperclasses[] = {
    &X86::GR64RegClass,
    NULL
  };

  static const TargetRegisterClass* const FR64Superclasses[] = {
    &X86::FR32RegClass,
    NULL
  };

  static const TargetRegisterClass* const VR128Superclasses[] = {
    &X86::FR32RegClass,
    &X86::FR64RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_NOSPSuperclasses[] = {
    &X86::GR32RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_NOAXSuperclasses[] = {
    &X86::GR32RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_NOSPSuperclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_TCSuperclasses[] = {
    &X86::GR64RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_NOREXSuperclasses[] = {
    &X86::GR64RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR8_NOREXSuperclasses[] = {
    &X86::GR8RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR16_NOREXSuperclasses[] = {
    &X86::GR16RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_NOREXSuperclasses[] = {
    &X86::GR32RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_TC_with_sub_8bitSuperclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::GR64_NOSPRegClass,
    &X86::GR64_TCRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_NOREX_with_sub_8bitSuperclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::GR64_NOREXRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_NOREX_NOSPSuperclasses[] = {
    &X86::GR32RegClass,
    &X86::GR32_NOSPRegClass,
    &X86::GR32_NOREXRegClass,
    NULL
  };

  static const TargetRegisterClass* const RFP64Superclasses[] = {
    &X86::RFP32RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_NOREX_NOSPSuperclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::GR64_NOSPRegClass,
    &X86::GR64_NOREXRegClass,
    &X86::GR64_NOREX_with_sub_8bitRegClass,
    NULL
  };

  static const TargetRegisterClass* const RFP80Superclasses[] = {
    &X86::RFP32RegClass,
    &X86::RFP64RegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_TCW64Superclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::GR64_NOSPRegClass,
    &X86::GR64_TCRegClass,
    &X86::GR64_TC_with_sub_8bitRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR8_ABCD_LSuperclasses[] = {
    &X86::GR8RegClass,
    &X86::GR8_NOREXRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR8_ABCD_HSuperclasses[] = {
    &X86::GR8RegClass,
    &X86::GR8_NOREXRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR16_ABCDSuperclasses[] = {
    &X86::GR16RegClass,
    &X86::GR16_NOREXRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_ABCDSuperclasses[] = {
    &X86::GR32RegClass,
    &X86::GR32_NOSPRegClass,
    &X86::GR32_NOREXRegClass,
    &X86::GR32_NOREX_NOSPRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_ABCDSuperclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::GR64_NOSPRegClass,
    &X86::GR64_NOREXRegClass,
    &X86::GR64_NOREX_with_sub_8bitRegClass,
    &X86::GR64_NOREX_NOSPRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_TCSuperclasses[] = {
    &X86::GR32RegClass,
    &X86::GR32_NOSPRegClass,
    &X86::GR32_NOREXRegClass,
    &X86::GR32_NOREX_NOSPRegClass,
    &X86::GR32_ABCDRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_NOAX_with_sub_8bit_hiSuperclasses[] = {
    &X86::GR32RegClass,
    &X86::GR32_NOSPRegClass,
    &X86::GR32_NOAXRegClass,
    &X86::GR32_NOREXRegClass,
    &X86::GR32_NOREX_NOSPRegClass,
    &X86::GR32_ABCDRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR64_TC_with_sub_8bit_hiSuperclasses[] = {
    &X86::GR64RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::GR64_NOSPRegClass,
    &X86::GR64_TCRegClass,
    &X86::GR64_NOREXRegClass,
    &X86::GR64_TC_with_sub_8bitRegClass,
    &X86::GR64_NOREX_with_sub_8bitRegClass,
    &X86::GR64_NOREX_NOSPRegClass,
    &X86::GR64_TCW64RegClass,
    &X86::GR64_ABCDRegClass,
    NULL
  };

  static const TargetRegisterClass* const GR32_ADSuperclasses[] = {
    &X86::GR32RegClass,
    &X86::GR32_NOSPRegClass,
    &X86::GR32_NOREXRegClass,
    &X86::GR32_NOREX_NOSPRegClass,
    &X86::GR32_ABCDRegClass,
    &X86::GR32_TCRegClass,
    NULL
  };

GR8Class::GR8Class()  : TargetRegisterClass(&X86MCRegisterClasses[GR8RegClassID], GR8VTs, GR8SubclassMask, NullRegClasses, GR8SuperRegClasses) {}

static inline unsigned GR8AltOrderSelect(const MachineFunction &MF) {
    return MF.getTarget().getSubtarget<X86Subtarget>().is64Bit();
  }

ArrayRef<unsigned> GR8Class::getRawAllocationOrder(const MachineFunction &MF) const {
  static const unsigned AltOrder1[] = { X86::AL, X86::CL, X86::DL, X86::BL, X86::SIL, X86::DIL, X86::BPL, X86::SPL, X86::R8B, X86::R9B, X86::R10B, X86::R11B, X86::R14B, X86::R15B, X86::R12B, X86::R13B };
  const MCRegisterClass &MCR = X86MCRegisterClasses[X86::GR8RegClassID];  static const ArrayRef<unsigned> Order[] = {
    makeArrayRef(MCR.begin(), MCR.getNumRegs()),
    makeArrayRef(AltOrder1)
  };
  const unsigned Select = GR8AltOrderSelect(MF);
  assert(Select < 2);
  return Order[Select];
}
GR64Class::GR64Class()  : TargetRegisterClass(&X86MCRegisterClasses[GR64RegClassID], GR64VTs, GR64SubclassMask, NullRegClasses, GR64SuperRegClasses) {}
GR16Class::GR16Class()  : TargetRegisterClass(&X86MCRegisterClasses[GR16RegClassID], GR16VTs, GR16SubclassMask, NullRegClasses, GR16SuperRegClasses) {}
GR32Class::GR32Class()  : TargetRegisterClass(&X86MCRegisterClasses[GR32RegClassID], GR32VTs, GR32SubclassMask, NullRegClasses, GR32SuperRegClasses) {}
FR32Class::FR32Class()  : TargetRegisterClass(&X86MCRegisterClasses[FR32RegClassID], FR32VTs, FR32SubclassMask, NullRegClasses, FR32SuperRegClasses) {}
GR64_with_sub_8bitClass::GR64_with_sub_8bitClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_with_sub_8bitRegClassID], GR64_with_sub_8bitVTs, GR64_with_sub_8bitSubclassMask, GR64_with_sub_8bitSuperclasses, GR64_with_sub_8bitSuperRegClasses) {}
FR64Class::FR64Class()  : TargetRegisterClass(&X86MCRegisterClasses[FR64RegClassID], FR64VTs, FR64SubclassMask, FR64Superclasses, FR64SuperRegClasses) {}
CONTROL_REGClass::CONTROL_REGClass()  : TargetRegisterClass(&X86MCRegisterClasses[CONTROL_REGRegClassID], CONTROL_REGVTs, CONTROL_REGSubclassMask, NullRegClasses, CONTROL_REGSuperRegClasses) {}
VR128Class::VR128Class()  : TargetRegisterClass(&X86MCRegisterClasses[VR128RegClassID], VR128VTs, VR128SubclassMask, VR128Superclasses, VR128SuperRegClasses) {}
VR256Class::VR256Class()  : TargetRegisterClass(&X86MCRegisterClasses[VR256RegClassID], VR256VTs, VR256SubclassMask, NullRegClasses, VR256SuperRegClasses) {}
GR32_NOSPClass::GR32_NOSPClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_NOSPRegClassID], GR32_NOSPVTs, GR32_NOSPSubclassMask, GR32_NOSPSuperclasses, GR32_NOSPSuperRegClasses) {}
GR32_NOAXClass::GR32_NOAXClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_NOAXRegClassID], GR32_NOAXVTs, GR32_NOAXSubclassMask, GR32_NOAXSuperclasses, GR32_NOAXSuperRegClasses) {}
GR64_NOSPClass::GR64_NOSPClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_NOSPRegClassID], GR64_NOSPVTs, GR64_NOSPSubclassMask, GR64_NOSPSuperclasses, GR64_NOSPSuperRegClasses) {}
GR64_TCClass::GR64_TCClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_TCRegClassID], GR64_TCVTs, GR64_TCSubclassMask, GR64_TCSuperclasses, GR64_TCSuperRegClasses) {}
GR64_NOREXClass::GR64_NOREXClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_NOREXRegClassID], GR64_NOREXVTs, GR64_NOREXSubclassMask, GR64_NOREXSuperclasses, GR64_NOREXSuperRegClasses) {}
GR8_NOREXClass::GR8_NOREXClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR8_NOREXRegClassID], GR8_NOREXVTs, GR8_NOREXSubclassMask, GR8_NOREXSuperclasses, GR8_NOREXSuperRegClasses) {}

static inline unsigned GR8_NOREXAltOrderSelect(const MachineFunction &MF) {
    return MF.getTarget().getSubtarget<X86Subtarget>().is64Bit();
  }

ArrayRef<unsigned> GR8_NOREXClass::getRawAllocationOrder(const MachineFunction &MF) const {
  static const unsigned AltOrder1[] = { X86::AL, X86::CL, X86::DL, X86::BL };
  const MCRegisterClass &MCR = X86MCRegisterClasses[X86::GR8_NOREXRegClassID];  static const ArrayRef<unsigned> Order[] = {
    makeArrayRef(MCR.begin(), MCR.getNumRegs()),
    makeArrayRef(AltOrder1)
  };
  const unsigned Select = GR8_NOREXAltOrderSelect(MF);
  assert(Select < 2);
  return Order[Select];
}
GR16_NOREXClass::GR16_NOREXClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR16_NOREXRegClassID], GR16_NOREXVTs, GR16_NOREXSubclassMask, GR16_NOREXSuperclasses, GR16_NOREXSuperRegClasses) {}
GR32_NOREXClass::GR32_NOREXClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_NOREXRegClassID], GR32_NOREXVTs, GR32_NOREXSubclassMask, GR32_NOREXSuperclasses, GR32_NOREXSuperRegClasses) {}
DEBUG_REGClass::DEBUG_REGClass()  : TargetRegisterClass(&X86MCRegisterClasses[DEBUG_REGRegClassID], DEBUG_REGVTs, DEBUG_REGSubclassMask, NullRegClasses, DEBUG_REGSuperRegClasses) {}
VR64Class::VR64Class()  : TargetRegisterClass(&X86MCRegisterClasses[VR64RegClassID], VR64VTs, VR64SubclassMask, NullRegClasses, VR64SuperRegClasses) {}
GR64_TC_with_sub_8bitClass::GR64_TC_with_sub_8bitClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_TC_with_sub_8bitRegClassID], GR64_TC_with_sub_8bitVTs, GR64_TC_with_sub_8bitSubclassMask, GR64_TC_with_sub_8bitSuperclasses, GR64_TC_with_sub_8bitSuperRegClasses) {}
GR64_NOREX_with_sub_8bitClass::GR64_NOREX_with_sub_8bitClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_NOREX_with_sub_8bitRegClassID], GR64_NOREX_with_sub_8bitVTs, GR64_NOREX_with_sub_8bitSubclassMask, GR64_NOREX_with_sub_8bitSuperclasses, GR64_NOREX_with_sub_8bitSuperRegClasses) {}
RSTClass::RSTClass()  : TargetRegisterClass(&X86MCRegisterClasses[RSTRegClassID], RSTVTs, RSTSubclassMask, NullRegClasses, RSTSuperRegClasses) {}
RFP32Class::RFP32Class()  : TargetRegisterClass(&X86MCRegisterClasses[RFP32RegClassID], RFP32VTs, RFP32SubclassMask, NullRegClasses, RFP32SuperRegClasses) {}
GR32_NOREX_NOSPClass::GR32_NOREX_NOSPClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_NOREX_NOSPRegClassID], GR32_NOREX_NOSPVTs, GR32_NOREX_NOSPSubclassMask, GR32_NOREX_NOSPSuperclasses, GR32_NOREX_NOSPSuperRegClasses) {}
RFP64Class::RFP64Class()  : TargetRegisterClass(&X86MCRegisterClasses[RFP64RegClassID], RFP64VTs, RFP64SubclassMask, RFP64Superclasses, RFP64SuperRegClasses) {}
GR64_NOREX_NOSPClass::GR64_NOREX_NOSPClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_NOREX_NOSPRegClassID], GR64_NOREX_NOSPVTs, GR64_NOREX_NOSPSubclassMask, GR64_NOREX_NOSPSuperclasses, GR64_NOREX_NOSPSuperRegClasses) {}
RFP80Class::RFP80Class()  : TargetRegisterClass(&X86MCRegisterClasses[RFP80RegClassID], RFP80VTs, RFP80SubclassMask, RFP80Superclasses, RFP80SuperRegClasses) {}
SEGMENT_REGClass::SEGMENT_REGClass()  : TargetRegisterClass(&X86MCRegisterClasses[SEGMENT_REGRegClassID], SEGMENT_REGVTs, SEGMENT_REGSubclassMask, NullRegClasses, SEGMENT_REGSuperRegClasses) {}
GR64_TCW64Class::GR64_TCW64Class()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_TCW64RegClassID], GR64_TCW64VTs, GR64_TCW64SubclassMask, GR64_TCW64Superclasses, GR64_TCW64SuperRegClasses) {}
GR8_ABCD_LClass::GR8_ABCD_LClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR8_ABCD_LRegClassID], GR8_ABCD_LVTs, GR8_ABCD_LSubclassMask, GR8_ABCD_LSuperclasses, GR8_ABCD_LSuperRegClasses) {}
GR8_ABCD_HClass::GR8_ABCD_HClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR8_ABCD_HRegClassID], GR8_ABCD_HVTs, GR8_ABCD_HSubclassMask, GR8_ABCD_HSuperclasses, GR8_ABCD_HSuperRegClasses) {}
GR16_ABCDClass::GR16_ABCDClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR16_ABCDRegClassID], GR16_ABCDVTs, GR16_ABCDSubclassMask, GR16_ABCDSuperclasses, GR16_ABCDSuperRegClasses) {}
GR32_ABCDClass::GR32_ABCDClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_ABCDRegClassID], GR32_ABCDVTs, GR32_ABCDSubclassMask, GR32_ABCDSuperclasses, GR32_ABCDSuperRegClasses) {}
GR64_ABCDClass::GR64_ABCDClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_ABCDRegClassID], GR64_ABCDVTs, GR64_ABCDSubclassMask, GR64_ABCDSuperclasses, GR64_ABCDSuperRegClasses) {}
GR32_TCClass::GR32_TCClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_TCRegClassID], GR32_TCVTs, GR32_TCSubclassMask, GR32_TCSuperclasses, GR32_TCSuperRegClasses) {}
GR32_NOAX_with_sub_8bit_hiClass::GR32_NOAX_with_sub_8bit_hiClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_NOAX_with_sub_8bit_hiRegClassID], GR32_NOAX_with_sub_8bit_hiVTs, GR32_NOAX_with_sub_8bit_hiSubclassMask, GR32_NOAX_with_sub_8bit_hiSuperclasses, GR32_NOAX_with_sub_8bit_hiSuperRegClasses) {}
GR64_TC_with_sub_8bit_hiClass::GR64_TC_with_sub_8bit_hiClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR64_TC_with_sub_8bit_hiRegClassID], GR64_TC_with_sub_8bit_hiVTs, GR64_TC_with_sub_8bit_hiSubclassMask, GR64_TC_with_sub_8bit_hiSuperclasses, GR64_TC_with_sub_8bit_hiSuperRegClasses) {}
GR32_ADClass::GR32_ADClass()  : TargetRegisterClass(&X86MCRegisterClasses[GR32_ADRegClassID], GR32_ADVTs, GR32_ADSubclassMask, GR32_ADSuperclasses, GR32_ADSuperRegClasses) {}
CCRClass::CCRClass()  : TargetRegisterClass(&X86MCRegisterClasses[CCRRegClassID], CCRVTs, CCRSubclassMask, NullRegClasses, CCRSuperRegClasses) {}
}

namespace {
  const TargetRegisterClass* const RegisterClasses[] = {
    &X86::GR8RegClass,
    &X86::GR64RegClass,
    &X86::GR16RegClass,
    &X86::GR32RegClass,
    &X86::FR32RegClass,
    &X86::GR64_with_sub_8bitRegClass,
    &X86::FR64RegClass,
    &X86::CONTROL_REGRegClass,
    &X86::VR128RegClass,
    &X86::VR256RegClass,
    &X86::GR32_NOSPRegClass,
    &X86::GR32_NOAXRegClass,
    &X86::GR64_NOSPRegClass,
    &X86::GR64_TCRegClass,
    &X86::GR64_NOREXRegClass,
    &X86::GR8_NOREXRegClass,
    &X86::GR16_NOREXRegClass,
    &X86::GR32_NOREXRegClass,
    &X86::DEBUG_REGRegClass,
    &X86::VR64RegClass,
    &X86::GR64_TC_with_sub_8bitRegClass,
    &X86::GR64_NOREX_with_sub_8bitRegClass,
    &X86::RSTRegClass,
    &X86::RFP32RegClass,
    &X86::GR32_NOREX_NOSPRegClass,
    &X86::RFP64RegClass,
    &X86::GR64_NOREX_NOSPRegClass,
    &X86::RFP80RegClass,
    &X86::SEGMENT_REGRegClass,
    &X86::GR64_TCW64RegClass,
    &X86::GR8_ABCD_LRegClass,
    &X86::GR8_ABCD_HRegClass,
    &X86::GR16_ABCDRegClass,
    &X86::GR32_ABCDRegClass,
    &X86::GR64_ABCDRegClass,
    &X86::GR32_TCRegClass,
    &X86::GR32_NOAX_with_sub_8bit_hiRegClass,
    &X86::GR64_TC_with_sub_8bit_hiRegClass,
    &X86::GR32_ADRegClass,
    &X86::CCRRegClass,
  };
}

  static const TargetRegisterInfoDesc X86RegInfoDesc[] = { // Extra Descriptors
    { 0, 0 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 1, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 1, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 0 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 1, 1 },
    { 0, 0 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 1, 1 },
    { 0, 1 },
    { 1, 1 },
    { 0, 1 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
    { 0, 1 },
  };

  static const char *const X86SubRegIndexTable[] = { "sub_8bit", "sub_8bit_hi", "sub_16bit", "sub_32bit", "sub_sd", "sub_ss", "sub_xmm" };


unsigned X86GenRegisterInfo::getSubReg(unsigned RegNo, unsigned Index) const {
  switch (RegNo) {
  default:
    return 0;
  case X86::AX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::AL;
    case X86::sub_8bit_hi: return X86::AH;
    };
    break;
  case X86::BP:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::BPL;
    };
    break;
  case X86::BX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::BL;
    case X86::sub_8bit_hi: return X86::BH;
    };
    break;
  case X86::CX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::CL;
    case X86::sub_8bit_hi: return X86::CH;
    };
    break;
  case X86::DI:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::DIL;
    };
    break;
  case X86::DX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::DL;
    case X86::sub_8bit_hi: return X86::DH;
    };
    break;
  case X86::EAX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::AL;
    case X86::sub_8bit_hi: return X86::AH;
    case X86::sub_16bit: return X86::AX;
    };
    break;
  case X86::EBP:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::BPL;
    case X86::sub_16bit: return X86::BP;
    };
    break;
  case X86::EBX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::BL;
    case X86::sub_8bit_hi: return X86::BH;
    case X86::sub_16bit: return X86::BX;
    };
    break;
  case X86::ECX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::CL;
    case X86::sub_8bit_hi: return X86::CH;
    case X86::sub_16bit: return X86::CX;
    };
    break;
  case X86::EDI:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::DIL;
    case X86::sub_16bit: return X86::DI;
    };
    break;
  case X86::EDX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::DL;
    case X86::sub_8bit_hi: return X86::DH;
    case X86::sub_16bit: return X86::DX;
    };
    break;
  case X86::EIP:
    switch (Index) {
    default: return 0;
    case X86::sub_16bit: return X86::IP;
    };
    break;
  case X86::ESI:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::SIL;
    case X86::sub_16bit: return X86::SI;
    };
    break;
  case X86::ESP:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::SPL;
    case X86::sub_16bit: return X86::SP;
    };
    break;
  case X86::R8:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R8B;
    case X86::sub_16bit: return X86::R8W;
    case X86::sub_32bit: return X86::R8D;
    };
    break;
  case X86::R8D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R8B;
    case X86::sub_16bit: return X86::R8W;
    };
    break;
  case X86::R8W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R8B;
    };
    break;
  case X86::R9:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R9B;
    case X86::sub_16bit: return X86::R9W;
    case X86::sub_32bit: return X86::R9D;
    };
    break;
  case X86::R9D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R9B;
    case X86::sub_16bit: return X86::R9W;
    };
    break;
  case X86::R9W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R9B;
    };
    break;
  case X86::R10:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R10B;
    case X86::sub_16bit: return X86::R10W;
    case X86::sub_32bit: return X86::R10D;
    };
    break;
  case X86::R10D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R10B;
    case X86::sub_16bit: return X86::R10W;
    };
    break;
  case X86::R10W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R10B;
    };
    break;
  case X86::R11:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R11B;
    case X86::sub_16bit: return X86::R11W;
    case X86::sub_32bit: return X86::R11D;
    };
    break;
  case X86::R11D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R11B;
    case X86::sub_16bit: return X86::R11W;
    };
    break;
  case X86::R11W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R11B;
    };
    break;
  case X86::R12:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R12B;
    case X86::sub_16bit: return X86::R12W;
    case X86::sub_32bit: return X86::R12D;
    };
    break;
  case X86::R12D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R12B;
    case X86::sub_16bit: return X86::R12W;
    };
    break;
  case X86::R12W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R12B;
    };
    break;
  case X86::R13:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R13B;
    case X86::sub_16bit: return X86::R13W;
    case X86::sub_32bit: return X86::R13D;
    };
    break;
  case X86::R13D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R13B;
    case X86::sub_16bit: return X86::R13W;
    };
    break;
  case X86::R13W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R13B;
    };
    break;
  case X86::R14:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R14B;
    case X86::sub_16bit: return X86::R14W;
    case X86::sub_32bit: return X86::R14D;
    };
    break;
  case X86::R14D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R14B;
    case X86::sub_16bit: return X86::R14W;
    };
    break;
  case X86::R14W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R14B;
    };
    break;
  case X86::R15:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R15B;
    case X86::sub_16bit: return X86::R15W;
    case X86::sub_32bit: return X86::R15D;
    };
    break;
  case X86::R15D:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R15B;
    case X86::sub_16bit: return X86::R15W;
    };
    break;
  case X86::R15W:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::R15B;
    };
    break;
  case X86::RAX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::AL;
    case X86::sub_8bit_hi: return X86::AH;
    case X86::sub_16bit: return X86::AX;
    case X86::sub_32bit: return X86::EAX;
    };
    break;
  case X86::RBP:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::BPL;
    case X86::sub_16bit: return X86::BP;
    case X86::sub_32bit: return X86::EBP;
    };
    break;
  case X86::RBX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::BL;
    case X86::sub_8bit_hi: return X86::BH;
    case X86::sub_16bit: return X86::BX;
    case X86::sub_32bit: return X86::EBX;
    };
    break;
  case X86::RCX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::CL;
    case X86::sub_8bit_hi: return X86::CH;
    case X86::sub_16bit: return X86::CX;
    case X86::sub_32bit: return X86::ECX;
    };
    break;
  case X86::RDI:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::DIL;
    case X86::sub_16bit: return X86::DI;
    case X86::sub_32bit: return X86::EDI;
    };
    break;
  case X86::RDX:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::DL;
    case X86::sub_8bit_hi: return X86::DH;
    case X86::sub_16bit: return X86::DX;
    case X86::sub_32bit: return X86::EDX;
    };
    break;
  case X86::RIP:
    switch (Index) {
    default: return 0;
    case X86::sub_16bit: return X86::IP;
    case X86::sub_32bit: return X86::EIP;
    };
    break;
  case X86::RSI:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::SIL;
    case X86::sub_16bit: return X86::SI;
    case X86::sub_32bit: return X86::ESI;
    };
    break;
  case X86::RSP:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::SPL;
    case X86::sub_16bit: return X86::SP;
    case X86::sub_32bit: return X86::ESP;
    };
    break;
  case X86::SI:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::SIL;
    };
    break;
  case X86::SP:
    switch (Index) {
    default: return 0;
    case X86::sub_8bit: return X86::SPL;
    };
    break;
  case X86::XMM0:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM0;
    case X86::sub_ss: return X86::XMM0;
    };
    break;
  case X86::XMM1:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM1;
    case X86::sub_ss: return X86::XMM1;
    };
    break;
  case X86::XMM2:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM2;
    case X86::sub_ss: return X86::XMM2;
    };
    break;
  case X86::XMM3:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM3;
    case X86::sub_ss: return X86::XMM3;
    };
    break;
  case X86::XMM4:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM4;
    case X86::sub_ss: return X86::XMM4;
    };
    break;
  case X86::XMM5:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM5;
    case X86::sub_ss: return X86::XMM5;
    };
    break;
  case X86::XMM6:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM6;
    case X86::sub_ss: return X86::XMM6;
    };
    break;
  case X86::XMM7:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM7;
    case X86::sub_ss: return X86::XMM7;
    };
    break;
  case X86::XMM8:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM8;
    case X86::sub_ss: return X86::XMM8;
    };
    break;
  case X86::XMM9:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM9;
    case X86::sub_ss: return X86::XMM9;
    };
    break;
  case X86::XMM10:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM10;
    case X86::sub_ss: return X86::XMM10;
    };
    break;
  case X86::XMM11:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM11;
    case X86::sub_ss: return X86::XMM11;
    };
    break;
  case X86::XMM12:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM12;
    case X86::sub_ss: return X86::XMM12;
    };
    break;
  case X86::XMM13:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM13;
    case X86::sub_ss: return X86::XMM13;
    };
    break;
  case X86::XMM14:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM14;
    case X86::sub_ss: return X86::XMM14;
    };
    break;
  case X86::XMM15:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM15;
    case X86::sub_ss: return X86::XMM15;
    };
    break;
  case X86::YMM0:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM0;
    case X86::sub_ss: return X86::XMM0;
    case X86::sub_xmm: return X86::XMM0;
    };
    break;
  case X86::YMM1:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM1;
    case X86::sub_ss: return X86::XMM1;
    case X86::sub_xmm: return X86::XMM1;
    };
    break;
  case X86::YMM2:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM2;
    case X86::sub_ss: return X86::XMM2;
    case X86::sub_xmm: return X86::XMM2;
    };
    break;
  case X86::YMM3:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM3;
    case X86::sub_ss: return X86::XMM3;
    case X86::sub_xmm: return X86::XMM3;
    };
    break;
  case X86::YMM4:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM4;
    case X86::sub_ss: return X86::XMM4;
    case X86::sub_xmm: return X86::XMM4;
    };
    break;
  case X86::YMM5:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM5;
    case X86::sub_ss: return X86::XMM5;
    case X86::sub_xmm: return X86::XMM5;
    };
    break;
  case X86::YMM6:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM6;
    case X86::sub_ss: return X86::XMM6;
    case X86::sub_xmm: return X86::XMM6;
    };
    break;
  case X86::YMM7:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM7;
    case X86::sub_ss: return X86::XMM7;
    case X86::sub_xmm: return X86::XMM7;
    };
    break;
  case X86::YMM8:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM8;
    case X86::sub_ss: return X86::XMM8;
    case X86::sub_xmm: return X86::XMM8;
    };
    break;
  case X86::YMM9:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM9;
    case X86::sub_ss: return X86::XMM9;
    case X86::sub_xmm: return X86::XMM9;
    };
    break;
  case X86::YMM10:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM10;
    case X86::sub_ss: return X86::XMM10;
    case X86::sub_xmm: return X86::XMM10;
    };
    break;
  case X86::YMM11:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM11;
    case X86::sub_ss: return X86::XMM11;
    case X86::sub_xmm: return X86::XMM11;
    };
    break;
  case X86::YMM12:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM12;
    case X86::sub_ss: return X86::XMM12;
    case X86::sub_xmm: return X86::XMM12;
    };
    break;
  case X86::YMM13:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM13;
    case X86::sub_ss: return X86::XMM13;
    case X86::sub_xmm: return X86::XMM13;
    };
    break;
  case X86::YMM14:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM14;
    case X86::sub_ss: return X86::XMM14;
    case X86::sub_xmm: return X86::XMM14;
    };
    break;
  case X86::YMM15:
    switch (Index) {
    default: return 0;
    case X86::sub_sd: return X86::XMM15;
    case X86::sub_ss: return X86::XMM15;
    case X86::sub_xmm: return X86::XMM15;
    };
    break;
  };
  return 0;
}

unsigned X86GenRegisterInfo::getSubRegIndex(unsigned RegNo, unsigned SubRegNo) const {
  switch (RegNo) {
  default:
    return 0;
  case X86::AX:
    if (SubRegNo == X86::AL)  return X86::sub_8bit;
    if (SubRegNo == X86::AH)  return X86::sub_8bit_hi;
    return 0;
  case X86::BP:
    if (SubRegNo == X86::BPL)  return X86::sub_8bit;
    return 0;
  case X86::BX:
    if (SubRegNo == X86::BL)  return X86::sub_8bit;
    if (SubRegNo == X86::BH)  return X86::sub_8bit_hi;
    return 0;
  case X86::CX:
    if (SubRegNo == X86::CL)  return X86::sub_8bit;
    if (SubRegNo == X86::CH)  return X86::sub_8bit_hi;
    return 0;
  case X86::DI:
    if (SubRegNo == X86::DIL)  return X86::sub_8bit;
    return 0;
  case X86::DX:
    if (SubRegNo == X86::DL)  return X86::sub_8bit;
    if (SubRegNo == X86::DH)  return X86::sub_8bit_hi;
    return 0;
  case X86::EAX:
    if (SubRegNo == X86::AL)  return X86::sub_8bit;
    if (SubRegNo == X86::AH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::AX)  return X86::sub_16bit;
    return 0;
  case X86::EBP:
    if (SubRegNo == X86::BPL)  return X86::sub_8bit;
    if (SubRegNo == X86::BP)  return X86::sub_16bit;
    return 0;
  case X86::EBX:
    if (SubRegNo == X86::BL)  return X86::sub_8bit;
    if (SubRegNo == X86::BH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::BX)  return X86::sub_16bit;
    return 0;
  case X86::ECX:
    if (SubRegNo == X86::CL)  return X86::sub_8bit;
    if (SubRegNo == X86::CH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::CX)  return X86::sub_16bit;
    return 0;
  case X86::EDI:
    if (SubRegNo == X86::DIL)  return X86::sub_8bit;
    if (SubRegNo == X86::DI)  return X86::sub_16bit;
    return 0;
  case X86::EDX:
    if (SubRegNo == X86::DL)  return X86::sub_8bit;
    if (SubRegNo == X86::DH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::DX)  return X86::sub_16bit;
    return 0;
  case X86::EIP:
    if (SubRegNo == X86::IP)  return X86::sub_16bit;
    return 0;
  case X86::ESI:
    if (SubRegNo == X86::SIL)  return X86::sub_8bit;
    if (SubRegNo == X86::SI)  return X86::sub_16bit;
    return 0;
  case X86::ESP:
    if (SubRegNo == X86::SPL)  return X86::sub_8bit;
    if (SubRegNo == X86::SP)  return X86::sub_16bit;
    return 0;
  case X86::R8:
    if (SubRegNo == X86::R8B)  return X86::sub_8bit;
    if (SubRegNo == X86::R8W)  return X86::sub_16bit;
    if (SubRegNo == X86::R8D)  return X86::sub_32bit;
    return 0;
  case X86::R8D:
    if (SubRegNo == X86::R8B)  return X86::sub_8bit;
    if (SubRegNo == X86::R8W)  return X86::sub_16bit;
    return 0;
  case X86::R8W:
    if (SubRegNo == X86::R8B)  return X86::sub_8bit;
    return 0;
  case X86::R9:
    if (SubRegNo == X86::R9B)  return X86::sub_8bit;
    if (SubRegNo == X86::R9W)  return X86::sub_16bit;
    if (SubRegNo == X86::R9D)  return X86::sub_32bit;
    return 0;
  case X86::R9D:
    if (SubRegNo == X86::R9B)  return X86::sub_8bit;
    if (SubRegNo == X86::R9W)  return X86::sub_16bit;
    return 0;
  case X86::R9W:
    if (SubRegNo == X86::R9B)  return X86::sub_8bit;
    return 0;
  case X86::R10:
    if (SubRegNo == X86::R10B)  return X86::sub_8bit;
    if (SubRegNo == X86::R10W)  return X86::sub_16bit;
    if (SubRegNo == X86::R10D)  return X86::sub_32bit;
    return 0;
  case X86::R10D:
    if (SubRegNo == X86::R10B)  return X86::sub_8bit;
    if (SubRegNo == X86::R10W)  return X86::sub_16bit;
    return 0;
  case X86::R10W:
    if (SubRegNo == X86::R10B)  return X86::sub_8bit;
    return 0;
  case X86::R11:
    if (SubRegNo == X86::R11B)  return X86::sub_8bit;
    if (SubRegNo == X86::R11W)  return X86::sub_16bit;
    if (SubRegNo == X86::R11D)  return X86::sub_32bit;
    return 0;
  case X86::R11D:
    if (SubRegNo == X86::R11B)  return X86::sub_8bit;
    if (SubRegNo == X86::R11W)  return X86::sub_16bit;
    return 0;
  case X86::R11W:
    if (SubRegNo == X86::R11B)  return X86::sub_8bit;
    return 0;
  case X86::R12:
    if (SubRegNo == X86::R12B)  return X86::sub_8bit;
    if (SubRegNo == X86::R12W)  return X86::sub_16bit;
    if (SubRegNo == X86::R12D)  return X86::sub_32bit;
    return 0;
  case X86::R12D:
    if (SubRegNo == X86::R12B)  return X86::sub_8bit;
    if (SubRegNo == X86::R12W)  return X86::sub_16bit;
    return 0;
  case X86::R12W:
    if (SubRegNo == X86::R12B)  return X86::sub_8bit;
    return 0;
  case X86::R13:
    if (SubRegNo == X86::R13B)  return X86::sub_8bit;
    if (SubRegNo == X86::R13W)  return X86::sub_16bit;
    if (SubRegNo == X86::R13D)  return X86::sub_32bit;
    return 0;
  case X86::R13D:
    if (SubRegNo == X86::R13B)  return X86::sub_8bit;
    if (SubRegNo == X86::R13W)  return X86::sub_16bit;
    return 0;
  case X86::R13W:
    if (SubRegNo == X86::R13B)  return X86::sub_8bit;
    return 0;
  case X86::R14:
    if (SubRegNo == X86::R14B)  return X86::sub_8bit;
    if (SubRegNo == X86::R14W)  return X86::sub_16bit;
    if (SubRegNo == X86::R14D)  return X86::sub_32bit;
    return 0;
  case X86::R14D:
    if (SubRegNo == X86::R14B)  return X86::sub_8bit;
    if (SubRegNo == X86::R14W)  return X86::sub_16bit;
    return 0;
  case X86::R14W:
    if (SubRegNo == X86::R14B)  return X86::sub_8bit;
    return 0;
  case X86::R15:
    if (SubRegNo == X86::R15B)  return X86::sub_8bit;
    if (SubRegNo == X86::R15W)  return X86::sub_16bit;
    if (SubRegNo == X86::R15D)  return X86::sub_32bit;
    return 0;
  case X86::R15D:
    if (SubRegNo == X86::R15B)  return X86::sub_8bit;
    if (SubRegNo == X86::R15W)  return X86::sub_16bit;
    return 0;
  case X86::R15W:
    if (SubRegNo == X86::R15B)  return X86::sub_8bit;
    return 0;
  case X86::RAX:
    if (SubRegNo == X86::AL)  return X86::sub_8bit;
    if (SubRegNo == X86::AH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::AX)  return X86::sub_16bit;
    if (SubRegNo == X86::EAX)  return X86::sub_32bit;
    return 0;
  case X86::RBP:
    if (SubRegNo == X86::BPL)  return X86::sub_8bit;
    if (SubRegNo == X86::BP)  return X86::sub_16bit;
    if (SubRegNo == X86::EBP)  return X86::sub_32bit;
    return 0;
  case X86::RBX:
    if (SubRegNo == X86::BL)  return X86::sub_8bit;
    if (SubRegNo == X86::BH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::BX)  return X86::sub_16bit;
    if (SubRegNo == X86::EBX)  return X86::sub_32bit;
    return 0;
  case X86::RCX:
    if (SubRegNo == X86::CL)  return X86::sub_8bit;
    if (SubRegNo == X86::CH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::CX)  return X86::sub_16bit;
    if (SubRegNo == X86::ECX)  return X86::sub_32bit;
    return 0;
  case X86::RDI:
    if (SubRegNo == X86::DIL)  return X86::sub_8bit;
    if (SubRegNo == X86::DI)  return X86::sub_16bit;
    if (SubRegNo == X86::EDI)  return X86::sub_32bit;
    return 0;
  case X86::RDX:
    if (SubRegNo == X86::DL)  return X86::sub_8bit;
    if (SubRegNo == X86::DH)  return X86::sub_8bit_hi;
    if (SubRegNo == X86::DX)  return X86::sub_16bit;
    if (SubRegNo == X86::EDX)  return X86::sub_32bit;
    return 0;
  case X86::RIP:
    if (SubRegNo == X86::IP)  return X86::sub_16bit;
    if (SubRegNo == X86::EIP)  return X86::sub_32bit;
    return 0;
  case X86::RSI:
    if (SubRegNo == X86::SIL)  return X86::sub_8bit;
    if (SubRegNo == X86::SI)  return X86::sub_16bit;
    if (SubRegNo == X86::ESI)  return X86::sub_32bit;
    return 0;
  case X86::RSP:
    if (SubRegNo == X86::SPL)  return X86::sub_8bit;
    if (SubRegNo == X86::SP)  return X86::sub_16bit;
    if (SubRegNo == X86::ESP)  return X86::sub_32bit;
    return 0;
  case X86::SI:
    if (SubRegNo == X86::SIL)  return X86::sub_8bit;
    return 0;
  case X86::SP:
    if (SubRegNo == X86::SPL)  return X86::sub_8bit;
    return 0;
  case X86::XMM0:
    if (SubRegNo == X86::XMM0)  return X86::sub_sd;
    if (SubRegNo == X86::XMM0)  return X86::sub_ss;
    return 0;
  case X86::XMM1:
    if (SubRegNo == X86::XMM1)  return X86::sub_sd;
    if (SubRegNo == X86::XMM1)  return X86::sub_ss;
    return 0;
  case X86::XMM2:
    if (SubRegNo == X86::XMM2)  return X86::sub_sd;
    if (SubRegNo == X86::XMM2)  return X86::sub_ss;
    return 0;
  case X86::XMM3:
    if (SubRegNo == X86::XMM3)  return X86::sub_sd;
    if (SubRegNo == X86::XMM3)  return X86::sub_ss;
    return 0;
  case X86::XMM4:
    if (SubRegNo == X86::XMM4)  return X86::sub_sd;
    if (SubRegNo == X86::XMM4)  return X86::sub_ss;
    return 0;
  case X86::XMM5:
    if (SubRegNo == X86::XMM5)  return X86::sub_sd;
    if (SubRegNo == X86::XMM5)  return X86::sub_ss;
    return 0;
  case X86::XMM6:
    if (SubRegNo == X86::XMM6)  return X86::sub_sd;
    if (SubRegNo == X86::XMM6)  return X86::sub_ss;
    return 0;
  case X86::XMM7:
    if (SubRegNo == X86::XMM7)  return X86::sub_sd;
    if (SubRegNo == X86::XMM7)  return X86::sub_ss;
    return 0;
  case X86::XMM8:
    if (SubRegNo == X86::XMM8)  return X86::sub_sd;
    if (SubRegNo == X86::XMM8)  return X86::sub_ss;
    return 0;
  case X86::XMM9:
    if (SubRegNo == X86::XMM9)  return X86::sub_sd;
    if (SubRegNo == X86::XMM9)  return X86::sub_ss;
    return 0;
  case X86::XMM10:
    if (SubRegNo == X86::XMM10)  return X86::sub_sd;
    if (SubRegNo == X86::XMM10)  return X86::sub_ss;
    return 0;
  case X86::XMM11:
    if (SubRegNo == X86::XMM11)  return X86::sub_sd;
    if (SubRegNo == X86::XMM11)  return X86::sub_ss;
    return 0;
  case X86::XMM12:
    if (SubRegNo == X86::XMM12)  return X86::sub_sd;
    if (SubRegNo == X86::XMM12)  return X86::sub_ss;
    return 0;
  case X86::XMM13:
    if (SubRegNo == X86::XMM13)  return X86::sub_sd;
    if (SubRegNo == X86::XMM13)  return X86::sub_ss;
    return 0;
  case X86::XMM14:
    if (SubRegNo == X86::XMM14)  return X86::sub_sd;
    if (SubRegNo == X86::XMM14)  return X86::sub_ss;
    return 0;
  case X86::XMM15:
    if (SubRegNo == X86::XMM15)  return X86::sub_sd;
    if (SubRegNo == X86::XMM15)  return X86::sub_ss;
    return 0;
  case X86::YMM0:
    if (SubRegNo == X86::XMM0)  return X86::sub_sd;
    if (SubRegNo == X86::XMM0)  return X86::sub_ss;
    if (SubRegNo == X86::XMM0)  return X86::sub_xmm;
    return 0;
  case X86::YMM1:
    if (SubRegNo == X86::XMM1)  return X86::sub_sd;
    if (SubRegNo == X86::XMM1)  return X86::sub_ss;
    if (SubRegNo == X86::XMM1)  return X86::sub_xmm;
    return 0;
  case X86::YMM2:
    if (SubRegNo == X86::XMM2)  return X86::sub_sd;
    if (SubRegNo == X86::XMM2)  return X86::sub_ss;
    if (SubRegNo == X86::XMM2)  return X86::sub_xmm;
    return 0;
  case X86::YMM3:
    if (SubRegNo == X86::XMM3)  return X86::sub_sd;
    if (SubRegNo == X86::XMM3)  return X86::sub_ss;
    if (SubRegNo == X86::XMM3)  return X86::sub_xmm;
    return 0;
  case X86::YMM4:
    if (SubRegNo == X86::XMM4)  return X86::sub_sd;
    if (SubRegNo == X86::XMM4)  return X86::sub_ss;
    if (SubRegNo == X86::XMM4)  return X86::sub_xmm;
    return 0;
  case X86::YMM5:
    if (SubRegNo == X86::XMM5)  return X86::sub_sd;
    if (SubRegNo == X86::XMM5)  return X86::sub_ss;
    if (SubRegNo == X86::XMM5)  return X86::sub_xmm;
    return 0;
  case X86::YMM6:
    if (SubRegNo == X86::XMM6)  return X86::sub_sd;
    if (SubRegNo == X86::XMM6)  return X86::sub_ss;
    if (SubRegNo == X86::XMM6)  return X86::sub_xmm;
    return 0;
  case X86::YMM7:
    if (SubRegNo == X86::XMM7)  return X86::sub_sd;
    if (SubRegNo == X86::XMM7)  return X86::sub_ss;
    if (SubRegNo == X86::XMM7)  return X86::sub_xmm;
    return 0;
  case X86::YMM8:
    if (SubRegNo == X86::XMM8)  return X86::sub_sd;
    if (SubRegNo == X86::XMM8)  return X86::sub_ss;
    if (SubRegNo == X86::XMM8)  return X86::sub_xmm;
    return 0;
  case X86::YMM9:
    if (SubRegNo == X86::XMM9)  return X86::sub_sd;
    if (SubRegNo == X86::XMM9)  return X86::sub_ss;
    if (SubRegNo == X86::XMM9)  return X86::sub_xmm;
    return 0;
  case X86::YMM10:
    if (SubRegNo == X86::XMM10)  return X86::sub_sd;
    if (SubRegNo == X86::XMM10)  return X86::sub_ss;
    if (SubRegNo == X86::XMM10)  return X86::sub_xmm;
    return 0;
  case X86::YMM11:
    if (SubRegNo == X86::XMM11)  return X86::sub_sd;
    if (SubRegNo == X86::XMM11)  return X86::sub_ss;
    if (SubRegNo == X86::XMM11)  return X86::sub_xmm;
    return 0;
  case X86::YMM12:
    if (SubRegNo == X86::XMM12)  return X86::sub_sd;
    if (SubRegNo == X86::XMM12)  return X86::sub_ss;
    if (SubRegNo == X86::XMM12)  return X86::sub_xmm;
    return 0;
  case X86::YMM13:
    if (SubRegNo == X86::XMM13)  return X86::sub_sd;
    if (SubRegNo == X86::XMM13)  return X86::sub_ss;
    if (SubRegNo == X86::XMM13)  return X86::sub_xmm;
    return 0;
  case X86::YMM14:
    if (SubRegNo == X86::XMM14)  return X86::sub_sd;
    if (SubRegNo == X86::XMM14)  return X86::sub_ss;
    if (SubRegNo == X86::XMM14)  return X86::sub_xmm;
    return 0;
  case X86::YMM15:
    if (SubRegNo == X86::XMM15)  return X86::sub_sd;
    if (SubRegNo == X86::XMM15)  return X86::sub_ss;
    if (SubRegNo == X86::XMM15)  return X86::sub_xmm;
    return 0;
  };
  return 0;
}

unsigned X86GenRegisterInfo::composeSubRegIndices(unsigned IdxA, unsigned IdxB) const {
  switch (IdxA) {
  default:
    return IdxB;
  }
}

const TargetRegisterClass *X86GenRegisterInfo::getSubClassWithSubReg(const TargetRegisterClass *RC, unsigned Idx) const {
  static const uint8_t Table[40][7] = {
    {	// GR8
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64
      6,	// sub_8bit -> GR64_with_sub_8bit
      35,	// sub_8bit_hi -> GR64_ABCD
      2,	// sub_16bit -> GR64
      2,	// sub_32bit -> GR64
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR16
      3,	// sub_8bit -> GR16
      33,	// sub_8bit_hi -> GR16_ABCD
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32
      4,	// sub_8bit -> GR32
      34,	// sub_8bit_hi -> GR32_ABCD
      4,	// sub_16bit -> GR32
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// FR32
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      5,	// sub_sd -> FR32
      5,	// sub_ss -> FR32
      0,	// sub_xmm
    },
    {	// GR64_with_sub_8bit
      6,	// sub_8bit -> GR64_with_sub_8bit
      35,	// sub_8bit_hi -> GR64_ABCD
      6,	// sub_16bit -> GR64_with_sub_8bit
      6,	// sub_32bit -> GR64_with_sub_8bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// FR64
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      7,	// sub_sd -> FR64
      7,	// sub_ss -> FR64
      0,	// sub_xmm
    },
    {	// CONTROL_REG
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// VR128
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      9,	// sub_sd -> VR128
      9,	// sub_ss -> VR128
      0,	// sub_xmm
    },
    {	// VR256
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      10,	// sub_sd -> VR256
      10,	// sub_ss -> VR256
      10,	// sub_xmm -> VR256
    },
    {	// GR32_NOSP
      11,	// sub_8bit -> GR32_NOSP
      34,	// sub_8bit_hi -> GR32_ABCD
      11,	// sub_16bit -> GR32_NOSP
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_NOAX
      12,	// sub_8bit -> GR32_NOAX
      37,	// sub_8bit_hi -> GR32_NOAX_with_sub_8bit_hi
      12,	// sub_16bit -> GR32_NOAX
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_NOSP
      13,	// sub_8bit -> GR64_NOSP
      35,	// sub_8bit_hi -> GR64_ABCD
      13,	// sub_16bit -> GR64_NOSP
      13,	// sub_32bit -> GR64_NOSP
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_TC
      21,	// sub_8bit -> GR64_TC_with_sub_8bit
      38,	// sub_8bit_hi -> GR64_TC_with_sub_8bit_hi
      14,	// sub_16bit -> GR64_TC
      14,	// sub_32bit -> GR64_TC
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_NOREX
      22,	// sub_8bit -> GR64_NOREX_with_sub_8bit
      35,	// sub_8bit_hi -> GR64_ABCD
      15,	// sub_16bit -> GR64_NOREX
      15,	// sub_32bit -> GR64_NOREX
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR8_NOREX
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR16_NOREX
      17,	// sub_8bit -> GR16_NOREX
      33,	// sub_8bit_hi -> GR16_ABCD
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_NOREX
      18,	// sub_8bit -> GR32_NOREX
      34,	// sub_8bit_hi -> GR32_ABCD
      18,	// sub_16bit -> GR32_NOREX
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// DEBUG_REG
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// VR64
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_TC_with_sub_8bit
      21,	// sub_8bit -> GR64_TC_with_sub_8bit
      38,	// sub_8bit_hi -> GR64_TC_with_sub_8bit_hi
      21,	// sub_16bit -> GR64_TC_with_sub_8bit
      21,	// sub_32bit -> GR64_TC_with_sub_8bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_NOREX_with_sub_8bit
      22,	// sub_8bit -> GR64_NOREX_with_sub_8bit
      35,	// sub_8bit_hi -> GR64_ABCD
      22,	// sub_16bit -> GR64_NOREX_with_sub_8bit
      22,	// sub_32bit -> GR64_NOREX_with_sub_8bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// RST
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// RFP32
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_NOREX_NOSP
      25,	// sub_8bit -> GR32_NOREX_NOSP
      34,	// sub_8bit_hi -> GR32_ABCD
      25,	// sub_16bit -> GR32_NOREX_NOSP
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// RFP64
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_NOREX_NOSP
      27,	// sub_8bit -> GR64_NOREX_NOSP
      35,	// sub_8bit_hi -> GR64_ABCD
      27,	// sub_16bit -> GR64_NOREX_NOSP
      27,	// sub_32bit -> GR64_NOREX_NOSP
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// RFP80
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// SEGMENT_REG
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_TCW64
      30,	// sub_8bit -> GR64_TCW64
      38,	// sub_8bit_hi -> GR64_TC_with_sub_8bit_hi
      30,	// sub_16bit -> GR64_TCW64
      30,	// sub_32bit -> GR64_TCW64
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR8_ABCD_L
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR8_ABCD_H
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR16_ABCD
      33,	// sub_8bit -> GR16_ABCD
      33,	// sub_8bit_hi -> GR16_ABCD
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_ABCD
      34,	// sub_8bit -> GR32_ABCD
      34,	// sub_8bit_hi -> GR32_ABCD
      34,	// sub_16bit -> GR32_ABCD
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_ABCD
      35,	// sub_8bit -> GR64_ABCD
      35,	// sub_8bit_hi -> GR64_ABCD
      35,	// sub_16bit -> GR64_ABCD
      35,	// sub_32bit -> GR64_ABCD
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_TC
      36,	// sub_8bit -> GR32_TC
      36,	// sub_8bit_hi -> GR32_TC
      36,	// sub_16bit -> GR32_TC
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_NOAX_with_sub_8bit_hi
      37,	// sub_8bit -> GR32_NOAX_with_sub_8bit_hi
      37,	// sub_8bit_hi -> GR32_NOAX_with_sub_8bit_hi
      37,	// sub_16bit -> GR32_NOAX_with_sub_8bit_hi
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR64_TC_with_sub_8bit_hi
      38,	// sub_8bit -> GR64_TC_with_sub_8bit_hi
      38,	// sub_8bit_hi -> GR64_TC_with_sub_8bit_hi
      38,	// sub_16bit -> GR64_TC_with_sub_8bit_hi
      38,	// sub_32bit -> GR64_TC_with_sub_8bit_hi
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// GR32_AD
      39,	// sub_8bit -> GR32_AD
      39,	// sub_8bit_hi -> GR32_AD
      39,	// sub_16bit -> GR32_AD
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
    {	// CCR
      0,	// sub_8bit
      0,	// sub_8bit_hi
      0,	// sub_16bit
      0,	// sub_32bit
      0,	// sub_sd
      0,	// sub_ss
      0,	// sub_xmm
    },
  };
  assert(RC && "Missing regclass");
  if (!Idx) return RC;
  --Idx;
  assert(Idx < 7 && "Bad subreg");
  unsigned TV = Table[RC->getID()][Idx];
  return TV ? getRegClass(TV - 1) : 0;
}

extern MCRegisterDesc X86RegDesc[];
X86GenRegisterInfo::X86GenRegisterInfo(unsigned RA, unsigned DwarfFlavour, unsigned EHFlavour)
  : TargetRegisterInfo(X86RegInfoDesc, RegisterClasses, RegisterClasses+40,
                 X86SubRegIndexTable) {
  InitMCRegisterInfo(X86RegDesc, 160, RA, X86MCRegisterClasses, 40);

  switch (DwarfFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    mapDwarfRegToLLVMReg(41, X86::MM0, false );
    mapDwarfRegToLLVMReg(42, X86::MM1, false );
    mapDwarfRegToLLVMReg(43, X86::MM2, false );
    mapDwarfRegToLLVMReg(44, X86::MM3, false );
    mapDwarfRegToLLVMReg(45, X86::MM4, false );
    mapDwarfRegToLLVMReg(46, X86::MM5, false );
    mapDwarfRegToLLVMReg(47, X86::MM6, false );
    mapDwarfRegToLLVMReg(48, X86::MM7, false );
    mapDwarfRegToLLVMReg(8, X86::R8, false );
    mapDwarfRegToLLVMReg(9, X86::R9, false );
    mapDwarfRegToLLVMReg(10, X86::R10, false );
    mapDwarfRegToLLVMReg(11, X86::R11, false );
    mapDwarfRegToLLVMReg(12, X86::R12, false );
    mapDwarfRegToLLVMReg(13, X86::R13, false );
    mapDwarfRegToLLVMReg(14, X86::R14, false );
    mapDwarfRegToLLVMReg(15, X86::R15, false );
    mapDwarfRegToLLVMReg(0, X86::RAX, false );
    mapDwarfRegToLLVMReg(6, X86::RBP, false );
    mapDwarfRegToLLVMReg(3, X86::RBX, false );
    mapDwarfRegToLLVMReg(2, X86::RCX, false );
    mapDwarfRegToLLVMReg(5, X86::RDI, false );
    mapDwarfRegToLLVMReg(1, X86::RDX, false );
    mapDwarfRegToLLVMReg(16, X86::RIP, false );
    mapDwarfRegToLLVMReg(4, X86::RSI, false );
    mapDwarfRegToLLVMReg(7, X86::RSP, false );
    mapDwarfRegToLLVMReg(33, X86::ST0, false );
    mapDwarfRegToLLVMReg(34, X86::ST1, false );
    mapDwarfRegToLLVMReg(35, X86::ST2, false );
    mapDwarfRegToLLVMReg(36, X86::ST3, false );
    mapDwarfRegToLLVMReg(37, X86::ST4, false );
    mapDwarfRegToLLVMReg(38, X86::ST5, false );
    mapDwarfRegToLLVMReg(39, X86::ST6, false );
    mapDwarfRegToLLVMReg(40, X86::ST7, false );
    mapDwarfRegToLLVMReg(17, X86::XMM0, false );
    mapDwarfRegToLLVMReg(18, X86::XMM1, false );
    mapDwarfRegToLLVMReg(19, X86::XMM2, false );
    mapDwarfRegToLLVMReg(20, X86::XMM3, false );
    mapDwarfRegToLLVMReg(21, X86::XMM4, false );
    mapDwarfRegToLLVMReg(22, X86::XMM5, false );
    mapDwarfRegToLLVMReg(23, X86::XMM6, false );
    mapDwarfRegToLLVMReg(24, X86::XMM7, false );
    mapDwarfRegToLLVMReg(25, X86::XMM8, false );
    mapDwarfRegToLLVMReg(26, X86::XMM9, false );
    mapDwarfRegToLLVMReg(27, X86::XMM10, false );
    mapDwarfRegToLLVMReg(28, X86::XMM11, false );
    mapDwarfRegToLLVMReg(29, X86::XMM12, false );
    mapDwarfRegToLLVMReg(30, X86::XMM13, false );
    mapDwarfRegToLLVMReg(31, X86::XMM14, false );
    mapDwarfRegToLLVMReg(32, X86::XMM15, false );
    break;
  case 1:
    mapDwarfRegToLLVMReg(0, X86::EAX, false );
    mapDwarfRegToLLVMReg(4, X86::EBP, false );
    mapDwarfRegToLLVMReg(3, X86::EBX, false );
    mapDwarfRegToLLVMReg(1, X86::ECX, false );
    mapDwarfRegToLLVMReg(7, X86::EDI, false );
    mapDwarfRegToLLVMReg(2, X86::EDX, false );
    mapDwarfRegToLLVMReg(8, X86::EIP, false );
    mapDwarfRegToLLVMReg(6, X86::ESI, false );
    mapDwarfRegToLLVMReg(5, X86::ESP, false );
    mapDwarfRegToLLVMReg(29, X86::MM0, false );
    mapDwarfRegToLLVMReg(30, X86::MM1, false );
    mapDwarfRegToLLVMReg(31, X86::MM2, false );
    mapDwarfRegToLLVMReg(32, X86::MM3, false );
    mapDwarfRegToLLVMReg(33, X86::MM4, false );
    mapDwarfRegToLLVMReg(34, X86::MM5, false );
    mapDwarfRegToLLVMReg(35, X86::MM6, false );
    mapDwarfRegToLLVMReg(36, X86::MM7, false );
    mapDwarfRegToLLVMReg(12, X86::ST0, false );
    mapDwarfRegToLLVMReg(13, X86::ST1, false );
    mapDwarfRegToLLVMReg(14, X86::ST2, false );
    mapDwarfRegToLLVMReg(15, X86::ST3, false );
    mapDwarfRegToLLVMReg(16, X86::ST4, false );
    mapDwarfRegToLLVMReg(17, X86::ST5, false );
    mapDwarfRegToLLVMReg(18, X86::ST6, false );
    mapDwarfRegToLLVMReg(19, X86::ST7, false );
    mapDwarfRegToLLVMReg(21, X86::XMM0, false );
    mapDwarfRegToLLVMReg(22, X86::XMM1, false );
    mapDwarfRegToLLVMReg(23, X86::XMM2, false );
    mapDwarfRegToLLVMReg(24, X86::XMM3, false );
    mapDwarfRegToLLVMReg(25, X86::XMM4, false );
    mapDwarfRegToLLVMReg(26, X86::XMM5, false );
    mapDwarfRegToLLVMReg(27, X86::XMM6, false );
    mapDwarfRegToLLVMReg(28, X86::XMM7, false );
    break;
  case 2:
    mapDwarfRegToLLVMReg(0, X86::EAX, false );
    mapDwarfRegToLLVMReg(5, X86::EBP, false );
    mapDwarfRegToLLVMReg(3, X86::EBX, false );
    mapDwarfRegToLLVMReg(1, X86::ECX, false );
    mapDwarfRegToLLVMReg(7, X86::EDI, false );
    mapDwarfRegToLLVMReg(2, X86::EDX, false );
    mapDwarfRegToLLVMReg(8, X86::EIP, false );
    mapDwarfRegToLLVMReg(6, X86::ESI, false );
    mapDwarfRegToLLVMReg(4, X86::ESP, false );
    mapDwarfRegToLLVMReg(29, X86::MM0, false );
    mapDwarfRegToLLVMReg(30, X86::MM1, false );
    mapDwarfRegToLLVMReg(31, X86::MM2, false );
    mapDwarfRegToLLVMReg(32, X86::MM3, false );
    mapDwarfRegToLLVMReg(33, X86::MM4, false );
    mapDwarfRegToLLVMReg(34, X86::MM5, false );
    mapDwarfRegToLLVMReg(35, X86::MM6, false );
    mapDwarfRegToLLVMReg(36, X86::MM7, false );
    mapDwarfRegToLLVMReg(11, X86::ST0, false );
    mapDwarfRegToLLVMReg(12, X86::ST1, false );
    mapDwarfRegToLLVMReg(13, X86::ST2, false );
    mapDwarfRegToLLVMReg(14, X86::ST3, false );
    mapDwarfRegToLLVMReg(15, X86::ST4, false );
    mapDwarfRegToLLVMReg(16, X86::ST5, false );
    mapDwarfRegToLLVMReg(17, X86::ST6, false );
    mapDwarfRegToLLVMReg(18, X86::ST7, false );
    mapDwarfRegToLLVMReg(21, X86::XMM0, false );
    mapDwarfRegToLLVMReg(22, X86::XMM1, false );
    mapDwarfRegToLLVMReg(23, X86::XMM2, false );
    mapDwarfRegToLLVMReg(24, X86::XMM3, false );
    mapDwarfRegToLLVMReg(25, X86::XMM4, false );
    mapDwarfRegToLLVMReg(26, X86::XMM5, false );
    mapDwarfRegToLLVMReg(27, X86::XMM6, false );
    mapDwarfRegToLLVMReg(28, X86::XMM7, false );
    break;
  }
  switch (EHFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    mapDwarfRegToLLVMReg(41, X86::MM0, true );
    mapDwarfRegToLLVMReg(42, X86::MM1, true );
    mapDwarfRegToLLVMReg(43, X86::MM2, true );
    mapDwarfRegToLLVMReg(44, X86::MM3, true );
    mapDwarfRegToLLVMReg(45, X86::MM4, true );
    mapDwarfRegToLLVMReg(46, X86::MM5, true );
    mapDwarfRegToLLVMReg(47, X86::MM6, true );
    mapDwarfRegToLLVMReg(48, X86::MM7, true );
    mapDwarfRegToLLVMReg(8, X86::R8, true );
    mapDwarfRegToLLVMReg(9, X86::R9, true );
    mapDwarfRegToLLVMReg(10, X86::R10, true );
    mapDwarfRegToLLVMReg(11, X86::R11, true );
    mapDwarfRegToLLVMReg(12, X86::R12, true );
    mapDwarfRegToLLVMReg(13, X86::R13, true );
    mapDwarfRegToLLVMReg(14, X86::R14, true );
    mapDwarfRegToLLVMReg(15, X86::R15, true );
    mapDwarfRegToLLVMReg(0, X86::RAX, true );
    mapDwarfRegToLLVMReg(6, X86::RBP, true );
    mapDwarfRegToLLVMReg(3, X86::RBX, true );
    mapDwarfRegToLLVMReg(2, X86::RCX, true );
    mapDwarfRegToLLVMReg(5, X86::RDI, true );
    mapDwarfRegToLLVMReg(1, X86::RDX, true );
    mapDwarfRegToLLVMReg(16, X86::RIP, true );
    mapDwarfRegToLLVMReg(4, X86::RSI, true );
    mapDwarfRegToLLVMReg(7, X86::RSP, true );
    mapDwarfRegToLLVMReg(33, X86::ST0, true );
    mapDwarfRegToLLVMReg(34, X86::ST1, true );
    mapDwarfRegToLLVMReg(35, X86::ST2, true );
    mapDwarfRegToLLVMReg(36, X86::ST3, true );
    mapDwarfRegToLLVMReg(37, X86::ST4, true );
    mapDwarfRegToLLVMReg(38, X86::ST5, true );
    mapDwarfRegToLLVMReg(39, X86::ST6, true );
    mapDwarfRegToLLVMReg(40, X86::ST7, true );
    mapDwarfRegToLLVMReg(17, X86::XMM0, true );
    mapDwarfRegToLLVMReg(18, X86::XMM1, true );
    mapDwarfRegToLLVMReg(19, X86::XMM2, true );
    mapDwarfRegToLLVMReg(20, X86::XMM3, true );
    mapDwarfRegToLLVMReg(21, X86::XMM4, true );
    mapDwarfRegToLLVMReg(22, X86::XMM5, true );
    mapDwarfRegToLLVMReg(23, X86::XMM6, true );
    mapDwarfRegToLLVMReg(24, X86::XMM7, true );
    mapDwarfRegToLLVMReg(25, X86::XMM8, true );
    mapDwarfRegToLLVMReg(26, X86::XMM9, true );
    mapDwarfRegToLLVMReg(27, X86::XMM10, true );
    mapDwarfRegToLLVMReg(28, X86::XMM11, true );
    mapDwarfRegToLLVMReg(29, X86::XMM12, true );
    mapDwarfRegToLLVMReg(30, X86::XMM13, true );
    mapDwarfRegToLLVMReg(31, X86::XMM14, true );
    mapDwarfRegToLLVMReg(32, X86::XMM15, true );
    break;
  case 1:
    mapDwarfRegToLLVMReg(0, X86::EAX, true );
    mapDwarfRegToLLVMReg(4, X86::EBP, true );
    mapDwarfRegToLLVMReg(3, X86::EBX, true );
    mapDwarfRegToLLVMReg(1, X86::ECX, true );
    mapDwarfRegToLLVMReg(7, X86::EDI, true );
    mapDwarfRegToLLVMReg(2, X86::EDX, true );
    mapDwarfRegToLLVMReg(8, X86::EIP, true );
    mapDwarfRegToLLVMReg(6, X86::ESI, true );
    mapDwarfRegToLLVMReg(5, X86::ESP, true );
    mapDwarfRegToLLVMReg(29, X86::MM0, true );
    mapDwarfRegToLLVMReg(30, X86::MM1, true );
    mapDwarfRegToLLVMReg(31, X86::MM2, true );
    mapDwarfRegToLLVMReg(32, X86::MM3, true );
    mapDwarfRegToLLVMReg(33, X86::MM4, true );
    mapDwarfRegToLLVMReg(34, X86::MM5, true );
    mapDwarfRegToLLVMReg(35, X86::MM6, true );
    mapDwarfRegToLLVMReg(36, X86::MM7, true );
    mapDwarfRegToLLVMReg(12, X86::ST0, true );
    mapDwarfRegToLLVMReg(13, X86::ST1, true );
    mapDwarfRegToLLVMReg(14, X86::ST2, true );
    mapDwarfRegToLLVMReg(15, X86::ST3, true );
    mapDwarfRegToLLVMReg(16, X86::ST4, true );
    mapDwarfRegToLLVMReg(17, X86::ST5, true );
    mapDwarfRegToLLVMReg(18, X86::ST6, true );
    mapDwarfRegToLLVMReg(19, X86::ST7, true );
    mapDwarfRegToLLVMReg(21, X86::XMM0, true );
    mapDwarfRegToLLVMReg(22, X86::XMM1, true );
    mapDwarfRegToLLVMReg(23, X86::XMM2, true );
    mapDwarfRegToLLVMReg(24, X86::XMM3, true );
    mapDwarfRegToLLVMReg(25, X86::XMM4, true );
    mapDwarfRegToLLVMReg(26, X86::XMM5, true );
    mapDwarfRegToLLVMReg(27, X86::XMM6, true );
    mapDwarfRegToLLVMReg(28, X86::XMM7, true );
    break;
  case 2:
    mapDwarfRegToLLVMReg(0, X86::EAX, true );
    mapDwarfRegToLLVMReg(5, X86::EBP, true );
    mapDwarfRegToLLVMReg(3, X86::EBX, true );
    mapDwarfRegToLLVMReg(1, X86::ECX, true );
    mapDwarfRegToLLVMReg(7, X86::EDI, true );
    mapDwarfRegToLLVMReg(2, X86::EDX, true );
    mapDwarfRegToLLVMReg(8, X86::EIP, true );
    mapDwarfRegToLLVMReg(6, X86::ESI, true );
    mapDwarfRegToLLVMReg(4, X86::ESP, true );
    mapDwarfRegToLLVMReg(29, X86::MM0, true );
    mapDwarfRegToLLVMReg(30, X86::MM1, true );
    mapDwarfRegToLLVMReg(31, X86::MM2, true );
    mapDwarfRegToLLVMReg(32, X86::MM3, true );
    mapDwarfRegToLLVMReg(33, X86::MM4, true );
    mapDwarfRegToLLVMReg(34, X86::MM5, true );
    mapDwarfRegToLLVMReg(35, X86::MM6, true );
    mapDwarfRegToLLVMReg(36, X86::MM7, true );
    mapDwarfRegToLLVMReg(11, X86::ST0, true );
    mapDwarfRegToLLVMReg(12, X86::ST1, true );
    mapDwarfRegToLLVMReg(13, X86::ST2, true );
    mapDwarfRegToLLVMReg(14, X86::ST3, true );
    mapDwarfRegToLLVMReg(15, X86::ST4, true );
    mapDwarfRegToLLVMReg(16, X86::ST5, true );
    mapDwarfRegToLLVMReg(17, X86::ST6, true );
    mapDwarfRegToLLVMReg(18, X86::ST7, true );
    mapDwarfRegToLLVMReg(21, X86::XMM0, true );
    mapDwarfRegToLLVMReg(22, X86::XMM1, true );
    mapDwarfRegToLLVMReg(23, X86::XMM2, true );
    mapDwarfRegToLLVMReg(24, X86::XMM3, true );
    mapDwarfRegToLLVMReg(25, X86::XMM4, true );
    mapDwarfRegToLLVMReg(26, X86::XMM5, true );
    mapDwarfRegToLLVMReg(27, X86::XMM6, true );
    mapDwarfRegToLLVMReg(28, X86::XMM7, true );
    break;
  }
  switch (DwarfFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    mapLLVMRegToDwarfReg(X86::AH, -1, false );
    mapLLVMRegToDwarfReg(X86::AL, -1, false );
    mapLLVMRegToDwarfReg(X86::AX, -1, false );
    mapLLVMRegToDwarfReg(X86::BH, -1, false );
    mapLLVMRegToDwarfReg(X86::BL, -1, false );
    mapLLVMRegToDwarfReg(X86::BP, -1, false );
    mapLLVMRegToDwarfReg(X86::BPL, -1, false );
    mapLLVMRegToDwarfReg(X86::BX, -1, false );
    mapLLVMRegToDwarfReg(X86::CH, -1, false );
    mapLLVMRegToDwarfReg(X86::CL, -1, false );
    mapLLVMRegToDwarfReg(X86::CR0, -1, false );
    mapLLVMRegToDwarfReg(X86::CR1, -1, false );
    mapLLVMRegToDwarfReg(X86::CR2, -1, false );
    mapLLVMRegToDwarfReg(X86::CR3, -1, false );
    mapLLVMRegToDwarfReg(X86::CR4, -1, false );
    mapLLVMRegToDwarfReg(X86::CR5, -1, false );
    mapLLVMRegToDwarfReg(X86::CR6, -1, false );
    mapLLVMRegToDwarfReg(X86::CR7, -1, false );
    mapLLVMRegToDwarfReg(X86::CR8, -1, false );
    mapLLVMRegToDwarfReg(X86::CR9, -1, false );
    mapLLVMRegToDwarfReg(X86::CR10, -1, false );
    mapLLVMRegToDwarfReg(X86::CR11, -1, false );
    mapLLVMRegToDwarfReg(X86::CR12, -1, false );
    mapLLVMRegToDwarfReg(X86::CR13, -1, false );
    mapLLVMRegToDwarfReg(X86::CR14, -1, false );
    mapLLVMRegToDwarfReg(X86::CR15, -1, false );
    mapLLVMRegToDwarfReg(X86::CS, -1, false );
    mapLLVMRegToDwarfReg(X86::CX, -1, false );
    mapLLVMRegToDwarfReg(X86::DH, -1, false );
    mapLLVMRegToDwarfReg(X86::DI, -1, false );
    mapLLVMRegToDwarfReg(X86::DIL, -1, false );
    mapLLVMRegToDwarfReg(X86::DL, -1, false );
    mapLLVMRegToDwarfReg(X86::DR0, -1, false );
    mapLLVMRegToDwarfReg(X86::DR1, -1, false );
    mapLLVMRegToDwarfReg(X86::DR2, -1, false );
    mapLLVMRegToDwarfReg(X86::DR3, -1, false );
    mapLLVMRegToDwarfReg(X86::DR4, -1, false );
    mapLLVMRegToDwarfReg(X86::DR5, -1, false );
    mapLLVMRegToDwarfReg(X86::DR6, -1, false );
    mapLLVMRegToDwarfReg(X86::DR7, -1, false );
    mapLLVMRegToDwarfReg(X86::DS, -1, false );
    mapLLVMRegToDwarfReg(X86::DX, -1, false );
    mapLLVMRegToDwarfReg(X86::EAX, -2, false );
    mapLLVMRegToDwarfReg(X86::EBP, -2, false );
    mapLLVMRegToDwarfReg(X86::EBX, -2, false );
    mapLLVMRegToDwarfReg(X86::ECX, -2, false );
    mapLLVMRegToDwarfReg(X86::EDI, -2, false );
    mapLLVMRegToDwarfReg(X86::EDX, -2, false );
    mapLLVMRegToDwarfReg(X86::EFLAGS, -1, false );
    mapLLVMRegToDwarfReg(X86::EIP, -2, false );
    mapLLVMRegToDwarfReg(X86::EIZ, -1, false );
    mapLLVMRegToDwarfReg(X86::ES, -1, false );
    mapLLVMRegToDwarfReg(X86::ESI, -2, false );
    mapLLVMRegToDwarfReg(X86::ESP, -2, false );
    mapLLVMRegToDwarfReg(X86::FP0, -1, false );
    mapLLVMRegToDwarfReg(X86::FP1, -1, false );
    mapLLVMRegToDwarfReg(X86::FP2, -1, false );
    mapLLVMRegToDwarfReg(X86::FP3, -1, false );
    mapLLVMRegToDwarfReg(X86::FP4, -1, false );
    mapLLVMRegToDwarfReg(X86::FP5, -1, false );
    mapLLVMRegToDwarfReg(X86::FP6, -1, false );
    mapLLVMRegToDwarfReg(X86::FS, -1, false );
    mapLLVMRegToDwarfReg(X86::GS, -1, false );
    mapLLVMRegToDwarfReg(X86::IP, -1, false );
    mapLLVMRegToDwarfReg(X86::MM0, 41, false );
    mapLLVMRegToDwarfReg(X86::MM1, 42, false );
    mapLLVMRegToDwarfReg(X86::MM2, 43, false );
    mapLLVMRegToDwarfReg(X86::MM3, 44, false );
    mapLLVMRegToDwarfReg(X86::MM4, 45, false );
    mapLLVMRegToDwarfReg(X86::MM5, 46, false );
    mapLLVMRegToDwarfReg(X86::MM6, 47, false );
    mapLLVMRegToDwarfReg(X86::MM7, 48, false );
    mapLLVMRegToDwarfReg(X86::R8, 8, false );
    mapLLVMRegToDwarfReg(X86::R8B, -1, false );
    mapLLVMRegToDwarfReg(X86::R8D, -1, false );
    mapLLVMRegToDwarfReg(X86::R8W, -1, false );
    mapLLVMRegToDwarfReg(X86::R9, 9, false );
    mapLLVMRegToDwarfReg(X86::R9B, -1, false );
    mapLLVMRegToDwarfReg(X86::R9D, -1, false );
    mapLLVMRegToDwarfReg(X86::R9W, -1, false );
    mapLLVMRegToDwarfReg(X86::R10, 10, false );
    mapLLVMRegToDwarfReg(X86::R10B, -1, false );
    mapLLVMRegToDwarfReg(X86::R10D, -1, false );
    mapLLVMRegToDwarfReg(X86::R10W, -1, false );
    mapLLVMRegToDwarfReg(X86::R11, 11, false );
    mapLLVMRegToDwarfReg(X86::R11B, -1, false );
    mapLLVMRegToDwarfReg(X86::R11D, -1, false );
    mapLLVMRegToDwarfReg(X86::R11W, -1, false );
    mapLLVMRegToDwarfReg(X86::R12, 12, false );
    mapLLVMRegToDwarfReg(X86::R12B, -1, false );
    mapLLVMRegToDwarfReg(X86::R12D, -1, false );
    mapLLVMRegToDwarfReg(X86::R12W, -1, false );
    mapLLVMRegToDwarfReg(X86::R13, 13, false );
    mapLLVMRegToDwarfReg(X86::R13B, -1, false );
    mapLLVMRegToDwarfReg(X86::R13D, -1, false );
    mapLLVMRegToDwarfReg(X86::R13W, -1, false );
    mapLLVMRegToDwarfReg(X86::R14, 14, false );
    mapLLVMRegToDwarfReg(X86::R14B, -1, false );
    mapLLVMRegToDwarfReg(X86::R14D, -1, false );
    mapLLVMRegToDwarfReg(X86::R14W, -1, false );
    mapLLVMRegToDwarfReg(X86::R15, 15, false );
    mapLLVMRegToDwarfReg(X86::R15B, -1, false );
    mapLLVMRegToDwarfReg(X86::R15D, -1, false );
    mapLLVMRegToDwarfReg(X86::R15W, -1, false );
    mapLLVMRegToDwarfReg(X86::RAX, 0, false );
    mapLLVMRegToDwarfReg(X86::RBP, 6, false );
    mapLLVMRegToDwarfReg(X86::RBX, 3, false );
    mapLLVMRegToDwarfReg(X86::RCX, 2, false );
    mapLLVMRegToDwarfReg(X86::RDI, 5, false );
    mapLLVMRegToDwarfReg(X86::RDX, 1, false );
    mapLLVMRegToDwarfReg(X86::RIP, 16, false );
    mapLLVMRegToDwarfReg(X86::RIZ, -1, false );
    mapLLVMRegToDwarfReg(X86::RSI, 4, false );
    mapLLVMRegToDwarfReg(X86::RSP, 7, false );
    mapLLVMRegToDwarfReg(X86::SI, -1, false );
    mapLLVMRegToDwarfReg(X86::SIL, -1, false );
    mapLLVMRegToDwarfReg(X86::SP, -1, false );
    mapLLVMRegToDwarfReg(X86::SPL, -1, false );
    mapLLVMRegToDwarfReg(X86::SS, -1, false );
    mapLLVMRegToDwarfReg(X86::ST0, 33, false );
    mapLLVMRegToDwarfReg(X86::ST1, 34, false );
    mapLLVMRegToDwarfReg(X86::ST2, 35, false );
    mapLLVMRegToDwarfReg(X86::ST3, 36, false );
    mapLLVMRegToDwarfReg(X86::ST4, 37, false );
    mapLLVMRegToDwarfReg(X86::ST5, 38, false );
    mapLLVMRegToDwarfReg(X86::ST6, 39, false );
    mapLLVMRegToDwarfReg(X86::ST7, 40, false );
    mapLLVMRegToDwarfReg(X86::XMM0, 17, false );
    mapLLVMRegToDwarfReg(X86::XMM1, 18, false );
    mapLLVMRegToDwarfReg(X86::XMM2, 19, false );
    mapLLVMRegToDwarfReg(X86::XMM3, 20, false );
    mapLLVMRegToDwarfReg(X86::XMM4, 21, false );
    mapLLVMRegToDwarfReg(X86::XMM5, 22, false );
    mapLLVMRegToDwarfReg(X86::XMM6, 23, false );
    mapLLVMRegToDwarfReg(X86::XMM7, 24, false );
    mapLLVMRegToDwarfReg(X86::XMM8, 25, false );
    mapLLVMRegToDwarfReg(X86::XMM9, 26, false );
    mapLLVMRegToDwarfReg(X86::XMM10, 27, false );
    mapLLVMRegToDwarfReg(X86::XMM11, 28, false );
    mapLLVMRegToDwarfReg(X86::XMM12, 29, false );
    mapLLVMRegToDwarfReg(X86::XMM13, 30, false );
    mapLLVMRegToDwarfReg(X86::XMM14, 31, false );
    mapLLVMRegToDwarfReg(X86::XMM15, 32, false );
    mapLLVMRegToDwarfReg(X86::YMM0, 17, false );
    mapLLVMRegToDwarfReg(X86::YMM1, 18, false );
    mapLLVMRegToDwarfReg(X86::YMM2, 19, false );
    mapLLVMRegToDwarfReg(X86::YMM3, 20, false );
    mapLLVMRegToDwarfReg(X86::YMM4, 21, false );
    mapLLVMRegToDwarfReg(X86::YMM5, 22, false );
    mapLLVMRegToDwarfReg(X86::YMM6, 23, false );
    mapLLVMRegToDwarfReg(X86::YMM7, 24, false );
    mapLLVMRegToDwarfReg(X86::YMM8, 25, false );
    mapLLVMRegToDwarfReg(X86::YMM9, 26, false );
    mapLLVMRegToDwarfReg(X86::YMM10, 27, false );
    mapLLVMRegToDwarfReg(X86::YMM11, 28, false );
    mapLLVMRegToDwarfReg(X86::YMM12, 29, false );
    mapLLVMRegToDwarfReg(X86::YMM13, 30, false );
    mapLLVMRegToDwarfReg(X86::YMM14, 31, false );
    mapLLVMRegToDwarfReg(X86::YMM15, 32, false );
    break;
  case 1:
    mapLLVMRegToDwarfReg(X86::AH, -1, false );
    mapLLVMRegToDwarfReg(X86::AL, -1, false );
    mapLLVMRegToDwarfReg(X86::AX, -1, false );
    mapLLVMRegToDwarfReg(X86::BH, -1, false );
    mapLLVMRegToDwarfReg(X86::BL, -1, false );
    mapLLVMRegToDwarfReg(X86::BP, -1, false );
    mapLLVMRegToDwarfReg(X86::BPL, -1, false );
    mapLLVMRegToDwarfReg(X86::BX, -1, false );
    mapLLVMRegToDwarfReg(X86::CH, -1, false );
    mapLLVMRegToDwarfReg(X86::CL, -1, false );
    mapLLVMRegToDwarfReg(X86::CR0, -1, false );
    mapLLVMRegToDwarfReg(X86::CR1, -1, false );
    mapLLVMRegToDwarfReg(X86::CR2, -1, false );
    mapLLVMRegToDwarfReg(X86::CR3, -1, false );
    mapLLVMRegToDwarfReg(X86::CR4, -1, false );
    mapLLVMRegToDwarfReg(X86::CR5, -1, false );
    mapLLVMRegToDwarfReg(X86::CR6, -1, false );
    mapLLVMRegToDwarfReg(X86::CR7, -1, false );
    mapLLVMRegToDwarfReg(X86::CR8, -1, false );
    mapLLVMRegToDwarfReg(X86::CR9, -1, false );
    mapLLVMRegToDwarfReg(X86::CR10, -1, false );
    mapLLVMRegToDwarfReg(X86::CR11, -1, false );
    mapLLVMRegToDwarfReg(X86::CR12, -1, false );
    mapLLVMRegToDwarfReg(X86::CR13, -1, false );
    mapLLVMRegToDwarfReg(X86::CR14, -1, false );
    mapLLVMRegToDwarfReg(X86::CR15, -1, false );
    mapLLVMRegToDwarfReg(X86::CS, -1, false );
    mapLLVMRegToDwarfReg(X86::CX, -1, false );
    mapLLVMRegToDwarfReg(X86::DH, -1, false );
    mapLLVMRegToDwarfReg(X86::DI, -1, false );
    mapLLVMRegToDwarfReg(X86::DIL, -1, false );
    mapLLVMRegToDwarfReg(X86::DL, -1, false );
    mapLLVMRegToDwarfReg(X86::DR0, -1, false );
    mapLLVMRegToDwarfReg(X86::DR1, -1, false );
    mapLLVMRegToDwarfReg(X86::DR2, -1, false );
    mapLLVMRegToDwarfReg(X86::DR3, -1, false );
    mapLLVMRegToDwarfReg(X86::DR4, -1, false );
    mapLLVMRegToDwarfReg(X86::DR5, -1, false );
    mapLLVMRegToDwarfReg(X86::DR6, -1, false );
    mapLLVMRegToDwarfReg(X86::DR7, -1, false );
    mapLLVMRegToDwarfReg(X86::DS, -1, false );
    mapLLVMRegToDwarfReg(X86::DX, -1, false );
    mapLLVMRegToDwarfReg(X86::EAX, 0, false );
    mapLLVMRegToDwarfReg(X86::EBP, 4, false );
    mapLLVMRegToDwarfReg(X86::EBX, 3, false );
    mapLLVMRegToDwarfReg(X86::ECX, 1, false );
    mapLLVMRegToDwarfReg(X86::EDI, 7, false );
    mapLLVMRegToDwarfReg(X86::EDX, 2, false );
    mapLLVMRegToDwarfReg(X86::EFLAGS, -1, false );
    mapLLVMRegToDwarfReg(X86::EIP, 8, false );
    mapLLVMRegToDwarfReg(X86::EIZ, -1, false );
    mapLLVMRegToDwarfReg(X86::ES, -1, false );
    mapLLVMRegToDwarfReg(X86::ESI, 6, false );
    mapLLVMRegToDwarfReg(X86::ESP, 5, false );
    mapLLVMRegToDwarfReg(X86::FP0, -1, false );
    mapLLVMRegToDwarfReg(X86::FP1, -1, false );
    mapLLVMRegToDwarfReg(X86::FP2, -1, false );
    mapLLVMRegToDwarfReg(X86::FP3, -1, false );
    mapLLVMRegToDwarfReg(X86::FP4, -1, false );
    mapLLVMRegToDwarfReg(X86::FP5, -1, false );
    mapLLVMRegToDwarfReg(X86::FP6, -1, false );
    mapLLVMRegToDwarfReg(X86::FS, -1, false );
    mapLLVMRegToDwarfReg(X86::GS, -1, false );
    mapLLVMRegToDwarfReg(X86::IP, -1, false );
    mapLLVMRegToDwarfReg(X86::MM0, 29, false );
    mapLLVMRegToDwarfReg(X86::MM1, 30, false );
    mapLLVMRegToDwarfReg(X86::MM2, 31, false );
    mapLLVMRegToDwarfReg(X86::MM3, 32, false );
    mapLLVMRegToDwarfReg(X86::MM4, 33, false );
    mapLLVMRegToDwarfReg(X86::MM5, 34, false );
    mapLLVMRegToDwarfReg(X86::MM6, 35, false );
    mapLLVMRegToDwarfReg(X86::MM7, 36, false );
    mapLLVMRegToDwarfReg(X86::R8, -2, false );
    mapLLVMRegToDwarfReg(X86::R8B, -1, false );
    mapLLVMRegToDwarfReg(X86::R8D, -1, false );
    mapLLVMRegToDwarfReg(X86::R8W, -1, false );
    mapLLVMRegToDwarfReg(X86::R9, -2, false );
    mapLLVMRegToDwarfReg(X86::R9B, -1, false );
    mapLLVMRegToDwarfReg(X86::R9D, -1, false );
    mapLLVMRegToDwarfReg(X86::R9W, -1, false );
    mapLLVMRegToDwarfReg(X86::R10, -2, false );
    mapLLVMRegToDwarfReg(X86::R10B, -1, false );
    mapLLVMRegToDwarfReg(X86::R10D, -1, false );
    mapLLVMRegToDwarfReg(X86::R10W, -1, false );
    mapLLVMRegToDwarfReg(X86::R11, -2, false );
    mapLLVMRegToDwarfReg(X86::R11B, -1, false );
    mapLLVMRegToDwarfReg(X86::R11D, -1, false );
    mapLLVMRegToDwarfReg(X86::R11W, -1, false );
    mapLLVMRegToDwarfReg(X86::R12, -2, false );
    mapLLVMRegToDwarfReg(X86::R12B, -1, false );
    mapLLVMRegToDwarfReg(X86::R12D, -1, false );
    mapLLVMRegToDwarfReg(X86::R12W, -1, false );
    mapLLVMRegToDwarfReg(X86::R13, -2, false );
    mapLLVMRegToDwarfReg(X86::R13B, -1, false );
    mapLLVMRegToDwarfReg(X86::R13D, -1, false );
    mapLLVMRegToDwarfReg(X86::R13W, -1, false );
    mapLLVMRegToDwarfReg(X86::R14, -2, false );
    mapLLVMRegToDwarfReg(X86::R14B, -1, false );
    mapLLVMRegToDwarfReg(X86::R14D, -1, false );
    mapLLVMRegToDwarfReg(X86::R14W, -1, false );
    mapLLVMRegToDwarfReg(X86::R15, -2, false );
    mapLLVMRegToDwarfReg(X86::R15B, -1, false );
    mapLLVMRegToDwarfReg(X86::R15D, -1, false );
    mapLLVMRegToDwarfReg(X86::R15W, -1, false );
    mapLLVMRegToDwarfReg(X86::RAX, -2, false );
    mapLLVMRegToDwarfReg(X86::RBP, -2, false );
    mapLLVMRegToDwarfReg(X86::RBX, -2, false );
    mapLLVMRegToDwarfReg(X86::RCX, -2, false );
    mapLLVMRegToDwarfReg(X86::RDI, -2, false );
    mapLLVMRegToDwarfReg(X86::RDX, -2, false );
    mapLLVMRegToDwarfReg(X86::RIP, -2, false );
    mapLLVMRegToDwarfReg(X86::RIZ, -1, false );
    mapLLVMRegToDwarfReg(X86::RSI, -2, false );
    mapLLVMRegToDwarfReg(X86::RSP, -2, false );
    mapLLVMRegToDwarfReg(X86::SI, -1, false );
    mapLLVMRegToDwarfReg(X86::SIL, -1, false );
    mapLLVMRegToDwarfReg(X86::SP, -1, false );
    mapLLVMRegToDwarfReg(X86::SPL, -1, false );
    mapLLVMRegToDwarfReg(X86::SS, -1, false );
    mapLLVMRegToDwarfReg(X86::ST0, 12, false );
    mapLLVMRegToDwarfReg(X86::ST1, 13, false );
    mapLLVMRegToDwarfReg(X86::ST2, 14, false );
    mapLLVMRegToDwarfReg(X86::ST3, 15, false );
    mapLLVMRegToDwarfReg(X86::ST4, 16, false );
    mapLLVMRegToDwarfReg(X86::ST5, 17, false );
    mapLLVMRegToDwarfReg(X86::ST6, 18, false );
    mapLLVMRegToDwarfReg(X86::ST7, 19, false );
    mapLLVMRegToDwarfReg(X86::XMM0, 21, false );
    mapLLVMRegToDwarfReg(X86::XMM1, 22, false );
    mapLLVMRegToDwarfReg(X86::XMM2, 23, false );
    mapLLVMRegToDwarfReg(X86::XMM3, 24, false );
    mapLLVMRegToDwarfReg(X86::XMM4, 25, false );
    mapLLVMRegToDwarfReg(X86::XMM5, 26, false );
    mapLLVMRegToDwarfReg(X86::XMM6, 27, false );
    mapLLVMRegToDwarfReg(X86::XMM7, 28, false );
    mapLLVMRegToDwarfReg(X86::XMM8, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM9, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM10, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM11, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM12, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM13, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM14, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM15, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM0, 21, false );
    mapLLVMRegToDwarfReg(X86::YMM1, 22, false );
    mapLLVMRegToDwarfReg(X86::YMM2, 23, false );
    mapLLVMRegToDwarfReg(X86::YMM3, 24, false );
    mapLLVMRegToDwarfReg(X86::YMM4, 25, false );
    mapLLVMRegToDwarfReg(X86::YMM5, 26, false );
    mapLLVMRegToDwarfReg(X86::YMM6, 27, false );
    mapLLVMRegToDwarfReg(X86::YMM7, 28, false );
    mapLLVMRegToDwarfReg(X86::YMM8, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM9, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM10, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM11, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM12, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM13, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM14, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM15, -2, false );
    break;
  case 2:
    mapLLVMRegToDwarfReg(X86::AH, -1, false );
    mapLLVMRegToDwarfReg(X86::AL, -1, false );
    mapLLVMRegToDwarfReg(X86::AX, -1, false );
    mapLLVMRegToDwarfReg(X86::BH, -1, false );
    mapLLVMRegToDwarfReg(X86::BL, -1, false );
    mapLLVMRegToDwarfReg(X86::BP, -1, false );
    mapLLVMRegToDwarfReg(X86::BPL, -1, false );
    mapLLVMRegToDwarfReg(X86::BX, -1, false );
    mapLLVMRegToDwarfReg(X86::CH, -1, false );
    mapLLVMRegToDwarfReg(X86::CL, -1, false );
    mapLLVMRegToDwarfReg(X86::CR0, -1, false );
    mapLLVMRegToDwarfReg(X86::CR1, -1, false );
    mapLLVMRegToDwarfReg(X86::CR2, -1, false );
    mapLLVMRegToDwarfReg(X86::CR3, -1, false );
    mapLLVMRegToDwarfReg(X86::CR4, -1, false );
    mapLLVMRegToDwarfReg(X86::CR5, -1, false );
    mapLLVMRegToDwarfReg(X86::CR6, -1, false );
    mapLLVMRegToDwarfReg(X86::CR7, -1, false );
    mapLLVMRegToDwarfReg(X86::CR8, -1, false );
    mapLLVMRegToDwarfReg(X86::CR9, -1, false );
    mapLLVMRegToDwarfReg(X86::CR10, -1, false );
    mapLLVMRegToDwarfReg(X86::CR11, -1, false );
    mapLLVMRegToDwarfReg(X86::CR12, -1, false );
    mapLLVMRegToDwarfReg(X86::CR13, -1, false );
    mapLLVMRegToDwarfReg(X86::CR14, -1, false );
    mapLLVMRegToDwarfReg(X86::CR15, -1, false );
    mapLLVMRegToDwarfReg(X86::CS, -1, false );
    mapLLVMRegToDwarfReg(X86::CX, -1, false );
    mapLLVMRegToDwarfReg(X86::DH, -1, false );
    mapLLVMRegToDwarfReg(X86::DI, -1, false );
    mapLLVMRegToDwarfReg(X86::DIL, -1, false );
    mapLLVMRegToDwarfReg(X86::DL, -1, false );
    mapLLVMRegToDwarfReg(X86::DR0, -1, false );
    mapLLVMRegToDwarfReg(X86::DR1, -1, false );
    mapLLVMRegToDwarfReg(X86::DR2, -1, false );
    mapLLVMRegToDwarfReg(X86::DR3, -1, false );
    mapLLVMRegToDwarfReg(X86::DR4, -1, false );
    mapLLVMRegToDwarfReg(X86::DR5, -1, false );
    mapLLVMRegToDwarfReg(X86::DR6, -1, false );
    mapLLVMRegToDwarfReg(X86::DR7, -1, false );
    mapLLVMRegToDwarfReg(X86::DS, -1, false );
    mapLLVMRegToDwarfReg(X86::DX, -1, false );
    mapLLVMRegToDwarfReg(X86::EAX, 0, false );
    mapLLVMRegToDwarfReg(X86::EBP, 5, false );
    mapLLVMRegToDwarfReg(X86::EBX, 3, false );
    mapLLVMRegToDwarfReg(X86::ECX, 1, false );
    mapLLVMRegToDwarfReg(X86::EDI, 7, false );
    mapLLVMRegToDwarfReg(X86::EDX, 2, false );
    mapLLVMRegToDwarfReg(X86::EFLAGS, -1, false );
    mapLLVMRegToDwarfReg(X86::EIP, 8, false );
    mapLLVMRegToDwarfReg(X86::EIZ, -1, false );
    mapLLVMRegToDwarfReg(X86::ES, -1, false );
    mapLLVMRegToDwarfReg(X86::ESI, 6, false );
    mapLLVMRegToDwarfReg(X86::ESP, 4, false );
    mapLLVMRegToDwarfReg(X86::FP0, -1, false );
    mapLLVMRegToDwarfReg(X86::FP1, -1, false );
    mapLLVMRegToDwarfReg(X86::FP2, -1, false );
    mapLLVMRegToDwarfReg(X86::FP3, -1, false );
    mapLLVMRegToDwarfReg(X86::FP4, -1, false );
    mapLLVMRegToDwarfReg(X86::FP5, -1, false );
    mapLLVMRegToDwarfReg(X86::FP6, -1, false );
    mapLLVMRegToDwarfReg(X86::FS, -1, false );
    mapLLVMRegToDwarfReg(X86::GS, -1, false );
    mapLLVMRegToDwarfReg(X86::IP, -1, false );
    mapLLVMRegToDwarfReg(X86::MM0, 29, false );
    mapLLVMRegToDwarfReg(X86::MM1, 30, false );
    mapLLVMRegToDwarfReg(X86::MM2, 31, false );
    mapLLVMRegToDwarfReg(X86::MM3, 32, false );
    mapLLVMRegToDwarfReg(X86::MM4, 33, false );
    mapLLVMRegToDwarfReg(X86::MM5, 34, false );
    mapLLVMRegToDwarfReg(X86::MM6, 35, false );
    mapLLVMRegToDwarfReg(X86::MM7, 36, false );
    mapLLVMRegToDwarfReg(X86::R8, -2, false );
    mapLLVMRegToDwarfReg(X86::R8B, -1, false );
    mapLLVMRegToDwarfReg(X86::R8D, -1, false );
    mapLLVMRegToDwarfReg(X86::R8W, -1, false );
    mapLLVMRegToDwarfReg(X86::R9, -2, false );
    mapLLVMRegToDwarfReg(X86::R9B, -1, false );
    mapLLVMRegToDwarfReg(X86::R9D, -1, false );
    mapLLVMRegToDwarfReg(X86::R9W, -1, false );
    mapLLVMRegToDwarfReg(X86::R10, -2, false );
    mapLLVMRegToDwarfReg(X86::R10B, -1, false );
    mapLLVMRegToDwarfReg(X86::R10D, -1, false );
    mapLLVMRegToDwarfReg(X86::R10W, -1, false );
    mapLLVMRegToDwarfReg(X86::R11, -2, false );
    mapLLVMRegToDwarfReg(X86::R11B, -1, false );
    mapLLVMRegToDwarfReg(X86::R11D, -1, false );
    mapLLVMRegToDwarfReg(X86::R11W, -1, false );
    mapLLVMRegToDwarfReg(X86::R12, -2, false );
    mapLLVMRegToDwarfReg(X86::R12B, -1, false );
    mapLLVMRegToDwarfReg(X86::R12D, -1, false );
    mapLLVMRegToDwarfReg(X86::R12W, -1, false );
    mapLLVMRegToDwarfReg(X86::R13, -2, false );
    mapLLVMRegToDwarfReg(X86::R13B, -1, false );
    mapLLVMRegToDwarfReg(X86::R13D, -1, false );
    mapLLVMRegToDwarfReg(X86::R13W, -1, false );
    mapLLVMRegToDwarfReg(X86::R14, -2, false );
    mapLLVMRegToDwarfReg(X86::R14B, -1, false );
    mapLLVMRegToDwarfReg(X86::R14D, -1, false );
    mapLLVMRegToDwarfReg(X86::R14W, -1, false );
    mapLLVMRegToDwarfReg(X86::R15, -2, false );
    mapLLVMRegToDwarfReg(X86::R15B, -1, false );
    mapLLVMRegToDwarfReg(X86::R15D, -1, false );
    mapLLVMRegToDwarfReg(X86::R15W, -1, false );
    mapLLVMRegToDwarfReg(X86::RAX, -2, false );
    mapLLVMRegToDwarfReg(X86::RBP, -2, false );
    mapLLVMRegToDwarfReg(X86::RBX, -2, false );
    mapLLVMRegToDwarfReg(X86::RCX, -2, false );
    mapLLVMRegToDwarfReg(X86::RDI, -2, false );
    mapLLVMRegToDwarfReg(X86::RDX, -2, false );
    mapLLVMRegToDwarfReg(X86::RIP, -2, false );
    mapLLVMRegToDwarfReg(X86::RIZ, -1, false );
    mapLLVMRegToDwarfReg(X86::RSI, -2, false );
    mapLLVMRegToDwarfReg(X86::RSP, -2, false );
    mapLLVMRegToDwarfReg(X86::SI, -1, false );
    mapLLVMRegToDwarfReg(X86::SIL, -1, false );
    mapLLVMRegToDwarfReg(X86::SP, -1, false );
    mapLLVMRegToDwarfReg(X86::SPL, -1, false );
    mapLLVMRegToDwarfReg(X86::SS, -1, false );
    mapLLVMRegToDwarfReg(X86::ST0, 11, false );
    mapLLVMRegToDwarfReg(X86::ST1, 12, false );
    mapLLVMRegToDwarfReg(X86::ST2, 13, false );
    mapLLVMRegToDwarfReg(X86::ST3, 14, false );
    mapLLVMRegToDwarfReg(X86::ST4, 15, false );
    mapLLVMRegToDwarfReg(X86::ST5, 16, false );
    mapLLVMRegToDwarfReg(X86::ST6, 17, false );
    mapLLVMRegToDwarfReg(X86::ST7, 18, false );
    mapLLVMRegToDwarfReg(X86::XMM0, 21, false );
    mapLLVMRegToDwarfReg(X86::XMM1, 22, false );
    mapLLVMRegToDwarfReg(X86::XMM2, 23, false );
    mapLLVMRegToDwarfReg(X86::XMM3, 24, false );
    mapLLVMRegToDwarfReg(X86::XMM4, 25, false );
    mapLLVMRegToDwarfReg(X86::XMM5, 26, false );
    mapLLVMRegToDwarfReg(X86::XMM6, 27, false );
    mapLLVMRegToDwarfReg(X86::XMM7, 28, false );
    mapLLVMRegToDwarfReg(X86::XMM8, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM9, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM10, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM11, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM12, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM13, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM14, -2, false );
    mapLLVMRegToDwarfReg(X86::XMM15, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM0, 21, false );
    mapLLVMRegToDwarfReg(X86::YMM1, 22, false );
    mapLLVMRegToDwarfReg(X86::YMM2, 23, false );
    mapLLVMRegToDwarfReg(X86::YMM3, 24, false );
    mapLLVMRegToDwarfReg(X86::YMM4, 25, false );
    mapLLVMRegToDwarfReg(X86::YMM5, 26, false );
    mapLLVMRegToDwarfReg(X86::YMM6, 27, false );
    mapLLVMRegToDwarfReg(X86::YMM7, 28, false );
    mapLLVMRegToDwarfReg(X86::YMM8, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM9, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM10, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM11, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM12, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM13, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM14, -2, false );
    mapLLVMRegToDwarfReg(X86::YMM15, -2, false );
    break;
  }
  switch (EHFlavour) {
  default:
    assert(0 && "Unknown DWARF flavour");
    break;
  case 0:
    mapLLVMRegToDwarfReg(X86::AH, -1, true );
    mapLLVMRegToDwarfReg(X86::AL, -1, true );
    mapLLVMRegToDwarfReg(X86::AX, -1, true );
    mapLLVMRegToDwarfReg(X86::BH, -1, true );
    mapLLVMRegToDwarfReg(X86::BL, -1, true );
    mapLLVMRegToDwarfReg(X86::BP, -1, true );
    mapLLVMRegToDwarfReg(X86::BPL, -1, true );
    mapLLVMRegToDwarfReg(X86::BX, -1, true );
    mapLLVMRegToDwarfReg(X86::CH, -1, true );
    mapLLVMRegToDwarfReg(X86::CL, -1, true );
    mapLLVMRegToDwarfReg(X86::CR0, -1, true );
    mapLLVMRegToDwarfReg(X86::CR1, -1, true );
    mapLLVMRegToDwarfReg(X86::CR2, -1, true );
    mapLLVMRegToDwarfReg(X86::CR3, -1, true );
    mapLLVMRegToDwarfReg(X86::CR4, -1, true );
    mapLLVMRegToDwarfReg(X86::CR5, -1, true );
    mapLLVMRegToDwarfReg(X86::CR6, -1, true );
    mapLLVMRegToDwarfReg(X86::CR7, -1, true );
    mapLLVMRegToDwarfReg(X86::CR8, -1, true );
    mapLLVMRegToDwarfReg(X86::CR9, -1, true );
    mapLLVMRegToDwarfReg(X86::CR10, -1, true );
    mapLLVMRegToDwarfReg(X86::CR11, -1, true );
    mapLLVMRegToDwarfReg(X86::CR12, -1, true );
    mapLLVMRegToDwarfReg(X86::CR13, -1, true );
    mapLLVMRegToDwarfReg(X86::CR14, -1, true );
    mapLLVMRegToDwarfReg(X86::CR15, -1, true );
    mapLLVMRegToDwarfReg(X86::CS, -1, true );
    mapLLVMRegToDwarfReg(X86::CX, -1, true );
    mapLLVMRegToDwarfReg(X86::DH, -1, true );
    mapLLVMRegToDwarfReg(X86::DI, -1, true );
    mapLLVMRegToDwarfReg(X86::DIL, -1, true );
    mapLLVMRegToDwarfReg(X86::DL, -1, true );
    mapLLVMRegToDwarfReg(X86::DR0, -1, true );
    mapLLVMRegToDwarfReg(X86::DR1, -1, true );
    mapLLVMRegToDwarfReg(X86::DR2, -1, true );
    mapLLVMRegToDwarfReg(X86::DR3, -1, true );
    mapLLVMRegToDwarfReg(X86::DR4, -1, true );
    mapLLVMRegToDwarfReg(X86::DR5, -1, true );
    mapLLVMRegToDwarfReg(X86::DR6, -1, true );
    mapLLVMRegToDwarfReg(X86::DR7, -1, true );
    mapLLVMRegToDwarfReg(X86::DS, -1, true );
    mapLLVMRegToDwarfReg(X86::DX, -1, true );
    mapLLVMRegToDwarfReg(X86::EAX, -2, true );
    mapLLVMRegToDwarfReg(X86::EBP, -2, true );
    mapLLVMRegToDwarfReg(X86::EBX, -2, true );
    mapLLVMRegToDwarfReg(X86::ECX, -2, true );
    mapLLVMRegToDwarfReg(X86::EDI, -2, true );
    mapLLVMRegToDwarfReg(X86::EDX, -2, true );
    mapLLVMRegToDwarfReg(X86::EFLAGS, -1, true );
    mapLLVMRegToDwarfReg(X86::EIP, -2, true );
    mapLLVMRegToDwarfReg(X86::EIZ, -1, true );
    mapLLVMRegToDwarfReg(X86::ES, -1, true );
    mapLLVMRegToDwarfReg(X86::ESI, -2, true );
    mapLLVMRegToDwarfReg(X86::ESP, -2, true );
    mapLLVMRegToDwarfReg(X86::FP0, -1, true );
    mapLLVMRegToDwarfReg(X86::FP1, -1, true );
    mapLLVMRegToDwarfReg(X86::FP2, -1, true );
    mapLLVMRegToDwarfReg(X86::FP3, -1, true );
    mapLLVMRegToDwarfReg(X86::FP4, -1, true );
    mapLLVMRegToDwarfReg(X86::FP5, -1, true );
    mapLLVMRegToDwarfReg(X86::FP6, -1, true );
    mapLLVMRegToDwarfReg(X86::FS, -1, true );
    mapLLVMRegToDwarfReg(X86::GS, -1, true );
    mapLLVMRegToDwarfReg(X86::IP, -1, true );
    mapLLVMRegToDwarfReg(X86::MM0, 41, true );
    mapLLVMRegToDwarfReg(X86::MM1, 42, true );
    mapLLVMRegToDwarfReg(X86::MM2, 43, true );
    mapLLVMRegToDwarfReg(X86::MM3, 44, true );
    mapLLVMRegToDwarfReg(X86::MM4, 45, true );
    mapLLVMRegToDwarfReg(X86::MM5, 46, true );
    mapLLVMRegToDwarfReg(X86::MM6, 47, true );
    mapLLVMRegToDwarfReg(X86::MM7, 48, true );
    mapLLVMRegToDwarfReg(X86::R8, 8, true );
    mapLLVMRegToDwarfReg(X86::R8B, -1, true );
    mapLLVMRegToDwarfReg(X86::R8D, -1, true );
    mapLLVMRegToDwarfReg(X86::R8W, -1, true );
    mapLLVMRegToDwarfReg(X86::R9, 9, true );
    mapLLVMRegToDwarfReg(X86::R9B, -1, true );
    mapLLVMRegToDwarfReg(X86::R9D, -1, true );
    mapLLVMRegToDwarfReg(X86::R9W, -1, true );
    mapLLVMRegToDwarfReg(X86::R10, 10, true );
    mapLLVMRegToDwarfReg(X86::R10B, -1, true );
    mapLLVMRegToDwarfReg(X86::R10D, -1, true );
    mapLLVMRegToDwarfReg(X86::R10W, -1, true );
    mapLLVMRegToDwarfReg(X86::R11, 11, true );
    mapLLVMRegToDwarfReg(X86::R11B, -1, true );
    mapLLVMRegToDwarfReg(X86::R11D, -1, true );
    mapLLVMRegToDwarfReg(X86::R11W, -1, true );
    mapLLVMRegToDwarfReg(X86::R12, 12, true );
    mapLLVMRegToDwarfReg(X86::R12B, -1, true );
    mapLLVMRegToDwarfReg(X86::R12D, -1, true );
    mapLLVMRegToDwarfReg(X86::R12W, -1, true );
    mapLLVMRegToDwarfReg(X86::R13, 13, true );
    mapLLVMRegToDwarfReg(X86::R13B, -1, true );
    mapLLVMRegToDwarfReg(X86::R13D, -1, true );
    mapLLVMRegToDwarfReg(X86::R13W, -1, true );
    mapLLVMRegToDwarfReg(X86::R14, 14, true );
    mapLLVMRegToDwarfReg(X86::R14B, -1, true );
    mapLLVMRegToDwarfReg(X86::R14D, -1, true );
    mapLLVMRegToDwarfReg(X86::R14W, -1, true );
    mapLLVMRegToDwarfReg(X86::R15, 15, true );
    mapLLVMRegToDwarfReg(X86::R15B, -1, true );
    mapLLVMRegToDwarfReg(X86::R15D, -1, true );
    mapLLVMRegToDwarfReg(X86::R15W, -1, true );
    mapLLVMRegToDwarfReg(X86::RAX, 0, true );
    mapLLVMRegToDwarfReg(X86::RBP, 6, true );
    mapLLVMRegToDwarfReg(X86::RBX, 3, true );
    mapLLVMRegToDwarfReg(X86::RCX, 2, true );
    mapLLVMRegToDwarfReg(X86::RDI, 5, true );
    mapLLVMRegToDwarfReg(X86::RDX, 1, true );
    mapLLVMRegToDwarfReg(X86::RIP, 16, true );
    mapLLVMRegToDwarfReg(X86::RIZ, -1, true );
    mapLLVMRegToDwarfReg(X86::RSI, 4, true );
    mapLLVMRegToDwarfReg(X86::RSP, 7, true );
    mapLLVMRegToDwarfReg(X86::SI, -1, true );
    mapLLVMRegToDwarfReg(X86::SIL, -1, true );
    mapLLVMRegToDwarfReg(X86::SP, -1, true );
    mapLLVMRegToDwarfReg(X86::SPL, -1, true );
    mapLLVMRegToDwarfReg(X86::SS, -1, true );
    mapLLVMRegToDwarfReg(X86::ST0, 33, true );
    mapLLVMRegToDwarfReg(X86::ST1, 34, true );
    mapLLVMRegToDwarfReg(X86::ST2, 35, true );
    mapLLVMRegToDwarfReg(X86::ST3, 36, true );
    mapLLVMRegToDwarfReg(X86::ST4, 37, true );
    mapLLVMRegToDwarfReg(X86::ST5, 38, true );
    mapLLVMRegToDwarfReg(X86::ST6, 39, true );
    mapLLVMRegToDwarfReg(X86::ST7, 40, true );
    mapLLVMRegToDwarfReg(X86::XMM0, 17, true );
    mapLLVMRegToDwarfReg(X86::XMM1, 18, true );
    mapLLVMRegToDwarfReg(X86::XMM2, 19, true );
    mapLLVMRegToDwarfReg(X86::XMM3, 20, true );
    mapLLVMRegToDwarfReg(X86::XMM4, 21, true );
    mapLLVMRegToDwarfReg(X86::XMM5, 22, true );
    mapLLVMRegToDwarfReg(X86::XMM6, 23, true );
    mapLLVMRegToDwarfReg(X86::XMM7, 24, true );
    mapLLVMRegToDwarfReg(X86::XMM8, 25, true );
    mapLLVMRegToDwarfReg(X86::XMM9, 26, true );
    mapLLVMRegToDwarfReg(X86::XMM10, 27, true );
    mapLLVMRegToDwarfReg(X86::XMM11, 28, true );
    mapLLVMRegToDwarfReg(X86::XMM12, 29, true );
    mapLLVMRegToDwarfReg(X86::XMM13, 30, true );
    mapLLVMRegToDwarfReg(X86::XMM14, 31, true );
    mapLLVMRegToDwarfReg(X86::XMM15, 32, true );
    mapLLVMRegToDwarfReg(X86::YMM0, 17, true );
    mapLLVMRegToDwarfReg(X86::YMM1, 18, true );
    mapLLVMRegToDwarfReg(X86::YMM2, 19, true );
    mapLLVMRegToDwarfReg(X86::YMM3, 20, true );
    mapLLVMRegToDwarfReg(X86::YMM4, 21, true );
    mapLLVMRegToDwarfReg(X86::YMM5, 22, true );
    mapLLVMRegToDwarfReg(X86::YMM6, 23, true );
    mapLLVMRegToDwarfReg(X86::YMM7, 24, true );
    mapLLVMRegToDwarfReg(X86::YMM8, 25, true );
    mapLLVMRegToDwarfReg(X86::YMM9, 26, true );
    mapLLVMRegToDwarfReg(X86::YMM10, 27, true );
    mapLLVMRegToDwarfReg(X86::YMM11, 28, true );
    mapLLVMRegToDwarfReg(X86::YMM12, 29, true );
    mapLLVMRegToDwarfReg(X86::YMM13, 30, true );
    mapLLVMRegToDwarfReg(X86::YMM14, 31, true );
    mapLLVMRegToDwarfReg(X86::YMM15, 32, true );
    break;
  case 1:
    mapLLVMRegToDwarfReg(X86::AH, -1, true );
    mapLLVMRegToDwarfReg(X86::AL, -1, true );
    mapLLVMRegToDwarfReg(X86::AX, -1, true );
    mapLLVMRegToDwarfReg(X86::BH, -1, true );
    mapLLVMRegToDwarfReg(X86::BL, -1, true );
    mapLLVMRegToDwarfReg(X86::BP, -1, true );
    mapLLVMRegToDwarfReg(X86::BPL, -1, true );
    mapLLVMRegToDwarfReg(X86::BX, -1, true );
    mapLLVMRegToDwarfReg(X86::CH, -1, true );
    mapLLVMRegToDwarfReg(X86::CL, -1, true );
    mapLLVMRegToDwarfReg(X86::CR0, -1, true );
    mapLLVMRegToDwarfReg(X86::CR1, -1, true );
    mapLLVMRegToDwarfReg(X86::CR2, -1, true );
    mapLLVMRegToDwarfReg(X86::CR3, -1, true );
    mapLLVMRegToDwarfReg(X86::CR4, -1, true );
    mapLLVMRegToDwarfReg(X86::CR5, -1, true );
    mapLLVMRegToDwarfReg(X86::CR6, -1, true );
    mapLLVMRegToDwarfReg(X86::CR7, -1, true );
    mapLLVMRegToDwarfReg(X86::CR8, -1, true );
    mapLLVMRegToDwarfReg(X86::CR9, -1, true );
    mapLLVMRegToDwarfReg(X86::CR10, -1, true );
    mapLLVMRegToDwarfReg(X86::CR11, -1, true );
    mapLLVMRegToDwarfReg(X86::CR12, -1, true );
    mapLLVMRegToDwarfReg(X86::CR13, -1, true );
    mapLLVMRegToDwarfReg(X86::CR14, -1, true );
    mapLLVMRegToDwarfReg(X86::CR15, -1, true );
    mapLLVMRegToDwarfReg(X86::CS, -1, true );
    mapLLVMRegToDwarfReg(X86::CX, -1, true );
    mapLLVMRegToDwarfReg(X86::DH, -1, true );
    mapLLVMRegToDwarfReg(X86::DI, -1, true );
    mapLLVMRegToDwarfReg(X86::DIL, -1, true );
    mapLLVMRegToDwarfReg(X86::DL, -1, true );
    mapLLVMRegToDwarfReg(X86::DR0, -1, true );
    mapLLVMRegToDwarfReg(X86::DR1, -1, true );
    mapLLVMRegToDwarfReg(X86::DR2, -1, true );
    mapLLVMRegToDwarfReg(X86::DR3, -1, true );
    mapLLVMRegToDwarfReg(X86::DR4, -1, true );
    mapLLVMRegToDwarfReg(X86::DR5, -1, true );
    mapLLVMRegToDwarfReg(X86::DR6, -1, true );
    mapLLVMRegToDwarfReg(X86::DR7, -1, true );
    mapLLVMRegToDwarfReg(X86::DS, -1, true );
    mapLLVMRegToDwarfReg(X86::DX, -1, true );
    mapLLVMRegToDwarfReg(X86::EAX, 0, true );
    mapLLVMRegToDwarfReg(X86::EBP, 4, true );
    mapLLVMRegToDwarfReg(X86::EBX, 3, true );
    mapLLVMRegToDwarfReg(X86::ECX, 1, true );
    mapLLVMRegToDwarfReg(X86::EDI, 7, true );
    mapLLVMRegToDwarfReg(X86::EDX, 2, true );
    mapLLVMRegToDwarfReg(X86::EFLAGS, -1, true );
    mapLLVMRegToDwarfReg(X86::EIP, 8, true );
    mapLLVMRegToDwarfReg(X86::EIZ, -1, true );
    mapLLVMRegToDwarfReg(X86::ES, -1, true );
    mapLLVMRegToDwarfReg(X86::ESI, 6, true );
    mapLLVMRegToDwarfReg(X86::ESP, 5, true );
    mapLLVMRegToDwarfReg(X86::FP0, -1, true );
    mapLLVMRegToDwarfReg(X86::FP1, -1, true );
    mapLLVMRegToDwarfReg(X86::FP2, -1, true );
    mapLLVMRegToDwarfReg(X86::FP3, -1, true );
    mapLLVMRegToDwarfReg(X86::FP4, -1, true );
    mapLLVMRegToDwarfReg(X86::FP5, -1, true );
    mapLLVMRegToDwarfReg(X86::FP6, -1, true );
    mapLLVMRegToDwarfReg(X86::FS, -1, true );
    mapLLVMRegToDwarfReg(X86::GS, -1, true );
    mapLLVMRegToDwarfReg(X86::IP, -1, true );
    mapLLVMRegToDwarfReg(X86::MM0, 29, true );
    mapLLVMRegToDwarfReg(X86::MM1, 30, true );
    mapLLVMRegToDwarfReg(X86::MM2, 31, true );
    mapLLVMRegToDwarfReg(X86::MM3, 32, true );
    mapLLVMRegToDwarfReg(X86::MM4, 33, true );
    mapLLVMRegToDwarfReg(X86::MM5, 34, true );
    mapLLVMRegToDwarfReg(X86::MM6, 35, true );
    mapLLVMRegToDwarfReg(X86::MM7, 36, true );
    mapLLVMRegToDwarfReg(X86::R8, -2, true );
    mapLLVMRegToDwarfReg(X86::R8B, -1, true );
    mapLLVMRegToDwarfReg(X86::R8D, -1, true );
    mapLLVMRegToDwarfReg(X86::R8W, -1, true );
    mapLLVMRegToDwarfReg(X86::R9, -2, true );
    mapLLVMRegToDwarfReg(X86::R9B, -1, true );
    mapLLVMRegToDwarfReg(X86::R9D, -1, true );
    mapLLVMRegToDwarfReg(X86::R9W, -1, true );
    mapLLVMRegToDwarfReg(X86::R10, -2, true );
    mapLLVMRegToDwarfReg(X86::R10B, -1, true );
    mapLLVMRegToDwarfReg(X86::R10D, -1, true );
    mapLLVMRegToDwarfReg(X86::R10W, -1, true );
    mapLLVMRegToDwarfReg(X86::R11, -2, true );
    mapLLVMRegToDwarfReg(X86::R11B, -1, true );
    mapLLVMRegToDwarfReg(X86::R11D, -1, true );
    mapLLVMRegToDwarfReg(X86::R11W, -1, true );
    mapLLVMRegToDwarfReg(X86::R12, -2, true );
    mapLLVMRegToDwarfReg(X86::R12B, -1, true );
    mapLLVMRegToDwarfReg(X86::R12D, -1, true );
    mapLLVMRegToDwarfReg(X86::R12W, -1, true );
    mapLLVMRegToDwarfReg(X86::R13, -2, true );
    mapLLVMRegToDwarfReg(X86::R13B, -1, true );
    mapLLVMRegToDwarfReg(X86::R13D, -1, true );
    mapLLVMRegToDwarfReg(X86::R13W, -1, true );
    mapLLVMRegToDwarfReg(X86::R14, -2, true );
    mapLLVMRegToDwarfReg(X86::R14B, -1, true );
    mapLLVMRegToDwarfReg(X86::R14D, -1, true );
    mapLLVMRegToDwarfReg(X86::R14W, -1, true );
    mapLLVMRegToDwarfReg(X86::R15, -2, true );
    mapLLVMRegToDwarfReg(X86::R15B, -1, true );
    mapLLVMRegToDwarfReg(X86::R15D, -1, true );
    mapLLVMRegToDwarfReg(X86::R15W, -1, true );
    mapLLVMRegToDwarfReg(X86::RAX, -2, true );
    mapLLVMRegToDwarfReg(X86::RBP, -2, true );
    mapLLVMRegToDwarfReg(X86::RBX, -2, true );
    mapLLVMRegToDwarfReg(X86::RCX, -2, true );
    mapLLVMRegToDwarfReg(X86::RDI, -2, true );
    mapLLVMRegToDwarfReg(X86::RDX, -2, true );
    mapLLVMRegToDwarfReg(X86::RIP, -2, true );
    mapLLVMRegToDwarfReg(X86::RIZ, -1, true );
    mapLLVMRegToDwarfReg(X86::RSI, -2, true );
    mapLLVMRegToDwarfReg(X86::RSP, -2, true );
    mapLLVMRegToDwarfReg(X86::SI, -1, true );
    mapLLVMRegToDwarfReg(X86::SIL, -1, true );
    mapLLVMRegToDwarfReg(X86::SP, -1, true );
    mapLLVMRegToDwarfReg(X86::SPL, -1, true );
    mapLLVMRegToDwarfReg(X86::SS, -1, true );
    mapLLVMRegToDwarfReg(X86::ST0, 12, true );
    mapLLVMRegToDwarfReg(X86::ST1, 13, true );
    mapLLVMRegToDwarfReg(X86::ST2, 14, true );
    mapLLVMRegToDwarfReg(X86::ST3, 15, true );
    mapLLVMRegToDwarfReg(X86::ST4, 16, true );
    mapLLVMRegToDwarfReg(X86::ST5, 17, true );
    mapLLVMRegToDwarfReg(X86::ST6, 18, true );
    mapLLVMRegToDwarfReg(X86::ST7, 19, true );
    mapLLVMRegToDwarfReg(X86::XMM0, 21, true );
    mapLLVMRegToDwarfReg(X86::XMM1, 22, true );
    mapLLVMRegToDwarfReg(X86::XMM2, 23, true );
    mapLLVMRegToDwarfReg(X86::XMM3, 24, true );
    mapLLVMRegToDwarfReg(X86::XMM4, 25, true );
    mapLLVMRegToDwarfReg(X86::XMM5, 26, true );
    mapLLVMRegToDwarfReg(X86::XMM6, 27, true );
    mapLLVMRegToDwarfReg(X86::XMM7, 28, true );
    mapLLVMRegToDwarfReg(X86::XMM8, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM9, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM10, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM11, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM12, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM13, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM14, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM15, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM0, 21, true );
    mapLLVMRegToDwarfReg(X86::YMM1, 22, true );
    mapLLVMRegToDwarfReg(X86::YMM2, 23, true );
    mapLLVMRegToDwarfReg(X86::YMM3, 24, true );
    mapLLVMRegToDwarfReg(X86::YMM4, 25, true );
    mapLLVMRegToDwarfReg(X86::YMM5, 26, true );
    mapLLVMRegToDwarfReg(X86::YMM6, 27, true );
    mapLLVMRegToDwarfReg(X86::YMM7, 28, true );
    mapLLVMRegToDwarfReg(X86::YMM8, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM9, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM10, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM11, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM12, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM13, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM14, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM15, -2, true );
    break;
  case 2:
    mapLLVMRegToDwarfReg(X86::AH, -1, true );
    mapLLVMRegToDwarfReg(X86::AL, -1, true );
    mapLLVMRegToDwarfReg(X86::AX, -1, true );
    mapLLVMRegToDwarfReg(X86::BH, -1, true );
    mapLLVMRegToDwarfReg(X86::BL, -1, true );
    mapLLVMRegToDwarfReg(X86::BP, -1, true );
    mapLLVMRegToDwarfReg(X86::BPL, -1, true );
    mapLLVMRegToDwarfReg(X86::BX, -1, true );
    mapLLVMRegToDwarfReg(X86::CH, -1, true );
    mapLLVMRegToDwarfReg(X86::CL, -1, true );
    mapLLVMRegToDwarfReg(X86::CR0, -1, true );
    mapLLVMRegToDwarfReg(X86::CR1, -1, true );
    mapLLVMRegToDwarfReg(X86::CR2, -1, true );
    mapLLVMRegToDwarfReg(X86::CR3, -1, true );
    mapLLVMRegToDwarfReg(X86::CR4, -1, true );
    mapLLVMRegToDwarfReg(X86::CR5, -1, true );
    mapLLVMRegToDwarfReg(X86::CR6, -1, true );
    mapLLVMRegToDwarfReg(X86::CR7, -1, true );
    mapLLVMRegToDwarfReg(X86::CR8, -1, true );
    mapLLVMRegToDwarfReg(X86::CR9, -1, true );
    mapLLVMRegToDwarfReg(X86::CR10, -1, true );
    mapLLVMRegToDwarfReg(X86::CR11, -1, true );
    mapLLVMRegToDwarfReg(X86::CR12, -1, true );
    mapLLVMRegToDwarfReg(X86::CR13, -1, true );
    mapLLVMRegToDwarfReg(X86::CR14, -1, true );
    mapLLVMRegToDwarfReg(X86::CR15, -1, true );
    mapLLVMRegToDwarfReg(X86::CS, -1, true );
    mapLLVMRegToDwarfReg(X86::CX, -1, true );
    mapLLVMRegToDwarfReg(X86::DH, -1, true );
    mapLLVMRegToDwarfReg(X86::DI, -1, true );
    mapLLVMRegToDwarfReg(X86::DIL, -1, true );
    mapLLVMRegToDwarfReg(X86::DL, -1, true );
    mapLLVMRegToDwarfReg(X86::DR0, -1, true );
    mapLLVMRegToDwarfReg(X86::DR1, -1, true );
    mapLLVMRegToDwarfReg(X86::DR2, -1, true );
    mapLLVMRegToDwarfReg(X86::DR3, -1, true );
    mapLLVMRegToDwarfReg(X86::DR4, -1, true );
    mapLLVMRegToDwarfReg(X86::DR5, -1, true );
    mapLLVMRegToDwarfReg(X86::DR6, -1, true );
    mapLLVMRegToDwarfReg(X86::DR7, -1, true );
    mapLLVMRegToDwarfReg(X86::DS, -1, true );
    mapLLVMRegToDwarfReg(X86::DX, -1, true );
    mapLLVMRegToDwarfReg(X86::EAX, 0, true );
    mapLLVMRegToDwarfReg(X86::EBP, 5, true );
    mapLLVMRegToDwarfReg(X86::EBX, 3, true );
    mapLLVMRegToDwarfReg(X86::ECX, 1, true );
    mapLLVMRegToDwarfReg(X86::EDI, 7, true );
    mapLLVMRegToDwarfReg(X86::EDX, 2, true );
    mapLLVMRegToDwarfReg(X86::EFLAGS, -1, true );
    mapLLVMRegToDwarfReg(X86::EIP, 8, true );
    mapLLVMRegToDwarfReg(X86::EIZ, -1, true );
    mapLLVMRegToDwarfReg(X86::ES, -1, true );
    mapLLVMRegToDwarfReg(X86::ESI, 6, true );
    mapLLVMRegToDwarfReg(X86::ESP, 4, true );
    mapLLVMRegToDwarfReg(X86::FP0, -1, true );
    mapLLVMRegToDwarfReg(X86::FP1, -1, true );
    mapLLVMRegToDwarfReg(X86::FP2, -1, true );
    mapLLVMRegToDwarfReg(X86::FP3, -1, true );
    mapLLVMRegToDwarfReg(X86::FP4, -1, true );
    mapLLVMRegToDwarfReg(X86::FP5, -1, true );
    mapLLVMRegToDwarfReg(X86::FP6, -1, true );
    mapLLVMRegToDwarfReg(X86::FS, -1, true );
    mapLLVMRegToDwarfReg(X86::GS, -1, true );
    mapLLVMRegToDwarfReg(X86::IP, -1, true );
    mapLLVMRegToDwarfReg(X86::MM0, 29, true );
    mapLLVMRegToDwarfReg(X86::MM1, 30, true );
    mapLLVMRegToDwarfReg(X86::MM2, 31, true );
    mapLLVMRegToDwarfReg(X86::MM3, 32, true );
    mapLLVMRegToDwarfReg(X86::MM4, 33, true );
    mapLLVMRegToDwarfReg(X86::MM5, 34, true );
    mapLLVMRegToDwarfReg(X86::MM6, 35, true );
    mapLLVMRegToDwarfReg(X86::MM7, 36, true );
    mapLLVMRegToDwarfReg(X86::R8, -2, true );
    mapLLVMRegToDwarfReg(X86::R8B, -1, true );
    mapLLVMRegToDwarfReg(X86::R8D, -1, true );
    mapLLVMRegToDwarfReg(X86::R8W, -1, true );
    mapLLVMRegToDwarfReg(X86::R9, -2, true );
    mapLLVMRegToDwarfReg(X86::R9B, -1, true );
    mapLLVMRegToDwarfReg(X86::R9D, -1, true );
    mapLLVMRegToDwarfReg(X86::R9W, -1, true );
    mapLLVMRegToDwarfReg(X86::R10, -2, true );
    mapLLVMRegToDwarfReg(X86::R10B, -1, true );
    mapLLVMRegToDwarfReg(X86::R10D, -1, true );
    mapLLVMRegToDwarfReg(X86::R10W, -1, true );
    mapLLVMRegToDwarfReg(X86::R11, -2, true );
    mapLLVMRegToDwarfReg(X86::R11B, -1, true );
    mapLLVMRegToDwarfReg(X86::R11D, -1, true );
    mapLLVMRegToDwarfReg(X86::R11W, -1, true );
    mapLLVMRegToDwarfReg(X86::R12, -2, true );
    mapLLVMRegToDwarfReg(X86::R12B, -1, true );
    mapLLVMRegToDwarfReg(X86::R12D, -1, true );
    mapLLVMRegToDwarfReg(X86::R12W, -1, true );
    mapLLVMRegToDwarfReg(X86::R13, -2, true );
    mapLLVMRegToDwarfReg(X86::R13B, -1, true );
    mapLLVMRegToDwarfReg(X86::R13D, -1, true );
    mapLLVMRegToDwarfReg(X86::R13W, -1, true );
    mapLLVMRegToDwarfReg(X86::R14, -2, true );
    mapLLVMRegToDwarfReg(X86::R14B, -1, true );
    mapLLVMRegToDwarfReg(X86::R14D, -1, true );
    mapLLVMRegToDwarfReg(X86::R14W, -1, true );
    mapLLVMRegToDwarfReg(X86::R15, -2, true );
    mapLLVMRegToDwarfReg(X86::R15B, -1, true );
    mapLLVMRegToDwarfReg(X86::R15D, -1, true );
    mapLLVMRegToDwarfReg(X86::R15W, -1, true );
    mapLLVMRegToDwarfReg(X86::RAX, -2, true );
    mapLLVMRegToDwarfReg(X86::RBP, -2, true );
    mapLLVMRegToDwarfReg(X86::RBX, -2, true );
    mapLLVMRegToDwarfReg(X86::RCX, -2, true );
    mapLLVMRegToDwarfReg(X86::RDI, -2, true );
    mapLLVMRegToDwarfReg(X86::RDX, -2, true );
    mapLLVMRegToDwarfReg(X86::RIP, -2, true );
    mapLLVMRegToDwarfReg(X86::RIZ, -1, true );
    mapLLVMRegToDwarfReg(X86::RSI, -2, true );
    mapLLVMRegToDwarfReg(X86::RSP, -2, true );
    mapLLVMRegToDwarfReg(X86::SI, -1, true );
    mapLLVMRegToDwarfReg(X86::SIL, -1, true );
    mapLLVMRegToDwarfReg(X86::SP, -1, true );
    mapLLVMRegToDwarfReg(X86::SPL, -1, true );
    mapLLVMRegToDwarfReg(X86::SS, -1, true );
    mapLLVMRegToDwarfReg(X86::ST0, 11, true );
    mapLLVMRegToDwarfReg(X86::ST1, 12, true );
    mapLLVMRegToDwarfReg(X86::ST2, 13, true );
    mapLLVMRegToDwarfReg(X86::ST3, 14, true );
    mapLLVMRegToDwarfReg(X86::ST4, 15, true );
    mapLLVMRegToDwarfReg(X86::ST5, 16, true );
    mapLLVMRegToDwarfReg(X86::ST6, 17, true );
    mapLLVMRegToDwarfReg(X86::ST7, 18, true );
    mapLLVMRegToDwarfReg(X86::XMM0, 21, true );
    mapLLVMRegToDwarfReg(X86::XMM1, 22, true );
    mapLLVMRegToDwarfReg(X86::XMM2, 23, true );
    mapLLVMRegToDwarfReg(X86::XMM3, 24, true );
    mapLLVMRegToDwarfReg(X86::XMM4, 25, true );
    mapLLVMRegToDwarfReg(X86::XMM5, 26, true );
    mapLLVMRegToDwarfReg(X86::XMM6, 27, true );
    mapLLVMRegToDwarfReg(X86::XMM7, 28, true );
    mapLLVMRegToDwarfReg(X86::XMM8, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM9, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM10, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM11, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM12, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM13, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM14, -2, true );
    mapLLVMRegToDwarfReg(X86::XMM15, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM0, 21, true );
    mapLLVMRegToDwarfReg(X86::YMM1, 22, true );
    mapLLVMRegToDwarfReg(X86::YMM2, 23, true );
    mapLLVMRegToDwarfReg(X86::YMM3, 24, true );
    mapLLVMRegToDwarfReg(X86::YMM4, 25, true );
    mapLLVMRegToDwarfReg(X86::YMM5, 26, true );
    mapLLVMRegToDwarfReg(X86::YMM6, 27, true );
    mapLLVMRegToDwarfReg(X86::YMM7, 28, true );
    mapLLVMRegToDwarfReg(X86::YMM8, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM9, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM10, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM11, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM12, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM13, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM14, -2, true );
    mapLLVMRegToDwarfReg(X86::YMM15, -2, true );
    break;
  }
}

} // End llvm namespace 
#endif // GET_REGINFO_TARGET_DESC

