// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/graph_processor.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/shared_memory_tracker.h"
#include "base/strings/string_split.h"

namespace memory_instrumentation {

using base::CompareCase;
using base::ProcessId;
using base::trace_event::MemoryAllocatorDump;
using base::trace_event::MemoryAllocatorDumpGuid;
using base::trace_event::ProcessMemoryDump;
using Edge = memory_instrumentation::GlobalDumpGraph::Edge;
using Node = memory_instrumentation::GlobalDumpGraph::Node;
using Process = memory_instrumentation::GlobalDumpGraph::Process;

namespace {

const char kSizeEntryName[] = "size";
const char kEffectiveSizeEntryName[] = "effective_size";

Node::Entry::ScalarUnits EntryUnitsFromString(std::string units) {
  if (units == MemoryAllocatorDump::kUnitsBytes) {
    return Node::Entry::ScalarUnits::kBytes;
  } else if (units == MemoryAllocatorDump::kUnitsObjects) {
    return Node::Entry::ScalarUnits::kObjects;
  } else {
    // Invalid units so we just return a value of the correct type.
    return Node::Entry::ScalarUnits::kObjects;
  }
}

base::Optional<uint64_t> GetSizeEntryOfNode(Node* node) {
  auto size_it = node->entries()->find(kSizeEntryName);
  if (size_it == node->entries()->end())
    return base::nullopt;

  DCHECK(size_it->second.type == Node::Entry::Type::kUInt64);
  DCHECK(size_it->second.units == Node::Entry::ScalarUnits::kBytes);
  return base::Optional<uint64_t>(size_it->second.value_uint64);
}

}  // namespace

// static
std::unique_ptr<GlobalDumpGraph> GraphProcessor::CreateMemoryGraph(
    const GraphProcessor::MemoryDumpMap& process_dumps) {
  auto global_graph = std::make_unique<GlobalDumpGraph>();

  // First pass: collects allocator dumps into a graph and populate
  // with entries.
  for (const auto& pid_to_dump : process_dumps) {
    // There can be null entries in the map; simply filter these out.
    if (!pid_to_dump.second)
      continue;

    auto* graph = global_graph->CreateGraphForProcess(pid_to_dump.first);
    CollectAllocatorDumps(*pid_to_dump.second, global_graph.get(), graph);
  }

  // Second pass: generate the graph of edges between the nodes.
  for (const auto& pid_to_dump : process_dumps) {
    // There can be null entries in the map; simply filter these out.
    if (!pid_to_dump.second)
      continue;

    AddEdges(*pid_to_dump.second, global_graph.get());
  }

  return global_graph;
}

// static
void GraphProcessor::RemoveWeakNodesFromGraph(GlobalDumpGraph* global_graph) {
  auto* global_root = global_graph->shared_memory_graph()->root();

  // Third pass: mark recursively nodes as weak if they don't have an associated
  // dump and all their children are weak.
  MarkImplicitWeakParentsRecursively(global_root);
  for (const auto& pid_to_process : global_graph->process_dump_graphs()) {
    MarkImplicitWeakParentsRecursively(pid_to_process.second->root());
  }

  // Fourth pass: recursively mark nodes as weak if they own a node which is
  // weak or if they have a parent who is weak.
  {
    std::set<const Node*> visited;
    MarkWeakOwnersAndChildrenRecursively(global_root, &visited);
    for (const auto& pid_to_process : global_graph->process_dump_graphs()) {
      MarkWeakOwnersAndChildrenRecursively(pid_to_process.second->root(),
                                           &visited);
    }
  }

  // Fifth pass: remove all nodes which are weak (including their descendants)
  // and clean owned by edges to match.
  RemoveWeakNodesRecursively(global_root);
  for (const auto& pid_to_process : global_graph->process_dump_graphs()) {
    RemoveWeakNodesRecursively(pid_to_process.second->root());
  }
}

// static
void GraphProcessor::AddOverheadsAndPropogateEntries(
    GlobalDumpGraph* global_graph) {
  // Sixth pass: account for tracing overhead in system memory allocators.
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    Process* process = pid_to_process.second.get();
    if (process->FindNode("winheap")) {
      AssignTracingOverhead("winheap", global_graph,
                            pid_to_process.second.get());
    } else if (process->FindNode("malloc")) {
      AssignTracingOverhead("malloc", global_graph,
                            pid_to_process.second.get());
    }
  }

