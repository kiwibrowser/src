/*
 * Copyright (C) 2006, 2009, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/platform/wtf/text/string_impl.h"

#include "build/build_config.h"

#if defined(OS_MACOSX)

#include <CoreFoundation/CoreFoundation.h>
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/retain_ptr.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace WTF {

namespace StringWrapperCFAllocator {

static StringImpl* g_current_string;

static const void* Retain(const void* info) {
  return info;
}

static void Release(const void*) {
  NOTREACHED();
}

static CFStringRef CopyDescription(const void*) {
  return CFSTR("WTF::String-based allocator");
}

static void* Allocate(CFIndex size, CFOptionFlags, void*) {
  StringImpl* underlying_string = 0;
  if (IsMainThread()) {
    underlying_string = g_current_string;
    if (underlying_string) {
      g_current_string = 0;
      underlying_string
          ->AddRef();  // Balanced by call to deref in deallocate below.
    }
  }
  StringImpl** header = static_cast<StringImpl**>(WTF::Partitions::FastMalloc(
      sizeof(StringImpl*) + size, WTF_HEAP_PROFILER_TYPE_NAME(StringImpl*)));
  *header = underlying_string;
  return header + 1;
}

static void* Reallocate(void* pointer, CFIndex new_size, CFOptionFlags, void*) {
  size_t new_allocation_size = sizeof(StringImpl*) + new_size;
  StringImpl** header = static_cast<StringImpl**>(pointer) - 1;
  DCHECK(!*header);
  header = static_cast<StringImpl**>(WTF::Partitions::FastRealloc(
      header, new_allocation_size, WTF_HEAP_PROFILER_TYPE_NAME(StringImpl*)));
  return header + 1;
}

static void DeallocateOnMainThread(void* header_pointer) {
  StringImpl** header = static_cast<StringImpl**>(header_pointer);
  StringImpl* underlying_string = *header;
  DCHECK(underlying_string);
  underlying_string->Release();  // Balanced by call to ref in allocate above.
  WTF::Partitions::FastFree(header);
}

static void Deallocate(void* pointer, void*) {
  StringImpl** header = static_cast<StringImpl**>(pointer) - 1;
  StringImpl* underlying_string = *header;
  if (!underlying_string) {
    WTF::Partitions::FastFree(header);
  } else {
    if (!IsMainThread()) {
      internal::CallOnMainThread(&DeallocateOnMainThread, header);
    } else {
      underlying_string
          ->Release();  // Balanced by call to ref in allocate above.
      WTF::Partitions::FastFree(header);
    }
  }
}

static CFIndex PreferredSize(CFIndex size, CFOptionFlags, void*) {
  // FIXME: If FastMalloc provided a "good size" callback, we'd want to use it
  // here.  Note that this optimization would help performance for strings
  // created with the allocator that are mutable, and those typically are only
  // created by callers who make a new string using the old string's allocator,
  // such as some of the call sites in CFURL.
  return size;
}

static CFAllocatorRef Create() {
  CFAllocatorContext context = {
      0,        0,          Retain,     Release,      CopyDescription,
      Allocate, Reallocate, Deallocate, PreferredSize};
  return CFAllocatorCreate(0, &context);
}

static CFAllocatorRef Allocator() {
  static CFAllocatorRef allocator = Create();
  return allocator;
}

}  // namespace StringWrapperCFAllocator

RetainPtr<CFStringRef> StringImpl::CreateCFString() {
  // Since garbage collection isn't compatible with custom allocators, we
  // can't use the NoCopy variants of CFStringCreate*() when GC is enabled.
  if (!length_ || !IsMainThread()) {
    if (Is8Bit())
      return AdoptCF(CFStringCreateWithBytes(
          0, reinterpret_cast<const UInt8*>(Characters8()), length_,
          kCFStringEncodingISOLatin1, false));
    return AdoptCF(CFStringCreateWithCharacters(
        0, reinterpret_cast<const UniChar*>(Characters16()), length_));
  }
  CFAllocatorRef allocator = StringWrapperCFAllocator::Allocator();

  // Put pointer to the StringImpl in a global so the allocator can store it
  // with the CFString.
  DCHECK(!StringWrapperCFAllocator::g_current_string);
  StringWrapperCFAllocator::g_current_string = this;

  CFStringRef string;
  if (Is8Bit())
    string = CFStringCreateWithBytesNoCopy(
        allocator, reinterpret_cast<const UInt8*>(Characters8()), length_,
        kCFStringEncodingISOLatin1, false, kCFAllocatorNull);
  else
    string = CFStringCreateWithCharactersNoCopy(
        allocator, reinterpret_cast<const UniChar*>(Characters16()), length_,
        kCFAllocatorNull);
  // CoreFoundation might not have to allocate anything, we clear currentString
  // in case we did not execute allocate().
  StringWrapperCFAllocator::g_current_string = 0;

  return AdoptCF(string);
}

// On StringImpl creation we could check if the allocator is the
// StringWrapperCFAllocator.  If it is, then we could find the original
// StringImpl and just return that.  But to do that we'd have to compute the
// offset from CFStringRef to the allocated block; the CFStringRef is *not* at
// the start of an allocated block. Testing shows 1000x more calls to
// createCFString than calls to the create functions with the appropriate
// allocator, so it's probably not urgent optimize that case.

}  // namespace WTF

#endif  // defined(OS_MACOSX)
