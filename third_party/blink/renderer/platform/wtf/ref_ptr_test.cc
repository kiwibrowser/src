// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_refptr.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/text/string_impl.h"
#include "third_party/blink/renderer/platform/wtf/thread_safe_ref_counted.h"

namespace WTF {
namespace {

TEST(RefPtrTest, Basic) {
  scoped_refptr<StringImpl> string;
  EXPECT_TRUE(!string);
  string = StringImpl::Create("test");
  EXPECT_TRUE(!!string);
  string = nullptr;
  EXPECT_TRUE(!string);
}

TEST(RefPtrTest, MoveAssignmentOperator) {
  scoped_refptr<StringImpl> a = StringImpl::Create("a");
  scoped_refptr<StringImpl> b = StringImpl::Create("b");
  b = std::move(a);
  EXPECT_TRUE(!!b);
  EXPECT_TRUE(!a);
}

class RefCountedClass : public RefCounted<RefCountedClass> {};

TEST(RefPtrTest, ConstObject) {
  // This test is only to ensure we force the compilation of a const RefCounted
  // object to ensure the generated code compiles.
  scoped_refptr<const RefCountedClass> ptr_to_const =
      base::AdoptRef(new RefCountedClass());
}

class CustomDeleter;

struct Deleter {
  static void Destruct(const CustomDeleter*);
};

class CustomDeleter : public RefCounted<CustomDeleter, Deleter> {
 public:
  explicit CustomDeleter(bool* deleted) : deleted_(deleted) {}

 private:
  friend struct Deleter;
  ~CustomDeleter() = default;

  bool* deleted_;
};

void Deleter::Destruct(const CustomDeleter* obj) {
  EXPECT_FALSE(*obj->deleted_);
  *obj->deleted_ = true;
  delete obj;
}

TEST(RefPtrTest, CustomDeleter) {
  bool deleted = false;
  scoped_refptr<CustomDeleter> obj =
      base::AdoptRef(new CustomDeleter(&deleted));
  EXPECT_FALSE(deleted);
  obj = nullptr;
  EXPECT_TRUE(deleted);
}

class CustomDeleterThreadSafe;

struct DeleterThreadSafe {
  static void Destruct(const CustomDeleterThreadSafe*);
};

class CustomDeleterThreadSafe
    : public ThreadSafeRefCounted<CustomDeleterThreadSafe, DeleterThreadSafe> {
 public:
  explicit CustomDeleterThreadSafe(bool* deleted) : deleted_(deleted) {}

 private:
  friend struct DeleterThreadSafe;
  ~CustomDeleterThreadSafe() = default;

  bool* deleted_;
};

void DeleterThreadSafe::Destruct(const CustomDeleterThreadSafe* obj) {
  EXPECT_FALSE(*obj->deleted_);
  *obj->deleted_ = true;
  delete obj;
}

TEST(RefPtrTest, CustomDeleterThreadSafe) {
  bool deleted = false;
  scoped_refptr<CustomDeleterThreadSafe> obj =
      base::AdoptRef(new CustomDeleterThreadSafe(&deleted));
  EXPECT_FALSE(deleted);
  obj = nullptr;
  EXPECT_TRUE(deleted);
}

}  // namespace
}  // namespace WTF
