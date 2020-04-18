//===- TableGen'erated file -------------------------------------*- C++ -*-===//
//
// Assembly Matcher Source Fragment
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//


#ifdef GET_ASSEMBLER_HEADER
#undef GET_ASSEMBLER_HEADER
  // This should be included into the middle of the declaration of
  // your subclasses implementation of MCTargetAsmParser.
  unsigned ComputeAvailableFeatures(uint64_t FeatureBits) const;
  bool ConvertToMCInst(unsigned Kind, MCInst &Inst, unsigned Opcode,
                       const SmallVectorImpl<MCParsedAsmOperand*> &Operands);
  bool MnemonicIsValid(StringRef Mnemonic);
  unsigned MatchInstructionImpl(
    const SmallVectorImpl<MCParsedAsmOperand*> &Operands,
    MCInst &Inst, unsigned &ErrorInfo);
#endif // GET_ASSEMBLER_HEADER_INFO


#ifdef GET_REGISTER_MATCHER
#undef GET_REGISTER_MATCHER

// Flags for subtarget features that participate in instruction matching.
enum SubtargetFeatureFlag {
  Feature_In32BitMode = (1 << 0),
  Feature_In64BitMode = (1 << 1),
  Feature_IsNaCl = (1 << 2),
  Feature_IsNaCl32 = (1 << 3),
  Feature_IsNaCl64 = (1 << 4),
  Feature_NotNaCl = (1 << 5),
  Feature_None = 0
};

static unsigned MatchRegisterName(StringRef Name) {
  switch (Name.size()) {
  default: break;
  case 2:	 // 25 strings to match.
    switch (Name[0]) {
    default: break;
    case 'a':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'h':	 // 1 string to match.
        return 1;	 // "ah"
      case 'l':	 // 1 string to match.
        return 2;	 // "al"
      case 'x':	 // 1 string to match.
        return 3;	 // "ax"
      }
      break;
    case 'b':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'h':	 // 1 string to match.
        return 4;	 // "bh"
      case 'l':	 // 1 string to match.
        return 5;	 // "bl"
      case 'p':	 // 1 string to match.
        return 6;	 // "bp"
      case 'x':	 // 1 string to match.
        return 8;	 // "bx"
      }
      break;
    case 'c':	 // 4 strings to match.
      switch (Name[1]) {
      default: break;
      case 'h':	 // 1 string to match.
        return 9;	 // "ch"
      case 'l':	 // 1 string to match.
        return 10;	 // "cl"
      case 's':	 // 1 string to match.
        return 27;	 // "cs"
      case 'x':	 // 1 string to match.
        return 28;	 // "cx"
      }
      break;
    case 'd':	 // 5 strings to match.
      switch (Name[1]) {
      default: break;
      case 'h':	 // 1 string to match.
        return 29;	 // "dh"
      case 'i':	 // 1 string to match.
        return 30;	 // "di"
      case 'l':	 // 1 string to match.
        return 32;	 // "dl"
      case 's':	 // 1 string to match.
        return 41;	 // "ds"
      case 'x':	 // 1 string to match.
        return 42;	 // "dx"
      }
      break;
    case 'e':	 // 1 string to match.
      if (Name[1] != 's')
        break;
      return 52;	 // "es"
    case 'f':	 // 1 string to match.
      if (Name[1] != 's')
        break;
      return 62;	 // "fs"
    case 'g':	 // 1 string to match.
      if (Name[1] != 's')
        break;
      return 63;	 // "gs"
    case 'i':	 // 1 string to match.
      if (Name[1] != 'p')
        break;
      return 64;	 // "ip"
    case 'r':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case '8':	 // 1 string to match.
        return 73;	 // "r8"
      case '9':	 // 1 string to match.
        return 77;	 // "r9"
      }
      break;
    case 's':	 // 3 strings to match.
      switch (Name[1]) {
      default: break;
      case 'i':	 // 1 string to match.
        return 115;	 // "si"
      case 'p':	 // 1 string to match.
        return 117;	 // "sp"
      case 's':	 // 1 string to match.
        return 119;	 // "ss"
      }
      break;
    }
    break;
  case 3:	 // 69 strings to match.
    switch (Name[0]) {
    default: break;
    case 'b':	 // 1 string to match.
      if (Name.substr(1, 2) != "pl")
        break;
      return 7;	 // "bpl"
    case 'c':	 // 10 strings to match.
      if (Name[1] != 'r')
        break;
      switch (Name[2]) {
      default: break;
      case '0':	 // 1 string to match.
        return 11;	 // "cr0"
      case '1':	 // 1 string to match.
        return 12;	 // "cr1"
      case '2':	 // 1 string to match.
        return 13;	 // "cr2"
      case '3':	 // 1 string to match.
        return 14;	 // "cr3"
      case '4':	 // 1 string to match.
        return 15;	 // "cr4"
      case '5':	 // 1 string to match.
        return 16;	 // "cr5"
      case '6':	 // 1 string to match.
        return 17;	 // "cr6"
      case '7':	 // 1 string to match.
        return 18;	 // "cr7"
      case '8':	 // 1 string to match.
        return 19;	 // "cr8"
      case '9':	 // 1 string to match.
        return 20;	 // "cr9"
      }
      break;
    case 'd':	 // 9 strings to match.
      switch (Name[1]) {
      default: break;
      case 'i':	 // 1 string to match.
        if (Name[2] != 'l')
          break;
        return 31;	 // "dil"
      case 'r':	 // 8 strings to match.
        switch (Name[2]) {
        default: break;
        case '0':	 // 1 string to match.
          return 33;	 // "dr0"
        case '1':	 // 1 string to match.
          return 34;	 // "dr1"
        case '2':	 // 1 string to match.
          return 35;	 // "dr2"
        case '3':	 // 1 string to match.
          return 36;	 // "dr3"
        case '4':	 // 1 string to match.
          return 37;	 // "dr4"
        case '5':	 // 1 string to match.
          return 38;	 // "dr5"
        case '6':	 // 1 string to match.
          return 39;	 // "dr6"
        case '7':	 // 1 string to match.
          return 40;	 // "dr7"
        }
        break;
      }
      break;
    case 'e':	 // 10 strings to match.
      switch (Name[1]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (Name[2] != 'x')
          break;
        return 43;	 // "eax"
      case 'b':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'p':	 // 1 string to match.
          return 44;	 // "ebp"
        case 'x':	 // 1 string to match.
          return 45;	 // "ebx"
        }
        break;
      case 'c':	 // 1 string to match.
        if (Name[2] != 'x')
          break;
        return 46;	 // "ecx"
      case 'd':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'i':	 // 1 string to match.
          return 47;	 // "edi"
        case 'x':	 // 1 string to match.
          return 48;	 // "edx"
        }
        break;
      case 'i':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'p':	 // 1 string to match.
          return 50;	 // "eip"
        case 'z':	 // 1 string to match.
          return 51;	 // "eiz"
        }
        break;
      case 's':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'i':	 // 1 string to match.
          return 53;	 // "esi"
        case 'p':	 // 1 string to match.
          return 54;	 // "esp"
        }
        break;
      }
      break;
    case 'f':	 // 7 strings to match.
      if (Name[1] != 'p')
        break;
      switch (Name[2]) {
      default: break;
      case '0':	 // 1 string to match.
        return 55;	 // "fp0"
      case '1':	 // 1 string to match.
        return 56;	 // "fp1"
      case '2':	 // 1 string to match.
        return 57;	 // "fp2"
      case '3':	 // 1 string to match.
        return 58;	 // "fp3"
      case '4':	 // 1 string to match.
        return 59;	 // "fp4"
      case '5':	 // 1 string to match.
        return 60;	 // "fp5"
      case '6':	 // 1 string to match.
        return 61;	 // "fp6"
      }
      break;
    case 'm':	 // 8 strings to match.
      if (Name[1] != 'm')
        break;
      switch (Name[2]) {
      default: break;
      case '0':	 // 1 string to match.
        return 65;	 // "mm0"
      case '1':	 // 1 string to match.
        return 66;	 // "mm1"
      case '2':	 // 1 string to match.
        return 67;	 // "mm2"
      case '3':	 // 1 string to match.
        return 68;	 // "mm3"
      case '4':	 // 1 string to match.
        return 69;	 // "mm4"
      case '5':	 // 1 string to match.
        return 70;	 // "mm5"
      case '6':	 // 1 string to match.
        return 71;	 // "mm6"
      case '7':	 // 1 string to match.
        return 72;	 // "mm7"
      }
      break;
    case 'r':	 // 22 strings to match.
      switch (Name[1]) {
      default: break;
      case '1':	 // 6 strings to match.
        switch (Name[2]) {
        default: break;
        case '0':	 // 1 string to match.
          return 81;	 // "r10"
        case '1':	 // 1 string to match.
          return 85;	 // "r11"
        case '2':	 // 1 string to match.
          return 89;	 // "r12"
        case '3':	 // 1 string to match.
          return 93;	 // "r13"
        case '4':	 // 1 string to match.
          return 97;	 // "r14"
        case '5':	 // 1 string to match.
          return 101;	 // "r15"
        }
        break;
      case '8':	 // 3 strings to match.
        switch (Name[2]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 74;	 // "r8b"
        case 'd':	 // 1 string to match.
          return 75;	 // "r8d"
        case 'w':	 // 1 string to match.
          return 76;	 // "r8w"
        }
        break;
      case '9':	 // 3 strings to match.
        switch (Name[2]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 78;	 // "r9b"
        case 'd':	 // 1 string to match.
          return 79;	 // "r9d"
        case 'w':	 // 1 string to match.
          return 80;	 // "r9w"
        }
        break;
      case 'a':	 // 1 string to match.
        if (Name[2] != 'x')
          break;
        return 105;	 // "rax"
      case 'b':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'p':	 // 1 string to match.
          return 106;	 // "rbp"
        case 'x':	 // 1 string to match.
          return 107;	 // "rbx"
        }
        break;
      case 'c':	 // 1 string to match.
        if (Name[2] != 'x')
          break;
        return 108;	 // "rcx"
      case 'd':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'i':	 // 1 string to match.
          return 109;	 // "rdi"
        case 'x':	 // 1 string to match.
          return 110;	 // "rdx"
        }
        break;
      case 'i':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'p':	 // 1 string to match.
          return 111;	 // "rip"
        case 'z':	 // 1 string to match.
          return 112;	 // "riz"
        }
        break;
      case 's':	 // 2 strings to match.
        switch (Name[2]) {
        default: break;
        case 'i':	 // 1 string to match.
          return 113;	 // "rsi"
        case 'p':	 // 1 string to match.
          return 114;	 // "rsp"
        }
        break;
      }
      break;
    case 's':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'i':	 // 1 string to match.
        if (Name[2] != 'l')
          break;
        return 116;	 // "sil"
      case 'p':	 // 1 string to match.
        if (Name[2] != 'l')
          break;
        return 118;	 // "spl"
      }
      break;
    }
    break;
  case 4:	 // 44 strings to match.
    switch (Name[0]) {
    default: break;
    case 'c':	 // 6 strings to match.
      if (Name.substr(1, 2) != "r1")
        break;
      switch (Name[3]) {
      default: break;
      case '0':	 // 1 string to match.
        return 21;	 // "cr10"
      case '1':	 // 1 string to match.
        return 22;	 // "cr11"
      case '2':	 // 1 string to match.
        return 23;	 // "cr12"
      case '3':	 // 1 string to match.
        return 24;	 // "cr13"
      case '4':	 // 1 string to match.
        return 25;	 // "cr14"
      case '5':	 // 1 string to match.
        return 26;	 // "cr15"
      }
      break;
    case 'r':	 // 18 strings to match.
      if (Name[1] != '1')
        break;
      switch (Name[2]) {
      default: break;
      case '0':	 // 3 strings to match.
        switch (Name[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 82;	 // "r10b"
        case 'd':	 // 1 string to match.
          return 83;	 // "r10d"
        case 'w':	 // 1 string to match.
          return 84;	 // "r10w"
        }
        break;
      case '1':	 // 3 strings to match.
        switch (Name[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 86;	 // "r11b"
        case 'd':	 // 1 string to match.
          return 87;	 // "r11d"
        case 'w':	 // 1 string to match.
          return 88;	 // "r11w"
        }
        break;
      case '2':	 // 3 strings to match.
        switch (Name[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 90;	 // "r12b"
        case 'd':	 // 1 string to match.
          return 91;	 // "r12d"
        case 'w':	 // 1 string to match.
          return 92;	 // "r12w"
        }
        break;
      case '3':	 // 3 strings to match.
        switch (Name[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 94;	 // "r13b"
        case 'd':	 // 1 string to match.
          return 95;	 // "r13d"
        case 'w':	 // 1 string to match.
          return 96;	 // "r13w"
        }
        break;
      case '4':	 // 3 strings to match.
        switch (Name[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 98;	 // "r14b"
        case 'd':	 // 1 string to match.
          return 99;	 // "r14d"
        case 'w':	 // 1 string to match.
          return 100;	 // "r14w"
        }
        break;
      case '5':	 // 3 strings to match.
        switch (Name[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          return 102;	 // "r15b"
        case 'd':	 // 1 string to match.
          return 103;	 // "r15d"
        case 'w':	 // 1 string to match.
          return 104;	 // "r15w"
        }
        break;
      }
      break;
    case 'x':	 // 10 strings to match.
      if (Name.substr(1, 2) != "mm")
        break;
      switch (Name[3]) {
      default: break;
      case '0':	 // 1 string to match.
        return 128;	 // "xmm0"
      case '1':	 // 1 string to match.
        return 129;	 // "xmm1"
      case '2':	 // 1 string to match.
        return 130;	 // "xmm2"
      case '3':	 // 1 string to match.
        return 131;	 // "xmm3"
      case '4':	 // 1 string to match.
        return 132;	 // "xmm4"
      case '5':	 // 1 string to match.
        return 133;	 // "xmm5"
      case '6':	 // 1 string to match.
        return 134;	 // "xmm6"
      case '7':	 // 1 string to match.
        return 135;	 // "xmm7"
      case '8':	 // 1 string to match.
        return 136;	 // "xmm8"
      case '9':	 // 1 string to match.
        return 137;	 // "xmm9"
      }
      break;
    case 'y':	 // 10 strings to match.
      if (Name.substr(1, 2) != "mm")
        break;
      switch (Name[3]) {
      default: break;
      case '0':	 // 1 string to match.
        return 144;	 // "ymm0"
      case '1':	 // 1 string to match.
        return 145;	 // "ymm1"
      case '2':	 // 1 string to match.
        return 146;	 // "ymm2"
      case '3':	 // 1 string to match.
        return 147;	 // "ymm3"
      case '4':	 // 1 string to match.
        return 148;	 // "ymm4"
      case '5':	 // 1 string to match.
        return 149;	 // "ymm5"
      case '6':	 // 1 string to match.
        return 150;	 // "ymm6"
      case '7':	 // 1 string to match.
        return 151;	 // "ymm7"
      case '8':	 // 1 string to match.
        return 152;	 // "ymm8"
      case '9':	 // 1 string to match.
        return 153;	 // "ymm9"
      }
      break;
    }
    break;
  case 5:	 // 21 strings to match.
    switch (Name[0]) {
    default: break;
    case 'f':	 // 1 string to match.
      if (Name.substr(1, 4) != "lags")
        break;
      return 49;	 // "flags"
    case 's':	 // 8 strings to match.
      if (Name.substr(1, 2) != "t(")
        break;
      switch (Name[3]) {
      default: break;
      case '0':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 120;	 // "st(0)"
      case '1':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 121;	 // "st(1)"
      case '2':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 122;	 // "st(2)"
      case '3':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 123;	 // "st(3)"
      case '4':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 124;	 // "st(4)"
      case '5':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 125;	 // "st(5)"
      case '6':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 126;	 // "st(6)"
      case '7':	 // 1 string to match.
        if (Name[4] != ')')
          break;
        return 127;	 // "st(7)"
      }
      break;
    case 'x':	 // 6 strings to match.
      if (Name.substr(1, 3) != "mm1")
        break;
      switch (Name[4]) {
      default: break;
      case '0':	 // 1 string to match.
        return 138;	 // "xmm10"
      case '1':	 // 1 string to match.
        return 139;	 // "xmm11"
      case '2':	 // 1 string to match.
        return 140;	 // "xmm12"
      case '3':	 // 1 string to match.
        return 141;	 // "xmm13"
      case '4':	 // 1 string to match.
        return 142;	 // "xmm14"
      case '5':	 // 1 string to match.
        return 143;	 // "xmm15"
      }
      break;
    case 'y':	 // 6 strings to match.
      if (Name.substr(1, 3) != "mm1")
        break;
      switch (Name[4]) {
      default: break;
      case '0':	 // 1 string to match.
        return 154;	 // "ymm10"
      case '1':	 // 1 string to match.
        return 155;	 // "ymm11"
      case '2':	 // 1 string to match.
        return 156;	 // "ymm12"
      case '3':	 // 1 string to match.
        return 157;	 // "ymm13"
      case '4':	 // 1 string to match.
        return 158;	 // "ymm14"
      case '5':	 // 1 string to match.
        return 159;	 // "ymm15"
      }
      break;
    }
    break;
  }
  return 0;
}

#endif // GET_REGISTER_MATCHER


#ifdef GET_MATCHER_IMPLEMENTATION
#undef GET_MATCHER_IMPLEMENTATION

static void ApplyMnemonicAliases(StringRef &Mnemonic, unsigned Features) {
  switch (Mnemonic.size()) {
  default: break;
  case 2:	 // 2 strings to match.
    if (Mnemonic[0] != 'j')
      break;
    switch (Mnemonic[1]) {
    default: break;
    case 'c':	 // 1 string to match.
      Mnemonic = "jb";	 // "jc"
      return;
    case 'z':	 // 1 string to match.
      Mnemonic = "je";	 // "jz"
      return;
    }
    break;
  case 3:	 // 12 strings to match.
    switch (Mnemonic[0]) {
    default: break;
    case 'c':	 // 3 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'b':	 // 1 string to match.
        if (Mnemonic[2] != 'w')
          break;
        Mnemonic = "cbtw";	 // "cbw"
        return;
      case 'd':	 // 1 string to match.
        if (Mnemonic[2] != 'q')
          break;
        Mnemonic = "cltd";	 // "cdq"
        return;
      case 'w':	 // 1 string to match.
        if (Mnemonic[2] != 'd')
          break;
        Mnemonic = "cwtd";	 // "cwd"
        return;
      }
      break;
    case 'j':	 // 8 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'n':	 // 6 strings to match.
        switch (Mnemonic[2]) {
        default: break;
        case 'a':	 // 1 string to match.
          Mnemonic = "jbe";	 // "jna"
          return;
        case 'b':	 // 1 string to match.
          Mnemonic = "jae";	 // "jnb"
          return;
        case 'c':	 // 1 string to match.
          Mnemonic = "jae";	 // "jnc"
          return;
        case 'g':	 // 1 string to match.
          Mnemonic = "jle";	 // "jng"
          return;
        case 'l':	 // 1 string to match.
          Mnemonic = "jge";	 // "jnl"
          return;
        case 'z':	 // 1 string to match.
          Mnemonic = "jne";	 // "jnz"
          return;
        }
        break;
      case 'p':	 // 2 strings to match.
        switch (Mnemonic[2]) {
        default: break;
        case 'e':	 // 1 string to match.
          Mnemonic = "jp";	 // "jpe"
          return;
        case 'o':	 // 1 string to match.
          Mnemonic = "jnp";	 // "jpo"
          return;
        }
        break;
      }
      break;
    case 'p':	 // 1 string to match.
      if (Mnemonic.substr(1, 2) != "op")
        break;
      if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "pop"
        Mnemonic = "popl";
      else if ((Features & Feature_In64BitMode) == Feature_In64BitMode)
        Mnemonic = "popq";
      return;
    }
    break;
  case 4:	 // 22 strings to match.
    switch (Mnemonic[0]) {
    default: break;
    case 'c':	 // 3 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (Mnemonic.substr(2, 2) != "ll")
          break;
        if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "call"
          Mnemonic = "calll";
        else if ((Features & Feature_In64BitMode) == Feature_In64BitMode)
          Mnemonic = "callq";
        return;
      case 'd':	 // 1 string to match.
        if (Mnemonic.substr(2, 2) != "qe")
          break;
        Mnemonic = "cltq";	 // "cdqe"
        return;
      case 'w':	 // 1 string to match.
        if (Mnemonic.substr(2, 2) != "de")
          break;
        Mnemonic = "cwtl";	 // "cwde"
        return;
      }
      break;
    case 'i':	 // 1 string to match.
      if (Mnemonic.substr(1, 3) != "ret")
        break;
      Mnemonic = "iretl";	 // "iret"
      return;
    case 'j':	 // 4 strings to match.
      if (Mnemonic[1] != 'n')
        break;
      switch (Mnemonic[2]) {
      default: break;
      case 'a':	 // 1 string to match.
        if (Mnemonic[3] != 'e')
          break;
        Mnemonic = "jb";	 // "jnae"
        return;
      case 'b':	 // 1 string to match.
        if (Mnemonic[3] != 'e')
          break;
        Mnemonic = "ja";	 // "jnbe"
        return;
      case 'g':	 // 1 string to match.
        if (Mnemonic[3] != 'e')
          break;
        Mnemonic = "jl";	 // "jnge"
        return;
      case 'l':	 // 1 string to match.
        if (Mnemonic[3] != 'e')
          break;
        Mnemonic = "jg";	 // "jnle"
        return;
      }
      break;
    case 'l':	 // 1 string to match.
      if (Mnemonic.substr(1, 3) != "ret")
        break;
      Mnemonic = "lretl";	 // "lret"
      return;
    case 'p':	 // 2 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'o':	 // 1 string to match.
        if (Mnemonic.substr(2, 2) != "pf")
          break;
        if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "popf"
          Mnemonic = "popfl";
        else if ((Features & Feature_In64BitMode) == Feature_In64BitMode)
          Mnemonic = "popfq";
        return;
      case 'u':	 // 1 string to match.
        if (Mnemonic.substr(2, 2) != "sh")
          break;
        if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "push"
          Mnemonic = "pushl";
        else if ((Features & Feature_In64BitMode) == Feature_In64BitMode)
          Mnemonic = "pushq";
        return;
      }
      break;
    case 'r':	 // 4 strings to match.
      if (Mnemonic[1] != 'e')
        break;
      switch (Mnemonic[2]) {
      default: break;
      case 'p':	 // 2 strings to match.
        switch (Mnemonic[3]) {
        default: break;
        case 'e':	 // 1 string to match.
          Mnemonic = "rep";	 // "repe"
          return;
        case 'z':	 // 1 string to match.
          Mnemonic = "rep";	 // "repz"
          return;
        }
        break;
      case 't':	 // 2 strings to match.
        switch (Mnemonic[3]) {
        default: break;
        case 'l':	 // 1 string to match.
          if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "retl"
            Mnemonic = "ret";
          return;
        case 'q':	 // 1 string to match.
          if ((Features & Feature_In64BitMode) == Feature_In64BitMode)	 // "retq"
            Mnemonic = "ret";
          return;
        }
        break;
      }
      break;
    case 's':	 // 6 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'a':	 // 4 strings to match.
        if (Mnemonic[2] != 'l')
          break;
        switch (Mnemonic[3]) {
        default: break;
        case 'b':	 // 1 string to match.
          Mnemonic = "shlb";	 // "salb"
          return;
        case 'l':	 // 1 string to match.
          Mnemonic = "shll";	 // "sall"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "shlq";	 // "salq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "shlw";	 // "salw"
          return;
        }
        break;
      case 'e':	 // 2 strings to match.
        if (Mnemonic[2] != 't')
          break;
        switch (Mnemonic[3]) {
        default: break;
        case 'c':	 // 1 string to match.
          Mnemonic = "setb";	 // "setc"
          return;
        case 'z':	 // 1 string to match.
          Mnemonic = "sete";	 // "setz"
          return;
        }
        break;
      }
      break;
    case 'u':	 // 1 string to match.
      if (Mnemonic.substr(1, 3) != "d2a")
        break;
      Mnemonic = "ud2";	 // "ud2a"
      return;
    }
    break;
  case 5:	 // 27 strings to match.
    switch (Mnemonic[0]) {
    default: break;
    case 'f':	 // 2 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'i':	 // 1 string to match.
        if (Mnemonic.substr(2, 3) != "ldq")
          break;
        Mnemonic = "fildll";	 // "fildq"
        return;
      case 'w':	 // 1 string to match.
        if (Mnemonic.substr(2, 3) != "ait")
          break;
        Mnemonic = "wait";	 // "fwait"
        return;
      }
      break;
    case 'l':	 // 5 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'g':	 // 2 strings to match.
        if (Mnemonic.substr(2, 2) != "dt")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'l':	 // 1 string to match.
          if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "lgdtl"
            Mnemonic = "lgdt";
          return;
        case 'q':	 // 1 string to match.
          if ((Features & Feature_In64BitMode) == Feature_In64BitMode)	 // "lgdtq"
            Mnemonic = "lgdt";
          return;
        }
        break;
      case 'i':	 // 2 strings to match.
        if (Mnemonic.substr(2, 2) != "dt")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'l':	 // 1 string to match.
          if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "lidtl"
            Mnemonic = "lidt";
          return;
        case 'q':	 // 1 string to match.
          if ((Features & Feature_In64BitMode) == Feature_In64BitMode)	 // "lidtq"
            Mnemonic = "lidt";
          return;
        }
        break;
      case 'o':	 // 1 string to match.
        if (Mnemonic.substr(2, 3) != "opz")
          break;
        Mnemonic = "loope";	 // "loopz"
        return;
      }
      break;
    case 'p':	 // 2 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'o':	 // 1 string to match.
        if (Mnemonic.substr(2, 3) != "pfd")
          break;
        Mnemonic = "popfl";	 // "popfd"
        return;
      case 'u':	 // 1 string to match.
        if (Mnemonic.substr(2, 3) != "shf")
          break;
        if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "pushf"
          Mnemonic = "pushfl";
        else if ((Features & Feature_In64BitMode) == Feature_In64BitMode)
          Mnemonic = "pushfq";
        return;
      }
      break;
    case 'r':	 // 1 string to match.
      if (Mnemonic.substr(1, 4) != "epnz")
        break;
      Mnemonic = "repne";	 // "repnz"
      return;
    case 's':	 // 16 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'e':	 // 8 strings to match.
        if (Mnemonic[2] != 't')
          break;
        switch (Mnemonic[3]) {
        default: break;
        case 'n':	 // 6 strings to match.
          switch (Mnemonic[4]) {
          default: break;
          case 'a':	 // 1 string to match.
            Mnemonic = "setbe";	 // "setna"
            return;
          case 'b':	 // 1 string to match.
            Mnemonic = "setae";	 // "setnb"
            return;
          case 'c':	 // 1 string to match.
            Mnemonic = "setae";	 // "setnc"
            return;
          case 'g':	 // 1 string to match.
            Mnemonic = "setle";	 // "setng"
            return;
          case 'l':	 // 1 string to match.
            Mnemonic = "setge";	 // "setnl"
            return;
          case 'z':	 // 1 string to match.
            Mnemonic = "setne";	 // "setnz"
            return;
          }
          break;
        case 'p':	 // 2 strings to match.
          switch (Mnemonic[4]) {
          default: break;
          case 'e':	 // 1 string to match.
            Mnemonic = "setp";	 // "setpe"
            return;
          case 'o':	 // 1 string to match.
            Mnemonic = "setnp";	 // "setpo"
            return;
          }
          break;
        }
        break;
      case 'g':	 // 2 strings to match.
        if (Mnemonic.substr(2, 2) != "dt")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'l':	 // 1 string to match.
          if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "sgdtl"
            Mnemonic = "sgdt";
          return;
        case 'q':	 // 1 string to match.
          if ((Features & Feature_In64BitMode) == Feature_In64BitMode)	 // "sgdtq"
            Mnemonic = "sgdt";
          return;
        }
        break;
      case 'i':	 // 2 strings to match.
        if (Mnemonic.substr(2, 2) != "dt")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'l':	 // 1 string to match.
          if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "sidtl"
            Mnemonic = "sidt";
          return;
        case 'q':	 // 1 string to match.
          if ((Features & Feature_In64BitMode) == Feature_In64BitMode)	 // "sidtq"
            Mnemonic = "sidt";
          return;
        }
        break;
      case 'm':	 // 4 strings to match.
        if (Mnemonic.substr(2, 2) != "ov")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'b':	 // 1 string to match.
          Mnemonic = "movsb";	 // "smovb"
          return;
        case 'l':	 // 1 string to match.
          Mnemonic = "movsl";	 // "smovl"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "movsq";	 // "smovq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "movsw";	 // "smovw"
          return;
        }
        break;
      }
      break;
    case 'v':	 // 1 string to match.
      if (Mnemonic.substr(1, 4) != "errw")
        break;
      Mnemonic = "verr";	 // "verrw"
      return;
    }
    break;
  case 6:	 // 19 strings to match.
    switch (Mnemonic[0]) {
    default: break;
    case 'c':	 // 6 strings to match.
      if (Mnemonic.substr(1, 3) != "mov")
        break;
      switch (Mnemonic[4]) {
      default: break;
      case 'c':	 // 3 strings to match.
        switch (Mnemonic[5]) {
        default: break;
        case 'l':	 // 1 string to match.
          Mnemonic = "cmovbl";	 // "cmovcl"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "cmovbq";	 // "cmovcq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "cmovbw";	 // "cmovcw"
          return;
        }
        break;
      case 'z':	 // 3 strings to match.
        switch (Mnemonic[5]) {
        default: break;
        case 'l':	 // 1 string to match.
          Mnemonic = "cmovel";	 // "cmovzl"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "cmoveq";	 // "cmovzq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "cmovew";	 // "cmovzw"
          return;
        }
        break;
      }
      break;
    case 'f':	 // 4 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'c':	 // 3 strings to match.
        switch (Mnemonic[2]) {
        default: break;
        case 'm':	 // 2 strings to match.
          if (Mnemonic.substr(3, 2) != "ov")
            break;
          switch (Mnemonic[5]) {
          default: break;
          case 'a':	 // 1 string to match.
            Mnemonic = "fcmovnbe";	 // "fcmova"
            return;
          case 'z':	 // 1 string to match.
            Mnemonic = "fcmove";	 // "fcmovz"
            return;
          }
          break;
        case 'o':	 // 1 string to match.
          if (Mnemonic.substr(3, 3) != "mip")
            break;
          Mnemonic = "fcompi";	 // "fcomip"
          return;
        }
        break;
      case 'l':	 // 1 string to match.
        if (Mnemonic.substr(2, 4) != "dcww")
          break;
        Mnemonic = "fldcw";	 // "fldcww"
        return;
      }
      break;
    case 'l':	 // 3 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'e':	 // 2 strings to match.
        if (Mnemonic.substr(2, 3) != "ave")
          break;
        switch (Mnemonic[5]) {
        default: break;
        case 'l':	 // 1 string to match.
          if ((Features & Feature_In32BitMode) == Feature_In32BitMode)	 // "leavel"
            Mnemonic = "leave";
          return;
        case 'q':	 // 1 string to match.
          if ((Features & Feature_In64BitMode) == Feature_In64BitMode)	 // "leaveq"
            Mnemonic = "leave";
          return;
        }
        break;
      case 'o':	 // 1 string to match.
        if (Mnemonic.substr(2, 4) != "opnz")
          break;
        Mnemonic = "loopne";	 // "loopnz"
        return;
      }
      break;
    case 'p':	 // 1 string to match.
      if (Mnemonic.substr(1, 5) != "ushfd")
        break;
      Mnemonic = "pushfl";	 // "pushfd"
      return;
    case 's':	 // 5 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'e':	 // 4 strings to match.
        if (Mnemonic.substr(2, 2) != "tn")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Mnemonic[5] != 'e')
            break;
          Mnemonic = "setb";	 // "setnae"
          return;
        case 'b':	 // 1 string to match.
          if (Mnemonic[5] != 'e')
            break;
          Mnemonic = "seta";	 // "setnbe"
          return;
        case 'g':	 // 1 string to match.
          if (Mnemonic[5] != 'e')
            break;
          Mnemonic = "setl";	 // "setnge"
          return;
        case 'l':	 // 1 string to match.
          if (Mnemonic[5] != 'e')
            break;
          Mnemonic = "setg";	 // "setnle"
          return;
        }
        break;
      case 'y':	 // 1 string to match.
        if (Mnemonic.substr(2, 4) != "sret")
          break;
        Mnemonic = "sysretl";	 // "sysret"
        return;
      }
      break;
    }
    break;
  case 7:	 // 29 strings to match.
    switch (Mnemonic[0]) {
    default: break;
    case 'c':	 // 24 strings to match.
      if (Mnemonic.substr(1, 3) != "mov")
        break;
      switch (Mnemonic[4]) {
      default: break;
      case 'n':	 // 18 strings to match.
        switch (Mnemonic[5]) {
        default: break;
        case 'a':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovbel";	 // "cmovnal"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovbeq";	 // "cmovnaq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovbew";	 // "cmovnaw"
            return;
          }
          break;
        case 'b':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovael";	 // "cmovnbl"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovaeq";	 // "cmovnbq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovaew";	 // "cmovnbw"
            return;
          }
          break;
        case 'c':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovael";	 // "cmovncl"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovaeq";	 // "cmovncq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovaew";	 // "cmovncw"
            return;
          }
          break;
        case 'g':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovlel";	 // "cmovngl"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovleq";	 // "cmovngq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovlew";	 // "cmovngw"
            return;
          }
          break;
        case 'l':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovgel";	 // "cmovnll"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovgeq";	 // "cmovnlq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovgew";	 // "cmovnlw"
            return;
          }
          break;
        case 'z':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovnel";	 // "cmovnzl"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovneq";	 // "cmovnzq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovnew";	 // "cmovnzw"
            return;
          }
          break;
        }
        break;
      case 'p':	 // 6 strings to match.
        switch (Mnemonic[5]) {
        default: break;
        case 'e':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovpl";	 // "cmovpel"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovpq";	 // "cmovpeq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovpw";	 // "cmovpew"
            return;
          }
          break;
        case 'o':	 // 3 strings to match.
          switch (Mnemonic[6]) {
          default: break;
          case 'l':	 // 1 string to match.
            Mnemonic = "cmovnpl";	 // "cmovpol"
            return;
          case 'q':	 // 1 string to match.
            Mnemonic = "cmovnpq";	 // "cmovpoq"
            return;
          case 'w':	 // 1 string to match.
            Mnemonic = "cmovnpw";	 // "cmovpow"
            return;
          }
          break;
        }
        break;
      }
      break;
    case 'f':	 // 5 strings to match.
      switch (Mnemonic[1]) {
      default: break;
      case 'c':	 // 2 strings to match.
        if (Mnemonic.substr(2, 3) != "mov")
          break;
        switch (Mnemonic[5]) {
        default: break;
        case 'a':	 // 1 string to match.
          if (Mnemonic[6] != 'e')
            break;
          Mnemonic = "fcmovnb";	 // "fcmovae"
          return;
        case 'n':	 // 1 string to match.
          if (Mnemonic[6] != 'a')
            break;
          Mnemonic = "fcmovbe";	 // "fcmovna"
          return;
        }
        break;
      case 'n':	 // 2 strings to match.
        if (Mnemonic.substr(2, 2) != "st")
          break;
        switch (Mnemonic[4]) {
        default: break;
        case 'c':	 // 1 string to match.
          if (Mnemonic.substr(5, 2) != "ww")
            break;
          Mnemonic = "fnstcw";	 // "fnstcww"
          return;
        case 's':	 // 1 string to match.
          if (Mnemonic.substr(5, 2) != "ww")
            break;
          Mnemonic = "fnstsw";	 // "fnstsww"
          return;
        }
        break;
      case 'u':	 // 1 string to match.
        if (Mnemonic.substr(2, 5) != "comip")
          break;
        Mnemonic = "fucompi";	 // "fucomip"
        return;
      }
      break;
    }
    break;
  case 8:	 // 13 strings to match.
    switch (Mnemonic[0]) {
    default: break;
    case 'c':	 // 12 strings to match.
      if (Mnemonic.substr(1, 4) != "movn")
        break;
      switch (Mnemonic[5]) {
      default: break;
      case 'a':	 // 3 strings to match.
        if (Mnemonic[6] != 'e')
          break;
        switch (Mnemonic[7]) {
        default: break;
        case 'l':	 // 1 string to match.
          Mnemonic = "cmovbl";	 // "cmovnael"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "cmovbq";	 // "cmovnaeq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "cmovbw";	 // "cmovnaew"
          return;
        }
        break;
      case 'b':	 // 3 strings to match.
        if (Mnemonic[6] != 'e')
          break;
        switch (Mnemonic[7]) {
        default: break;
        case 'l':	 // 1 string to match.
          Mnemonic = "cmoval";	 // "cmovnbel"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "cmovaq";	 // "cmovnbeq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "cmovaw";	 // "cmovnbew"
          return;
        }
        break;
      case 'g':	 // 3 strings to match.
        if (Mnemonic[6] != 'e')
          break;
        switch (Mnemonic[7]) {
        default: break;
        case 'l':	 // 1 string to match.
          Mnemonic = "cmovll";	 // "cmovngel"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "cmovlq";	 // "cmovngeq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "cmovlw";	 // "cmovngew"
          return;
        }
        break;
      case 'l':	 // 3 strings to match.
        if (Mnemonic[6] != 'e')
          break;
        switch (Mnemonic[7]) {
        default: break;
        case 'l':	 // 1 string to match.
          Mnemonic = "cmovgl";	 // "cmovnlel"
          return;
        case 'q':	 // 1 string to match.
          Mnemonic = "cmovgq";	 // "cmovnleq"
          return;
        case 'w':	 // 1 string to match.
          Mnemonic = "cmovgw";	 // "cmovnlew"
          return;
        }
        break;
      }
      break;
    case 'f':	 // 1 string to match.
      if (Mnemonic.substr(1, 7) != "cmovnae")
        break;
      Mnemonic = "fcmovb";	 // "fcmovnae"
      return;
    }
    break;
  }
}