  // Seventh pass: aggregate non-size integer entries into parents and propogate
  // string and int entries for shared graph.
  auto* global_root = global_graph->shared_memory_graph()->root();
  AggregateNumericsRecursively(global_root);
  PropagateNumericsAndDiagnosticsRecursively(global_root);
  for (auto& pid_to_process : global_graph->process_dump_graphs()) {
    AggregateNumericsRecursively(pid_to_process.second->root());
  }
}

// static
void GraphProcessor::CalculateSizesForGraph(GlobalDumpGraph* global_graph) {
  // Eighth pass: calculate the size field for nodes by considering the sizes
  // of their children and owners.
  {
    auto it = global_graph->VisitInDepthFirstPostOrder();
    while (Node* node = it.next()) {
      CalculateSizeForNode(node);
    }
  }

  // Ninth pass: Calculate not-owned and not-owning sub-sizes of all nodes.
  {
    auto it = global_graph->VisitInDepthFirstPostOrder();
    while (Node* node = it.next()) {
      CalculateDumpSubSizes(node);
    }
  }

  // Tenth pass: Calculate owned and owning coefficients of owned and owner
  // nodes.
  {
    auto it = global_graph->VisitInDepthFirstPostOrder();
    while (Node* node = it.next()) {
      CalculateDumpOwnershipCoefficient(node);
    }
  }

  // Eleventh pass: Calculate cumulative owned and owning coefficients of all
  // nodes.
  {
    auto it = global_graph->VisitInDepthFirstPreOrder();
    while (Node* node = it.next()) {
      CalculateDumpCumulativeOwnershipCoefficient(node);
    }
  }

  // Twelfth pass: Calculate the effective sizes of all nodes.
  {
    auto it = global_graph->VisitInDepthFirstPostOrder();
    while (Node* node = it.next()) {
      CalculateDumpEffectiveSize(node);
    }
  }
}

// static
std::map<base::ProcessId, uint64_t>
GraphProcessor::ComputeSharedFootprintFromGraph(
    const GlobalDumpGraph& global_graph) {
  // Go through all nodes associated with global dumps and find if they are
  // owned by shared memory nodes.
  Node* global_root =
      global_graph.shared_memory_graph()->root()->GetChild("global");

  // If there are no global dumps then just return an empty map with no data.
  if (!global_root) {
    return std::map<base::ProcessId, uint64_t>();
  }

  struct GlobalNodeOwners {
    std::list<Edge*> edges;
    int max_priority = 0;
  };

  std::map<Node*, GlobalNodeOwners> global_node_to_shared_owners;
  for (const auto& path_to_child : *global_root->children()) {
    // The path of this node is something like "global/foo".
    Node* global_node = path_to_child.second;

    // If there's no size to attribute, there's no point in propogating
    // anything.
    if (global_node->entries()->count("size") == 0)
      continue;

    for (auto* edge : *global_node->owned_by_edges()) {
      // Find if the source node's path starts with "shared_memory/" which
      // indcates shared memory.
      Node* source_root = edge->source()->dump_graph()->root();
      const Node* current = edge->source();
      DCHECK_NE(current, source_root);

      // Traverse up until we hit the point where |current| holds a node which
      // is the child of |source_root|.
      while (current->parent() != source_root)
        current = current->parent();

      // If the source is indeed a shared memory node, add the edge to the map.
      if (source_root->GetChild(base::SharedMemoryTracker::kDumpRootName) ==
          current) {
        GlobalNodeOwners* owners = &global_node_to_shared_owners[global_node];
        owners->edges.push_back(edge);
        owners->max_priority = std::max(owners->max_priority, edge->priority());
      }
    }
  }

  // Go through the map and leave only the edges which have the maximum
  // priority.
  for (auto& global_to_shared_edges : global_node_to_shared_owners) {
    int max_priority = global_to_shared_edges.second.max_priority;
    global_to_shared_edges.second.edges.remove_if(
        [max_priority](Edge* edge) { return edge->priority() < max_priority; });
  }

  // Compute the footprints by distributing the memory of the nodes
  // among the processes which have edges left.
  std::map<base::ProcessId, uint64_t> pid_to_shared_footprint;
  for (const auto& global_to_shared_edges : global_node_to_shared_owners) {
    Node* node = global_to_shared_edges.first;
    const auto& edges = global_to_shared_edges.second.edges;

    const Node::Entry& size_entry =
        node->entries()->find(kSizeEntryName)->second;
    DCHECK_EQ(size_entry.type, Node::Entry::kUInt64);

    uint64_t size_per_process = size_entry.value_uint64 / edges.size();
    for (auto* edge : edges) {
      base::ProcessId pid = edge->source()->dump_graph()->pid();
      pid_to_shared_footprint[pid] += size_per_process;
    }
  }

  return pid_to_shared_footprint;
}

