// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_util.h"

#include <stdio.h>

namespace crazy {

// Return the base name from a file path. Important: this is a pointer
// into the original string.
// static
const char* GetBaseNamePtr(const char* path) {
  const char* p = strrchr(path, '/');
  if (!p)
    return path;
  else
    return p + 1;
}

// static
const char String::kEmpty[] = "";

String::String() { Init(); }

String::String(const String& other) {
  InitFrom(other.ptr_, other.size_);
}

String::String(String&& other)
    : ptr_(other.ptr_), size_(other.size_), capacity_(other.capacity_) {
  other.Init();
}

String::String(const char* str) {
  InitFrom(str, ::strlen(str));
}

String::String(char ch) {
  InitFrom(&ch, 1);
}

String::~String() {
  if (HasValidPointer()) {
    free(ptr_);
    ptr_ = const_cast<char*>(kEmpty);
  }
}

String::String(const char* str, size_t len) {
  InitFrom(str, len);
}

String& String::operator=(String&& other) {
  if (this != &other) {
    this->~String();  // Free pointer if needed.
    ptr_ = other.ptr_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.Init();
  }
  return *this;
}

void String::Assign(const char* str, size_t len) {
  Resize(len);
  if (len > 0) {
    memcpy(ptr_, str, len);
  }
}

void String::Append(const char* str, size_t len) {
  if (len > 0) {
    size_t old_size = size_;
    Resize(size_ + len);
    memcpy(ptr_ + old_size, str, len);
  }
}

void String::Resize(size_t new_size) {
  if (new_size > capacity_) {
    size_t new_capacity = capacity_;
    while (new_capacity < new_size) {
      new_capacity += (new_capacity >> 1) + 16;
    }
    Reserve(new_capacity);
  }

  if (new_size > size_)
    memset(ptr_ + size_, '\0', new_size - size_);

  size_ = new_size;
  if (HasValidPointer()) {
    ptr_[size_] = '\0';
  }
}

void String::Reserve(size_t new_capacity) {
  char* old_ptr = HasValidPointer() ? ptr_ : nullptr;
  // Always allocate one more byte for the trailing \0
  ptr_ = reinterpret_cast<char*>(realloc(old_ptr, new_capacity + 1));
  ptr_[new_capacity] = '\0';
  capacity_ = new_capacity;

  if (size_ > new_capacity)
    size_ = new_capacity;
}

void String::InitFrom(const char* str, size_t len) {
  Init();
  if (str != nullptr && len != 0) {
    Resize(len);
    memcpy(ptr_, str, len);
  }
}

}  // namespace crazy
