// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_rdebug.h"

#include <elf.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_proc_maps.h"
#include "crazy_linker_system.h"
#include "crazy_linker_util.h"
#include "elf_traits.h"

namespace crazy {

namespace {

// Find the full path of the current executable. On success return true
// and sets |exe_path|. On failure, return false and sets errno.
bool FindExecutablePath(String* exe_path) {
  // /proc/self/exe is a symlink to the full path. Read it with
  // readlink().
  exe_path->Resize(512);
  ssize_t ret = TEMP_FAILURE_RETRY(
      readlink("/proc/self/exe", exe_path->ptr(), exe_path->size()));
  if (ret < 0) {
    LOG_ERRNO("Could not get /proc/self/exe link");
    return false;
  }

  exe_path->Resize(static_cast<size_t>(ret));
  LOG("Current executable: %s", exe_path->c_str());
  return true;
}

// Given an ELF binary at |path| that is _already_ mapped in the process,
// find the address of its dynamic section and its size.
// |path| is the full path of the binary (as it appears in /proc/self/maps.
// |self_maps| is an instance of ProcMaps that is used to inspect
// /proc/self/maps. The function rewind + iterates over it.
// On success, return true and set |*dynamic_offset| and |*dynamic_size|.
bool FindElfDynamicSection(const char* path,
                           ProcMaps* self_maps,
                           size_t* dynamic_address,
                           size_t* dynamic_size) {
  // Read the ELF header first.
  ELF::Ehdr header[1];

  crazy::FileDescriptor fd(path);
  if (!fd.IsOk() || !fd.ReadFull(header, sizeof(header))) {
    LOG_ERRNO("Could not load ELF binary header");
    return false;
  }

  // Sanity check.
  if (header->e_ident[0] != 127 || header->e_ident[1] != 'E' ||
      header->e_ident[2] != 'L' || header->e_ident[3] != 'F' ||
      header->e_ident[4] != ELF::kElfClass) {
    LOG("Not a %d-bit ELF binary: %s", ELF::kElfBits, path);
    return false;
  }

  if (header->e_phoff == 0 || header->e_phentsize != sizeof(ELF::Phdr)) {
    LOG("Invalid program header values: %s", path);
    return false;
  }

  // Scan the program header table.
  if (fd.SeekTo(header->e_phoff) < 0) {
    LOG_ERRNO("Could not find ELF program header table");
    return false;
  }

  ELF::Phdr phdr_load0 = {0, };
  ELF::Phdr phdr_dyn = {0, };
  bool found_load0 = false;
  bool found_dyn = false;

  for (size_t n = 0; n < header->e_phnum; ++n) {
    ELF::Phdr phdr;
    if (!fd.ReadFull(&phdr, sizeof(phdr))) {
      LOG_ERRNO("Could not read program header entry");
      return false;
    }

    if (phdr.p_type == PT_LOAD && !found_load0) {
      phdr_load0 = phdr;
      found_load0 = true;
    } else if (phdr.p_type == PT_DYNAMIC && !found_dyn) {
      phdr_dyn = phdr;
      found_dyn = true;
    }
  }

  if (!found_load0) {
    LOG("Could not find loadable segment!?");
    return false;
  }
  if (!found_dyn) {
    LOG("Could not find dynamic segment!?");
    return false;
  }

  LOG("Found first loadable segment [offset=%p vaddr=%p]",
      (void*)phdr_load0.p_offset, (void*)phdr_load0.p_vaddr);

  LOG("Found dynamic segment [offset=%p vaddr=%p size=%p]",
      (void*)phdr_dyn.p_offset, (void*)phdr_dyn.p_vaddr,
      (void*)phdr_dyn.p_memsz);

  // Parse /proc/self/maps to find the load address of the first
  // loadable segment.
  size_t path_len = strlen(path);
  self_maps->Rewind();
  ProcMaps::Entry entry;
  while (self_maps->GetNextEntry(&entry)) {
    if (!entry.path || entry.path_len != path_len ||
        memcmp(entry.path, path, path_len) != 0)
      continue;

    LOG("Found executable segment mapped [%p-%p offset=%p]",
        (void*)entry.vma_start, (void*)entry.vma_end, (void*)entry.load_offset);

    size_t load_bias = entry.vma_start - phdr_load0.p_vaddr;
    LOG("Load bias is %p", (void*)load_bias);

    *dynamic_address = load_bias + phdr_dyn.p_vaddr;
    *dynamic_size = phdr_dyn.p_memsz;
    LOG("Dynamic section addr=%p size=%p", (void*)*dynamic_address,
        (void*)*dynamic_size);
    return true;
  }

  LOG("Executable is not mapped in current process.");
  return false;
}

// Helper class to temporarily remap a page to readable+writable until
// scope exit.
class ScopedPageReadWriteRemapper {
 public:
  ScopedPageReadWriteRemapper(void* address);
  ~ScopedPageReadWriteRemapper();