// static
void GraphProcessor::CollectAllocatorDumps(
    const base::trace_event::ProcessMemoryDump& source,
    GlobalDumpGraph* global_graph,
    Process* process_graph) {
  // Turn each dump into a node in the graph of dumps in the appropriate
  // process dump or global dump.
  for (const auto& path_to_dump : source.allocator_dumps()) {
    const std::string& path = path_to_dump.first;
    const MemoryAllocatorDump& dump = *path_to_dump.second;

    // All global dumps (i.e. those starting with global/) should be redirected
    // to the shared graph.
    bool is_global = base::StartsWith(path, "global/", CompareCase::SENSITIVE);
    Process* process =
        is_global ? global_graph->shared_memory_graph() : process_graph;

    Node* node;
    auto node_iterator = global_graph->nodes_by_guid().find(dump.guid());
    if (node_iterator == global_graph->nodes_by_guid().end()) {
      // Storing whether the process is weak here will allow for later
      // computations on whether or not the node should be removed.
      bool is_weak = dump.flags() & MemoryAllocatorDump::Flags::WEAK;
      node = process->CreateNode(dump.guid(), path, is_weak);
    } else {
      node = node_iterator->second;

      DCHECK_EQ(node, process->FindNode(path))
          << "Nodes have different paths but same GUIDs";
      DCHECK(is_global) << "Multiple nodes have same GUID without being global";
    }

    // Copy any entries not already present into the node.
    for (auto& entry : dump.entries()) {
      switch (entry.entry_type) {
        case MemoryAllocatorDump::Entry::EntryType::kUint64:
          node->AddEntry(entry.name, EntryUnitsFromString(entry.units),
                         entry.value_uint64);
          break;
        case MemoryAllocatorDump::Entry::EntryType::kString:
          node->AddEntry(entry.name, entry.value_string);
          break;
      }
    }
  }
}

// static
void GraphProcessor::AddEdges(
    const base::trace_event::ProcessMemoryDump& source,
    GlobalDumpGraph* global_graph) {
  const auto& nodes_by_guid = global_graph->nodes_by_guid();
  for (const auto& guid_to_edge : source.allocator_dumps_edges()) {
    auto& edge = guid_to_edge.second;

    // Find the source and target nodes in the global map by guid.
    auto source_it = nodes_by_guid.find(edge.source);
    auto target_it = nodes_by_guid.find(edge.target);

    if (source_it == nodes_by_guid.end()) {
      // If the source is missing then simply pretend the edge never existed
      // leading to the memory being allocated to the target (if it exists).
      continue;
    } else if (target_it == nodes_by_guid.end()) {
      // If the target is lost but the source is present, then also ignore
      // this edge for now.
      // TODO(lalitm): see crbug.com/770712 for the permanent fix for this
      // issue.
      continue;
    } else {
      // Add an edge indicating the source node owns the memory of the
      // target node with the given importance of the edge.
      global_graph->AddNodeOwnershipEdge(source_it->second, target_it->second,
                                         edge.importance);
    }
  }
}

