// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_GRAPH_PROCESSOR_H_
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_GRAPH_PROCESSOR_H_

#include <memory>

#include "base/process/process_handle.h"
#include "base/trace_event/process_memory_dump.h"
#include "services/resource_coordinator/memory_instrumentation/graph.h"

namespace memory_instrumentation {

class GraphProcessor {
 public:
  // This map does not own the pointers inside.
  using MemoryDumpMap =
      std::map<base::ProcessId, const base::trace_event::ProcessMemoryDump*>;

  static std::unique_ptr<GlobalDumpGraph> CreateMemoryGraph(
      const MemoryDumpMap& process_dumps);

  static void RemoveWeakNodesFromGraph(GlobalDumpGraph* global_graph);

  static void AddOverheadsAndPropogateEntries(GlobalDumpGraph* global_graph);

  static void CalculateSizesForGraph(GlobalDumpGraph* global_graph);

  static std::map<base::ProcessId, uint64_t> ComputeSharedFootprintFromGraph(
      const GlobalDumpGraph& global_graph);

 private:
  friend class GraphProcessorTest;

  static void CollectAllocatorDumps(
      const base::trace_event::ProcessMemoryDump& source,
      GlobalDumpGraph* global_graph,
      GlobalDumpGraph::Process* process_graph);

  static void AddEdges(const base::trace_event::ProcessMemoryDump& source,
                       GlobalDumpGraph* global_graph);

  static void MarkImplicitWeakParentsRecursively(GlobalDumpGraph::Node* node);

  static void MarkWeakOwnersAndChildrenRecursively(
      GlobalDumpGraph::Node* node,
      std::set<const GlobalDumpGraph::Node*>* nodes);

  static void RemoveWeakNodesRecursively(GlobalDumpGraph::Node* parent);

  static void AssignTracingOverhead(base::StringPiece allocator,
                                    GlobalDumpGraph* global_graph,
                                    GlobalDumpGraph::Process* process);

  static GlobalDumpGraph::Node::Entry AggregateNumericWithNameForNode(
      GlobalDumpGraph::Node* node,
      base::StringPiece name);

  static void AggregateNumericsRecursively(GlobalDumpGraph::Node* node);

  static void PropagateNumericsAndDiagnosticsRecursively(
      GlobalDumpGraph::Node* node);

  static base::Optional<uint64_t> AggregateSizeForDescendantNode(
      GlobalDumpGraph::Node* root,
      GlobalDumpGraph::Node* descendant);

  static void CalculateSizeForNode(GlobalDumpGraph::Node* node);

  /**
   * Calculate not-owned and not-owning sub-sizes of a memory allocator dump
   * from its children's (sub-)sizes.
   *
   * Not-owned sub-size refers to the aggregated memory of all children which
   * is not owned by other MADs. Conversely, not-owning sub-size is the
   * aggregated memory of all children which do not own another MAD. The
   * diagram below illustrates these two concepts:
   *
   *     ROOT 1                         ROOT 2
   *     size: 4                        size: 5
   *     not-owned sub-size: 4          not-owned sub-size: 1 (!)
   *     not-owning sub-size: 0 (!)     not-owning sub-size: 5
   *
   *      ^                              ^
   *      |                              |
   *
   *     PARENT 1   ===== owns =====>   PARENT 2
   *     size: 4                        size: 5
   *     not-owned sub-size: 4          not-owned sub-size: 5
   *     not-owning sub-size: 4         not-owning sub-size: 5
   *
   *      ^                              ^
   *      |                              |
   *
   *     CHILD 1                        CHILD 2
   *     size [given]: 4                size [given]: 5
   *     not-owned sub-size: 4          not-owned sub-size: 5
   *     not-owning sub-size: 4         not-owning sub-size: 5
   *
   * This method assumes that (1) the size of the dump, its children, and its
   * owners [see calculateSizes()] and (2) the not-owned and not-owning
   * sub-sizes of both the children and owners of the dump have already been
   * calculated [depth-first post-order traversal].
   */
  static void CalculateDumpSubSizes(GlobalDumpGraph::Node* node);

