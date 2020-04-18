// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_H
#define CRAZY_LINKER_H

// This is the crazy linker, a custom dynamic linker that can be used
// by NDK applications to load shared libraries (not executables) with
// a twist.
//
// Compared to the dynamic linker, the crazy one has the following
// features:
//
//   - It can use an arbitrary search path.
//
//   - It can load a library at a memory fixed address.
//
//   - It can load libraries from zip archives (as long as they are
//     page aligned and uncompressed). Even when running on pre-Android M
//     systems.
//
//   - It can share the RELRO section between two libraries
//     loaded at the same address in two distinct processes.
//
#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Function attribute to indicate that it needs to be exported by
// the library.
#define _CRAZY_PUBLIC __attribute__((__visibility__("default")))

// Maximum path length of a file in a zip file.
const size_t kMaxFilePathLengthInZip = 256;

// Status values returned by crazy linker functions to indicate
// success or failure. They were chosen to match boolean values,
// this allows one to test for failures with:
//
//    if (!crazy_linker_function(....)) {
//       ... an error occured.
//    }
//
// If the function called used a crazy_context_t, it is possible to
// retrieve the error details with crazy_context_get_error().
typedef enum {
  CRAZY_STATUS_FAILURE = 0,
  CRAZY_STATUS_SUCCESS = 1
} crazy_status_t;

// Opaque handle to a context object that will hold parameters
// for the crazy linker's operations. For example, this is where
// you would set the explicit load address, and other user-provided
// values before calling functions like crazy_library_open().
//
// The context holds a list of library search paths, initialized to
// the content of the LD_LIBRARY_PATH variable on creation.
//
// The context also holds a string buffer to hold error messages that
// can be queried with crazy_context_get_error().
typedef struct crazy_context_t crazy_context_t;

// Create a new context object.
// Note that this calls crazy_context_reset_search_paths().
crazy_context_t* crazy_context_create(void) _CRAZY_PUBLIC;

// Return current error string, or NULL if there was no error.
const char* crazy_context_get_error(crazy_context_t* context) _CRAZY_PUBLIC;

// Clear error in a given context.
void crazy_context_clear_error(crazy_context_t* context) _CRAZY_PUBLIC;

// Set the explicit load address in a context object. Value 0 means
// the address is randomized.
void crazy_context_set_load_address(crazy_context_t* context,
                                    size_t load_address) _CRAZY_PUBLIC;

// Return the current load address in a context.
size_t crazy_context_get_load_address(crazy_context_t* context) _CRAZY_PUBLIC;

// Add one or more paths to the list of library search paths held
// by a given context. |path| is a string using a column (:) as a
// list separator. As with the PATH variable, an empty list item
// is equivalent to '.', the current directory.
// This can fail if too many paths are added to the context.
//
// NOTE: Calling this function appends new paths to the search list,
// but all paths added with this function will be searched before
// the ones listed in LD_LIBRARY_PATH.
crazy_status_t crazy_context_add_search_path(
    crazy_context_t* context,
    const char* file_path) _CRAZY_PUBLIC;

// Find the ELF binary that contains |address|, and add its directory
// path to the context's list of search directories. This is useful to
// load libraries in the same directory than the current program itself.
crazy_status_t crazy_context_add_search_path_for_address(
    crazy_context_t* context,
    void* address) _CRAZY_PUBLIC;

// Reset the search paths to the value of the LD_LIBRARY_PATH
// environment variable. This essentially removes any paths
// that were added with crazy_context_add_search_path() or
// crazy_context_add_search_path_for_address().
void crazy_context_reset_search_paths(crazy_context_t* context) _CRAZY_PUBLIC;

// Record the value of |java_vm| inside of |context|. If this is not NULL,
// which is the default, then after loading any library, the crazy linker
// will look for a "JNI_OnLoad" symbol within it, and, if it exists, will call
// it, passing the value of |java_vm| to it. If the function returns with
// a jni version number that is smaller than |minimum_jni_version|, then
// the library load will fail with an error.
//
// The |java_vm| field is also saved in the crazy_library_t object, and
// used at unload time to call JNI_OnUnload() if it exists.
void crazy_context_set_java_vm(crazy_context_t* context,
                               void* java_vm,
                               int minimum_jni_version);

// Retrieves the last values set with crazy_context_set_java_vm().
// A value of NUMM in |*java_vm| means the feature is disabled.
void crazy_context_get_java_vm(crazy_context_t* context,
                               void** java_vm,
                               int* minimum_jni_version);

// Destroy a given context object.
void crazy_context_destroy(crazy_context_t* context) _CRAZY_PUBLIC;

