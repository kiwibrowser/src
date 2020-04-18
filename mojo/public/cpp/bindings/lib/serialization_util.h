// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_SERIALIZATION_UTIL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_SERIALIZATION_UTIL_H_

#include <stddef.h>
#include <stdint.h>

#include <queue>

#include "base/logging.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/lib/bindings_internal.h"
#include "mojo/public/cpp/bindings/lib/serialization_context.h"

namespace mojo {
namespace internal {

template <typename T>
struct HasIsNullMethod {
  template <typename U>
  static char Test(decltype(U::IsNull) *);
  template <typename U>
  static int Test(...);
  static const bool value = sizeof(Test<T>(0)) == sizeof(char);

 private:
  EnsureTypeIsComplete<T> check_t_;
};

template <
    typename Traits,
    typename UserType,
    typename std::enable_if<HasIsNullMethod<Traits>::value>::type* = nullptr>
bool CallIsNullIfExists(const UserType& input) {
  return Traits::IsNull(input);
}

template <
    typename Traits,
    typename UserType,
    typename std::enable_if<!HasIsNullMethod<Traits>::value>::type* = nullptr>
bool CallIsNullIfExists(const UserType& input) {
  return false;
}
template <typename T>
struct HasSetToNullMethod {
  template <typename U>
  static char Test(decltype(U::SetToNull) *);
  template <typename U>
  static int Test(...);
  static const bool value = sizeof(Test<T>(0)) == sizeof(char);

 private:
  EnsureTypeIsComplete<T> check_t_;
};

template <
    typename Traits,
    typename UserType,
    typename std::enable_if<HasSetToNullMethod<Traits>::value>::type* = nullptr>
bool CallSetToNullIfExists(UserType* output) {
  Traits::SetToNull(output);
  return true;
}

template <typename Traits,
          typename UserType,
          typename std::enable_if<!HasSetToNullMethod<Traits>::value>::type* =
              nullptr>
bool CallSetToNullIfExists(UserType* output) {
  LOG(ERROR) << "A null value is received. But the Struct/Array/StringTraits "
             << "class doesn't define a SetToNull() function and therefore is "
             << "unable to deserialize the value.";
  return false;
}

template <typename T>
struct HasSetUpContextMethod {
  template <typename U>
  static char Test(decltype(U::SetUpContext) *);
  template <typename U>
  static int Test(...);
  static const bool value = sizeof(Test<T>(0)) == sizeof(char);

 private:
  EnsureTypeIsComplete<T> check_t_;
};

template <typename Traits,
          bool has_context = HasSetUpContextMethod<Traits>::value>
struct CustomContextHelper;

template <typename Traits>
struct CustomContextHelper<Traits, true> {
  template <typename MaybeConstUserType>
  static void* SetUp(MaybeConstUserType& input, SerializationContext* context) {
    return Traits::SetUpContext(input);
  }

  template <typename MaybeConstUserType>
  static void TearDown(MaybeConstUserType& input, void* custom_context) {
    Traits::TearDownContext(input, custom_context);
  }
};

template <typename Traits>
struct CustomContextHelper<Traits, false> {
  template <typename MaybeConstUserType>
  static void* SetUp(MaybeConstUserType& input, SerializationContext* context) {
    return nullptr;
  }

  template <typename MaybeConstUserType>
  static void TearDown(MaybeConstUserType& input, void* custom_context) {
    DCHECK(!custom_context);
  }
};

template <typename ReturnType, typename ParamType, typename InputUserType>
ReturnType CallWithContext(ReturnType (*f)(ParamType, void*),
                           InputUserType&& input,
                           void* context) {
  return f(std::forward<InputUserType>(input), context);
}

template <typename ReturnType, typename ParamType, typename InputUserType>
ReturnType CallWithContext(ReturnType (*f)(ParamType),
                           InputUserType&& input,
                           void* context) {
  return f(std::forward<InputUserType>(input));
}

template <typename T, typename MaybeConstUserType>
struct HasGetBeginMethod {
  template <typename U>
  static char Test(
      decltype(U::GetBegin(std::declval<MaybeConstUserType&>())) *);
  template <typename U>
  static int Test(...);
  static const bool value = sizeof(Test<T>(0)) == sizeof(char);

 private:
  EnsureTypeIsComplete<T> check_t_;
};

template <
    typename Traits,
    typename MaybeConstUserType,
    typename std::enable_if<
        HasGetBeginMethod<Traits, MaybeConstUserType>::value>::type* = nullptr>
decltype(Traits::GetBegin(std::declval<MaybeConstUserType&>()))
CallGetBeginIfExists(MaybeConstUserType& input) {
  return Traits::GetBegin(input);
}

template <
    typename Traits,
    typename MaybeConstUserType,
    typename std::enable_if<
        !HasGetBeginMethod<Traits, MaybeConstUserType>::value>::type* = nullptr>
size_t CallGetBeginIfExists(MaybeConstUserType& input) {
  return 0;
}

template <typename T, typename MaybeConstUserType>
struct HasGetDataMethod {
  template <typename U>
  static char Test(decltype(U::GetData(std::declval<MaybeConstUserType&>())) *);
  template <typename U>
  static int Test(...);
  static const bool value = sizeof(Test<T>(0)) == sizeof(char);

 private:
  EnsureTypeIsComplete<T> check_t_;
};

template <
    typename Traits,
    typename MaybeConstUserType,
    typename std::enable_if<
        HasGetDataMethod<Traits, MaybeConstUserType>::value>::type* = nullptr>
decltype(Traits::GetData(std::declval<MaybeConstUserType&>()))
CallGetDataIfExists(MaybeConstUserType& input) {
  return Traits::GetData(input);
}

template <
    typename Traits,
    typename MaybeConstUserType,
    typename std::enable_if<
        !HasGetDataMethod<Traits, MaybeConstUserType>::value>::type* = nullptr>
void* CallGetDataIfExists(MaybeConstUserType& input) {
  return nullptr;
}

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_SERIALIZATION_UTIL_H_
