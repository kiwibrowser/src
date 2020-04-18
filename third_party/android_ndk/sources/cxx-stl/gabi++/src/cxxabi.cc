// Copyright (C) 2012 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include <limits.h>
#include <string.h>
#include <sys/mman.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <pthread.h>

#include "cxxabi_defines.h"
#include "helper_func_internal.h"

namespace {

  using namespace __cxxabiv1;

  bool isOurCxxException(uint64_t exc) {
    // Compatible with GNU
    return exc == __gxx_exception_class;
  }

  void defaultExceptionCleanupFunc(_Unwind_Reason_Code reason,
                                   _Unwind_Exception* exc) {
    __cxa_free_exception(exc+1);
  }

  // Helper class used to ensure a lock is acquire immediately, and released
  // on scope exit. Usage example:
  //
  //     {
  //       AutoLock lock(some_mutex);   // acquires the mutex.
  //       ... do stuff
  //       if (error)
  //          return;                   // releases mutex before returning.
  //       ... do other stuff.
  //     }                              // releases mutex before exiting scope.
  //
  class AutoLock {
  public:
    AutoLock(pthread_mutex_t& lock) : lock_(lock) {
      pthread_mutex_lock(&lock_);
    }

    ~AutoLock(void) {
      pthread_mutex_unlock(&lock_);
    }
  private:
    pthread_mutex_t& lock_;

    AutoLock(const AutoLock&);
    AutoLock& operator=(const AutoLock&);
  };

  // MMap-based memory allocator for fixed-sized items.
  //
  // IMPORTANT: This must be POD-struct compatible, which means:
  //    - No constructor or destructor.
  //    - No virtual methods.
  //
  // This allocates large blocks of memory, called 'slabs' that can contain
  // several items of the same size. A slab contains an array of item slots,
  // followed by a pointer, used to put all slabs in a single linked list.
  class PageBasedAllocator {
  public:
    // Used to initialize this allocator to hold items of type |T|.
    template <typename T>
    void Init() {
      InitExplicit(sizeof(T), __alignof__(T));
    }

    // Used to initialize this instance to hold items of |item_size| bytes,
    // with alignment |align_size|.
    void InitExplicit(size_t item_size, size_t align_size) {
      const size_t ptr_size = sizeof(void*);
      if (align_size < ptr_size)
        align_size = ptr_size;
      item_size_ = (item_size + align_size - 1) & ~(align_size - 1);
      slab_next_offset_ = kSlabSize - ptr_size;
      item_slab_count_ = slab_next_offset_ / item_size_;

      pthread_mutex_init(&lock_, NULL);
      free_items_ = NULL;
      slab_list_ = NULL;
    }

    // Call this to deallocate this instance. This releases all pages directly.
    // Ensure that all items are freed first, or bad things could happen.
    void Deinit() {
      pthread_mutex_lock(&lock_);
      while (slab_list_) {
        void* slab = slab_list_;
        void* next_slab = *(void**)((char*)slab + slab_next_offset_);
        slab_list_ = next_slab;
        ::munmap(slab, PAGE_SIZE);
      }
      pthread_mutex_unlock(&lock_);
      pthread_mutex_destroy(&lock_);
    }

    // Allocate a new item, or NULL in case of failure.
    void* Alloc() {
      AutoLock lock(lock_);

      if (!free_items_ && !AllocateSlab())
        return NULL;

      FreeItem* item = free_items_;
      free_items_ = item->next;
      ::memset(item, 0, item_size_);
      return item;
    }

    void Release(void* obj) {
      if (!obj)
        return;

      AutoLock lock(lock_);
      FreeItem* item = reinterpret_cast<FreeItem*>(obj);
      item->next = free_items_;
      free_items_ = item;
    }

  private:
    static const size_t kSlabSize = PAGE_SIZE;

    bool AllocateSlab() {
      // No more free items, allocate a new slab with mmap().
      void* new_slab = mmap(NULL, kSlabSize, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      if (new_slab == MAP_FAILED)
        return false;

      // Prepend to the slab list.
      *((void**)((char*)new_slab + slab_next_offset_)) = slab_list_;
      slab_list_ = new_slab;

      // Put all item slots in the new slab into the free item list.
      FreeItem** pparent = &free_items_;
      FreeItem* item = reinterpret_cast<FreeItem*>(new_slab);
      for (size_t n = 0; n < item_slab_count_; ++n) {
        *pparent = item;
        pparent = &item->next;
        item = reinterpret_cast<FreeItem*>((char*)item + item_size_);
      }
      *pparent = NULL;

      // Done.
      return true;
    }

    struct FreeItem {
      FreeItem* next;
    };

    size_t item_size_;         // size of each item in bytes.
    size_t item_slab_count_;   // number of items in each slab.
    size_t slab_next_offset_;  // offset of pointer to next slab in list.
    pthread_mutex_t lock_;     // mutex synchronizing access to data below.
    void* slab_list_;          // Linked list of slabs.
    FreeItem* free_items_;     // Linked list of free items.
  };

