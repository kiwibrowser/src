// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_LIST_H__
#define GESTURES_LIST_H__

#include "gestures/include/logging.h"
#include "gestures/include/memory_manager.h"

namespace gestures {

// Elt must have the following members:
// Elt* next_;
// Elt* prev_;

template<typename Elt>
class List {
 public:
  List() { Init(); }
  ~List() { DeleteAll(); }

  // inserts new_elt before existing. Assumes existing is in list already.
  void InsertBefore(Elt *existing, Elt* new_elt) {
    size_++;
    Elt* pre_new = existing->prev_;
    pre_new->next_ = new_elt;
    new_elt->prev_ = pre_new;
    new_elt->next_ = existing;
    existing->prev_ = new_elt;
  }

  Elt* Unlink(Elt* existing) {
    if (Empty()) {
      Err("Can't pop from empty list!");
      return NULL;
    }
    size_--;
    existing->prev_->next_ = existing->next_;
    existing->next_->prev_ = existing->prev_;
    existing->prev_ = existing->next_ = NULL;
    return existing;
  }

  void PushFront(Elt* elt) { InsertBefore(sentinel_.next_, elt); }
  Elt* PopFront() { return Unlink(sentinel_.next_); }
  void PushBack(Elt* elt) { InsertBefore(&sentinel_, elt); }
  Elt* PopBack() { return Unlink(sentinel_.prev_); }

  virtual void DeleteAll() {
    while (!Empty())
      PopFront();
  }

  Elt* Head() const { return sentinel_.next_; }
  Elt* Tail() const { return sentinel_.prev_; }
  size_t size() const { return size_; }
  bool Empty() const { return size() == 0; }

  // Iterator-like methods
  Elt* Begin() const { return Head(); }
  Elt* End() const { return const_cast<Elt*>(&sentinel_); }

  void Init() {
    size_ = 0;
    sentinel_.next_ = sentinel_.prev_ = &sentinel_;
  }

 private:
  // sentinel element
  Elt sentinel_;

  size_t size_;
};


template<typename Elt>
class MemoryManagedList : public List<Elt> {
 public:
  using List<Elt>::Empty;
  using List<Elt>::PopBack;
  using List<Elt>::PopFront;
  using List<Elt>::PushBack;
  using List<Elt>::PushFront;

  MemoryManagedList() { };
  ~MemoryManagedList() { DeleteAll(); }

  void Init(MemoryManager<Elt>* memory_manager) {
    memory_manager_ = memory_manager;
    List<Elt>::Init();
  }

  Elt* NewElt() {
    Elt* elt = memory_manager_->Allocate();
    AssertWithReturnValue(elt, NULL);
    elt->next_ = elt->prev_ = NULL;
    return elt;
  }

  Elt* PushNewEltBack() {
    AssertWithReturnValue(memory_manager_, NULL);
    Elt* elt = NewElt();
    AssertWithReturnValue(elt, NULL);
    PushBack(elt);
    return elt;
  }

  Elt* PushNewEltFront() {
    AssertWithReturnValue(memory_manager_, NULL);
    Elt* elt = NewElt();
    AssertWithReturnValue(elt, NULL);
    PushFront(elt);
    return elt;
  }

  void DeleteFront() {
    AssertWithReturn(memory_manager_);
    memory_manager_->Free(PopFront());
  }

  void DeleteBack() {
    AssertWithReturn(memory_manager_);
    memory_manager_->Free(PopBack());
  }

  virtual void DeleteAll() {
    while (!Empty())
      DeleteFront();
  }
 private:
  MemoryManager<Elt>* memory_manager_;
};


}  // namespace gestures

#endif  // GESTURES_LIST_H__