// static
void GraphProcessor::MarkImplicitWeakParentsRecursively(Node* node) {
  // Ensure that we aren't in a bad state where we have an implicit node
  // which doesn't have any children (which is not the root node).
  DCHECK(node->is_explicit() || !node->children()->empty() || !node->parent());

  // Check that at this stage, any node which is weak is only so because
  // it was explicitly created as such.
  DCHECK(!node->is_weak() || node->is_explicit());

  // If a node is already weak then all children will be marked weak at a
  // later stage.
  if (node->is_weak())
    return;

  // Recurse into each child and find out if all the children of this node are
  // weak.
  bool all_children_weak = true;
  for (const auto& path_to_child : *node->children()) {
    MarkImplicitWeakParentsRecursively(path_to_child.second);
    all_children_weak = all_children_weak && path_to_child.second->is_weak();
  }

  // If all the children are weak and the parent is only an implicit one then we
  // consider the parent as weak as well and we will later remove it.
  node->set_weak(!node->is_explicit() && all_children_weak);
}

// static
void GraphProcessor::MarkWeakOwnersAndChildrenRecursively(
    Node* node,
    std::set<const Node*>* visited) {
  // If we've already visited this node then nothing to do.
  if (visited->count(node) != 0)
    return;

  // If we haven't visited the node which this node owns then wait for that.
  if (node->owns_edge() && visited->count(node->owns_edge()->target()) == 0)
    return;

  // If we haven't visited the node's parent then wait for that.
  if (node->parent() && visited->count(node->parent()) == 0)
    return;

  // If either the node we own or our parent is weak, then mark this node
  // as weak.
  if ((node->owns_edge() && node->owns_edge()->target()->is_weak()) ||
      (node->parent() && node->parent()->is_weak())) {
    node->set_weak(true);
  }
  visited->insert(node);

  // Recurse into each owner node to mark any other nodes.
  for (auto* owned_by_edge : *node->owned_by_edges()) {
    MarkWeakOwnersAndChildrenRecursively(owned_by_edge->source(), visited);
  }

  // Recurse into each child and find out if all the children of this node are
  // weak.
  for (const auto& path_to_child : *node->children()) {
    MarkWeakOwnersAndChildrenRecursively(path_to_child.second, visited);
  }
}

// static
void GraphProcessor::RemoveWeakNodesRecursively(Node* node) {
  auto* children = node->children();
  for (auto child_it = children->begin(); child_it != children->end();) {
    Node* child = child_it->second;

    // If the node is weak, remove it. This automatically makes all
    // descendents unreachable from the parents. If this node owned
    // by another, it will have been marked earlier in
    // |MarkWeakOwnersAndChildrenRecursively| and so will be removed
    // by this method at some point.
    if (child->is_weak()) {
      child_it = children->erase(child_it);
      continue;
    }

    // We should never be in a situation where we're about to
    // keep a node which owns a weak node (which will be/has been
    // removed).
    DCHECK(!child->owns_edge() || !child->owns_edge()->target()->is_weak());

    // Descend and remove all weak child nodes.
    RemoveWeakNodesRecursively(child);

    // Remove all edges with owner nodes which are weak.
    std::vector<Edge*>* owned_by_edges = child->owned_by_edges();
    auto new_end =
        std::remove_if(owned_by_edges->begin(), owned_by_edges->end(),
                       [](Edge* edge) { return edge->source()->is_weak(); });
    owned_by_edges->erase(new_end, owned_by_edges->end());

    ++child_it;
  }
}

// static
void GraphProcessor::AssignTracingOverhead(base::StringPiece allocator,
                                           GlobalDumpGraph* global_graph,
                                           Process* process) {
  // This method should only be called if the allocator node exists.
  DCHECK(process->FindNode(allocator));

  // Check that the tracing dump exists and isn't already owning another node.
  Node* tracing_node = process->FindNode("tracing");
  if (!tracing_node)
    return;

  // This should be first edge associated with the tracing node.
  DCHECK(!tracing_node->owns_edge());

  // Create the node under the allocator to which tracing overhead can be
  // assigned.
  std::string child_name =
      allocator.as_string() + "/allocated_objects/tracing_overhead";
  Node* child_node = process->CreateNode(MemoryAllocatorDumpGuid(), child_name,
                                         false /* weak */);

  // Assign the overhead of tracing to the tracing node.
  global_graph->AddNodeOwnershipEdge(tracing_node, child_node,
                                     0 /* importance */);
}

