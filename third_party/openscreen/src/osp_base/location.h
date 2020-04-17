// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_BASE_LOCATION_H_
#define OSP_BASE_LOCATION_H_

#include <stddef.h>

#include <cassert>
#include <functional>
#include <string>

namespace openscreen {

// NOTE: lifted from Chromium's base Location implementation, forked to work
// with our base library.

// Instances of the location class include basic information about a position
// in program source, for example the place where an object was constructed.
class Location {
 public:
  Location();
  Location(const Location&);
  Location(Location&&);

  // Initializes the program counter
  explicit Location(const void* program_counter);

  Location& operator=(const Location& other);
  Location& operator=(Location&& other);

  // Comparator for hash map insertion. The program counter should uniquely
  // identify a location.
  bool operator==(const Location& other) const {
    return program_counter_ == other.program_counter_;
  }

  // The address of the code generating this Location object. Should always be
  // valid except for default initialized Location objects, which will be
  // nullptr.
  const void* program_counter() const { return program_counter_; }

  // Converts to the most user-readable form possible. This will return
  // "pc:<hex address>".
  std::string ToString() const;

  static Location CreateFromHere();

 private:
  const void* program_counter_ = nullptr;
};

const void* GetProgramCounter();

#define CURRENT_LOCATION ::openscreen::Location::CreateFromHere()

}  // namespace openscreen

#endif  // OSP_BASE_LOCATION_H_