// Some operations performed by the crazy linker might conflict with the
// system linker if they are used concurrently in different threads
// (e.g. modifying the list of shared libraries seen by GDB). To work
// around this, the crazy linker provides a way to delay these conflicting
// operations for a later time.
//
// This works by wrapping each of these operations in a small data structure
// (crazy_callback_t), which can later be passed to crazy_callback_run()
// to execute it.
//
// The user must provide a function to record these callbacks during
// library loading, by calling crazy_linker_set_callback_poster().
//
// Once all libraries are loaded, the callbacks can be later called either
// in a different thread, or when it is safe to assume the system linker
// cannot be running in parallel.

// Callback handler.
typedef void (*crazy_callback_handler_t)(void* opaque);

// A small structure used to model a callback provided by the crazy linker.
// Use crazy_callback_run() to run the callback.
typedef struct {
  crazy_callback_handler_t handler;
  void* opaque;
} crazy_callback_t;

// Function to call to enable a callback into the crazy linker when delayed
// operations are enabled (see crazy_context_set_callback_poster). A call
// to crazy_callback_poster_t returns true if the callback was successfully
// set up and will occur later, false if callback could not be set up (and
// so will never occur).
typedef bool (*crazy_callback_poster_t)(
    crazy_callback_t* callback, void* poster_opaque);

// Enable delayed operation, by passing the address of a
// |crazy_callback_poster_t| function, that will be called during library
// loading to let the user record callbacks for delayed operations.
// Callers must copy the |crazy_callback_t| passed to |poster|.
//
// Note: If client code calls this function to supply a callback poster,
// it must guarantee to invoke any callback requested through the poster.
// The call will be (typically) on another thread, but may instead be
// immediate from the poster. However, the callback must be invoked,
// otherwise if it is a blocking callback the crazy linker will deadlock
// waiting for it.
//
// |poster_opaque| is an opaque value for client code use, passed back
// on each call to |poster|.
// |poster| can be NULL to disable the feature.
void crazy_context_set_callback_poster(crazy_context_t* context,
                                       crazy_callback_poster_t poster,
                                       void* poster_opaque);

// Return the address of the function that the crazy linker can use to
// request callbacks, and the |poster_opaque| passed back on each call
// to |poster|. |poster| is NULL if the feature is disabled.
void crazy_context_get_callback_poster(crazy_context_t* context,
                                       crazy_callback_poster_t* poster,
                                       void** poster_opaque);

// Run a given |callback| in the current thread. Must only be called once
// per callback.
void crazy_callback_run(crazy_callback_t* callback);

// Pass the platform's SDK build version to the crazy linker. The value is
// from android.os.Build.VERSION.SDK_INT.
void crazy_set_sdk_build_version(int sdk_build_version);

// Opaque handle to a library as seen/loaded by the crazy linker.
typedef struct crazy_library_t crazy_library_t;

// Try to open or load a library with the crazy linker.
// |lib_name| if the library name or path. If it contains a directory
// separator (/), this is treated as a explicit file path, otherwise
// it is treated as a base name, and the context's search path list
// will be used to locate the corresponding file.
// |context| is a linker context handle. Can be NULL for defaults.
// On success, return CRAZY_STATUS_SUCCESS and sets |*library|.
// Libraries are reference-counted, trying to open the same library
// twice will return the same library handle.
//
// NOTE: The load address and file offset from the context only apply
// to the library being loaded (when not already in the process). If the
// operations needs to load any dependency libraries, these will use
// offset and address values of 0 to do so.
//
// NOTE: It is possible to open NDK system libraries (e.g. "liblog.so")
// with this function, but they will be loaded with the system dlopen().
crazy_status_t crazy_library_open(crazy_library_t** library,
                                  const char* lib_name,
                                  crazy_context_t* context) _CRAZY_PUBLIC;

// Try to open or load a library with the crazy linker. The
// library is in a zip file with the name |zipfile_name|. Within the zip
// file the library must be uncompressed and page aligned. |zipfile_name|
// should be an absolute path name and |lib_name| should be a relative
// pathname. The library in the zip file is expected to have the name
// lib/<abi_tag>/crazy.<lib_name> where abi_tag is the abi directory matching
// the ABI for which the crazy linker was compiled. Note this does not support
// opening multiple libraries in the same zipfile, see crbug/388223.
crazy_status_t crazy_library_open_in_zip_file(crazy_library_t** library,
                                              const char* zipfile_name,
                                              const char* lib_name,
                                              crazy_context_t* context)
    _CRAZY_PUBLIC;