  // Technical note:
  // Use a pthread_key_t to hold the key used to store our thread-specific
  // __cxa_eh_globals objects. The key is created and destroyed through
  // a static C++ object.
  //

  // Due to a bug in the dynamic linker that was only fixed in Froyo, the
  // static C++ destructor may be called with a value of NULL for the
  // 'this' pointer. As such, any attempt to access any field in the
  // object there will result in a crash. To work-around this, store
  // the members of CxaThreadKey as static variables outside of the
  // C++ object.
  static pthread_key_t __cxa_thread_key;
  static PageBasedAllocator __cxa_eh_globals_allocator;

  class CxaThreadKey {
  public:
    // Called at program initialization time, or when the shared library
    // is loaded through dlopen().
    CxaThreadKey() {
      if (pthread_key_create(&__cxa_thread_key, freeObject) != 0)
        __gabixx::__fatal_error("Can't allocate C++ runtime pthread_key_t");
      __cxa_eh_globals_allocator.Init<__cxa_eh_globals>();
    }

    // Called at program exit time, or when the shared library is
    // unloaded through dlclose(). See note above.
    ~CxaThreadKey() {
      __cxa_eh_globals_allocator.Deinit();
      pthread_key_delete(__cxa_thread_key);
    }

    static __cxa_eh_globals* getFast() {
      void* obj = pthread_getspecific(__cxa_thread_key);
      return reinterpret_cast<__cxa_eh_globals*>(obj);
    }

    static __cxa_eh_globals* getSlow() {
      void* obj = pthread_getspecific(__cxa_thread_key);
      if (obj == NULL) {
        // malloc() cannot be used here because this method is sometimes
        // called from malloc() on Android, and this would dead-lock.
        //
        // More specifically, if the libc.debug.malloc system property is not 0
        // on a userdebug or eng build of the platform, malloc() will call
        // backtrace() to record stack traces of allocation. On ARM, this
        // forces an unwinding operation which will call this function at
        // some point.
        obj = __cxa_eh_globals_allocator.Alloc();
        if (!obj) {
          // Shouldn't happen, but better be safe than sorry.
          __gabixx::__fatal_error(
              "Can't allocate thread-specific C++ runtime info block.");
        }
        pthread_setspecific(__cxa_thread_key, obj);
      }
      return reinterpret_cast<__cxa_eh_globals*>(obj);
    }

  private:
    // Called when a thread is destroyed.
    static void freeObject(void* obj) {
      __cxa_eh_globals_allocator.Release(obj);
    }

  };

  // The single static instance, this forces the compiler to register
  // a constructor and destructor for this object in the final library
  // file. They handle the pthread_key_t allocation/deallocation.
  static CxaThreadKey instance;

  _GABIXX_NORETURN void throwException(__cxa_exception *header) {
    __cxa_eh_globals* globals = __cxa_get_globals();
    header->unexpectedHandler = std::get_unexpected();
    header->terminateHandler = std::get_terminate();
    globals->uncaughtExceptions += 1;

    _Unwind_RaiseException(&header->unwindHeader);

    // Should not be here
    call_terminate(&header->unwindHeader);
  }

} // anonymous namespace


namespace __cxxabiv1 {
  __shim_type_info::~__shim_type_info() {
}  // namespace __cxxabiv1

  extern "C" void __cxa_pure_virtual() {
    __gabixx::__fatal_error("Pure virtual function called!");
  }

  extern "C" void __cxa_deleted_virtual() {
    __gabixx::__fatal_error("Deleted virtual function called!");
  }

  extern "C" __cxa_eh_globals* __cxa_get_globals() _GABIXX_NOEXCEPT {
    return CxaThreadKey::getSlow();
  }

  extern "C" __cxa_eh_globals* __cxa_get_globals_fast() _GABIXX_NOEXCEPT {
    return CxaThreadKey::getFast();
  }

  extern "C" void *__cxa_allocate_exception(size_t thrown_size) _GABIXX_NOEXCEPT {
    size_t size = thrown_size + sizeof(__cxa_exception);
    __cxa_exception *buffer = static_cast<__cxa_exception*>(memalign(__alignof__(__cxa_exception), size));
    if (!buffer) {
      // Since Android uses memory-overcommit, we enter here only when
      // the exception object is VERY large. This will propably never happen.
      // Therefore, we decide to use no emergency spaces.
      __gabixx::__fatal_error("Not enough memory to allocate exception!");
    }

    ::memset(buffer, 0, sizeof(__cxa_exception));
    return buffer + 1;
  }

  extern "C" void __cxa_free_exception(void* thrown_exception) _GABIXX_NOEXCEPT {
    __cxa_exception *exc = static_cast<__cxa_exception*>(thrown_exception)-1;

    if (exc->exceptionDestructor) {
      try {
        exc->exceptionDestructor(thrown_exception);
      } catch (...) {
        __gabixx::__fatal_error("Exception destructor has thrown!");
      }
    }

    free(exc);
  }

