// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Reactor.hpp"

#include "Optimizer.hpp"

#include "src/IceTypes.h"
#include "src/IceCfg.h"
#include "src/IceELFStreamer.h"
#include "src/IceGlobalContext.h"
#include "src/IceCfgNode.h"
#include "src/IceELFObjectWriter.h"
#include "src/IceGlobalInits.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_os_ostream.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX
#include <Windows.h>
#else
#include <sys/mman.h>
#if !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

//#include <mutex>
#include <limits>
#include <iostream>
#include <cassert>

namespace
{
	Ice::GlobalContext *context = nullptr;
	Ice::Cfg *function = nullptr;
	Ice::CfgNode *basicBlock = nullptr;
	Ice::CfgLocalAllocatorScope *allocator = nullptr;
	sw::Routine *routine = nullptr;

	std::mutex codegenMutex;

	Ice::ELFFileStreamer *elfFile = nullptr;
	Ice::Fdstream *out = nullptr;
}

namespace
{
	#if !defined(__i386__) && defined(_M_IX86)
		#define __i386__ 1
	#endif

	#if !defined(__x86_64__) && (defined(_M_AMD64) || defined (_M_X64))
		#define __x86_64__ 1
	#endif

	class CPUID
	{
	public:
		const static bool ARM;
		const static bool SSE4_1;

	private:
		static void cpuid(int registers[4], int info)
		{
			#if defined(__i386__) || defined(__x86_64__)
				#if defined(_WIN32)
					__cpuid(registers, info);
				#else
					__asm volatile("cpuid": "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]): "a" (info));
				#endif
			#else
				registers[0] = 0;
				registers[1] = 0;
				registers[2] = 0;
				registers[3] = 0;
			#endif
		}

		static bool detectARM()
		{
			#if defined(__arm__)
				return true;
			#elif defined(__i386__) || defined(__x86_64__)
				return false;
			#else
				#error "Unknown architecture"
			#endif
		}

		static bool detectSSE4_1()
		{
			#if defined(__i386__) || defined(__x86_64__)
				int registers[4];
				cpuid(registers, 1);
				return (registers[2] & 0x00080000) != 0;
			#else
				return false;
			#endif
		}
	};

	const bool CPUID::ARM = CPUID::detectARM();
	const bool CPUID::SSE4_1 = CPUID::detectSSE4_1();
	const bool emulateIntrinsics = false;
	const bool emulateMismatchedBitCast = CPUID::ARM;
}

namespace sw
{
	enum EmulatedType
	{
		EmulatedShift = 16,
		EmulatedV2 = 2 << EmulatedShift,
		EmulatedV4 = 4 << EmulatedShift,
		EmulatedV8 = 8 << EmulatedShift,
		EmulatedBits = EmulatedV2 | EmulatedV4 | EmulatedV8,

		Type_v2i32 = Ice::IceType_v4i32 | EmulatedV2,
		Type_v4i16 = Ice::IceType_v8i16 | EmulatedV4,
		Type_v2i16 = Ice::IceType_v8i16 | EmulatedV2,
		Type_v8i8 =  Ice::IceType_v16i8 | EmulatedV8,
		Type_v4i8 =  Ice::IceType_v16i8 | EmulatedV4,
		Type_v2f32 = Ice::IceType_v4f32 | EmulatedV2,
	};

	class Value : public Ice::Operand {};
	class SwitchCases : public Ice::InstSwitch {};
	class BasicBlock : public Ice::CfgNode {};

	Ice::Type T(Type *t)
	{
		static_assert(static_cast<unsigned int>(Ice::IceType_NUM) < static_cast<unsigned int>(EmulatedBits), "Ice::Type overlaps with our emulated types!");
		return (Ice::Type)(reinterpret_cast<std::intptr_t>(t) & ~EmulatedBits);
	}

	Type *T(Ice::Type t)
	{
		return reinterpret_cast<Type*>(t);
	}

	Type *T(EmulatedType t)
	{
		return reinterpret_cast<Type*>(t);
	}

	Value *V(Ice::Operand *v)
	{
		return reinterpret_cast<Value*>(v);
	}

	BasicBlock *B(Ice::CfgNode *b)
	{
		return reinterpret_cast<BasicBlock*>(b);
	}

	static size_t typeSize(Type *type)
	{
		if(reinterpret_cast<std::intptr_t>(type) & EmulatedBits)
		{
			switch(reinterpret_cast<std::intptr_t>(type))
			{
			case Type_v2i32: return 8;
			case Type_v4i16: return 8;
			case Type_v2i16: return 4;
			case Type_v8i8:  return 8;
			case Type_v4i8:  return 4;
			case Type_v2f32: return 8;
			default: assert(false);
			}
		}

		return Ice::typeWidthInBytes(T(type));
	}

	Optimization optimization[10] = {InstructionCombining, Disabled};

	using ElfHeader = std::conditional<sizeof(void*) == 8, Elf64_Ehdr, Elf32_Ehdr>::type;
	using SectionHeader = std::conditional<sizeof(void*) == 8, Elf64_Shdr, Elf32_Shdr>::type;

	inline const SectionHeader *sectionHeader(const ElfHeader *elfHeader)
	{
		return reinterpret_cast<const SectionHeader*>((intptr_t)elfHeader + elfHeader->e_shoff);
	}

	inline const SectionHeader *elfSection(const ElfHeader *elfHeader, int index)
	{
		return &sectionHeader(elfHeader)[index];
	}

	static void *relocateSymbol(const ElfHeader *elfHeader, const Elf32_Rel &relocation, const SectionHeader &relocationTable)
	{
		const SectionHeader *target = elfSection(elfHeader, relocationTable.sh_info);

		intptr_t address = (intptr_t)elfHeader + target->sh_offset;
		int32_t *patchSite = (int*)(address + relocation.r_offset);
		uint32_t index = relocation.getSymbol();
		int table = relocationTable.sh_link;
		void *symbolValue = nullptr;

		if(index != SHN_UNDEF)
		{
			if(table == SHN_UNDEF) return nullptr;
			const SectionHeader *symbolTable = elfSection(elfHeader, table);

			uint32_t symtab_entries = symbolTable->sh_size / symbolTable->sh_entsize;
			if(index >= symtab_entries)
			{
				assert(index < symtab_entries && "Symbol Index out of range");
				return nullptr;
			}

			intptr_t symbolAddress = (intptr_t)elfHeader + symbolTable->sh_offset;
			Elf32_Sym &symbol = ((Elf32_Sym*)symbolAddress)[index];
			uint16_t section = symbol.st_shndx;

			if(section != SHN_UNDEF && section < SHN_LORESERVE)
			{
				const SectionHeader *target = elfSection(elfHeader, symbol.st_shndx);
				symbolValue = reinterpret_cast<void*>((intptr_t)elfHeader + symbol.st_value + target->sh_offset);
			}
			else
			{
				return nullptr;
			}
		}

		if(CPUID::ARM)
		{
			switch(relocation.getType())
			{
			case R_ARM_NONE:
				// No relocation
				break;
			case R_ARM_MOVW_ABS_NC:
				{
					uint32_t thumb = 0;   // Calls to Thumb code not supported.
					uint32_t lo = (uint32_t)(intptr_t)symbolValue | thumb;
					*patchSite = (*patchSite & 0xFFF0F000) | ((lo & 0xF000) << 4) | (lo & 0x0FFF);
				}
				break;
			case R_ARM_MOVT_ABS:
				{
					uint32_t hi = (uint32_t)(intptr_t)(symbolValue) >> 16;
					*patchSite = (*patchSite & 0xFFF0F000) | ((hi & 0xF000) << 4) | (hi & 0x0FFF);
				}
				break;
			default:
				assert(false && "Unsupported relocation type");
				return nullptr;
			}
		}
		else
		{
			switch(relocation.getType())
			{
			case R_386_NONE:
				// No relocation
				break;
			case R_386_32:
				*patchSite = (int32_t)((intptr_t)symbolValue + *patchSite);
				break;
		//	case R_386_PC32:
		//		*patchSite = (int32_t)((intptr_t)symbolValue + *patchSite - (intptr_t)patchSite);
		//		break;
			default:
				assert(false && "Unsupported relocation type");
				return nullptr;
			}
		}

		return symbolValue;
	}

	static void *relocateSymbol(const ElfHeader *elfHeader, const Elf64_Rela &relocation, const SectionHeader &relocationTable)
	{
		const SectionHeader *target = elfSection(elfHeader, relocationTable.sh_info);

		intptr_t address = (intptr_t)elfHeader + target->sh_offset;
		int32_t *patchSite = (int*)(address + relocation.r_offset);
		uint32_t index = relocation.getSymbol();
		int table = relocationTable.sh_link;
		void *symbolValue = nullptr;

		if(index != SHN_UNDEF)
		{
			if(table == SHN_UNDEF) return nullptr;
			const SectionHeader *symbolTable = elfSection(elfHeader, table);

			uint32_t symtab_entries = symbolTable->sh_size / symbolTable->sh_entsize;
			if(index >= symtab_entries)
			{
				assert(index < symtab_entries && "Symbol Index out of range");
				return nullptr;
			}

			intptr_t symbolAddress = (intptr_t)elfHeader + symbolTable->sh_offset;
			Elf64_Sym &symbol = ((Elf64_Sym*)symbolAddress)[index];
			uint16_t section = symbol.st_shndx;

			if(section != SHN_UNDEF && section < SHN_LORESERVE)
			{
				const SectionHeader *target = elfSection(elfHeader, symbol.st_shndx);
				symbolValue = reinterpret_cast<void*>((intptr_t)elfHeader + symbol.st_value + target->sh_offset);
			}
			else
			{
				return nullptr;
			}
		}

		switch(relocation.getType())
		{
		case R_X86_64_NONE:
			// No relocation
			break;
		case R_X86_64_64:
			*(int64_t*)patchSite = (int64_t)((intptr_t)symbolValue + *(int64_t*)patchSite) + relocation.r_addend;
			break;
		case R_X86_64_PC32:
			*patchSite = (int32_t)((intptr_t)symbolValue + *patchSite - (intptr_t)patchSite) + relocation.r_addend;
			break;
		case R_X86_64_32S:
			*patchSite = (int32_t)((intptr_t)symbolValue + *patchSite) + relocation.r_addend;
			break;
		default:
			assert(false && "Unsupported relocation type");
			return nullptr;
		}

		return symbolValue;
	}

	void *loadImage(uint8_t *const elfImage, size_t &codeSize)
	{
		ElfHeader *elfHeader = (ElfHeader*)elfImage;

		if(!elfHeader->checkMagic())
		{
			return nullptr;
		}

		// Expect ELF bitness to match platform
		assert(sizeof(void*) == 8 ? elfHeader->getFileClass() == ELFCLASS64 : elfHeader->getFileClass() == ELFCLASS32);
		#if defined(__i386__)
			assert(sizeof(void*) == 4 && elfHeader->e_machine == EM_386);
		#elif defined(__x86_64__)
			assert(sizeof(void*) == 8 && elfHeader->e_machine == EM_X86_64);
		#elif defined(__arm__)
			assert(sizeof(void*) == 4 && elfHeader->e_machine == EM_ARM);
		#else
			#error "Unsupported platform"
		#endif

		SectionHeader *sectionHeader = (SectionHeader*)(elfImage + elfHeader->e_shoff);
		void *entry = nullptr;

		for(int i = 0; i < elfHeader->e_shnum; i++)
		{
			if(sectionHeader[i].sh_type == SHT_PROGBITS)
			{
				if(sectionHeader[i].sh_flags & SHF_EXECINSTR)
				{
					entry = elfImage + sectionHeader[i].sh_offset;
					codeSize = sectionHeader[i].sh_size;
				}
			}
			else if(sectionHeader[i].sh_type == SHT_REL)
			{
				assert(sizeof(void*) == 4 && "UNIMPLEMENTED");   // Only expected/implemented for 32-bit code

				for(Elf32_Word index = 0; index < sectionHeader[i].sh_size / sectionHeader[i].sh_entsize; index++)
				{
					const Elf32_Rel &relocation = ((const Elf32_Rel*)(elfImage + sectionHeader[i].sh_offset))[index];
					relocateSymbol(elfHeader, relocation, sectionHeader[i]);
				}
			}
			else if(sectionHeader[i].sh_type == SHT_RELA)
			{
				assert(sizeof(void*) == 8 && "UNIMPLEMENTED");   // Only expected/implemented for 64-bit code

				for(Elf32_Word index = 0; index < sectionHeader[i].sh_size / sectionHeader[i].sh_entsize; index++)
				{
					const Elf64_Rela &relocation = ((const Elf64_Rela*)(elfImage + sectionHeader[i].sh_offset))[index];
					relocateSymbol(elfHeader, relocation, sectionHeader[i]);
				}
			}
		}

		return entry;
	}

	template<typename T>
	struct ExecutableAllocator
	{
		ExecutableAllocator() {};
		template<class U> ExecutableAllocator(const ExecutableAllocator<U> &other) {};

		using value_type = T;
		using size_type = std::size_t;

		T *allocate(size_type n)
		{
			#if defined(_WIN32)
				return (T*)VirtualAlloc(NULL, sizeof(T) * n, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			#else
				return (T*)mmap(nullptr, sizeof(T) * n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			#endif
		}

		void deallocate(T *p, size_type n)
		{
			#if defined(_WIN32)
				VirtualFree(p, 0, MEM_RELEASE);
			#else
				munmap(p, sizeof(T) * n);
			#endif
		}
	};

	class ELFMemoryStreamer : public Ice::ELFStreamer, public Routine
	{
		ELFMemoryStreamer(const ELFMemoryStreamer &) = delete;
		ELFMemoryStreamer &operator=(const ELFMemoryStreamer &) = delete;

	public:
		ELFMemoryStreamer() : Routine(), entry(nullptr)
		{
			position = 0;
			buffer.reserve(0x1000);
		}

		~ELFMemoryStreamer() override
		{
			#if defined(_WIN32)
				if(buffer.size() != 0)
				{
					DWORD exeProtection;
					VirtualProtect(&buffer[0], buffer.size(), oldProtection, &exeProtection);
				}
			#endif
		}

		void write8(uint8_t Value) override
		{
			if(position == (uint64_t)buffer.size())
			{
				buffer.push_back(Value);
				position++;
			}
			else if(position < (uint64_t)buffer.size())
			{
				buffer[position] = Value;
				position++;
			}
			else assert(false && "UNIMPLEMENTED");
		}

		void writeBytes(llvm::StringRef Bytes) override
		{
			std::size_t oldSize = buffer.size();
			buffer.resize(oldSize + Bytes.size());
			memcpy(&buffer[oldSize], Bytes.begin(), Bytes.size());
			position += Bytes.size();
		}

		uint64_t tell() const override { return position; }

		void seek(uint64_t Off) override { position = Off; }

		const void *getEntry() override
		{
			if(!entry)
			{
				position = std::numeric_limits<std::size_t>::max();   // Can't stream more data after this

				size_t codeSize = 0;
				entry = loadImage(&buffer[0], codeSize);

				#if defined(_WIN32)
					VirtualProtect(&buffer[0], buffer.size(), PAGE_EXECUTE_READ, &oldProtection);
					FlushInstructionCache(GetCurrentProcess(), NULL, 0);
				#else
					mprotect(&buffer[0], buffer.size(), PROT_READ | PROT_EXEC);
					__builtin___clear_cache((char*)entry, (char*)entry + codeSize);
				#endif
			}

			return entry;
		}

	private:
		void *entry;
		std::vector<uint8_t, ExecutableAllocator<uint8_t>> buffer;
		std::size_t position;

		#if defined(_WIN32)
		DWORD oldProtection;
		#endif
	};

	Nucleus::Nucleus()
	{
		::codegenMutex.lock();   // Reactor is currently not thread safe

		Ice::ClFlags &Flags = Ice::ClFlags::Flags;
		Ice::ClFlags::getParsedClFlags(Flags);

		#if defined(__arm__)
			Flags.setTargetArch(Ice::Target_ARM32);
			Flags.setTargetInstructionSet(Ice::ARM32InstructionSet_HWDivArm);
		#else   // x86
			Flags.setTargetArch(sizeof(void*) == 8 ? Ice::Target_X8664 : Ice::Target_X8632);
			Flags.setTargetInstructionSet(CPUID::SSE4_1 ? Ice::X86InstructionSet_SSE4_1 : Ice::X86InstructionSet_SSE2);
		#endif
		Flags.setOutFileType(Ice::FT_Elf);
		Flags.setOptLevel(Ice::Opt_2);
		Flags.setApplicationBinaryInterface(Ice::ABI_Platform);
		Flags.setVerbose(false ? Ice::IceV_Most : Ice::IceV_None);
		Flags.setDisableHybridAssembly(true);

		static llvm::raw_os_ostream cout(std::cout);
		static llvm::raw_os_ostream cerr(std::cerr);

		if(false)   // Write out to a file
		{
			std::error_code errorCode;
			::out = new Ice::Fdstream("out.o", errorCode, llvm::sys::fs::F_None);
			::elfFile = new Ice::ELFFileStreamer(*out);
			::context = new Ice::GlobalContext(&cout, &cout, &cerr, elfFile);
		}
		else
		{
			ELFMemoryStreamer *elfMemory = new ELFMemoryStreamer();
			::context = new Ice::GlobalContext(&cout, &cout, &cerr, elfMemory);
			::routine = elfMemory;
		}
	}

	Nucleus::~Nucleus()
	{
		delete ::routine;

		delete ::allocator;
		delete ::function;
		delete ::context;

		delete ::elfFile;
		delete ::out;

		::codegenMutex.unlock();
	}

	Routine *Nucleus::acquireRoutine(const wchar_t *name, bool runOptimizations)
	{
		if(basicBlock->getInsts().empty() || basicBlock->getInsts().back().getKind() != Ice::Inst::Ret)
		{
			createRetVoid();
		}

		std::wstring wideName(name);
		std::string asciiName(wideName.begin(), wideName.end());
		::function->setFunctionName(Ice::GlobalString::createWithString(::context, asciiName));

		optimize();

		::function->translate();
		assert(!::function->hasError());

		auto globals = ::function->getGlobalInits();

		if(globals && !globals->empty())
		{
			::context->getGlobals()->merge(globals.get());
		}

		::context->emitFileHeader();
		::function->emitIAS();
		auto assembler = ::function->releaseAssembler();
		auto objectWriter = ::context->getObjectWriter();
		assembler->alignFunction();
		objectWriter->writeFunctionCode(::function->getFunctionName(), false, assembler.get());
		::context->lowerGlobals("last");
		::context->lowerConstants();
		::context->lowerJumpTables();
		objectWriter->setUndefinedSyms(::context->getConstantExternSyms());
		objectWriter->writeNonUserSections();

		Routine *handoffRoutine = ::routine;
		::routine = nullptr;

		return handoffRoutine;
	}

	void Nucleus::optimize()
	{
		sw::optimize(::function);
	}

	Value *Nucleus::allocateStackVariable(Type *t, int arraySize)
	{
		Ice::Type type = T(t);
		int typeSize = Ice::typeWidthInBytes(type);
		int totalSize = typeSize * (arraySize ? arraySize : 1);

		auto bytes = Ice::ConstantInteger32::create(::context, type, totalSize);
		auto address = ::function->makeVariable(T(getPointerType(t)));
		auto alloca = Ice::InstAlloca::create(::function, address, bytes, typeSize);
		::function->getEntryNode()->getInsts().push_front(alloca);

		return V(address);
	}

	BasicBlock *Nucleus::createBasicBlock()
	{
		return B(::function->makeNode());
	}

	BasicBlock *Nucleus::getInsertBlock()
	{
		return B(::basicBlock);
	}

	void Nucleus::setInsertBlock(BasicBlock *basicBlock)
	{
	//	assert(::basicBlock->getInsts().back().getTerminatorEdges().size() >= 0 && "Previous basic block must have a terminator");
		::basicBlock = basicBlock;
	}

	void Nucleus::createFunction(Type *ReturnType, std::vector<Type*> &Params)
	{
		uint32_t sequenceNumber = 0;
		::function = Ice::Cfg::create(::context, sequenceNumber).release();
		::allocator = new Ice::CfgLocalAllocatorScope(::function);

		for(Type *type : Params)
		{
			Ice::Variable *arg = ::function->makeVariable(T(type));
			::function->addArg(arg);
		}

		Ice::CfgNode *node = ::function->makeNode();
		::function->setEntryNode(node);
		::basicBlock = node;
	}

	Value *Nucleus::getArgument(unsigned int index)
	{
		return V(::function->getArgs()[index]);
	}

	void Nucleus::createRetVoid()
	{
		Ice::InstRet *ret = Ice::InstRet::create(::function);
		::basicBlock->appendInst(ret);
	}

	void Nucleus::createRet(Value *v)
	{
		Ice::InstRet *ret = Ice::InstRet::create(::function, v);
		::basicBlock->appendInst(ret);
	}

	void Nucleus::createBr(BasicBlock *dest)
	{
		auto br = Ice::InstBr::create(::function, dest);
		::basicBlock->appendInst(br);
	}

	void Nucleus::createCondBr(Value *cond, BasicBlock *ifTrue, BasicBlock *ifFalse)
	{
		auto br = Ice::InstBr::create(::function, cond, ifTrue, ifFalse);
		::basicBlock->appendInst(br);
	}

	static bool isCommutative(Ice::InstArithmetic::OpKind op)
	{
		switch(op)
		{
		case Ice::InstArithmetic::Add:
		case Ice::InstArithmetic::Fadd:
		case Ice::InstArithmetic::Mul:
		case Ice::InstArithmetic::Fmul:
		case Ice::InstArithmetic::And:
		case Ice::InstArithmetic::Or:
		case Ice::InstArithmetic::Xor:
			return true;
		default:
			return false;
		}
	}

	static Value *createArithmetic(Ice::InstArithmetic::OpKind op, Value *lhs, Value *rhs)
	{
		assert(lhs->getType() == rhs->getType() || llvm::isa<Ice::Constant>(rhs));

		bool swapOperands = llvm::isa<Ice::Constant>(lhs) && isCommutative(op);

		Ice::Variable *result = ::function->makeVariable(lhs->getType());
		Ice::InstArithmetic *arithmetic = Ice::InstArithmetic::create(::function, op, result, swapOperands ? rhs : lhs, swapOperands ? lhs : rhs);
		::basicBlock->appendInst(arithmetic);

		return V(result);
	}

	Value *Nucleus::createAdd(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Add, lhs, rhs);
	}

