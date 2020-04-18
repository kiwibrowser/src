// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EmptyOffsetMappingBuilder_h
#define EmptyOffsetMappingBuilder_h

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutObject;

// A mock class providing all APIs of an offset mapping builder, but not doing
// anything. For templates functions/classes that can optionally create an
// offset mapping, this mock class is passed to create an instantiation that
// does not create any offset mapping.
class EmptyOffsetMappingBuilder {
  STACK_ALLOCATED();

 public:
  class SourceNodeScope {
   public:
    SourceNodeScope(EmptyOffsetMappingBuilder*, const void*) {}
    ~SourceNodeScope() = default;
  };

  EmptyOffsetMappingBuilder() = default;
  void AppendIdentityMapping(unsigned) {}
  void AppendCollapsedMapping(unsigned) {}
  void CollapseTrailingSpace(unsigned) {}
  void Composite(const EmptyOffsetMappingBuilder&) {}
  void Concatenate(const EmptyOffsetMappingBuilder&) {}
  void EnterInline(const LayoutObject&) {}
  void ExitInline(const LayoutObject&) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(EmptyOffsetMappingBuilder);
};

}  // namespace blink

#endif  // EmptyOffsetMappingBuilder_h
