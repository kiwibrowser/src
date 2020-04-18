// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/v8_embedder_graph_builder.h"

#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_node.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_map.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable_visitor.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"

namespace blink {

V8EmbedderGraphBuilder::V8EmbedderGraphBuilder(v8::Isolate* isolate,
                                               Graph* graph)
    : isolate_(isolate), current_parent_(nullptr), graph_(graph) {}

void V8EmbedderGraphBuilder::BuildEmbedderGraphCallback(
    v8::Isolate* isolate,
    v8::EmbedderGraph* graph) {
  V8EmbedderGraphBuilder builder(isolate, graph);
  builder.BuildEmbedderGraph();
}

void V8EmbedderGraphBuilder::BuildEmbedderGraph() {
  isolate_->VisitHandlesWithClassIds(this);
// At this point we collected ScriptWrappables in three groups:
// attached, detached, and unknown.
#if DCHECK_IS_ON()
  for (const WorklistItem& item : worklist_) {
    DCHECK_EQ(DomTreeState::kAttached, item.node->GetDomTreeState());
  }
  for (const WorklistItem& item : detached_worklist_) {
    DCHECK_EQ(DomTreeState::kDetached, item.node->GetDomTreeState());
  }
  for (const WorklistItem& item : unknown_worklist_) {
    DCHECK_EQ(DomTreeState::kUnknown, item.node->GetDomTreeState());
  }
#endif
  // We need to propagate attached/detached information to ScriptWrappables
  // with the unknown state. The information propagates from a parent to
  // a child as follows:
  // - if the parent is attached, then the child is considered attached.
  // - if the parent is detached and the child is unknown, then the child is
  //   considered detached.
  // - if the parent is unknown, then the state of the child does not change.
  //
  // We need to organize DOM traversal in three stages to ensure correct
  // propagation:
  // 1) Traverse from the attached nodes. All nodes discovered in this stage
  //    will be marked as kAttached.
  // 2) Traverse from the detached nodes. All nodes discovered in this stage
  //    will be marked as kDetached if they are not already marked as kAttached.
  // 3) Traverse from the unknown nodes. This is needed only for edge recording.
  // Stage 1: find transitive closure of the attached nodes.
  VisitTransitiveClosure();
  // Stage 2: find transitive closure of the detached nodes.
  while (!detached_worklist_.empty()) {
    auto item = detached_worklist_.back();
    detached_worklist_.pop_back();
    PushToWorklist(item);
  }
  VisitTransitiveClosure();
  // Stage 3: find transitive closure of the unknown nodes.
  // Nodes reachable only via pending activities are treated as unknown.
  VisitPendingActivities();
  while (!unknown_worklist_.empty()) {
    auto item = unknown_worklist_.back();
    unknown_worklist_.pop_back();
    PushToWorklist(item);
  }
  VisitTransitiveClosure();
  DCHECK(worklist_.empty());
  DCHECK(detached_worklist_.empty());
  DCHECK(unknown_worklist_.empty());
}

V8EmbedderGraphBuilder::DomTreeState
V8EmbedderGraphBuilder::DomTreeStateFromWrapper(
    uint16_t class_id,
    v8::Local<v8::Object> v8_value) {
  if (class_id != WrapperTypeInfo::kNodeClassId)
    return DomTreeState::kUnknown;
  Node* node = V8Node::ToImpl(v8_value);
  Node* root = V8GCController::OpaqueRootForGC(isolate_, node);
  if (root->isConnected() &&
      !node->GetDocument().MasterDocument().IsContextDestroyed()) {
    return DomTreeState::kAttached;
  }
  return DomTreeState::kDetached;
}

void V8EmbedderGraphBuilder::VisitPersistentHandle(
    v8::Persistent<v8::Value>* value,
    uint16_t class_id) {
  if (class_id != WrapperTypeInfo::kNodeClassId &&
      class_id != WrapperTypeInfo::kObjectClassId)
    return;
  v8::Local<v8::Object> v8_value = v8::Local<v8::Object>::New(
      isolate_, v8::Persistent<v8::Object>::Cast(*value));
  ScriptWrappable* traceable = ToScriptWrappable(v8_value);
  if (!traceable)
    return;
  Graph::Node* wrapper = GraphNode(v8_value);
  DomTreeState dom_tree_state = DomTreeStateFromWrapper(class_id, v8_value);
  EmbedderNode* graph_node = GraphNode(
      traceable, traceable->NameInHeapSnapshot(), wrapper, dom_tree_state);
  const TraceWrapperDescriptor& wrapper_descriptor =
      TraceWrapperDescriptorFor<ScriptWrappable>(traceable);
  WorklistItem item = ToWorklistItem(graph_node, wrapper_descriptor);
  switch (graph_node->GetDomTreeState()) {
    case DomTreeState::kAttached:
      PushToWorklist(item);
      break;
    case DomTreeState::kDetached:
      detached_worklist_.push_back(item);
      break;
    case DomTreeState::kUnknown:
      unknown_worklist_.push_back(item);
      break;
  }
}

void V8EmbedderGraphBuilder::Visit(
    const TraceWrapperV8Reference<v8::Value>& traced_wrapper) {
  const v8::PersistentBase<v8::Value>* value = &traced_wrapper.Get();
  // Add an edge from the current parent to the V8 object.
  v8::Local<v8::Value> v8_value = v8::Local<v8::Value>::New(isolate_, *value);
  if (!v8_value.IsEmpty()) {
    graph_->AddEdge(current_parent_, GraphNode(v8_value));
  }
}

void V8EmbedderGraphBuilder::Visit(void* object,
                                   TraceWrapperDescriptor wrapper_descriptor) {
  // Add an edge from the current parent to this object.
  // Also push the object to the worklist in order to process its members.
  const void* traceable = wrapper_descriptor.base_object_payload;
  const char* name =
      GCInfoTable::Get()
          .GCInfoFromIndex(
              HeapObjectHeader::FromPayload(traceable)->GcInfoIndex())
          ->name_(traceable);
  EmbedderNode* graph_node =
      GraphNode(traceable, name, nullptr, current_parent_->GetDomTreeState());
  graph_->AddEdge(current_parent_, graph_node);
  PushToWorklist(ToWorklistItem(graph_node, wrapper_descriptor));
}

void V8EmbedderGraphBuilder::Visit(DOMWrapperMap<ScriptWrappable>* wrapper_map,
                                   const ScriptWrappable* key) {
  // Add an edge from the current parent to the V8 object.
  v8::Local<v8::Value> v8_value =
      wrapper_map->NewLocal(isolate_, const_cast<ScriptWrappable*>(key));
  if (!v8_value.IsEmpty())
    graph_->AddEdge(current_parent_, GraphNode(v8_value));
}

v8::EmbedderGraph::Node* V8EmbedderGraphBuilder::GraphNode(
    const v8::Local<v8::Value>& value) const {
  return graph_->V8Node(value);
}

V8EmbedderGraphBuilder::EmbedderNode* V8EmbedderGraphBuilder::GraphNode(
    Traceable traceable,
    const char* name,
    v8::EmbedderGraph::Node* wrapper,
    DomTreeState dom_tree_state) const {
  auto iter = graph_node_.find(traceable);
  if (iter != graph_node_.end()) {
    iter->value->UpdateDomTreeState(dom_tree_state);
    return iter->value;
  }
  // Ownership of the new node is transferred to the graph_.
  // graph_node_.at(tracable) is valid for all BuildEmbedderGraph execution.
  auto* raw_node = new EmbedderNode(name, wrapper, dom_tree_state);
  EmbedderNode* node = static_cast<EmbedderNode*>(
      graph_->AddNode(std::unique_ptr<Graph::Node>(raw_node)));
  graph_node_.insert(traceable, node);
  return node;
}

void V8EmbedderGraphBuilder::VisitPendingActivities() {
  // Ownership of the new node is transferred to the graph_.
  EmbedderNode* root =
      static_cast<EmbedderNode*>(graph_->AddNode(std::unique_ptr<Graph::Node>(
          new EmbedderRootNode("Pending activities"))));
  ParentScope parent(this, root);
  ActiveScriptWrappableBase::TraceActiveScriptWrappables(isolate_, this);
}

V8EmbedderGraphBuilder::WorklistItem V8EmbedderGraphBuilder::ToWorklistItem(
    EmbedderNode* node,
    const TraceWrapperDescriptor& wrapper_descriptor) const {
  return {node, wrapper_descriptor.base_object_payload,
          wrapper_descriptor.trace_wrappers_callback};
}

void V8EmbedderGraphBuilder::PushToWorklist(WorklistItem item) const {
  if (!visited_.Contains(item.traceable)) {
    visited_.insert(item.traceable);
    worklist_.push_back(item);
  }
}

void V8EmbedderGraphBuilder::VisitTransitiveClosure() {
  // Depth-first search.
  while (!worklist_.empty()) {
    auto item = worklist_.back();
    worklist_.pop_back();
    ParentScope parent(this, item.node);
    item.trace_wrappers_callback(this, const_cast<void*>(item.traceable));
  }
}

}  // namespace blink