// Unified function for converting operands to MCInst instances.

enum ConversionKind {
  Convert,
  Convert__imm10,
  Convert__Imm1_0,
  Convert__Reg1_1__Tie0__Reg1_0,
  Convert__Mem5_1__Reg1_0,
  Convert__Reg1_1__Tie0__Imm1_0,
  Convert__Mem5_1__Imm1_0,
  Convert__Reg1_1__Tie0__Mem5_0,
  Convert__Reg1_1__Tie0__ImmSExti32i81_0,
  Convert__Mem5_1__ImmSExti32i81_0,
  Convert__Reg1_1__Tie0__ImmSExti64i81_0,
  Convert__Mem5_1__ImmSExti64i81_0,
  Convert__ImmSExti64i321_0,
  Convert__Reg1_1__Tie0__ImmSExti64i321_0,
  Convert__Mem5_1__ImmSExti64i321_0,
  Convert__Reg1_1__Tie0__ImmSExti16i81_0,
  Convert__Mem5_1__ImmSExti16i81_0,
  Convert__Reg1_1__Reg1_0,
  Convert__Reg1_1__Mem5_0,
  Convert__Reg1_2__Reg1_1__Imm1_0,
  Convert__Reg1_2__Mem5_1__Imm1_0,
  Convert__Reg1_2__Reg1_1__Reg1_0,
  Convert__Reg1_2__Reg1_1__Mem5_0,
  Convert__Reg1_0__Reg1_1,
  Convert__Reg1_0__Mem5_1,
  Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0,
  Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0,
  Convert__Reg1_0__Tie0,
  Convert__Reg1_1__ImmSExti32i81_0,
  Convert__Reg1_1__ImmSExti64i81_0,
  Convert__Reg1_1__ImmSExti16i81_0,
  Convert__Imm1_1__Imm1_0,
  Convert__AbsMem1_0,
  Convert__Reg1_1,
  Convert__Mem5_1,
  Convert__Mem5_0,
  Convert__Reg1_0__Tie0__Reg1_0,
  Convert__Reg1_3__Tie0__Reg1_2__Imm1_0,
  Convert__Reg1_3__Tie0__Mem5_2__Imm1_0,
  Convert__Reg1_1__Imm1_0,
  Convert__Reg1_2__Tie0__Reg1_1__Imm1_0,
  Convert__Reg1_2__Tie0__Mem5_1__Imm1_0,
  Convert__Reg1_1__ImmSExti64i321_0,
  Convert__Reg1_0,
  Convert__Imm1_0__Imm1_1,
  Convert__Reg1_2__Reg1_1__ImmSExti32i81_0,
  Convert__Mem5_2__Reg1_1__ImmSExti32i81_0,
  Convert__regST0,
  Convert__regST1,
  Convert__Reg1_1__Reg1_1__ImmSExti32i81_0,
  Convert__Reg1_1__Reg1_1__Imm1_0,
  Convert__Reg1_2__Mem5_1__ImmSExti32i81_0,
  Convert__Reg1_1__Reg1_1__ImmSExti64i81_0,
  Convert__Reg1_1__Reg1_1__ImmSExti64i321_0,
  Convert__Reg1_2__Reg1_1__ImmSExti64i81_0,
  Convert__Reg1_2__Mem5_1__ImmSExti64i81_0,
  Convert__Reg1_2__Reg1_1__ImmSExti64i321_0,
  Convert__Reg1_2__Mem5_1__ImmSExti64i321_0,
  Convert__Reg1_1__Reg1_1__ImmSExti16i81_0,
  Convert__Reg1_2__Reg1_1__ImmSExti16i81_0,
  Convert__Reg1_2__Mem5_1__ImmSExti16i81_0,
  Convert__AbsMem1_1,
  Convert__Imm1_1,
  Convert__Reg1_1__Tie0__Reg1_0__imm17,
  Convert__Reg1_1__Tie0__Mem5_0__imm17,
  Convert__Reg1_1__Tie0__Reg1_0__imm1,
  Convert__Reg1_1__Tie0__Mem5_0__imm1,
  Convert__Reg1_1__Tie0__Reg1_0__imm16,
  Convert__Reg1_1__Tie0__Mem5_0__imm16,
  Convert__Reg1_1__Tie0__Reg1_0__imm0,
  Convert__Reg1_1__Tie0__Mem5_0__imm0,
  Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0,
  Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0,
  Convert__ImmSExti32i81_0,
  Convert__ImmSExti64i81_0,
  Convert__Reg1_1__Tie0,
  Convert__Reg1_0__Tie0__Reg1_1__imm1,
  Convert__Mem5_0__Reg1_1__imm1,
  Convert__Reg1_2__Tie0__Reg1_1,
  Convert__Mem5_2__Reg1_1,
  Convert__Mem5_2__Reg1_1__Imm1_0,
  Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0,
  Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0,
  Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0,
  Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0,
  Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0,
  Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0,
  Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0,
  Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0,
  Convert__Mem5_2__Reg1_1__Reg1_0,
  Convert__Reg1_2__Reg1_1__Reg1_0__imm17,
  Convert__Reg1_2__Reg1_1__Mem5_0__imm17,
  Convert__Reg1_2__Reg1_1__Reg1_0__imm1,
  Convert__Reg1_2__Reg1_1__Mem5_0__imm1,
  Convert__Reg1_2__Reg1_1__Reg1_0__imm16,
  Convert__Reg1_2__Reg1_1__Mem5_0__imm16,
  Convert__Reg1_2__Reg1_1__Reg1_0__imm0,
  Convert__Reg1_2__Reg1_1__Mem5_0__imm0,
  Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0,
  Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0,
  Convert__Reg1_0__Tie0__Reg1_1,
  Convert__Reg1_0__Tie0__Mem5_1,
  NumConversionVariants
};

bool X86ATTAsmParser::
ConvertToMCInst(unsigned Kind, MCInst &Inst, unsigned Opcode,
                      const SmallVectorImpl<MCParsedAsmOperand*> &Operands) {
  Inst.setOpcode(Opcode);
  switch (Kind) {
  default:
  case Convert:
    return true;
  case Convert__imm10:
    Inst.addOperand(MCOperand::CreateImm(10));
    return true;
  case Convert__Imm1_0:
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__Reg1_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Mem5_1__Reg1_0:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__Imm1_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_1__Imm1_0:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__Mem5_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    return true;
  case Convert__Reg1_1__Tie0__ImmSExti32i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__ImmSExti64i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_1__ImmSExti64i81_0:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__ImmSExti64i321_0:
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__ImmSExti64i321_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_1__ImmSExti64i321_0:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__ImmSExti16i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_1__ImmSExti16i81_0:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Reg1_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Mem5_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    return true;
  case Convert__Reg1_2__Reg1_1__Imm1_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Mem5_1__Imm1_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__Reg1_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__Mem5_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    return true;
  case Convert__Reg1_0__Reg1_1:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_0__Mem5_1:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    return true;
  case Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_0__Tie0:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    return true;
  case Convert__Reg1_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__ImmSExti64i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__ImmSExti16i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Imm1_1__Imm1_0:
    ((X86Operand*)Operands[2])->addImmOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__AbsMem1_0:
    ((X86Operand*)Operands[1])->addAbsMemOperands(Inst, 1);
    return true;
  case Convert__Reg1_1:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    return true;
  case Convert__Mem5_1:
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    return true;
  case Convert__Mem5_0:
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    return true;
  case Convert__Reg1_0__Tie0__Reg1_0:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Tie0__Reg1_2__Imm1_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Tie0__Mem5_2__Imm1_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[3])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Imm1_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Tie0__Reg1_1__Imm1_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Tie0__Mem5_1__Imm1_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__ImmSExti64i321_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_0:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Imm1_0__Imm1_1:
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_2__Reg1_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[3])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__regST0:
    Inst.addOperand(MCOperand::CreateReg(X86::ST0));
    return true;
  case Convert__regST1:
    Inst.addOperand(MCOperand::CreateReg(X86::ST1));
    return true;
  case Convert__Reg1_1__Reg1_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Reg1_1__Imm1_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Mem5_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Reg1_1__ImmSExti64i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Reg1_1__ImmSExti64i321_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__ImmSExti64i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Mem5_1__ImmSExti64i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__ImmSExti64i321_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Mem5_1__ImmSExti64i321_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Reg1_1__ImmSExti16i81_0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__ImmSExti16i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Mem5_1__ImmSExti16i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__AbsMem1_1:
    ((X86Operand*)Operands[2])->addAbsMemOperands(Inst, 1);
    return true;
  case Convert__Imm1_1:
    ((X86Operand*)Operands[2])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0__Reg1_0__imm17:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(17));
    return true;
  case Convert__Reg1_1__Tie0__Mem5_0__imm17:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(17));
    return true;
  case Convert__Reg1_1__Tie0__Reg1_0__imm1:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(1));
    return true;
  case Convert__Reg1_1__Tie0__Mem5_0__imm1:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(1));
    return true;
  case Convert__Reg1_1__Tie0__Reg1_0__imm16:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(16));
    return true;
  case Convert__Reg1_1__Tie0__Mem5_0__imm16:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(16));
    return true;
  case Convert__Reg1_1__Tie0__Reg1_0__imm0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(0));
    return true;
  case Convert__Reg1_1__Tie0__Mem5_0__imm0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(0));
    return true;
  case Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__ImmSExti32i81_0:
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__ImmSExti64i81_0:
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_1__Tie0:
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    return true;
  case Convert__Reg1_0__Tie0__Reg1_1__imm1:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(1));
    return true;
  case Convert__Mem5_0__Reg1_1__imm1:
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(1));
    return true;
  case Convert__Reg1_2__Tie0__Reg1_1:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    return true;
  case Convert__Mem5_2__Reg1_1:
    ((X86Operand*)Operands[3])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    return true;
  case Convert__Mem5_2__Reg1_1__Imm1_0:
    ((X86Operand*)Operands[3])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0:
    ((X86Operand*)Operands[5])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0:
    ((X86Operand*)Operands[5])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Mem5_2__Reg1_1__Reg1_0:
    ((X86Operand*)Operands[3])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_2__Reg1_1__Reg1_0__imm17:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(17));
    return true;
  case Convert__Reg1_2__Reg1_1__Mem5_0__imm17:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(17));
    return true;
  case Convert__Reg1_2__Reg1_1__Reg1_0__imm1:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(1));
    return true;
  case Convert__Reg1_2__Reg1_1__Mem5_0__imm1:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(1));
    return true;
  case Convert__Reg1_2__Reg1_1__Reg1_0__imm16:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(16));
    return true;
  case Convert__Reg1_2__Reg1_1__Mem5_0__imm16:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(16));
    return true;
  case Convert__Reg1_2__Reg1_1__Reg1_0__imm0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(MCOperand::CreateImm(0));
    return true;
  case Convert__Reg1_2__Reg1_1__Mem5_0__imm0:
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addMemOperands(Inst, 5);
    Inst.addOperand(MCOperand::CreateImm(0));
    return true;
  case Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0:
    ((X86Operand*)Operands[4])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[3])->addRegOperands(Inst, 1);
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    ((X86Operand*)Operands[1])->addImmOperands(Inst, 1);
    return true;
  case Convert__Reg1_0__Tie0__Reg1_1:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addRegOperands(Inst, 1);
    return true;
  case Convert__Reg1_0__Tie0__Mem5_1:
    ((X86Operand*)Operands[1])->addRegOperands(Inst, 1);
    Inst.addOperand(Inst.getOperand(0));
    ((X86Operand*)Operands[2])->addMemOperands(Inst, 5);
    return true;
  }
  return false;
}

namespace {

/// MatchClassKind - The kinds of classes which participate in
/// instruction matching.
enum MatchClassKind {
  InvalidMatchClass = 0,
  MCK__STAR_, // '*'
  MCK_pd, // 'pd'
  MCK_ps, // 'ps'
  MCK_sd, // 'sd'
  MCK_ss, // 'ss'
  MCK_AL, // register class 'AL'
  MCK_CL, // register class 'CL'
  MCK_GR8_ABCD_L, // register class 'GR8_ABCD_L'
  MCK_GR8_ABCD_H, // register class 'GR8_ABCD_H'
  MCK_GR8_NOREX, // register class 'GR8_NOREX'
  MCK_GR8, // register class 'GR8'
  MCK_AX, // register class 'AX'
  MCK_DX, // register class 'DX'
  MCK_GR16_ABCD, // register class 'GR16_ABCD'
  MCK_GR16_NOREX, // register class 'GR16_NOREX'
  MCK_GR16, // register class 'GR16'
  MCK_EAX, // register class 'EAX'
  MCK_EDX, // register class 'EDX'
  MCK_Reg20, // derived register class
  MCK_Reg21, // derived register class
  MCK_Reg22, // derived register class
  MCK_Reg24, // derived register class
  MCK_GR32_AD, // register class 'GR32_AD'
  MCK_ECX, // register class 'ECX'
  MCK_GR32_TC, // register class 'GR32_TC'
  MCK_GR32_ABCD, // register class 'GR32_ABCD'
  MCK_GR32_NOAX, // register class 'GR32_NOAX'
  MCK_GR32_NOREX_NOSP, // register class 'GR32_NOREX_NOSP'
  MCK_GR32_NOREX, // register class 'GR32_NOREX'
  MCK_GR32_NOSP, // register class 'GR32_NOSP'
  MCK_GR32, // register class 'GR32'
  MCK_RAX, // register class 'RAX'
  MCK_RDX, // register class 'RDX'
  MCK_RCX, // register class 'RCX'
  MCK_Reg27, // derived register class
  MCK_Reg35, // derived register class
  MCK_GR64_ABCD, // register class 'GR64_ABCD'
  MCK_GR64_NOREX_NOSP, // register class 'GR64_NOREX_NOSP'
  MCK_Reg30, // derived register class
  MCK_Reg31, // derived register class
  MCK_GR64_NOREX, // register class 'GR64_NOREX'
  MCK_GR64_TCW64, // register class 'GR64_TCW64'
  MCK_Reg36, // derived register class
  MCK_Reg38, // derived register class
  MCK_GR64_NOSP, // register class 'GR64_NOSP'
  MCK_GR64_TC, // register class 'GR64_TC'
  MCK_GR64, // register class 'GR64'
  MCK_VR64, // register class 'VR64'
  MCK_RFP32, // register class 'RFP32,RFP64,RFP80'
  MCK_FR32, // register class 'FR32,FR64,VR128'
  MCK_VR256, // register class 'VR256'
  MCK_ST0, // register class 'ST0'
  MCK_RST, // register class 'RST'
  MCK_CCR, // register class 'CCR'
  MCK_CS, // register class 'CS'
  MCK_DS, // register class 'DS'
  MCK_SS, // register class 'SS'
  MCK_ES, // register class 'ES'
  MCK_FS, // register class 'FS'
  MCK_GS, // register class 'GS'
  MCK_SEGMENT_REG, // register class 'SEGMENT_REG'
  MCK_DEBUG_REG, // register class 'DEBUG_REG'
  MCK_CONTROL_REG, // register class 'CONTROL_REG'
  MCK_ImmSExti64i8, // user defined class 'ImmSExti64i8AsmOperand'
  MCK_ImmSExti16i8, // user defined class 'ImmSExti16i8AsmOperand'
  MCK_ImmSExti32i8, // user defined class 'ImmSExti32i8AsmOperand'
  MCK_ImmSExti64i32, // user defined class 'ImmSExti64i32AsmOperand'
  MCK_ImmZExtu32u8, // user defined class 'ImmZExtu32u8AsmOperand'
  MCK_Imm, // user defined class 'ImmAsmOperand'
  MCK_AbsMem, // user defined class 'X86AbsMemAsmOperand'
  MCK_Mem, // user defined class 'X86MemAsmOperand'
  NumMatchClassKinds
};

}

static MatchClassKind MatchTokenString(StringRef Name) {
  switch (Name.size()) {
  default: break;
  case 1:	 // 1 string to match.
    if (Name[0] != '*')
      break;
    return MCK__STAR_;	 // "*"
  case 2:	 // 4 strings to match.
    switch (Name[0]) {
    default: break;
    case 'p':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'd':	 // 1 string to match.
        return MCK_pd;	 // "pd"
      case 's':	 // 1 string to match.
        return MCK_ps;	 // "ps"
      }
      break;
    case 's':	 // 2 strings to match.
      switch (Name[1]) {
      default: break;
      case 'd':	 // 1 string to match.
        return MCK_sd;	 // "sd"
      case 's':	 // 1 string to match.
        return MCK_ss;	 // "ss"
      }
      break;
    }
    break;
  }
  return InvalidMatchClass;
}

