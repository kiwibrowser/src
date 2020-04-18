/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#if defined(__GLIBC__)
# define DYNAMIC_LOADING_SUPPORT 1
#else
# define DYNAMIC_LOADING_SUPPORT 0
#endif
#if DYNAMIC_LOADING_SUPPORT
# include <link.h>
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>

#include "breakpad/src/google_breakpad/common/minidump_format.h"
#if !DYNAMIC_LOADING_SUPPORT
# include "native_client/src/include/elf_constants.h"
#endif
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/include/nacl/nacl_minidump.h"
#include "native_client/src/untrusted/minidump_generator/build_id.h"


extern char __executable_start[];  // Start of code segment
extern char __etext[];  // End of code segment

#if defined(__GLIBC__)
// Variable defined by ld.so, used as a workaround for
// https://code.google.com/p/nativeclient/issues/detail?id=3431.
extern void *__libc_stack_end;
#endif

class MinidumpAllocator;

// Restrict how much of the stack we dump to reduce upload size and to
// avoid dynamic allocation.
static const size_t kLimitStackDumpSize = 512 * 1024;

static const size_t kLimitNonStackSize = 64 * 1024;

// The crash reporter is expected to be used in a situation where the
// current process is damaged or out of memory, so it avoids dynamic
// memory allocation and allocates a fixed-size buffer of the
// following size at startup.
static const size_t kMinidumpBufferSize =
    kLimitStackDumpSize + kLimitNonStackSize;

static const char *g_module_name = "main.nexe";
static nacl_minidump_callback_t g_callback_func;
static MinidumpAllocator *g_minidump_writer;
static int g_handling_exception = 0;

#if !DYNAMIC_LOADING_SUPPORT
static MDGUID g_module_build_id;
static int g_module_build_id_set;
#endif

class MinidumpAllocator {
  char *buf_;
  uint32_t buf_size_;
  uint32_t offset_;

 public:
  explicit MinidumpAllocator(uint32_t size) :
      buf_(NULL), buf_size_(0), offset_(0) {
    void *mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mapping == MAP_FAILED) {
      perror("minidump: Failed to allocate memory");
      return;
    }
    buf_ = (char *) mapping;
    buf_size_ = size;
  }

  ~MinidumpAllocator() {
    if (buf_ != NULL) {
      int result = munmap(buf_, buf_size_);
      assert(result == 0);
      // Suppress unused variable warning in case where assert is compiled out.
      (void) result;
    }
  }

  bool AllocateSpace(size_t size, char **ptr, uint32_t *position) {
    if (offset_ + size >= buf_size_)
      return false;
    *position = offset_;
    *ptr = buf_ + offset_;
    offset_ += size;
    memset(*ptr, 0, size);
    return true;
  }

  void *Alloc(size_t size) {
    char *ptr;
    uint32_t position;
    if (!AllocateSpace(size, &ptr, &position))
      return NULL;
    return ptr;
  }

  char *StrDup(const char *str) {
    size_t size;
    char *str_copy;
    size = strlen(str) + 1;
    str_copy = reinterpret_cast<char *>(Alloc(size));
    if (str_copy == NULL)
      return NULL;
    memcpy(str_copy, str, size);
    return str_copy;
  }

  char *data() { return buf_; }
  size_t size() { return offset_; }
};

#if DYNAMIC_LOADING_SUPPORT

// Limit the number of modules to capture and their name length.
static const size_t kLimitModuleListSize = 64 * 1024;

struct ModuleEntry {
  struct ModuleEntry *next;
  char *name;
  MDGUID build_id;
  uintptr_t code_segment_start;
  uintptr_t code_segment_size;
};

static MinidumpAllocator *g_module_snapshot;
static MinidumpAllocator *g_module_snapshot_workspace;

#endif

// TypedMDRVA represents a minidump object chunk.  This interface is
// based on the TypedMDRVA class in the Breakpad codebase.  Breakpad's
// implementation writes directly to a file, whereas this
// implementation constructs the minidump file in memory.
template<typename MDType>
class TypedMDRVA {
  MinidumpAllocator *writer_;
  char *ptr_;
  uint32_t position_;
  size_t size_;

