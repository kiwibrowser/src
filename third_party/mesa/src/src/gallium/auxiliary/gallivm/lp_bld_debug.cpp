/**************************************************************************
 *
 * Copyright 2009-2011 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <stddef.h>

#include <llvm-c/Core.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetInstrInfo.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryObject.h>

#if HAVE_LLVM >= 0x0300
#include <llvm/Support/TargetRegistry.h>
#else /* HAVE_LLVM < 0x0300 */
#include <llvm/Target/TargetRegistry.h>
#endif /* HAVE_LLVM < 0x0300 */

#if HAVE_LLVM >= 0x0209
#include <llvm/Support/Host.h>
#else /* HAVE_LLVM < 0x0209 */
#include <llvm/System/Host.h>
#endif /* HAVE_LLVM < 0x0209 */

#if HAVE_LLVM >= 0x0207
#include <llvm/MC/MCDisassembler.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#endif /* HAVE_LLVM >= 0x0207 */
#if HAVE_LLVM >= 0x0301
#include <llvm/MC/MCRegisterInfo.h>
#endif /* HAVE_LLVM >= 0x0301 */

#include "util/u_math.h"
#include "util/u_debug.h"

#include "lp_bld_debug.h"



/**
 * Check alignment.
 *
 * It is important that this check is not implemented as a macro or inlined
 * function, as the compiler assumptions in respect to alignment of global
 * and stack variables would often make the check a no op, defeating the
 * whole purpose of the exercise.
 */
extern "C" boolean
lp_check_alignment(const void *ptr, unsigned alignment)
{
   assert(util_is_power_of_two(alignment));
   return ((uintptr_t)ptr & (alignment - 1)) == 0;
}


class raw_debug_ostream :
   public llvm::raw_ostream
{
private:
   uint64_t pos;

public:
   raw_debug_ostream() : pos(0) { }

   void write_impl(const char *Ptr, size_t Size);

#if HAVE_LLVM >= 0x207
   uint64_t current_pos() const { return pos; }
   size_t preferred_buffer_size() const { return 512; }
#else
   uint64_t current_pos() { return pos; }
   size_t preferred_buffer_size() { return 512; }
#endif
};


void
raw_debug_ostream::write_impl(const char *Ptr, size_t Size)
{
   if (Size > 0) {
      char *lastPtr = (char *)&Ptr[Size];
      char last = *lastPtr;
      *lastPtr = 0;
      _debug_printf("%*s", Size, Ptr);
      *lastPtr = last;
      pos += Size;
   }
}


/**
 * Same as LLVMDumpValue, but through our debugging channels.
 */
extern "C" void
lp_debug_dump_value(LLVMValueRef value)
{
#if (defined(PIPE_OS_WINDOWS) && !defined(PIPE_CC_MSVC)) || defined(PIPE_OS_EMBDDED)
   raw_debug_ostream os;
   llvm::unwrap(value)->print(os);
   os.flush();
#else
   LLVMDumpValue(value);
#endif
}


#if HAVE_LLVM >= 0x0207
/*
 * MemoryObject wrapper around a buffer of memory, to be used by MC
 * disassembler.
 */
class BufferMemoryObject:
   public llvm::MemoryObject
{
private:
   const uint8_t *Bytes;
   uint64_t Length;
public:
   BufferMemoryObject(const uint8_t *bytes, uint64_t length) :
      Bytes(bytes), Length(length)
   {
   }

   uint64_t getBase() const
   {
      return 0;
   }

   uint64_t getExtent() const
   {
      return Length;
   }

   int readByte(uint64_t addr, uint8_t *byte) const
   {
      if (addr > getExtent())
         return -1;
      *byte = Bytes[addr];
      return 0;
   }
};
#endif /* HAVE_LLVM >= 0x0207 */


/*
 * Disassemble a function, using the LLVM MC disassembler.
 *
 * See also:
 * - http://blog.llvm.org/2010/01/x86-disassembler.html
 * - http://blog.llvm.org/2010/04/intro-to-llvm-mc-project.html
 */
