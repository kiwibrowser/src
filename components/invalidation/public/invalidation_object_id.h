// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_PUBLIC_INVALIDATION_OBJECT_ID_H_
#define COMPONENTS_INVALIDATION_PUBLIC_INVALIDATION_OBJECT_ID_H_

#include <string>

namespace invalidation {

/* A class to represent a unique object id that an application can register or
 * unregister for.
 */
class InvalidationObjectId {
 public:
  InvalidationObjectId() : is_initialized_(false) {}

  /* Creates an object id for the given source and name (the name is copied). */
  InvalidationObjectId(int source, const std::string& name)
      : is_initialized_(true), source_(source), name_(name) {}

  void Init(int source, const std::string& name) {
    is_initialized_ = true;
    source_ = source;
    name_ = name;
  }

  int source() const {
    DCHECK(is_initialized_);
    return source_;
  }

  const std::string& name() const {
    DCHECK(is_initialized_);
    return name_;
  }

  bool operator==(const InvalidationObjectId& object_id) const {
    DCHECK(is_initialized_);
    DCHECK(object_id.is_initialized_);
    return (source() == object_id.source()) && (name() == object_id.name());
  }

 private:
  /* Whether the object id has been initialized. */
  bool is_initialized_;

  /* The invalidation source type. */
  int source_;

  /* The name/unique id for the object. */
  std::string name_;
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_PUBLIC_INVALIDATION_OBJECT_ID_H_