 public:
  explicit TypedMDRVA(MinidumpAllocator *writer) :
      writer_(writer),
      ptr_(NULL),
      position_(0),
      size_(0) {}

  // Allocates space for MDType.
  bool Allocate() {
    return AllocateArray(1);
  }

  // Allocates an array of |count| elements of MDType.
  bool AllocateArray(size_t count) {
    size_ = sizeof(MDType) * count;
    return writer_->AllocateSpace(size_, &ptr_, &position_);
  }

  // Allocates an array of |count| elements of |size| after object of MDType.
  bool AllocateObjectAndArray(size_t count, size_t length) {
    size_ = sizeof(MDType) + count * length;
    return writer_->AllocateSpace(size_, &ptr_, &position_);
  }

  // Copy |item| to |index|.  Must have been allocated using AllocateArray().
  void CopyIndexAfterObject(unsigned int index, void *src, size_t length) {
    size_t offset = sizeof(MDType) + index * length;
    assert(offset + length <= size_);
    memcpy(ptr_ + offset, src, length);
  }

  MDType *get() { return (MDType *) ptr_; }

  uint32_t position() { return position_; }

  MDLocationDescriptor location() {
    MDLocationDescriptor location = { size_, position_ };
    return location;
  }
};


static void ConvertRegisters(MinidumpAllocator *minidump_writer,
                             struct NaClExceptionContext *context,
                             MDRawThread *thread) {
  NaClExceptionPortableContext *pcontext =
      nacl_exception_context_get_portable(context);
#define COPY_REG(REG) regs.get()->REG = src_regs->REG
  switch (context->arch) {
    case EM_386: {
      struct NaClUserRegisterStateX8632 *src_regs =
          (struct NaClUserRegisterStateX8632 *) &context->regs;
      TypedMDRVA<MDRawContextX86> regs(minidump_writer);
      if (!regs.Allocate())
        return;
      thread->thread_context = regs.location();
      // TODO(mseaborn): Report x87/SSE registers too.
      regs.get()->context_flags =
        MD_CONTEXT_X86_CONTROL | MD_CONTEXT_X86_INTEGER;
      COPY_REG(edi);
      COPY_REG(esi);
      COPY_REG(ebx);
      COPY_REG(edx);
      COPY_REG(ecx);
      COPY_REG(eax);
      COPY_REG(ebp);
      regs.get()->eip = src_regs->prog_ctr;
      regs.get()->eflags = src_regs->flags;
      regs.get()->esp = src_regs->stack_ptr;
      break;
    }
    case EM_X86_64: {
      struct NaClUserRegisterStateX8664 *src_regs =
          (struct NaClUserRegisterStateX8664 *) &context->regs;
      TypedMDRVA<MDRawContextAMD64> regs(minidump_writer);
      if (!regs.Allocate())
        return;
      thread->thread_context = regs.location();
      // TODO(mseaborn): Report x87/SSE registers too.
      regs.get()->context_flags =
        MD_CONTEXT_AMD64_CONTROL | MD_CONTEXT_AMD64_INTEGER;
      regs.get()->eflags = src_regs->flags;
      COPY_REG(rax);
      COPY_REG(rcx);
      COPY_REG(rdx);
      COPY_REG(rbx);
      regs.get()->rsp = pcontext->stack_ptr;
      regs.get()->rbp = pcontext->frame_ptr;
      COPY_REG(rsi);
      COPY_REG(rdi);
      COPY_REG(r8);
      COPY_REG(r9);
      COPY_REG(r10);
      COPY_REG(r11);
      COPY_REG(r12);
      COPY_REG(r13);
      COPY_REG(r14);
      COPY_REG(r15);
      regs.get()->rip = pcontext->prog_ctr;
      break;
    }
    case EM_ARM: {
      struct NaClUserRegisterStateARM *src_regs =
          (struct NaClUserRegisterStateARM *) &context->regs;
      TypedMDRVA<MDRawContextARM> regs(minidump_writer);
      if (!regs.Allocate())
        return;
      thread->thread_context = regs.location();
      for (int regnum = 0; regnum < 16; regnum++) {
        regs.get()->iregs[regnum] = ((uint32_t *) &src_regs->r0)[regnum];
      }
      regs.get()->cpsr = src_regs->cpsr;
      break;
    }
    case EM_MIPS: {
      struct NaClUserRegisterStateMIPS *src_regs =
          (struct NaClUserRegisterStateMIPS *) &context->regs;
      TypedMDRVA<MDRawContextMIPS> regs(minidump_writer);
      if (!regs.Allocate())
        return;
      thread->thread_context = regs.location();
      regs.get()->context_flags = MD_CONTEXT_MIPS | MD_CONTEXT_MIPS_INTEGER;
      for (int regnum = 0; regnum < 32; regnum++) {
        regs.get()->iregs[regnum] = ((uint32_t *) &src_regs->zero)[regnum];
      }
      regs.get()->epc = src_regs->prog_ctr;
      break;
    }
    default: {
      // Architecture not recognized.  Dump the register state anyway.
      // Maybe we should do this on all architectures, and Breakpad
      // should be adapted to read NaCl's portable container format
      // for register state so that the client code is
      // architecture-neutral.
      TypedMDRVA<uint8_t> regs(minidump_writer);
      if (!regs.AllocateArray(context->size))
        return;
      thread->thread_context = regs.location();
      memcpy(regs.get(), &context, context->size);
      break;
    }
  }
#undef COPY_REG
}

