// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PORTS_NAME_H_
#define MOJO_EDK_SYSTEM_PORTS_NAME_H_

#include <stdint.h>

#include <ostream>
#include <tuple>

#include "base/component_export.h"
#include "base/hash.h"

namespace mojo {
namespace edk {
namespace ports {

struct COMPONENT_EXPORT(MOJO_EDK_PORTS) Name {
  Name(uint64_t v1, uint64_t v2) : v1(v1), v2(v2) {}
  uint64_t v1, v2;
};

inline bool operator==(const Name& a, const Name& b) {
  return a.v1 == b.v1 && a.v2 == b.v2;
}

inline bool operator!=(const Name& a, const Name& b) {
  return !(a == b);
}

inline bool operator<(const Name& a, const Name& b) {
  return std::tie(a.v1, a.v2) < std::tie(b.v1, b.v2);
}

COMPONENT_EXPORT(MOJO_EDK_PORTS)
std::ostream& operator<<(std::ostream& stream, const Name& name);

struct COMPONENT_EXPORT(MOJO_EDK_PORTS) PortName : Name {
  PortName() : Name(0, 0) {}
  PortName(uint64_t v1, uint64_t v2) : Name(v1, v2) {}
};

extern COMPONENT_EXPORT(MOJO_EDK_PORTS) const PortName kInvalidPortName;

struct COMPONENT_EXPORT(MOJO_EDK_PORTS) NodeName : Name {
  NodeName() : Name(0, 0) {}
  NodeName(uint64_t v1, uint64_t v2) : Name(v1, v2) {}
};

extern COMPONENT_EXPORT(MOJO_EDK_PORTS) const NodeName kInvalidNodeName;

}  // namespace ports
}  // namespace edk
}  // namespace mojo

namespace std {

template <>
struct COMPONENT_EXPORT(MOJO_EDK_PORTS) hash<mojo::edk::ports::PortName> {
  std::size_t operator()(const mojo::edk::ports::PortName& name) const {
    return base::HashInts64(name.v1, name.v2);
  }
};

template <>
struct COMPONENT_EXPORT(MOJO_EDK_PORTS) hash<mojo::edk::ports::NodeName> {
  std::size_t operator()(const mojo::edk::ports::NodeName& name) const {
    return base::HashInts64(name.v1, name.v2);
  }
};

}  // namespace std

#endif  // MOJO_EDK_SYSTEM_PORTS_NAME_H_