// static
Node::Entry GraphProcessor::AggregateNumericWithNameForNode(
    Node* node,
    base::StringPiece name) {
  bool first = true;
  Node::Entry::ScalarUnits units = Node::Entry::ScalarUnits::kObjects;
  uint64_t aggregated = 0;
  for (auto& path_to_child : *node->children()) {
    auto* entries = path_to_child.second->entries();

    // Retrieve the entry with the given column name.
    auto name_to_entry_it = entries->find(name.as_string());
    if (name_to_entry_it == entries->end())
      continue;

    // Extract the entry from the iterator.
    const Node::Entry& entry = name_to_entry_it->second;

    // Ensure that the entry is numeric.
    DCHECK_EQ(entry.type, Node::Entry::Type::kUInt64);

    // Check that the units of every child's entry with the given name is the
    // same (i.e. we don't get a number for one child and size for another
    // child). We do this by having a DCHECK that the units match the first
    // child's units.
    DCHECK(first || units == entry.units);
    units = entry.units;
    aggregated += entry.value_uint64;
    first = false;
  }
  return Node::Entry(units, aggregated);
}

// static
void GraphProcessor::AggregateNumericsRecursively(Node* node) {
  std::set<std::string> numeric_names;

  for (const auto& path_to_child : *node->children()) {
    AggregateNumericsRecursively(path_to_child.second);
    for (const auto& name_to_entry : *path_to_child.second->entries()) {
      const std::string& name = name_to_entry.first;
      if (name_to_entry.second.type == Node::Entry::Type::kUInt64 &&
          name != kSizeEntryName && name != kEffectiveSizeEntryName) {
        numeric_names.insert(name);
      }
    }
  }

  for (auto& name : numeric_names) {
    node->entries()->emplace(name, AggregateNumericWithNameForNode(node, name));
  }
}

// static
void GraphProcessor::PropagateNumericsAndDiagnosticsRecursively(Node* node) {
  for (const auto& name_to_entry : *node->entries()) {
    for (auto* edge : *node->owned_by_edges()) {
      edge->source()->entries()->insert(name_to_entry);
    }
  }
  for (const auto& path_to_child : *node->children()) {
    PropagateNumericsAndDiagnosticsRecursively(path_to_child.second);
  }
}

// static
base::Optional<uint64_t> GraphProcessor::AggregateSizeForDescendantNode(
    Node* root,
    Node* descendant) {
  Edge* owns_edge = descendant->owns_edge();
  if (owns_edge && owns_edge->target()->IsDescendentOf(*root))
    return 0;

  if (descendant->children()->size() == 0)
    return GetSizeEntryOfNode(descendant).value_or(0ul);

  base::Optional<uint64_t> size;
  for (auto path_to_child : *descendant->children()) {
    auto c_size = AggregateSizeForDescendantNode(root, path_to_child.second);
    if (size)
      *size += c_size.value_or(0);
    else
      size = std::move(c_size);
  }
  return size;
}