static MDMemoryDescriptor SnapshotMemory(MinidumpAllocator *minidump_writer,
                                         uintptr_t start, size_t size) {
  TypedMDRVA<uint8_t> mem_copy(minidump_writer);
  MDMemoryDescriptor desc = {0};
  if (mem_copy.AllocateArray(size)) {
    memcpy(mem_copy.get(), (void *) start, size);

    desc.start_of_memory_range = start;
    desc.memory = mem_copy.location();
  }
  return desc;
}

static bool GetStackEnd(void **stack_end) {
#if defined(_NEWLIB_VERSION)
  return pthread_get_stack_end_np(pthread_self(), stack_end) == 0;
#else
  void *stack_base;
  size_t stack_size;
  pthread_attr_t attr;
  if (pthread_getattr_np(pthread_self(), &attr) == 0 &&
      pthread_attr_getstack(&attr, &stack_base, &stack_size) == 0) {
    *stack_end = (void *) ((char *) stack_base + stack_size);
    pthread_attr_destroy(&attr);
    return true;
  }
#if defined(__GLIBC__)
  // pthread_getattr_np() currently fails on the initial thread.  As a
  // workaround, if we reach here, assume we are on the initial thread
  // and get the initial thread's stack end as recorded by glibc.
  // See https://code.google.com/p/nativeclient/issues/detail?id=3431
  *stack_end = __libc_stack_end;
  return true;
#else
  return false;
#endif
#endif
}

static void WriteExceptionList(MinidumpAllocator *minidump_writer,
                               MDRawDirectory *dirent,
                               MDLocationDescriptor thread_context) {
  TypedMDRVA<MDRawExceptionStream> exception(minidump_writer);
  if (!exception.Allocate())
    return;

  // TODO(bradnelson): Specify a particular thread once we gather more than the
  // crashing thread.
  exception.get()->thread_id = 0;
  // TODO(bradnelson): Provide information on the type of exception once we
  // have it. For now report everything as SIGSEGV.
  exception.get()->exception_record.exception_code =
      MD_EXCEPTION_CODE_LIN_SIGSEGV;
  // TODO(bradnelson): Provide the address of the fault, once we have it.
  exception.get()->exception_record.exception_address = 0;
  exception.get()->thread_context = thread_context;

  dirent->stream_type = MD_EXCEPTION_STREAM;
  dirent->location = exception.location();
}