  // Releases the page so that the destructor does not undo the remapping.
  void Release();

 private:
  static const uintptr_t kPageSize = 4096;
  uintptr_t page_address_;
  int page_prot_;
};

ScopedPageReadWriteRemapper::ScopedPageReadWriteRemapper(void* address) {
  page_address_ = reinterpret_cast<uintptr_t>(address) & ~(kPageSize - 1);
  page_prot_ = 0;
  if (!FindProtectionFlagsForAddress(address, &page_prot_)) {
    LOG("Could not find protection flags for %p\n", address);
    page_address_ = 0;
    return;
  }

  // Note: page_prot_ may already indicate read/write, but because of
  // possible races with the system linker we cannot be confident that
  // this is reliable. So we always set read/write here.
  //
  // See commentary in WriteLinkMapField for more.
  int new_page_prot = page_prot_ | PROT_READ | PROT_WRITE;
  int ret = mprotect(
      reinterpret_cast<void*>(page_address_), kPageSize, new_page_prot);
  if (ret < 0) {
    LOG_ERRNO("Could not remap page to read/write");
    page_address_ = 0;
  }
}

ScopedPageReadWriteRemapper::~ScopedPageReadWriteRemapper() {
  if (page_address_) {
    int ret =
        mprotect(reinterpret_cast<void*>(page_address_), kPageSize, page_prot_);
    if (ret < 0)
      LOG_ERRNO("Could not remap page to old protection flags");
  }
}

void ScopedPageReadWriteRemapper::Release() {
  page_address_ = 0;
  page_prot_ = 0;
}

}  // namespace

r_debug* RDebug::GetAddress() {
  if (!init_) {
    Init();
  }
  return r_debug_;
}

bool RDebug::Init() {
  // The address of '_r_debug' is in the DT_DEBUG entry of the current
  // executable.
  init_ = true;

  size_t dynamic_addr = 0;
  size_t dynamic_size = 0;
  String path;

  // Find the current executable's full path, and its dynamic section
  // information.
  if (!FindExecutablePath(&path))
    return false;

  ProcMaps self_maps;
  if (!FindElfDynamicSection(
           path.c_str(), &self_maps, &dynamic_addr, &dynamic_size)) {
    return false;
  }

  // Parse the dynamic table and find the DT_DEBUG entry.
  const ELF::Dyn* dyn_section = reinterpret_cast<const ELF::Dyn*>(dynamic_addr);

  while (dynamic_size >= sizeof(*dyn_section)) {
    if (dyn_section->d_tag == DT_DEBUG) {
      // Found it!
      LOG("Found DT_DEBUG entry inside %s at %p, pointing to %p", path.c_str(),
          dyn_section, dyn_section->d_un.d_ptr);
      if (dyn_section->d_un.d_ptr) {
        r_debug_ = reinterpret_cast<r_debug*>(dyn_section->d_un.d_ptr);
        LOG("r_debug [r_version=%d r_map=%p r_brk=%p r_ldbase=%p]",
            r_debug_->r_version, r_debug_->r_map, r_debug_->r_brk,
            r_debug_->r_ldbase);
        // Only version 1 of the struct is supported.
        if (r_debug_->r_version != 1) {
          LOG("r_debug.r_version is %d, 1 expected.", r_debug_->r_version);
          r_debug_ = NULL;
        }

        // The linker of recent Android releases maps its link map entries
        // in read-only pages. Determine if this is the case and record it
        // for later. The first entry in the list corresponds to the
        // executable.
        int prot = self_maps.GetProtectionFlagsForAddress(r_debug_->r_map);
        readonly_entries_ = (prot & PROT_WRITE) == 0;

        LOG("r_debug.readonly_entries=%s",
            readonly_entries_ ? "true" : "false");
        return true;
      }
    }
    dyn_section++;
    dynamic_size -= sizeof(*dyn_section);
  }

  LOG("There is no non-0 DT_DEBUG entry in this process");
  return false;
}

void RDebug::CallRBrk(int state) {
#if !defined(CRAZY_DISABLE_R_BRK)
  r_debug_->r_state = state;
  r_debug_->r_brk();
#endif  // !CRAZY_DISABLE_R_BRK
}

namespace {

// Helper class providing a simple scoped pthreads mutex.
class ScopedMutexLock {
 public:
  explicit ScopedMutexLock(pthread_mutex_t* mutex) : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }
  ~ScopedMutexLock() {
    pthread_mutex_unlock(mutex_);
  }