// Assumes that this function has been called on all children and owner nodes.
// static
void GraphProcessor::CalculateSizeForNode(Node* node) {
  // Get the size at the root node if it exists.
  base::Optional<uint64_t> node_size = GetSizeEntryOfNode(node);

  // Aggregate the size of all the child nodes.
  base::Optional<uint64_t> aggregated_size;
  for (auto path_to_child : *node->children()) {
    auto c_size = AggregateSizeForDescendantNode(node, path_to_child.second);
    if (aggregated_size)
      *aggregated_size += c_size.value_or(0ul);
    else
      aggregated_size = std::move(c_size);
  }

  // Check that if both aggregated and node sizes exist that the node size
  // is bigger than the aggregated.
  // TODO(lalitm): the following condition is triggered very often even though
  // it is a warning in JS code. Find a way to add the warning to display in UI
  // or to fix all instances where this is violated and then enable this check.
  // DCHECK(!node_size || !aggregated_size || *node_size >= *aggregated_size);

  // Calculate the maximal size of an owner node.
  base::Optional<uint64_t> max_owner_size;
  for (auto* edge : *node->owned_by_edges()) {
    auto o_size = GetSizeEntryOfNode(edge->source());
    if (max_owner_size)
      *max_owner_size = std::max(o_size.value_or(0ul), *max_owner_size);
    else
      max_owner_size = std::move(o_size);
  }

  // Check that if both owner and node sizes exist that the node size
  // is bigger than the owner.
  // TODO(lalitm): the following condition is triggered very often even though
  // it is a warning in JS code. Find a way to add the warning to display in UI
  // or to fix all instances where this is violated and then enable this check.
  // DCHECK(!node_size || !max_owner_size || *node_size >= *max_owner_size);

  // Clear out any existing size entry which may exist.
  node->entries()->erase(kSizeEntryName);

  // If no inference about size can be made then simply return.
  if (!node_size && !aggregated_size && !max_owner_size)
    return;

  // Update the node with the new size entry.
  uint64_t aggregated_size_value = aggregated_size.value_or(0ul);
  uint64_t process_size =
      std::max({node_size.value_or(0ul), aggregated_size_value,
                max_owner_size.value_or(0ul)});
  node->AddEntry(kSizeEntryName, Node::Entry::ScalarUnits::kBytes,
                 process_size);

  // If this is an intermediate node then add a ghost node which stores
  // all sizes not accounted for by the children.
  uint64_t unaccounted = process_size - aggregated_size_value;
  if (unaccounted > 0 && !node->children()->empty()) {
    Node* unspecified = node->CreateChild("<unspecified>");
    unspecified->AddEntry(kSizeEntryName, Node::Entry::ScalarUnits::kBytes,
                          unaccounted);
  }
}

// Assumes that this function has been called on all children and owner nodes.
// static
void GraphProcessor::CalculateDumpSubSizes(Node* node) {
  // Completely skip dumps with undefined size.
  base::Optional<uint64_t> size_opt = GetSizeEntryOfNode(node);
  if (!size_opt)
    return;

  // If the dump is a leaf node, then both sub-sizes are equal to the size.
  if (node->children()->empty()) {
    node->add_not_owning_sub_size(*size_opt);
    node->add_not_owned_sub_size(*size_opt);
    return;
  }

  // Calculate this node's not-owning sub-size by summing up the not-owning
  // sub-sizes of children which do not own another node.
  for (const auto& path_to_child : *node->children()) {
    if (path_to_child.second->owns_edge())
      continue;
    node->add_not_owning_sub_size(path_to_child.second->not_owning_sub_size());
  }

  // Calculate this dump's not-owned sub-size.
  for (const auto& path_to_child : *node->children()) {
    Node* child = path_to_child.second;

    // If the child dump is not owned, then add its not-owned sub-size.
    if (child->owned_by_edges()->empty()) {
      node->add_not_owned_sub_size(child->not_owned_sub_size());
      continue;
    }

    // If the child dump is owned, then add the difference between its size
    // and the largest owner.
    uint64_t largest_owner_size = 0;
    for (Edge* edge : *child->owned_by_edges()) {
      uint64_t source_size = GetSizeEntryOfNode(edge->source()).value_or(0);
      largest_owner_size = std::max(largest_owner_size, source_size);
    }
    uint64_t child_size = GetSizeEntryOfNode(child).value_or(0);
    node->add_not_owned_sub_size(child_size - largest_owner_size);
  }
}