static void WriteThreadList(MinidumpAllocator *minidump_writer,
                            MDRawDirectory *dirent,
                            struct NaClExceptionContext *context,
                            MDLocationDescriptor *thread_context_out) {
  // This records only the thread that crashed.
  // TODO(mseaborn): Record other threads too.  This will require NaCl
  // to provide an interface for suspending threads.
  TypedMDRVA<uint32_t> list(minidump_writer);
  int num_threads = 1;
  if (!list.AllocateObjectAndArray(num_threads, sizeof(MDRawThread)))
    return;
  *list.get() = num_threads;

  MDRawThread thread = {0};
  ConvertRegisters(minidump_writer, context, &thread);
  *thread_context_out = thread.thread_context;

  // Record the stack contents.
  NaClExceptionPortableContext *pcontext =
      nacl_exception_context_get_portable(context);
  uintptr_t stack_start = pcontext->stack_ptr;
  if (context->arch == EM_X86_64) {
    // Include the x86-64 red zone too to capture local variables.
    stack_start -= 128;
  }
  void *stack_end;
  if (GetStackEnd(&stack_end) && stack_start <= (uintptr_t) stack_end) {
    size_t stack_size = (uintptr_t) stack_end - stack_start;
    stack_size = std::min(stack_size, kLimitStackDumpSize);
    thread.stack = SnapshotMemory(minidump_writer, stack_start, stack_size);
  }

  list.CopyIndexAfterObject(0, &thread, sizeof(thread));

  dirent->stream_type = MD_THREAD_LIST_STREAM;
  dirent->location = list.location();
}

static int MinidumpArchFromElfMachine(int e_machine) {
  switch (e_machine) {
    case EM_386:     return MD_CPU_ARCHITECTURE_X86;
    case EM_X86_64:  return MD_CPU_ARCHITECTURE_AMD64;
    case EM_ARM:     return MD_CPU_ARCHITECTURE_ARM;
    case EM_MIPS:    return MD_CPU_ARCHITECTURE_MIPS;
    default:         return MD_CPU_ARCHITECTURE_UNKNOWN;
  }
}

static uint32_t WriteString(MinidumpAllocator *minidump_writer,
                            const char *string) {
  int string_length = strlen(string);
  TypedMDRVA<uint32_t> obj(minidump_writer);
  if (!obj.AllocateObjectAndArray(string_length + 1, sizeof(uint16_t)))
    return 0;
  *obj.get() = string_length * sizeof(uint16_t);
  for (int i = 0; i < string_length + 1; ++i) {
    ((MDString *) obj.get())->buffer[i] = string[i];
  }
  return obj.position();
}

static void WriteSystemInfo(MinidumpAllocator *minidump_writer,
                            MDRawDirectory *dirent,
                            struct NaClExceptionContext *context) {
  TypedMDRVA<MDRawSystemInfo> sysinfo(minidump_writer);
  if (!sysinfo.Allocate())
    return;
  sysinfo.get()->processor_architecture =
      MinidumpArchFromElfMachine(context->arch);
  sysinfo.get()->platform_id = MD_OS_NACL;
  sysinfo.get()->csd_version_rva = WriteString(minidump_writer, "nacl");
  dirent->stream_type = MD_SYSTEM_INFO_STREAM;
  dirent->location = sysinfo.location();
}

static void WriteMiscInfo(MinidumpAllocator *minidump_writer,
                          MDRawDirectory *dirent) {
  // Write empty record to keep minidump_dump happy.
  TypedMDRVA<MDRawMiscInfo> info(minidump_writer);
  if (!info.Allocate())
    return;
  info.get()->size_of_info = sizeof(MDRawMiscInfo);
  dirent->stream_type = MD_MISC_INFO_STREAM;
  dirent->location = info.location();
}