  /**
   * Calculate owned and owning coefficients of a memory allocator dump and
   * its owners.
   *
   * The owning coefficient refers to the proportion of a dump's not-owning
   * sub-size which is attributed to the dump (only relevant to owning MADs).
   * Conversely, the owned coefficient is the proportion of a dump's
   * not-owned sub-size, which is attributed to it (only relevant to owned
   * MADs).
   *
   * The not-owned size of the owned dump is split among its owners in the
   * order of the ownership importance as demonstrated by the following
   * example:
   *
   *                                          memory allocator dumps
   *                                   OWNED  OWNER1  OWNER2  OWNER3  OWNER4
   *       not-owned sub-size [given]     10       -       -       -       -
   *      not-owning sub-size [given]      -       6       7       5       8
   *               importance [given]      -       2       2       1       0
   *    attributed not-owned sub-size      2       -       -       -       -
   *   attributed not-owning sub-size      -       3       4       0       1
   *                owned coefficient   2/10       -       -       -       -
   *               owning coefficient      -     3/6     4/7     0/5     1/8
   *
   * Explanation: Firstly, 6 bytes are split equally among OWNER1 and OWNER2
   * (highest importance). OWNER2 owns one more byte, so its attributed
   * not-owning sub-size is 6/2 + 1 = 4 bytes. OWNER3 is attributed no size
   * because it is smaller than the owners with higher priority. However,
   * OWNER4 is larger, so it's attributed the difference 8 - 7 = 1 byte.
   * Finally, 2 bytes remain unattributed and are hence kept in the OWNED
   * dump as attributed not-owned sub-size. The coefficients are then
   * directly calculated as fractions of the sub-sizes and corresponding
   * attributed sub-sizes.
   *
   * Note that we always assume that all ownerships of a dump overlap (e.g.
   * OWNER3 is subsumed by both OWNER1 and OWNER2). Hence, the table could
   * be alternatively represented as follows:
   *
   *                                 owned memory range
   *              0   1   2    3    4    5    6        7        8   9  10
   *   Priority 2 |  OWNER1 + OWNER2 (split)  | OWNER2 |
   *   Priority 1 | (already attributed) |
   *   Priority 0 | - - -  (already attributed)  - - - | OWNER4 |
   *    Remainder | - - - - - (already attributed) - - - - - -  | OWNED |
   *
   * This method assumes that (1) the size of the dump [see calculateSizes()]
   * and (2) the not-owned size of the dump and not-owning sub-sizes of its
   * owners [see the first step of calculateEffectiveSizes()] have already
   * been calculated. Note that the method doesn't make any assumptions about
   * the order in which dumps are visited.
   */
  static void CalculateDumpOwnershipCoefficient(GlobalDumpGraph::Node* node);

  /**
   * Calculate cumulative owned and owning coefficients of a memory allocator
   * dump from its (non-cumulative) owned and owning coefficients and the
   * cumulative coefficients of its parent and/or owned dump.
   *
   * The cumulative coefficients represent the total effect of all
   * (non-strict) ancestor ownerships on a memory allocator dump. The
   * cumulative owned coefficient of a MAD can be calculated simply as:
   *
   *   cumulativeOwnedC(M) = ownedC(M) * cumulativeOwnedC(parent(M))
   *
   * This reflects the assumption that if a parent of a child MAD is
   * (partially) owned, then the parent's owner also indirectly owns (a part
   * of) the child MAD.
   *
   * The cumulative owning coefficient of a MAD depends on whether the MAD
   * owns another dump:
   *
   *                           [if M doesn't own another MAD]
   *                         / cumulativeOwningC(parent(M))
   *   cumulativeOwningC(M) =
   *                         \ [if M owns another MAD]
   *                           owningC(M) * cumulativeOwningC(owned(M))
   *
   * The reasoning behind the first case is similar to the one for cumulative
   * owned coefficient above. The only difference is that we don't need to
   * include the dump's (non-cumulative) owning coefficient because it is
   * implicitly 1.
   *
   * The formula for the second case is derived as follows: Since the MAD
   * owns another dump, its memory is not included in its parent's not-owning
   * sub-size and hence shouldn't be affected by the parent's corresponding
   * cumulative coefficient. Instead, the MAD indirectly owns everything
   * owned by its owned dump (and so it should be affected by the
   * corresponding coefficient).
   *
   * Note that undefined coefficients (and coefficients of non-existent
   * dumps) are implicitly assumed to be 1.
   *
   * This method assumes that (1) the size of the dump [see calculateSizes()],
   * (2) the (non-cumulative) owned and owning coefficients of the dump [see
   * the second step of calculateEffectiveSizes()], and (3) the cumulative
   * coefficients of the dump's parent and owned MADs (if present)
   * [depth-first pre-order traversal] have already been calculated.
   */
  static void CalculateDumpCumulativeOwnershipCoefficient(
      GlobalDumpGraph::Node* node);

  /**
   * Calculate the effective size of a memory allocator dump.
   *
   * In order to simplify the (already complex) calculation, we use the fact
   * that effective size is cumulative (unlike regular size), i.e. the
   * effective size of a non-leaf node is equal to the sum of effective sizes
   * of its children. The effective size of a leaf MAD is calculated as:
   *
   *   effectiveSize(M) = size(M) * cumulativeOwningC(M) * cumulativeOwnedC(M)
   *
   * This method assumes that (1) the size of the dump and its children [see
   * calculateSizes()] and (2) the cumulative owning and owned coefficients
   * of the dump (if it's a leaf node) [see the third step of
   * calculateEffectiveSizes()] or the effective sizes of its children (if
   * it's a non-leaf node) [depth-first post-order traversal] have already
   * been calculated.
   */
  static void CalculateDumpEffectiveSize(GlobalDumpGraph::Node* node);
};

}  // namespace memory_instrumentation
#endif