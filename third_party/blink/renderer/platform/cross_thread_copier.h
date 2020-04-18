/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_CROSS_THREAD_COPIER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_CROSS_THREAD_COPIER_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/interface_ptr_info.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"  // FunctionThreadAffinity
#include "third_party/blink/renderer/platform/wtf/type_traits.h"

namespace base {
template <typename, typename>
class RefCountedThreadSafe;
}

class SkRefCnt;
template <typename T>
class sk_sp;

namespace WTF {

template <typename T>
class PassedWrapper;
}

namespace blink {

class IntRect;
class IntSize;
class KURL;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
struct CrossThreadResourceResponseData;
struct CrossThreadResourceRequestData;
template <typename T>
class CrossThreadPersistent;
template <typename T>
class CrossThreadWeakPersistent;

template <typename T>
struct CrossThreadCopierPassThrough {
  STATIC_ONLY(CrossThreadCopierPassThrough);
  typedef T Type;
  static Type Copy(const T& parameter) { return parameter; }
};

template <typename T, bool isArithmeticOrEnum>
struct CrossThreadCopierBase;

// Arithmetic values (integers or floats) and enums can be safely copied.
template <typename T>
struct CrossThreadCopierBase<T, true> : public CrossThreadCopierPassThrough<T> {
  STATIC_ONLY(CrossThreadCopierBase);
};

template <typename T>
struct CrossThreadCopier
    : public CrossThreadCopierBase<T,
                                   std::is_arithmetic<T>::value ||
                                       std::is_enum<T>::value> {
  STATIC_ONLY(CrossThreadCopier);
};

// CrossThreadCopier specializations follow.
template <typename T>
struct CrossThreadCopier<WTF::RetainedRefWrapper<T>> {
  STATIC_ONLY(CrossThreadCopier);
  static_assert(WTF::IsSubclassOfTemplate<T, base::RefCountedThreadSafe>::value,
                "scoped_refptr<T> can be passed across threads only if T is "
                "WTF::ThreadSafeRefCounted or base::RefCountedThreadSafe.");
  using Type = WTF::RetainedRefWrapper<T>;
  static Type Copy(Type pointer) { return pointer; }
};
template <typename T>
struct CrossThreadCopier<scoped_refptr<T>> {
  STATIC_ONLY(CrossThreadCopier);
  static_assert(WTF::IsSubclassOfTemplate<T, base::RefCountedThreadSafe>::value,
                "scoped_refptr<T> can be passed across threads only if T is "
                "WTF::ThreadSafeRefCounted or base::RefCountedThreadSafe.");
  using Type = scoped_refptr<T>;
  static scoped_refptr<T> Copy(scoped_refptr<T> pointer) { return pointer; }
};
template <typename T>
struct CrossThreadCopier<sk_sp<T>>
    : public CrossThreadCopierPassThrough<sk_sp<T>> {
  STATIC_ONLY(CrossThreadCopier);
  static_assert(std::is_base_of<SkRefCnt, T>::value,
                "sk_sp<T> can be passed across threads only if T is SkRefCnt.");
};

// nullptr_t can be passed through without any changes.
template <>
struct CrossThreadCopier<std::nullptr_t>
    : public CrossThreadCopierPassThrough<std::nullptr_t> {
  STATIC_ONLY(CrossThreadCopier);
};

// To allow a type to be passed across threads using its copy constructor, add a
// forward declaration of the type and provide a specialization of
// CrossThreadCopier<T> in this file, like IntRect below.
template <>
struct CrossThreadCopier<IntRect>
    : public CrossThreadCopierPassThrough<IntRect> {
  STATIC_ONLY(CrossThreadCopier);
};

template <>
struct CrossThreadCopier<IntSize>
    : public CrossThreadCopierPassThrough<IntSize> {
  STATIC_ONLY(CrossThreadCopier);
};

template <typename T, typename Deleter>
struct CrossThreadCopier<std::unique_ptr<T, Deleter>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = std::unique_ptr<T, Deleter>;
  static std::unique_ptr<T, Deleter> Copy(std::unique_ptr<T, Deleter> pointer) {
    return pointer;  // This is in fact a move.
  }
};

template <typename T, size_t inlineCapacity, typename Allocator>
struct CrossThreadCopier<
    Vector<std::unique_ptr<T>, inlineCapacity, Allocator>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = Vector<std::unique_ptr<T>, inlineCapacity, Allocator>;
  static Type Copy(Type pointer) {
    return pointer;  // This is in fact a move.
  }
};

