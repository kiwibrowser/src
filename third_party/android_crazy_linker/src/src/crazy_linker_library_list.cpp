// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_library_list.h"

#include <assert.h>
#include <crazy_linker.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_library_view.h"
#include "crazy_linker_pointer_set.h"
#include "crazy_linker_rdebug.h"
#include "crazy_linker_shared_library.h"
#include "crazy_linker_system.h"
#include "crazy_linker_system_linker.h"
#include "crazy_linker_util.h"
#include "crazy_linker_zip.h"

namespace crazy {

namespace {

// From android.os.Build.VERSION_CODES.LOLLIPOP.
static const int SDK_VERSION_CODE_LOLLIPOP = 21;

// Page size for alignment in a zip file.
const size_t kZipAlignmentPageSize = 4096;
COMPILE_ASSERT(kZipAlignmentPageSize % PAGE_SIZE == 0,
               kZipAlignmentPageSize_must_be_a_multiple_of_PAGE_SIZE);

// A helper struct used when looking up symbols in libraries.
struct SymbolLookupState {
  void* found_addr;
  void* weak_addr;
  int weak_count;

  SymbolLookupState() : found_addr(NULL), weak_addr(NULL), weak_count(0) {}

  // Check a symbol entry.
  bool CheckSymbol(const char* symbol, SharedLibrary* lib) {
    const ELF::Sym* entry = lib->LookupSymbolEntry(symbol);
    if (!entry)
      return false;

    void* address = reinterpret_cast<void*>(lib->load_bias() + entry->st_value);

    // If this is a strong symbol, record it and return true.
    if (ELF_ST_BIND(entry->st_info) == STB_GLOBAL) {
      found_addr = address;
      return true;
    }
    // If this is a weak symbol, record the first one and
    // increment the weak_count.
    if (++weak_count == 1)
      weak_addr = address;

    return false;
  }
};

}  // namespace

LibraryList::LibraryList() : head_(0), has_error_(false) {
  // NOTE: This constructor is called from the Globals::Globals() constructor,
  // hence it is important that Globals::sdk_build_version is a static member
  // that can be set before Globals::Get() is called for the first time.
  const int sdk_build_version = Globals::sdk_build_version;

  // If SDK version is Lollipop or earlier, we need to load anything
  // listed in LD_PRELOAD explicitly, because dlsym() on the main executable
  // fails to lookup in preloads on those releases. Also, when doing our
  // symbol resolution we need to explicity search preloads *before* we
  // search the main executable, to ensure that preloads override symbols
  // correctly. Searching preloads before main is opposite to the way the
  // system linker's ordering of these searches, but it is required here to
  // work round the platform's dlsym() issue.
  //
  // If SDK version is Lollipop-mr1 or later then dlsym() will search
  // preloads when invoked on the main executable, meaning that we do not
  // want (or need) to load them here. The platform itself will take care
  // of them for us, and so by not loading preloads here our preloads list
  // remains empty, so that searching it for name lookups is a no-op.
  //
  // For more, see:
  //   https://code.google.com/p/android/issues/detail?id=74255
  if (sdk_build_version <= SDK_VERSION_CODE_LOLLIPOP)
    LoadPreloads();
}

LibraryList::~LibraryList() {
  // Invalidate crazy library list.
  head_ = NULL;

  // Destroy all known libraries.
  while (!known_libraries_.IsEmpty()) {
    LibraryView* wrap = known_libraries_.PopLast();
    delete wrap;
  }
}

void LibraryList::LoadPreloads() {
  const char* ld_preload = GetEnv("LD_PRELOAD");
  if (!ld_preload)
    return;

  SearchPathList search_path_list;
  search_path_list.ResetFromEnv("LD_LIBRARY_PATH");

  LOG("Preloads list is: %s", ld_preload);
  const char* current = ld_preload;
  const char* end = ld_preload + strlen(ld_preload);

  // Iterate over library names listed in the environment. The separator
  // here may be either space or colon.
  while (current < end) {
    const char* item = current;
    const size_t item_length = strcspn(current, " :");
    if (item_length == 0) {
      current += 1;
      continue;
    }
    current = item + item_length + 1;

    String lib_name(item, item_length);
    LOG("Attempting to preload %s", lib_name.c_str());

    if (FindKnownLibrary(lib_name.c_str())) {
      LOG("already loaded %s: ignoring", lib_name.c_str());
      continue;
    }

    Error error;
    LibraryView* preload = LoadLibrary(lib_name.c_str(),
                                       RTLD_NOW | RTLD_GLOBAL,
                                       0U /* load address */,
                                       0U /* file offset */,
                                       &search_path_list,
                                       true /* is_dependency_or_preload */,
                                       &error);
    if (!preload) {
      LOG("'%s' cannot be preloaded: ignored\n", lib_name.c_str());
      continue;
    }

    preloaded_libraries_.PushBack(preload);
  }

  if (CRAZY_DEBUG) {
    LOG("Preloads loaded");
    for (size_t n = 0; n < preloaded_libraries_.GetCount(); ++n)
      LOG("  ... %p %s\n",
          preloaded_libraries_[n], preloaded_libraries_[n]->GetName());
    LOG("    preloads @%p\n", &preloaded_libraries_);
  }
}

LibraryView* LibraryList::FindLibraryByName(const char* lib_name) {
  // Sanity check.
  if (!lib_name)
    return NULL;

  for (size_t n = 0; n < known_libraries_.GetCount(); ++n) {
    LibraryView* wrap = known_libraries_[n];
    if (!strcmp(lib_name, wrap->GetName()))
      return wrap;
  }
  return NULL;
}

void* LibraryList::FindSymbolFrom(const char* symbol_name, LibraryView* from) {
  SymbolLookupState lookup_state;

  if (!from)
    return NULL;

  // Use a work-queue and a set to ensure to perform a breadth-first
  // search.
  Vector<LibraryView*> work_queue;
  PointerSet visited_set;

  work_queue.PushBack(from);

  while (!work_queue.IsEmpty()) {
    LibraryView* lib = work_queue.PopFirst();
    if (lib->IsCrazy()) {
      if (lookup_state.CheckSymbol(symbol_name, lib->GetCrazy()))
        return lookup_state.found_addr;
    } else if (lib->IsSystem()) {
      // TODO(digit): Support weak symbols in system libraries.
      // With the current code, all symbols in system libraries
      // are assumed to be non-weak.
      void* addr = lib->LookupSymbol(symbol_name);
      if (addr)
        return addr;
    }

    // If this is a crazy library, add non-visited dependencies
    // to the work queue.
    if (lib->IsCrazy()) {
      SharedLibrary::DependencyIterator iter(lib->GetCrazy());
      while (iter.GetNext()) {
        LibraryView* dependency = FindKnownLibrary(iter.GetName());
        if (dependency && !visited_set.Has(dependency)) {
          work_queue.PushBack(dependency);
          visited_set.Add(dependency);
        }
      }
    }
  }

  if (lookup_state.weak_count >= 1) {
    // There was at least a single weak symbol definition, so use
    // the first one found in breadth-first search order.
    return lookup_state.weak_addr;
  }

  // There was no symbol definition.
  return NULL;
}

LibraryView* LibraryList::FindLibraryForAddress(void* address) {
  // Linearly scan all libraries, looking for one that contains
  // a given address. NOTE: This doesn't check that this falls
  // inside one of the mapped library segments.
  for (size_t n = 0; n < known_libraries_.GetCount(); ++n) {
    LibraryView* wrap = known_libraries_[n];
    // TODO(digit): Search addresses inside system libraries.
    if (wrap->IsCrazy()) {
      SharedLibrary* lib = wrap->GetCrazy();
      if (lib->ContainsAddress(address))
        return wrap;
    }
  }
  return NULL;
}

#ifdef __arm__
_Unwind_Ptr LibraryList::FindArmExIdx(void* pc, int* count) {
  for (SharedLibrary* lib = head_; lib; lib = lib->list_next_) {
    if (lib->ContainsAddress(pc)) {
      *count = static_cast<int>(lib->arm_exidx_count_);
      return reinterpret_cast<_Unwind_Ptr>(lib->arm_exidx_);
    }
  }
  *count = 0;
  return static_cast<_Unwind_Ptr>(NULL);
}
#else  // !__arm__
int LibraryList::IteratePhdr(PhdrIterationCallback callback, void* data) {
  int result = 0;
  for (SharedLibrary* lib = head_; lib; lib = lib->list_next_) {
    dl_phdr_info info;
    info.dlpi_addr = lib->link_map_.l_addr;
    info.dlpi_name = lib->link_map_.l_name;
    info.dlpi_phdr = lib->phdr();
    info.dlpi_phnum = lib->phdr_count();
    result = callback(&info, sizeof(info), data);
    if (result)
      break;
  }
  return result;
}
#endif  // !__arm__

void LibraryList::UnloadLibrary(LibraryView* wrap) {
  // Sanity check.
  LOG("for %s (ref_count=%d)", wrap->GetName(), wrap->ref_count());

  if (!wrap->IsSystem() && !wrap->IsCrazy())
    return;

  if (!wrap->SafeDecrementRef())
    return;

  // If this is a crazy library, perform manual cleanup first.
  if (wrap->IsCrazy()) {
    SharedLibrary* lib = wrap->GetCrazy();

    // Remove from internal list of crazy libraries.
    if (lib->list_next_)
      lib->list_next_->list_prev_ = lib->list_prev_;
    if (lib->list_prev_)
      lib->list_prev_->list_next_ = lib->list_next_;
    if (lib == head_)
      head_ = lib->list_next_;

    // Call JNI_OnUnload, if necessary, then the destructors.
    lib->CallJniOnUnload();
    lib->CallDestructors();

    // Unload the dependencies recursively.
    SharedLibrary::DependencyIterator iter(lib);
    while (iter.GetNext()) {
      LibraryView* dependency = FindKnownLibrary(iter.GetName());
      if (dependency)
        UnloadLibrary(dependency);
    }

    // Tell GDB of this removal.
    Globals::GetRDebug()->DelEntry(&lib->link_map_);
  }

  known_libraries_.Remove(wrap);

  // Delete the wrapper, which will delete the crazy library, or
  // dlclose() the system one.
  delete wrap;
}

LibraryView* LibraryList::LoadLibrary(const char* lib_name,
                                      int dlopen_mode,
                                      uintptr_t load_address,
                                      off_t file_offset,
                                      SearchPathList* search_path_list,
                                      bool is_dependency_or_preload,
                                      Error* error) {
  const char* base_name = GetBaseNamePtr(lib_name);

  LOG("lib_name='%s'", lib_name);

  // First check whether a library with the same base name was
  // already loaded.
  LibraryView* wrap = FindKnownLibrary(lib_name);
  if (wrap) {
    if (load_address) {
      // Check that this is a crazy library and that is was loaded at
      // the correct address.
      if (!wrap->IsCrazy()) {
        error->Format("System library can't be loaded at fixed address %08x",
                      load_address);
        return NULL;
      }
      uintptr_t actual_address = wrap->GetCrazy()->load_address();
      if (actual_address != load_address) {
        error->Format("Library already loaded at @%08x, can't load it at @%08x",
                      actual_address,
                      load_address);
        return NULL;
      }
    }
    wrap->AddRef();
    return wrap;
  }

  // If this load is prompted by either dependencies or preloads, open
  // normally with dlopen() and do not proceed to try and load the library
  // crazily.
  if (is_dependency_or_preload) {
    LOG("Loading system library '%s'", lib_name);
    void* system_lib = SystemLinker::Open(lib_name, dlopen_mode);
    if (!system_lib) {
      error->Format("Can't load system library %s: %s", lib_name, ::dlerror());
      return NULL;
    }

    LibraryView* wrap = new LibraryView(system_lib, base_name);
    known_libraries_.PushBack(wrap);

    LOG("System library %s loaded at %p", lib_name, wrap);
    LOG("  name=%s\n", wrap->GetName());
    return wrap;
  }

  ScopedPtr<SharedLibrary> lib(new SharedLibrary());

  // Find the full library path.
  String full_path;

  LOG("Looking through the search path list");
  SearchPathList::Result probe = search_path_list->FindFile(lib_name);
  if (!probe.IsValid()) {
    error->Format("Can't find library file %s", lib_name);
    return NULL;
  }
  LOG("Found library: path %s @ 0x%x", probe.path.c_str(), probe.offset);

  // Load the library
  if (!lib->Load(probe.path.c_str(), load_address, file_offset + probe.offset,
                 error))
    return NULL;

  // Load all dependendent libraries.
  LOG("Loading dependencies of %s", base_name);
  SharedLibrary::DependencyIterator iter(lib.Get());
  Vector<LibraryView*> dependencies;
  while (iter.GetNext()) {
    Error dep_error;
    LibraryView* dependency = LoadLibrary(iter.GetName(),
                                          dlopen_mode,
                                          0U /* load address */,
                                          0U /* file offset */,
                                          search_path_list,
                                          true /* is_dependency_or_preload */,
                                          &dep_error);
    if (!dependency) {
      error->Format("When loading %s: %s", base_name, dep_error.c_str());
      return NULL;
    }
    dependencies.PushBack(dependency);
  }
  if (CRAZY_DEBUG) {
    LOG("Dependencies loaded for %s", base_name);
    for (size_t n = 0; n < dependencies.GetCount(); ++n)
      LOG("  ... %p %s\n", dependencies[n], dependencies[n]->GetName());
    LOG("    dependencies @%p\n", &dependencies);
  }

  // Relocate the library.
  LOG("Relocating %s", base_name);
  if (!lib->Relocate(this, &preloaded_libraries_, &dependencies, error))
    return NULL;

  // Notify GDB of load.
  lib->link_map_.l_addr = lib->load_bias();
  lib->link_map_.l_name = const_cast<char*>(lib->base_name_);
  lib->link_map_.l_ld = reinterpret_cast<uintptr_t>(lib->view_.dynamic());
  Globals::GetRDebug()->AddEntry(&lib->link_map_);

  // The library was properly loaded, add it to the list of crazy
  // libraries. IMPORTANT: Do this _before_ calling the constructors
  // because these could call dlopen().
  lib->list_next_ = head_;
  lib->list_prev_ = NULL;
  if (head_)
    head_->list_prev_ = lib.Get();
  head_ = lib.Get();

  // Then create a new LibraryView for it.
  // TODO(digit): Use the library's soname() instead of |lib_name| here.
  // This is not possible yet because the current code relies on the fact
  // that lib_name is /data/data/..../base.apk + a file offset at the moment
  // to perform RELRO sharing properly. This will be fixed in a future CL
  // that also modifies the client code in chromium_android_linker.
  wrap = new LibraryView(lib.Get(), lib_name);
  known_libraries_.PushBack(wrap);

  LOG("Running constructors for %s", base_name);

  // Now run the constructors.
  lib->CallConstructors();

  LOG("Done loading %s", base_name);
  lib.Release();

  return wrap;
}

// We identify the abi tag for which the linker is running. This allows
// us to select the library which matches the abi of the linker.

#if defined(__arm__) && defined(__ARM_ARCH_7A__)
#define CURRENT_ABI "armeabi-v7a"
#elif defined(__arm__)
#define CURRENT_ABI "armeabi"
#elif defined(__i386__)
#define CURRENT_ABI "x86"
#elif defined(__mips__)
#define CURRENT_ABI "mips"
#elif defined(__x86_64__)
#define CURRENT_ABI "x86_64"
#elif defined(__aarch64__)
#define CURRENT_ABI "arm64-v8a"
#else
#error "Unsupported target abi"
#endif

int LibraryList::FindMappableLibraryInZipFile(
    const char* zip_file_path,
    const char* lib_name,
    Error* error) {
  String path("lib/" CURRENT_ABI "/crazy.");
  path += lib_name;
  if (path.size() >= kMaxFilePathLengthInZip) {
    error->Format("Filename too long for a file in a zip file %s\n",
                  path.c_str());
    return CRAZY_OFFSET_FAILED;
  }

  int32_t offset = FindStartOffsetOfFileInZipFile(zip_file_path, path.c_str());
  if (offset == CRAZY_OFFSET_FAILED) {
    return CRAZY_OFFSET_FAILED;
  }

  COMPILE_ASSERT((kZipAlignmentPageSize & (kZipAlignmentPageSize - 1)) == 0,
                 kZipAlignmentPageSize_must_be_a_power_of_2);

  if ((offset & (kZipAlignmentPageSize - 1)) != 0) {
    error->Format("Library %s is not page aligned in zipfile %s\n",
                  lib_name, zip_file_path);
    return CRAZY_OFFSET_FAILED;
  }

  assert(offset != CRAZY_OFFSET_FAILED);
  return offset;
}

LibraryView* LibraryList::LoadLibraryInZipFile(
    const char* zip_file_path,
    const char* lib_name,
    int dlopen_flags,
    uintptr_t load_address,
    SearchPathList* search_path_list,
    bool is_dependency_or_preload,
    Error* error) {
  int offset = FindMappableLibraryInZipFile(zip_file_path, lib_name, error);
  if (offset == CRAZY_OFFSET_FAILED) {
    return NULL;
  }

  return LoadLibrary(
      zip_file_path, dlopen_flags, load_address, offset,
      search_path_list, is_dependency_or_preload, error);
}

void LibraryList::AddLibrary(LibraryView* wrap) {
  known_libraries_.PushBack(wrap);
}

LibraryView* LibraryList::FindKnownLibrary(const char* name) {
  const char* base_name = GetBaseNamePtr(name);
  for (size_t n = 0; n < known_libraries_.GetCount(); ++n) {
    LibraryView* wrap = known_libraries_[n];
    if (!strcmp(base_name, wrap->GetName()))
      return wrap;
  }
  return NULL;
}

}  // namespace crazy