  extern "C" void __cxa_throw(void* thrown_exc,
                              std::type_info* tinfo,
                              void (*dest)(void*)) {
    __cxa_exception* header = static_cast<__cxa_exception*>(thrown_exc)-1;
    header->exceptionType = tinfo;
    header->exceptionDestructor = dest;

    header->unwindHeader.exception_class = __gxx_exception_class;
    header->unwindHeader.exception_cleanup = defaultExceptionCleanupFunc;

    throwException(header);
  }

  extern "C" void __cxa_rethrow() {
    __cxa_eh_globals *globals = __cxa_get_globals();
    __cxa_exception* header = globals->caughtExceptions;
    _Unwind_Exception* exception = &header->unwindHeader;
    if (!header) {
      __gabixx::__fatal_error(
          "Attempting to rethrow an exception that doesn't exist!");
    }

    if (isOurCxxException(exception->exception_class)) {
      header->handlerCount = -header->handlerCount; // Set rethrow flag
    } else {
      globals->caughtExceptions = 0;
    }

    throwException(header);
  }

  extern "C" void* __cxa_begin_catch(void* exc) _GABIXX_NOEXCEPT {
    _Unwind_Exception *exception = static_cast<_Unwind_Exception*>(exc);
    __cxa_exception* header = reinterpret_cast<__cxa_exception*>(exception+1)-1;
    __cxa_eh_globals* globals = __cxa_get_globals();

    if (!isOurCxxException(exception->exception_class)) {
      if (globals->caughtExceptions) {
        __gabixx::__fatal_error("Can't handle non-C++ exception!");
      }
    }

    // Check rethrow flag
    header->handlerCount = (header->handlerCount < 0) ?
      (-header->handlerCount+1) : (header->handlerCount+1);

    if (header != globals->caughtExceptions) {
      header->nextException = globals->caughtExceptions;
      globals->caughtExceptions = header;
    }
    globals->uncaughtExceptions -= 1;

    return header->adjustedPtr;
  }

  extern "C" void __cxa_end_catch() _GABIXX_NOEXCEPT {
    __cxa_eh_globals *globals = __cxa_get_globals_fast();
    __cxa_exception *header = globals->caughtExceptions;
    _Unwind_Exception* exception = &header->unwindHeader;

    if (!header) {
      return;
    }

    if (!isOurCxxException(exception->exception_class)) {
      globals->caughtExceptions = 0;
      _Unwind_DeleteException(exception);
      return;
    }

    int count = header->handlerCount;
    if (count < 0) { // Rethrow
      if (++count == 0) {
        globals->caughtExceptions = header->nextException;
      }
    } else if (--count == 0) {
      globals->caughtExceptions = header->nextException;
      __cxa_free_exception(header+1);
      return;
    } else if (count < 0) {
      __gabixx::__fatal_error("Internal error during exception handling!");
    }

    header->handlerCount = count;
  }

  extern "C" void* __cxa_get_exception_ptr(void* exceptionObject) _GABIXX_NOEXCEPT {
    __cxa_exception* header =
      reinterpret_cast<__cxa_exception*>(
        reinterpret_cast<_Unwind_Exception *>(exceptionObject)+1)-1;
    return header->adjustedPtr;
  }

  extern "C" bool __cxa_uncaught_exception() _GABIXX_NOEXCEPT {
    __cxa_eh_globals* globals = __cxa_get_globals();
    if (globals == NULL)
      return false;
    return globals->uncaughtExceptions == 0;
  }

  extern "C" void __cxa_decrement_exception_refcount(void* exceptionObject)
      _GABIXX_NOEXCEPT {
    if (exceptionObject != NULL)
    {
      __cxa_exception* header =
          reinterpret_cast<__cxa_exception*>(exceptionObject)-1;
      if (__sync_sub_and_fetch(&header->referenceCount, 1) == 0)
        __cxa_free_exception(exceptionObject);
    }
  }

  extern "C" void __cxa_increment_exception_refcount(void* exceptionObject)
      _GABIXX_NOEXCEPT {
    if (exceptionObject != NULL)
    {
      __cxa_exception* header =
          reinterpret_cast<__cxa_exception*>(exceptionObject)-1;
      __sync_add_and_fetch(&header->referenceCount, 1);
    }
  }

  extern "C" void __cxa_rethrow_primary_exception(void* primary_exception) {
#if defined(LIBCXXABI)
// Only warn if we're building for libcxx since other libraries do not use
// this.
#warning "not implemented."
#endif /* defined(LIBCXXABI) */
  }

  extern "C" void* __cxa_current_primary_exception() _GABIXX_NOEXCEPT {
#if defined(LIBCXXABI)
// Only warn if we're building for libcxx since other libraries do not use
// this.
#warning "not implemented."
#endif /* defined(LIBCXXABI) */
    return NULL;
  }

} // namespace __cxxabiv1
