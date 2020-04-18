// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_EMBEDDER_GRAPH_BUILDER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_EMBEDDER_GRAPH_BUILDER_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable_visitor.h"
#include "v8/include/v8-profiler.h"
#include "v8/include/v8.h"

namespace blink {

class V8EmbedderGraphBuilder : public ScriptWrappableVisitor,
                               public v8::PersistentHandleVisitor {
 public:
  using Traceable = const void*;
  using Graph = v8::EmbedderGraph;

  V8EmbedderGraphBuilder(v8::Isolate*, Graph*);

  static void BuildEmbedderGraphCallback(v8::Isolate*, v8::EmbedderGraph*);
  void BuildEmbedderGraph();

  // v8::PersistentHandleVisitor override.
  void VisitPersistentHandle(v8::Persistent<v8::Value>*,
                             uint16_t class_id) override;

  // Visitor overrides.
  void Visit(const TraceWrapperV8Reference<v8::Value>&) final;
  void Visit(void*, TraceWrapperDescriptor) final;
  void Visit(DOMWrapperMap<ScriptWrappable>*, const ScriptWrappable*) final;

 protected:
  using Visitor::Visit;

 private:
  // Information about whether a node is attached to the main DOM tree
  // or not. It is computed as follows:
  // 1) A Document with IsContextDestroyed() = true is detached.
  // 2) A Document with IsContextDestroyed() = false is attached.
  // 3) A Node that is not connected to any Document is detached.
  // 4) A Node that is connected to a detached Document is detached.
  // 5) A Node that is connected to an attached Document is attached.
  // 6) A ScriptWrappable that is reachable from an attached Node is
  //    attached.
  // 7) A ScriptWrappable that is reachable from a detached Node is
  //    detached.
  // 8) A ScriptWrappable that is not reachable from any Node is
  //    considered (conservatively) as attached.
  // The unknown state applies to ScriptWrappables during graph
  // traversal when we don't have reachability information yet.
  enum class DomTreeState { kAttached, kDetached, kUnknown };

  DomTreeState DomTreeStateFromWrapper(uint16_t class_id,
                                       v8::Local<v8::Object> v8_value);

  class EmbedderNode : public Graph::Node {
   public:
    EmbedderNode(const char* name,
                 Graph::Node* wrapper,
                 DomTreeState dom_tree_state)
        : name_(name), wrapper_(wrapper), dom_tree_state_(dom_tree_state) {}

    DomTreeState GetDomTreeState() { return dom_tree_state_; }
    void UpdateDomTreeState(DomTreeState parent_dom_tree_state) {
      // If the child's state is unknown, then take the parent's state.
      // If the parent is attached, then the child is also attached.
      if (dom_tree_state_ == DomTreeState::kUnknown ||
          parent_dom_tree_state == DomTreeState::kAttached) {
        dom_tree_state_ = parent_dom_tree_state;
      }
    }
    // Graph::Node overrides.
    const char* Name() override { return name_; }
    const char* NamePrefix() override {
      return dom_tree_state_ == DomTreeState::kDetached ? "Detached" : nullptr;
    }
    size_t SizeInBytes() override { return 0; }
    Graph::Node* WrapperNode() override { return wrapper_; }

   private:
    const char* name_;
    Graph::Node* wrapper_;
    DomTreeState dom_tree_state_;
  };

  class EmbedderRootNode : public EmbedderNode {
   public:
    explicit EmbedderRootNode(const char* name)
        : EmbedderNode(name, nullptr, DomTreeState::kUnknown) {}
    // Graph::Node override.
    bool IsRootNode() override { return true; }
  };

  class ParentScope {
    STACK_ALLOCATED();

   public:
    ParentScope(V8EmbedderGraphBuilder* visitor, EmbedderNode* parent)
        : visitor_(visitor) {
      DCHECK_EQ(visitor->current_parent_, nullptr);
      visitor->current_parent_ = parent;
    }
    ~ParentScope() { visitor_->current_parent_ = nullptr; }

   private:
    V8EmbedderGraphBuilder* visitor_;
  };

  struct WorklistItem {
    EmbedderNode* node;
    Traceable traceable;
    TraceWrappersCallback trace_wrappers_callback;
  };

  WorklistItem ToWorklistItem(EmbedderNode*,
                              const TraceWrapperDescriptor&) const;

  Graph::Node* GraphNode(const v8::Local<v8::Value>&) const;
  EmbedderNode* GraphNode(Traceable,
                          const char* name,
                          Graph::Node* wrapper,
                          DomTreeState) const;

  void VisitPendingActivities();
  void VisitTransitiveClosure();

  // Push the item to the default worklist if item.traceable was not
  // already visited.
  void PushToWorklist(WorklistItem) const;

  v8::Isolate* isolate_;
  EmbedderNode* current_parent_;
  mutable Graph* graph_;
  mutable HashSet<Traceable> visited_;
  mutable HashMap<Traceable, EmbedderNode*> graph_node_;
  // The default worklist that is used to visit transitive closure.
  mutable Deque<WorklistItem> worklist_;
  // The worklist that collects detached Nodes during persistent handle
  // iteration.
  mutable Deque<WorklistItem> detached_worklist_;
  // The worklist that collects ScriptWrappables with unknown information
  // about attached/detached state during persistent handle iteration.
  mutable Deque<WorklistItem> unknown_worklist_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_EMBEDDER_GRAPH_BUILDER_H_