 private:
  pthread_mutex_t* mutex_;
};

// Helper runnable class. Handler is one of the two static functions
// AddEntryInternal() or DelEntryInternal(). Calling these invokes
// AddEntryImpl() or DelEntryImpl() respectively on rdebug.
class RDebugRunnable {
 public:
  RDebugRunnable(rdebug_callback_handler_t handler,
                 RDebug* rdebug,
                 link_map_t* entry,
                 bool is_blocking)
      : handler_(handler), rdebug_(rdebug),
        entry_(entry), is_blocking_(is_blocking), has_run_(false) {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
  }

  static void Run(void* opaque);
  static void WaitForCallback(void* opaque);

 private:
  rdebug_callback_handler_t handler_;
  RDebug* rdebug_;
  link_map_t* entry_;
  bool is_blocking_;
  bool has_run_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
};

// Callback entry point.
void RDebugRunnable::Run(void* opaque) {
  RDebugRunnable* runnable = static_cast<RDebugRunnable*>(opaque);

  LOG("Callback received, runnable=%p", runnable);
  (*runnable->handler_)(runnable->rdebug_, runnable->entry_);

  if (!runnable->is_blocking_) {
    delete runnable;
    return;
  }

  LOG("Signalling callback, runnable=%p", runnable);
  {
    ScopedMutexLock m(&runnable->mutex_);
    runnable->has_run_ = true;
    pthread_cond_signal(&runnable->cond_);
  }
}

// For blocking callbacks, wait for the call to Run().
void RDebugRunnable::WaitForCallback(void* opaque) {
  RDebugRunnable* runnable = static_cast<RDebugRunnable*>(opaque);

  if (!runnable->is_blocking_) {
    LOG("Non-blocking, not waiting, runnable=%p", runnable);
    return;
  }

  LOG("Waiting for signal, runnable=%p", runnable);
  {
    ScopedMutexLock m(&runnable->mutex_);
    while (!runnable->has_run_)
      pthread_cond_wait(&runnable->cond_, &runnable->mutex_);
  }

  delete runnable;
}

}  // namespace

// Helper function to schedule AddEntry() and DelEntry() calls onto another
// thread where possible. Running them there avoids races with the system
// linker, which expects to be able to set r_map pages readonly when it
// is not using them and which may run simultaneously on the main thread.
bool RDebug::PostCallback(rdebug_callback_handler_t handler,
                          link_map_t* entry,
                          bool is_blocking) {
  if (!post_for_later_execution_) {
    LOG("Deferred execution disabled");
    return false;
  }

  RDebugRunnable* runnable =
      new RDebugRunnable(handler, this, entry, is_blocking);
  void* context = post_for_later_execution_context_;

  if (!(*post_for_later_execution_)(context, &RDebugRunnable::Run, runnable)) {
    LOG("Deferred execution enabled, but posting failed");
    delete runnable;
    return false;
  }

  LOG("Posted for later execution, runnable=%p", runnable);

  if (is_blocking) {
    RDebugRunnable::WaitForCallback(runnable);
    LOG("Completed execution, runnable=%p", runnable);
  }

  return true;
}