// A structure used to hold information about a given library.
// |load_address| is the library's actual (page-aligned) load address.
// |load_size| is the library's actual (page-aligned) size.
// |relro_start| is the address of the library's RELRO section in memory.
// |relso_size| is the size of the library's RELRO section (or 0 if none).
// |relro_fd| is the ashmem file descriptor for the shared section, if one
// was created with crazy_library_enable_relro_sharing(), -1 otherwise.
typedef struct {
  size_t load_address;
  size_t load_size;
  size_t relro_start;
  size_t relro_size;
} crazy_library_info_t;

// Retrieve information about a given library.
// |library| is a library handle.
// |context| will get an error message on failure.
// On success, return true and sets |*info|.
// Note that this function will fail for system libraries.
crazy_status_t crazy_library_get_info(crazy_library_t* library,
                                      crazy_context_t* context,
                                      crazy_library_info_t* info);

// Create an ashmem region containing a copy of the RELRO section for a given
// |library|. This can be used with crazy_library_use_shared_relro().
// |load_address| can be specified as non-0 to ensure that the content of the
// ashmem region corresponds to a RELRO relocated for a new load address.
// on success, return CRAZY_STATUS_SUCCESS and sets |*relro_start| to the
// start of the RELRO section in memory, |*relro_size| to its size in bytes
// and |*relro_fd| to a file descriptor to a read-only ashmem region containing
// the data. On failure, return false and set error message in |context|.
// NOTE: On success, the caller becomes the owner of |*relro_fd| and is shall
// close it appropriately.
crazy_status_t crazy_library_create_shared_relro(crazy_library_t* library,
                                                 crazy_context_t* context,
                                                 size_t load_address,
                                                 size_t* relro_start,
                                                 size_t* relro_size,
                                                 int* relro_fd) _CRAZY_PUBLIC;

// Use the shared RELRO section of the same library loaded in a different
// address space. On success, return CRAZY_STATUS_SUCCESS and owns |relro_fd|.
// On failure, return CRAZY_STATUS_FAILURE and sets error message in |context|.
// |library| is the library handle.
// |relro_start| is the address of the RELRO section in memory.
// |relro_size| is the size of the RELRO section.
// |relro_fd| is the file descriptor for the shared RELRO ashmem region.
// |context| will receive an error in case of failure.
// NOTE: This will fail if this is a system library, or if the RELRO
// parameters do not match the library's actual load address.
// NOTE: The caller is responsible for closing the file descriptor after this
// call.
crazy_status_t crazy_library_use_shared_relro(crazy_library_t* library,
                                              crazy_context_t* context,
                                              size_t relro_start,
                                              size_t relro_size,
                                              int relro_fd) _CRAZY_PUBLIC;

// Look for a library named |library_name| in the set of currently
// loaded libraries, and return a handle for it in |*library| on success.
// Note that this increments the reference count on the library, thus
// the caller shall call crazy_library_close() when it's done with it.
crazy_status_t crazy_library_find_by_name(const char* library_name,
                                          crazy_library_t** library);

// Find the library that contains a given |address| in memory.
// On success, return CRAZY_STATUS_SUCCESS and sets |*library|.
crazy_status_t crazy_linker_find_library_from_address(
    void* address,
    crazy_library_t** library) _CRAZY_PUBLIC;

// Lookup a symbol's address by its |symbol_name| in a given library.
// This only looks at the symbols in |library|.
// On success, returns CRAZY_STATUS_SUCCESS and sets |*symbol_address|,
// which could be NULL for some symbols.
crazy_status_t crazy_library_find_symbol(crazy_library_t* library,
                                         const char* symbol_name,
                                         void** symbol_address) _CRAZY_PUBLIC;

// Lookup a symbol's address in all libraries known by the crazy linker.
// |symbol_name| is the symbol name. On success, returns CRAZY_STATUS_SUCCESS
// and sets |*symbol_address|.
// NOTE: This will _not_ look into system libraries that were not opened
// with the crazy linker.
crazy_status_t crazy_linker_find_symbol(const char* symbol_name,
                                        void** symbol_address) _CRAZY_PUBLIC;

// Find the in-process library that contains a given memory address.
// Note that this works even if the memory is inside a system library that
// was not previously opened with crazy_library_open().
// |address| is the memory address.
// On success, returns CRAZY_STATUS_SUCCESS and sets |*library|.
// The caller muyst call crazy_library_close() once it's done with the
// library.
crazy_status_t crazy_library_find_from_address(
    void* address,
    crazy_library_t** library) _CRAZY_PUBLIC;

// Close a library. This decrements its reference count. If it reaches
// zero, the library be unloaded from the process.
void crazy_library_close(crazy_library_t* library) _CRAZY_PUBLIC;

// Close a library, with associated context to support delayed operations.
void crazy_library_close_with_context(crazy_library_t* library,
                                      crazy_context_t* context) _CRAZY_PUBLIC;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CRAZY_LINKER_H */
