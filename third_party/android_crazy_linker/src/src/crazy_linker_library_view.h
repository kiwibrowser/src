// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_LIBRARY_VIEW_H
#define CRAZY_LINKER_LIBRARY_VIEW_H

#include "crazy_linker_error.h"
#include "crazy_linker_util.h"

namespace crazy {

class SharedLibrary;

// A LibraryView is a reference-counted handle to either a
// crazy::SharedLibrary object or a library handle returned by the system's
// dlopen() function.
//
// It has a name, which normally corresponds to the DT_SONAME entry in the
// dynamic table (i.e. the name embedded within the ELF file, which does not
// always matches its file path name).
class LibraryView {
 public:
  enum {
    TYPE_NONE = 0xbaadbaad,
    TYPE_SYSTEM = 0x2387cef,
    TYPE_CRAZY = 0xcdef2387,
  };

  // Constructor for wrapping a system library handle.
  // |system_lib| is the dlopen() handle, and |lib_name| should be the
  // library soname for it.
  LibraryView(void* system_handle, const char* lib_name)
      : type_(TYPE_SYSTEM), system_(system_handle), name_(lib_name) {}

  // Constructor for wrapping a crazy::SharedLibrary instance.
  LibraryView(SharedLibrary* crazy_lib, const char* lib_name)
      : type_(TYPE_CRAZY), crazy_(crazy_lib), name_(lib_name) {}

  // Constructor for warpping a crazy::SharedLibrary instance.
  // This version takes the library name from the its soname() value.
  LibraryView(SharedLibrary* crazy_lib);

  // Destructor, this will either dlclose() a system library, or delete
  // a SharedLibrary instance.
  ~LibraryView();

  // Returns true iff this is a system library handle.
  bool IsSystem() const { return type_ == TYPE_SYSTEM; }

  // Returns true iff this is a SharedLibrary instance.
  bool IsCrazy() const { return type_ == TYPE_CRAZY; }

  // Returns the soname of the current library (or its base name if not
  // available).
  const char* GetName() { return name_.c_str(); }

  SharedLibrary* GetCrazy() { return IsCrazy() ? crazy_ : NULL; }

  void* GetSystem() { return IsSystem() ? system_ : NULL; }

  // Increment reference count for this LibraryView.
  void AddRef() { ref_count_++; }

  // Decrement reference count. Returns true iff it reaches 0.
  // This never destroys the object.
  bool SafeDecrementRef() { return (--ref_count_ == 0); }

  // Lookup a symbol from this library.
  // If this is a crazy library, perform a breadth-first search,
  // for system libraries, use dlsym() instead.
  void* LookupSymbol(const char* symbol_name);

  // Retrieve library information.
  bool GetInfo(size_t* load_address,
               size_t* load_size,
               size_t* relro_start,
               size_t* relro_size,
               Error* error);

  // Only used for debugging.
  int ref_count() const { return ref_count_; }

 private:
  uint32_t type_ = TYPE_NONE;
  int ref_count_ = 1;
  union {
    SharedLibrary* crazy_ = nullptr;
    void* system_;
  };
  String name_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_LIBRARY_VIEW_H