#if DYNAMIC_LOADING_SUPPORT
static int CaptureModulesCallback(
    struct dl_phdr_info *info, size_t size, void *data) {
  MinidumpAllocator *modules_arena = reinterpret_cast<MinidumpAllocator *>(
      data);
  ModuleEntry **modules = reinterpret_cast<ModuleEntry **>(
      modules_arena->data());

  ModuleEntry *module = reinterpret_cast<ModuleEntry *>(
      modules_arena->Alloc(sizeof(ModuleEntry)));
  if (module == NULL) {
    return 1;
  }

  if (strlen(info->dlpi_name) > 0) {
    module->name = modules_arena->StrDup(info->dlpi_name);
  } else {
    module->name = modules_arena->StrDup(g_module_name);
  }
  if (module->name == NULL)
    return 1;

  // Blank these out in case we don't find values for them.
  module->code_segment_start = 0;
  module->code_segment_size = 0;
  memset(&module->build_id, 0, sizeof(module->build_id));

  bool found_code = false;
  bool found_build_id = false;

  for (int i = 0; i < info->dlpi_phnum; ++i) {
    if (!found_build_id && info->dlpi_phdr[i].p_type == PT_NOTE) {
      const char *data_ptr;
      size_t size;
      const void *addr = reinterpret_cast<const void *>(
          info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
      if (!nacl_get_build_id_from_notes(
              addr, info->dlpi_phdr[i].p_memsz, &data_ptr, &size)) {
        continue;
      }
      // Truncate the ID if necessary.  The minidump format uses a 16
      // byte ID, whereas ELF build IDs are typically 20-byte SHA1
      // hashes.
      memcpy(&module->build_id, data_ptr,
             std::min(size, sizeof(module->build_id)));
      found_build_id = true;
    } else if (!found_code &&
               info->dlpi_phdr[i].p_type == PT_LOAD &&
               (info->dlpi_phdr[i].p_flags & PF_X) != 0) {
      module->code_segment_start = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr;
      module->code_segment_size = info->dlpi_phdr[i].p_memsz;
      found_code = true;
    }
  }

  // The entry for runnable-ld.so doesn't report a PT_LOAD segment.
  // Don't emit it, as breakpad is confused by zero length modules.
  if (found_code) {
    module->next = *modules;
    *modules = module;
  }

  return 0;
}

static void CaptureModules(MinidumpAllocator *modules_arena) {
  // Allocate space for the pointer to the head of the module list
  // so that it will be at modules_arena.data().
  ModuleEntry **head = reinterpret_cast<ModuleEntry **>(
      modules_arena->Alloc(sizeof(ModuleEntry *)));
  *head = NULL;

  dl_iterate_phdr(CaptureModulesCallback, modules_arena);

  // TODO(bradnelson): Convert this to a test once we have the plumbing to
  // post-process the minidumps in a test.
  // There should be at least one module.
  assert(*head != NULL);
}
#endif

static void WriteModuleList(MinidumpAllocator *minidump_writer,
                            MDRawDirectory *dirent) {
  // TODO(mseaborn): Report the IRT's build ID here too, once the IRT
  // provides an interface for querying it.
  TypedMDRVA<uint32_t> module_list(minidump_writer);

#if DYNAMIC_LOADING_SUPPORT
  MinidumpAllocator *modules_arena = __sync_lock_test_and_set(
      &g_module_snapshot, NULL);
  if (modules_arena == NULL) {
    modules_arena = g_module_snapshot_workspace;
    CaptureModules(modules_arena);
  }
  // NOTE: Consciously leaking modules_arena. We are crashing and about to
  // shut down anyhow. Attempting to free it can only produce more volatility.
  ModuleEntry **modules = reinterpret_cast<ModuleEntry **>(
      modules_arena->data());
  int module_count = 0;
  for (ModuleEntry *module = *modules; module; module = module->next) {
    ++module_count;
  }
#else
  int module_count = 1;
#endif
  if (!module_list.AllocateObjectAndArray(module_count, MD_MODULE_SIZE)) {
    return;
  }
  *module_list.get() = module_count;

#if DYNAMIC_LOADING_SUPPORT
  int index = 0;
  for (ModuleEntry *module = *modules; module; module = module->next) {
    TypedMDRVA<MDCVInfoPDB70> cv(minidump_writer);
    size_t name_size = strlen(module->name) + 1;
    if (!cv.AllocateObjectAndArray(name_size, sizeof(char))) {
      return;
    }
    cv.get()->cv_signature = MD_CVINFOPDB70_SIGNATURE;
    cv.get()->signature = module->build_id;
    memcpy(cv.get()->pdb_file_name, module->name, name_size);

    MDRawModule dst_module = {0};
    dst_module.base_of_image = module->code_segment_start;
    dst_module.size_of_image = module->code_segment_size;
    dst_module.module_name_rva = WriteString(minidump_writer, module->name);
    dst_module.cv_record = cv.location();
    module_list.CopyIndexAfterObject(index++, &dst_module, MD_MODULE_SIZE);
  }
#else
  TypedMDRVA<MDCVInfoPDB70> cv(minidump_writer);
  size_t name_size = strlen(g_module_name) + 1;
  if (!cv.AllocateObjectAndArray(name_size, sizeof(char))) {
    return;
  }
  cv.get()->cv_signature = MD_CVINFOPDB70_SIGNATURE;
  cv.get()->signature = g_module_build_id;
  memcpy(cv.get()->pdb_file_name, g_module_name, name_size);

  MDRawModule dst_module = {0};
  dst_module.base_of_image = (uintptr_t) &__executable_start;
  dst_module.size_of_image = (uintptr_t) &__etext -
                             (uintptr_t) &__executable_start;
  dst_module.module_name_rva = WriteString(minidump_writer, g_module_name);
  dst_module.cv_record = cv.location();
  module_list.CopyIndexAfterObject(0, &dst_module, MD_MODULE_SIZE);
#endif

  dirent->stream_type = MD_MODULE_LIST_STREAM;
  dirent->location = module_list.location();
}

static void WriteMemoryList(MinidumpAllocator *minidump_writer,
                            MDRawDirectory *dirent) {
  // TODO(bradnelson): Actually capture memory regions.
  // Write empty list to keep minidump_dump happy.
  TypedMDRVA<uint32_t> memory_list(minidump_writer);
  if (!memory_list.AllocateObjectAndArray(0, sizeof(MDMemoryDescriptor))) {
    return;
  }
  *memory_list.get() = 0;

  dirent->stream_type = MD_MEMORY_LIST_STREAM;
  dirent->location = memory_list.location();
}

static void WriteMemoryInfoList(MinidumpAllocator *minidump_writer,
                                MDRawDirectory *dirent) {
  // TODO(bradnelson): Actually capture memory info regions.
  // Write empty list to keep minidump_dump happy.
  TypedMDRVA<MDRawMemoryInfoList> memory_info_list(minidump_writer);
  if (!memory_info_list.AllocateObjectAndArray(
        0, sizeof(MDRawMemoryInfo))) {
    return;
  }
  memory_info_list.get()->size_of_header = sizeof(MDRawMemoryInfoList);
  memory_info_list.get()->size_of_entry = sizeof(MDRawMemoryInfo);
  memory_info_list.get()->number_of_entries = 0;

  dirent->stream_type = MD_MEMORY_INFO_LIST_STREAM;
  dirent->location = memory_info_list.location();
}

static void WriteMinidump(MinidumpAllocator *minidump_writer,
                          struct NaClExceptionContext *context) {
  const int kNumWriters = 7;
  TypedMDRVA<MDRawHeader> header(minidump_writer);
  TypedMDRVA<MDRawDirectory> dir(minidump_writer);
  if (!header.Allocate())
    return;
  if (!dir.AllocateArray(kNumWriters))
    return;
  header.get()->signature = MD_HEADER_SIGNATURE;
  header.get()->version = MD_HEADER_VERSION;
  header.get()->time_date_stamp = time(NULL);
  header.get()->stream_count = kNumWriters;
  header.get()->stream_directory_rva = dir.position();

  int dir_index = 0;
  MDLocationDescriptor thread_context = {0};
  WriteThreadList(minidump_writer, &dir.get()[dir_index++], context,
                  &thread_context);
  WriteExceptionList(minidump_writer, &dir.get()[dir_index++], thread_context);
  WriteSystemInfo(minidump_writer, &dir.get()[dir_index++], context);
  WriteMiscInfo(minidump_writer, &dir.get()[dir_index++]);
  WriteModuleList(minidump_writer, &dir.get()[dir_index++]);
  WriteMemoryList(minidump_writer, &dir.get()[dir_index++]);
  WriteMemoryInfoList(minidump_writer, &dir.get()[dir_index++]);
  assert(dir_index == kNumWriters);
}

static void CrashHandler(struct NaClExceptionContext *context) {
  static const char msg[] = "minidump: Caught crash\n";
  write(2, msg, sizeof(msg) - 1);

  // Prevent re-entering the crash handler if two crashes occur
  // concurrently.  We preallocate storage that cannot be used
  // concurrently.  We avoid using a pthread mutex here in case
  // libpthread's data structures are corrupted.
  if (__sync_lock_test_and_set(&g_handling_exception, 1)) {
    // Wait forever here so that the first crashing thread can report
    // the crash and exit.
    for (;;)
      sleep(9999);
  }

  MinidumpAllocator *minidump_writer = g_minidump_writer;
  WriteMinidump(minidump_writer, context);

  if (g_callback_func)
    g_callback_func(minidump_writer->data(), minidump_writer->size());

  // Flush streams to aid debugging, although since the process might
  // be in a corrupted state this might crash.
  fflush(NULL);

  _exit(127);
}

void nacl_minidump_register_crash_handler(void) {
  errno = nacl_exception_set_handler(CrashHandler);
  if (errno != 0) {
    perror("minidump: Failed to register an exception handler");
    return;
  }

#if !DYNAMIC_LOADING_SUPPORT
  /*
   * With dynamic linking, all modules' build IDs are discovered
   * via dl_iterate_phdr->PT_NOTE->NT_BUILD_ID.  g_module_build_id
   * is not used at all (see WriteModuleList, above).
   */
  if (!g_module_build_id_set) {
    // Try to use the nexe's built-in build ID.
    const char *data_ptr;
    size_t size;
    if (nacl_get_build_id(&data_ptr, &size)) {
      // Truncate the ID if necessary.  The minidump format uses a 16
      // byte ID, whereas ELF build IDs are typically 20-byte SHA1
      // hashes.
      memcpy(&g_module_build_id, data_ptr,
             std::min(size, sizeof(g_module_build_id)));
      g_module_build_id_set = 1;
    }
  }
#endif

#if DYNAMIC_LOADING_SUPPORT
  g_module_snapshot_workspace = new MinidumpAllocator(kLimitModuleListSize);
#endif
  g_minidump_writer = new MinidumpAllocator(kMinidumpBufferSize);
}

void nacl_minidump_set_callback(nacl_minidump_callback_t callback) {
  g_callback_func = callback;
}

void nacl_minidump_set_module_name(const char *module_name) {
  g_module_name = module_name;
}

/*
 * Under dynamic linking, this interface is a no-op.
 */
void nacl_minidump_set_module_build_id(
    const uint8_t data[NACL_MINIDUMP_BUILD_ID_SIZE]) {
#if !DYNAMIC_LOADING_SUPPORT
  assert(sizeof(g_module_build_id) == NACL_MINIDUMP_BUILD_ID_SIZE);
  memcpy(&g_module_build_id, data, NACL_MINIDUMP_BUILD_ID_SIZE);
  g_module_build_id_set = 1;
#endif
}

void nacl_minidump_snapshot_module_list(void) {
#if DYNAMIC_LOADING_SUPPORT
  MinidumpAllocator *modules_arena = new MinidumpAllocator(
      kLimitModuleListSize);
  CaptureModules(modules_arena);
  modules_arena = __sync_lock_test_and_set(&g_module_snapshot, modules_arena);
  if (modules_arena != NULL)
    delete modules_arena;
#endif
}

void nacl_minidump_clear_module_list(void) {
#if DYNAMIC_LOADING_SUPPORT
  MinidumpAllocator *modules_arena = __sync_lock_test_and_set(
      &g_module_snapshot, NULL);
  if (modules_arena != NULL)
    delete modules_arena;
#endif
}
