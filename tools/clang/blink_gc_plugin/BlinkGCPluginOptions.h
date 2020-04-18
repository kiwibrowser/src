// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BLINK_GC_PLUGIN_BLINK_GC_PLUGIN_OPTIONS_H_
#define TOOLS_BLINK_GC_PLUGIN_BLINK_GC_PLUGIN_OPTIONS_H_

#include <set>
#include <string>
#include <vector>

struct BlinkGCPluginOptions {
  bool dump_graph = false;

  // If |true|, emit warning for class types which derive from from
  // GarbageCollectedFinalized<> when just GarbageCollected<> will do.
  bool warn_unneeded_finalizer = false;

  // Member<T> fields are only permitted in managed classes,
  // something CheckFieldsVisitor verifies, issuing errors if
  // found in unmanaged classes. WeakMember<T> should be treated
  // the exact same, but CheckFieldsVisitor was missing the case
  // for handling the weak member variant until crbug.com/724418.
  //
  // We've default-enabled the checking for those also now, but do
  // offer an opt-out option should enabling the check lead to
  // unexpected (but wanted, really) compilation errors while
  // rolling out an updated GC plugin version.
  //
  // TODO(sof): remove this option once safely rolled out.
  bool enable_weak_members_in_unmanaged_classes = false;

  // Warn on missing dispatches to base class TraceWrappers.
  bool warn_trace_wrappers_missing_base_dispatch = false;

  std::set<std::string> ignored_classes;
  std::set<std::string> checked_namespaces;
  std::vector<std::string> ignored_directories;
};

#endif  // TOOLS_BLINK_GC_PLUGIN_BLINK_GC_PLUGIN_OPTIONS_H_