/// IsSubclass - Compute whether \arg A is a subclass of \arg B.
static bool IsSubclass(MatchClassKind A, MatchClassKind B) {
  if (A == B)
    return true;

  switch (A) {
  default:
    return false;

  case MCK_AL:
    switch (B) {
    default: return false;
    case MCK_GR8_ABCD_L: return true;
    case MCK_GR8_NOREX: return true;
    case MCK_GR8: return true;
    }

  case MCK_CL:
    switch (B) {
    default: return false;
    case MCK_GR8_ABCD_L: return true;
    case MCK_GR8_NOREX: return true;
    case MCK_GR8: return true;
    }

  case MCK_GR8_ABCD_L:
    switch (B) {
    default: return false;
    case MCK_GR8_NOREX: return true;
    case MCK_GR8: return true;
    }

  case MCK_GR8_ABCD_H:
    switch (B) {
    default: return false;
    case MCK_GR8_NOREX: return true;
    case MCK_GR8: return true;
    }

  case MCK_GR8_NOREX:
    return B == MCK_GR8;

  case MCK_AX:
    switch (B) {
    default: return false;
    case MCK_GR16_ABCD: return true;
    case MCK_GR16_NOREX: return true;
    case MCK_GR16: return true;
    }

  case MCK_DX:
    switch (B) {
    default: return false;
    case MCK_GR16_ABCD: return true;
    case MCK_GR16_NOREX: return true;
    case MCK_GR16: return true;
    }

  case MCK_GR16_ABCD:
    switch (B) {
    default: return false;
    case MCK_GR16_NOREX: return true;
    case MCK_GR16: return true;
    }

  case MCK_GR16_NOREX:
    return B == MCK_GR16;

  case MCK_EAX:
    switch (B) {
    default: return false;
    case MCK_GR32_AD: return true;
    case MCK_GR32_TC: return true;
    case MCK_GR32_ABCD: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_EDX:
    switch (B) {
    default: return false;
    case MCK_Reg20: return true;
    case MCK_Reg21: return true;
    case MCK_Reg22: return true;
    case MCK_Reg24: return true;
    case MCK_GR32_AD: return true;
    case MCK_GR32_TC: return true;
    case MCK_GR32_ABCD: return true;
    case MCK_GR32_NOAX: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_Reg20:
    switch (B) {
    default: return false;
    case MCK_Reg21: return true;
    case MCK_Reg22: return true;
    case MCK_Reg24: return true;
    case MCK_GR32_ABCD: return true;
    case MCK_GR32_NOAX: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_Reg21:
    switch (B) {
    default: return false;
    case MCK_Reg22: return true;
    case MCK_Reg24: return true;
    case MCK_GR32_NOAX: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_Reg22:
    switch (B) {
    default: return false;
    case MCK_GR32_NOAX: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32: return true;
    }

  case MCK_Reg24:
    switch (B) {
    default: return false;
    case MCK_GR32_NOAX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_GR32_AD:
    switch (B) {
    default: return false;
    case MCK_GR32_TC: return true;
    case MCK_GR32_ABCD: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_ECX:
    switch (B) {
    default: return false;
    case MCK_Reg20: return true;
    case MCK_Reg21: return true;
    case MCK_Reg22: return true;
    case MCK_Reg24: return true;
    case MCK_GR32_TC: return true;
    case MCK_GR32_ABCD: return true;
    case MCK_GR32_NOAX: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_GR32_TC:
    switch (B) {
    default: return false;
    case MCK_GR32_ABCD: return true;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_GR32_ABCD:
    switch (B) {
    default: return false;
    case MCK_GR32_NOREX_NOSP: return true;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_GR32_NOAX:
    return B == MCK_GR32;

  case MCK_GR32_NOREX_NOSP:
    switch (B) {
    default: return false;
    case MCK_GR32_NOREX: return true;
    case MCK_GR32_NOSP: return true;
    case MCK_GR32: return true;
    }

  case MCK_GR32_NOREX:
    return B == MCK_GR32;

  case MCK_GR32_NOSP:
    return B == MCK_GR32;

  case MCK_RAX:
    switch (B) {
    default: return false;
    case MCK_Reg27: return true;
    case MCK_Reg35: return true;
    case MCK_GR64_ABCD: return true;
    case MCK_GR64_NOREX_NOSP: return true;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_TCW64: return true;
    case MCK_Reg36: return true;
    case MCK_Reg38: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_RDX:
    switch (B) {
    default: return false;
    case MCK_Reg27: return true;
    case MCK_Reg35: return true;
    case MCK_GR64_ABCD: return true;
    case MCK_GR64_NOREX_NOSP: return true;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_TCW64: return true;
    case MCK_Reg36: return true;
    case MCK_Reg38: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_RCX:
    switch (B) {
    default: return false;
    case MCK_Reg27: return true;
    case MCK_Reg35: return true;
    case MCK_GR64_ABCD: return true;
    case MCK_GR64_NOREX_NOSP: return true;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_TCW64: return true;
    case MCK_Reg36: return true;
    case MCK_Reg38: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_Reg27:
    switch (B) {
    default: return false;
    case MCK_Reg35: return true;
    case MCK_GR64_ABCD: return true;
    case MCK_GR64_NOREX_NOSP: return true;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_TCW64: return true;
    case MCK_Reg36: return true;
    case MCK_Reg38: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_Reg35:
    switch (B) {
    default: return false;
    case MCK_GR64_NOREX_NOSP: return true;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_Reg36: return true;
    case MCK_Reg38: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_GR64_ABCD:
    switch (B) {
    default: return false;
    case MCK_GR64_NOREX_NOSP: return true;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64: return true;
    }

  case MCK_GR64_NOREX_NOSP:
    switch (B) {
    default: return false;
    case MCK_Reg30: return true;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64: return true;
    }

  case MCK_Reg30:
    switch (B) {
    default: return false;
    case MCK_Reg31: return true;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64: return true;
    }

  case MCK_Reg31:
    return B == MCK_GR64;

  case MCK_GR64_NOREX:
    return B == MCK_GR64;

  case MCK_GR64_TCW64:
    switch (B) {
    default: return false;
    case MCK_Reg31: return true;
    case MCK_Reg36: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_Reg36:
    switch (B) {
    default: return false;
    case MCK_Reg31: return true;
    case MCK_GR64_NOSP: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_Reg38:
    switch (B) {
    default: return false;
    case MCK_GR64_NOREX: return true;
    case MCK_GR64_TC: return true;
    case MCK_GR64: return true;
    }

  case MCK_GR64_NOSP:
    switch (B) {
    default: return false;
    case MCK_Reg31: return true;
    case MCK_GR64: return true;
    }

  case MCK_GR64_TC:
    return B == MCK_GR64;

  case MCK_ST0:
    return B == MCK_RST;

  case MCK_CS:
    return B == MCK_SEGMENT_REG;

  case MCK_DS:
    return B == MCK_SEGMENT_REG;

  case MCK_SS:
    return B == MCK_SEGMENT_REG;

  case MCK_ES:
    return B == MCK_SEGMENT_REG;

  case MCK_FS:
    return B == MCK_SEGMENT_REG;

  case MCK_GS:
    return B == MCK_SEGMENT_REG;

  case MCK_ImmSExti64i8:
    switch (B) {
    default: return false;
    case MCK_ImmSExti16i8: return true;
    case MCK_ImmSExti32i8: return true;
    case MCK_ImmSExti64i32: return true;
    case MCK_Imm: return true;
    }

  case MCK_ImmSExti16i8:
    switch (B) {
    default: return false;
    case MCK_ImmSExti64i32: return true;
    case MCK_Imm: return true;
    }

  case MCK_ImmSExti32i8:
    return B == MCK_Imm;

  case MCK_ImmSExti64i32:
    return B == MCK_Imm;

  case MCK_ImmZExtu32u8:
    return B == MCK_Imm;

  case MCK_AbsMem:
    return B == MCK_Mem;
  }
}

static bool ValidateOperandClass(MCParsedAsmOperand *GOp, MatchClassKind Kind) {
  X86Operand &Operand = *(X86Operand*)GOp;
  if (Kind == InvalidMatchClass)
    return false;

  if (Operand.isToken())
    return MatchTokenString(Operand.getToken()) == Kind;

  if (Operand.isReg()) {
    MatchClassKind OpKind;
    switch (Operand.getReg()) {
    default: OpKind = InvalidMatchClass; break;
    case X86::AL: OpKind = MCK_AL; break;
    case X86::DL: OpKind = MCK_GR8_ABCD_L; break;
    case X86::CL: OpKind = MCK_CL; break;
    case X86::BL: OpKind = MCK_GR8_ABCD_L; break;
    case X86::SIL: OpKind = MCK_GR8; break;
    case X86::DIL: OpKind = MCK_GR8; break;
    case X86::BPL: OpKind = MCK_GR8; break;
    case X86::SPL: OpKind = MCK_GR8; break;
    case X86::R8B: OpKind = MCK_GR8; break;
    case X86::R9B: OpKind = MCK_GR8; break;
    case X86::R10B: OpKind = MCK_GR8; break;
    case X86::R11B: OpKind = MCK_GR8; break;
    case X86::R12B: OpKind = MCK_GR8; break;
    case X86::R13B: OpKind = MCK_GR8; break;
    case X86::R14B: OpKind = MCK_GR8; break;
    case X86::R15B: OpKind = MCK_GR8; break;
    case X86::AH: OpKind = MCK_GR8_ABCD_H; break;
    case X86::DH: OpKind = MCK_GR8_ABCD_H; break;
    case X86::CH: OpKind = MCK_GR8_ABCD_H; break;
    case X86::BH: OpKind = MCK_GR8_ABCD_H; break;
    case X86::AX: OpKind = MCK_AX; break;
    case X86::DX: OpKind = MCK_DX; break;
    case X86::CX: OpKind = MCK_GR16_ABCD; break;
    case X86::BX: OpKind = MCK_GR16_ABCD; break;
    case X86::SI: OpKind = MCK_GR16_NOREX; break;
    case X86::DI: OpKind = MCK_GR16_NOREX; break;
    case X86::BP: OpKind = MCK_GR16_NOREX; break;
    case X86::SP: OpKind = MCK_GR16_NOREX; break;
    case X86::R8W: OpKind = MCK_GR16; break;
    case X86::R9W: OpKind = MCK_GR16; break;
    case X86::R10W: OpKind = MCK_GR16; break;
    case X86::R11W: OpKind = MCK_GR16; break;
    case X86::R12W: OpKind = MCK_GR16; break;
    case X86::R13W: OpKind = MCK_GR16; break;
    case X86::R14W: OpKind = MCK_GR16; break;
    case X86::R15W: OpKind = MCK_GR16; break;
    case X86::EAX: OpKind = MCK_EAX; break;
    case X86::EDX: OpKind = MCK_EDX; break;
    case X86::ECX: OpKind = MCK_ECX; break;
    case X86::EBX: OpKind = MCK_Reg20; break;
    case X86::ESI: OpKind = MCK_Reg21; break;
    case X86::EDI: OpKind = MCK_Reg21; break;
    case X86::EBP: OpKind = MCK_Reg21; break;
    case X86::ESP: OpKind = MCK_Reg22; break;
    case X86::R8D: OpKind = MCK_Reg24; break;
    case X86::R9D: OpKind = MCK_Reg24; break;
    case X86::R10D: OpKind = MCK_Reg24; break;
    case X86::R11D: OpKind = MCK_Reg24; break;
    case X86::R12D: OpKind = MCK_Reg24; break;
    case X86::R13D: OpKind = MCK_Reg24; break;
    case X86::R14D: OpKind = MCK_Reg24; break;
    case X86::R15D: OpKind = MCK_Reg24; break;
    case X86::RAX: OpKind = MCK_RAX; break;
    case X86::RDX: OpKind = MCK_RDX; break;
    case X86::RCX: OpKind = MCK_RCX; break;
    case X86::RBX: OpKind = MCK_GR64_ABCD; break;
    case X86::RSI: OpKind = MCK_Reg35; break;
    case X86::RDI: OpKind = MCK_Reg35; break;
    case X86::RBP: OpKind = MCK_GR64_NOREX_NOSP; break;
    case X86::RSP: OpKind = MCK_Reg30; break;
    case X86::R8: OpKind = MCK_GR64_TCW64; break;
    case X86::R9: OpKind = MCK_GR64_TCW64; break;
    case X86::R10: OpKind = MCK_GR64_NOSP; break;
    case X86::R11: OpKind = MCK_GR64_TCW64; break;
    case X86::R12: OpKind = MCK_GR64_NOSP; break;
    case X86::R13: OpKind = MCK_GR64_NOSP; break;
    case X86::R14: OpKind = MCK_GR64_NOSP; break;
    case X86::R15: OpKind = MCK_GR64_NOSP; break;
    case X86::RIP: OpKind = MCK_Reg38; break;
    case X86::MM0: OpKind = MCK_VR64; break;
    case X86::MM1: OpKind = MCK_VR64; break;
    case X86::MM2: OpKind = MCK_VR64; break;
    case X86::MM3: OpKind = MCK_VR64; break;
    case X86::MM4: OpKind = MCK_VR64; break;
    case X86::MM5: OpKind = MCK_VR64; break;
    case X86::MM6: OpKind = MCK_VR64; break;
    case X86::MM7: OpKind = MCK_VR64; break;
    case X86::FP0: OpKind = MCK_RFP32; break;
    case X86::FP1: OpKind = MCK_RFP32; break;
    case X86::FP2: OpKind = MCK_RFP32; break;
    case X86::FP3: OpKind = MCK_RFP32; break;
    case X86::FP4: OpKind = MCK_RFP32; break;
    case X86::FP5: OpKind = MCK_RFP32; break;
    case X86::FP6: OpKind = MCK_RFP32; break;
    case X86::XMM0: OpKind = MCK_FR32; break;
    case X86::XMM1: OpKind = MCK_FR32; break;
    case X86::XMM2: OpKind = MCK_FR32; break;
    case X86::XMM3: OpKind = MCK_FR32; break;
    case X86::XMM4: OpKind = MCK_FR32; break;
    case X86::XMM5: OpKind = MCK_FR32; break;
    case X86::XMM6: OpKind = MCK_FR32; break;
    case X86::XMM7: OpKind = MCK_FR32; break;
    case X86::XMM8: OpKind = MCK_FR32; break;
    case X86::XMM9: OpKind = MCK_FR32; break;
    case X86::XMM10: OpKind = MCK_FR32; break;
    case X86::XMM11: OpKind = MCK_FR32; break;
    case X86::XMM12: OpKind = MCK_FR32; break;
    case X86::XMM13: OpKind = MCK_FR32; break;
    case X86::XMM14: OpKind = MCK_FR32; break;
    case X86::XMM15: OpKind = MCK_FR32; break;
    case X86::YMM0: OpKind = MCK_VR256; break;
    case X86::YMM1: OpKind = MCK_VR256; break;
    case X86::YMM2: OpKind = MCK_VR256; break;
    case X86::YMM3: OpKind = MCK_VR256; break;
    case X86::YMM4: OpKind = MCK_VR256; break;
    case X86::YMM5: OpKind = MCK_VR256; break;
    case X86::YMM6: OpKind = MCK_VR256; break;
    case X86::YMM7: OpKind = MCK_VR256; break;
    case X86::YMM8: OpKind = MCK_VR256; break;
    case X86::YMM9: OpKind = MCK_VR256; break;
    case X86::YMM10: OpKind = MCK_VR256; break;
    case X86::YMM11: OpKind = MCK_VR256; break;
    case X86::YMM12: OpKind = MCK_VR256; break;
    case X86::YMM13: OpKind = MCK_VR256; break;
    case X86::YMM14: OpKind = MCK_VR256; break;
    case X86::YMM15: OpKind = MCK_VR256; break;
    case X86::ST0: OpKind = MCK_ST0; break;
    case X86::ST1: OpKind = MCK_RST; break;
    case X86::ST2: OpKind = MCK_RST; break;
    case X86::ST3: OpKind = MCK_RST; break;
    case X86::ST4: OpKind = MCK_RST; break;
    case X86::ST5: OpKind = MCK_RST; break;
    case X86::ST6: OpKind = MCK_RST; break;
    case X86::ST7: OpKind = MCK_RST; break;
    case X86::EFLAGS: OpKind = MCK_CCR; break;
    case X86::CS: OpKind = MCK_CS; break;
    case X86::DS: OpKind = MCK_DS; break;
    case X86::SS: OpKind = MCK_SS; break;
    case X86::ES: OpKind = MCK_ES; break;
    case X86::FS: OpKind = MCK_FS; break;
    case X86::GS: OpKind = MCK_GS; break;
    case X86::DR0: OpKind = MCK_DEBUG_REG; break;
    case X86::DR1: OpKind = MCK_DEBUG_REG; break;
    case X86::DR2: OpKind = MCK_DEBUG_REG; break;
    case X86::DR3: OpKind = MCK_DEBUG_REG; break;
    case X86::DR4: OpKind = MCK_DEBUG_REG; break;
    case X86::DR5: OpKind = MCK_DEBUG_REG; break;
    case X86::DR6: OpKind = MCK_DEBUG_REG; break;
    case X86::DR7: OpKind = MCK_DEBUG_REG; break;
    case X86::CR0: OpKind = MCK_CONTROL_REG; break;
    case X86::CR1: OpKind = MCK_CONTROL_REG; break;
    case X86::CR2: OpKind = MCK_CONTROL_REG; break;
    case X86::CR3: OpKind = MCK_CONTROL_REG; break;
    case X86::CR4: OpKind = MCK_CONTROL_REG; break;
    case X86::CR5: OpKind = MCK_CONTROL_REG; break;
    case X86::CR6: OpKind = MCK_CONTROL_REG; break;
    case X86::CR7: OpKind = MCK_CONTROL_REG; break;
    case X86::CR8: OpKind = MCK_CONTROL_REG; break;
    case X86::CR9: OpKind = MCK_CONTROL_REG; break;
    case X86::CR10: OpKind = MCK_CONTROL_REG; break;
    case X86::CR11: OpKind = MCK_CONTROL_REG; break;
    case X86::CR12: OpKind = MCK_CONTROL_REG; break;
    case X86::CR13: OpKind = MCK_CONTROL_REG; break;
    case X86::CR14: OpKind = MCK_CONTROL_REG; break;
    case X86::CR15: OpKind = MCK_CONTROL_REG; break;
    }
    return IsSubclass(OpKind, Kind);
  }

  // 'ImmSExti64i8' class
  if (Kind == MCK_ImmSExti64i8 && Operand.isImmSExti64i8()) {
    return true;
  }

  // 'ImmSExti16i8' class
  if (Kind == MCK_ImmSExti16i8 && Operand.isImmSExti16i8()) {
    return true;
  }

  // 'ImmSExti32i8' class
  if (Kind == MCK_ImmSExti32i8 && Operand.isImmSExti32i8()) {
    return true;
  }

  // 'ImmSExti64i32' class
  if (Kind == MCK_ImmSExti64i32 && Operand.isImmSExti64i32()) {
    return true;
  }

  // 'ImmZExtu32u8' class
  if (Kind == MCK_ImmZExtu32u8 && Operand.isImmZExtu32u8()) {
    return true;
  }

  // 'Imm' class
  if (Kind == MCK_Imm && Operand.isImm()) {
    return true;
  }

  // 'AbsMem' class
  if (Kind == MCK_AbsMem && Operand.isAbsMem()) {
    return true;
  }

  // 'Mem' class
  if (Kind == MCK_Mem && Operand.isMem()) {
    return true;
  }

  return false;
}

unsigned X86ATTAsmParser::
ComputeAvailableFeatures(uint64_t FB) const {
  unsigned Features = 0;
  if (((FB & X86::Mode64Bit) == 0))
    Features |= Feature_In32BitMode;
  if (((FB & X86::Mode64Bit) != 0))
    Features |= Feature_In64BitMode;
  if (((FB & X86::ModeNaCl) != 0))
    Features |= Feature_IsNaCl;
  if (((FB & X86::ModeNaCl) != 0) && ((FB & X86::Mode64Bit) == 0))
    Features |= Feature_IsNaCl32;
  if (((FB & X86::ModeNaCl) != 0) && ((FB & X86::Mode64Bit) != 0))
    Features |= Feature_IsNaCl64;
  if (((FB & X86::ModeNaCl) == 0))
    Features |= Feature_NotNaCl;
  return Features;
}

namespace {
  struct MatchEntry {
    unsigned Opcode;
    const char *Mnemonic;
    ConversionKind ConvertFn;
    MatchClassKind Classes[5];
    unsigned RequiredFeatures;
  };

  // Predicate for searching for an opcode.
  struct LessOpcode {
    bool operator()(const MatchEntry &LHS, StringRef RHS) {
      return StringRef(LHS.Mnemonic) < RHS;
    }
    bool operator()(StringRef LHS, const MatchEntry &RHS) {
      return LHS < StringRef(RHS.Mnemonic);
    }
    bool operator()(const MatchEntry &LHS, const MatchEntry &RHS) {
      return StringRef(LHS.Mnemonic) < StringRef(RHS.Mnemonic);
    }
  };
} // end anonymous namespace.

static const MatchEntry MatchTable[3300] = {
  { X86::AAA, "aaa", Convert, {  }, Feature_In32BitMode},
  { X86::AAD8i8, "aad", Convert__imm10, {  }, 0},
  { X86::AAD8i8, "aad", Convert__Imm1_0, { MCK_Imm }, Feature_In32BitMode},
  { X86::AAM8i8, "aam", Convert__imm10, {  }, 0},
  { X86::AAM8i8, "aam", Convert__Imm1_0, { MCK_Imm }, Feature_In32BitMode},
  { X86::AAS, "aas", Convert, {  }, Feature_In32BitMode},
  { X86::ADC8rr, "adcb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::ADC8mr, "adcb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::ADC8i8, "adcb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::ADC8ri, "adcb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::ADC8mi, "adcb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ADC8rm, "adcb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::ADC32rr, "adcl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::ADC32mr, "adcl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::ADC32ri8, "adcl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::ADC32mi8, "adcl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::ADC32i32, "adcl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::ADC32ri, "adcl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::ADC32mi, "adcl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ADC32rm, "adcl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::ADC64rr, "adcq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::ADC64mr, "adcq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::ADC64ri8, "adcq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::ADC64mi8, "adcq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::ADC64i32, "adcq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::ADC64ri32, "adcq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::ADC64mi32, "adcq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::ADC64rm, "adcq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::ADC16rr, "adcw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::ADC16mr, "adcw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::ADC16ri8, "adcw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::ADC16mi8, "adcw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::ADC16i16, "adcw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::ADC16ri, "adcw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::ADC16mi, "adcw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ADC16rm, "adcw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::ADD8rr, "addb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::ADD8mr, "addb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::ADD8i8, "addb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::ADD8ri, "addb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::ADD8mi, "addb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ADD8rm, "addb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::ADD32rr, "addl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::ADD32mr, "addl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::ADD32ri8, "addl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::ADD32mi8, "addl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::ADD32i32, "addl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::ADD32ri, "addl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::ADD32mi, "addl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ADD32rm, "addl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::ADDPDrr, "addpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ADDPDrm, "addpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ADDPSrr, "addps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ADDPSrm, "addps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ADD64rr, "addq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::ADD64mr, "addq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::ADD64ri8, "addq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::ADD64mi8, "addq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::ADD64i32, "addq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::ADD64ri32, "addq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::ADD64mi32, "addq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::ADD64rm, "addq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::ADDSDrr, "addsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ADDSDrm, "addsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ADDSSrr, "addss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ADDSSrm, "addss", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ADDSUBPDrr, "addsubpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ADDSUBPDrm, "addsubpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ADDSUBPSrr, "addsubps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ADDSUBPSrm, "addsubps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ADD16rr, "addw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::ADD16mr, "addw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::ADD16ri8, "addw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::ADD16mi8, "addw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::ADD16i16, "addw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::ADD16ri, "addw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::ADD16mi, "addw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ADD16rm, "addw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::AESDECrr, "aesdec", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::AESDECrm, "aesdec", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::AESDECLASTrr, "aesdeclast", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::AESDECLASTrm, "aesdeclast", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::AESENCrr, "aesenc", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::AESENCrm, "aesenc", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::AESENCLASTrr, "aesenclast", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::AESENCLASTrm, "aesenclast", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::AESIMCrr, "aesimc", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::AESIMCrm, "aesimc", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::AESKEYGENASSIST128rr, "aeskeygenassist", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::AESKEYGENASSIST128rm, "aeskeygenassist", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::AND8rr, "andb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::AND8mr, "andb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::AND8i8, "andb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::AND8ri, "andb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::AND8mi, "andb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::AND8rm, "andb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::AND32rr, "andl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::AND32mr, "andl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::AND32ri8, "andl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::AND32mi8, "andl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::AND32i32, "andl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::AND32ri, "andl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::AND32mi, "andl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::AND32rm, "andl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::ANDN32rr, "andnl", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32, MCK_GR32 }, 0},
  { X86::ANDN32rm, "andnl", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32, MCK_GR32 }, 0},
  { X86::ANDNPDrr, "andnpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsANDNPDrr, "andnpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ANDNPDrm, "andnpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsANDNPDrm, "andnpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ANDNPSrr, "andnps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsANDNPSrr, "andnps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ANDNPSrm, "andnps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsANDNPSrm, "andnps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ANDN64rr, "andnq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64, MCK_GR64 }, 0},
  { X86::ANDN64rm, "andnq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64, MCK_GR64 }, 0},
  { X86::ANDPDrr, "andpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsANDPDrr, "andpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ANDPDrm, "andpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsANDPDrm, "andpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ANDPSrr, "andps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsANDPSrr, "andps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ANDPSrm, "andps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsANDPSrm, "andps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::AND64rr, "andq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::AND64mr, "andq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::AND64ri8, "andq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::AND64mi8, "andq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::AND64i32, "andq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::AND64ri32, "andq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::AND64mi32, "andq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::AND64rm, "andq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::AND16rr, "andw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::AND16mr, "andw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::AND16ri8, "andw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::AND16mi8, "andw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::AND16i16, "andw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::AND16ri, "andw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::AND16mi, "andw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::AND16rm, "andw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::ARPL16rr, "arpl", Convert__Reg1_0__Reg1_1, { MCK_GR16, MCK_GR16 }, Feature_In32BitMode},
  { X86::ARPL16mr, "arpl", Convert__Reg1_0__Mem5_1, { MCK_GR16, MCK_Mem }, Feature_In32BitMode},
  { X86::BLENDPDrri, "blendpd", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::BLENDPDrmi, "blendpd", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::BLENDPSrri, "blendps", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::BLENDPSrmi, "blendps", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::BLENDVPDrr0, "blendvpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::BLENDVPDrm0, "blendvpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::BLENDVPSrr0, "blendvps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::BLENDVPSrm0, "blendvps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::BOUNDS16rm, "bound", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, Feature_In32BitMode},
  { X86::BOUNDS32rm, "bound", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, Feature_In32BitMode},
  { X86::BSF32rr, "bsfl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::BSF32rm, "bsfl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::BSF64rr, "bsfq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::BSF64rm, "bsfq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::BSF16rr, "bsfw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::BSF16rm, "bsfw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::BSR32rr, "bsrl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::BSR32rm, "bsrl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::BSR64rr, "bsrq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::BSR64rm, "bsrq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::BSR16rr, "bsrw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::BSR16rm, "bsrw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::BSWAP32r, "bswapl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::BSWAP64r, "bswapq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::BT32mi8, "bt", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::BTC32rr, "btcl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::BTC32mr, "btcl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::BTC32ri8, "btcl", Convert__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::BTC32mi8, "btcl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::BTC64rr, "btcq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::BTC64mr, "btcq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::BTC64ri8, "btcq", Convert__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::BTC64mi8, "btcq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::BTC16rr, "btcw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::BTC16mr, "btcw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::BTC16ri8, "btcw", Convert__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::BTC16mi8, "btcw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::BT32rr, "btl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::BT32mr, "btl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::BT32ri8, "btl", Convert__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::BT32mi8, "btl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::BT64rr, "btq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::BT64mr, "btq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::BT64ri8, "btq", Convert__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::BT64mi8, "btq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::BTR32rr, "btrl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::BTR32mr, "btrl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::BTR32ri8, "btrl", Convert__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::BTR32mi8, "btrl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::BTR64rr, "btrq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::BTR64mr, "btrq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::BTR64ri8, "btrq", Convert__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::BTR64mi8, "btrq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::BTR16rr, "btrw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::BTR16mr, "btrw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::BTR16ri8, "btrw", Convert__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::BTR16mi8, "btrw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::BTS32rr, "btsl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::BTS32mr, "btsl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::BTS32ri8, "btsl", Convert__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::BTS32mi8, "btsl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::BTS64rr, "btsq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::BTS64mr, "btsq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::BTS64ri8, "btsq", Convert__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::BTS64mi8, "btsq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::BTS16rr, "btsw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::BTS16mr, "btsw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::BTS16ri8, "btsw", Convert__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::BTS16mi8, "btsw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::BT16rr, "btw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::BT16mr, "btw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::BT16ri8, "btw", Convert__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::BT16mi8, "btw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::FARCALL32i, "call", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::CALLpcrel32, "calll", Convert__AbsMem1_0, { MCK_AbsMem }, Feature_In32BitMode},
  { X86::CALL32r, "calll", Convert__Reg1_1, { MCK__STAR_, MCK_GR32 }, Feature_In32BitMode},
  { X86::CALL32m, "calll", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, Feature_In32BitMode},
  { X86::FARCALL32i, "calll", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::CALL64pcrel32, "callq", Convert__AbsMem1_0, { MCK_AbsMem }, Feature_In64BitMode},
  { X86::CALL64r, "callq", Convert__Reg1_1, { MCK__STAR_, MCK_GR64 }, Feature_In64BitMode},
  { X86::CALL64m, "callq", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, Feature_In64BitMode},
  { X86::CALLpcrel16, "callw", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::FARCALL16i, "callw", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::CBW, "cbtw", Convert, {  }, 0},
  { X86::CLC, "clc", Convert, {  }, 0},
  { X86::CLD, "cld", Convert, {  }, 0},
  { X86::CLFLUSH, "clflush", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::CLI, "cli", Convert, {  }, 0},
  { X86::XOR8rr, "clrb", Convert__Reg1_0__Tie0__Reg1_0, { MCK_GR8 }, 0},
  { X86::XOR32rr, "clrl", Convert__Reg1_0__Tie0__Reg1_0, { MCK_GR32 }, 0},
  { X86::XOR64rr, "clrq", Convert__Reg1_0__Tie0__Reg1_0, { MCK_GR64 }, 0},
  { X86::XOR16rr, "clrw", Convert__Reg1_0__Tie0__Reg1_0, { MCK_GR16 }, 0},
  { X86::CDQ, "cltd", Convert, {  }, 0},
  { X86::CDQE, "cltq", Convert, {  }, 0},
  { X86::CLTS, "clts", Convert, {  }, 0},
  { X86::CMC, "cmc", Convert, {  }, 0},
  { X86::CMOVAE32rr, "cmovael", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVAE32rm, "cmovael", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVAE64rr, "cmovaeq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVAE64rm, "cmovaeq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVAE16rr, "cmovaew", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVAE16rm, "cmovaew", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVA32rr, "cmoval", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVA32rm, "cmoval", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVA64rr, "cmovaq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVA64rm, "cmovaq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVA16rr, "cmovaw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVA16rm, "cmovaw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVBE32rr, "cmovbel", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVBE32rm, "cmovbel", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVBE64rr, "cmovbeq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVBE64rm, "cmovbeq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVBE16rr, "cmovbew", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVBE16rm, "cmovbew", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVB32rr, "cmovbl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVB32rm, "cmovbl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVB64rr, "cmovbq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVB64rm, "cmovbq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVB16rr, "cmovbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVB16rm, "cmovbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVE32rr, "cmovel", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVE32rm, "cmovel", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVE64rr, "cmoveq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVE64rm, "cmoveq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVE16rr, "cmovew", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVE16rm, "cmovew", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVGE32rr, "cmovgel", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVGE32rm, "cmovgel", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVGE64rr, "cmovgeq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVGE64rm, "cmovgeq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVGE16rr, "cmovgew", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVGE16rm, "cmovgew", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVG32rr, "cmovgl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVG32rm, "cmovgl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVG64rr, "cmovgq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVG64rm, "cmovgq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVG16rr, "cmovgw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVG16rm, "cmovgw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVLE32rr, "cmovlel", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVLE32rm, "cmovlel", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVLE64rr, "cmovleq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVLE64rm, "cmovleq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVLE16rr, "cmovlew", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVLE16rm, "cmovlew", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVL32rr, "cmovll", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVL32rm, "cmovll", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVL64rr, "cmovlq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVL64rm, "cmovlq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVL16rr, "cmovlw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVL16rm, "cmovlw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVNE32rr, "cmovnel", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVNE32rm, "cmovnel", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVNE64rr, "cmovneq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVNE64rm, "cmovneq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVNE16rr, "cmovnew", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVNE16rm, "cmovnew", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVNO32rr, "cmovnol", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVNO32rm, "cmovnol", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVNO64rr, "cmovnoq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVNO64rm, "cmovnoq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVNO16rr, "cmovnow", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVNO16rm, "cmovnow", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVNP32rr, "cmovnpl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVNP32rm, "cmovnpl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVNP64rr, "cmovnpq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVNP64rm, "cmovnpq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVNP16rr, "cmovnpw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVNP16rm, "cmovnpw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVNS32rr, "cmovnsl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVNS32rm, "cmovnsl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVNS64rr, "cmovnsq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVNS64rm, "cmovnsq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVNS16rr, "cmovnsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVNS16rm, "cmovnsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVO32rr, "cmovol", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVO32rm, "cmovol", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVO64rr, "cmovoq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVO64rm, "cmovoq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVO16rr, "cmovow", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVO16rm, "cmovow", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVP32rr, "cmovpl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVP32rm, "cmovpl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVP64rr, "cmovpq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVP64rm, "cmovpq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVP16rr, "cmovpw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVP16rm, "cmovpw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMOVS32rr, "cmovsl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMOVS32rm, "cmovsl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMOVS64rr, "cmovsq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMOVS64rm, "cmovsq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMOVS16rr, "cmovsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMOVS16rm, "cmovsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMPPDrri, "cmp", Convert__Reg1_3__Tie0__Reg1_2__Imm1_0, { MCK_Imm, MCK_pd, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPPDrmi, "cmp", Convert__Reg1_3__Tie0__Mem5_2__Imm1_0, { MCK_Imm, MCK_pd, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMPPSrri, "cmp", Convert__Reg1_3__Tie0__Reg1_2__Imm1_0, { MCK_Imm, MCK_ps, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPPSrmi, "cmp", Convert__Reg1_3__Tie0__Mem5_2__Imm1_0, { MCK_Imm, MCK_ps, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMPSDrr, "cmp", Convert__Reg1_3__Tie0__Reg1_2__Imm1_0, { MCK_Imm, MCK_sd, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPSDrm, "cmp", Convert__Reg1_3__Tie0__Mem5_2__Imm1_0, { MCK_Imm, MCK_sd, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMPSSrr, "cmp", Convert__Reg1_3__Tie0__Reg1_2__Imm1_0, { MCK_Imm, MCK_ss, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPSSrm, "cmp", Convert__Reg1_3__Tie0__Mem5_2__Imm1_0, { MCK_Imm, MCK_ss, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMP8rr, "cmpb", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::CMP8mr, "cmpb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::CMP8i8, "cmpb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::CMP8ri, "cmpb", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::CMP8mi, "cmpb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::CMP8rm, "cmpb", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::CMP32rr, "cmpl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMP32mr, "cmpl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::CMP32ri8, "cmpl", Convert__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::CMP32mi8, "cmpl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::CMP32i32, "cmpl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::CMP32ri, "cmpl", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::CMP32mi, "cmpl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::CMP32rm, "cmpl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CMPPDrri_alt, "cmppd", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPPDrmi_alt, "cmppd", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMPPSrri_alt, "cmpps", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPPSrmi_alt, "cmpps", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMP64rr, "cmpq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMP64mr, "cmpq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::CMP64ri8, "cmpq", Convert__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::CMP64mi8, "cmpq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::CMP64i32, "cmpq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::CMP64ri32, "cmpq", Convert__Reg1_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::CMP64mi32, "cmpq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::CMP64rm, "cmpq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CMPS8, "cmpsb", Convert, {  }, 0},
  { X86::CMPSDrr_alt, "cmpsd", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPSDrm_alt, "cmpsd", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMPS32, "cmpsl", Convert, {  }, 0},
  { X86::CMPS64, "cmpsq", Convert, {  }, 0},
  { X86::CMPSSrr_alt, "cmpss", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::CMPSSrm_alt, "cmpss", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::CMPS16, "cmpsw", Convert, {  }, 0},
  { X86::CMP16rr, "cmpw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMP16mr, "cmpw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::CMP16ri8, "cmpw", Convert__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::CMP16mi8, "cmpw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::CMP16i16, "cmpw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::CMP16ri, "cmpw", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::CMP16mi, "cmpw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::CMP16rm, "cmpw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::CMPXCHG16B, "cmpxchg16b", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::CMPXCHG8B, "cmpxchg8b", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::CMPXCHG8rr, "cmpxchgb", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::CMPXCHG8rm, "cmpxchgb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::CMPXCHG32rr, "cmpxchgl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CMPXCHG32rm, "cmpxchgl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::CMPXCHG64rr, "cmpxchgq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CMPXCHG64rm, "cmpxchgq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::CMPXCHG16rr, "cmpxchgw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::CMPXCHG16rm, "cmpxchgw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::COMISDrr, "comisd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::COMISDrm, "comisd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::COMISSrr, "comiss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::COMISSrm, "comiss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CPUID, "cpuid", Convert, {  }, 0},
  { X86::CQO, "cqto", Convert, {  }, 0},
  { X86::CRC32r32r8, "crc32b", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR32 }, 0},
  { X86::CRC32r64r8, "crc32b", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR64 }, 0},
  { X86::CRC32r32m8, "crc32b", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CRC32r64m8, "crc32b", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CRC32r32r32, "crc32l", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::CRC32r32m32, "crc32l", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CRC32r64r64, "crc32q", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::CRC32r64m64, "crc32q", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CRC32r32r16, "crc32w", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR32 }, 0},
  { X86::CRC32r32m16, "crc32w", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CS_PREFIX, "cs", Convert, {  }, 0},
  { X86::CVTDQ2PDrr, "cvtdq2pd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTDQ2PDrm, "cvtdq2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTDQ2PSrr, "cvtdq2ps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTDQ2PSrm, "cvtdq2ps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTPD2DQrr, "cvtpd2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTPD2DQrm, "cvtpd2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_CVTPD2PIirr, "cvtpd2pi", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR64 }, 0},
  { X86::MMX_CVTPD2PIirm, "cvtpd2pi", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::CVTPD2PSrr, "cvtpd2ps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTPD2PSrm, "cvtpd2ps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_CVTPI2PDirr, "cvtpi2pd", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_FR32 }, 0},
  { X86::MMX_CVTPI2PDirm, "cvtpi2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_CVTPI2PSirr, "cvtpi2ps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_FR32 }, 0},
  { X86::MMX_CVTPI2PSirm, "cvtpi2ps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTPS2DQrr, "cvtps2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTPS2DQrm, "cvtps2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTPS2PDrr, "cvtps2pd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTPS2PDrm, "cvtps2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_CVTPS2PIirr, "cvtps2pi", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR64 }, 0},
  { X86::MMX_CVTPS2PIirm, "cvtps2pi", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::CVTSD2SIrr, "cvtsd2sil", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::CVTSD2SIrm, "cvtsd2sil", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CVTSD2SI64rr, "cvtsd2siq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::CVTSD2SI64rm, "cvtsd2siq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CVTSD2SSrr, "cvtsd2ss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTSD2SSrm, "cvtsd2ss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTSI2SDrr, "cvtsi2sd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::CVTSI2SDrm, "cvtsi2sd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTSI2SD64rr, "cvtsi2sdq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::CVTSI2SD64rm, "cvtsi2sdq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTSI2SSrr, "cvtsi2ss", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::CVTSI2SSrm, "cvtsi2ss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTSI2SS64rr, "cvtsi2ssq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::CVTSI2SS64rm, "cvtsi2ssq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTSS2SDrr, "cvtss2sd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTSS2SDrm, "cvtss2sd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::CVTSS2SIrr, "cvtss2sil", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::CVTSS2SIrm, "cvtss2sil", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CVTSS2SI64rr, "cvtss2siq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::CVTSS2SI64rm, "cvtss2siq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CVTTPD2DQrr, "cvttpd2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTTPD2DQrm, "cvttpd2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_CVTTPD2PIirr, "cvttpd2pi", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR64 }, 0},
  { X86::MMX_CVTTPD2PIirm, "cvttpd2pi", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::CVTTPS2DQrr, "cvttps2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::CVTTPS2DQrm, "cvttps2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_CVTTPS2PIirr, "cvttps2pi", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR64 }, 0},
  { X86::MMX_CVTTPS2PIirm, "cvttps2pi", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::CVTTSD2SIrr, "cvttsd2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::CVTTSD2SIrm, "cvttsd2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CVTTSD2SI64rr, "cvttsd2siq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::CVTTSD2SI64rm, "cvttsd2siq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CVTTSS2SIrr, "cvttss2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::CVTTSS2SIrm, "cvttss2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::CVTTSS2SI64rr, "cvttss2siq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::CVTTSS2SI64rm, "cvttss2siq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::CWD, "cwtd", Convert, {  }, 0},
  { X86::CWDE, "cwtl", Convert, {  }, 0},
  { X86::DAA, "daa", Convert, {  }, Feature_In32BitMode},
  { X86::DAS, "das", Convert, {  }, Feature_In32BitMode},
  { X86::DATA16_PREFIX, "data16", Convert, {  }, 0},
  { X86::DEC8r, "decb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::DEC8m, "decb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DEC32r, "decl", Convert__Reg1_0__Tie0, { MCK_GR32 }, Feature_In32BitMode},
  { X86::DEC64_32r, "decl", Convert__Reg1_0__Tie0, { MCK_GR32 }, Feature_In64BitMode},
  { X86::DEC32m, "decl", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::DEC64_32m, "decl", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::DEC64r, "decq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::DEC64m, "decq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DEC16r, "decw", Convert__Reg1_0__Tie0, { MCK_GR16 }, Feature_In32BitMode},
  { X86::DEC64_16r, "decw", Convert__Reg1_0__Tie0, { MCK_GR16 }, Feature_In64BitMode},
  { X86::DEC16m, "decw", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::DEC64_16m, "decw", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::DIV8r, "divb", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::DIV8m, "divb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV8r, "divb", Convert__Reg1_0, { MCK_GR8, MCK_AL }, 0},
  { X86::DIV8m, "divb", Convert__Mem5_0, { MCK_Mem, MCK_AL }, 0},
  { X86::DIV32r, "divl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::DIV32m, "divl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV32r, "divl", Convert__Reg1_0, { MCK_GR32, MCK_EAX }, 0},
  { X86::DIV32m, "divl", Convert__Mem5_0, { MCK_Mem, MCK_EAX }, 0},
  { X86::DIVPDrr, "divpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::DIVPDrm, "divpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::DIVPSrr, "divps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::DIVPSrm, "divps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::DIV64r, "divq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::DIV64m, "divq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV64r, "divq", Convert__Reg1_0, { MCK_GR64, MCK_RAX }, 0},
  { X86::DIV64m, "divq", Convert__Mem5_0, { MCK_Mem, MCK_RAX }, 0},
  { X86::DIVSDrr, "divsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::DIVSDrm, "divsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::DIVSSrr, "divss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::DIVSSrm, "divss", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::DIV16r, "divw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::DIV16m, "divw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV16r, "divw", Convert__Reg1_0, { MCK_GR16, MCK_AX }, 0},
  { X86::DIV16m, "divw", Convert__Mem5_0, { MCK_Mem, MCK_AX }, 0},
  { X86::DPPDrri, "dppd", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::DPPDrmi, "dppd", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::DPPSrri, "dpps", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::DPPSrmi, "dpps", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::DS_PREFIX, "ds", Convert, {  }, 0},
  { X86::MMX_EMMS, "emms", Convert, {  }, 0},
  { X86::ENTER, "enter", Convert__Imm1_0__Imm1_1, { MCK_Imm, MCK_Imm }, 0},
  { X86::ES_PREFIX, "es", Convert, {  }, 0},
  { X86::EXTRACTPSrr, "extractps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::EXTRACTPSmr, "extractps", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::F2XM1, "f2xm1", Convert, {  }, 0},
  { X86::ABS_F, "fabs", Convert, {  }, 0},
  { X86::ADD_FST0r, "fadd", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::ADD_FST0r, "fadd", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::ADD_FrST0, "fadd", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::ADD_FST0r, "fadd", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::ADD_F64m, "faddl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ADD_FPrST0, "faddp", Convert__regST1, {  }, 0},
  { X86::ADD_FPrST0, "faddp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::ADD_FPrST0, "faddp", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::ADD_FPrST0, "faddp", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::ADD_FPrST0, "faddp", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::ADD_F32m, "fadds", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FBLDm, "fbld", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FBSTPm, "fbstp", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::CHS_F, "fchs", Convert, {  }, 0},
  { X86::CMOVB_F, "fcmovb", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVBE_F, "fcmovbe", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVE_F, "fcmove", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVNB_F, "fcmovnb", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVNBE_F, "fcmovnbe", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVNE_F, "fcmovne", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVNP_F, "fcmovnu", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::CMOVP_F, "fcmovu", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::COM_FST0r, "fcom", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::COM_FIr, "fcomi", Convert__regST1, {  }, 0},
  { X86::COM_FIr, "fcomi", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::COM_FIr, "fcomi", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::COM_FIr, "fcomi", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::FCOM64m, "fcoml", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::COMP_FST0r, "fcomp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::COM_FIPr, "fcompi", Convert__regST1, {  }, 0},
  { X86::COM_FIPr, "fcompi", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::COM_FIPr, "fcompi", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::COM_FIPr, "fcompi", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::FCOMP64m, "fcompl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FCOMPP, "fcompp", Convert, {  }, 0},
  { X86::FCOMP32m, "fcomps", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FCOM32m, "fcoms", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::COS_F, "fcos", Convert, {  }, 0},
  { X86::FDECSTP, "fdecstp", Convert, {  }, 0},
  { X86::DIV_FST0r, "fdiv", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::DIV_FST0r, "fdiv", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::DIVR_FrST0, "fdiv", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::DIV_FST0r, "fdiv", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::DIV_F64m, "fdivl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIVR_FPrST0, "fdivp", Convert__regST1, {  }, 0},
  { X86::DIVR_FPrST0, "fdivp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::DIVR_FPrST0, "fdivp", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::DIVR_FPrST0, "fdivp", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::DIVR_FPrST0, "fdivp", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::DIVR_FST0r, "fdivr", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::DIVR_FST0r, "fdivr", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::DIV_FrST0, "fdivr", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::DIVR_FST0r, "fdivr", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::DIVR_F64m, "fdivrl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV_FPrST0, "fdivrp", Convert__regST1, {  }, 0},
  { X86::DIV_FPrST0, "fdivrp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::DIV_FPrST0, "fdivrp", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::DIV_FPrST0, "fdivrp", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::DIV_FPrST0, "fdivrp", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::DIVR_F32m, "fdivrs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV_F32m, "fdivs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FEMMS, "femms", Convert, {  }, 0},
  { X86::FFREE, "ffree", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::ADD_FI32m, "fiaddl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ADD_FI16m, "fiadds", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FICOM32m, "ficoml", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FICOMP32m, "ficompl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FICOMP16m, "ficomps", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FICOM16m, "ficoms", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV_FI32m, "fidivl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIVR_FI32m, "fidivrl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIVR_FI16m, "fidivrs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::DIV_FI16m, "fidivs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ILD_F32m, "fildl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ILD_F64m, "fildll", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ILD_F16m, "filds", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MUL_FI32m, "fimull", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MUL_FI16m, "fimuls", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FINCSTP, "fincstp", Convert, {  }, 0},
  { X86::IST_F32m, "fistl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IST_FP32m, "fistpl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IST_FP64m, "fistpll", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IST_FP16m, "fistps", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IST_F16m, "fists", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ISTT_FP32m, "fisttpl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ISTT_FP64m, "fisttpll", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ISTT_FP16m, "fisttps", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUB_FI32m, "fisubl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUBR_FI32m, "fisubrl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUBR_FI16m, "fisubrs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUB_FI16m, "fisubs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LD_Frr, "fld", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::LD_F1, "fld1", Convert, {  }, 0},
  { X86::FLDCW16m, "fldcw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FLDENVm, "fldenv", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LD_F64m, "fldl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FLDL2E, "fldl2e", Convert, {  }, 0},
  { X86::FLDL2T, "fldl2t", Convert, {  }, 0},
  { X86::FLDLG2, "fldlg2", Convert, {  }, 0},
  { X86::FLDLN2, "fldln2", Convert, {  }, 0},
  { X86::FLDPI, "fldpi", Convert, {  }, 0},
  { X86::LD_F32m, "flds", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LD_F80m, "fldt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LD_F0, "fldz", Convert, {  }, 0},
  { X86::MUL_FST0r, "fmul", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::MUL_FST0r, "fmul", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::MUL_FrST0, "fmul", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::MUL_FST0r, "fmul", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::MUL_F64m, "fmull", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MUL_FPrST0, "fmulp", Convert__regST1, {  }, 0},
  { X86::MUL_FPrST0, "fmulp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::MUL_FPrST0, "fmulp", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::MUL_FPrST0, "fmulp", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::MUL_FPrST0, "fmulp", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::MUL_F32m, "fmuls", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FNCLEX, "fnclex", Convert, {  }, 0},
  { X86::FNINIT, "fninit", Convert, {  }, 0},
  { X86::FNOP, "fnop", Convert, {  }, 0},
  { X86::FSAVEm, "fnsave", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FNSTCW16m, "fnstcw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FSTENVm, "fnstenv", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FNSTSW8r, "fnstsw", Convert, {  }, 0},
  { X86::FNSTSW8r, "fnstsw", Convert, { MCK_AL }, 0},
  { X86::FNSTSW8r, "fnstsw", Convert, { MCK_AX }, 0},
  { X86::FNSTSW8r, "fnstsw", Convert, { MCK_EAX }, 0},
  { X86::FNSTSWm, "fnstsw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FPATAN, "fpatan", Convert, {  }, 0},
  { X86::FPREM, "fprem", Convert, {  }, 0},
  { X86::FPREM1, "fprem1", Convert, {  }, 0},
  { X86::FPTAN, "fptan", Convert, {  }, 0},
  { X86::FRNDINT, "frndint", Convert, {  }, 0},
  { X86::FRSTORm, "frstor", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FS_PREFIX, "fs", Convert, {  }, 0},
  { X86::FSCALE, "fscale", Convert, {  }, 0},
  { X86::SIN_F, "fsin", Convert, {  }, 0},
  { X86::FSINCOS, "fsincos", Convert, {  }, 0},
  { X86::SQRT_F, "fsqrt", Convert, {  }, 0},
  { X86::ST_Frr, "fst", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::ST_F64m, "fstl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ST_FPrr, "fstp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::ST_FP64m, "fstpl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ST_FP32m, "fstps", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ST_FP80m, "fstpt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ST_F32m, "fsts", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUB_FST0r, "fsub", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::SUB_FST0r, "fsub", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::SUBR_FrST0, "fsub", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::SUB_FST0r, "fsub", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::SUB_F64m, "fsubl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUBR_FPrST0, "fsubp", Convert__regST1, {  }, 0},
  { X86::SUBR_FPrST0, "fsubp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::SUBR_FPrST0, "fsubp", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::SUBR_FPrST0, "fsubp", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::SUBR_FPrST0, "fsubp", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::SUBR_FST0r, "fsubr", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::SUBR_FST0r, "fsubr", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::SUB_FrST0, "fsubr", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::SUBR_FST0r, "fsubr", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::SUBR_F64m, "fsubrl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUB_FPrST0, "fsubrp", Convert__regST1, {  }, 0},
  { X86::SUB_FPrST0, "fsubrp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::SUB_FPrST0, "fsubrp", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::SUB_FPrST0, "fsubrp", Convert__Reg1_1, { MCK_ST0, MCK_RST }, 0},
  { X86::SUB_FPrST0, "fsubrp", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::SUBR_F32m, "fsubrs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUB_F32m, "fsubs", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::TST_F, "ftst", Convert, {  }, 0},
  { X86::UCOM_Fr, "fucom", Convert__regST1, {  }, 0},
  { X86::UCOM_Fr, "fucom", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::UCOM_FIr, "fucomi", Convert__regST1, {  }, 0},
  { X86::UCOM_FIr, "fucomi", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::UCOM_FIr, "fucomi", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::UCOM_FIr, "fucomi", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::UCOM_FPr, "fucomp", Convert__regST1, {  }, 0},
  { X86::UCOM_FPr, "fucomp", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::UCOM_FIPr, "fucompi", Convert__regST1, {  }, 0},
  { X86::UCOM_FIPr, "fucompi", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::UCOM_FIPr, "fucompi", Convert__regST0, { MCK_ST0, MCK_ST0 }, 0},
  { X86::UCOM_FIPr, "fucompi", Convert__Reg1_0, { MCK_RST, MCK_ST0 }, 0},
  { X86::UCOM_FPPr, "fucompp", Convert, {  }, 0},
  { X86::FXAM, "fxam", Convert, {  }, 0},
  { X86::XCH_F, "fxch", Convert__regST1, {  }, 0},
  { X86::XCH_F, "fxch", Convert__Reg1_0, { MCK_RST }, 0},
  { X86::FXRSTOR, "fxrstor", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FXRSTOR64, "fxrstorq", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::FXSAVE, "fxsave", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::FXSAVE64, "fxsaveq", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::FXTRACT, "fxtract", Convert, {  }, 0},
  { X86::FYL2X, "fyl2x", Convert, {  }, 0},
  { X86::FYL2XP1, "fyl2xp1", Convert, {  }, 0},
  { X86::GS_PREFIX, "gs", Convert, {  }, 0},
  { X86::HADDPDrr, "haddpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::HADDPDrm, "haddpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::HADDPSrr, "haddps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::HADDPSrm, "haddps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::HLT, "hlt", Convert, {  }, 0},
  { X86::HSUBPDrr, "hsubpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::HSUBPDrm, "hsubpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::HSUBPSrr, "hsubps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::HSUBPSrm, "hsubps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::IDIV8r, "idivb", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::IDIV8m, "idivb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IDIV8r, "idivb", Convert__Reg1_0, { MCK_GR8, MCK_AL }, 0},
  { X86::IDIV8m, "idivb", Convert__Mem5_0, { MCK_Mem, MCK_AL }, 0},
  { X86::IDIV32r, "idivl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::IDIV32m, "idivl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IDIV32r, "idivl", Convert__Reg1_0, { MCK_GR32, MCK_EAX }, 0},
  { X86::IDIV32m, "idivl", Convert__Mem5_0, { MCK_Mem, MCK_EAX }, 0},
  { X86::IDIV64r, "idivq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::IDIV64m, "idivq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IDIV64r, "idivq", Convert__Reg1_0, { MCK_GR64, MCK_RAX }, 0},
  { X86::IDIV64m, "idivq", Convert__Mem5_0, { MCK_Mem, MCK_RAX }, 0},
  { X86::IDIV16r, "idivw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::IDIV16m, "idivw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IDIV16r, "idivw", Convert__Reg1_0, { MCK_GR16, MCK_AX }, 0},
  { X86::IDIV16m, "idivw", Convert__Mem5_0, { MCK_Mem, MCK_AX }, 0},
  { X86::IMUL8r, "imulb", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::IMUL8m, "imulb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IMUL32r, "imull", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::IMUL32m, "imull", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IMUL32rr, "imull", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::IMUL32rri8, "imull", Convert__Reg1_1__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::IMUL32rri, "imull", Convert__Reg1_1__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::IMUL32rm, "imull", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::IMUL32rri8, "imull", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_GR32 }, 0},
  { X86::IMUL32rmi8, "imull", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_GR32 }, 0},
  { X86::IMUL32rri, "imull", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32, MCK_GR32 }, 0},
  { X86::IMUL32rmi, "imull", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_GR32 }, 0},
  { X86::IMUL64r, "imulq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::IMUL64m, "imulq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IMUL64rr, "imulq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::IMUL64rri8, "imulq", Convert__Reg1_1__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::IMUL64rri32, "imulq", Convert__Reg1_1__Reg1_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::IMUL64rm, "imulq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::IMUL64rri8, "imulq", Convert__Reg1_2__Reg1_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64, MCK_GR64 }, 0},
  { X86::IMUL64rmi8, "imulq", Convert__Reg1_2__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem, MCK_GR64 }, 0},
  { X86::IMUL64rri32, "imulq", Convert__Reg1_2__Reg1_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64, MCK_GR64 }, 0},
  { X86::IMUL64rmi32, "imulq", Convert__Reg1_2__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem, MCK_GR64 }, 0},
  { X86::IMUL16r, "imulw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::IMUL16m, "imulw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::IMUL16rr, "imulw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::IMUL16rri8, "imulw", Convert__Reg1_1__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::IMUL16rri, "imulw", Convert__Reg1_1__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::IMUL16rm, "imulw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::IMUL16rri8, "imulw", Convert__Reg1_2__Reg1_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16, MCK_GR16 }, 0},
  { X86::IMUL16rmi8, "imulw", Convert__Reg1_2__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem, MCK_GR16 }, 0},
  { X86::IMUL16rri, "imulw", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16, MCK_GR16 }, 0},
  { X86::IMUL16rmi, "imulw", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_GR16 }, 0},
  { X86::IN8rr, "inb", Convert, { MCK_DX }, 0},
  { X86::IN8ri, "inb", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::IN8rr, "inb", Convert, { MCK_DX, MCK_AL }, 0},
  { X86::IN8ri, "inb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::INC8r, "incb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::INC8m, "incb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::INC32r, "incl", Convert__Reg1_0__Tie0, { MCK_GR32 }, Feature_In32BitMode},
  { X86::INC64_32r, "incl", Convert__Reg1_0__Tie0, { MCK_GR32 }, Feature_In64BitMode},
  { X86::INC32m, "incl", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::INC64_32m, "incl", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::INC64r, "incq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::INC64m, "incq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::INC16r, "incw", Convert__Reg1_0__Tie0, { MCK_GR16 }, Feature_In32BitMode},
  { X86::INC64_16r, "incw", Convert__Reg1_0__Tie0, { MCK_GR16 }, Feature_In64BitMode},
  { X86::INC16m, "incw", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::INC64_16m, "incw", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::IN32rr, "inl", Convert, { MCK_DX }, 0},
  { X86::IN32ri, "inl", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::IN32rr, "inl", Convert, { MCK_DX, MCK_EAX }, 0},
  { X86::IN32ri, "inl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::IN8, "insb", Convert, {  }, 0},
  { X86::INSERTPSrr, "insertps", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::INSERTPSrm, "insertps", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::IN32, "insl", Convert, {  }, 0},
  { X86::IN16, "insw", Convert, {  }, 0},
  { X86::INT, "int", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::INT3, "int3", Convert, {  }, 0},
  { X86::INTO, "into", Convert, {  }, 0},
  { X86::INVD, "invd", Convert, {  }, 0},
  { X86::INVEPT32, "invept", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::INVEPT64, "invept", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::INVLPG, "invlpg", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::INVVPID32, "invvpid", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::INVVPID64, "invvpid", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::IN16rr, "inw", Convert, { MCK_DX }, 0},
  { X86::IN16ri, "inw", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::IN16rr, "inw", Convert, { MCK_DX, MCK_AX }, 0},
  { X86::IN16ri, "inw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::IRET32, "iretl", Convert, {  }, 0},
  { X86::IRET64, "iretq", Convert, {  }, Feature_In64BitMode},
  { X86::IRET16, "iretw", Convert, {  }, 0},
  { X86::JA_1, "ja", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JA_4, "ja", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JAE_1, "jae", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JAE_4, "jae", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JB_1, "jb", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JB_4, "jb", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JBE_1, "jbe", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JBE_4, "jbe", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JCXZ, "jcxz", Convert__AbsMem1_0, { MCK_AbsMem }, Feature_In32BitMode},
  { X86::JE_1, "je", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JE_4, "je", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JECXZ_32, "jecxz", Convert__AbsMem1_0, { MCK_AbsMem }, Feature_In32BitMode},
  { X86::JECXZ_64, "jecxz", Convert__AbsMem1_0, { MCK_AbsMem }, Feature_In64BitMode},
  { X86::JG_1, "jg", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JG_4, "jg", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JGE_1, "jge", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JGE_4, "jge", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JL_1, "jl", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JL_4, "jl", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JLE_1, "jle", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JLE_4, "jle", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JMP_1, "jmp", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JMP_4, "jmp", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::FARJMP32i, "jmp", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::JMP32r, "jmpl", Convert__Reg1_1, { MCK__STAR_, MCK_GR32 }, Feature_In32BitMode},
  { X86::JMP32m, "jmpl", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, Feature_In32BitMode},
  { X86::FARJMP32i, "jmpl", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::JMP64pcrel32, "jmpq", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JMP64r, "jmpq", Convert__Reg1_1, { MCK__STAR_, MCK_GR64 }, Feature_In64BitMode},
  { X86::JMP64m, "jmpq", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, Feature_In64BitMode},
  { X86::FARJMP16i, "jmpw", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::JNE_1, "jne", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNE_4, "jne", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNO_1, "jno", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNO_4, "jno", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNP_1, "jnp", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNP_4, "jnp", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNS_1, "jns", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JNS_4, "jns", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JO_1, "jo", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JO_4, "jo", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JP_1, "jp", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JP_4, "jp", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JRCXZ, "jrcxz", Convert__AbsMem1_0, { MCK_AbsMem }, Feature_In64BitMode},
  { X86::JS_1, "js", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::JS_4, "js", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::LAHF, "lahf", Convert, {  }, 0},
  { X86::LAR32rr, "larl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::LAR32rm, "larl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LAR64rr, "larq", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR64 }, 0},
  { X86::LAR64rm, "larq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LAR16rr, "larw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::LAR16rm, "larw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::FARCALL32m, "lcall", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARCALL32i, "lcall", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::FARCALL32m, "lcalll", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARCALL32i, "lcalll", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::FARCALL64, "lcallq", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARCALL16m, "lcallw", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARCALL16i, "lcallw", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::LDDQUrm, "lddqu", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::LDMXCSR, "ldmxcsr", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LDS32rm, "ldsl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LDS16rm, "ldsw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LEA32r, "leal", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, Feature_In32BitMode},
  { X86::LEA64_32r, "leal", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, Feature_In64BitMode},
  { X86::LEA64r, "leaq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LEAVE, "leave", Convert, {  }, Feature_In32BitMode},
  { X86::LEAVE64, "leave", Convert, {  }, Feature_In64BitMode},
  { X86::LEA16r, "leaw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LES32rm, "lesl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LES16rm, "lesw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LFENCE, "lfence", Convert, {  }, 0},
  { X86::LFS32rm, "lfsl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LFS64rm, "lfsq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LFS16rm, "lfsw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LGDTm, "lgdt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LGDT16m, "lgdtw", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::LGS32rm, "lgsl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LGS64rm, "lgsq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LGS16rm, "lgsw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LIDTm, "lidt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LIDT16m, "lidtw", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::FARJMP32m, "ljmp", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARJMP32i, "ljmp", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::FARJMP32m, "ljmpl", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARJMP32i, "ljmpl", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::FARJMP64, "ljmpq", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARJMP16m, "ljmpw", Convert__Mem5_1, { MCK__STAR_, MCK_Mem }, 0},
  { X86::FARJMP16i, "ljmpw", Convert__Imm1_1__Imm1_0, { MCK_Imm, MCK_Imm }, 0},
  { X86::LLDT16r, "lldtw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::LLDT16m, "lldtw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LMSW16r, "lmsww", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::LMSW16m, "lmsww", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LOCK_PREFIX, "lock", Convert, {  }, 0},
  { X86::LODSB, "lodsb", Convert, {  }, 0},
  { X86::LODSD, "lodsl", Convert, {  }, 0},
  { X86::LODSQ, "lodsq", Convert, {  }, 0},
  { X86::LODSW, "lodsw", Convert, {  }, 0},
  { X86::LOOP, "loop", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::LOOPE, "loope", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::LOOPNE, "loopne", Convert__AbsMem1_0, { MCK_AbsMem }, 0},
  { X86::LRETI, "lret", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::LRETL, "lretl", Convert, {  }, 0},
  { X86::LRETQ, "lretq", Convert, {  }, 0},
  { X86::LRETIW, "lretw", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::LSL32rr, "lsll", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::LSL32rm, "lsll", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LSL64rr, "lslq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::LSL64rm, "lslq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LSL16rr, "lslw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::LSL16rm, "lslw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LSS32rm, "lssl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LSS64rm, "lssq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LSS16rm, "lssw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::LTRr, "ltrw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::LTRm, "ltrw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::LZCNT32rr, "lzcntl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::LZCNT32rm, "lzcntl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::LZCNT64rr, "lzcntq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::LZCNT64rm, "lzcntq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::LZCNT16rr, "lzcntw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::LZCNT16rm, "lzcntw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MASKMOVDQU, "maskmovdqu", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MASKMOVDQU64, "maskmovdqu", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_MASKMOVQ, "maskmovq", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::MMX_MASKMOVQ64, "maskmovq", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, Feature_In64BitMode},
  { X86::MAXPDrr, "maxpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MAXPDrm, "maxpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MAXPSrr, "maxps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MAXPSrm, "maxps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MAXSDrr, "maxsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MAXSDrm, "maxsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MAXSSrr, "maxss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MAXSSrm, "maxss", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MFENCE, "mfence", Convert, {  }, 0},
  { X86::MINPDrr, "minpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MINPDrm, "minpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MINPSrr, "minps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MINPSrm, "minps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MINSDrr, "minsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MINSDrm, "minsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MINSSrr, "minss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MINSSrm, "minss", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MONITORrrr, "monitor", Convert, {  }, 0},
  { X86::MONITORrrr, "monitor", Convert, { MCK_EAX, MCK_ECX, MCK_EDX }, Feature_In32BitMode},
  { X86::MONITORrrr, "monitor", Convert, { MCK_RAX, MCK_RCX, MCK_RDX }, Feature_In64BitMode},
  { X86::MONTMUL, "montmul", Convert, {  }, 0},
  { X86::MOV32ms, "mov", Convert__Mem5_1__Reg1_0, { MCK_SEGMENT_REG, MCK_Mem }, 0},
  { X86::MOV32sm, "mov", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_SEGMENT_REG }, 0},
  { X86::MOV64ri, "movabsq", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::FsMOVAPDrr, "movapd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVAPDrr, "movapd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVAPDmr, "movapd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::FsMOVAPDrm, "movapd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVAPDrm, "movapd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsMOVAPSrr, "movaps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVAPSrr, "movaps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVAPSmr, "movaps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::FsMOVAPSrm, "movaps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVAPSrm, "movaps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOV8ao8, "movb", Convert__AbsMem1_1, { MCK_AL, MCK_AbsMem }, Feature_In32BitMode},
  { X86::MOV8rr, "movb", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::MOV8mr, "movb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::MOV8ri, "movb", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::MOV8mi, "movb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::MOV8o8a, "movb", Convert__AbsMem1_0, { MCK_AbsMem, MCK_AL }, Feature_In32BitMode},
  { X86::MOV8rm, "movb", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::MOVBE32mr, "movbel", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::MOVBE32rm, "movbel", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::MOVBE64mr, "movbeq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::MOVBE64rm, "movbeq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MOVBE16mr, "movbew", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::MOVBE16rm, "movbew", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MMX_MOVD64rr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_VR64 }, 0},
  { X86::MMX_MOVZDI2PDIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_VR64 }, 0},
  { X86::MOVDI2PDIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::MOVDI2SSrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::MOVZDI2PDIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::MMX_MOVD64rrv164, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_VR64 }, 0},
  { X86::MMX_MOVD64to64rr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_VR64 }, 0},
  { X86::MOV64toPQIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::MOV64toSDrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::MOVZQI2PQIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::MMX_MOVD64grr, "movd", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_GR32 }, 0},
  { X86::MMX_MOVD64from64rr, "movd", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_GR64 }, 0},
  { X86::MMX_MOVD64mr, "movd", Convert__Mem5_1__Reg1_0, { MCK_VR64, MCK_Mem }, 0},
  { X86::MOVPDI2DIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::MOVSS2DIrr, "movd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::MOVPQIto64rr, "movd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::MOVSDto64rr, "movd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVPQIto64rr, "movd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, Feature_In64BitMode},
  { X86::MOVPDI2DImr, "movd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVSS2DImr, "movd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MMX_MOVD64rm, "movd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::MMX_MOVZDI2PDIrm, "movd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::MOVDI2PDIrm, "movd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVDI2SSrm, "movd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVZDI2PDIrm, "movd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVDDUPrr, "movddup", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVDDUPrm, "movddup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_MOVDQ2Qrr, "movdq2q", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR64 }, 0},
  { X86::MMX_MOVFR642Qrr, "movdq2q", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR64 }, 0},
  { X86::MOVDQArr, "movdqa", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVDQAmr, "movdqa", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVDQArm, "movdqa", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVDQUrr, "movdqu", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVDQUmr, "movdqu", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVDQUrm, "movdqu", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVHLPSrr, "movhlps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVHPDmr, "movhpd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVHPDrm, "movhpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVHPSmr, "movhps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVHPSrm, "movhps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOV32ao32, "movl", Convert__AbsMem1_1, { MCK_EAX, MCK_AbsMem }, Feature_In32BitMode},
  { X86::MOV32rr, "movl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::MOV32sr, "movl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_SEGMENT_REG }, 0},
  { X86::MOV32dr, "movl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_DEBUG_REG }, 0},
  { X86::MOV32cr, "movl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_CONTROL_REG }, 0},
  { X86::MOV32mr, "movl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::MOV32rs, "movl", Convert__Reg1_1__Reg1_0, { MCK_SEGMENT_REG, MCK_GR32 }, 0},
  { X86::MOV32ms, "movl", Convert__Mem5_1__Reg1_0, { MCK_SEGMENT_REG, MCK_Mem }, 0},
  { X86::MOV32rd, "movl", Convert__Reg1_1__Reg1_0, { MCK_DEBUG_REG, MCK_GR32 }, 0},
  { X86::MOV32rc, "movl", Convert__Reg1_1__Reg1_0, { MCK_CONTROL_REG, MCK_GR32 }, 0},
  { X86::MOV32ri, "movl", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::MOV32mi, "movl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::MOV32o32a, "movl", Convert__AbsMem1_0, { MCK_AbsMem, MCK_EAX }, Feature_In32BitMode},
  { X86::MOV32rm, "movl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::MOV32sm, "movl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_SEGMENT_REG }, 0},
  { X86::MOVLHPSrr, "movlhps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVLPDmr, "movlpd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVLPDrm, "movlpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVLPSmr, "movlps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVLPSrm, "movlps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVMSKPDrr32, "movmskpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::MOVMSKPDrr64, "movmskpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::MOVMSKPSrr32, "movmskps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::MOVMSKPSrr64, "movmskps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::MOVNTDQ_64mr, "movntdq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVNTDQmr, "movntdq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVNTDQArm, "movntdqa", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVNTImr, "movntil", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::MOVNTI_64mr, "movntiq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::MOVNTPDmr, "movntpd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVNTPSmr, "movntps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MMX_MOVNTQmr, "movntq", Convert__Mem5_1__Reg1_0, { MCK_VR64, MCK_Mem }, 0},
  { X86::MOV64rr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::MMX_MOVD64to64rr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_VR64 }, 0},
  { X86::MOV64toPQIrr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::MOV64toSDrr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::VMOVZQI2PQIrr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::MOVZQI2PQIrr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::MOV64sr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_SEGMENT_REG }, 0},
  { X86::MOV64dr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_DEBUG_REG }, 0},
  { X86::MOV64cr, "movq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_CONTROL_REG }, 0},
  { X86::MOV64mr, "movq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::MMX_MOVD64from64rr, "movq", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_GR64 }, 0},
  { X86::MMX_MOVQ64rr, "movq", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::MMX_MOVQ64mr, "movq", Convert__Mem5_1__Reg1_0, { MCK_VR64, MCK_Mem }, 0},
  { X86::MOVPQIto64rr, "movq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::MOVSDto64rr, "movq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::MOVQxrxr, "movq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVZPQILo2PQIrr, "movq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVLQ128mr, "movq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVPQI2QImr, "movq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVSDto64mr, "movq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOV64rs, "movq", Convert__Reg1_1__Reg1_0, { MCK_SEGMENT_REG, MCK_GR64 }, 0},
  { X86::MOV64ms, "movq", Convert__Mem5_1__Reg1_0, { MCK_SEGMENT_REG, MCK_Mem }, 0},
  { X86::MOV64rd, "movq", Convert__Reg1_1__Reg1_0, { MCK_DEBUG_REG, MCK_GR64 }, 0},
  { X86::MOV64rc, "movq", Convert__Reg1_1__Reg1_0, { MCK_CONTROL_REG, MCK_GR64 }, 0},
  { X86::MOV64ri32, "movq", Convert__Reg1_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::MOV64mi32, "movq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::MOV64ri, "movq", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::MOV64rm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MMX_MOVQ64rm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::MOV64toSDrm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVQI2PQIrm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVZPQILo2PQIrm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVZQI2PQIrm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOV64sm, "movq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_SEGMENT_REG }, 0},
  { X86::MMX_MOVQ2DQrr, "movq2dq", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_FR32 }, 0},
  { X86::MMX_MOVQ2FR64rr, "movq2dq", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_FR32 }, 0},
  { X86::MOVSB, "movsb", Convert, {  }, 0},
  { X86::MOVSX32rr8, "movsbl", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR32 }, 0},
  { X86::MOVSX32rm8, "movsbl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::MOVSX64rr8, "movsbq", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR64 }, 0},
  { X86::MOVSX64rm8, "movsbq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MOVSX16rr8, "movsbw", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR16 }, 0},
  { X86::MOVSX16rm8, "movsbw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MOVSD, "movsd", Convert, {  }, 0},
  { X86::MOVSDrr, "movsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVSDmr, "movsd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVSDrm, "movsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVSHDUPrr, "movshdup", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVSHDUPrm, "movshdup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVSD, "movsl", Convert, {  }, 0},
  { X86::MOVSLDUPrr, "movsldup", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVSLDUPrm, "movsldup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVSX64rr32, "movslq", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR64 }, 0},
  { X86::MOVSX64rm32, "movslq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MOVSQ, "movsq", Convert, {  }, 0},
  { X86::MOVSSrr, "movss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVSSmr, "movss", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVSSrm, "movss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVSW, "movsw", Convert, {  }, 0},
  { X86::MOVSX32rr16, "movswl", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR32 }, 0},
  { X86::MOVSX32rm16, "movswl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::MOVSX64rr16, "movswq", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR64 }, 0},
  { X86::MOVSX64rm16, "movswq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MOVSX16rr8, "movsx", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR16 }, 0},
  { X86::MOVSX32rr8, "movsx", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR32 }, 0},
  { X86::MOVSX64rr8, "movsx", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR64 }, 0},
  { X86::MOVSX32rr16, "movsx", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR32 }, 0},
  { X86::MOVSX64rr16, "movsx", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR64 }, 0},
  { X86::MOVSX64rr32, "movsx", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR64 }, 0},
  { X86::MOVSX16rm8, "movsx", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MOVUPDrr, "movupd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVUPDmr, "movupd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVUPDrm, "movupd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOVUPSrr, "movups", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MOVUPSmr, "movups", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::MOVUPSrm, "movups", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MOV16ao16, "movw", Convert__AbsMem1_1, { MCK_AX, MCK_AbsMem }, Feature_In32BitMode},
  { X86::MOV16rr, "movw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::MOV16sr, "movw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_SEGMENT_REG }, 0},
  { X86::MOV16mr, "movw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::MOV16rs, "movw", Convert__Reg1_1__Reg1_0, { MCK_SEGMENT_REG, MCK_GR16 }, 0},
  { X86::MOV16ms, "movw", Convert__Mem5_1__Reg1_0, { MCK_SEGMENT_REG, MCK_Mem }, 0},
  { X86::MOV16ri, "movw", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::MOV16mi, "movw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::MOV16o16a, "movw", Convert__AbsMem1_0, { MCK_AbsMem, MCK_AX }, Feature_In32BitMode},
  { X86::MOV16rm, "movw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MOV16sm, "movw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_SEGMENT_REG }, 0},
  { X86::MOVZX32_NOREXrr8, "movzbl", Convert__Reg1_1__Reg1_0, { MCK_GR8_NOREX, MCK_GR32_NOREX }, 0},
  { X86::MOVZX32rr8, "movzbl", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR32 }, 0},
  { X86::MOVZX32_NOREXrm8, "movzbl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32_NOREX }, 0},
  { X86::MOVZX32rm8, "movzbl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::MOVZX64rr8_Q, "movzbq", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR64 }, 0},
  { X86::MOVZX64rm8_Q, "movzbq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MOVZX16rr8, "movzbw", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR16 }, 0},
  { X86::MOVZX16rm8, "movzbw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MOVZX32rr16, "movzwl", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR32 }, 0},
  { X86::MOVZX32rm16, "movzwl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::MOVZX64rr16_Q, "movzwq", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR64 }, 0},
  { X86::MOVZX64rm16_Q, "movzwq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::MOVZX16rr8, "movzx", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR16 }, 0},
  { X86::MOVZX32rr8, "movzx", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR32 }, 0},
  { X86::MOVZX64rr8_Q, "movzx", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR64 }, 0},
  { X86::MOVZX32rr16, "movzx", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR32 }, 0},
  { X86::MOVZX64rr16_Q, "movzx", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR64 }, 0},
  { X86::MOVZX16rm8, "movzx", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::MPSADBWrri, "mpsadbw", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::MPSADBWrmi, "mpsadbw", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::MUL8r, "mulb", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::MUL8m, "mulb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MUL32r, "mull", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::MUL32m, "mull", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MULPDrr, "mulpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MULPDrm, "mulpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MULPSrr, "mulps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MULPSrm, "mulps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MUL64r, "mulq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::MUL64m, "mulq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MULSDrr, "mulsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MULSDrm, "mulsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MULSSrr, "mulss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MULSSrm, "mulss", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MUL16r, "mulw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::MUL16m, "mulw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MWAITrr, "mwait", Convert, {  }, 0},
  { X86::MWAITrr, "mwait", Convert, { MCK_EAX, MCK_ECX }, Feature_In32BitMode},
  { X86::MWAITrr, "mwait", Convert, { MCK_RAX, MCK_RCX }, Feature_In64BitMode},
  { X86::NEG8r, "negb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::NEG8m, "negb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NEG32r, "negl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::NEG32m, "negl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NEG64r, "negq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::NEG64m, "negq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NEG16r, "negw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::NEG16m, "negw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NOOP, "nop", Convert, {  }, 0},
  { X86::NOOPL, "nopl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NOOPW, "nopw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NOT8r, "notb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::NOT8m, "notb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NOT32r, "notl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::NOT32m, "notl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NOT64r, "notq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::NOT64m, "notq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::NOT16r, "notw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::NOT16m, "notw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::OR8rr, "orb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::OR8mr, "orb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::OR8i8, "orb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::OR8ri, "orb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::OR8mi, "orb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::OR8rm, "orb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::OR32rr, "orl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::OR32mr, "orl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::OR32ri8, "orl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::OR32mi8, "orl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::OR32i32, "orl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::OR32ri, "orl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::OR32mi, "orl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::OR32rm, "orl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::FsORPDrr, "orpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ORPDrr, "orpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsORPDrm, "orpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ORPDrm, "orpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsORPSrr, "orps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::ORPSrr, "orps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsORPSrm, "orps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::ORPSrm, "orps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::OR64rr, "orq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::OR64mr, "orq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::OR64ri8, "orq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::OR64mi8, "orq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::OR64i32, "orq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::OR64ri32, "orq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::OR64mi32, "orq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::OR64rm, "orq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::OR16rr, "orw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::OR16mr, "orw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::OR16ri8, "orw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::OR16mi8, "orw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::OR16i16, "orw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::OR16ri, "orw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::OR16mi, "orw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::OR16rm, "orw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::OUT8rr, "outb", Convert, { MCK_DX }, 0},
  { X86::OUT8ir, "outb", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::OUT8rr, "outb", Convert, { MCK_AL, MCK_DX }, 0},
  { X86::OUT8ir, "outb", Convert__Imm1_1, { MCK_AL, MCK_Imm }, 0},
  { X86::OUT32rr, "outl", Convert, { MCK_DX }, 0},
  { X86::OUT32ir, "outl", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::OUT32rr, "outl", Convert, { MCK_EAX, MCK_DX }, 0},
  { X86::OUT32ir, "outl", Convert__Imm1_1, { MCK_EAX, MCK_Imm }, 0},
  { X86::OUTSB, "outsb", Convert, {  }, 0},
  { X86::OUTSD, "outsl", Convert, {  }, 0},
  { X86::OUTSW, "outsw", Convert, {  }, 0},
  { X86::OUT16rr, "outw", Convert, { MCK_DX }, 0},
  { X86::OUT16ir, "outw", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::OUT16rr, "outw", Convert, { MCK_AX, MCK_DX }, 0},
  { X86::OUT16ir, "outw", Convert__Imm1_1, { MCK_AX, MCK_Imm }, 0},
  { X86::MMX_PABSBrr64, "pabsb", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PABSBrr128, "pabsb", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PABSBrm64, "pabsb", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PABSBrm128, "pabsb", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PABSDrr64, "pabsd", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PABSDrr128, "pabsd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PABSDrm64, "pabsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PABSDrm128, "pabsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PABSWrr64, "pabsw", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PABSWrr128, "pabsw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PABSWrm64, "pabsw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PABSWrm128, "pabsw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PACKSSDWirr, "packssdw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PACKSSDWrr, "packssdw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PACKSSDWirm, "packssdw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PACKSSDWrm, "packssdw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PACKSSWBirr, "packsswb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PACKSSWBrr, "packsswb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PACKSSWBirm, "packsswb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PACKSSWBrm, "packsswb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PACKUSDWrr, "packusdw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PACKUSDWrm, "packusdw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PACKUSWBirr, "packuswb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PACKUSWBrr, "packuswb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PACKUSWBirm, "packuswb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PACKUSWBrm, "packuswb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDBirr, "paddb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDBrr, "paddb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDBirm, "paddb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDBrm, "paddb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDDirr, "paddd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDDrr, "paddd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDDirm, "paddd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDDrm, "paddd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDQirr, "paddq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDQrr, "paddq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDQirm, "paddq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDQrm, "paddq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDSBirr, "paddsb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDSBrr, "paddsb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDSBirm, "paddsb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDSBrm, "paddsb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDSWirr, "paddsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDSWrr, "paddsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDSWirm, "paddsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDSWrm, "paddsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDUSBirr, "paddusb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDUSBrr, "paddusb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDUSBirm, "paddusb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDUSBrm, "paddusb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDUSWirr, "paddusw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDUSWrr, "paddusw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDUSWirm, "paddusw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDUSWrm, "paddusw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PADDWirr, "paddw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PADDWrr, "paddw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PADDWirm, "paddw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PADDWrm, "paddw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PALIGNR64irr, "palignr", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR64, MCK_VR64 }, 0},
  { X86::PALIGNR128rr, "palignr", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PALIGNR64irm, "palignr", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR64 }, 0},
  { X86::PALIGNR128rm, "palignr", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PANDirr, "pand", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PANDrr, "pand", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PANDirm, "pand", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PANDrm, "pand", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PANDNirr, "pandn", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PANDNrr, "pandn", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PANDNirm, "pandn", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PANDNrm, "pandn", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PAUSE, "pause", Convert, {  }, 0},
  { X86::MMX_PAVGBirr, "pavgb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PAVGBrr, "pavgb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PAVGBirm, "pavgb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PAVGBrm, "pavgb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PAVGUSBrr, "pavgusb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PAVGUSBrm, "pavgusb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::MMX_PAVGWirr, "pavgw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PAVGWrr, "pavgw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PAVGWirm, "pavgw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PAVGWrm, "pavgw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PBLENDVBrr0, "pblendvb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PBLENDVBrm0, "pblendvb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PBLENDWrri, "pblendw", Convert__Reg1_2__Tie0__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32 }, 0},
  { X86::PBLENDWrmi, "pblendw", Convert__Reg1_2__Tie0__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCLMULQDQrr, "pclmulhqhqdq", Convert__Reg1_1__Tie0__Reg1_0__imm17, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PCLMULQDQrm, "pclmulhqhqdq", Convert__Reg1_1__Tie0__Mem5_0__imm17, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCLMULQDQrr, "pclmulhqlqdq", Convert__Reg1_1__Tie0__Reg1_0__imm1, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PCLMULQDQrm, "pclmulhqlqdq", Convert__Reg1_1__Tie0__Mem5_0__imm1, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCLMULQDQrr, "pclmullqhqdq", Convert__Reg1_1__Tie0__Reg1_0__imm16, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PCLMULQDQrm, "pclmullqhqdq", Convert__Reg1_1__Tie0__Mem5_0__imm16, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCLMULQDQrr, "pclmullqlqdq", Convert__Reg1_1__Tie0__Reg1_0__imm0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PCLMULQDQrm, "pclmullqlqdq", Convert__Reg1_1__Tie0__Mem5_0__imm0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCLMULQDQrr, "pclmulqdq", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCLMULQDQrm, "pclmulqdq", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PCMPEQBirr, "pcmpeqb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PCMPEQBrr, "pcmpeqb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PCMPEQBirm, "pcmpeqb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PCMPEQBrm, "pcmpeqb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PCMPEQDirr, "pcmpeqd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PCMPEQDrr, "pcmpeqd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PCMPEQDirm, "pcmpeqd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PCMPEQDrm, "pcmpeqd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPEQQrr, "pcmpeqq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPEQQrm, "pcmpeqq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PCMPEQWirr, "pcmpeqw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PCMPEQWrr, "pcmpeqw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PCMPEQWirm, "pcmpeqw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PCMPEQWrm, "pcmpeqw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRIArr, "pcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRICrr, "pcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRIOrr, "pcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRISrr, "pcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRIZrr, "pcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRIrr, "pcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRIArm, "pcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRICrm, "pcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRIOrm, "pcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRISrm, "pcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRIZrm, "pcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRIrm, "pcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPESTRM128rr, "pcmpestrm", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPESTRM128rm, "pcmpestrm", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PCMPGTBirr, "pcmpgtb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PCMPGTBrr, "pcmpgtb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PCMPGTBirm, "pcmpgtb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PCMPGTBrm, "pcmpgtb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PCMPGTDirr, "pcmpgtd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PCMPGTDrr, "pcmpgtd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PCMPGTDirm, "pcmpgtd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PCMPGTDrm, "pcmpgtd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPGTQrr, "pcmpgtq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPGTQrm, "pcmpgtq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PCMPGTWirr, "pcmpgtw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PCMPGTWrr, "pcmpgtw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PCMPGTWirm, "pcmpgtw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PCMPGTWrm, "pcmpgtw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRIArr, "pcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRICrr, "pcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRIOrr, "pcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRISrr, "pcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRIZrr, "pcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRIrr, "pcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRIArm, "pcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRICrm, "pcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRIOrm, "pcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRISrm, "pcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRIZrm, "pcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRIrm, "pcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PCMPISTRM128rr, "pcmpistrm", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PCMPISTRM128rm, "pcmpistrm", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PEXTRBrr, "pextrb", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::PEXTRBmr, "pextrb", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::PEXTRDrr, "pextrd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::PEXTRDmr, "pextrd", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::PEXTRQrr, "pextrq", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR64 }, 0},
  { X86::PEXTRQmr, "pextrq", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::MMX_PEXTRWirri, "pextrw", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64, MCK_GR32 }, 0},
  { X86::PEXTRWri, "pextrw", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::PEXTRWmr, "pextrw", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::PF2IDrr, "pf2id", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PF2IDrm, "pf2id", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PF2IWrr, "pf2iw", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PF2IWrm, "pf2iw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFACCrr, "pfacc", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFACCrm, "pfacc", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFADDrr, "pfadd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFADDrm, "pfadd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFCMPEQrr, "pfcmpeq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFCMPEQrm, "pfcmpeq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFCMPGErr, "pfcmpge", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFCMPGErm, "pfcmpge", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFCMPGTrr, "pfcmpgt", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFCMPGTrm, "pfcmpgt", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFMAXrr, "pfmax", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFMAXrm, "pfmax", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFMINrr, "pfmin", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFMINrm, "pfmin", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFMULrr, "pfmul", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFMULrm, "pfmul", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFNACCrr, "pfnacc", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFNACCrm, "pfnacc", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFPNACCrr, "pfpnacc", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFPNACCrm, "pfpnacc", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFRCPrr, "pfrcp", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFRCPrm, "pfrcp", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFRCPIT1rr, "pfrcpit1", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFRCPIT1rm, "pfrcpit1", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFRCPIT2rr, "pfrcpit2", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFRCPIT2rm, "pfrcpit2", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFRSQIT1rr, "pfrsqit1", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFRSQIT1rm, "pfrsqit1", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFRSQRTrr, "pfrsqrt", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFRSQRTrm, "pfrsqrt", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFSUBrr, "pfsub", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFSUBrm, "pfsub", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PFSUBRrr, "pfsubr", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PFSUBRrm, "pfsubr", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::MMX_PHADDrr64, "phaddd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PHADDDrr128, "phaddd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PHADDrm64, "phaddd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PHADDDrm128, "phaddd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PHADDSWrr64, "phaddsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PHADDSWrr128, "phaddsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PHADDSWrm64, "phaddsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PHADDSWrm128, "phaddsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PHADDWrr64, "phaddw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PHADDWrr128, "phaddw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PHADDWrm64, "phaddw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PHADDWrm128, "phaddw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PHMINPOSUWrr128, "phminposuw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PHMINPOSUWrm128, "phminposuw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PHSUBDrr64, "phsubd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PHSUBDrr128, "phsubd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PHSUBDrm64, "phsubd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PHSUBDrm128, "phsubd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PHSUBSWrr64, "phsubsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PHSUBSWrr128, "phsubsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PHSUBSWrm64, "phsubsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PHSUBSWrm128, "phsubsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PHSUBWrr64, "phsubw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PHSUBWrr128, "phsubw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PHSUBWrm64, "phsubw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PHSUBWrm128, "phsubw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PI2FDrr, "pi2fd", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PI2FDrm, "pi2fd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PI2FWrr, "pi2fw", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PI2FWrm, "pi2fw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PINSRBrr, "pinsrb", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_FR32 }, 0},
  { X86::PINSRBrm, "pinsrb", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::PINSRDrr, "pinsrd", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_FR32 }, 0},
  { X86::PINSRDrm, "pinsrd", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::PINSRQrr, "pinsrq", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR64, MCK_FR32 }, 0},
  { X86::PINSRQrm, "pinsrq", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PINSRWirri, "pinsrw", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_VR64 }, 0},
  { X86::PINSRWrri, "pinsrw", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_FR32 }, 0},
  { X86::MMX_PINSRWirmi, "pinsrw", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_VR64 }, 0},
  { X86::PINSRWrmi, "pinsrw", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMADDUBSWrr64, "pmaddubsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMADDUBSWrr128, "pmaddubsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMADDUBSWrm64, "pmaddubsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMADDUBSWrm128, "pmaddubsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMADDWDirr, "pmaddwd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMADDWDrr, "pmaddwd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMADDWDirm, "pmaddwd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMADDWDrm, "pmaddwd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMAXSBrr, "pmaxsb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMAXSBrm, "pmaxsb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMAXSDrr, "pmaxsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMAXSDrm, "pmaxsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMAXSWirr, "pmaxsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMAXSWrr, "pmaxsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMAXSWirm, "pmaxsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMAXSWrm, "pmaxsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMAXUBirr, "pmaxub", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMAXUBrr, "pmaxub", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMAXUBirm, "pmaxub", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMAXUBrm, "pmaxub", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMAXUDrr, "pmaxud", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMAXUDrm, "pmaxud", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMAXUWrr, "pmaxuw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMAXUWrm, "pmaxuw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMINSBrr, "pminsb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMINSBrm, "pminsb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMINSDrr, "pminsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMINSDrm, "pminsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMINSWirr, "pminsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMINSWrr, "pminsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMINSWirm, "pminsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMINSWrm, "pminsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMINUBirr, "pminub", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMINUBrr, "pminub", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMINUBirm, "pminub", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMINUBrm, "pminub", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMINUDrr, "pminud", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMINUDrm, "pminud", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMINUWrr, "pminuw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMINUWrm, "pminuw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMOVMSKBrr, "pmovmskb", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_GR32 }, 0},
  { X86::PMOVMSKBrr, "pmovmskb", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::PMOVSXBDrr, "pmovsxbd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVSXBDrm, "pmovsxbd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVSXBQrr, "pmovsxbq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVSXBQrm, "pmovsxbq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVSXBWrr, "pmovsxbw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVSXBWrm, "pmovsxbw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVSXDQrr, "pmovsxdq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVSXDQrm, "pmovsxdq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVSXWDrr, "pmovsxwd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVSXWDrm, "pmovsxwd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVSXWQrr, "pmovsxwq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVSXWQrm, "pmovsxwq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVZXBDrr, "pmovzxbd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVZXBDrm, "pmovzxbd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVZXBQrr, "pmovzxbq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVZXBQrm, "pmovzxbq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVZXBWrr, "pmovzxbw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVZXBWrm, "pmovzxbw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVZXDQrr, "pmovzxdq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVZXDQrm, "pmovzxdq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVZXWDrr, "pmovzxwd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVZXWDrm, "pmovzxwd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMOVZXWQrr, "pmovzxwq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMOVZXWQrm, "pmovzxwq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMULDQrr, "pmuldq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMULDQrm, "pmuldq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMULHRSWrr64, "pmulhrsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMULHRSWrr128, "pmulhrsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMULHRSWrm64, "pmulhrsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMULHRSWrm128, "pmulhrsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMULHRWrr, "pmulhrw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMULHRWrm, "pmulhrw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::MMX_PMULHUWirr, "pmulhuw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMULHUWrr, "pmulhuw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMULHUWirm, "pmulhuw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMULHUWrm, "pmulhuw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMULHWirr, "pmulhw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMULHWrr, "pmulhw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMULHWirm, "pmulhw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMULHWrm, "pmulhw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PMULLDrr, "pmulld", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PMULLDrm, "pmulld", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMULLWirr, "pmullw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMULLWrr, "pmullw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMULLWirm, "pmullw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMULLWrm, "pmullw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PMULUDQirr, "pmuludq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PMULUDQrr, "pmuludq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PMULUDQirm, "pmuludq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PMULUDQrm, "pmuludq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::POPA32, "popal", Convert, {  }, Feature_In32BitMode},
  { X86::POPCNT32rr, "popcntl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::POPCNT32rm, "popcntl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::POPCNT64rr, "popcntq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::POPCNT64rm, "popcntq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::POPCNT16rr, "popcntw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::POPCNT16rm, "popcntw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::POPF32, "popfl", Convert, {  }, Feature_In32BitMode},
  { X86::POPF64, "popfq", Convert, {  }, Feature_In64BitMode},
  { X86::POPF16, "popfw", Convert, {  }, 0},
  { X86::POP32r, "popl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::POP32rmr, "popl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::POPDS32, "popl", Convert, { MCK_DS }, Feature_In32BitMode},
  { X86::POPSS32, "popl", Convert, { MCK_SS }, Feature_In32BitMode},
  { X86::POPES32, "popl", Convert, { MCK_ES }, Feature_In32BitMode},
  { X86::POPFS32, "popl", Convert, { MCK_FS }, Feature_In32BitMode},
  { X86::POPGS32, "popl", Convert, { MCK_GS }, Feature_In32BitMode},
  { X86::POP32rmm, "popl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::POP64r, "popq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::POP64rmr, "popq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::POPFS64, "popq", Convert, { MCK_FS }, 0},
  { X86::POPGS64, "popq", Convert, { MCK_GS }, 0},
  { X86::POP64rmm, "popq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::POP16r, "popw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::POP16rmr, "popw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::POPDS16, "popw", Convert, { MCK_DS }, Feature_In32BitMode},
  { X86::POPSS16, "popw", Convert, { MCK_SS }, Feature_In32BitMode},
  { X86::POPES16, "popw", Convert, { MCK_ES }, Feature_In32BitMode},
  { X86::POPFS16, "popw", Convert, { MCK_FS }, 0},
  { X86::POPGS16, "popw", Convert, { MCK_GS }, 0},
  { X86::POP16rmm, "popw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MMX_PORirr, "por", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PORrr, "por", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PORirm, "por", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PORrm, "por", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PREFETCH, "prefetch", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PREFETCHNTA, "prefetchnta", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PREFETCHT0, "prefetcht0", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PREFETCHT1, "prefetcht1", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PREFETCHT2, "prefetcht2", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PREFETCHW, "prefetchw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MMX_PSADBWirr, "psadbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSADBWrr, "psadbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSADBWirm, "psadbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSADBWrm, "psadbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSHUFBrr64, "pshufb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSHUFBrr128, "pshufb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSHUFBrm64, "pshufb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSHUFBrm128, "pshufb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PSHUFDri, "pshufd", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PSHUFDmi, "pshufd", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PSHUFHWri, "pshufhw", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PSHUFHWmi, "pshufhw", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::PSHUFLWri, "pshuflw", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::PSHUFLWmi, "pshuflw", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSHUFWri, "pshufw", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR64, MCK_VR64 }, 0},
  { X86::MMX_PSHUFWmi, "pshufw", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR64 }, 0},
  { X86::MMX_PSIGNBrr64, "psignb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSIGNBrr128, "psignb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSIGNBrm64, "psignb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSIGNBrm128, "psignb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSIGNDrr64, "psignd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSIGNDrr128, "psignd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSIGNDrm64, "psignd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSIGNDrm128, "psignd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSIGNWrr64, "psignw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSIGNWrr128, "psignw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSIGNWrm64, "psignw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSIGNWrm128, "psignw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSLLDrr, "pslld", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSLLDrr, "pslld", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSLLDri, "pslld", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSLLDri, "pslld", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSLLDrm, "pslld", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSLLDrm, "pslld", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PSLLDQri, "pslldq", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSLLQrr, "psllq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSLLQrr, "psllq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSLLQri, "psllq", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSLLQri, "psllq", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSLLQrm, "psllq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSLLQrm, "psllq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSLLWrr, "psllw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSLLWrr, "psllw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSLLWri, "psllw", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSLLWri, "psllw", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSLLWrm, "psllw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSLLWrm, "psllw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSRADrr, "psrad", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSRADrr, "psrad", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSRADri, "psrad", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSRADri, "psrad", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSRADrm, "psrad", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSRADrm, "psrad", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSRAWrr, "psraw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSRAWrr, "psraw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSRAWri, "psraw", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSRAWri, "psraw", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSRAWrm, "psraw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSRAWrm, "psraw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSRLDrr, "psrld", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSRLDrr, "psrld", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSRLDri, "psrld", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSRLDri, "psrld", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSRLDrm, "psrld", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSRLDrm, "psrld", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PSRLDQri, "psrldq", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSRLQrr, "psrlq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSRLQrr, "psrlq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSRLQri, "psrlq", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSRLQri, "psrlq", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSRLQrm, "psrlq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSRLQrm, "psrlq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSRLWrr, "psrlw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSRLWrr, "psrlw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSRLWri, "psrlw", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR64 }, 0},
  { X86::PSRLWri, "psrlw", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32 }, 0},
  { X86::MMX_PSRLWrm, "psrlw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSRLWrm, "psrlw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBBirr, "psubb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBBrr, "psubb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBBirm, "psubb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBBrm, "psubb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBDirr, "psubd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBDrr, "psubd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBDirm, "psubd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBDrm, "psubd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBQirr, "psubq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBQrr, "psubq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBQirm, "psubq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBQrm, "psubq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBSBirr, "psubsb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBSBrr, "psubsb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBSBirm, "psubsb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBSBrm, "psubsb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBSWirr, "psubsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBSWrr, "psubsw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBSWirm, "psubsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBSWrm, "psubsw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBUSBirr, "psubusb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBUSBrr, "psubusb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBUSBirm, "psubusb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBUSBrm, "psubusb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBUSWirr, "psubusw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBUSWrr, "psubusw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBUSWirm, "psubusw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBUSWrm, "psubusw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PSUBWirr, "psubw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSUBWrr, "psubw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PSUBWirm, "psubw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PSUBWrm, "psubw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PSWAPDrr, "pswapd", Convert__Reg1_1__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PSWAPDrm, "pswapd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PTESTrr, "ptest", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PTESTrm, "ptest", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKHBWirr, "punpckhbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PUNPCKHBWrr, "punpckhbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKHBWirm, "punpckhbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PUNPCKHBWrm, "punpckhbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKHDQirr, "punpckhdq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PUNPCKHDQrr, "punpckhdq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKHDQirm, "punpckhdq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PUNPCKHDQrm, "punpckhdq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PUNPCKHQDQrr, "punpckhqdq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PUNPCKHQDQrm, "punpckhqdq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKHWDirr, "punpckhwd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PUNPCKHWDrr, "punpckhwd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKHWDirm, "punpckhwd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PUNPCKHWDrm, "punpckhwd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKLBWirr, "punpcklbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PUNPCKLBWrr, "punpcklbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKLBWirm, "punpcklbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PUNPCKLBWrm, "punpcklbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKLDQirr, "punpckldq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PUNPCKLDQrr, "punpckldq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKLDQirm, "punpckldq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PUNPCKLDQrm, "punpckldq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PUNPCKLQDQrr, "punpcklqdq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::PUNPCKLQDQrm, "punpcklqdq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKLWDirr, "punpcklwd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PUNPCKLWDrr, "punpcklwd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PUNPCKLWDirm, "punpcklwd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PUNPCKLWDrm, "punpcklwd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::PUSHA32, "pushal", Convert, {  }, Feature_In32BitMode},
  { X86::PUSHF32, "pushfl", Convert, {  }, Feature_In32BitMode},
  { X86::PUSHF64, "pushfq", Convert, {  }, Feature_In64BitMode},
  { X86::PUSHF16, "pushfw", Convert, {  }, 0},
  { X86::PUSH32r, "pushl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::PUSH32rmr, "pushl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::PUSHCS32, "pushl", Convert, { MCK_CS }, Feature_In32BitMode},
  { X86::PUSHDS32, "pushl", Convert, { MCK_DS }, Feature_In32BitMode},
  { X86::PUSHSS32, "pushl", Convert, { MCK_SS }, Feature_In32BitMode},
  { X86::PUSHES32, "pushl", Convert, { MCK_ES }, Feature_In32BitMode},
  { X86::PUSHFS32, "pushl", Convert, { MCK_FS }, Feature_In32BitMode},
  { X86::PUSHGS32, "pushl", Convert, { MCK_GS }, Feature_In32BitMode},
  { X86::PUSHi8, "pushl", Convert__ImmSExti32i81_0, { MCK_ImmSExti32i8 }, 0},
  { X86::PUSHi32, "pushl", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::PUSH32rmm, "pushl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PUSH64r, "pushq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::PUSH64rmr, "pushq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::PUSHFS64, "pushq", Convert, { MCK_FS }, 0},
  { X86::PUSHGS64, "pushq", Convert, { MCK_GS }, 0},
  { X86::PUSH64i8, "pushq", Convert__ImmSExti64i81_0, { MCK_ImmSExti64i8 }, 0},
  { X86::PUSH64i32, "pushq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32 }, 0},
  { X86::PUSH64i16, "pushq", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::PUSH64rmm, "pushq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::PUSH16r, "pushw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::PUSH16rmr, "pushw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::PUSHCS16, "pushw", Convert, { MCK_CS }, Feature_In32BitMode},
  { X86::PUSHDS16, "pushw", Convert, { MCK_DS }, Feature_In32BitMode},
  { X86::PUSHSS16, "pushw", Convert, { MCK_SS }, Feature_In32BitMode},
  { X86::PUSHES16, "pushw", Convert, { MCK_ES }, Feature_In32BitMode},
  { X86::PUSHFS16, "pushw", Convert, { MCK_FS }, 0},
  { X86::PUSHGS16, "pushw", Convert, { MCK_GS }, 0},
  { X86::PUSHi16, "pushw", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::PUSH16rmm, "pushw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::MMX_PXORirr, "pxor", Convert__Reg1_1__Tie0__Reg1_0, { MCK_VR64, MCK_VR64 }, 0},
  { X86::PXORrr, "pxor", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::MMX_PXORirm, "pxor", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_VR64 }, 0},
  { X86::PXORrm, "pxor", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::RCL8r1, "rclb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::RCL8m1, "rclb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCL8rCL, "rclb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::RCL8mCL, "rclb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCL8ri, "rclb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::RCL8mi, "rclb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCL32r1, "rcll", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::RCL32m1, "rcll", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCL32rCL, "rcll", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::RCL32mCL, "rcll", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCL32ri, "rcll", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::RCL32mi, "rcll", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCL64r1, "rclq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::RCL64m1, "rclq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCL64rCL, "rclq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::RCL64mCL, "rclq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCL64ri, "rclq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::RCL64mi, "rclq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCL16r1, "rclw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::RCL16m1, "rclw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCL16rCL, "rclw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::RCL16mCL, "rclw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCL16ri, "rclw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::RCL16mi, "rclw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCPPSr, "rcpps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::RCPPSm, "rcpps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::RCPSSr, "rcpss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::RCPSSm, "rcpss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::RCR8r1, "rcrb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::RCR8m1, "rcrb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCR8rCL, "rcrb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::RCR8mCL, "rcrb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCR8ri, "rcrb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::RCR8mi, "rcrb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCR32r1, "rcrl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::RCR32m1, "rcrl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCR32rCL, "rcrl", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::RCR32mCL, "rcrl", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCR32ri, "rcrl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::RCR32mi, "rcrl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCR64r1, "rcrq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::RCR64m1, "rcrq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCR64rCL, "rcrq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::RCR64mCL, "rcrq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCR64ri, "rcrq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::RCR64mi, "rcrq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RCR16r1, "rcrw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::RCR16m1, "rcrw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::RCR16rCL, "rcrw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::RCR16mCL, "rcrw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::RCR16ri, "rcrw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::RCR16mi, "rcrw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::RDFSBASE, "rdfsbasel", Convert__Reg1_0, { MCK_GR32 }, Feature_In64BitMode},
  { X86::RDFSBASE64, "rdfsbaseq", Convert__Reg1_0, { MCK_GR64 }, Feature_In64BitMode},
  { X86::RDGSBASE, "rdgsbasel", Convert__Reg1_0, { MCK_GR32 }, Feature_In64BitMode},
  { X86::RDGSBASE64, "rdgsbaseq", Convert__Reg1_0, { MCK_GR64 }, Feature_In64BitMode},
  { X86::RDMSR, "rdmsr", Convert, {  }, 0},
  { X86::RDPMC, "rdpmc", Convert, {  }, 0},
  { X86::RDRAND32r, "rdrandl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::RDRAND64r, "rdrandq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::RDRAND16r, "rdrandw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::RDTSC, "rdtsc", Convert, {  }, 0},
  { X86::RDTSCP, "rdtscp", Convert, {  }, 0},
  { X86::REP_PREFIX, "rep", Convert, {  }, 0},
  { X86::REPNE_PREFIX, "repne", Convert, {  }, 0},
  { X86::RET, "ret", Convert, {  }, 0},
  { X86::RETI, "ret", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::RETIW, "retw", Convert__Imm1_0, { MCK_Imm }, 0},
  { X86::REX64_PREFIX, "rex64", Convert, {  }, 0},
  { X86::ROL8r1, "rolb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::ROL8m1, "rolb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROL8rCL, "rolb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::ROL8mCL, "rolb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROL8ri, "rolb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::ROL8mi, "rolb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROL32r1, "roll", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::ROL32m1, "roll", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROL32rCL, "roll", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::ROL32mCL, "roll", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROL32ri, "roll", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::ROL32mi, "roll", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROL64r1, "rolq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::ROL64m1, "rolq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROL64rCL, "rolq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::ROL64mCL, "rolq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROL64ri, "rolq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::ROL64mi, "rolq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROL16r1, "rolw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::ROL16m1, "rolw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROL16rCL, "rolw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::ROL16mCL, "rolw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROL16ri, "rolw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::ROL16mi, "rolw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROR8r1, "rorb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::ROR8m1, "rorb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROR8rCL, "rorb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::ROR8mCL, "rorb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROR8ri, "rorb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::ROR8mi, "rorb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROR32r1, "rorl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::ROR32m1, "rorl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROR32rCL, "rorl", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::ROR32mCL, "rorl", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROR32ri, "rorl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::ROR32mi, "rorl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROR64r1, "rorq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::ROR64m1, "rorq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROR64rCL, "rorq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::ROR64mCL, "rorq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROR64ri, "rorq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::ROR64mi, "rorq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROR16r1, "rorw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::ROR16m1, "rorw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::ROR16rCL, "rorw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::ROR16mCL, "rorw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::ROR16ri, "rorw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::ROR16mi, "rorw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::ROUNDPDr, "roundpd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::ROUNDPDm, "roundpd", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::ROUNDPSr, "roundps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::ROUNDPSm, "roundps", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::ROUNDSDr, "roundsd", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::ROUNDSDm, "roundsd", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::ROUNDSSr, "roundss", Convert__Reg1_2__Tie0__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::ROUNDSSm, "roundss", Convert__Reg1_2__Tie0__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::RSM, "rsm", Convert, {  }, 0},
  { X86::RSQRTPSr, "rsqrtps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::RSQRTPSm, "rsqrtps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::RSQRTSSr, "rsqrtss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::RSQRTSSm, "rsqrtss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SAHF, "sahf", Convert, {  }, 0},
  { X86::SAR8r1, "sarb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::SAR8m1, "sarb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SAR8rCL, "sarb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::SAR8mCL, "sarb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SAR8ri, "sarb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::SAR8mi, "sarb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SAR32r1, "sarl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::SAR32m1, "sarl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SAR32rCL, "sarl", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::SAR32mCL, "sarl", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SAR32ri, "sarl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::SAR32mi, "sarl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SAR64r1, "sarq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::SAR64m1, "sarq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SAR64rCL, "sarq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::SAR64mCL, "sarq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SAR64ri, "sarq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::SAR64mi, "sarq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SAR16r1, "sarw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::SAR16m1, "sarw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SAR16rCL, "sarw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::SAR16mCL, "sarw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SAR16ri, "sarw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::SAR16mi, "sarw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SBB8rr, "sbbb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::SBB8mr, "sbbb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::SBB8i8, "sbbb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::SBB8ri, "sbbb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::SBB8mi, "sbbb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SBB8rm, "sbbb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::SBB32rr, "sbbl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::SBB32mr, "sbbl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::SBB32ri8, "sbbl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::SBB32mi8, "sbbl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::SBB32i32, "sbbl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::SBB32ri, "sbbl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::SBB32mi, "sbbl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SBB32rm, "sbbl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::SBB64rr, "sbbq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::SBB64mr, "sbbq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::SBB64ri8, "sbbq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::SBB64mi8, "sbbq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::SBB64i32, "sbbq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::SBB64ri32, "sbbq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::SBB64mi32, "sbbq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::SBB64rm, "sbbq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::SBB16rr, "sbbw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::SBB16mr, "sbbw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::SBB16ri8, "sbbw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::SBB16mi8, "sbbw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::SBB16i16, "sbbw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::SBB16ri, "sbbw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::SBB16mi, "sbbw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SBB16rm, "sbbw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::SCAS8, "scasb", Convert, {  }, 0},
  { X86::SCAS32, "scasl", Convert, {  }, 0},
  { X86::SCAS64, "scasq", Convert, {  }, 0},
  { X86::SCAS16, "scasw", Convert, {  }, 0},
  { X86::SETAr, "seta", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETAm, "seta", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETAEr, "setae", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETAEm, "setae", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETBr, "setb", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETBm, "setb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETBEr, "setbe", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETBEm, "setbe", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETEr, "sete", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETEm, "sete", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETGr, "setg", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETGm, "setg", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETGEr, "setge", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETGEm, "setge", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETLr, "setl", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETLm, "setl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETLEr, "setle", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETLEm, "setle", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETNEr, "setne", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETNEm, "setne", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETNOr, "setno", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETNOm, "setno", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETNPr, "setnp", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETNPm, "setnp", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETNSr, "setns", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETNSm, "setns", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETOr, "seto", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETOm, "seto", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETPr, "setp", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETPm, "setp", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SETSr, "sets", Convert__Reg1_0, { MCK_GR8 }, 0},
  { X86::SETSm, "sets", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SFENCE, "sfence", Convert, {  }, 0},
  { X86::SGDTm, "sgdt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SGDT16m, "sgdtw", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::SHL8r1, "shlb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::SHL8m1, "shlb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHL8rCL, "shlb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::SHL8mCL, "shlb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHL8ri, "shlb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::SHL8mi, "shlb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHLD32rri8, "shldl", Convert__Reg1_0__Tie0__Reg1_1__imm1, { MCK_GR32, MCK_GR32 }, 0},
  { X86::SHLD32mri8, "shldl", Convert__Mem5_0__Reg1_1__imm1, { MCK_Mem, MCK_GR32 }, 0},
  { X86::SHLD32rrCL, "shldl", Convert__Reg1_2__Tie0__Reg1_1, { MCK_CL, MCK_GR32, MCK_GR32 }, 0},
  { X86::SHLD32mrCL, "shldl", Convert__Mem5_2__Reg1_1, { MCK_CL, MCK_GR32, MCK_Mem }, 0},
  { X86::SHLD32rri8, "shldl", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32, MCK_GR32 }, 0},
  { X86::SHLD32mri8, "shldl", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32, MCK_Mem }, 0},
  { X86::SHLD64rri8, "shldq", Convert__Reg1_0__Tie0__Reg1_1__imm1, { MCK_GR64, MCK_GR64 }, 0},
  { X86::SHLD64mri8, "shldq", Convert__Mem5_0__Reg1_1__imm1, { MCK_Mem, MCK_GR64 }, 0},
  { X86::SHLD64rrCL, "shldq", Convert__Reg1_2__Tie0__Reg1_1, { MCK_CL, MCK_GR64, MCK_GR64 }, 0},
  { X86::SHLD64mrCL, "shldq", Convert__Mem5_2__Reg1_1, { MCK_CL, MCK_GR64, MCK_Mem }, 0},
  { X86::SHLD64rri8, "shldq", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR64, MCK_GR64 }, 0},
  { X86::SHLD64mri8, "shldq", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR64, MCK_Mem }, 0},
  { X86::SHLD16rri8, "shldw", Convert__Reg1_0__Tie0__Reg1_1__imm1, { MCK_GR16, MCK_GR16 }, 0},
  { X86::SHLD16mri8, "shldw", Convert__Mem5_0__Reg1_1__imm1, { MCK_Mem, MCK_GR16 }, 0},
  { X86::SHLD16rrCL, "shldw", Convert__Reg1_2__Tie0__Reg1_1, { MCK_CL, MCK_GR16, MCK_GR16 }, 0},
  { X86::SHLD16mrCL, "shldw", Convert__Mem5_2__Reg1_1, { MCK_CL, MCK_GR16, MCK_Mem }, 0},
  { X86::SHLD16rri8, "shldw", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16, MCK_GR16 }, 0},
  { X86::SHLD16mri8, "shldw", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16, MCK_Mem }, 0},
  { X86::SHL32r1, "shll", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::SHL32m1, "shll", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHL32rCL, "shll", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::SHL32mCL, "shll", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHL32ri, "shll", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::SHL32mi, "shll", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHL64r1, "shlq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::SHL64m1, "shlq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHL64rCL, "shlq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::SHL64mCL, "shlq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHL64ri, "shlq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::SHL64mi, "shlq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHL16r1, "shlw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::SHL16m1, "shlw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHL16rCL, "shlw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::SHL16mCL, "shlw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHL16ri, "shlw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::SHL16mi, "shlw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHR8r1, "shrb", Convert__Reg1_0__Tie0, { MCK_GR8 }, 0},
  { X86::SHR8m1, "shrb", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHR8rCL, "shrb", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR8 }, 0},
  { X86::SHR8mCL, "shrb", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHR8ri, "shrb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::SHR8mi, "shrb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHRD32rri8, "shrdl", Convert__Reg1_0__Tie0__Reg1_1__imm1, { MCK_GR32, MCK_GR32 }, 0},
  { X86::SHRD32mri8, "shrdl", Convert__Mem5_0__Reg1_1__imm1, { MCK_Mem, MCK_GR32 }, 0},
  { X86::SHRD32rrCL, "shrdl", Convert__Reg1_2__Tie0__Reg1_1, { MCK_CL, MCK_GR32, MCK_GR32 }, 0},
  { X86::SHRD32mrCL, "shrdl", Convert__Mem5_2__Reg1_1, { MCK_CL, MCK_GR32, MCK_Mem }, 0},
  { X86::SHRD32rri8, "shrdl", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32, MCK_GR32 }, 0},
  { X86::SHRD32mri8, "shrdl", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32, MCK_Mem }, 0},
  { X86::SHRD64rri8, "shrdq", Convert__Reg1_0__Tie0__Reg1_1__imm1, { MCK_GR64, MCK_GR64 }, 0},
  { X86::SHRD64mri8, "shrdq", Convert__Mem5_0__Reg1_1__imm1, { MCK_Mem, MCK_GR64 }, 0},
  { X86::SHRD64rrCL, "shrdq", Convert__Reg1_2__Tie0__Reg1_1, { MCK_CL, MCK_GR64, MCK_GR64 }, 0},
  { X86::SHRD64mrCL, "shrdq", Convert__Mem5_2__Reg1_1, { MCK_CL, MCK_GR64, MCK_Mem }, 0},
  { X86::SHRD64rri8, "shrdq", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR64, MCK_GR64 }, 0},
  { X86::SHRD64mri8, "shrdq", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR64, MCK_Mem }, 0},
  { X86::SHRD16rri8, "shrdw", Convert__Reg1_0__Tie0__Reg1_1__imm1, { MCK_GR16, MCK_GR16 }, 0},
  { X86::SHRD16mri8, "shrdw", Convert__Mem5_0__Reg1_1__imm1, { MCK_Mem, MCK_GR16 }, 0},
  { X86::SHRD16rrCL, "shrdw", Convert__Reg1_2__Tie0__Reg1_1, { MCK_CL, MCK_GR16, MCK_GR16 }, 0},
  { X86::SHRD16mrCL, "shrdw", Convert__Mem5_2__Reg1_1, { MCK_CL, MCK_GR16, MCK_Mem }, 0},
  { X86::SHRD16rri8, "shrdw", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16, MCK_GR16 }, 0},
  { X86::SHRD16mri8, "shrdw", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16, MCK_Mem }, 0},
  { X86::SHR32r1, "shrl", Convert__Reg1_0__Tie0, { MCK_GR32 }, 0},
  { X86::SHR32m1, "shrl", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHR32rCL, "shrl", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR32 }, 0},
  { X86::SHR32mCL, "shrl", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHR32ri, "shrl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::SHR32mi, "shrl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHR64r1, "shrq", Convert__Reg1_0__Tie0, { MCK_GR64 }, 0},
  { X86::SHR64m1, "shrq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHR64rCL, "shrq", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR64 }, 0},
  { X86::SHR64mCL, "shrq", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHR64ri, "shrq", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR64 }, 0},
  { X86::SHR64mi, "shrq", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHR16r1, "shrw", Convert__Reg1_0__Tie0, { MCK_GR16 }, 0},
  { X86::SHR16m1, "shrw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SHR16rCL, "shrw", Convert__Reg1_1__Tie0, { MCK_CL, MCK_GR16 }, 0},
  { X86::SHR16mCL, "shrw", Convert__Mem5_1, { MCK_CL, MCK_Mem }, 0},
  { X86::SHR16ri, "shrw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::SHR16mi, "shrw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SHUFPDrri, "shufpd", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::SHUFPDrmi, "shufpd", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::SHUFPSrri, "shufps", Convert__Reg1_2__Tie0__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::SHUFPSrmi, "shufps", Convert__Reg1_2__Tie0__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::SIDTm, "sidt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SIDT16m, "sidtw", Convert__Mem5_0, { MCK_Mem }, Feature_In32BitMode},
  { X86::SLDT16m, "sldt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SLDT32r, "sldtl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::SLDT64r, "sldtq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::SLDT64m, "sldtq", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SLDT16r, "sldtw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::SLDT16m, "sldtw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SMSW32r, "smswl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::SMSW64r, "smswq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::SMSW16r, "smsww", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::SMSW16m, "smsww", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SQRTPDr, "sqrtpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SQRTPDm, "sqrtpd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SQRTPSr, "sqrtps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SQRTPSm, "sqrtps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SQRTSDr, "sqrtsd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SQRTSDm, "sqrtsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SQRTSSr, "sqrtss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SQRTSSm, "sqrtss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SS_PREFIX, "ss", Convert, {  }, 0},
  { X86::STC, "stc", Convert, {  }, 0},
  { X86::STD, "std", Convert, {  }, 0},
  { X86::STI, "sti", Convert, {  }, 0},
  { X86::STMXCSR, "stmxcsr", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::STOSB, "stosb", Convert, {  }, 0},
  { X86::STOSD, "stosl", Convert, {  }, 0},
  { X86::STOSQ, "stosq", Convert, {  }, 0},
  { X86::STOSW, "stosw", Convert, {  }, 0},
  { X86::STR32r, "strl", Convert__Reg1_0, { MCK_GR32 }, 0},
  { X86::STR64r, "strq", Convert__Reg1_0, { MCK_GR64 }, 0},
  { X86::STR16r, "strw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::STRm, "strw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::SUB8rr, "subb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::SUB8mr, "subb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::SUB8i8, "subb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::SUB8ri, "subb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::SUB8mi, "subb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SUB8rm, "subb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::SUB32rr, "subl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::SUB32mr, "subl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::SUB32ri8, "subl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::SUB32mi8, "subl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::SUB32i32, "subl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::SUB32ri, "subl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::SUB32mi, "subl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SUB32rm, "subl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::SUBPDrr, "subpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SUBPDrm, "subpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SUBPSrr, "subps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SUBPSrm, "subps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SUB64rr, "subq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::SUB64mr, "subq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::SUB64ri8, "subq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::SUB64mi8, "subq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::SUB64i32, "subq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::SUB64ri32, "subq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::SUB64mi32, "subq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::SUB64rm, "subq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::SUBSDrr, "subsd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SUBSDrm, "subsd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SUBSSrr, "subss", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::SUBSSrm, "subss", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::SUB16rr, "subw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::SUB16mr, "subw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::SUB16ri8, "subw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::SUB16mi8, "subw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::SUB16i16, "subw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::SUB16ri, "subw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::SUB16mi, "subw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::SUB16rm, "subw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::SWAPGS, "swapgs", Convert, {  }, 0},
  { X86::SYSCALL, "syscall", Convert, {  }, 0},
  { X86::SYSENTER, "sysenter", Convert, {  }, 0},
  { X86::SYSEXIT, "sysexit", Convert, {  }, Feature_In32BitMode},
  { X86::SYSEXIT64, "sysexit", Convert, {  }, Feature_In64BitMode},
  { X86::SYSRETL, "sysretl", Convert, {  }, 0},
  { X86::SYSRETQ, "sysretq", Convert, {  }, Feature_In64BitMode},
  { X86::TEST8rr, "testb", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::TEST8rm, "testb", Convert__Reg1_0__Mem5_1, { MCK_GR8, MCK_Mem }, 0},
  { X86::TEST8i8, "testb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::TEST8ri, "testb", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::TEST8mi, "testb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::TEST8rm, "testb", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::TEST32rr, "testl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::TEST32rm, "testl", Convert__Reg1_0__Mem5_1, { MCK_GR32, MCK_Mem }, 0},
  { X86::TEST32i32, "testl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::TEST32ri, "testl", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::TEST32mi, "testl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::TEST32rm, "testl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::TEST64rr, "testq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::TEST64rm, "testq", Convert__Reg1_0__Mem5_1, { MCK_GR64, MCK_Mem }, 0},
  { X86::TEST64i32, "testq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::TEST64ri32, "testq", Convert__Reg1_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::TEST64mi32, "testq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::TEST64rm, "testq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::TEST16rr, "testw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::TEST16rm, "testw", Convert__Reg1_0__Mem5_1, { MCK_GR16, MCK_Mem }, 0},
  { X86::TEST16i16, "testw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::TEST16ri, "testw", Convert__Reg1_1__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::TEST16mi, "testw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::TEST16rm, "testw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::TZCNT32rr, "tzcntl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::TZCNT32rm, "tzcntl", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::TZCNT64rr, "tzcntq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::TZCNT64rm, "tzcntq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::TZCNT16rr, "tzcntw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::TZCNT16rm, "tzcntw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::UCOMISDrr, "ucomisd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::UCOMISDrm, "ucomisd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::UCOMISSrr, "ucomiss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::UCOMISSrm, "ucomiss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::TRAP, "ud2", Convert, {  }, 0},
  { X86::UD2B, "ud2b", Convert, {  }, 0},
  { X86::UNPCKHPDrr, "unpckhpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::UNPCKHPDrm, "unpckhpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::UNPCKHPSrr, "unpckhps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::UNPCKHPSrm, "unpckhps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::UNPCKLPDrr, "unpcklpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::UNPCKLPDrm, "unpcklpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::UNPCKLPSrr, "unpcklps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::UNPCKLPSrm, "unpcklps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VADDPDrr, "vaddpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDPDYrr, "vaddpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDPDrm, "vaddpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDPDYrm, "vaddpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDPSrr, "vaddps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDPSYrr, "vaddps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDPSrm, "vaddps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDPSYrm, "vaddps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDSDrr, "vaddsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSDrm, "vaddsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSSrr, "vaddss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSSrm, "vaddss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSUBPDrr, "vaddsubpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSUBPDYrr, "vaddsubpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDSUBPDrm, "vaddsubpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSUBPDYrm, "vaddsubpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDSUBPSrr, "vaddsubps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSUBPSYrr, "vaddsubps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VADDSUBPSrm, "vaddsubps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VADDSUBPSYrm, "vaddsubps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VAESDECrr, "vaesdec", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESDECrm, "vaesdec", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESDECLASTrr, "vaesdeclast", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESDECLASTrm, "vaesdeclast", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESENCrr, "vaesenc", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESENCrm, "vaesenc", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESENCLASTrr, "vaesenclast", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESENCLASTrm, "vaesenclast", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESIMCrr, "vaesimc", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESIMCrm, "vaesimc", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VAESKEYGENASSIST128rr, "vaeskeygenassist", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VAESKEYGENASSIST128rm, "vaeskeygenassist", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VANDNPDrr, "vandnpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDNPDrr, "vandnpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDNPDYrr, "vandnpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDNPDrm, "vandnpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDNPDrm, "vandnpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDNPDYrm, "vandnpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDNPSrr, "vandnps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDNPSrr, "vandnps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDNPSYrr, "vandnps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDNPSrm, "vandnps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDNPSrm, "vandnps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDNPSYrm, "vandnps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDPDrr, "vandpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDPDrr, "vandpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDPDYrr, "vandpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDPDrm, "vandpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDPDrm, "vandpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDPDYrm, "vandpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDPSrr, "vandps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDPSrr, "vandps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDPSYrr, "vandps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VANDPSrm, "vandps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFsANDPSrm, "vandps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VANDPSYrm, "vandps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDPDrri, "vblendpd", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDPDYrri, "vblendpd", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDPDrmi, "vblendpd", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDPDYrmi, "vblendpd", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDPSrri, "vblendps", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDPSYrri, "vblendps", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDPSrmi, "vblendps", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDPSYrmi, "vblendps", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDVPDrr, "vblendvpd", Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDVPDrm, "vblendvpd", Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDVPDYrr, "vblendvpd", Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDVPDYrm, "vblendvpd", Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDVPSrr, "vblendvps", Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDVPSrm, "vblendvps", Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VBLENDVPSYrr, "vblendvps", Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBLENDVPSYrm, "vblendvps", Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VBROADCASTF128, "vbroadcastf128", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VBROADCASTSD, "vbroadcastsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VBROADCASTSS, "vbroadcastss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VBROADCASTSSY, "vbroadcastss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCMPPDrri, "vcmp", Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0, { MCK_Imm, MCK_pd, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPDYrri, "vcmp", Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0, { MCK_Imm, MCK_pd, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPPDrmi, "vcmp", Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0, { MCK_Imm, MCK_pd, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPDYrmi, "vcmp", Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0, { MCK_Imm, MCK_pd, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPPSrri, "vcmp", Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0, { MCK_Imm, MCK_ps, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPSYrri, "vcmp", Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0, { MCK_Imm, MCK_ps, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPPSrmi, "vcmp", Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0, { MCK_Imm, MCK_ps, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPSYrmi, "vcmp", Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0, { MCK_Imm, MCK_ps, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPSDrr, "vcmp", Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0, { MCK_Imm, MCK_sd, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPSDrm, "vcmp", Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0, { MCK_Imm, MCK_sd, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPSSrr, "vcmp", Convert__Reg1_4__Reg1_3__Reg1_2__Imm1_0, { MCK_Imm, MCK_ss, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPSSrm, "vcmp", Convert__Reg1_4__Reg1_3__Mem5_2__Imm1_0, { MCK_Imm, MCK_ss, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPDrri_alt, "vcmppd", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPDYrri_alt, "vcmppd", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPPDrmi_alt, "vcmppd", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPDYrmi_alt, "vcmppd", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPPSrri_alt, "vcmpps", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPSYrri_alt, "vcmpps", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPPSrmi_alt, "vcmpps", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPPSYrmi_alt, "vcmpps", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VCMPSDrr_alt, "vcmpsd", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPSDrm_alt, "vcmpsd", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPSSrr_alt, "vcmpss", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCMPSSrm_alt, "vcmpss", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCOMISDrr, "vcomisd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCOMISDrm, "vcomisd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCOMISSrr, "vcomiss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCOMISSrm, "vcomiss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTDQ2PDrr, "vcvtdq2pd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTDQ2PDYrr, "vcvtdq2pd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR256 }, 0},
  { X86::VCVTDQ2PDrm, "vcvtdq2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTDQ2PDYrm, "vcvtdq2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCVTDQ2PSrr, "vcvtdq2ps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTDQ2PSYrr, "vcvtdq2ps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VCVTDQ2PSrm, "vcvtdq2ps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTDQ2PSYrm, "vcvtdq2ps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCVTPD2DQrr, "vcvtpd2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPD2DQXrYr, "vcvtpd2dq", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTPD2DQXrr, "vcvtpd2dqx", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPD2DQXrm, "vcvtpd2dqx", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPD2DQYrr, "vcvtpd2dqy", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTPD2DQYrm, "vcvtpd2dqy", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPD2PSrr, "vcvtpd2ps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPD2PSXrYr, "vcvtpd2ps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTPD2PSXrr, "vcvtpd2psx", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPD2PSXrm, "vcvtpd2psx", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPD2PSYrr, "vcvtpd2psy", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTPD2PSYrm, "vcvtpd2psy", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPH2PSrr, "vcvtph2ps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPH2PSYrr, "vcvtph2ps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR256 }, 0},
  { X86::VCVTPH2PSrm, "vcvtph2ps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPH2PSYrm, "vcvtph2ps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCVTPS2DQrr, "vcvtps2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPS2DQYrr, "vcvtps2dq", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VCVTPS2DQrm, "vcvtps2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPS2DQYrm, "vcvtps2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCVTPS2PDrr, "vcvtps2pd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPS2PDYrr, "vcvtps2pd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_VR256 }, 0},
  { X86::VCVTPS2PDrm, "vcvtps2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTPS2PDYrm, "vcvtps2pd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCVTPS2PHrr, "vcvtps2ph", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTPS2PHmr, "vcvtps2ph", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::VCVTPS2PHYrr, "vcvtps2ph", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTPS2PHYmr, "vcvtps2ph", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR256, MCK_Mem }, 0},
  { X86::VCVTSD2SIrr, "vcvtsd2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VCVTSD2SI64rr, "vcvtsd2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VCVTSD2SIrm, "vcvtsd2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::VCVTSD2SI64rm, "vcvtsd2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::VCVTSD2SSrr, "vcvtsd2ss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSD2SSrm, "vcvtsd2ss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SDrr, "vcvtsi2sd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SDrm, "vcvtsi2sd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SDLrr, "vcvtsi2sdl", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SDLrm, "vcvtsi2sdl", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SD64rr, "vcvtsi2sdq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SD64rm, "vcvtsi2sdq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SSrr, "vcvtsi2ss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SSrm, "vcvtsi2ss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SS64rr, "vcvtsi2ssq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSI2SS64rm, "vcvtsi2ssq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSS2SDrr, "vcvtss2sd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSS2SDrm, "vcvtss2sd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTSS2SI64rr, "vcvtss2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VCVTSS2SI64rm, "vcvtss2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::VCVTSS2SIrr, "vcvtss2sil", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VCVTSS2SIrm, "vcvtss2sil", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::VCVTTPD2DQrr, "vcvttpd2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTTPD2DQXrYr, "vcvttpd2dq", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTTPD2DQXrr, "vcvttpd2dqx", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTTPD2DQXrm, "vcvttpd2dqx", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTTPD2DQYrr, "vcvttpd2dqy", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_FR32 }, 0},
  { X86::VCVTTPD2DQYrm, "vcvttpd2dqy", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTTPS2DQrr, "vcvttps2dq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VCVTTPS2DQYrr, "vcvttps2dq", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VCVTTPS2DQrm, "vcvttps2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VCVTTPS2DQYrm, "vcvttps2dq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VCVTTSD2SIrr, "vcvttsd2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VCVTTSD2SI64rr, "vcvttsd2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VCVTTSD2SIrm, "vcvttsd2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::VCVTTSD2SI64rm, "vcvttsd2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::VCVTTSS2SIrr, "vcvttss2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VCVTTSS2SI64rr, "vcvttss2si", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VCVTTSS2SIrm, "vcvttss2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::VCVTTSS2SI64rm, "vcvttss2si", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::VDIVPDrr, "vdivpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVPDYrr, "vdivpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VDIVPDrm, "vdivpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVPDYrm, "vdivpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VDIVPSrr, "vdivps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVPSYrr, "vdivps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VDIVPSrm, "vdivps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVPSYrm, "vdivps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VDIVSDrr, "vdivsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVSDrm, "vdivsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVSSrr, "vdivss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDIVSSrm, "vdivss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDPPDrri, "vdppd", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDPPDrmi, "vdppd", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDPPSrri, "vdpps", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDPPSYrri, "vdpps", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VDPPSrmi, "vdpps", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VDPPSYrmi, "vdpps", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VERRr, "verr", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::VERRm, "verr", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VERWr, "verw", Convert__Reg1_0, { MCK_GR16 }, 0},
  { X86::VERWm, "verw", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VEXTRACTF128rr, "vextractf128", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_FR32 }, 0},
  { X86::VEXTRACTF128mr, "vextractf128", Convert__Mem5_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_Mem }, 0},
  { X86::VEXTRACTPSrr, "vextractps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::VEXTRACTPSrr64, "vextractps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR64 }, 0},
  { X86::VEXTRACTPSmr, "vextractps", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::VFMADDPDr132r, "vfmadd132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPDr132rY, "vfmadd132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPDr132m, "vfmadd132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPDr132mY, "vfmadd132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPSr132r, "vfmadd132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPSr132rY, "vfmadd132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPSr132m, "vfmadd132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPSr132mY, "vfmadd132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPDr213r, "vfmadd213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPDr213rY, "vfmadd213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPDr213m, "vfmadd213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPDr213mY, "vfmadd213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPSr213r, "vfmadd213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPSr213rY, "vfmadd213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPSr213m, "vfmadd213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPSr213mY, "vfmadd213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPDr231r, "vfmadd231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPDr231rY, "vfmadd231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPDr231m, "vfmadd231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPDr231mY, "vfmadd231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPSr231r, "vfmadd231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPSr231rY, "vfmadd231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDPSr231m, "vfmadd231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDPSr231mY, "vfmadd231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPDr132r, "vfmaddsub132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPDr132rY, "vfmaddsub132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPDr132m, "vfmaddsub132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPDr132mY, "vfmaddsub132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPSr132r, "vfmaddsub132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPSr132rY, "vfmaddsub132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPSr132m, "vfmaddsub132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPSr132mY, "vfmaddsub132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPDr213r, "vfmaddsub213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPDr213rY, "vfmaddsub213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPDr213m, "vfmaddsub213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPDr213mY, "vfmaddsub213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPSr213r, "vfmaddsub213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPSr213rY, "vfmaddsub213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPSr213m, "vfmaddsub213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPSr213mY, "vfmaddsub213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPDr231r, "vfmaddsub231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPDr231rY, "vfmaddsub231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPDr231m, "vfmaddsub231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPDr231mY, "vfmaddsub231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPSr231r, "vfmaddsub231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPSr231rY, "vfmaddsub231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMADDSUBPSr231m, "vfmaddsub231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMADDSUBPSr231mY, "vfmaddsub231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPDr132r, "vfmsub132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPDr132rY, "vfmsub132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPDr132m, "vfmsub132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPDr132mY, "vfmsub132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPSr132r, "vfmsub132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPSr132rY, "vfmsub132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPSr132m, "vfmsub132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPSr132mY, "vfmsub132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPDr213r, "vfmsub213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPDr213rY, "vfmsub213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPDr213m, "vfmsub213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPDr213mY, "vfmsub213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPSr213r, "vfmsub213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPSr213rY, "vfmsub213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPSr213m, "vfmsub213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPSr213mY, "vfmsub213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPDr231r, "vfmsub231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPDr231rY, "vfmsub231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPDr231m, "vfmsub231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPDr231mY, "vfmsub231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPSr231r, "vfmsub231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPSr231rY, "vfmsub231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBPSr231m, "vfmsub231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBPSr231mY, "vfmsub231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPDr132r, "vfmsubadd132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPDr132rY, "vfmsubadd132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPDr132m, "vfmsubadd132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPDr132mY, "vfmsubadd132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPSr132r, "vfmsubadd132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPSr132rY, "vfmsubadd132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPSr132m, "vfmsubadd132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPSr132mY, "vfmsubadd132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPDr213r, "vfmsubadd213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPDr213rY, "vfmsubadd213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPDr213m, "vfmsubadd213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPDr213mY, "vfmsubadd213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPSr213r, "vfmsubadd213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPSr213rY, "vfmsubadd213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPSr213m, "vfmsubadd213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPSr213mY, "vfmsubadd213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPDr231r, "vfmsubadd231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPDr231rY, "vfmsubadd231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPDr231m, "vfmsubadd231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPDr231mY, "vfmsubadd231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPSr231r, "vfmsubadd231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPSr231rY, "vfmsubadd231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFMSUBADDPSr231m, "vfmsubadd231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFMSUBADDPSr231mY, "vfmsubadd231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPDr132r, "vfnmadd132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPDr132rY, "vfnmadd132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPDr132m, "vfnmadd132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPDr132mY, "vfnmadd132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPSr132r, "vfnmadd132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPSr132rY, "vfnmadd132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPSr132m, "vfnmadd132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPSr132mY, "vfnmadd132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPDr213r, "vfnmadd213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPDr213rY, "vfnmadd213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPDr213m, "vfnmadd213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPDr213mY, "vfnmadd213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPSr213r, "vfnmadd213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPSr213rY, "vfnmadd213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPSr213m, "vfnmadd213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPSr213mY, "vfnmadd213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPDr231r, "vfnmadd231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPDr231rY, "vfnmadd231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPDr231m, "vfnmadd231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPDr231mY, "vfnmadd231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPSr231r, "vfnmadd231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPSr231rY, "vfnmadd231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMADDPSr231m, "vfnmadd231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMADDPSr231mY, "vfnmadd231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPDr132r, "vfnmsub132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPDr132rY, "vfnmsub132pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPDr132m, "vfnmsub132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPDr132mY, "vfnmsub132pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPSr132r, "vfnmsub132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPSr132rY, "vfnmsub132ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPSr132m, "vfnmsub132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPSr132mY, "vfnmsub132ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPDr213r, "vfnmsub213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPDr213rY, "vfnmsub213pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPDr213m, "vfnmsub213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPDr213mY, "vfnmsub213pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPSr213r, "vfnmsub213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPSr213rY, "vfnmsub213ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPSr213m, "vfnmsub213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPSr213mY, "vfnmsub213ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPDr231r, "vfnmsub231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPDr231rY, "vfnmsub231pd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPDr231m, "vfnmsub231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPDr231mY, "vfnmsub231pd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPSr231r, "vfnmsub231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPSr231rY, "vfnmsub231ps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFNMSUBPSr231m, "vfnmsub231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VFNMSUBPSr231mY, "vfnmsub231ps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHADDPDrr, "vhaddpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHADDPDYrr, "vhaddpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHADDPDrm, "vhaddpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHADDPDYrm, "vhaddpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHADDPSrr, "vhaddps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHADDPSYrr, "vhaddps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHADDPSrm, "vhaddps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHADDPSYrm, "vhaddps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHSUBPDrr, "vhsubpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHSUBPDYrr, "vhsubpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHSUBPDrm, "vhsubpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHSUBPDYrm, "vhsubpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHSUBPSrr, "vhsubps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHSUBPSYrr, "vhsubps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VHSUBPSrm, "vhsubps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VHSUBPSYrm, "vhsubps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VINSERTF128rr, "vinsertf128", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_VR256, MCK_VR256 }, 0},
  { X86::VINSERTF128rm, "vinsertf128", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VINSERTPSrr, "vinsertps", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VINSERTPSrm, "vinsertps", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VLDDQUrm, "vlddqu", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VLDDQUYrm, "vlddqu", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VLDMXCSR, "vldmxcsr", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VMASKMOVDQU, "vmaskmovdqu", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMASKMOVDQU64, "vmaskmovdqu", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMASKMOVPDmr, "vmaskmovpd", Convert__Mem5_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_Mem }, 0},
  { X86::VMASKMOVPDYmr, "vmaskmovpd", Convert__Mem5_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_Mem }, 0},
  { X86::VMASKMOVPDrm, "vmaskmovpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMASKMOVPDYrm, "vmaskmovpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMASKMOVPSmr, "vmaskmovps", Convert__Mem5_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_Mem }, 0},
  { X86::VMASKMOVPSYmr, "vmaskmovps", Convert__Mem5_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_Mem }, 0},
  { X86::VMASKMOVPSrm, "vmaskmovps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMASKMOVPSYrm, "vmaskmovps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMAXPDrr, "vmaxpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXPDYrr, "vmaxpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMAXPDrm, "vmaxpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXPDYrm, "vmaxpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMAXPSrr, "vmaxps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXPSYrr, "vmaxps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMAXPSrm, "vmaxps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXPSYrm, "vmaxps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMAXSDrr, "vmaxsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXSDrm, "vmaxsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXSSrr, "vmaxss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMAXSSrm, "vmaxss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMCALL, "vmcall", Convert, {  }, 0},
  { X86::VMCLEARm, "vmclear", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VMINPDrr, "vminpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINPDYrr, "vminpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMINPDrm, "vminpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINPDYrm, "vminpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMINPSrr, "vminps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINPSYrr, "vminps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMINPSrm, "vminps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINPSYrm, "vminps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMINSDrr, "vminsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINSDrm, "vminsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINSSrr, "vminss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMINSSrm, "vminss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMLAUNCH, "vmlaunch", Convert, {  }, 0},
  { X86::FsVMOVAPDrr, "vmovapd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVAPDrr, "vmovapd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVAPDmr, "vmovapd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVAPDYrr, "vmovapd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVAPDYmr, "vmovapd", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVAPDrm, "vmovapd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVAPDYrm, "vmovapd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::FsVMOVAPSrr, "vmovaps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVAPSrr, "vmovaps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVAPSmr, "vmovaps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVAPSYrr, "vmovaps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVAPSYmr, "vmovaps", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVAPSrm, "vmovaps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVAPSYrm, "vmovaps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVDI2PDIrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::VMOVDI2SSrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::VMOVZDI2PDIrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_FR32 }, 0},
  { X86::VMOV64toPQIrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::VMOV64toSDrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::VMOVZQI2PQIrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::VMOVPDI2DIrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VMOVSS2DIrr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VMOVQd64rr_alt, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVSDto64rr, "vmovd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVPDI2DImr, "vmovd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVSS2DImr, "vmovd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVDI2PDIrm, "vmovd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVDI2SSrm, "vmovd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVZDI2PDIrm, "vmovd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVDDUPrr, "vmovddup", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVDDUPYrr, "vmovddup", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVDDUPrm, "vmovddup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVDDUPYrm, "vmovddup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVDQArr, "vmovdqa", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVDQAmr, "vmovdqa", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVDQAYrr, "vmovdqa", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVDQAYmr, "vmovdqa", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVDQArm, "vmovdqa", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVDQAYrm, "vmovdqa", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVDQUrr, "vmovdqu", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVDQUmr, "vmovdqu", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVDQUYrr, "vmovdqu", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVDQUYmr, "vmovdqu", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVDQUrm, "vmovdqu", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVDQUYrm, "vmovdqu", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVHLPSrr, "vmovhlps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVHPDmr, "vmovhpd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVHPDrm, "vmovhpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVHPSmr, "vmovhps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVHPSrm, "vmovhps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVLHPSrr, "vmovlhps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVLPDmr, "vmovlpd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVLPDrm, "vmovlpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVLPSmr, "vmovlps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVLPSrm, "vmovlps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVMSKPDrr32, "vmovmskpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VMOVMSKPDr64r, "vmovmskpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVMSKPDrr64, "vmovmskpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVMSKPDYrr32, "vmovmskpd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_GR32 }, 0},
  { X86::VMOVMSKPDYr64r, "vmovmskpd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_GR64 }, 0},
  { X86::VMOVMSKPDYrr64, "vmovmskpd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_GR64 }, 0},
  { X86::VMOVMSKPSrr32, "vmovmskps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VMOVMSKPSr64r, "vmovmskps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVMSKPSrr64, "vmovmskps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVMSKPSYrr32, "vmovmskps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_GR32 }, 0},
  { X86::VMOVMSKPSYr64r, "vmovmskps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_GR64 }, 0},
  { X86::VMOVMSKPSYrr64, "vmovmskps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_GR64 }, 0},
  { X86::VMOVNTDQ_64mr, "vmovntdq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVNTDQmr, "vmovntdq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVNTDQY_64mr, "vmovntdq", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVNTDQYmr, "vmovntdq", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVNTDQArm, "vmovntdqa", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVNTPDmr, "vmovntpd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVNTPDYmr, "vmovntpd", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVNTPSmr, "vmovntps", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVNTPSYmr, "vmovntps", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVQs64rr, "vmovq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_FR32 }, 0},
  { X86::VMOVQd64rr, "vmovq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VMOVQxrxr, "vmovq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVZPQILo2PQIrr, "vmovq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVLQ128mr, "vmovq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVPQI2QImr, "vmovq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVSDto64mr, "vmovq", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOV64toSDrm, "vmovq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVQI2PQIrm, "vmovq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVZPQILo2PQIrm, "vmovq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVZQI2PQIrm, "vmovq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVSDmr, "vmovsd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVSDrm, "vmovsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVSDrr, "vmovsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVSHDUPrr, "vmovshdup", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVSHDUPYrr, "vmovshdup", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVSHDUPrm, "vmovshdup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVSHDUPYrm, "vmovshdup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVSLDUPrr, "vmovsldup", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVSLDUPYrr, "vmovsldup", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVSLDUPrm, "vmovsldup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVSLDUPYrm, "vmovsldup", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVSSmr, "vmovss", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVSSrm, "vmovss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVSSrr, "vmovss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVUPDrr, "vmovupd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVUPDmr, "vmovupd", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVUPDYrr, "vmovupd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVUPDYmr, "vmovupd", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVUPDrm, "vmovupd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVUPDYrm, "vmovupd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMOVUPSrr, "vmovups", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VMOVUPSmr, "vmovups", Convert__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem }, 0},
  { X86::VMOVUPSYrr, "vmovups", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VMOVUPSYmr, "vmovups", Convert__Mem5_1__Reg1_0, { MCK_VR256, MCK_Mem }, 0},
  { X86::VMOVUPSrm, "vmovups", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VMOVUPSYrm, "vmovups", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VMPSADBWrri, "vmpsadbw", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMPSADBWrmi, "vmpsadbw", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMPTRLDm, "vmptrld", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VMPTRSTm, "vmptrst", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VMREAD32rr, "vmreadl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::VMREAD32rm, "vmreadl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::VMREAD64rr, "vmreadq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::VMREAD64rm, "vmreadq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::VMRESUME, "vmresume", Convert, {  }, 0},
  { X86::VMULPDrr, "vmulpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULPDYrr, "vmulpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMULPDrm, "vmulpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULPDYrm, "vmulpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMULPSrr, "vmulps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULPSYrr, "vmulps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMULPSrm, "vmulps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULPSYrm, "vmulps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VMULSDrr, "vmulsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULSDrm, "vmulsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULSSrr, "vmulss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMULSSrm, "vmulss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VMWRITE32rr, "vmwritel", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::VMWRITE32rm, "vmwritel", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::VMWRITE64rr, "vmwriteq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::VMWRITE64rm, "vmwriteq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::VMXOFF, "vmxoff", Convert, {  }, 0},
  { X86::VMXON, "vmxon", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VFsORPDrr, "vorpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPDrr, "vorpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPDYrr, "vorpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsORPDrm, "vorpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPDrm, "vorpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPDYrm, "vorpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsORPSrr, "vorps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPSrr, "vorps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPSYrr, "vorps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsORPSrm, "vorps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPSrm, "vorps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VORPSYrm, "vorps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPABSBrr128, "vpabsb", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPABSBrm128, "vpabsb", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPABSDrr128, "vpabsd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPABSDrm128, "vpabsd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPABSWrr128, "vpabsw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPABSWrm128, "vpabsw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPACKSSDWrr, "vpackssdw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKSSDWrm, "vpackssdw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKSSWBrr, "vpacksswb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKSSWBrm, "vpacksswb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKUSDWrr, "vpackusdw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKUSDWrm, "vpackusdw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKUSWBrr, "vpackuswb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPACKUSWBrm, "vpackuswb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDBrr, "vpaddb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDBrm, "vpaddb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDDrr, "vpaddd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDDrm, "vpaddd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDQrr, "vpaddq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDQrm, "vpaddq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDSBrr, "vpaddsb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDSBrm, "vpaddsb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDSWrr, "vpaddsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDSWrm, "vpaddsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDUSBrr, "vpaddusb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDUSBrm, "vpaddusb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDUSWrr, "vpaddusw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDUSWrm, "vpaddusw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDWrr, "vpaddw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPADDWrm, "vpaddw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPALIGNR128rr, "vpalignr", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPALIGNR128rm, "vpalignr", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPANDrr, "vpand", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPANDrm, "vpand", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPANDNrr, "vpandn", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPANDNrm, "vpandn", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPAVGBrr, "vpavgb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPAVGBrm, "vpavgb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPAVGWrr, "vpavgw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPAVGWrm, "vpavgw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPBLENDVBrr, "vpblendvb", Convert__Reg1_3__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPBLENDVBrm, "vpblendvb", Convert__Reg1_3__Reg1_2__Mem5_1__Reg1_0, { MCK_FR32, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPBLENDWrri, "vpblendw", Convert__Reg1_3__Reg1_2__Reg1_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPBLENDWrmi, "vpblendw", Convert__Reg1_3__Reg1_2__Mem5_1__ImmZExtu32u81_0, { MCK_ImmZExtu32u8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrr, "vpclmulhqhqdq", Convert__Reg1_2__Reg1_1__Reg1_0__imm17, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrm, "vpclmulhqhqdq", Convert__Reg1_2__Reg1_1__Mem5_0__imm17, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrr, "vpclmulhqlqdq", Convert__Reg1_2__Reg1_1__Reg1_0__imm1, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrm, "vpclmulhqlqdq", Convert__Reg1_2__Reg1_1__Mem5_0__imm1, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrr, "vpclmullqhqdq", Convert__Reg1_2__Reg1_1__Reg1_0__imm16, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrm, "vpclmullqhqdq", Convert__Reg1_2__Reg1_1__Mem5_0__imm16, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrr, "vpclmullqlqdq", Convert__Reg1_2__Reg1_1__Reg1_0__imm0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrm, "vpclmullqlqdq", Convert__Reg1_2__Reg1_1__Mem5_0__imm0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrr, "vpclmulqdq", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCLMULQDQrm, "vpclmulqdq", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQBrr, "vpcmpeqb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQBrm, "vpcmpeqb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQDrr, "vpcmpeqd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQDrm, "vpcmpeqd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQQrr, "vpcmpeqq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQQrm, "vpcmpeqq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQWrr, "vpcmpeqw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPEQWrm, "vpcmpeqw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRIArr, "vpcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRICrr, "vpcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRIOrr, "vpcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRISrr, "vpcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRIZrr, "vpcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRIrr, "vpcmpestri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRIArm, "vpcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPESTRICrm, "vpcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPESTRIOrm, "vpcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPESTRISrm, "vpcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPESTRIZrm, "vpcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPESTRIrm, "vpcmpestri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPESTRM128rr, "vpcmpestrm", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPESTRM128rm, "vpcmpestrm", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPGTBrr, "vpcmpgtb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTBrm, "vpcmpgtb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTDrr, "vpcmpgtd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTDrm, "vpcmpgtd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTQrr, "vpcmpgtq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTQrm, "vpcmpgtq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTWrr, "vpcmpgtw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPGTWrm, "vpcmpgtw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRIArr, "vpcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRICrr, "vpcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRIOrr, "vpcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRISrr, "vpcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRIZrr, "vpcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRIrr, "vpcmpistri", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRIArm, "vpcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPISTRICrm, "vpcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPISTRIOrm, "vpcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPISTRISrm, "vpcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPISTRIZrm, "vpcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPISTRIrm, "vpcmpistri", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPCMPISTRM128rr, "vpcmpistrm", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPCMPISTRM128rm, "vpcmpistrm", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPERM2F128rr, "vperm2f128", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERM2F128rm, "vperm2f128", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERMILPDrr, "vpermilpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPERMILPDYrr, "vpermilpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERMILPDri, "vpermilpd", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPERMILPDYri, "vpermilpd", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERMILPDmi, "vpermilpd", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPERMILPDYmi, "vpermilpd", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256 }, 0},
  { X86::VPERMILPDrm, "vpermilpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPERMILPDYrm, "vpermilpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERMILPSrr, "vpermilps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPERMILPSYrr, "vpermilps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERMILPSri, "vpermilps", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPERMILPSYri, "vpermilps", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPERMILPSmi, "vpermilps", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPERMILPSYmi, "vpermilps", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256 }, 0},
  { X86::VPERMILPSrm, "vpermilps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPERMILPSYrm, "vpermilps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VPEXTRBrr, "vpextrb", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::VPEXTRBrr64, "vpextrb", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR64 }, 0},
  { X86::VPEXTRBmr, "vpextrb", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::VPEXTRDrr, "vpextrd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::VPEXTRDmr, "vpextrd", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::VPEXTRQrr, "vpextrq", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR64 }, 0},
  { X86::VPEXTRQmr, "vpextrq", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::VPEXTRWri, "vpextrw", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_GR32 }, 0},
  { X86::VPEXTRWmr, "vpextrw", Convert__Mem5_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_Mem }, 0},
  { X86::VPHADDDrr128, "vphaddd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHADDDrm128, "vphaddd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHADDSWrr128, "vphaddsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHADDSWrm128, "vphaddsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHADDWrr128, "vphaddw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHADDWrm128, "vphaddw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHMINPOSUWrr128, "vphminposuw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHMINPOSUWrm128, "vphminposuw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPHSUBDrr128, "vphsubd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHSUBDrm128, "vphsubd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHSUBSWrr128, "vphsubsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHSUBSWrm128, "vphsubsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHSUBWrr128, "vphsubw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPHSUBWrm128, "vphsubw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRBrr, "vpinsrb", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRBrm, "vpinsrb", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRDrr, "vpinsrd", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRDrm, "vpinsrd", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRQrr, "vpinsrq", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR64, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRQrm, "vpinsrq", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRWrri, "vpinsrw", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRWrr64i, "vpinsrw", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR64, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPINSRWrmi, "vpinsrw", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMADDUBSWrr128, "vpmaddubsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMADDUBSWrm128, "vpmaddubsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMADDWDrr, "vpmaddwd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMADDWDrm, "vpmaddwd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXSBrr, "vpmaxsb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXSBrm, "vpmaxsb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXSDrr, "vpmaxsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXSDrm, "vpmaxsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXSWrr, "vpmaxsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXSWrm, "vpmaxsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXUBrr, "vpmaxub", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXUBrm, "vpmaxub", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXUDrr, "vpmaxud", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXUDrm, "vpmaxud", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXUWrr, "vpmaxuw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMAXUWrm, "vpmaxuw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINSBrr, "vpminsb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINSBrm, "vpminsb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINSDrr, "vpminsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINSDrm, "vpminsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINSWrr, "vpminsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINSWrm, "vpminsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINUBrr, "vpminub", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINUBrm, "vpminub", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINUDrr, "vpminud", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINUDrm, "vpminud", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINUWrr, "vpminuw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMINUWrm, "vpminuw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVMSKBrr, "vpmovmskb", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR32 }, 0},
  { X86::VPMOVMSKBr64r, "vpmovmskb", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_GR64 }, 0},
  { X86::VPMOVSXBDrr, "vpmovsxbd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVSXBDrm, "vpmovsxbd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVSXBQrr, "vpmovsxbq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVSXBQrm, "vpmovsxbq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVSXBWrr, "vpmovsxbw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVSXBWrm, "vpmovsxbw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVSXDQrr, "vpmovsxdq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVSXDQrm, "vpmovsxdq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVSXWDrr, "vpmovsxwd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVSXWDrm, "vpmovsxwd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVSXWQrr, "vpmovsxwq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVSXWQrm, "vpmovsxwq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVZXBDrr, "vpmovzxbd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVZXBDrm, "vpmovzxbd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVZXBQrr, "vpmovzxbq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVZXBQrm, "vpmovzxbq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVZXBWrr, "vpmovzxbw", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVZXBWrm, "vpmovzxbw", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVZXDQrr, "vpmovzxdq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVZXDQrm, "vpmovzxdq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVZXWDrr, "vpmovzxwd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVZXWDrm, "vpmovzxwd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMOVZXWQrr, "vpmovzxwq", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMOVZXWQrm, "vpmovzxwq", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPMULDQrr, "vpmuldq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULDQrm, "vpmuldq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULHRSWrr128, "vpmulhrsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULHRSWrm128, "vpmulhrsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULHUWrr, "vpmulhuw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULHUWrm, "vpmulhuw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULHWrr, "vpmulhw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULHWrm, "vpmulhw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULLDrr, "vpmulld", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULLDrm, "vpmulld", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULLWrr, "vpmullw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULLWrm, "vpmullw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULUDQrr, "vpmuludq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPMULUDQrm, "vpmuludq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPORrr, "vpor", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPORrm, "vpor", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSADBWrr, "vpsadbw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSADBWrm, "vpsadbw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSHUFBrr128, "vpshufb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSHUFBrm128, "vpshufb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSHUFDri, "vpshufd", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSHUFDmi, "vpshufd", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPSHUFHWri, "vpshufhw", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSHUFHWmi, "vpshufhw", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPSHUFLWri, "vpshuflw", Convert__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSHUFLWmi, "vpshuflw", Convert__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32 }, 0},
  { X86::VPSIGNBrr128, "vpsignb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSIGNBrm128, "vpsignb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSIGNDrr128, "vpsignd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSIGNDrm128, "vpsignd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSIGNWrr128, "vpsignw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSIGNWrm128, "vpsignw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLDrr, "vpslld", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLDri, "vpslld", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLDrm, "vpslld", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLDQri, "vpslldq", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLQrr, "vpsllq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLQri, "vpsllq", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLQrm, "vpsllq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLWrr, "vpsllw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLWri, "vpsllw", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSLLWrm, "vpsllw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRADrr, "vpsrad", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRADri, "vpsrad", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRADrm, "vpsrad", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRAWrr, "vpsraw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRAWri, "vpsraw", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRAWrm, "vpsraw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLDrr, "vpsrld", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLDri, "vpsrld", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLDrm, "vpsrld", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLDQri, "vpsrldq", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLQrr, "vpsrlq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLQri, "vpsrlq", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLQrm, "vpsrlq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLWrr, "vpsrlw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLWri, "vpsrlw", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSRLWrm, "vpsrlw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBBrr, "vpsubb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBBrm, "vpsubb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBDrr, "vpsubd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBDrm, "vpsubd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBQrr, "vpsubq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBQrm, "vpsubq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBSBrr, "vpsubsb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBSBrm, "vpsubsb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBSWrr, "vpsubsw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBSWrm, "vpsubsw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBUSBrr, "vpsubusb", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBUSBrm, "vpsubusb", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBUSWrr, "vpsubusw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBUSWrm, "vpsubusw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBWrr, "vpsubw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPSUBWrm, "vpsubw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPTESTrr, "vptest", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VPTESTYrr, "vptest", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VPTESTrm, "vptest", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VPTESTYrm, "vptest", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VPUNPCKHBWrr, "vpunpckhbw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHBWrm, "vpunpckhbw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHDQrr, "vpunpckhdq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHDQrm, "vpunpckhdq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHQDQrr, "vpunpckhqdq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHQDQrm, "vpunpckhqdq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHWDrr, "vpunpckhwd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKHWDrm, "vpunpckhwd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLBWrr, "vpunpcklbw", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLBWrm, "vpunpcklbw", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLDQrr, "vpunpckldq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLDQrm, "vpunpckldq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLQDQrr, "vpunpcklqdq", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLQDQrm, "vpunpcklqdq", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLWDrr, "vpunpcklwd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPUNPCKLWDrm, "vpunpcklwd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPXORrr, "vpxor", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VPXORrm, "vpxor", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VRCPPSr, "vrcpps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VRCPPSYr, "vrcpps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VRCPPSm, "vrcpps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VRCPPSYm, "vrcpps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VRCPSSr, "vrcpss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VRCPSSm, "vrcpss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDPDr, "vroundpd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDPDr_AVX, "vroundpd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDYPDr, "vroundpd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR256, MCK_VR256 }, 0},
  { X86::VROUNDYPDr_AVX, "vroundpd", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR256, MCK_VR256 }, 0},
  { X86::VROUNDPDm, "vroundpd", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::VROUNDPDm_AVX, "vroundpd", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::VROUNDYPDm, "vroundpd", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_VR256 }, 0},
  { X86::VROUNDYPDm_AVX, "vroundpd", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_VR256 }, 0},
  { X86::VROUNDPSr, "vroundps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDPSr_AVX, "vroundps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDYPSr, "vroundps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR256, MCK_VR256 }, 0},
  { X86::VROUNDYPSr_AVX, "vroundps", Convert__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_VR256, MCK_VR256 }, 0},
  { X86::VROUNDPSm, "vroundps", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::VROUNDPSm_AVX, "vroundps", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32 }, 0},
  { X86::VROUNDYPSm, "vroundps", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_VR256 }, 0},
  { X86::VROUNDYPSm_AVX, "vroundps", Convert__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_VR256 }, 0},
  { X86::VROUNDSDr, "vroundsd", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSDr_AVX, "vroundsd", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSDm, "vroundsd", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSDm_AVX, "vroundsd", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSSr, "vroundss", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSSr_AVX, "vroundss", Convert__Reg1_3__Reg1_2__Reg1_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSSm, "vroundss", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VROUNDSSm_AVX, "vroundss", Convert__Reg1_3__Reg1_2__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VRSQRTPSr, "vrsqrtps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VRSQRTPSYr, "vrsqrtps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VRSQRTPSm, "vrsqrtps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VRSQRTPSYm, "vrsqrtps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VRSQRTSSr, "vrsqrtss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VRSQRTSSm, "vrsqrtss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSHUFPDrri, "vshufpd", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSHUFPDYrri, "vshufpd", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSHUFPDrmi, "vshufpd", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSHUFPDYrmi, "vshufpd", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSHUFPSrri, "vshufps", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSHUFPSYrri, "vshufps", Convert__Reg1_3__Reg1_2__Reg1_1__Imm1_0, { MCK_Imm, MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSHUFPSrmi, "vshufps", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSHUFPSYrmi, "vshufps", Convert__Reg1_3__Reg1_2__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSQRTPDr, "vsqrtpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VSQRTPDYr, "vsqrtpd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VSQRTPDm, "vsqrtpd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VSQRTPDYm, "vsqrtpd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VSQRTPSr, "vsqrtps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VSQRTPSYr, "vsqrtps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VSQRTPSm, "vsqrtps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VSQRTPSYm, "vsqrtps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VSQRTSDr, "vsqrtsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSQRTSDm, "vsqrtsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSQRTSSr, "vsqrtss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSQRTSSm, "vsqrtss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSTMXCSR, "vstmxcsr", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::VSUBPDrr, "vsubpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBPDYrr, "vsubpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSUBPDrm, "vsubpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBPDYrm, "vsubpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSUBPSrr, "vsubps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBPSYrr, "vsubps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSUBPSrm, "vsubps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBPSYrm, "vsubps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VSUBSDrr, "vsubsd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBSDrm, "vsubsd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBSSrr, "vsubss", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VSUBSSrm, "vsubss", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VTESTPDrr, "vtestpd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VTESTPDYrr, "vtestpd", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VTESTPDrm, "vtestpd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VTESTPDYrm, "vtestpd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VTESTPSrr, "vtestps", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VTESTPSYrr, "vtestps", Convert__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256 }, 0},
  { X86::VTESTPSrm, "vtestps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VTESTPSYrm, "vtestps", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256 }, 0},
  { X86::VUCOMISDrr, "vucomisd", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VUCOMISDrm, "vucomisd", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VUCOMISSrr, "vucomiss", Convert__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::VUCOMISSrm, "vucomiss", Convert__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::VUNPCKHPDrr, "vunpckhpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKHPDYrr, "vunpckhpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKHPDrm, "vunpckhpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKHPDYrm, "vunpckhpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKHPSrr, "vunpckhps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKHPSYrr, "vunpckhps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKHPSrm, "vunpckhps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKHPSYrm, "vunpckhps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKLPDrr, "vunpcklpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKLPDYrr, "vunpcklpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKLPDrm, "vunpcklpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKLPDYrm, "vunpcklpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKLPSrr, "vunpcklps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKLPSYrr, "vunpcklps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VUNPCKLPSrm, "vunpcklps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VUNPCKLPSYrm, "vunpcklps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsXORPDrr, "vxorpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPDrr, "vxorpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPDYrr, "vxorpd", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsXORPDrm, "vxorpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPDrm, "vxorpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPDYrm, "vxorpd", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsXORPSrr, "vxorps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPSrr, "vxorps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_FR32, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPSYrr, "vxorps", Convert__Reg1_2__Reg1_1__Reg1_0, { MCK_VR256, MCK_VR256, MCK_VR256 }, 0},
  { X86::VFsXORPSrm, "vxorps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPSrm, "vxorps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_FR32, MCK_FR32 }, 0},
  { X86::VXORPSYrm, "vxorps", Convert__Reg1_2__Reg1_1__Mem5_0, { MCK_Mem, MCK_VR256, MCK_VR256 }, 0},
  { X86::VZEROALL, "vzeroall", Convert, {  }, 0},
  { X86::VZEROUPPER, "vzeroupper", Convert, {  }, 0},
  { X86::WAIT, "wait", Convert, {  }, 0},
  { X86::WBINVD, "wbinvd", Convert, {  }, 0},
  { X86::WRFSBASE, "wrfsbasel", Convert__Reg1_0, { MCK_GR32 }, Feature_In64BitMode},
  { X86::WRFSBASE64, "wrfsbaseq", Convert__Reg1_0, { MCK_GR64 }, Feature_In64BitMode},
  { X86::WRGSBASE, "wrgsbasel", Convert__Reg1_0, { MCK_GR32 }, Feature_In64BitMode},
  { X86::WRGSBASE64, "wrgsbaseq", Convert__Reg1_0, { MCK_GR64 }, Feature_In64BitMode},
  { X86::WRMSR, "wrmsr", Convert, {  }, 0},
  { X86::XADD8rr, "xaddb", Convert__Reg1_1__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::XADD8rm, "xaddb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::XADD32rr, "xaddl", Convert__Reg1_1__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::XADD32rm, "xaddl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::XADD64rr, "xaddq", Convert__Reg1_1__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::XADD64rm, "xaddq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::XADD16rr, "xaddw", Convert__Reg1_1__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::XADD16rm, "xaddw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::XCHG8rr, "xchgb", Convert__Reg1_0__Tie0__Reg1_1, { MCK_GR8, MCK_GR8 }, 0},
  { X86::XCHG8rm, "xchgb", Convert__Reg1_0__Tie0__Mem5_1, { MCK_GR8, MCK_Mem }, 0},
  { X86::XCHG8rm, "xchgb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::XCHG32ar64, "xchgl", Convert__Reg1_1, { MCK_EAX, MCK_GR32_NOAX }, Feature_In64BitMode},
  { X86::XCHG32ar, "xchgl", Convert__Reg1_1, { MCK_EAX, MCK_GR32 }, Feature_In32BitMode},
  { X86::XCHG32ar64, "xchgl", Convert__Reg1_0, { MCK_GR32_NOAX, MCK_EAX }, Feature_In64BitMode},
  { X86::XCHG32ar, "xchgl", Convert__Reg1_0, { MCK_GR32, MCK_EAX }, Feature_In32BitMode},
  { X86::XCHG32rr, "xchgl", Convert__Reg1_0__Tie0__Reg1_1, { MCK_GR32, MCK_GR32 }, 0},
  { X86::XCHG32rm, "xchgl", Convert__Reg1_0__Tie0__Mem5_1, { MCK_GR32, MCK_Mem }, 0},
  { X86::XCHG32rm, "xchgl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::XCHG64ar, "xchgq", Convert__Reg1_1, { MCK_RAX, MCK_GR64 }, 0},
  { X86::XCHG64ar, "xchgq", Convert__Reg1_0, { MCK_GR64, MCK_RAX }, 0},
  { X86::XCHG64rr, "xchgq", Convert__Reg1_0__Tie0__Reg1_1, { MCK_GR64, MCK_GR64 }, 0},
  { X86::XCHG64rm, "xchgq", Convert__Reg1_0__Tie0__Mem5_1, { MCK_GR64, MCK_Mem }, 0},
  { X86::XCHG64rm, "xchgq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::XCHG16ar, "xchgw", Convert__Reg1_1, { MCK_AX, MCK_GR16 }, 0},
  { X86::XCHG16ar, "xchgw", Convert__Reg1_0, { MCK_GR16, MCK_AX }, 0},
  { X86::XCHG16rr, "xchgw", Convert__Reg1_0__Tie0__Reg1_1, { MCK_GR16, MCK_GR16 }, 0},
  { X86::XCHG16rm, "xchgw", Convert__Reg1_0__Tie0__Mem5_1, { MCK_GR16, MCK_Mem }, 0},
  { X86::XCHG16rm, "xchgw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::XCRYPTCBC, "xcryptcbc", Convert, {  }, 0},
  { X86::XCRYPTCFB, "xcryptcfb", Convert, {  }, 0},
  { X86::XCRYPTCTR, "xcryptctr", Convert, {  }, 0},
  { X86::XCRYPTECB, "xcryptecb", Convert, {  }, 0},
  { X86::XCRYPTOFB, "xcryptofb", Convert, {  }, 0},
  { X86::XGETBV, "xgetbv", Convert, {  }, 0},
  { X86::XLAT, "xlatb", Convert, {  }, 0},
  { X86::XOR8rr, "xorb", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR8, MCK_GR8 }, 0},
  { X86::XOR8mr, "xorb", Convert__Mem5_1__Reg1_0, { MCK_GR8, MCK_Mem }, 0},
  { X86::XOR8i8, "xorb", Convert__Imm1_0, { MCK_Imm, MCK_AL }, 0},
  { X86::XOR8ri, "xorb", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR8 }, 0},
  { X86::XOR8mi, "xorb", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::XOR8rm, "xorb", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR8 }, 0},
  { X86::XOR32rr, "xorl", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR32, MCK_GR32 }, 0},
  { X86::XOR32mr, "xorl", Convert__Mem5_1__Reg1_0, { MCK_GR32, MCK_Mem }, 0},
  { X86::XOR32ri8, "xorl", Convert__Reg1_1__Tie0__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_GR32 }, 0},
  { X86::XOR32mi8, "xorl", Convert__Mem5_1__ImmSExti32i81_0, { MCK_ImmSExti32i8, MCK_Mem }, 0},
  { X86::XOR32i32, "xorl", Convert__Imm1_0, { MCK_Imm, MCK_EAX }, 0},
  { X86::XOR32ri, "xorl", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR32 }, 0},
  { X86::XOR32mi, "xorl", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::XOR32rm, "xorl", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR32 }, 0},
  { X86::FsXORPDrr, "xorpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::XORPDrr, "xorpd", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsXORPDrm, "xorpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::XORPDrm, "xorpd", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::FsXORPSrr, "xorps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::XORPSrr, "xorps", Convert__Reg1_1__Tie0__Reg1_0, { MCK_FR32, MCK_FR32 }, 0},
  { X86::FsXORPSrm, "xorps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::XORPSrm, "xorps", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_FR32 }, 0},
  { X86::XOR64rr, "xorq", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR64, MCK_GR64 }, 0},
  { X86::XOR64mr, "xorq", Convert__Mem5_1__Reg1_0, { MCK_GR64, MCK_Mem }, 0},
  { X86::XOR64ri8, "xorq", Convert__Reg1_1__Tie0__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_GR64 }, 0},
  { X86::XOR64mi8, "xorq", Convert__Mem5_1__ImmSExti64i81_0, { MCK_ImmSExti64i8, MCK_Mem }, 0},
  { X86::XOR64i32, "xorq", Convert__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_RAX }, 0},
  { X86::XOR64ri32, "xorq", Convert__Reg1_1__Tie0__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_GR64 }, 0},
  { X86::XOR64mi32, "xorq", Convert__Mem5_1__ImmSExti64i321_0, { MCK_ImmSExti64i32, MCK_Mem }, 0},
  { X86::XOR64rm, "xorq", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR64 }, 0},
  { X86::XOR16rr, "xorw", Convert__Reg1_1__Tie0__Reg1_0, { MCK_GR16, MCK_GR16 }, 0},
  { X86::XOR16mr, "xorw", Convert__Mem5_1__Reg1_0, { MCK_GR16, MCK_Mem }, 0},
  { X86::XOR16ri8, "xorw", Convert__Reg1_1__Tie0__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_GR16 }, 0},
  { X86::XOR16mi8, "xorw", Convert__Mem5_1__ImmSExti16i81_0, { MCK_ImmSExti16i8, MCK_Mem }, 0},
  { X86::XOR16i16, "xorw", Convert__Imm1_0, { MCK_Imm, MCK_AX }, 0},
  { X86::XOR16ri, "xorw", Convert__Reg1_1__Tie0__Imm1_0, { MCK_Imm, MCK_GR16 }, 0},
  { X86::XOR16mi, "xorw", Convert__Mem5_1__Imm1_0, { MCK_Imm, MCK_Mem }, 0},
  { X86::XOR16rm, "xorw", Convert__Reg1_1__Tie0__Mem5_0, { MCK_Mem, MCK_GR16 }, 0},
  { X86::XRSTOR, "xrstor", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::XRSTOR64, "xrstorq", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::XSAVE, "xsave", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::XSAVEOPT, "xsaveopt", Convert__Mem5_0, { MCK_Mem }, 0},
  { X86::XSAVEOPT64, "xsaveoptq", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::XSAVE64, "xsaveq", Convert__Mem5_0, { MCK_Mem }, Feature_In64BitMode},
  { X86::XSETBV, "xsetbv", Convert, {  }, 0},
  { X86::XSHA1, "xsha1", Convert, {  }, 0},
  { X86::XSHA256, "xsha256", Convert, {  }, 0},
  { X86::XSTORE, "xstore", Convert, {  }, 0},
  { X86::XSTORE, "xstorerng", Convert, {  }, 0},
};

bool X86ATTAsmParser::
MnemonicIsValid(StringRef Mnemonic) {
  // Search the table.
  std::pair<const MatchEntry*, const MatchEntry*> MnemonicRange =
    std::equal_range(MatchTable, MatchTable+3300, Mnemonic, LessOpcode());
  return MnemonicRange.first != MnemonicRange.second;
}

unsigned X86ATTAsmParser::
MatchInstructionImpl(const SmallVectorImpl<MCParsedAsmOperand*> &Operands,
                     MCInst &Inst, unsigned &ErrorInfo) {
  // Get the current feature set.
  unsigned AvailableFeatures = getAvailableFeatures();

  // Get the instruction mnemonic, which is the first token.
  StringRef Mnemonic = ((X86Operand*)Operands[0])->getToken();

  // Process all MnemonicAliases to remap the mnemonic.
  ApplyMnemonicAliases(Mnemonic, AvailableFeatures);

  // Eliminate obvious mismatches.
  if (Operands.size() > 6) {
    ErrorInfo = 6;
    return Match_InvalidOperand;
  }

  // Some state to try to produce better error messages.
  bool HadMatchOtherThanFeatures = false;
  bool HadMatchOtherThanPredicate = false;
  unsigned RetCode = Match_InvalidOperand;
  // Set ErrorInfo to the operand that mismatches if it is
  // wrong for all instances of the instruction.
  ErrorInfo = ~0U;
  // Search the table.
  std::pair<const MatchEntry*, const MatchEntry*> MnemonicRange =
    std::equal_range(MatchTable, MatchTable+3300, Mnemonic, LessOpcode());

  // Return a more specific error code if no mnemonics match.
  if (MnemonicRange.first == MnemonicRange.second)
    return Match_MnemonicFail;

  for (const MatchEntry *it = MnemonicRange.first, *ie = MnemonicRange.second;
       it != ie; ++it) {
    // equal_range guarantees that instruction mnemonic matches.
    assert(Mnemonic == it->Mnemonic);
    bool OperandsValid = true;
    for (unsigned i = 0; i != 5; ++i) {
      if (i + 1 >= Operands.size()) {
        OperandsValid = (it->Classes[i] == InvalidMatchClass);
        break;
      }
      if (ValidateOperandClass(Operands[i+1], it->Classes[i]))
        continue;
      // If this operand is broken for all of the instances of this
      // mnemonic, keep track of it so we can report loc info.
      if (it == MnemonicRange.first || ErrorInfo <= i+1)
        ErrorInfo = i+1;
      // Otherwise, just reject this instance of the mnemonic.
      OperandsValid = false;
      break;
    }

    if (!OperandsValid) continue;
    if ((AvailableFeatures & it->RequiredFeatures) != it->RequiredFeatures) {
      HadMatchOtherThanFeatures = true;
      continue;
    }

    // We have selected a definite instruction, convert the parsed
    // operands into the appropriate MCInst.
    if (!ConvertToMCInst(it->ConvertFn, Inst,
                         it->Opcode, Operands))
      return Match_ConversionFail;

    // We have a potential match. Check the target predicate to
    // handle any context sensitive constraints.
    unsigned MatchResult;
    if ((MatchResult = checkTargetMatchPredicate(Inst)) != Match_Success) {
      Inst.clear();
      RetCode = MatchResult;
      HadMatchOtherThanPredicate = true;
      continue;
    }

    return Match_Success;
  }

  // Okay, we had no match.  Try to return a useful error code.
  if (HadMatchOtherThanPredicate || !HadMatchOtherThanFeatures) return RetCode;
  return Match_MissingFeature;
}

#endif // GET_MATCHER_IMPLEMENTATION