	Value *Nucleus::createSub(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Sub, lhs, rhs);
	}

	Value *Nucleus::createMul(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Mul, lhs, rhs);
	}

	Value *Nucleus::createUDiv(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Udiv, lhs, rhs);
	}

	Value *Nucleus::createSDiv(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Sdiv, lhs, rhs);
	}

	Value *Nucleus::createFAdd(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Fadd, lhs, rhs);
	}

	Value *Nucleus::createFSub(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Fsub, lhs, rhs);
	}

	Value *Nucleus::createFMul(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Fmul, lhs, rhs);
	}

	Value *Nucleus::createFDiv(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Fdiv, lhs, rhs);
	}

	Value *Nucleus::createURem(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Urem, lhs, rhs);
	}

	Value *Nucleus::createSRem(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Srem, lhs, rhs);
	}

	Value *Nucleus::createFRem(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Frem, lhs, rhs);
	}

	Value *Nucleus::createShl(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Shl, lhs, rhs);
	}

	Value *Nucleus::createLShr(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Lshr, lhs, rhs);
	}

	Value *Nucleus::createAShr(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Ashr, lhs, rhs);
	}

	Value *Nucleus::createAnd(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::And, lhs, rhs);
	}

	Value *Nucleus::createOr(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Or, lhs, rhs);
	}

	Value *Nucleus::createXor(Value *lhs, Value *rhs)
	{
		return createArithmetic(Ice::InstArithmetic::Xor, lhs, rhs);
	}

	Value *Nucleus::createNeg(Value *v)
	{
		return createSub(createNullValue(T(v->getType())), v);
	}

	Value *Nucleus::createFNeg(Value *v)
	{
		double c[4] = {-0.0, -0.0, -0.0, -0.0};
		Value *negativeZero = Ice::isVectorType(v->getType()) ?
		                      createConstantVector(c, T(v->getType())) :
		                      V(::context->getConstantFloat(-0.0f));

		return createFSub(negativeZero, v);
	}

	Value *Nucleus::createNot(Value *v)
	{
		if(Ice::isScalarIntegerType(v->getType()))
		{
			return createXor(v, V(::context->getConstantInt(v->getType(), -1)));
		}
		else   // Vector
		{
			int64_t c[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
			return createXor(v, createConstantVector(c, T(v->getType())));
		}
	}

	Value *Nucleus::createLoad(Value *ptr, Type *type, bool isVolatile, unsigned int align)
	{
		int valueType = (int)reinterpret_cast<intptr_t>(type);
		Ice::Variable *result = ::function->makeVariable(T(type));

		if((valueType & EmulatedBits) && (align != 0))   // Narrow vector not stored on stack.
		{
			if(emulateIntrinsics)
			{
				if(typeSize(type) == 4)
				{
					auto pointer = RValue<Pointer<Byte>>(ptr);
					Int x = *Pointer<Int>(pointer);

					Int4 vector;
					vector = Insert(vector, x, 0);

					auto bitcast = Ice::InstCast::create(::function, Ice::InstCast::Bitcast, result, vector.loadValue());
					::basicBlock->appendInst(bitcast);
				}
				else if(typeSize(type) == 8)
				{
					auto pointer = RValue<Pointer<Byte>>(ptr);
					Int x = *Pointer<Int>(pointer);
					Int y = *Pointer<Int>(pointer + 4);

					Int4 vector;
					vector = Insert(vector, x, 0);
					vector = Insert(vector, y, 1);

					auto bitcast = Ice::InstCast::create(::function, Ice::InstCast::Bitcast, result, vector.loadValue());
					::basicBlock->appendInst(bitcast);
				}
				else assert(false);
			}
			else
			{
				const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::LoadSubVector, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
				auto target = ::context->getConstantUndef(Ice::IceType_i32);
				auto load = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
				load->addArg(ptr);
				load->addArg(::context->getConstantInt32(typeSize(type)));
				::basicBlock->appendInst(load);
			}
		}
		else
		{
			auto load = Ice::InstLoad::create(::function, result, ptr, align);
			::basicBlock->appendInst(load);
		}

		return V(result);
	}

	Value *Nucleus::createStore(Value *value, Value *ptr, Type *type, bool isVolatile, unsigned int align)
	{
		int valueType = (int)reinterpret_cast<intptr_t>(type);

		if((valueType & EmulatedBits) && (align != 0))   // Narrow vector not stored on stack.
		{
			if(emulateIntrinsics)
			{
				if(typeSize(type) == 4)
				{
					Ice::Variable *vector = ::function->makeVariable(Ice::IceType_v4i32);
					auto bitcast = Ice::InstCast::create(::function, Ice::InstCast::Bitcast, vector, value);
					::basicBlock->appendInst(bitcast);

					RValue<Int4> v(V(vector));

					auto pointer = RValue<Pointer<Byte>>(ptr);
					Int x = Extract(v, 0);
					*Pointer<Int>(pointer) = x;
				}
				else if(typeSize(type) == 8)
				{
					Ice::Variable *vector = ::function->makeVariable(Ice::IceType_v4i32);
					auto bitcast = Ice::InstCast::create(::function, Ice::InstCast::Bitcast, vector, value);
					::basicBlock->appendInst(bitcast);

					RValue<Int4> v(V(vector));

					auto pointer = RValue<Pointer<Byte>>(ptr);
					Int x = Extract(v, 0);
					*Pointer<Int>(pointer) = x;
					Int y = Extract(v, 1);
					*Pointer<Int>(pointer + 4) = y;
				}
				else assert(false);
			}
			else
			{
				const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::StoreSubVector, Ice::Intrinsics::SideEffects_T, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_T};
				auto target = ::context->getConstantUndef(Ice::IceType_i32);
				auto store = Ice::InstIntrinsicCall::create(::function, 3, nullptr, target, intrinsic);
				store->addArg(value);
				store->addArg(ptr);
				store->addArg(::context->getConstantInt32(typeSize(type)));
				::basicBlock->appendInst(store);
			}
		}
		else
		{
			assert(value->getType() == T(type));

			auto store = Ice::InstStore::create(::function, value, ptr, align);
			::basicBlock->appendInst(store);
		}

		return value;
	}

	Value *Nucleus::createGEP(Value *ptr, Type *type, Value *index, bool unsignedIndex)
	{
		assert(index->getType() == Ice::IceType_i32);

		if(auto *constant = llvm::dyn_cast<Ice::ConstantInteger32>(index))
		{
			int32_t offset = constant->getValue() * (int)typeSize(type);

			if(offset == 0)
			{
				return ptr;
			}

			return createAdd(ptr, createConstantInt(offset));
		}

		if(!Ice::isByteSizedType(T(type)))
		{
			index = createMul(index, createConstantInt((int)typeSize(type)));
		}

		if(sizeof(void*) == 8)
		{
			if(unsignedIndex)
			{
				index = createZExt(index, T(Ice::IceType_i64));
			}
			else
			{
				index = createSExt(index, T(Ice::IceType_i64));
			}
		}

		return createAdd(ptr, index);
	}

	Value *Nucleus::createAtomicAdd(Value *ptr, Value *value)
	{
		assert(false && "UNIMPLEMENTED"); return nullptr;
	}

	static Value *createCast(Ice::InstCast::OpKind op, Value *v, Type *destType)
	{
		if(v->getType() == T(destType))
		{
			return v;
		}

		Ice::Variable *result = ::function->makeVariable(T(destType));
		Ice::InstCast *cast = Ice::InstCast::create(::function, op, result, v);
		::basicBlock->appendInst(cast);

		return V(result);
	}

	Value *Nucleus::createTrunc(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Trunc, v, destType);
	}

	Value *Nucleus::createZExt(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Zext, v, destType);
	}

	Value *Nucleus::createSExt(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Sext, v, destType);
	}

	Value *Nucleus::createFPToSI(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Fptosi, v, destType);
	}

	Value *Nucleus::createSIToFP(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Sitofp, v, destType);
	}

	Value *Nucleus::createFPTrunc(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Fptrunc, v, destType);
	}

	Value *Nucleus::createFPExt(Value *v, Type *destType)
	{
		return createCast(Ice::InstCast::Fpext, v, destType);
	}

	Value *Nucleus::createBitCast(Value *v, Type *destType)
	{
		// Bitcasts must be between types of the same logical size. But with emulated narrow vectors we need
		// support for casting between scalars and wide vectors. For platforms where this is not supported,
		// emulate them by writing to the stack and reading back as the destination type.
		if(emulateMismatchedBitCast)
		{
			if(!Ice::isVectorType(v->getType()) && Ice::isVectorType(T(destType)))
			{
				Value *address = allocateStackVariable(destType);
				createStore(v, address, T(v->getType()));
				return createLoad(address, destType);
			}
			else if(Ice::isVectorType(v->getType()) && !Ice::isVectorType(T(destType)))
			{
				Value *address = allocateStackVariable(T(v->getType()));
				createStore(v, address, T(v->getType()));
				return createLoad(address, destType);
			}
		}

		return createCast(Ice::InstCast::Bitcast, v, destType);
	}

	static Value *createIntCompare(Ice::InstIcmp::ICond condition, Value *lhs, Value *rhs)
	{
		assert(lhs->getType() == rhs->getType());

		auto result = ::function->makeVariable(Ice::isScalarIntegerType(lhs->getType()) ? Ice::IceType_i1 : lhs->getType());
		auto cmp = Ice::InstIcmp::create(::function, condition, result, lhs, rhs);
		::basicBlock->appendInst(cmp);

		return V(result);
	}

	Value *Nucleus::createICmpEQ(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Eq, lhs, rhs);
	}

	Value *Nucleus::createICmpNE(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Ne, lhs, rhs);
	}

	Value *Nucleus::createICmpUGT(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Ugt, lhs, rhs);
	}

	Value *Nucleus::createICmpUGE(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Uge, lhs, rhs);
	}

	Value *Nucleus::createICmpULT(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Ult, lhs, rhs);
	}

	Value *Nucleus::createICmpULE(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Ule, lhs, rhs);
	}

	Value *Nucleus::createICmpSGT(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Sgt, lhs, rhs);
	}

	Value *Nucleus::createICmpSGE(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Sge, lhs, rhs);
	}

	Value *Nucleus::createICmpSLT(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Slt, lhs, rhs);
	}

	Value *Nucleus::createICmpSLE(Value *lhs, Value *rhs)
	{
		return createIntCompare(Ice::InstIcmp::Sle, lhs, rhs);
	}

	static Value *createFloatCompare(Ice::InstFcmp::FCond condition, Value *lhs, Value *rhs)
	{
		assert(lhs->getType() == rhs->getType());
		assert(Ice::isScalarFloatingType(lhs->getType()) || lhs->getType() == Ice::IceType_v4f32);

		auto result = ::function->makeVariable(Ice::isScalarFloatingType(lhs->getType()) ? Ice::IceType_i1 : Ice::IceType_v4i32);
		auto cmp = Ice::InstFcmp::create(::function, condition, result, lhs, rhs);
		::basicBlock->appendInst(cmp);

		return V(result);
	}

	Value *Nucleus::createFCmpOEQ(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Oeq, lhs, rhs);
	}

	Value *Nucleus::createFCmpOGT(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ogt, lhs, rhs);
	}

	Value *Nucleus::createFCmpOGE(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Oge, lhs, rhs);
	}

	Value *Nucleus::createFCmpOLT(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Olt, lhs, rhs);
	}

	Value *Nucleus::createFCmpOLE(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ole, lhs, rhs);
	}

	Value *Nucleus::createFCmpONE(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::One, lhs, rhs);
	}

	Value *Nucleus::createFCmpORD(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ord, lhs, rhs);
	}

	Value *Nucleus::createFCmpUNO(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Uno, lhs, rhs);
	}

	Value *Nucleus::createFCmpUEQ(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ueq, lhs, rhs);
	}

	Value *Nucleus::createFCmpUGT(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ugt, lhs, rhs);
	}

	Value *Nucleus::createFCmpUGE(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Uge, lhs, rhs);
	}

	Value *Nucleus::createFCmpULT(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ult, lhs, rhs);
	}

	Value *Nucleus::createFCmpULE(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Ule, lhs, rhs);
	}

	Value *Nucleus::createFCmpUNE(Value *lhs, Value *rhs)
	{
		return createFloatCompare(Ice::InstFcmp::Une, lhs, rhs);
	}

	Value *Nucleus::createExtractElement(Value *vector, Type *type, int index)
	{
		auto result = ::function->makeVariable(T(type));
		auto extract = Ice::InstExtractElement::create(::function, result, vector, ::context->getConstantInt32(index));
		::basicBlock->appendInst(extract);

		return V(result);
	}

	Value *Nucleus::createInsertElement(Value *vector, Value *element, int index)
	{
		auto result = ::function->makeVariable(vector->getType());
		auto insert = Ice::InstInsertElement::create(::function, result, vector, element, ::context->getConstantInt32(index));
		::basicBlock->appendInst(insert);

		return V(result);
	}

	Value *Nucleus::createShuffleVector(Value *V1, Value *V2, const int *select)
	{
		assert(V1->getType() == V2->getType());

		int size = Ice::typeNumElements(V1->getType());
		auto result = ::function->makeVariable(V1->getType());
		auto shuffle = Ice::InstShuffleVector::create(::function, result, V1, V2);

		for(int i = 0; i < size; i++)
		{
			shuffle->addIndex(llvm::cast<Ice::ConstantInteger32>(::context->getConstantInt32(select[i])));
		}

		::basicBlock->appendInst(shuffle);

		return V(result);
	}

	Value *Nucleus::createSelect(Value *C, Value *ifTrue, Value *ifFalse)
	{
		assert(ifTrue->getType() == ifFalse->getType());

		auto result = ::function->makeVariable(ifTrue->getType());
		auto *select = Ice::InstSelect::create(::function, result, C, ifTrue, ifFalse);
		::basicBlock->appendInst(select);

		return V(result);
	}

	SwitchCases *Nucleus::createSwitch(Value *control, BasicBlock *defaultBranch, unsigned numCases)
	{
		auto switchInst = Ice::InstSwitch::create(::function, numCases, control, defaultBranch);
		::basicBlock->appendInst(switchInst);

		return reinterpret_cast<SwitchCases*>(switchInst);
	}

	void Nucleus::addSwitchCase(SwitchCases *switchCases, int label, BasicBlock *branch)
	{
		switchCases->addBranch(label, label, branch);
	}

	void Nucleus::createUnreachable()
	{
		Ice::InstUnreachable *unreachable = Ice::InstUnreachable::create(::function);
		::basicBlock->appendInst(unreachable);
	}

	static Value *createSwizzle4(Value *val, unsigned char select)
	{
		int swizzle[4] =
		{
			(select >> 0) & 0x03,
			(select >> 2) & 0x03,
			(select >> 4) & 0x03,
			(select >> 6) & 0x03,
		};

		return Nucleus::createShuffleVector(val, val, swizzle);
	}

	static Value *createMask4(Value *lhs, Value *rhs, unsigned char select)
	{
		int64_t mask[4] = {0, 0, 0, 0};

		mask[(select >> 0) & 0x03] = -1;
		mask[(select >> 2) & 0x03] = -1;
		mask[(select >> 4) & 0x03] = -1;
		mask[(select >> 6) & 0x03] = -1;

		Value *condition = Nucleus::createConstantVector(mask, T(Ice::IceType_v4i1));
		Value *result = Nucleus::createSelect(condition, rhs, lhs);

		return result;
	}

	Type *Nucleus::getPointerType(Type *ElementType)
	{
		if(sizeof(void*) == 8)
		{
			return T(Ice::IceType_i64);
		}
		else
		{
			return T(Ice::IceType_i32);
		}
	}

	Value *Nucleus::createNullValue(Type *Ty)
	{
		if(Ice::isVectorType(T(Ty)))
		{
			assert(Ice::typeNumElements(T(Ty)) <= 16);
			int64_t c[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			return createConstantVector(c, Ty);
		}
		else
		{
			return V(::context->getConstantZero(T(Ty)));
		}
	}

	Value *Nucleus::createConstantLong(int64_t i)
	{
		return V(::context->getConstantInt64(i));
	}

	Value *Nucleus::createConstantInt(int i)
	{
		return V(::context->getConstantInt32(i));
	}

	Value *Nucleus::createConstantInt(unsigned int i)
	{
		return V(::context->getConstantInt32(i));
	}

	Value *Nucleus::createConstantBool(bool b)
	{
		return V(::context->getConstantInt1(b));
	}

	Value *Nucleus::createConstantByte(signed char i)
	{
		return V(::context->getConstantInt8(i));
	}

	Value *Nucleus::createConstantByte(unsigned char i)
	{
		return V(::context->getConstantInt8(i));
	}

	Value *Nucleus::createConstantShort(short i)
	{
		return V(::context->getConstantInt16(i));
	}

	Value *Nucleus::createConstantShort(unsigned short i)
	{
		return V(::context->getConstantInt16(i));
	}

	Value *Nucleus::createConstantFloat(float x)
	{
		return V(::context->getConstantFloat(x));
	}

	Value *Nucleus::createNullPointer(Type *Ty)
	{
		return createNullValue(T(sizeof(void*) == 8 ? Ice::IceType_i64 : Ice::IceType_i32));
	}

	Value *Nucleus::createConstantVector(const int64_t *constants, Type *type)
	{
		const int vectorSize = 16;
		assert(Ice::typeWidthInBytes(T(type)) == vectorSize);
		const int alignment = vectorSize;
		auto globalPool = ::function->getGlobalPool();

		const int64_t *i = constants;
		const double *f = reinterpret_cast<const double*>(constants);
		Ice::VariableDeclaration::DataInitializer *dataInitializer = nullptr;

		switch((int)reinterpret_cast<intptr_t>(type))
		{
		case Ice::IceType_v4i32:
		case Ice::IceType_v4i1:
			{
				const int initializer[4] = {(int)i[0], (int)i[1], (int)i[2], (int)i[3]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Ice::IceType_v4f32:
			{
				const float initializer[4] = {(float)f[0], (float)f[1], (float)f[2], (float)f[3]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Ice::IceType_v8i16:
		case Ice::IceType_v8i1:
			{
				const short initializer[8] = {(short)i[0], (short)i[1], (short)i[2], (short)i[3], (short)i[4], (short)i[5], (short)i[6], (short)i[7]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Ice::IceType_v16i8:
		case Ice::IceType_v16i1:
			{
				const char initializer[16] = {(char)i[0], (char)i[1], (char)i[2], (char)i[3], (char)i[4], (char)i[5], (char)i[6], (char)i[7], (char)i[8], (char)i[9], (char)i[10], (char)i[11], (char)i[12], (char)i[13], (char)i[14], (char)i[15]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Type_v2i32:
			{
				const int initializer[4] = {(int)i[0], (int)i[1], (int)i[0], (int)i[1]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Type_v2f32:
			{
				const float initializer[4] = {(float)f[0], (float)f[1], (float)f[0], (float)f[1]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Type_v4i16:
			{
				const short initializer[8] = {(short)i[0], (short)i[1], (short)i[2], (short)i[3], (short)i[0], (short)i[1], (short)i[2], (short)i[3]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Type_v8i8:
			{
				const char initializer[16] = {(char)i[0], (char)i[1], (char)i[2], (char)i[3], (char)i[4], (char)i[5], (char)i[6], (char)i[7], (char)i[0], (char)i[1], (char)i[2], (char)i[3], (char)i[4], (char)i[5], (char)i[6], (char)i[7]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		case Type_v4i8:
			{
				const char initializer[16] = {(char)i[0], (char)i[1], (char)i[2], (char)i[3], (char)i[0], (char)i[1], (char)i[2], (char)i[3], (char)i[0], (char)i[1], (char)i[2], (char)i[3], (char)i[0], (char)i[1], (char)i[2], (char)i[3]};
				static_assert(sizeof(initializer) == vectorSize, "!");
				dataInitializer = Ice::VariableDeclaration::DataInitializer::create(globalPool, (const char*)initializer, vectorSize);
			}
			break;
		default:
			assert(false && "Unknown constant vector type" && type);
		}

		auto name = Ice::GlobalString::createWithoutString(::context);
		auto *variableDeclaration = Ice::VariableDeclaration::create(globalPool);
		variableDeclaration->setName(name);
		variableDeclaration->setAlignment(alignment);
		variableDeclaration->setIsConstant(true);
		variableDeclaration->addInitializer(dataInitializer);

		::function->addGlobal(variableDeclaration);

		constexpr int32_t offset = 0;
		Ice::Operand *ptr = ::context->getConstantSym(offset, name);

		Ice::Variable *result = ::function->makeVariable(T(type));
		auto load = Ice::InstLoad::create(::function, result, ptr, alignment);
		::basicBlock->appendInst(load);

		return V(result);
	}

	Value *Nucleus::createConstantVector(const double *constants, Type *type)
	{
		return createConstantVector((const int64_t*)constants, type);
	}

	Type *Void::getType()
	{
		return T(Ice::IceType_void);
	}

	Bool::Bool(Argument<Bool> argument)
	{
		storeValue(argument.value);
	}

	Bool::Bool(bool x)
	{
		storeValue(Nucleus::createConstantBool(x));
	}

	Bool::Bool(RValue<Bool> rhs)
	{
		storeValue(rhs.value);
	}

	Bool::Bool(const Bool &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Bool::Bool(const Reference<Bool> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Bool> Bool::operator=(RValue<Bool> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Bool> Bool::operator=(const Bool &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Bool>(value);
	}

	RValue<Bool> Bool::operator=(const Reference<Bool> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Bool>(value);
	}

	RValue<Bool> operator!(RValue<Bool> val)
	{
		return RValue<Bool>(Nucleus::createNot(val.value));
	}

	RValue<Bool> operator&&(RValue<Bool> lhs, RValue<Bool> rhs)
	{
		return RValue<Bool>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Bool> operator||(RValue<Bool> lhs, RValue<Bool> rhs)
	{
		return RValue<Bool>(Nucleus::createOr(lhs.value, rhs.value));
	}

	Type *Bool::getType()
	{
		return T(Ice::IceType_i1);
	}

	Byte::Byte(Argument<Byte> argument)
	{
		storeValue(argument.value);
	}

	Byte::Byte(RValue<Int> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, Byte::getType());

		storeValue(integer);
	}

	Byte::Byte(RValue<UInt> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, Byte::getType());

		storeValue(integer);
	}

	Byte::Byte(RValue<UShort> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, Byte::getType());

		storeValue(integer);
	}

	Byte::Byte(int x)
	{
		storeValue(Nucleus::createConstantByte((unsigned char)x));
	}

	Byte::Byte(unsigned char x)
	{
		storeValue(Nucleus::createConstantByte(x));
	}

	Byte::Byte(RValue<Byte> rhs)
	{
		storeValue(rhs.value);
	}

	Byte::Byte(const Byte &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Byte::Byte(const Reference<Byte> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Byte> Byte::operator=(RValue<Byte> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Byte> Byte::operator=(const Byte &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Byte>(value);
	}

	RValue<Byte> Byte::operator=(const Reference<Byte> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Byte>(value);
	}

	RValue<Byte> operator+(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Byte> operator-(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<Byte> operator*(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<Byte> operator/(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createUDiv(lhs.value, rhs.value));
	}

	RValue<Byte> operator%(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createURem(lhs.value, rhs.value));
	}

	RValue<Byte> operator&(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Byte> operator|(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Byte> operator^(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<Byte> operator<<(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<Byte> operator>>(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Byte>(Nucleus::createLShr(lhs.value, rhs.value));
	}

	RValue<Byte> operator+=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Byte> operator-=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Byte> operator*=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<Byte> operator/=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<Byte> operator%=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<Byte> operator&=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Byte> operator|=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Byte> operator^=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<Byte> operator<<=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<Byte> operator>>=(Byte &lhs, RValue<Byte> rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<Byte> operator+(RValue<Byte> val)
	{
		return val;
	}

	RValue<Byte> operator-(RValue<Byte> val)
	{
		return RValue<Byte>(Nucleus::createNeg(val.value));
	}

	RValue<Byte> operator~(RValue<Byte> val)
	{
		return RValue<Byte>(Nucleus::createNot(val.value));
	}

	RValue<Byte> operator++(Byte &val, int)   // Post-increment
	{
		RValue<Byte> res = val;
		val += Byte(1);
		return res;
	}

	const Byte &operator++(Byte &val)   // Pre-increment
	{
		val += Byte(1);
		return val;
	}

	RValue<Byte> operator--(Byte &val, int)   // Post-decrement
	{
		RValue<Byte> res = val;
		val -= Byte(1);
		return res;
	}

	const Byte &operator--(Byte &val)   // Pre-decrement
	{
		val -= Byte(1);
		return val;
	}

	RValue<Bool> operator<(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpULT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpULE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpUGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpUGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpNE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<Byte> lhs, RValue<Byte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpEQ(lhs.value, rhs.value));
	}

	Type *Byte::getType()
	{
		return T(Ice::IceType_i8);
	}

	SByte::SByte(Argument<SByte> argument)
	{
		storeValue(argument.value);
	}

	SByte::SByte(RValue<Int> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, SByte::getType());

		storeValue(integer);
	}

	SByte::SByte(RValue<Short> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, SByte::getType());

		storeValue(integer);
	}

	SByte::SByte(signed char x)
	{
		storeValue(Nucleus::createConstantByte(x));
	}

	SByte::SByte(RValue<SByte> rhs)
	{
		storeValue(rhs.value);
	}

	SByte::SByte(const SByte &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	SByte::SByte(const Reference<SByte> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<SByte> SByte::operator=(RValue<SByte> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<SByte> SByte::operator=(const SByte &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<SByte>(value);
	}

	RValue<SByte> SByte::operator=(const Reference<SByte> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<SByte>(value);
	}

	RValue<SByte> operator+(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<SByte> operator-(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<SByte> operator*(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<SByte> operator/(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createSDiv(lhs.value, rhs.value));
	}

	RValue<SByte> operator%(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createSRem(lhs.value, rhs.value));
	}

	RValue<SByte> operator&(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<SByte> operator|(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<SByte> operator^(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<SByte> operator<<(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<SByte> operator>>(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<SByte>(Nucleus::createAShr(lhs.value, rhs.value));
	}

	RValue<SByte> operator+=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<SByte> operator-=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<SByte> operator*=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<SByte> operator/=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<SByte> operator%=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<SByte> operator&=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<SByte> operator|=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<SByte> operator^=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<SByte> operator<<=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<SByte> operator>>=(SByte &lhs, RValue<SByte> rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<SByte> operator+(RValue<SByte> val)
	{
		return val;
	}

	RValue<SByte> operator-(RValue<SByte> val)
	{
		return RValue<SByte>(Nucleus::createNeg(val.value));
	}

	RValue<SByte> operator~(RValue<SByte> val)
	{
		return RValue<SByte>(Nucleus::createNot(val.value));
	}

	RValue<SByte> operator++(SByte &val, int)   // Post-increment
	{
		RValue<SByte> res = val;
		val += SByte(1);
		return res;
	}

	const SByte &operator++(SByte &val)   // Pre-increment
	{
		val += SByte(1);
		return val;
	}

	RValue<SByte> operator--(SByte &val, int)   // Post-decrement
	{
		RValue<SByte> res = val;
		val -= SByte(1);
		return res;
	}

	const SByte &operator--(SByte &val)   // Pre-decrement
	{
		val -= SByte(1);
		return val;
	}

	RValue<Bool> operator<(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSLT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSLE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpNE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<SByte> lhs, RValue<SByte> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpEQ(lhs.value, rhs.value));
	}

	Type *SByte::getType()
	{
		return T(Ice::IceType_i8);
	}

	Short::Short(Argument<Short> argument)
	{
		storeValue(argument.value);
	}

	Short::Short(RValue<Int> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, Short::getType());

		storeValue(integer);
	}

	Short::Short(short x)
	{
		storeValue(Nucleus::createConstantShort(x));
	}

	Short::Short(RValue<Short> rhs)
	{
		storeValue(rhs.value);
	}

	Short::Short(const Short &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Short::Short(const Reference<Short> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Short> Short::operator=(RValue<Short> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Short> Short::operator=(const Short &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Short>(value);
	}

	RValue<Short> Short::operator=(const Reference<Short> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Short>(value);
	}

	RValue<Short> operator+(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Short> operator-(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<Short> operator*(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<Short> operator/(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createSDiv(lhs.value, rhs.value));
	}

	RValue<Short> operator%(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createSRem(lhs.value, rhs.value));
	}

	RValue<Short> operator&(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Short> operator|(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Short> operator^(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<Short> operator<<(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<Short> operator>>(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Short>(Nucleus::createAShr(lhs.value, rhs.value));
	}

	RValue<Short> operator+=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Short> operator-=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Short> operator*=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<Short> operator/=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<Short> operator%=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<Short> operator&=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Short> operator|=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Short> operator^=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<Short> operator<<=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<Short> operator>>=(Short &lhs, RValue<Short> rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<Short> operator+(RValue<Short> val)
	{
		return val;
	}

	RValue<Short> operator-(RValue<Short> val)
	{
		return RValue<Short>(Nucleus::createNeg(val.value));
	}

	RValue<Short> operator~(RValue<Short> val)
	{
		return RValue<Short>(Nucleus::createNot(val.value));
	}

	RValue<Short> operator++(Short &val, int)   // Post-increment
	{
		RValue<Short> res = val;
		val += Short(1);
		return res;
	}

	const Short &operator++(Short &val)   // Pre-increment
	{
		val += Short(1);
		return val;
	}

	RValue<Short> operator--(Short &val, int)   // Post-decrement
	{
		RValue<Short> res = val;
		val -= Short(1);
		return res;
	}

	const Short &operator--(Short &val)   // Pre-decrement
	{
		val -= Short(1);
		return val;
	}

	RValue<Bool> operator<(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSLT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSLE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpNE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<Short> lhs, RValue<Short> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpEQ(lhs.value, rhs.value));
	}

	Type *Short::getType()
	{
		return T(Ice::IceType_i16);
	}

	UShort::UShort(Argument<UShort> argument)
	{
		storeValue(argument.value);
	}

	UShort::UShort(RValue<UInt> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, UShort::getType());

		storeValue(integer);
	}

	UShort::UShort(RValue<Int> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, UShort::getType());

		storeValue(integer);
	}

	UShort::UShort(unsigned short x)
	{
		storeValue(Nucleus::createConstantShort(x));
	}

	UShort::UShort(RValue<UShort> rhs)
	{
		storeValue(rhs.value);
	}

	UShort::UShort(const UShort &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UShort::UShort(const Reference<UShort> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<UShort> UShort::operator=(RValue<UShort> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<UShort> UShort::operator=(const UShort &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort>(value);
	}

	RValue<UShort> UShort::operator=(const Reference<UShort> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort>(value);
	}

	RValue<UShort> operator+(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<UShort> operator-(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<UShort> operator*(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<UShort> operator/(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createUDiv(lhs.value, rhs.value));
	}

	RValue<UShort> operator%(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createURem(lhs.value, rhs.value));
	}

	RValue<UShort> operator&(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<UShort> operator|(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<UShort> operator^(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<UShort> operator<<(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<UShort> operator>>(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<UShort>(Nucleus::createLShr(lhs.value, rhs.value));
	}

	RValue<UShort> operator+=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<UShort> operator-=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<UShort> operator*=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<UShort> operator/=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<UShort> operator%=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<UShort> operator&=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<UShort> operator|=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<UShort> operator^=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<UShort> operator<<=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<UShort> operator>>=(UShort &lhs, RValue<UShort> rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<UShort> operator+(RValue<UShort> val)
	{
		return val;
	}

	RValue<UShort> operator-(RValue<UShort> val)
	{
		return RValue<UShort>(Nucleus::createNeg(val.value));
	}

	RValue<UShort> operator~(RValue<UShort> val)
	{
		return RValue<UShort>(Nucleus::createNot(val.value));
	}

	RValue<UShort> operator++(UShort &val, int)   // Post-increment
	{
		RValue<UShort> res = val;
		val += UShort(1);
		return res;
	}

	const UShort &operator++(UShort &val)   // Pre-increment
	{
		val += UShort(1);
		return val;
	}

	RValue<UShort> operator--(UShort &val, int)   // Post-decrement
	{
		RValue<UShort> res = val;
		val -= UShort(1);
		return res;
	}

	const UShort &operator--(UShort &val)   // Pre-decrement
	{
		val -= UShort(1);
		return val;
	}

	RValue<Bool> operator<(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpULT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpULE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpUGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpUGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpNE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<UShort> lhs, RValue<UShort> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpEQ(lhs.value, rhs.value));
	}

	Type *UShort::getType()
	{
		return T(Ice::IceType_i16);
	}

	Byte4::Byte4(RValue<Byte8> cast)
	{
		storeValue(Nucleus::createBitCast(cast.value, getType()));
	}

	Byte4::Byte4(const Reference<Byte4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Type *Byte4::getType()
	{
		return T(Type_v4i8);
	}

	Type *SByte4::getType()
	{
		return T(Type_v4i8);
	}

	Byte8::Byte8(uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4, uint8_t x5, uint8_t x6, uint8_t x7)
	{
		int64_t constantVector[8] = {x0, x1, x2, x3, x4, x5, x6, x7};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Byte8::Byte8(RValue<Byte8> rhs)
	{
		storeValue(rhs.value);
	}

	Byte8::Byte8(const Byte8 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Byte8::Byte8(const Reference<Byte8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Byte8> Byte8::operator=(RValue<Byte8> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Byte8> Byte8::operator=(const Byte8 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Byte8>(value);
	}

	RValue<Byte8> Byte8::operator=(const Reference<Byte8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Byte8>(value);
	}

	RValue<Byte8> operator+(RValue<Byte8> lhs, RValue<Byte8> rhs)
	{
		return RValue<Byte8>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Byte8> operator-(RValue<Byte8> lhs, RValue<Byte8> rhs)
	{
		return RValue<Byte8>(Nucleus::createSub(lhs.value, rhs.value));
	}

//	RValue<Byte8> operator*(RValue<Byte8> lhs, RValue<Byte8> rhs)
//	{
//		return RValue<Byte8>(Nucleus::createMul(lhs.value, rhs.value));
//	}

//	RValue<Byte8> operator/(RValue<Byte8> lhs, RValue<Byte8> rhs)
//	{
//		return RValue<Byte8>(Nucleus::createUDiv(lhs.value, rhs.value));
//	}

//	RValue<Byte8> operator%(RValue<Byte8> lhs, RValue<Byte8> rhs)
//	{
//		return RValue<Byte8>(Nucleus::createURem(lhs.value, rhs.value));
//	}

	RValue<Byte8> operator&(RValue<Byte8> lhs, RValue<Byte8> rhs)
	{
		return RValue<Byte8>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Byte8> operator|(RValue<Byte8> lhs, RValue<Byte8> rhs)
	{
		return RValue<Byte8>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Byte8> operator^(RValue<Byte8> lhs, RValue<Byte8> rhs)
	{
		return RValue<Byte8>(Nucleus::createXor(lhs.value, rhs.value));
	}

//	RValue<Byte8> operator<<(RValue<Byte8> lhs, unsigned char rhs)
//	{
//		return RValue<Byte8>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
//	}

//	RValue<Byte8> operator>>(RValue<Byte8> lhs, unsigned char rhs)
//	{
//		return RValue<Byte8>(Nucleus::createLShr(lhs.value, V(::context->getConstantInt32(rhs))));
//	}

	RValue<Byte8> operator+=(Byte8 &lhs, RValue<Byte8> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Byte8> operator-=(Byte8 &lhs, RValue<Byte8> rhs)
	{
		return lhs = lhs - rhs;
	}

//	RValue<Byte8> operator*=(Byte8 &lhs, RValue<Byte8> rhs)
//	{
//		return lhs = lhs * rhs;
//	}

//	RValue<Byte8> operator/=(Byte8 &lhs, RValue<Byte8> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<Byte8> operator%=(Byte8 &lhs, RValue<Byte8> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<Byte8> operator&=(Byte8 &lhs, RValue<Byte8> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Byte8> operator|=(Byte8 &lhs, RValue<Byte8> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Byte8> operator^=(Byte8 &lhs, RValue<Byte8> rhs)
	{
		return lhs = lhs ^ rhs;
	}

//	RValue<Byte8> operator<<=(Byte8 &lhs, RValue<Byte8> rhs)
//	{
//		return lhs = lhs << rhs;
//	}

//	RValue<Byte8> operator>>=(Byte8 &lhs, RValue<Byte8> rhs)
//	{
//		return lhs = lhs >> rhs;
//	}

//	RValue<Byte8> operator+(RValue<Byte8> val)
//	{
//		return val;
//	}

//	RValue<Byte8> operator-(RValue<Byte8> val)
//	{
//		return RValue<Byte8>(Nucleus::createNeg(val.value));
//	}

	RValue<Byte8> operator~(RValue<Byte8> val)
	{
		return RValue<Byte8>(Nucleus::createNot(val.value));
	}

	RValue<Byte> Extract(RValue<Byte8> val, int i)
	{
		return RValue<Byte>(Nucleus::createExtractElement(val.value, Byte::getType(), i));
	}

	RValue<Byte8> Insert(RValue<Byte8> val, RValue<Byte> element, int i)
	{
		return RValue<Byte8>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<Byte> SaturateUnsigned(RValue<Short> x)
	{
		return Byte(IfThenElse(Int(x) > 0xFF, Int(0xFF), IfThenElse(Int(x) < 0, Int(0), Int(x))));
	}

	RValue<Byte8> AddSat(RValue<Byte8> x, RValue<Byte8> y)
	{
		if(emulateIntrinsics)
		{
			Byte8 result;
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 0)) + Int(Extract(y, 0)))), 0);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 1)) + Int(Extract(y, 1)))), 1);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 2)) + Int(Extract(y, 2)))), 2);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 3)) + Int(Extract(y, 3)))), 3);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 4)) + Int(Extract(y, 4)))), 4);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 5)) + Int(Extract(y, 5)))), 5);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 6)) + Int(Extract(y, 6)))), 6);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 7)) + Int(Extract(y, 7)))), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v16i8);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::AddSaturateUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto paddusb = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			paddusb->addArg(x.value);
			paddusb->addArg(y.value);
			::basicBlock->appendInst(paddusb);

			return RValue<Byte8>(V(result));
		}
	}

	RValue<Byte8> SubSat(RValue<Byte8> x, RValue<Byte8> y)
	{
		if(emulateIntrinsics)
		{
			Byte8 result;
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 0)) - Int(Extract(y, 0)))), 0);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 1)) - Int(Extract(y, 1)))), 1);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 2)) - Int(Extract(y, 2)))), 2);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 3)) - Int(Extract(y, 3)))), 3);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 4)) - Int(Extract(y, 4)))), 4);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 5)) - Int(Extract(y, 5)))), 5);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 6)) - Int(Extract(y, 6)))), 6);
			result = Insert(result, SaturateUnsigned(Short(Int(Extract(x, 7)) - Int(Extract(y, 7)))), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v16i8);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SubtractSaturateUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto psubusw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			psubusw->addArg(x.value);
			psubusw->addArg(y.value);
			::basicBlock->appendInst(psubusw);

			return RValue<Byte8>(V(result));
		}
	}

	RValue<Short4> Unpack(RValue<Byte4> x)
	{
		int shuffle[16] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};   // Real type is v16i8
		return As<Short4>(Nucleus::createShuffleVector(x.value, x.value, shuffle));
	}

	RValue<Short4> Unpack(RValue<Byte4> x, RValue<Byte4> y)
	{
		return UnpackLow(As<Byte8>(x), As<Byte8>(y));
	}

	RValue<Short4> UnpackLow(RValue<Byte8> x, RValue<Byte8> y)
	{
		int shuffle[16] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};   // Real type is v16i8
		return As<Short4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Short4> UnpackHigh(RValue<Byte8> x, RValue<Byte8> y)
	{
		int shuffle[16] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};   // Real type is v16i8
		auto lowHigh = RValue<Byte16>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
		return As<Short4>(Swizzle(As<Int4>(lowHigh), 0xEE));
	}

	RValue<SByte> Extract(RValue<SByte8> val, int i)
	{
		return RValue<SByte>(Nucleus::createExtractElement(val.value, SByte::getType(), i));
	}

	RValue<SByte8> Insert(RValue<SByte8> val, RValue<SByte> element, int i)
	{
		return RValue<SByte8>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<SByte8> operator>>(RValue<SByte8> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			SByte8 result;
			result = Insert(result, Extract(lhs, 0) >> SByte(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> SByte(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> SByte(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> SByte(rhs), 3);
			result = Insert(result, Extract(lhs, 4) >> SByte(rhs), 4);
			result = Insert(result, Extract(lhs, 5) >> SByte(rhs), 5);
			result = Insert(result, Extract(lhs, 6) >> SByte(rhs), 6);
			result = Insert(result, Extract(lhs, 7) >> SByte(rhs), 7);

			return result;
		}
		else
		{
			#if defined(__i386__) || defined(__x86_64__)
				// SSE2 doesn't support byte vector shifts, so shift as shorts and recombine.
				RValue<Short4> hi = (As<Short4>(lhs) >> rhs) & Short4(0xFF00u);
				RValue<Short4> lo = As<Short4>(As<UShort4>((As<Short4>(lhs) << 8) >> rhs) >> 8);

				return As<SByte8>(hi | lo);
			#else
				return RValue<SByte8>(Nucleus::createAShr(lhs.value, V(::context->getConstantInt32(rhs))));
			#endif
		}
	}

	RValue<Int> SignMask(RValue<Byte8> x)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			Byte8 xx = As<Byte8>(As<SByte8>(x) >> 7) & Byte8(0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80);
			return Int(Extract(xx, 0)) | Int(Extract(xx, 1)) | Int(Extract(xx, 2)) | Int(Extract(xx, 3)) | Int(Extract(xx, 4)) | Int(Extract(xx, 5)) | Int(Extract(xx, 6)) | Int(Extract(xx, 7));
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_i32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SignMask, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto movmsk = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			movmsk->addArg(x.value);
			::basicBlock->appendInst(movmsk);

			return RValue<Int>(V(result)) & 0xFF;
		}
	}

//	RValue<Byte8> CmpGT(RValue<Byte8> x, RValue<Byte8> y)
//	{
//		return RValue<Byte8>(createIntCompare(Ice::InstIcmp::Ugt, x.value, y.value));
//	}

	RValue<Byte8> CmpEQ(RValue<Byte8> x, RValue<Byte8> y)
	{
		return RValue<Byte8>(Nucleus::createICmpEQ(x.value, y.value));
	}

	Type *Byte8::getType()
	{
		return T(Type_v8i8);
	}

	SByte8::SByte8(uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4, uint8_t x5, uint8_t x6, uint8_t x7)
	{
		int64_t constantVector[8] = { x0, x1, x2, x3, x4, x5, x6, x7 };
		Value *vector = V(Nucleus::createConstantVector(constantVector, getType()));

		storeValue(Nucleus::createBitCast(vector, getType()));
	}

	SByte8::SByte8(RValue<SByte8> rhs)
	{
		storeValue(rhs.value);
	}

	SByte8::SByte8(const SByte8 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	SByte8::SByte8(const Reference<SByte8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<SByte8> SByte8::operator=(RValue<SByte8> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<SByte8> SByte8::operator=(const SByte8 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<SByte8>(value);
	}

	RValue<SByte8> SByte8::operator=(const Reference<SByte8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<SByte8>(value);
	}

	RValue<SByte8> operator+(RValue<SByte8> lhs, RValue<SByte8> rhs)
	{
		return RValue<SByte8>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<SByte8> operator-(RValue<SByte8> lhs, RValue<SByte8> rhs)
	{
		return RValue<SByte8>(Nucleus::createSub(lhs.value, rhs.value));
	}

//	RValue<SByte8> operator*(RValue<SByte8> lhs, RValue<SByte8> rhs)
//	{
//		return RValue<SByte8>(Nucleus::createMul(lhs.value, rhs.value));
//	}

//	RValue<SByte8> operator/(RValue<SByte8> lhs, RValue<SByte8> rhs)
//	{
//		return RValue<SByte8>(Nucleus::createSDiv(lhs.value, rhs.value));
//	}

//	RValue<SByte8> operator%(RValue<SByte8> lhs, RValue<SByte8> rhs)
//	{
//		return RValue<SByte8>(Nucleus::createSRem(lhs.value, rhs.value));
//	}

	RValue<SByte8> operator&(RValue<SByte8> lhs, RValue<SByte8> rhs)
	{
		return RValue<SByte8>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<SByte8> operator|(RValue<SByte8> lhs, RValue<SByte8> rhs)
	{
		return RValue<SByte8>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<SByte8> operator^(RValue<SByte8> lhs, RValue<SByte8> rhs)
	{
		return RValue<SByte8>(Nucleus::createXor(lhs.value, rhs.value));
	}

//	RValue<SByte8> operator<<(RValue<SByte8> lhs, unsigned char rhs)
//	{
//		return RValue<SByte8>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
//	}

//	RValue<SByte8> operator>>(RValue<SByte8> lhs, unsigned char rhs)
//	{
//		return RValue<SByte8>(Nucleus::createAShr(lhs.value, V(::context->getConstantInt32(rhs))));
//	}

	RValue<SByte8> operator+=(SByte8 &lhs, RValue<SByte8> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<SByte8> operator-=(SByte8 &lhs, RValue<SByte8> rhs)
	{
		return lhs = lhs - rhs;
	}

//	RValue<SByte8> operator*=(SByte8 &lhs, RValue<SByte8> rhs)
//	{
//		return lhs = lhs * rhs;
//	}

//	RValue<SByte8> operator/=(SByte8 &lhs, RValue<SByte8> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<SByte8> operator%=(SByte8 &lhs, RValue<SByte8> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<SByte8> operator&=(SByte8 &lhs, RValue<SByte8> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<SByte8> operator|=(SByte8 &lhs, RValue<SByte8> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<SByte8> operator^=(SByte8 &lhs, RValue<SByte8> rhs)
	{
		return lhs = lhs ^ rhs;
	}

//	RValue<SByte8> operator<<=(SByte8 &lhs, RValue<SByte8> rhs)
//	{
//		return lhs = lhs << rhs;
//	}

//	RValue<SByte8> operator>>=(SByte8 &lhs, RValue<SByte8> rhs)
//	{
//		return lhs = lhs >> rhs;
//	}

//	RValue<SByte8> operator+(RValue<SByte8> val)
//	{
//		return val;
//	}

//	RValue<SByte8> operator-(RValue<SByte8> val)
//	{
//		return RValue<SByte8>(Nucleus::createNeg(val.value));
//	}

	RValue<SByte8> operator~(RValue<SByte8> val)
	{
		return RValue<SByte8>(Nucleus::createNot(val.value));
	}

	RValue<SByte> SaturateSigned(RValue<Short> x)
	{
		return SByte(IfThenElse(Int(x) > 0x7F, Int(0x7F), IfThenElse(Int(x) < -0x80, Int(0x80), Int(x))));
	}

	RValue<SByte8> AddSat(RValue<SByte8> x, RValue<SByte8> y)
	{
		if(emulateIntrinsics)
		{
			SByte8 result;
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 0)) + Int(Extract(y, 0)))), 0);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 1)) + Int(Extract(y, 1)))), 1);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 2)) + Int(Extract(y, 2)))), 2);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 3)) + Int(Extract(y, 3)))), 3);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 4)) + Int(Extract(y, 4)))), 4);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 5)) + Int(Extract(y, 5)))), 5);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 6)) + Int(Extract(y, 6)))), 6);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 7)) + Int(Extract(y, 7)))), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v16i8);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::AddSaturateSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto paddsb = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			paddsb->addArg(x.value);
			paddsb->addArg(y.value);
			::basicBlock->appendInst(paddsb);

			return RValue<SByte8>(V(result));
		}
	}

	RValue<SByte8> SubSat(RValue<SByte8> x, RValue<SByte8> y)
	{
		if(emulateIntrinsics)
		{
			SByte8 result;
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 0)) - Int(Extract(y, 0)))), 0);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 1)) - Int(Extract(y, 1)))), 1);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 2)) - Int(Extract(y, 2)))), 2);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 3)) - Int(Extract(y, 3)))), 3);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 4)) - Int(Extract(y, 4)))), 4);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 5)) - Int(Extract(y, 5)))), 5);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 6)) - Int(Extract(y, 6)))), 6);
			result = Insert(result, SaturateSigned(Short(Int(Extract(x, 7)) - Int(Extract(y, 7)))), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v16i8);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SubtractSaturateSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto psubsb = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			psubsb->addArg(x.value);
			psubsb->addArg(y.value);
			::basicBlock->appendInst(psubsb);

			return RValue<SByte8>(V(result));
		}
	}

	RValue<Short4> UnpackLow(RValue<SByte8> x, RValue<SByte8> y)
	{
		int shuffle[16] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};   // Real type is v16i8
		return As<Short4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Short4> UnpackHigh(RValue<SByte8> x, RValue<SByte8> y)
	{
		int shuffle[16] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};   // Real type is v16i8
		auto lowHigh = RValue<Byte16>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
		return As<Short4>(Swizzle(As<Int4>(lowHigh), 0xEE));
	}

	RValue<Int> SignMask(RValue<SByte8> x)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			SByte8 xx = (x >> 7) & SByte8(0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80);
			return Int(Extract(xx, 0)) | Int(Extract(xx, 1)) | Int(Extract(xx, 2)) | Int(Extract(xx, 3)) | Int(Extract(xx, 4)) | Int(Extract(xx, 5)) | Int(Extract(xx, 6)) | Int(Extract(xx, 7));
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_i32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SignMask, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto movmsk = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			movmsk->addArg(x.value);
			::basicBlock->appendInst(movmsk);

			return RValue<Int>(V(result)) & 0xFF;
		}
	}

	RValue<Byte8> CmpGT(RValue<SByte8> x, RValue<SByte8> y)
	{
		return RValue<Byte8>(createIntCompare(Ice::InstIcmp::Sgt, x.value, y.value));
	}

	RValue<Byte8> CmpEQ(RValue<SByte8> x, RValue<SByte8> y)
	{
		return RValue<Byte8>(Nucleus::createICmpEQ(x.value, y.value));
	}

	Type *SByte8::getType()
	{
		return T(Type_v8i8);
	}

	Byte16::Byte16(RValue<Byte16> rhs)
	{
		storeValue(rhs.value);
	}

	Byte16::Byte16(const Byte16 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Byte16::Byte16(const Reference<Byte16> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Byte16> Byte16::operator=(RValue<Byte16> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Byte16> Byte16::operator=(const Byte16 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Byte16>(value);
	}

	RValue<Byte16> Byte16::operator=(const Reference<Byte16> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Byte16>(value);
	}

	Type *Byte16::getType()
	{
		return T(Ice::IceType_v16i8);
	}

	Type *SByte16::getType()
	{
		return T(Ice::IceType_v16i8);
	}

	Short2::Short2(RValue<Short4> cast)
	{
		storeValue(Nucleus::createBitCast(cast.value, getType()));
	}

	Type *Short2::getType()
	{
		return T(Type_v2i16);
	}

	UShort2::UShort2(RValue<UShort4> cast)
	{
		storeValue(Nucleus::createBitCast(cast.value, getType()));
	}

	Type *UShort2::getType()
	{
		return T(Type_v2i16);
	}

	Short4::Short4(RValue<Int> cast)
	{
		Value *vector = loadValue();
		Value *element = Nucleus::createTrunc(cast.value, Short::getType());
		Value *insert = Nucleus::createInsertElement(vector, element, 0);
		Value *swizzle = Swizzle(RValue<Short4>(insert), 0x00).value;

		storeValue(swizzle);
	}

	Short4::Short4(RValue<Int4> cast)
	{
		int select[8] = {0, 2, 4, 6, 0, 2, 4, 6};
		Value *short8 = Nucleus::createBitCast(cast.value, Short8::getType());
		Value *packed = Nucleus::createShuffleVector(short8, short8, select);

		Value *int2 = RValue<Int2>(Int2(As<Int4>(packed))).value;
		Value *short4 = Nucleus::createBitCast(int2, Short4::getType());

		storeValue(short4);
	}

//	Short4::Short4(RValue<Float> cast)
//	{
//	}

	Short4::Short4(RValue<Float4> cast)
	{
		assert(false && "UNIMPLEMENTED");
	}

	Short4::Short4(short xyzw)
	{
		int64_t constantVector[4] = {xyzw, xyzw, xyzw, xyzw};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Short4::Short4(short x, short y, short z, short w)
	{
		int64_t constantVector[4] = {x, y, z, w};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Short4::Short4(RValue<Short4> rhs)
	{
		storeValue(rhs.value);
	}

	Short4::Short4(const Short4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Short4::Short4(const Reference<Short4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Short4::Short4(RValue<UShort4> rhs)
	{
		storeValue(rhs.value);
	}

	Short4::Short4(const UShort4 &rhs)
	{
		storeValue(rhs.loadValue());
	}

	Short4::Short4(const Reference<UShort4> &rhs)
	{
		storeValue(rhs.loadValue());
	}

	RValue<Short4> Short4::operator=(RValue<Short4> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Short4> Short4::operator=(const Short4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Short4>(value);
	}

	RValue<Short4> Short4::operator=(const Reference<Short4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Short4>(value);
	}

	RValue<Short4> Short4::operator=(RValue<UShort4> rhs)
	{
		storeValue(rhs.value);

		return RValue<Short4>(rhs);
	}

	RValue<Short4> Short4::operator=(const UShort4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Short4>(value);
	}

	RValue<Short4> Short4::operator=(const Reference<UShort4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Short4>(value);
	}

	RValue<Short4> operator+(RValue<Short4> lhs, RValue<Short4> rhs)
	{
		return RValue<Short4>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Short4> operator-(RValue<Short4> lhs, RValue<Short4> rhs)
	{
		return RValue<Short4>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<Short4> operator*(RValue<Short4> lhs, RValue<Short4> rhs)
	{
		return RValue<Short4>(Nucleus::createMul(lhs.value, rhs.value));
	}

//	RValue<Short4> operator/(RValue<Short4> lhs, RValue<Short4> rhs)
//	{
//		return RValue<Short4>(Nucleus::createSDiv(lhs.value, rhs.value));
//	}

//	RValue<Short4> operator%(RValue<Short4> lhs, RValue<Short4> rhs)
//	{
//		return RValue<Short4>(Nucleus::createSRem(lhs.value, rhs.value));
//	}

	RValue<Short4> operator&(RValue<Short4> lhs, RValue<Short4> rhs)
	{
		return RValue<Short4>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Short4> operator|(RValue<Short4> lhs, RValue<Short4> rhs)
	{
		return RValue<Short4>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Short4> operator^(RValue<Short4> lhs, RValue<Short4> rhs)
	{
		return RValue<Short4>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<Short4> operator<<(RValue<Short4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Short4 result;
			result = Insert(result, Extract(lhs, 0) << Short(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << Short(rhs), 1);
			result = Insert(result, Extract(lhs, 2) << Short(rhs), 2);
			result = Insert(result, Extract(lhs, 3) << Short(rhs), 3);

			return result;
		}
		else
		{
			return RValue<Short4>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Short4> operator>>(RValue<Short4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Short4 result;
			result = Insert(result, Extract(lhs, 0) >> Short(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> Short(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> Short(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> Short(rhs), 3);

			return result;
		}
		else
		{
			return RValue<Short4>(Nucleus::createAShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Short4> operator+=(Short4 &lhs, RValue<Short4> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Short4> operator-=(Short4 &lhs, RValue<Short4> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Short4> operator*=(Short4 &lhs, RValue<Short4> rhs)
	{
		return lhs = lhs * rhs;
	}

//	RValue<Short4> operator/=(Short4 &lhs, RValue<Short4> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<Short4> operator%=(Short4 &lhs, RValue<Short4> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<Short4> operator&=(Short4 &lhs, RValue<Short4> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Short4> operator|=(Short4 &lhs, RValue<Short4> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Short4> operator^=(Short4 &lhs, RValue<Short4> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<Short4> operator<<=(Short4 &lhs, unsigned char rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<Short4> operator>>=(Short4 &lhs, unsigned char rhs)
	{
		return lhs = lhs >> rhs;
	}

//	RValue<Short4> operator+(RValue<Short4> val)
//	{
//		return val;
//	}

	RValue<Short4> operator-(RValue<Short4> val)
	{
		return RValue<Short4>(Nucleus::createNeg(val.value));
	}

	RValue<Short4> operator~(RValue<Short4> val)
	{
		return RValue<Short4>(Nucleus::createNot(val.value));
	}

	RValue<Short4> RoundShort4(RValue<Float4> cast)
	{
		RValue<Int4> int4 = RoundInt(cast);
		return As<Short4>(PackSigned(int4, int4));
	}

	RValue<Short4> Max(RValue<Short4> x, RValue<Short4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v8i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Sle, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<Short4>(V(result));
	}

	RValue<Short4> Min(RValue<Short4> x, RValue<Short4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v8i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Sgt, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<Short4>(V(result));
	}

	RValue<Short> SaturateSigned(RValue<Int> x)
	{
		return Short(IfThenElse(x > 0x7FFF, Int(0x7FFF), IfThenElse(x < -0x8000, Int(0x8000), x)));
	}

	RValue<Short4> AddSat(RValue<Short4> x, RValue<Short4> y)
	{
		if(emulateIntrinsics)
		{
			Short4 result;
			result = Insert(result, SaturateSigned(Int(Extract(x, 0)) + Int(Extract(y, 0))), 0);
			result = Insert(result, SaturateSigned(Int(Extract(x, 1)) + Int(Extract(y, 1))), 1);
			result = Insert(result, SaturateSigned(Int(Extract(x, 2)) + Int(Extract(y, 2))), 2);
			result = Insert(result, SaturateSigned(Int(Extract(x, 3)) + Int(Extract(y, 3))), 3);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::AddSaturateSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto paddsw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			paddsw->addArg(x.value);
			paddsw->addArg(y.value);
			::basicBlock->appendInst(paddsw);

			return RValue<Short4>(V(result));
		}
	}

	RValue<Short4> SubSat(RValue<Short4> x, RValue<Short4> y)
	{
		if(emulateIntrinsics)
		{
			Short4 result;
			result = Insert(result, SaturateSigned(Int(Extract(x, 0)) - Int(Extract(y, 0))), 0);
			result = Insert(result, SaturateSigned(Int(Extract(x, 1)) - Int(Extract(y, 1))), 1);
			result = Insert(result, SaturateSigned(Int(Extract(x, 2)) - Int(Extract(y, 2))), 2);
			result = Insert(result, SaturateSigned(Int(Extract(x, 3)) - Int(Extract(y, 3))), 3);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SubtractSaturateSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto psubsw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			psubsw->addArg(x.value);
			psubsw->addArg(y.value);
			::basicBlock->appendInst(psubsw);

			return RValue<Short4>(V(result));
		}
	}

	RValue<Short4> MulHigh(RValue<Short4> x, RValue<Short4> y)
	{
		if(emulateIntrinsics)
		{
			Short4 result;
			result = Insert(result, Short((Int(Extract(x, 0)) * Int(Extract(y, 0))) >> 16), 0);
			result = Insert(result, Short((Int(Extract(x, 1)) * Int(Extract(y, 1))) >> 16), 1);
			result = Insert(result, Short((Int(Extract(x, 2)) * Int(Extract(y, 2))) >> 16), 2);
			result = Insert(result, Short((Int(Extract(x, 3)) * Int(Extract(y, 3))) >> 16), 3);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::MultiplyHighSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pmulhw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pmulhw->addArg(x.value);
			pmulhw->addArg(y.value);
			::basicBlock->appendInst(pmulhw);

			return RValue<Short4>(V(result));
		}
	}

	RValue<Int2> MulAdd(RValue<Short4> x, RValue<Short4> y)
	{
		if(emulateIntrinsics)
		{
			Int2 result;
			result = Insert(result, Int(Extract(x, 0)) * Int(Extract(y, 0)) + Int(Extract(x, 1)) * Int(Extract(y, 1)), 0);
			result = Insert(result, Int(Extract(x, 2)) * Int(Extract(y, 2)) + Int(Extract(x, 3)) * Int(Extract(y, 3)), 1);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::MultiplyAddPairs, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pmaddwd = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pmaddwd->addArg(x.value);
			pmaddwd->addArg(y.value);
			::basicBlock->appendInst(pmaddwd);

			return As<Int2>(V(result));
		}
	}

	RValue<SByte8> PackSigned(RValue<Short4> x, RValue<Short4> y)
	{
		if(emulateIntrinsics)
		{
			SByte8 result;
			result = Insert(result, SaturateSigned(Extract(x, 0)), 0);
			result = Insert(result, SaturateSigned(Extract(x, 1)), 1);
			result = Insert(result, SaturateSigned(Extract(x, 2)), 2);
			result = Insert(result, SaturateSigned(Extract(x, 3)), 3);
			result = Insert(result, SaturateSigned(Extract(y, 0)), 4);
			result = Insert(result, SaturateSigned(Extract(y, 1)), 5);
			result = Insert(result, SaturateSigned(Extract(y, 2)), 6);
			result = Insert(result, SaturateSigned(Extract(y, 3)), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v16i8);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::VectorPackSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pack = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pack->addArg(x.value);
			pack->addArg(y.value);
			::basicBlock->appendInst(pack);

			return As<SByte8>(Swizzle(As<Int4>(V(result)), 0x88));
		}
	}

	RValue<Byte8> PackUnsigned(RValue<Short4> x, RValue<Short4> y)
	{
		if(emulateIntrinsics)
		{
			Byte8 result;
			result = Insert(result, SaturateUnsigned(Extract(x, 0)), 0);
			result = Insert(result, SaturateUnsigned(Extract(x, 1)), 1);
			result = Insert(result, SaturateUnsigned(Extract(x, 2)), 2);
			result = Insert(result, SaturateUnsigned(Extract(x, 3)), 3);
			result = Insert(result, SaturateUnsigned(Extract(y, 0)), 4);
			result = Insert(result, SaturateUnsigned(Extract(y, 1)), 5);
			result = Insert(result, SaturateUnsigned(Extract(y, 2)), 6);
			result = Insert(result, SaturateUnsigned(Extract(y, 3)), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v16i8);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::VectorPackUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pack = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pack->addArg(x.value);
			pack->addArg(y.value);
			::basicBlock->appendInst(pack);

			return As<Byte8>(Swizzle(As<Int4>(V(result)), 0x88));
		}
	}

	RValue<Int2> UnpackLow(RValue<Short4> x, RValue<Short4> y)
	{
		int shuffle[8] = {0, 8, 1, 9, 2, 10, 3, 11};   // Real type is v8i16
		return As<Int2>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Int2> UnpackHigh(RValue<Short4> x, RValue<Short4> y)
	{
		int shuffle[8] = {0, 8, 1, 9, 2, 10, 3, 11};   // Real type is v8i16
		auto lowHigh = RValue<Short8>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
		return As<Int2>(Swizzle(As<Int4>(lowHigh), 0xEE));
	}

	RValue<Short4> Swizzle(RValue<Short4> x, unsigned char select)
	{
		// Real type is v8i16
		int shuffle[8] =
		{
			(select >> 0) & 0x03,
			(select >> 2) & 0x03,
			(select >> 4) & 0x03,
			(select >> 6) & 0x03,
			(select >> 0) & 0x03,
			(select >> 2) & 0x03,
			(select >> 4) & 0x03,
			(select >> 6) & 0x03,
		};

		return RValue<Short4>(Nucleus::createShuffleVector(x.value, x.value, shuffle));
	}

	RValue<Short4> Insert(RValue<Short4> val, RValue<Short> element, int i)
	{
		return RValue<Short4>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<Short> Extract(RValue<Short4> val, int i)
	{
		return RValue<Short>(Nucleus::createExtractElement(val.value, Short::getType(), i));
	}

	RValue<Short4> CmpGT(RValue<Short4> x, RValue<Short4> y)
	{
		return RValue<Short4>(createIntCompare(Ice::InstIcmp::Sgt, x.value, y.value));
	}

	RValue<Short4> CmpEQ(RValue<Short4> x, RValue<Short4> y)
	{
		return RValue<Short4>(Nucleus::createICmpEQ(x.value, y.value));
	}

	Type *Short4::getType()
	{
		return T(Type_v4i16);
	}

	UShort4::UShort4(RValue<Int4> cast)
	{
		*this = Short4(cast);
	}

	UShort4::UShort4(RValue<Float4> cast, bool saturate)
	{
		if(saturate)
		{
			if(CPUID::SSE4_1)
			{
				// x86 produces 0x80000000 on 32-bit integer overflow/underflow.
				// PackUnsigned takes care of 0x0000 saturation.
				Int4 int4(Min(cast, Float4(0xFFFF)));
				*this = As<UShort4>(PackUnsigned(int4, int4));
			}
			else if(CPUID::ARM)
			{
				// ARM saturates the 32-bit integer result on overflow/undeflow.
				Int4 int4(cast);
				*this = As<UShort4>(PackUnsigned(int4, int4));
			}
			else
			{
				*this = Short4(Int4(Max(Min(cast, Float4(0xFFFF)), Float4(0x0000))));
			}
		}
		else
		{
			*this = Short4(Int4(cast));
		}
	}

	UShort4::UShort4(unsigned short xyzw)
	{
		int64_t constantVector[4] = {xyzw, xyzw, xyzw, xyzw};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	UShort4::UShort4(unsigned short x, unsigned short y, unsigned short z, unsigned short w)
	{
		int64_t constantVector[4] = {x, y, z, w};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	UShort4::UShort4(RValue<UShort4> rhs)
	{
		storeValue(rhs.value);
	}

	UShort4::UShort4(const UShort4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UShort4::UShort4(const Reference<UShort4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UShort4::UShort4(RValue<Short4> rhs)
	{
		storeValue(rhs.value);
	}

	UShort4::UShort4(const Short4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UShort4::UShort4(const Reference<Short4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<UShort4> UShort4::operator=(RValue<UShort4> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<UShort4> UShort4::operator=(const UShort4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort4>(value);
	}

	RValue<UShort4> UShort4::operator=(const Reference<UShort4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort4>(value);
	}

	RValue<UShort4> UShort4::operator=(RValue<Short4> rhs)
	{
		storeValue(rhs.value);

		return RValue<UShort4>(rhs);
	}

	RValue<UShort4> UShort4::operator=(const Short4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort4>(value);
	}

	RValue<UShort4> UShort4::operator=(const Reference<Short4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort4>(value);
	}

	RValue<UShort4> operator+(RValue<UShort4> lhs, RValue<UShort4> rhs)
	{
		return RValue<UShort4>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<UShort4> operator-(RValue<UShort4> lhs, RValue<UShort4> rhs)
	{
		return RValue<UShort4>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<UShort4> operator*(RValue<UShort4> lhs, RValue<UShort4> rhs)
	{
		return RValue<UShort4>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<UShort4> operator&(RValue<UShort4> lhs, RValue<UShort4> rhs)
	{
		return RValue<UShort4>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<UShort4> operator|(RValue<UShort4> lhs, RValue<UShort4> rhs)
	{
		return RValue<UShort4>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<UShort4> operator^(RValue<UShort4> lhs, RValue<UShort4> rhs)
	{
		return RValue<UShort4>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<UShort> Extract(RValue<UShort4> val, int i)
	{
		return RValue<UShort>(Nucleus::createExtractElement(val.value, UShort::getType(), i));
	}

	RValue<UShort4> Insert(RValue<UShort4> val, RValue<UShort> element, int i)
	{
		return RValue<UShort4>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<UShort4> operator<<(RValue<UShort4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UShort4 result;
			result = Insert(result, Extract(lhs, 0) << UShort(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << UShort(rhs), 1);
			result = Insert(result, Extract(lhs, 2) << UShort(rhs), 2);
			result = Insert(result, Extract(lhs, 3) << UShort(rhs), 3);

			return result;
		}
		else
		{
			return RValue<UShort4>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UShort4> operator>>(RValue<UShort4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UShort4 result;
			result = Insert(result, Extract(lhs, 0) >> UShort(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> UShort(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> UShort(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> UShort(rhs), 3);

			return result;
		}
		else
		{
			return RValue<UShort4>(Nucleus::createLShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UShort4> operator<<=(UShort4 &lhs, unsigned char rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<UShort4> operator>>=(UShort4 &lhs, unsigned char rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<UShort4> operator~(RValue<UShort4> val)
	{
		return RValue<UShort4>(Nucleus::createNot(val.value));
	}

	RValue<UShort4> Max(RValue<UShort4> x, RValue<UShort4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v8i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Ule, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<UShort4>(V(result));
	}

	RValue<UShort4> Min(RValue<UShort4> x, RValue<UShort4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v8i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Ugt, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<UShort4>(V(result));
	}

	RValue<UShort> SaturateUnsigned(RValue<Int> x)
	{
		return UShort(IfThenElse(x > 0xFFFF, Int(0xFFFF), IfThenElse(x < 0, Int(0), x)));
	}

	RValue<UShort4> AddSat(RValue<UShort4> x, RValue<UShort4> y)
	{
		if(emulateIntrinsics)
		{
			UShort4 result;
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 0)) + Int(Extract(y, 0))), 0);
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 1)) + Int(Extract(y, 1))), 1);
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 2)) + Int(Extract(y, 2))), 2);
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 3)) + Int(Extract(y, 3))), 3);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::AddSaturateUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto paddusw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			paddusw->addArg(x.value);
			paddusw->addArg(y.value);
			::basicBlock->appendInst(paddusw);

			return RValue<UShort4>(V(result));
		}
	}

	RValue<UShort4> SubSat(RValue<UShort4> x, RValue<UShort4> y)
	{
		if(emulateIntrinsics)
		{
			UShort4 result;
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 0)) - Int(Extract(y, 0))), 0);
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 1)) - Int(Extract(y, 1))), 1);
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 2)) - Int(Extract(y, 2))), 2);
			result = Insert(result, SaturateUnsigned(Int(Extract(x, 3)) - Int(Extract(y, 3))), 3);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SubtractSaturateUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto psubusw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			psubusw->addArg(x.value);
			psubusw->addArg(y.value);
			::basicBlock->appendInst(psubusw);

			return RValue<UShort4>(V(result));
		}
	}

	RValue<UShort4> MulHigh(RValue<UShort4> x, RValue<UShort4> y)
	{
		if(emulateIntrinsics)
		{
			UShort4 result;
			result = Insert(result, UShort((UInt(Extract(x, 0)) * UInt(Extract(y, 0))) >> 16), 0);
			result = Insert(result, UShort((UInt(Extract(x, 1)) * UInt(Extract(y, 1))) >> 16), 1);
			result = Insert(result, UShort((UInt(Extract(x, 2)) * UInt(Extract(y, 2))) >> 16), 2);
			result = Insert(result, UShort((UInt(Extract(x, 3)) * UInt(Extract(y, 3))) >> 16), 3);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::MultiplyHighUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pmulhuw = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pmulhuw->addArg(x.value);
			pmulhuw->addArg(y.value);
			::basicBlock->appendInst(pmulhuw);

			return RValue<UShort4>(V(result));
		}
	}

	RValue<UShort4> Average(RValue<UShort4> x, RValue<UShort4> y)
	{
		assert(false && "UNIMPLEMENTED"); return RValue<UShort4>(V(nullptr));
	}

	Type *UShort4::getType()
	{
		return T(Type_v4i16);
	}

	Short8::Short8(short c)
	{
		int64_t constantVector[8] = {c, c, c, c, c, c, c, c};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Short8::Short8(short c0, short c1, short c2, short c3, short c4, short c5, short c6, short c7)
	{
		int64_t constantVector[8] = {c0, c1, c2, c3, c4, c5, c6, c7};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Short8::Short8(RValue<Short8> rhs)
	{
		storeValue(rhs.value);
	}

	Short8::Short8(const Reference<Short8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Short8::Short8(RValue<Short4> lo, RValue<Short4> hi)
	{
		int shuffle[8] = {0, 1, 2, 3, 8, 9, 10, 11};   // Real type is v8i16
		Value *packed = Nucleus::createShuffleVector(lo.value, hi.value, shuffle);

		storeValue(packed);
	}

	RValue<Short8> operator+(RValue<Short8> lhs, RValue<Short8> rhs)
	{
		return RValue<Short8>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Short8> operator&(RValue<Short8> lhs, RValue<Short8> rhs)
	{
		return RValue<Short8>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Short> Extract(RValue<Short8> val, int i)
	{
		return RValue<Short>(Nucleus::createExtractElement(val.value, Short::getType(), i));
	}

	RValue<Short8> Insert(RValue<Short8> val, RValue<Short> element, int i)
	{
		return RValue<Short8>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<Short8> operator<<(RValue<Short8> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Short8 result;
			result = Insert(result, Extract(lhs, 0) << Short(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << Short(rhs), 1);
			result = Insert(result, Extract(lhs, 2) << Short(rhs), 2);
			result = Insert(result, Extract(lhs, 3) << Short(rhs), 3);
			result = Insert(result, Extract(lhs, 4) << Short(rhs), 4);
			result = Insert(result, Extract(lhs, 5) << Short(rhs), 5);
			result = Insert(result, Extract(lhs, 6) << Short(rhs), 6);
			result = Insert(result, Extract(lhs, 7) << Short(rhs), 7);

			return result;
		}
		else
		{
			return RValue<Short8>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Short8> operator>>(RValue<Short8> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Short8 result;
			result = Insert(result, Extract(lhs, 0) >> Short(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> Short(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> Short(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> Short(rhs), 3);
			result = Insert(result, Extract(lhs, 4) >> Short(rhs), 4);
			result = Insert(result, Extract(lhs, 5) >> Short(rhs), 5);
			result = Insert(result, Extract(lhs, 6) >> Short(rhs), 6);
			result = Insert(result, Extract(lhs, 7) >> Short(rhs), 7);

			return result;
		}
		else
		{
			return RValue<Short8>(Nucleus::createAShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Int4> MulAdd(RValue<Short8> x, RValue<Short8> y)
	{
		assert(false && "UNIMPLEMENTED"); return RValue<Int4>(V(nullptr));
	}

	RValue<Int4> Abs(RValue<Int4> x)
	{
		auto negative = x >> 31;
		return (x ^ negative) - negative;
	}

	RValue<Short8> MulHigh(RValue<Short8> x, RValue<Short8> y)
	{
		assert(false && "UNIMPLEMENTED"); return RValue<Short8>(V(nullptr));
	}

	Type *Short8::getType()
	{
		return T(Ice::IceType_v8i16);
	}

	UShort8::UShort8(unsigned short c)
	{
		int64_t constantVector[8] = {c, c, c, c, c, c, c, c};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	UShort8::UShort8(unsigned short c0, unsigned short c1, unsigned short c2, unsigned short c3, unsigned short c4, unsigned short c5, unsigned short c6, unsigned short c7)
	{
		int64_t constantVector[8] = {c0, c1, c2, c3, c4, c5, c6, c7};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	UShort8::UShort8(RValue<UShort8> rhs)
	{
		storeValue(rhs.value);
	}

	UShort8::UShort8(const Reference<UShort8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UShort8::UShort8(RValue<UShort4> lo, RValue<UShort4> hi)
	{
		int shuffle[8] = {0, 1, 2, 3, 8, 9, 10, 11};   // Real type is v8i16
		Value *packed = Nucleus::createShuffleVector(lo.value, hi.value, shuffle);

		storeValue(packed);
	}

	RValue<UShort8> UShort8::operator=(RValue<UShort8> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<UShort8> UShort8::operator=(const UShort8 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort8>(value);
	}

	RValue<UShort8> UShort8::operator=(const Reference<UShort8> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UShort8>(value);
	}

	RValue<UShort8> operator&(RValue<UShort8> lhs, RValue<UShort8> rhs)
	{
		return RValue<UShort8>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<UShort> Extract(RValue<UShort8> val, int i)
	{
		return RValue<UShort>(Nucleus::createExtractElement(val.value, UShort::getType(), i));
	}

	RValue<UShort8> Insert(RValue<UShort8> val, RValue<UShort> element, int i)
	{
		return RValue<UShort8>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<UShort8> operator<<(RValue<UShort8> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UShort8 result;
			result = Insert(result, Extract(lhs, 0) << UShort(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << UShort(rhs), 1);
			result = Insert(result, Extract(lhs, 2) << UShort(rhs), 2);
			result = Insert(result, Extract(lhs, 3) << UShort(rhs), 3);
			result = Insert(result, Extract(lhs, 4) << UShort(rhs), 4);
			result = Insert(result, Extract(lhs, 5) << UShort(rhs), 5);
			result = Insert(result, Extract(lhs, 6) << UShort(rhs), 6);
			result = Insert(result, Extract(lhs, 7) << UShort(rhs), 7);

			return result;
		}
		else
		{
			return RValue<UShort8>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UShort8> operator>>(RValue<UShort8> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UShort8 result;
			result = Insert(result, Extract(lhs, 0) >> UShort(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> UShort(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> UShort(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> UShort(rhs), 3);
			result = Insert(result, Extract(lhs, 4) >> UShort(rhs), 4);
			result = Insert(result, Extract(lhs, 5) >> UShort(rhs), 5);
			result = Insert(result, Extract(lhs, 6) >> UShort(rhs), 6);
			result = Insert(result, Extract(lhs, 7) >> UShort(rhs), 7);

			return result;
		}
		else
		{
			return RValue<UShort8>(Nucleus::createLShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UShort8> operator+(RValue<UShort8> lhs, RValue<UShort8> rhs)
	{
		return RValue<UShort8>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<UShort8> operator*(RValue<UShort8> lhs, RValue<UShort8> rhs)
	{
		return RValue<UShort8>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<UShort8> operator+=(UShort8 &lhs, RValue<UShort8> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<UShort8> operator~(RValue<UShort8> val)
	{
		return RValue<UShort8>(Nucleus::createNot(val.value));
	}

	RValue<UShort8> Swizzle(RValue<UShort8> x, char select0, char select1, char select2, char select3, char select4, char select5, char select6, char select7)
	{
		assert(false && "UNIMPLEMENTED"); return RValue<UShort8>(V(nullptr));
	}

	RValue<UShort8> MulHigh(RValue<UShort8> x, RValue<UShort8> y)
	{
		assert(false && "UNIMPLEMENTED"); return RValue<UShort8>(V(nullptr));
	}

	// FIXME: Implement as Shuffle(x, y, Select(i0, ..., i16)) and Shuffle(x, y, SELECT_PACK_REPEAT(element))
//	RValue<UShort8> PackRepeat(RValue<Byte16> x, RValue<Byte16> y, int element)
//	{
//		assert(false && "UNIMPLEMENTED"); return RValue<UShort8>(V(nullptr));
//	}

	Type *UShort8::getType()
	{
		return T(Ice::IceType_v8i16);
	}

	Int::Int(Argument<Int> argument)
	{
		storeValue(argument.value);
	}

	Int::Int(RValue<Byte> cast)
	{
		Value *integer = Nucleus::createZExt(cast.value, Int::getType());

		storeValue(integer);
	}

	Int::Int(RValue<SByte> cast)
	{
		Value *integer = Nucleus::createSExt(cast.value, Int::getType());

		storeValue(integer);
	}

	Int::Int(RValue<Short> cast)
	{
		Value *integer = Nucleus::createSExt(cast.value, Int::getType());

		storeValue(integer);
	}

	Int::Int(RValue<UShort> cast)
	{
		Value *integer = Nucleus::createZExt(cast.value, Int::getType());

		storeValue(integer);
	}

	Int::Int(RValue<Int2> cast)
	{
		*this = Extract(cast, 0);
	}

	Int::Int(RValue<Long> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, Int::getType());

		storeValue(integer);
	}

	Int::Int(RValue<Float> cast)
	{
		Value *integer = Nucleus::createFPToSI(cast.value, Int::getType());

		storeValue(integer);
	}

	Int::Int(int x)
	{
		storeValue(Nucleus::createConstantInt(x));
	}

	Int::Int(RValue<Int> rhs)
	{
		storeValue(rhs.value);
	}

	Int::Int(RValue<UInt> rhs)
	{
		storeValue(rhs.value);
	}

	Int::Int(const Int &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int::Int(const Reference<Int> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int::Int(const UInt &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int::Int(const Reference<UInt> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Int> Int::operator=(int rhs)
	{
		return RValue<Int>(storeValue(Nucleus::createConstantInt(rhs)));
	}

	RValue<Int> Int::operator=(RValue<Int> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Int> Int::operator=(RValue<UInt> rhs)
	{
		storeValue(rhs.value);

		return RValue<Int>(rhs);
	}

	RValue<Int> Int::operator=(const Int &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int>(value);
	}

	RValue<Int> Int::operator=(const Reference<Int> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int>(value);
	}

	RValue<Int> Int::operator=(const UInt &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int>(value);
	}

	RValue<Int> Int::operator=(const Reference<UInt> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int>(value);
	}

	RValue<Int> operator+(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Int> operator-(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<Int> operator*(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<Int> operator/(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createSDiv(lhs.value, rhs.value));
	}

	RValue<Int> operator%(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createSRem(lhs.value, rhs.value));
	}

	RValue<Int> operator&(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Int> operator|(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Int> operator^(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<Int> operator<<(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<Int> operator>>(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Int>(Nucleus::createAShr(lhs.value, rhs.value));
	}

	RValue<Int> operator+=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Int> operator-=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Int> operator*=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<Int> operator/=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<Int> operator%=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<Int> operator&=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Int> operator|=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Int> operator^=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<Int> operator<<=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<Int> operator>>=(Int &lhs, RValue<Int> rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<Int> operator+(RValue<Int> val)
	{
		return val;
	}

	RValue<Int> operator-(RValue<Int> val)
	{
		return RValue<Int>(Nucleus::createNeg(val.value));
	}

	RValue<Int> operator~(RValue<Int> val)
	{
		return RValue<Int>(Nucleus::createNot(val.value));
	}

	RValue<Int> operator++(Int &val, int)   // Post-increment
	{
		RValue<Int> res = val;
		val += 1;
		return res;
	}

	const Int &operator++(Int &val)   // Pre-increment
	{
		val += 1;
		return val;
	}

	RValue<Int> operator--(Int &val, int)   // Post-decrement
	{
		RValue<Int> res = val;
		val -= 1;
		return res;
	}

	const Int &operator--(Int &val)   // Pre-decrement
	{
		val -= 1;
		return val;
	}

	RValue<Bool> operator<(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSLT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSLE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpSGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpNE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<Int> lhs, RValue<Int> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpEQ(lhs.value, rhs.value));
	}

	RValue<Int> Max(RValue<Int> x, RValue<Int> y)
	{
		return IfThenElse(x > y, x, y);
	}

	RValue<Int> Min(RValue<Int> x, RValue<Int> y)
	{
		return IfThenElse(x < y, x, y);
	}

	RValue<Int> Clamp(RValue<Int> x, RValue<Int> min, RValue<Int> max)
	{
		return Min(Max(x, min), max);
	}

	RValue<Int> RoundInt(RValue<Float> cast)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			// Push the fractional part off the mantissa. Accurate up to +/-2^22.
			return Int((cast + Float(0x00C00000)) - Float(0x00C00000));
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_i32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Nearbyint, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto nearbyint = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			nearbyint->addArg(cast.value);
			::basicBlock->appendInst(nearbyint);

			return RValue<Int>(V(result));
		}
	}

	Type *Int::getType()
	{
		return T(Ice::IceType_i32);
	}

	Long::Long(RValue<Int> cast)
	{
		Value *integer = Nucleus::createSExt(cast.value, Long::getType());

		storeValue(integer);
	}

	Long::Long(RValue<UInt> cast)
	{
		Value *integer = Nucleus::createZExt(cast.value, Long::getType());

		storeValue(integer);
	}

	Long::Long(RValue<Long> rhs)
	{
		storeValue(rhs.value);
	}

	RValue<Long> Long::operator=(int64_t rhs)
	{
		return RValue<Long>(storeValue(Nucleus::createConstantLong(rhs)));
	}

	RValue<Long> Long::operator=(RValue<Long> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Long> Long::operator=(const Long &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Long>(value);
	}

	RValue<Long> Long::operator=(const Reference<Long> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Long>(value);
	}

	RValue<Long> operator+(RValue<Long> lhs, RValue<Long> rhs)
	{
		return RValue<Long>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Long> operator-(RValue<Long> lhs, RValue<Long> rhs)
	{
		return RValue<Long>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<Long> operator+=(Long &lhs, RValue<Long> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Long> operator-=(Long &lhs, RValue<Long> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Long> AddAtomic(RValue<Pointer<Long> > x, RValue<Long> y)
	{
		return RValue<Long>(Nucleus::createAtomicAdd(x.value, y.value));
	}

	Type *Long::getType()
	{
		return T(Ice::IceType_i64);
	}

	UInt::UInt(Argument<UInt> argument)
	{
		storeValue(argument.value);
	}

	UInt::UInt(RValue<UShort> cast)
	{
		Value *integer = Nucleus::createZExt(cast.value, UInt::getType());

		storeValue(integer);
	}

	UInt::UInt(RValue<Long> cast)
	{
		Value *integer = Nucleus::createTrunc(cast.value, UInt::getType());

		storeValue(integer);
	}

	UInt::UInt(RValue<Float> cast)
	{
		// Smallest positive value representable in UInt, but not in Int
		const unsigned int ustart = 0x80000000u;
		const float ustartf = float(ustart);

		// If the value is negative, store 0, otherwise store the result of the conversion
		storeValue((~(As<Int>(cast) >> 31) &
		// Check if the value can be represented as an Int
			IfThenElse(cast >= ustartf,
		// If the value is too large, subtract ustart and re-add it after conversion.
				As<Int>(As<UInt>(Int(cast - Float(ustartf))) + UInt(ustart)),
		// Otherwise, just convert normally
				Int(cast))).value);
	}

	UInt::UInt(int x)
	{
		storeValue(Nucleus::createConstantInt(x));
	}

	UInt::UInt(unsigned int x)
	{
		storeValue(Nucleus::createConstantInt(x));
	}

	UInt::UInt(RValue<UInt> rhs)
	{
		storeValue(rhs.value);
	}

	UInt::UInt(RValue<Int> rhs)
	{
		storeValue(rhs.value);
	}

	UInt::UInt(const UInt &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt::UInt(const Reference<UInt> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt::UInt(const Int &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt::UInt(const Reference<Int> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<UInt> UInt::operator=(unsigned int rhs)
	{
		return RValue<UInt>(storeValue(Nucleus::createConstantInt(rhs)));
	}

	RValue<UInt> UInt::operator=(RValue<UInt> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<UInt> UInt::operator=(RValue<Int> rhs)
	{
		storeValue(rhs.value);

		return RValue<UInt>(rhs);
	}

	RValue<UInt> UInt::operator=(const UInt &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt>(value);
	}

	RValue<UInt> UInt::operator=(const Reference<UInt> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt>(value);
	}

	RValue<UInt> UInt::operator=(const Int &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt>(value);
	}

	RValue<UInt> UInt::operator=(const Reference<Int> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt>(value);
	}

	RValue<UInt> operator+(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<UInt> operator-(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<UInt> operator*(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<UInt> operator/(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createUDiv(lhs.value, rhs.value));
	}

	RValue<UInt> operator%(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createURem(lhs.value, rhs.value));
	}

	RValue<UInt> operator&(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<UInt> operator|(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<UInt> operator^(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<UInt> operator<<(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<UInt> operator>>(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<UInt>(Nucleus::createLShr(lhs.value, rhs.value));
	}

	RValue<UInt> operator+=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<UInt> operator-=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<UInt> operator*=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<UInt> operator/=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<UInt> operator%=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<UInt> operator&=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<UInt> operator|=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<UInt> operator^=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<UInt> operator<<=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<UInt> operator>>=(UInt &lhs, RValue<UInt> rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<UInt> operator+(RValue<UInt> val)
	{
		return val;
	}

	RValue<UInt> operator-(RValue<UInt> val)
	{
		return RValue<UInt>(Nucleus::createNeg(val.value));
	}

	RValue<UInt> operator~(RValue<UInt> val)
	{
		return RValue<UInt>(Nucleus::createNot(val.value));
	}

	RValue<UInt> operator++(UInt &val, int)   // Post-increment
	{
		RValue<UInt> res = val;
		val += 1;
		return res;
	}

	const UInt &operator++(UInt &val)   // Pre-increment
	{
		val += 1;
		return val;
	}

	RValue<UInt> operator--(UInt &val, int)   // Post-decrement
	{
		RValue<UInt> res = val;
		val -= 1;
		return res;
	}

	const UInt &operator--(UInt &val)   // Pre-decrement
	{
		val -= 1;
		return val;
	}

	RValue<UInt> Max(RValue<UInt> x, RValue<UInt> y)
	{
		return IfThenElse(x > y, x, y);
	}

	RValue<UInt> Min(RValue<UInt> x, RValue<UInt> y)
	{
		return IfThenElse(x < y, x, y);
	}

	RValue<UInt> Clamp(RValue<UInt> x, RValue<UInt> min, RValue<UInt> max)
	{
		return Min(Max(x, min), max);
	}

	RValue<Bool> operator<(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpULT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpULE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpUGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpUGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpNE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<UInt> lhs, RValue<UInt> rhs)
	{
		return RValue<Bool>(Nucleus::createICmpEQ(lhs.value, rhs.value));
	}

//	RValue<UInt> RoundUInt(RValue<Float> cast)
//	{
//		assert(false && "UNIMPLEMENTED"); return RValue<UInt>(V(nullptr));
//	}

	Type *UInt::getType()
	{
		return T(Ice::IceType_i32);
	}

//	Int2::Int2(RValue<Int> cast)
//	{
//		Value *extend = Nucleus::createZExt(cast.value, Long::getType());
//		Value *vector = Nucleus::createBitCast(extend, Int2::getType());
//
//		Constant *shuffle[2];
//		shuffle[0] = Nucleus::createConstantInt(0);
//		shuffle[1] = Nucleus::createConstantInt(0);
//
//		Value *replicate = Nucleus::createShuffleVector(vector, UndefValue::get(Int2::getType()), Nucleus::createConstantVector(shuffle, 2));
//
//		storeValue(replicate);
//	}

	Int2::Int2(RValue<Int4> cast)
	{
		storeValue(Nucleus::createBitCast(cast.value, getType()));
	}

	Int2::Int2(int x, int y)
	{
		int64_t constantVector[2] = {x, y};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Int2::Int2(RValue<Int2> rhs)
	{
		storeValue(rhs.value);
	}

	Int2::Int2(const Int2 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int2::Int2(const Reference<Int2> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int2::Int2(RValue<Int> lo, RValue<Int> hi)
	{
		int shuffle[4] = {0, 4, 1, 5};
		Value *packed = Nucleus::createShuffleVector(Int4(lo).loadValue(), Int4(hi).loadValue(), shuffle);

		storeValue(Nucleus::createBitCast(packed, Int2::getType()));
	}

	RValue<Int2> Int2::operator=(RValue<Int2> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Int2> Int2::operator=(const Int2 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int2>(value);
	}

	RValue<Int2> Int2::operator=(const Reference<Int2> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int2>(value);
	}

	RValue<Int2> operator+(RValue<Int2> lhs, RValue<Int2> rhs)
	{
		return RValue<Int2>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Int2> operator-(RValue<Int2> lhs, RValue<Int2> rhs)
	{
		return RValue<Int2>(Nucleus::createSub(lhs.value, rhs.value));
	}

//	RValue<Int2> operator*(RValue<Int2> lhs, RValue<Int2> rhs)
//	{
//		return RValue<Int2>(Nucleus::createMul(lhs.value, rhs.value));
//	}

//	RValue<Int2> operator/(RValue<Int2> lhs, RValue<Int2> rhs)
//	{
//		return RValue<Int2>(Nucleus::createSDiv(lhs.value, rhs.value));
//	}

//	RValue<Int2> operator%(RValue<Int2> lhs, RValue<Int2> rhs)
//	{
//		return RValue<Int2>(Nucleus::createSRem(lhs.value, rhs.value));
//	}

	RValue<Int2> operator&(RValue<Int2> lhs, RValue<Int2> rhs)
	{
		return RValue<Int2>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Int2> operator|(RValue<Int2> lhs, RValue<Int2> rhs)
	{
		return RValue<Int2>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Int2> operator^(RValue<Int2> lhs, RValue<Int2> rhs)
	{
		return RValue<Int2>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<Int2> operator<<(RValue<Int2> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Int2 result;
			result = Insert(result, Extract(lhs, 0) << Int(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << Int(rhs), 1);

			return result;
		}
		else
		{
			return RValue<Int2>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Int2> operator>>(RValue<Int2> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Int2 result;
			result = Insert(result, Extract(lhs, 0) >> Int(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> Int(rhs), 1);

			return result;
		}
		else
		{
			return RValue<Int2>(Nucleus::createAShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Int2> operator+=(Int2 &lhs, RValue<Int2> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Int2> operator-=(Int2 &lhs, RValue<Int2> rhs)
	{
		return lhs = lhs - rhs;
	}

//	RValue<Int2> operator*=(Int2 &lhs, RValue<Int2> rhs)
//	{
//		return lhs = lhs * rhs;
//	}

//	RValue<Int2> operator/=(Int2 &lhs, RValue<Int2> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<Int2> operator%=(Int2 &lhs, RValue<Int2> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<Int2> operator&=(Int2 &lhs, RValue<Int2> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Int2> operator|=(Int2 &lhs, RValue<Int2> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Int2> operator^=(Int2 &lhs, RValue<Int2> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<Int2> operator<<=(Int2 &lhs, unsigned char rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<Int2> operator>>=(Int2 &lhs, unsigned char rhs)
	{
		return lhs = lhs >> rhs;
	}

//	RValue<Int2> operator+(RValue<Int2> val)
//	{
//		return val;
//	}

//	RValue<Int2> operator-(RValue<Int2> val)
//	{
//		return RValue<Int2>(Nucleus::createNeg(val.value));
//	}

	RValue<Int2> operator~(RValue<Int2> val)
	{
		return RValue<Int2>(Nucleus::createNot(val.value));
	}

	RValue<Short4> UnpackLow(RValue<Int2> x, RValue<Int2> y)
	{
		int shuffle[4] = {0, 4, 1, 5};   // Real type is v4i32
		return As<Short4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Short4> UnpackHigh(RValue<Int2> x, RValue<Int2> y)
	{
		int shuffle[4] = {0, 4, 1, 5};   // Real type is v4i32
		auto lowHigh = RValue<Int4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
		return As<Short4>(Swizzle(lowHigh, 0xEE));
	}

	RValue<Int> Extract(RValue<Int2> val, int i)
	{
		return RValue<Int>(Nucleus::createExtractElement(val.value, Int::getType(), i));
	}

	RValue<Int2> Insert(RValue<Int2> val, RValue<Int> element, int i)
	{
		return RValue<Int2>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	Type *Int2::getType()
	{
		return T(Type_v2i32);
	}

	UInt2::UInt2(unsigned int x, unsigned int y)
	{
		int64_t constantVector[2] = {x, y};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	UInt2::UInt2(RValue<UInt2> rhs)
	{
		storeValue(rhs.value);
	}

	UInt2::UInt2(const UInt2 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt2::UInt2(const Reference<UInt2> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<UInt2> UInt2::operator=(RValue<UInt2> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<UInt2> UInt2::operator=(const UInt2 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt2>(value);
	}

	RValue<UInt2> UInt2::operator=(const Reference<UInt2> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt2>(value);
	}

	RValue<UInt2> operator+(RValue<UInt2> lhs, RValue<UInt2> rhs)
	{
		return RValue<UInt2>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<UInt2> operator-(RValue<UInt2> lhs, RValue<UInt2> rhs)
	{
		return RValue<UInt2>(Nucleus::createSub(lhs.value, rhs.value));
	}

//	RValue<UInt2> operator*(RValue<UInt2> lhs, RValue<UInt2> rhs)
//	{
//		return RValue<UInt2>(Nucleus::createMul(lhs.value, rhs.value));
//	}

//	RValue<UInt2> operator/(RValue<UInt2> lhs, RValue<UInt2> rhs)
//	{
//		return RValue<UInt2>(Nucleus::createUDiv(lhs.value, rhs.value));
//	}

//	RValue<UInt2> operator%(RValue<UInt2> lhs, RValue<UInt2> rhs)
//	{
//		return RValue<UInt2>(Nucleus::createURem(lhs.value, rhs.value));
//	}

	RValue<UInt2> operator&(RValue<UInt2> lhs, RValue<UInt2> rhs)
	{
		return RValue<UInt2>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<UInt2> operator|(RValue<UInt2> lhs, RValue<UInt2> rhs)
	{
		return RValue<UInt2>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<UInt2> operator^(RValue<UInt2> lhs, RValue<UInt2> rhs)
	{
		return RValue<UInt2>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<UInt> Extract(RValue<UInt2> val, int i)
	{
		return RValue<UInt>(Nucleus::createExtractElement(val.value, UInt::getType(), i));
	}

	RValue<UInt2> Insert(RValue<UInt2> val, RValue<UInt> element, int i)
	{
		return RValue<UInt2>(Nucleus::createInsertElement(val.value, element.value, i));
	}

	RValue<UInt2> operator<<(RValue<UInt2> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UInt2 result;
			result = Insert(result, Extract(lhs, 0) << UInt(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << UInt(rhs), 1);

			return result;
		}
		else
		{
			return RValue<UInt2>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UInt2> operator>>(RValue<UInt2> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UInt2 result;
			result = Insert(result, Extract(lhs, 0) >> UInt(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> UInt(rhs), 1);

			return result;
		}
		else
		{
			return RValue<UInt2>(Nucleus::createLShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UInt2> operator+=(UInt2 &lhs, RValue<UInt2> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<UInt2> operator-=(UInt2 &lhs, RValue<UInt2> rhs)
	{
		return lhs = lhs - rhs;
	}

//	RValue<UInt2> operator*=(UInt2 &lhs, RValue<UInt2> rhs)
//	{
//		return lhs = lhs * rhs;
//	}

//	RValue<UInt2> operator/=(UInt2 &lhs, RValue<UInt2> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<UInt2> operator%=(UInt2 &lhs, RValue<UInt2> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<UInt2> operator&=(UInt2 &lhs, RValue<UInt2> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<UInt2> operator|=(UInt2 &lhs, RValue<UInt2> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<UInt2> operator^=(UInt2 &lhs, RValue<UInt2> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<UInt2> operator<<=(UInt2 &lhs, unsigned char rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<UInt2> operator>>=(UInt2 &lhs, unsigned char rhs)
	{
		return lhs = lhs >> rhs;
	}

//	RValue<UInt2> operator+(RValue<UInt2> val)
//	{
//		return val;
//	}

//	RValue<UInt2> operator-(RValue<UInt2> val)
//	{
//		return RValue<UInt2>(Nucleus::createNeg(val.value));
//	}

	RValue<UInt2> operator~(RValue<UInt2> val)
	{
		return RValue<UInt2>(Nucleus::createNot(val.value));
	}

	Type *UInt2::getType()
	{
		return T(Type_v2i32);
	}

	Int4::Int4() : XYZW(this)
	{
	}

	Int4::Int4(RValue<Byte4> cast) : XYZW(this)
	{
		Value *x = Nucleus::createBitCast(cast.value, Int::getType());
		Value *a = Nucleus::createInsertElement(loadValue(), x, 0);

		Value *e;
		int swizzle[16] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};
		Value *b = Nucleus::createBitCast(a, Byte16::getType());
		Value *c = Nucleus::createShuffleVector(b, V(Nucleus::createNullValue(Byte16::getType())), swizzle);

		int swizzle2[8] = {0, 8, 1, 9, 2, 10, 3, 11};
		Value *d = Nucleus::createBitCast(c, Short8::getType());
		e = Nucleus::createShuffleVector(d, V(Nucleus::createNullValue(Short8::getType())), swizzle2);

		Value *f = Nucleus::createBitCast(e, Int4::getType());
		storeValue(f);
	}

	Int4::Int4(RValue<SByte4> cast) : XYZW(this)
	{
		Value *x = Nucleus::createBitCast(cast.value, Int::getType());
		Value *a = Nucleus::createInsertElement(loadValue(), x, 0);

		int swizzle[16] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
		Value *b = Nucleus::createBitCast(a, Byte16::getType());
		Value *c = Nucleus::createShuffleVector(b, b, swizzle);

		int swizzle2[8] = {0, 0, 1, 1, 2, 2, 3, 3};
		Value *d = Nucleus::createBitCast(c, Short8::getType());
		Value *e = Nucleus::createShuffleVector(d, d, swizzle2);

		*this = As<Int4>(e) >> 24;
	}

	Int4::Int4(RValue<Float4> cast) : XYZW(this)
	{
		Value *xyzw = Nucleus::createFPToSI(cast.value, Int4::getType());

		storeValue(xyzw);
	}

	Int4::Int4(RValue<Short4> cast) : XYZW(this)
	{
		int swizzle[8] = {0, 0, 1, 1, 2, 2, 3, 3};
		Value *c = Nucleus::createShuffleVector(cast.value, cast.value, swizzle);

		*this = As<Int4>(c) >> 16;
	}

	Int4::Int4(RValue<UShort4> cast) : XYZW(this)
	{
		int swizzle[8] = {0, 8, 1, 9, 2, 10, 3, 11};
		Value *c = Nucleus::createShuffleVector(cast.value, Short8(0, 0, 0, 0, 0, 0, 0, 0).loadValue(), swizzle);
		Value *d = Nucleus::createBitCast(c, Int4::getType());
		storeValue(d);
	}

	Int4::Int4(int xyzw) : XYZW(this)
	{
		constant(xyzw, xyzw, xyzw, xyzw);
	}

	Int4::Int4(int x, int yzw) : XYZW(this)
	{
		constant(x, yzw, yzw, yzw);
	}

	Int4::Int4(int x, int y, int zw) : XYZW(this)
	{
		constant(x, y, zw, zw);
	}

	Int4::Int4(int x, int y, int z, int w) : XYZW(this)
	{
		constant(x, y, z, w);
	}

	void Int4::constant(int x, int y, int z, int w)
	{
		int64_t constantVector[4] = {x, y, z, w};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Int4::Int4(RValue<Int4> rhs) : XYZW(this)
	{
		storeValue(rhs.value);
	}

	Int4::Int4(const Int4 &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int4::Int4(const Reference<Int4> &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int4::Int4(RValue<UInt4> rhs) : XYZW(this)
	{
		storeValue(rhs.value);
	}

	Int4::Int4(const UInt4 &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int4::Int4(const Reference<UInt4> &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Int4::Int4(RValue<Int2> lo, RValue<Int2> hi) : XYZW(this)
	{
		int shuffle[4] = {0, 1, 4, 5};   // Real type is v4i32
		Value *packed = Nucleus::createShuffleVector(lo.value, hi.value, shuffle);

		storeValue(packed);
	}

	Int4::Int4(RValue<Int> rhs) : XYZW(this)
	{
		Value *vector = Nucleus::createBitCast(rhs.value, Int4::getType());

		int swizzle[4] = {0, 0, 0, 0};
		Value *replicate = Nucleus::createShuffleVector(vector, vector, swizzle);

		storeValue(replicate);
	}

	Int4::Int4(const Int &rhs) : XYZW(this)
	{
		*this = RValue<Int>(rhs.loadValue());
	}

	Int4::Int4(const Reference<Int> &rhs) : XYZW(this)
	{
		*this = RValue<Int>(rhs.loadValue());
	}

	RValue<Int4> Int4::operator=(RValue<Int4> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Int4> Int4::operator=(const Int4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int4>(value);
	}

	RValue<Int4> Int4::operator=(const Reference<Int4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Int4>(value);
	}

	RValue<Int4> operator+(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<Int4> operator-(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<Int4> operator*(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<Int4> operator/(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createSDiv(lhs.value, rhs.value));
	}

	RValue<Int4> operator%(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createSRem(lhs.value, rhs.value));
	}

	RValue<Int4> operator&(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<Int4> operator|(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<Int4> operator^(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<Int4> operator<<(RValue<Int4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Int4 result;
			result = Insert(result, Extract(lhs, 0) << Int(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << Int(rhs), 1);
			result = Insert(result, Extract(lhs, 2) << Int(rhs), 2);
			result = Insert(result, Extract(lhs, 3) << Int(rhs), 3);

			return result;
		}
		else
		{
			return RValue<Int4>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Int4> operator>>(RValue<Int4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			Int4 result;
			result = Insert(result, Extract(lhs, 0) >> Int(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> Int(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> Int(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> Int(rhs), 3);

			return result;
		}
		else
		{
			return RValue<Int4>(Nucleus::createAShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<Int4> operator<<(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<Int4> operator>>(RValue<Int4> lhs, RValue<Int4> rhs)
	{
		return RValue<Int4>(Nucleus::createAShr(lhs.value, rhs.value));
	}

	RValue<Int4> operator+=(Int4 &lhs, RValue<Int4> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Int4> operator-=(Int4 &lhs, RValue<Int4> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Int4> operator*=(Int4 &lhs, RValue<Int4> rhs)
	{
		return lhs = lhs * rhs;
	}

//	RValue<Int4> operator/=(Int4 &lhs, RValue<Int4> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<Int4> operator%=(Int4 &lhs, RValue<Int4> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<Int4> operator&=(Int4 &lhs, RValue<Int4> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<Int4> operator|=(Int4 &lhs, RValue<Int4> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<Int4> operator^=(Int4 &lhs, RValue<Int4> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<Int4> operator<<=(Int4 &lhs, unsigned char rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<Int4> operator>>=(Int4 &lhs, unsigned char rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<Int4> operator+(RValue<Int4> val)
	{
		return val;
	}

	RValue<Int4> operator-(RValue<Int4> val)
	{
		return RValue<Int4>(Nucleus::createNeg(val.value));
	}

	RValue<Int4> operator~(RValue<Int4> val)
	{
		return RValue<Int4>(Nucleus::createNot(val.value));
	}

	RValue<Int4> CmpEQ(RValue<Int4> x, RValue<Int4> y)
	{
		return RValue<Int4>(Nucleus::createICmpEQ(x.value, y.value));
	}

	RValue<Int4> CmpLT(RValue<Int4> x, RValue<Int4> y)
	{
		return RValue<Int4>(Nucleus::createICmpSLT(x.value, y.value));
	}

	RValue<Int4> CmpLE(RValue<Int4> x, RValue<Int4> y)
	{
		return RValue<Int4>(Nucleus::createICmpSLE(x.value, y.value));
	}

	RValue<Int4> CmpNEQ(RValue<Int4> x, RValue<Int4> y)
	{
		return RValue<Int4>(Nucleus::createICmpNE(x.value, y.value));
	}

	RValue<Int4> CmpNLT(RValue<Int4> x, RValue<Int4> y)
	{
		return RValue<Int4>(Nucleus::createICmpSGE(x.value, y.value));
	}

	RValue<Int4> CmpNLE(RValue<Int4> x, RValue<Int4> y)
	{
		return RValue<Int4>(Nucleus::createICmpSGT(x.value, y.value));
	}

	RValue<Int4> Max(RValue<Int4> x, RValue<Int4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v4i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Sle, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4i32);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<Int4>(V(result));
	}

	RValue<Int4> Min(RValue<Int4> x, RValue<Int4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v4i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Sgt, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4i32);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<Int4>(V(result));
	}

	RValue<Int4> RoundInt(RValue<Float4> cast)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			// Push the fractional part off the mantissa. Accurate up to +/-2^22.
			return Int4((cast + Float4(0x00C00000)) - Float4(0x00C00000));
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4i32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Nearbyint, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto nearbyint = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			nearbyint->addArg(cast.value);
			::basicBlock->appendInst(nearbyint);

			return RValue<Int4>(V(result));
		}
	}

	RValue<Short8> PackSigned(RValue<Int4> x, RValue<Int4> y)
	{
		if(emulateIntrinsics)
		{
			Short8 result;
			result = Insert(result, SaturateSigned(Extract(x, 0)), 0);
			result = Insert(result, SaturateSigned(Extract(x, 1)), 1);
			result = Insert(result, SaturateSigned(Extract(x, 2)), 2);
			result = Insert(result, SaturateSigned(Extract(x, 3)), 3);
			result = Insert(result, SaturateSigned(Extract(y, 0)), 4);
			result = Insert(result, SaturateSigned(Extract(y, 1)), 5);
			result = Insert(result, SaturateSigned(Extract(y, 2)), 6);
			result = Insert(result, SaturateSigned(Extract(y, 3)), 7);

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::VectorPackSigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pack = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pack->addArg(x.value);
			pack->addArg(y.value);
			::basicBlock->appendInst(pack);

			return RValue<Short8>(V(result));
		}
	}

	RValue<UShort8> PackUnsigned(RValue<Int4> x, RValue<Int4> y)
	{
		if(emulateIntrinsics || !(CPUID::SSE4_1 || CPUID::ARM))
		{
			RValue<Int4> sx = As<Int4>(x);
			RValue<Int4> bx = (sx & ~(sx >> 31)) - Int4(0x8000);

			RValue<Int4> sy = As<Int4>(y);
			RValue<Int4> by = (sy & ~(sy >> 31)) - Int4(0x8000);

			return As<UShort8>(PackSigned(bx, by) + Short8(0x8000u));
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v8i16);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::VectorPackUnsigned, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto pack = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			pack->addArg(x.value);
			pack->addArg(y.value);
			::basicBlock->appendInst(pack);

			return RValue<UShort8>(V(result));
		}
	}

	RValue<Int> Extract(RValue<Int4> x, int i)
	{
		return RValue<Int>(Nucleus::createExtractElement(x.value, Int::getType(), i));
	}

	RValue<Int4> Insert(RValue<Int4> x, RValue<Int> element, int i)
	{
		return RValue<Int4>(Nucleus::createInsertElement(x.value, element.value, i));
	}

	RValue<Int> SignMask(RValue<Int4> x)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			Int4 xx = (x >> 31) & Int4(0x00000001, 0x00000002, 0x00000004, 0x00000008);
			return Extract(xx, 0) | Extract(xx, 1) | Extract(xx, 2) | Extract(xx, 3);
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_i32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SignMask, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto movmsk = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			movmsk->addArg(x.value);
			::basicBlock->appendInst(movmsk);

			return RValue<Int>(V(result));
		}
	}

	RValue<Int4> Swizzle(RValue<Int4> x, unsigned char select)
	{
		return RValue<Int4>(createSwizzle4(x.value, select));
	}

	Type *Int4::getType()
	{
		return T(Ice::IceType_v4i32);
	}

	UInt4::UInt4() : XYZW(this)
	{
	}

	UInt4::UInt4(RValue<Float4> cast) : XYZW(this)
	{
		// Smallest positive value representable in UInt, but not in Int
		const unsigned int ustart = 0x80000000u;
		const float ustartf = float(ustart);

		// Check if the value can be represented as an Int
		Int4 uiValue = CmpNLT(cast, Float4(ustartf));
		// If the value is too large, subtract ustart and re-add it after conversion.
		uiValue = (uiValue & As<Int4>(As<UInt4>(Int4(cast - Float4(ustartf))) + UInt4(ustart))) |
		// Otherwise, just convert normally
		          (~uiValue & Int4(cast));
		// If the value is negative, store 0, otherwise store the result of the conversion
		storeValue((~(As<Int4>(cast) >> 31) & uiValue).value);
	}

	UInt4::UInt4(int xyzw) : XYZW(this)
	{
		constant(xyzw, xyzw, xyzw, xyzw);
	}

	UInt4::UInt4(int x, int yzw) : XYZW(this)
	{
		constant(x, yzw, yzw, yzw);
	}

	UInt4::UInt4(int x, int y, int zw) : XYZW(this)
	{
		constant(x, y, zw, zw);
	}

	UInt4::UInt4(int x, int y, int z, int w) : XYZW(this)
	{
		constant(x, y, z, w);
	}

	void UInt4::constant(int x, int y, int z, int w)
	{
		int64_t constantVector[4] = {x, y, z, w};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	UInt4::UInt4(RValue<UInt4> rhs) : XYZW(this)
	{
		storeValue(rhs.value);
	}

	UInt4::UInt4(const UInt4 &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt4::UInt4(const Reference<UInt4> &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt4::UInt4(RValue<Int4> rhs) : XYZW(this)
	{
		storeValue(rhs.value);
	}

	UInt4::UInt4(const Int4 &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt4::UInt4(const Reference<Int4> &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	UInt4::UInt4(RValue<UInt2> lo, RValue<UInt2> hi) : XYZW(this)
	{
		int shuffle[4] = {0, 1, 4, 5};   // Real type is v4i32
		Value *packed = Nucleus::createShuffleVector(lo.value, hi.value, shuffle);

		storeValue(packed);
	}

	RValue<UInt4> UInt4::operator=(RValue<UInt4> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<UInt4> UInt4::operator=(const UInt4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt4>(value);
	}

	RValue<UInt4> UInt4::operator=(const Reference<UInt4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<UInt4>(value);
	}

	RValue<UInt4> operator+(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createAdd(lhs.value, rhs.value));
	}

	RValue<UInt4> operator-(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createSub(lhs.value, rhs.value));
	}

	RValue<UInt4> operator*(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createMul(lhs.value, rhs.value));
	}

	RValue<UInt4> operator/(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createUDiv(lhs.value, rhs.value));
	}

	RValue<UInt4> operator%(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createURem(lhs.value, rhs.value));
	}

	RValue<UInt4> operator&(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createAnd(lhs.value, rhs.value));
	}

	RValue<UInt4> operator|(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createOr(lhs.value, rhs.value));
	}

	RValue<UInt4> operator^(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createXor(lhs.value, rhs.value));
	}

	RValue<UInt> Extract(RValue<UInt4> x, int i)
	{
		return RValue<UInt>(Nucleus::createExtractElement(x.value, UInt::getType(), i));
	}

	RValue<UInt4> Insert(RValue<UInt4> x, RValue<UInt> element, int i)
	{
		return RValue<UInt4>(Nucleus::createInsertElement(x.value, element.value, i));
	}

	RValue<UInt4> operator<<(RValue<UInt4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UInt4 result;
			result = Insert(result, Extract(lhs, 0) << UInt(rhs), 0);
			result = Insert(result, Extract(lhs, 1) << UInt(rhs), 1);
			result = Insert(result, Extract(lhs, 2) << UInt(rhs), 2);
			result = Insert(result, Extract(lhs, 3) << UInt(rhs), 3);

			return result;
		}
		else
		{
			return RValue<UInt4>(Nucleus::createShl(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UInt4> operator>>(RValue<UInt4> lhs, unsigned char rhs)
	{
		if(emulateIntrinsics)
		{
			UInt4 result;
			result = Insert(result, Extract(lhs, 0) >> UInt(rhs), 0);
			result = Insert(result, Extract(lhs, 1) >> UInt(rhs), 1);
			result = Insert(result, Extract(lhs, 2) >> UInt(rhs), 2);
			result = Insert(result, Extract(lhs, 3) >> UInt(rhs), 3);

			return result;
		}
		else
		{
			return RValue<UInt4>(Nucleus::createLShr(lhs.value, V(::context->getConstantInt32(rhs))));
		}
	}

	RValue<UInt4> operator<<(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createShl(lhs.value, rhs.value));
	}

	RValue<UInt4> operator>>(RValue<UInt4> lhs, RValue<UInt4> rhs)
	{
		return RValue<UInt4>(Nucleus::createLShr(lhs.value, rhs.value));
	}

	RValue<UInt4> operator+=(UInt4 &lhs, RValue<UInt4> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<UInt4> operator-=(UInt4 &lhs, RValue<UInt4> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<UInt4> operator*=(UInt4 &lhs, RValue<UInt4> rhs)
	{
		return lhs = lhs * rhs;
	}

//	RValue<UInt4> operator/=(UInt4 &lhs, RValue<UInt4> rhs)
//	{
//		return lhs = lhs / rhs;
//	}

//	RValue<UInt4> operator%=(UInt4 &lhs, RValue<UInt4> rhs)
//	{
//		return lhs = lhs % rhs;
//	}

	RValue<UInt4> operator&=(UInt4 &lhs, RValue<UInt4> rhs)
	{
		return lhs = lhs & rhs;
	}

	RValue<UInt4> operator|=(UInt4 &lhs, RValue<UInt4> rhs)
	{
		return lhs = lhs | rhs;
	}

	RValue<UInt4> operator^=(UInt4 &lhs, RValue<UInt4> rhs)
	{
		return lhs = lhs ^ rhs;
	}

	RValue<UInt4> operator<<=(UInt4 &lhs, unsigned char rhs)
	{
		return lhs = lhs << rhs;
	}

	RValue<UInt4> operator>>=(UInt4 &lhs, unsigned char rhs)
	{
		return lhs = lhs >> rhs;
	}

	RValue<UInt4> operator+(RValue<UInt4> val)
	{
		return val;
	}

	RValue<UInt4> operator-(RValue<UInt4> val)
	{
		return RValue<UInt4>(Nucleus::createNeg(val.value));
	}

	RValue<UInt4> operator~(RValue<UInt4> val)
	{
		return RValue<UInt4>(Nucleus::createNot(val.value));
	}

	RValue<UInt4> CmpEQ(RValue<UInt4> x, RValue<UInt4> y)
	{
		return RValue<UInt4>(Nucleus::createICmpEQ(x.value, y.value));
	}

	RValue<UInt4> CmpLT(RValue<UInt4> x, RValue<UInt4> y)
	{
		return RValue<UInt4>(Nucleus::createICmpULT(x.value, y.value));
	}

	RValue<UInt4> CmpLE(RValue<UInt4> x, RValue<UInt4> y)
	{
		return RValue<UInt4>(Nucleus::createICmpULE(x.value, y.value));
	}

	RValue<UInt4> CmpNEQ(RValue<UInt4> x, RValue<UInt4> y)
	{
		return RValue<UInt4>(Nucleus::createICmpNE(x.value, y.value));
	}

	RValue<UInt4> CmpNLT(RValue<UInt4> x, RValue<UInt4> y)
	{
		return RValue<UInt4>(Nucleus::createICmpUGE(x.value, y.value));
	}

	RValue<UInt4> CmpNLE(RValue<UInt4> x, RValue<UInt4> y)
	{
		return RValue<UInt4>(Nucleus::createICmpUGT(x.value, y.value));
	}

	RValue<UInt4> Max(RValue<UInt4> x, RValue<UInt4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v4i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Ule, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4i32);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<UInt4>(V(result));
	}

	RValue<UInt4> Min(RValue<UInt4> x, RValue<UInt4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v4i1);
		auto cmp = Ice::InstIcmp::create(::function, Ice::InstIcmp::Ugt, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4i32);
		auto select = Ice::InstSelect::create(::function, result, condition, y.value, x.value);
		::basicBlock->appendInst(select);

		return RValue<UInt4>(V(result));
	}

	Type *UInt4::getType()
	{
		return T(Ice::IceType_v4i32);
	}

	Float::Float(RValue<Int> cast)
	{
		Value *integer = Nucleus::createSIToFP(cast.value, Float::getType());

		storeValue(integer);
	}

	Float::Float(RValue<UInt> cast)
	{
		RValue<Float> result = Float(Int(cast & UInt(0x7FFFFFFF))) +
		                       As<Float>((As<Int>(cast) >> 31) & As<Int>(Float(0x80000000u)));

		storeValue(result.value);
	}

	Float::Float(float x)
	{
		storeValue(Nucleus::createConstantFloat(x));
	}

	Float::Float(RValue<Float> rhs)
	{
		storeValue(rhs.value);
	}

	Float::Float(const Float &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Float::Float(const Reference<Float> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	RValue<Float> Float::operator=(RValue<Float> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Float> Float::operator=(const Float &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Float>(value);
	}

	RValue<Float> Float::operator=(const Reference<Float> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Float>(value);
	}

	RValue<Float> operator+(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Float>(Nucleus::createFAdd(lhs.value, rhs.value));
	}

	RValue<Float> operator-(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Float>(Nucleus::createFSub(lhs.value, rhs.value));
	}

	RValue<Float> operator*(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Float>(Nucleus::createFMul(lhs.value, rhs.value));
	}

	RValue<Float> operator/(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Float>(Nucleus::createFDiv(lhs.value, rhs.value));
	}

	RValue<Float> operator+=(Float &lhs, RValue<Float> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Float> operator-=(Float &lhs, RValue<Float> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Float> operator*=(Float &lhs, RValue<Float> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<Float> operator/=(Float &lhs, RValue<Float> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<Float> operator+(RValue<Float> val)
	{
		return val;
	}

	RValue<Float> operator-(RValue<Float> val)
	{
		return RValue<Float>(Nucleus::createFNeg(val.value));
	}

	RValue<Bool> operator<(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Bool>(Nucleus::createFCmpOLT(lhs.value, rhs.value));
	}

	RValue<Bool> operator<=(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Bool>(Nucleus::createFCmpOLE(lhs.value, rhs.value));
	}

	RValue<Bool> operator>(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Bool>(Nucleus::createFCmpOGT(lhs.value, rhs.value));
	}

	RValue<Bool> operator>=(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Bool>(Nucleus::createFCmpOGE(lhs.value, rhs.value));
	}

	RValue<Bool> operator!=(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Bool>(Nucleus::createFCmpONE(lhs.value, rhs.value));
	}

	RValue<Bool> operator==(RValue<Float> lhs, RValue<Float> rhs)
	{
		return RValue<Bool>(Nucleus::createFCmpOEQ(lhs.value, rhs.value));
	}

	RValue<Float> Abs(RValue<Float> x)
	{
		return IfThenElse(x > 0.0f, x, -x);
	}

	RValue<Float> Max(RValue<Float> x, RValue<Float> y)
	{
		return IfThenElse(x > y, x, y);
	}

	RValue<Float> Min(RValue<Float> x, RValue<Float> y)
	{
		return IfThenElse(x < y, x, y);
	}

	RValue<Float> Rcp_pp(RValue<Float> x, bool exactAtPow2)
	{
		return 1.0f / x;
	}

	RValue<Float> RcpSqrt_pp(RValue<Float> x)
	{
		return Rcp_pp(Sqrt(x));
	}

	RValue<Float> Sqrt(RValue<Float> x)
	{
		Ice::Variable *result = ::function->makeVariable(Ice::IceType_f32);
		const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Sqrt, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
		auto target = ::context->getConstantUndef(Ice::IceType_i32);
		auto sqrt = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
		sqrt->addArg(x.value);
		::basicBlock->appendInst(sqrt);

		return RValue<Float>(V(result));
	}

	RValue<Float> Round(RValue<Float> x)
	{
		return Float4(Round(Float4(x))).x;
	}

	RValue<Float> Trunc(RValue<Float> x)
	{
		return Float4(Trunc(Float4(x))).x;
	}

	RValue<Float> Frac(RValue<Float> x)
	{
		return Float4(Frac(Float4(x))).x;
	}

	RValue<Float> Floor(RValue<Float> x)
	{
		return Float4(Floor(Float4(x))).x;
	}

	RValue<Float> Ceil(RValue<Float> x)
	{
		return Float4(Ceil(Float4(x))).x;
	}

	Type *Float::getType()
	{
		return T(Ice::IceType_f32);
	}

	Float2::Float2(RValue<Float4> cast)
	{
		storeValue(Nucleus::createBitCast(cast.value, getType()));
	}

	Type *Float2::getType()
	{
		return T(Type_v2f32);
	}

	Float4::Float4(RValue<Byte4> cast) : XYZW(this)
	{
		Value *a = Int4(cast).loadValue();
		Value *xyzw = Nucleus::createSIToFP(a, Float4::getType());

		storeValue(xyzw);
	}

	Float4::Float4(RValue<SByte4> cast) : XYZW(this)
	{
		Value *a = Int4(cast).loadValue();
		Value *xyzw = Nucleus::createSIToFP(a, Float4::getType());

		storeValue(xyzw);
	}

	Float4::Float4(RValue<Short4> cast) : XYZW(this)
	{
		Int4 c(cast);
		storeValue(Nucleus::createSIToFP(RValue<Int4>(c).value, Float4::getType()));
	}

	Float4::Float4(RValue<UShort4> cast) : XYZW(this)
	{
		Int4 c(cast);
		storeValue(Nucleus::createSIToFP(RValue<Int4>(c).value, Float4::getType()));
	}

	Float4::Float4(RValue<Int4> cast) : XYZW(this)
	{
		Value *xyzw = Nucleus::createSIToFP(cast.value, Float4::getType());

		storeValue(xyzw);
	}

	Float4::Float4(RValue<UInt4> cast) : XYZW(this)
	{
		RValue<Float4> result = Float4(Int4(cast & UInt4(0x7FFFFFFF))) +
		                        As<Float4>((As<Int4>(cast) >> 31) & As<Int4>(Float4(0x80000000u)));

		storeValue(result.value);
	}

	Float4::Float4() : XYZW(this)
	{
	}

	Float4::Float4(float xyzw) : XYZW(this)
	{
		constant(xyzw, xyzw, xyzw, xyzw);
	}

	Float4::Float4(float x, float yzw) : XYZW(this)
	{
		constant(x, yzw, yzw, yzw);
	}

	Float4::Float4(float x, float y, float zw) : XYZW(this)
	{
		constant(x, y, zw, zw);
	}

	Float4::Float4(float x, float y, float z, float w) : XYZW(this)
	{
		constant(x, y, z, w);
	}

	void Float4::constant(float x, float y, float z, float w)
	{
		double constantVector[4] = {x, y, z, w};
		storeValue(Nucleus::createConstantVector(constantVector, getType()));
	}

	Float4::Float4(RValue<Float4> rhs) : XYZW(this)
	{
		storeValue(rhs.value);
	}

	Float4::Float4(const Float4 &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Float4::Float4(const Reference<Float4> &rhs) : XYZW(this)
	{
		Value *value = rhs.loadValue();
		storeValue(value);
	}

	Float4::Float4(RValue<Float> rhs) : XYZW(this)
	{
		Value *vector = Nucleus::createBitCast(rhs.value, Float4::getType());

		int swizzle[4] = {0, 0, 0, 0};
		Value *replicate = Nucleus::createShuffleVector(vector, vector, swizzle);

		storeValue(replicate);
	}

	Float4::Float4(const Float &rhs) : XYZW(this)
	{
		*this = RValue<Float>(rhs.loadValue());
	}

	Float4::Float4(const Reference<Float> &rhs) : XYZW(this)
	{
		*this = RValue<Float>(rhs.loadValue());
	}

	RValue<Float4> Float4::operator=(float x)
	{
		return *this = Float4(x, x, x, x);
	}

	RValue<Float4> Float4::operator=(RValue<Float4> rhs)
	{
		storeValue(rhs.value);

		return rhs;
	}

	RValue<Float4> Float4::operator=(const Float4 &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Float4>(value);
	}

	RValue<Float4> Float4::operator=(const Reference<Float4> &rhs)
	{
		Value *value = rhs.loadValue();
		storeValue(value);

		return RValue<Float4>(value);
	}

	RValue<Float4> Float4::operator=(RValue<Float> rhs)
	{
		return *this = Float4(rhs);
	}

	RValue<Float4> Float4::operator=(const Float &rhs)
	{
		return *this = Float4(rhs);
	}

	RValue<Float4> Float4::operator=(const Reference<Float> &rhs)
	{
		return *this = Float4(rhs);
	}

	RValue<Float4> operator+(RValue<Float4> lhs, RValue<Float4> rhs)
	{
		return RValue<Float4>(Nucleus::createFAdd(lhs.value, rhs.value));
	}

	RValue<Float4> operator-(RValue<Float4> lhs, RValue<Float4> rhs)
	{
		return RValue<Float4>(Nucleus::createFSub(lhs.value, rhs.value));
	}

	RValue<Float4> operator*(RValue<Float4> lhs, RValue<Float4> rhs)
	{
		return RValue<Float4>(Nucleus::createFMul(lhs.value, rhs.value));
	}

	RValue<Float4> operator/(RValue<Float4> lhs, RValue<Float4> rhs)
	{
		return RValue<Float4>(Nucleus::createFDiv(lhs.value, rhs.value));
	}

	RValue<Float4> operator%(RValue<Float4> lhs, RValue<Float4> rhs)
	{
		return RValue<Float4>(Nucleus::createFRem(lhs.value, rhs.value));
	}

	RValue<Float4> operator+=(Float4 &lhs, RValue<Float4> rhs)
	{
		return lhs = lhs + rhs;
	}

	RValue<Float4> operator-=(Float4 &lhs, RValue<Float4> rhs)
	{
		return lhs = lhs - rhs;
	}

	RValue<Float4> operator*=(Float4 &lhs, RValue<Float4> rhs)
	{
		return lhs = lhs * rhs;
	}

	RValue<Float4> operator/=(Float4 &lhs, RValue<Float4> rhs)
	{
		return lhs = lhs / rhs;
	}

	RValue<Float4> operator%=(Float4 &lhs, RValue<Float4> rhs)
	{
		return lhs = lhs % rhs;
	}

	RValue<Float4> operator+(RValue<Float4> val)
	{
		return val;
	}

	RValue<Float4> operator-(RValue<Float4> val)
	{
		return RValue<Float4>(Nucleus::createFNeg(val.value));
	}

	RValue<Float4> Abs(RValue<Float4> x)
	{
		Value *vector = Nucleus::createBitCast(x.value, Int4::getType());
		int64_t constantVector[4] = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
		Value *result = Nucleus::createAnd(vector, V(Nucleus::createConstantVector(constantVector, Int4::getType())));

		return As<Float4>(result);
	}

	RValue<Float4> Max(RValue<Float4> x, RValue<Float4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v4i1);
		auto cmp = Ice::InstFcmp::create(::function, Ice::InstFcmp::Ogt, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
		auto select = Ice::InstSelect::create(::function, result, condition, x.value, y.value);
		::basicBlock->appendInst(select);

		return RValue<Float4>(V(result));
	}

	RValue<Float4> Min(RValue<Float4> x, RValue<Float4> y)
	{
		Ice::Variable *condition = ::function->makeVariable(Ice::IceType_v4i1);
		auto cmp = Ice::InstFcmp::create(::function, Ice::InstFcmp::Olt, condition, x.value, y.value);
		::basicBlock->appendInst(cmp);

		Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
		auto select = Ice::InstSelect::create(::function, result, condition, x.value, y.value);
		::basicBlock->appendInst(select);

		return RValue<Float4>(V(result));
	}

	RValue<Float4> Rcp_pp(RValue<Float4> x, bool exactAtPow2)
	{
		return Float4(1.0f) / x;
	}

	RValue<Float4> RcpSqrt_pp(RValue<Float4> x)
	{
		return Rcp_pp(Sqrt(x));
	}

	RValue<Float4> Sqrt(RValue<Float4> x)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			Float4 result;
			result.x = Sqrt(Float(Float4(x).x));
			result.y = Sqrt(Float(Float4(x).y));
			result.z = Sqrt(Float(Float4(x).z));
			result.w = Sqrt(Float(Float4(x).w));

			return result;
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Sqrt, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto sqrt = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			sqrt->addArg(x.value);
			::basicBlock->appendInst(sqrt);

			return RValue<Float4>(V(result));
		}
	}

	RValue<Float4> Insert(RValue<Float4> x, RValue<Float> element, int i)
	{
		return RValue<Float4>(Nucleus::createInsertElement(x.value, element.value, i));
	}

	RValue<Float> Extract(RValue<Float4> x, int i)
	{
		return RValue<Float>(Nucleus::createExtractElement(x.value, Float::getType(), i));
	}

	RValue<Float4> Swizzle(RValue<Float4> x, unsigned char select)
	{
		return RValue<Float4>(createSwizzle4(x.value, select));
	}

	RValue<Float4> ShuffleLowHigh(RValue<Float4> x, RValue<Float4> y, unsigned char imm)
	{
		int shuffle[4] =
		{
			((imm >> 0) & 0x03) + 0,
			((imm >> 2) & 0x03) + 0,
			((imm >> 4) & 0x03) + 4,
			((imm >> 6) & 0x03) + 4,
		};

		return RValue<Float4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Float4> UnpackLow(RValue<Float4> x, RValue<Float4> y)
	{
		int shuffle[4] = {0, 4, 1, 5};
		return RValue<Float4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Float4> UnpackHigh(RValue<Float4> x, RValue<Float4> y)
	{
		int shuffle[4] = {2, 6, 3, 7};
		return RValue<Float4>(Nucleus::createShuffleVector(x.value, y.value, shuffle));
	}

	RValue<Float4> Mask(Float4 &lhs, RValue<Float4> rhs, unsigned char select)
	{
		Value *vector = lhs.loadValue();
		Value *result = createMask4(vector, rhs.value, select);
		lhs.storeValue(result);

		return RValue<Float4>(result);
	}

	RValue<Int> SignMask(RValue<Float4> x)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			Int4 xx = (As<Int4>(x) >> 31) & Int4(0x00000001, 0x00000002, 0x00000004, 0x00000008);
			return Extract(xx, 0) | Extract(xx, 1) | Extract(xx, 2) | Extract(xx, 3);
		}
		else
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_i32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::SignMask, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto movmsk = Ice::InstIntrinsicCall::create(::function, 1, result, target, intrinsic);
			movmsk->addArg(x.value);
			::basicBlock->appendInst(movmsk);

			return RValue<Int>(V(result));
		}
	}

	RValue<Int4> CmpEQ(RValue<Float4> x, RValue<Float4> y)
	{
		return RValue<Int4>(Nucleus::createFCmpOEQ(x.value, y.value));
	}

	RValue<Int4> CmpLT(RValue<Float4> x, RValue<Float4> y)
	{
		return RValue<Int4>(Nucleus::createFCmpOLT(x.value, y.value));
	}

	RValue<Int4> CmpLE(RValue<Float4> x, RValue<Float4> y)
	{
		return RValue<Int4>(Nucleus::createFCmpOLE(x.value, y.value));
	}

	RValue<Int4> CmpNEQ(RValue<Float4> x, RValue<Float4> y)
	{
		return RValue<Int4>(Nucleus::createFCmpONE(x.value, y.value));
	}

	RValue<Int4> CmpNLT(RValue<Float4> x, RValue<Float4> y)
	{
		return RValue<Int4>(Nucleus::createFCmpOGE(x.value, y.value));
	}

	RValue<Int4> CmpNLE(RValue<Float4> x, RValue<Float4> y)
	{
		return RValue<Int4>(Nucleus::createFCmpOGT(x.value, y.value));
	}

	RValue<Int4> IsInf(RValue<Float4> x)
	{
		return CmpEQ(As<Int4>(x) & Int4(0x7FFFFFFF), Int4(0x7F800000));
	}

	RValue<Int4> IsNan(RValue<Float4> x)
	{
		return ~CmpEQ(x, x);
	}

	RValue<Float4> Round(RValue<Float4> x)
	{
		if(emulateIntrinsics || CPUID::ARM)
		{
			// Push the fractional part off the mantissa. Accurate up to +/-2^22.
			return (x + Float4(0x00C00000)) - Float4(0x00C00000);
		}
		else if(CPUID::SSE4_1)
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Round, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto round = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			round->addArg(x.value);
			round->addArg(::context->getConstantInt32(0));
			::basicBlock->appendInst(round);

			return RValue<Float4>(V(result));
		}
		else
		{
			return Float4(RoundInt(x));
		}
	}

	RValue<Float4> Trunc(RValue<Float4> x)
	{
		if(CPUID::SSE4_1)
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Round, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto round = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			round->addArg(x.value);
			round->addArg(::context->getConstantInt32(3));
			::basicBlock->appendInst(round);

			return RValue<Float4>(V(result));
		}
		else
		{
			return Float4(Int4(x));
		}
	}

	RValue<Float4> Frac(RValue<Float4> x)
	{
		Float4 frc;

		if(CPUID::SSE4_1)
		{
			frc = x - Floor(x);
		}
		else
		{
			frc = x - Float4(Int4(x));   // Signed fractional part.

			frc += As<Float4>(As<Int4>(CmpNLE(Float4(0.0f), frc)) & As<Int4>(Float4(1, 1, 1, 1)));   // Add 1.0 if negative.
		}

		// x - floor(x) can be 1.0 for very small negative x.
		// Clamp against the value just below 1.0.
		return Min(frc, As<Float4>(Int4(0x3F7FFFFF)));
	}

	RValue<Float4> Floor(RValue<Float4> x)
	{
		if(CPUID::SSE4_1)
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Round, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto round = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			round->addArg(x.value);
			round->addArg(::context->getConstantInt32(1));
			::basicBlock->appendInst(round);

			return RValue<Float4>(V(result));
		}
		else
		{
			return x - Frac(x);
		}
	}

	RValue<Float4> Ceil(RValue<Float4> x)
	{
		if(CPUID::SSE4_1)
		{
			Ice::Variable *result = ::function->makeVariable(Ice::IceType_v4f32);
			const Ice::Intrinsics::IntrinsicInfo intrinsic = {Ice::Intrinsics::Round, Ice::Intrinsics::SideEffects_F, Ice::Intrinsics::ReturnsTwice_F, Ice::Intrinsics::MemoryWrite_F};
			auto target = ::context->getConstantUndef(Ice::IceType_i32);
			auto round = Ice::InstIntrinsicCall::create(::function, 2, result, target, intrinsic);
			round->addArg(x.value);
			round->addArg(::context->getConstantInt32(2));
			::basicBlock->appendInst(round);

			return RValue<Float4>(V(result));
		}
		else
		{
			return -Floor(-x);
		}
	}

	Type *Float4::getType()
	{
		return T(Ice::IceType_v4f32);
	}

	RValue<Pointer<Byte>> operator+(RValue<Pointer<Byte>> lhs, int offset)
	{
		return lhs + RValue<Int>(Nucleus::createConstantInt(offset));
	}

	RValue<Pointer<Byte>> operator+(RValue<Pointer<Byte>> lhs, RValue<Int> offset)
	{
		return RValue<Pointer<Byte>>(Nucleus::createGEP(lhs.value, Byte::getType(), offset.value, false));
	}

	RValue<Pointer<Byte>> operator+(RValue<Pointer<Byte>> lhs, RValue<UInt> offset)
	{
		return RValue<Pointer<Byte>>(Nucleus::createGEP(lhs.value, Byte::getType(), offset.value, true));
	}

	RValue<Pointer<Byte>> operator+=(Pointer<Byte> &lhs, int offset)
	{
		return lhs = lhs + offset;
	}

	RValue<Pointer<Byte>> operator+=(Pointer<Byte> &lhs, RValue<Int> offset)
	{
		return lhs = lhs + offset;
	}

	RValue<Pointer<Byte>> operator+=(Pointer<Byte> &lhs, RValue<UInt> offset)
	{
		return lhs = lhs + offset;
	}

	RValue<Pointer<Byte>> operator-(RValue<Pointer<Byte>> lhs, int offset)
	{
		return lhs + -offset;
	}

	RValue<Pointer<Byte>> operator-(RValue<Pointer<Byte>> lhs, RValue<Int> offset)
	{
		return lhs + -offset;
	}

	RValue<Pointer<Byte>> operator-(RValue<Pointer<Byte>> lhs, RValue<UInt> offset)
	{
		return lhs + -offset;
	}

	RValue<Pointer<Byte>> operator-=(Pointer<Byte> &lhs, int offset)
	{
		return lhs = lhs - offset;
	}

	RValue<Pointer<Byte>> operator-=(Pointer<Byte> &lhs, RValue<Int> offset)
	{
		return lhs = lhs - offset;
	}

	RValue<Pointer<Byte>> operator-=(Pointer<Byte> &lhs, RValue<UInt> offset)
	{
		return lhs = lhs - offset;
	}

	void Return()
	{
		Nucleus::createRetVoid();
		Nucleus::setInsertBlock(Nucleus::createBasicBlock());
		Nucleus::createUnreachable();
	}

	void Return(RValue<Int> ret)
	{
		Nucleus::createRet(ret.value);
		Nucleus::setInsertBlock(Nucleus::createBasicBlock());
		Nucleus::createUnreachable();
	}

	void branch(RValue<Bool> cmp, BasicBlock *bodyBB, BasicBlock *endBB)
	{
		Nucleus::createCondBr(cmp.value, bodyBB, endBB);
		Nucleus::setInsertBlock(bodyBB);
	}

	RValue<Long> Ticks()
	{
		assert(false && "UNIMPLEMENTED"); return RValue<Long>(V(nullptr));
	}
}