// Helper function for AddEntryImpl and DelEntryImpl.
// Sets *link_pointer to entry.  link_pointer is either an 'l_prev' or an
// 'l_next' field in a neighbouring linkmap_t.  If link_pointer is in a
// page that is mapped readonly, the page is remapped to be writable before
// assignment.
void RDebug::WriteLinkMapField(link_map_t** link_pointer, link_map_t* entry) {
  ScopedPageReadWriteRemapper mapper(link_pointer);
  LOG("Remapped page for %p for read/write", link_pointer);

  *link_pointer = entry;

  // We always mprotect the page containing link_pointer to read/write,
  // then write the entry. The page may already be read/write, but on
  // recent Android release is most likely readonly. Because of the way
  // the system linker operates we cannot tell with certainty what its
  // correct setting should be.
  //
  // Now, we always leave the page read/write. Here is why. If we set it
  // back to readonly at the point between where the system linker sets
  // it to read/write and where it writes to the address, this will cause
  // the system linker to crash. Clearly that is undesirable. From
  // observations this occurs most frequently on the gpu process.
  //
  // TODO(simonb): Revisit this, details in:
  // https://code.google.com/p/chromium/issues/detail?id=450659
  // https://code.google.com/p/chromium/issues/detail?id=458346
  mapper.Release();
  LOG("Released mapper, leaving page read/write");
}

void RDebug::AddEntryImpl(link_map_t* entry) {
  LOG("Adding: %s", entry->l_name);
  if (!init_)
    Init();

  if (!r_debug_) {
    LOG("Nothing to do");
    return;
  }

  // Ensure modifications to the global link map are synchronized.
  ScopedLinkMapLocker locker;

  // IMPORTANT: GDB expects the first entry in the list to correspond
  // to the executable. So add our new entry just after it. This is ok
  // because by default, the linker is always the second entry, as in:
  //
  //   [<executable>, /system/bin/linker, libc.so, libm.so, ...]
  //
  // By design, the first two entries should never be removed since they
  // can't be unloaded from the process (they are loaded by the kernel
  // when invoking the program).
  //
  // TODO(digit): Does GDB expect the linker to be the second entry?
  // It doesn't seem so, but have a look at the GDB sources to confirm
  // this. No problem appear experimentally.
  //
  // What happens for static binaries? They don't have an .interp section,
  // and don't have a r_debug variable on Android, so GDB should not be
  // able to debug shared libraries at all for them (assuming one
  // statically links a linker into the executable).
  if (!r_debug_->r_map || !r_debug_->r_map->l_next ||
      !r_debug_->r_map->l_next->l_next) {
    // Sanity check: Must have at least two items in the list.
    LOG("Malformed r_debug.r_map list");
    r_debug_ = NULL;
    return;
  }

  // Tell GDB the list is going to be modified.
  CallRBrk(RT_ADD);

  link_map_t* before = r_debug_->r_map->l_next;
  link_map_t* after = before->l_next;

  // Prepare the new entry.
  entry->l_prev = before;
  entry->l_next = after;

  // IMPORTANT: Before modifying the previous and next entries in the
  // list, ensure that they are writable. This avoids crashing when
  // updating the 'l_prev' or 'l_next' fields of a system linker entry,
  // which are mapped read-only.
  WriteLinkMapField(&before->l_next, entry);
  WriteLinkMapField(&after->l_prev, entry);

  // Tell GDB the list modification has completed.
  CallRBrk(RT_CONSISTENT);
}

void RDebug::DelEntryImpl(link_map_t* entry) {
  if (!r_debug_)
    return;

  LOG("Deleting: %s", entry->l_name);

  // Ensure modifications to the global link map are synchronized.
  ScopedLinkMapLocker locker;

  // Tell GDB the list is going to be modified.
  CallRBrk(RT_DELETE);

  // IMPORTANT: Before modifying the previous and next entries in the
  // list, ensure that they are writable. See comment above for more
  // details.
  if (entry->l_prev)
    WriteLinkMapField(&entry->l_prev->l_next, entry->l_next);
  if (entry->l_next)
    WriteLinkMapField(&entry->l_next->l_prev, entry->l_prev);

  if (r_debug_->r_map == entry)
    r_debug_->r_map = entry->l_next;

  entry->l_prev = NULL;
  entry->l_next = NULL;

  // Tell GDB the list modification has completed.
  CallRBrk(RT_CONSISTENT);
}

}  // namespace crazy