// static
void GraphProcessor::CalculateDumpOwnershipCoefficient(Node* node) {
  // Completely skip dumps with undefined size.
  base::Optional<uint64_t> size_opt = GetSizeEntryOfNode(node);
  if (!size_opt)
    return;

  // We only need to consider owned dumps.
  if (node->owned_by_edges()->empty())
    return;

  // Sort the owners in decreasing order of ownership priority and
  // increasing order of not-owning sub-size (in case of equal priority).
  std::vector<Edge*> owners = *node->owned_by_edges();
  std::sort(owners.begin(), owners.end(), [](Edge* a, Edge* b) {
    if (a->priority() == b->priority()) {
      return a->source()->not_owning_sub_size() <
             b->source()->not_owning_sub_size();
    }
    return b->priority() < a->priority();
  });

  // Loop over the list of owners and distribute the owned dump's not-owned
  // sub-size among them according to their ownership priority and
  // not-owning sub-size.
  uint64_t already_attributed_sub_size = 0;
  for (auto current_it = owners.begin(); current_it != owners.end();) {
    // Find the position of the first owner with lower priority.
    int current_priority = (*current_it)->priority();
    auto next_it =
        std::find_if(current_it, owners.end(), [current_priority](Edge* edge) {
          return edge->priority() < current_priority;
        });

    // Compute the number of nodes which have the same priority as current.
    size_t difference = std::distance(current_it, next_it);

    // Visit the owners with the same priority in increasing order of
    // not-owned sub-size, split the owned memory among them appropriately,
    // and calculate their owning coefficients.
    double attributed_not_owning_sub_size = 0;
    for (; current_it != next_it; current_it++) {
      uint64_t not_owning_sub_size =
          (*current_it)->source()->not_owning_sub_size();
      if (not_owning_sub_size > already_attributed_sub_size) {
        attributed_not_owning_sub_size +=
            (not_owning_sub_size - already_attributed_sub_size) / difference;
        already_attributed_sub_size = not_owning_sub_size;
      }

      if (not_owning_sub_size != 0) {
        double coeff = attributed_not_owning_sub_size / not_owning_sub_size;
        (*current_it)->source()->set_owning_coefficient(coeff);
      }
      difference--;
    }

    // At the end of this loop, we should move to a node with a lower priority.
    DCHECK(current_it == next_it);
  }

  // Attribute the remainder of the owned dump's not-owned sub-size to
  // the dump itself and calculate its owned coefficient.
  uint64_t not_owned_sub_size = node->not_owned_sub_size();
  if (not_owned_sub_size != 0) {
    double remainder_sub_size =
        not_owned_sub_size - already_attributed_sub_size;
    node->set_owned_coefficient(remainder_sub_size / not_owned_sub_size);
  }
}

// static
void GraphProcessor::CalculateDumpCumulativeOwnershipCoefficient(Node* node) {
  // Completely skip nodes with undefined size.
  base::Optional<uint64_t> size_opt = GetSizeEntryOfNode(node);
  if (!size_opt)
    return;

  double cumulative_owned_coefficient = node->owned_coefficient();
  if (node->parent()) {
    cumulative_owned_coefficient *=
        node->parent()->cumulative_owned_coefficient();
  }
  node->set_cumulative_owned_coefficient(cumulative_owned_coefficient);

  if (node->owns_edge()) {
    node->set_cumulative_owning_coefficient(
        node->owning_coefficient() *
        node->owns_edge()->target()->cumulative_owning_coefficient());
  } else if (node->parent()) {
    node->set_cumulative_owning_coefficient(
        node->parent()->cumulative_owning_coefficient());
  } else {
    node->set_cumulative_owning_coefficient(1);
  }
}

// static
void GraphProcessor::CalculateDumpEffectiveSize(Node* node) {
  // Completely skip nodes with undefined size. As a result, each node will
  // have defined effective size if and only if it has defined size.
  base::Optional<uint64_t> size_opt = GetSizeEntryOfNode(node);
  if (!size_opt) {
    node->entries()->erase(kEffectiveSizeEntryName);
    return;
  }

  uint64_t effective_size = 0;
  if (node->children()->empty()) {
    // Leaf node.
    effective_size = *size_opt * node->cumulative_owning_coefficient() *
                     node->cumulative_owned_coefficient();
  } else {
    // Non-leaf node.
    for (const auto& path_to_child : *node->children()) {
      Node* child = path_to_child.second;
      if (!GetSizeEntryOfNode(child))
        continue;
      effective_size +=
          child->entries()->find(kEffectiveSizeEntryName)->second.value_uint64;
    }
  }
  node->AddEntry(kEffectiveSizeEntryName, Node::Entry::ScalarUnits::kBytes,
                 effective_size);
}

}  // namespace memory_instrumentation