template <size_t inlineCapacity, typename Allocator>
struct CrossThreadCopier<Vector<uint64_t, inlineCapacity, Allocator>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = Vector<uint64_t, inlineCapacity, Allocator>;
  static Type Copy(Type value) { return value; }
};

template <typename T>
struct CrossThreadCopier<CrossThreadPersistent<T>>
    : public CrossThreadCopierPassThrough<CrossThreadPersistent<T>> {
  STATIC_ONLY(CrossThreadCopier);
};

template <typename T>
struct CrossThreadCopier<CrossThreadWeakPersistent<T>>
    : public CrossThreadCopierPassThrough<CrossThreadWeakPersistent<T>> {
  STATIC_ONLY(CrossThreadCopier);
};

template <typename T>
struct CrossThreadCopier<WTF::CrossThreadUnretainedWrapper<T>>
    : public CrossThreadCopierPassThrough<
          WTF::CrossThreadUnretainedWrapper<T>> {
  STATIC_ONLY(CrossThreadCopier);
};

template <typename T>
struct CrossThreadCopier<base::WeakPtr<T>>
    : public CrossThreadCopierPassThrough<base::WeakPtr<T>> {
  STATIC_ONLY(CrossThreadCopier);
};

template <typename T>
struct CrossThreadCopier<WTF::PassedWrapper<T>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = WTF::PassedWrapper<typename CrossThreadCopier<T>::Type>;
  static Type Copy(WTF::PassedWrapper<T>&& value) {
    return WTF::Passed(CrossThreadCopier<T>::Copy(value.MoveOut()));
  }
};

template <typename Signature>
struct CrossThreadCopier<WTF::CrossThreadFunction<Signature>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = WTF::CrossThreadFunction<Signature>;
  static Type Copy(Type&& value) { return std::move(value); }
};

template <>
struct CrossThreadCopier<KURL> {
  STATIC_ONLY(CrossThreadCopier);
  typedef KURL Type;
  PLATFORM_EXPORT static Type Copy(const KURL&);
};

template <>
struct CrossThreadCopier<String> {
  STATIC_ONLY(CrossThreadCopier);
  typedef String Type;
  PLATFORM_EXPORT static Type Copy(const String&);
};

template <>
struct CrossThreadCopier<ResourceError> {
  STATIC_ONLY(CrossThreadCopier);
  typedef ResourceError Type;
  PLATFORM_EXPORT static Type Copy(const ResourceError&);
};

template <>
struct CrossThreadCopier<ResourceRequest> {
  STATIC_ONLY(CrossThreadCopier);
  typedef WTF::PassedWrapper<std::unique_ptr<CrossThreadResourceRequestData>>
      Type;
  PLATFORM_EXPORT static Type Copy(const ResourceRequest&);
};

template <>
struct CrossThreadCopier<ResourceResponse> {
  STATIC_ONLY(CrossThreadCopier);
  typedef WTF::PassedWrapper<std::unique_ptr<CrossThreadResourceResponseData>>
      Type;
  PLATFORM_EXPORT static Type Copy(const ResourceResponse&);
};

// mojo::InterfacePtrInfo is a cross-thread safe mojo::InterfacePtr.
template <typename Interface>
struct CrossThreadCopier<mojo::InterfacePtrInfo<Interface>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = mojo::InterfacePtrInfo<Interface>;
  static Type Copy(Type ptr_info) {
    return ptr_info;  // This is in fact a move.
  }
};

template <typename Interface>
struct CrossThreadCopier<mojo::InterfaceRequest<Interface>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = mojo::InterfaceRequest<Interface>;
  static Type Copy(Type request) {
    return request;  // This is in fact a move.
  }
};

template <>
struct CrossThreadCopier<MessagePortChannel> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = MessagePortChannel;
  static Type Copy(Type pointer) {
    return pointer;  // This is in fact a move.
  }
};

template <size_t inlineCapacity, typename Allocator>
struct CrossThreadCopier<
    Vector<MessagePortChannel, inlineCapacity, Allocator>> {
  STATIC_ONLY(CrossThreadCopier);
  using Type = Vector<MessagePortChannel, inlineCapacity, Allocator>;
  static Type Copy(Type pointer) {
    return pointer;  // This is in fact a move.
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_CROSS_THREAD_COPIER_H_