extern "C" void
lp_disassemble(const void* func)
{
#if HAVE_LLVM >= 0x0207
   using namespace llvm;

   const uint8_t *bytes = (const uint8_t *)func;

   /*
    * Limit disassembly to this extent
    */
   const uint64_t extent = 96 * 1024;

   uint64_t max_pc = 0;

   /*
    * Initialize all used objects.
    */

#if HAVE_LLVM >= 0x0301
   std::string Triple = sys::getDefaultTargetTriple();
#else
   std::string Triple = sys::getHostTriple();
#endif

   std::string Error;
   const Target *T = TargetRegistry::lookupTarget(Triple, Error);

#if HAVE_LLVM >= 0x0300
   OwningPtr<const MCAsmInfo> AsmInfo(T->createMCAsmInfo(Triple));
#else
   OwningPtr<const MCAsmInfo> AsmInfo(T->createAsmInfo(Triple));
#endif

   if (!AsmInfo) {
      debug_printf("error: no assembly info for target %s\n", Triple.c_str());
      return;
   }

#if HAVE_LLVM >= 0x0300
   const MCSubtargetInfo *STI = T->createMCSubtargetInfo(Triple, sys::getHostCPUName(), "");
   OwningPtr<const MCDisassembler> DisAsm(T->createMCDisassembler(*STI));
#else 
   OwningPtr<const MCDisassembler> DisAsm(T->createMCDisassembler());
#endif 
   if (!DisAsm) {
      debug_printf("error: no disassembler for target %s\n", Triple.c_str());
      return;
   }

   raw_debug_ostream Out;

#if HAVE_LLVM >= 0x0300
   unsigned int AsmPrinterVariant = AsmInfo->getAssemblerDialect();
#else
   int AsmPrinterVariant = AsmInfo->getAssemblerDialect();
#endif

#if HAVE_LLVM >= 0x0301
   OwningPtr<const MCRegisterInfo> MRI(T->createMCRegInfo(Triple));
   if (!MRI) {
      debug_printf("error: no register info for target %s\n", Triple.c_str());
      return;
   }

   OwningPtr<const MCInstrInfo> MII(T->createMCInstrInfo());
   if (!MII) {
      debug_printf("error: no instruction info for target %s\n", Triple.c_str());
      return;
   }
#endif

#if HAVE_LLVM >= 0x0301
   OwningPtr<MCInstPrinter> Printer(
         T->createMCInstPrinter(AsmPrinterVariant, *AsmInfo, *MII, *MRI, *STI));
#elif HAVE_LLVM == 0x0300
   OwningPtr<MCInstPrinter> Printer(
         T->createMCInstPrinter(AsmPrinterVariant, *AsmInfo, *STI));
#elif HAVE_LLVM >= 0x0208
   OwningPtr<MCInstPrinter> Printer(
         T->createMCInstPrinter(AsmPrinterVariant, *AsmInfo));
#else
   OwningPtr<MCInstPrinter> Printer(
         T->createMCInstPrinter(AsmPrinterVariant, *AsmInfo, Out));
#endif
   if (!Printer) {
      debug_printf("error: no instruction printer for target %s\n", Triple.c_str());
      return;
   }

#if HAVE_LLVM >= 0x0301
   TargetOptions options;
#if defined(DEBUG)
   options.JITEmitDebugInfo = true;
#endif
#if defined(PIPE_ARCH_X86)
   options.StackAlignmentOverride = 4;
#endif
#if defined(DEBUG) || defined(PROFILE)
   options.NoFramePointerElim = true;
#endif
   TargetMachine *TM = T->createTargetMachine(Triple, sys::getHostCPUName(), "", options);
#elif HAVE_LLVM == 0x0300
   TargetMachine *TM = T->createTargetMachine(Triple, sys::getHostCPUName(), "");
#else
   TargetMachine *TM = T->createTargetMachine(Triple, "");
#endif

   const TargetInstrInfo *TII = TM->getInstrInfo();

   /*
    * Wrap the data in a MemoryObject
    */
   BufferMemoryObject memoryObject((const uint8_t *)bytes, extent);

   uint64_t pc;
   pc = 0;
   while (true) {
      MCInst Inst;
      uint64_t Size;

      /*
       * Print address.  We use addresses relative to the start of the function,
       * so that between runs.
       */

      debug_printf("%6lu:\t", (unsigned long)pc);

      if (!DisAsm->getInstruction(Inst, Size, memoryObject,
                                 pc,
#if HAVE_LLVM >= 0x0300
				  nulls(), nulls())) {
#else
				  nulls())) {
#endif
         debug_printf("invalid\n");
         pc += 1;
      }

      /*
       * Output the bytes in hexidecimal format.
       */

      if (0) {
         unsigned i;
         for (i = 0; i < Size; ++i) {
            debug_printf("%02x ", ((const uint8_t*)bytes)[pc + i]);
         }
         for (; i < 16; ++i) {
            debug_printf("   ");
         }
      }

      /*
       * Print the instruction.
       */

#if HAVE_LLVM >= 0x0300
      Printer->printInst(&Inst, Out, "");
#elif HAVE_LLVM >= 0x208
      Printer->printInst(&Inst, Out);
#else
      Printer->printInst(&Inst);
#endif
      Out.flush();

      /*
       * Advance.
       */

      pc += Size;

#if HAVE_LLVM >= 0x0300
      const MCInstrDesc &TID = TII->get(Inst.getOpcode());
#else
      const TargetInstrDesc &TID = TII->get(Inst.getOpcode());
#endif

      /*
       * Keep track of forward jumps to a nearby address.
       */

      if (TID.isBranch()) {
         for (unsigned i = 0; i < Inst.getNumOperands(); ++i) {
            const MCOperand &operand = Inst.getOperand(i);
            if (operand.isImm()) {
               uint64_t jump;

               /*
                * FIXME: Handle both relative and absolute addresses correctly.
                * EDInstInfo actually has this info, but operandTypes and
                * operandFlags enums are not exposed in the public interface.
                */

               if (1) {
                  /*
                   * PC relative addr.
                   */

                  jump = pc + operand.getImm();
               } else {
                  /*
                   * Absolute addr.
                   */

                  jump = (uint64_t)operand.getImm();
               }

               /*
                * Output the address relative to the function start, given
                * that MC will print the addresses relative the current pc.
                */
               debug_printf("\t\t; %lu", (unsigned long)jump);

               /*
                * Ignore far jumps given it could be actually a tail return to
                * a random address.
                */

               if (jump > max_pc &&
                   jump < extent) {
                  max_pc = jump;
               }
            }
         }
      }

      debug_printf("\n");

      /*
       * Stop disassembling on return statements, if there is no record of a
       * jump to a successive address.
       */

      if (TID.isReturn()) {
         if (pc > max_pc) {
            break;
         }
      }
   }

   /*
    * Print GDB command, useful to verify output.
    */

   if (0) {
      debug_printf("disassemble %p %p\n", bytes, bytes + pc);
   }

   debug_printf("\n");
#else /* HAVE_LLVM < 0x0207 */
   (void)func;
#endif /* HAVE_LLVM < 0x0207 */
}

