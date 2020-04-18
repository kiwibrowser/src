# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Definitions of various graph-related generic functions, used by
# ndk-build internally.
#

# Coding style note:
#
# All internal variables in this file begin with '_ndk_mod_'
# All internal functions in this file begin with '-ndk-mod-'
#

# Set this to true if you want to debug the functions here.
_ndk_mod_debug := $(if $(NDK_DEBUG_MODULES),true)
_ndk_topo_debug := $(if $(NDK_DEBUG_TOPO),true)

# Use $(call -ndk-mod-debug,<message>) to print a debug message only
# if _ndk_mod_debug is set to 'true'. Useful for debugging the functions
# available here.
#
ifeq (true,$(_ndk_mod_debug))
-ndk-mod-debug = $(info $1)
else
-ndk-mod-debug := $(empty)
endif

ifeq (true,$(_ndk_topo_debug))
-ndk-topo-debug = $(info $1)
else
-ndk-topo-debug = $(empty)
endif

#######################################################################
# Filter a list of module with a predicate function
# $1: list of module names.
# $2: predicate function, will be called with $(call $2,<name>), if the
#     result is not empty, <name> will be added to the result.
# Out: subset of input list, where each item passes the predicate.
#######################################################################
-ndk-mod-filter = $(strip \
    $(foreach _ndk_mod_filter_n,$1,\
        $(if $(call $2,$(_ndk_mod_filter_n)),$(_ndk_mod_filter_n))\
    ))

-test-ndk-mod-filter = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call -ndk-mod-filter,,-local-func))\
    $(call test-expect,foo,$(call -ndk-mod-filter,foo,-local-func))\
    $(call test-expect,foo,$(call -ndk-mod-filter,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call -ndk-mod-filter,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call -ndk-mod-filter,,-local-func))\
    $(call test-expect,,$(call -ndk-mod-filter,foo,-local-func))\
    $(call test-expect,bar,$(call -ndk-mod-filter,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call -ndk-mod-filter,aaa foo bar,-local-func))


#######################################################################
# Filter out a list of modules with a predicate function
# $1: list of module names.
# $2: predicate function, will be called with $(call $2,<name>), if the
#     result is not empty, <name> will be added to the result.
# Out: subset of input list, where each item doesn't pass the predicate.
#######################################################################
-ndk-mod-filter-out = $(strip \
    $(foreach _ndk_mod_filter_n,$1,\
        $(if $(call $2,$(_ndk_mod_filter_n)),,$(_ndk_mod_filter_n))\
    ))

-test-ndk-mod-filter-out = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call -ndk-mod-filter-out,,-local-func))\
    $(call test-expect,,$(call -ndk-mod-filter-out,foo,-local-func))\
    $(call test-expect,bar,$(call -ndk-mod-filter-out,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call -ndk-mod-filter-out,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call -ndk-mod-filter-out,,-local-func))\
    $(call test-expect,foo,$(call -ndk-mod-filter-out,foo,-local-func))\
    $(call test-expect,foo,$(call -ndk-mod-filter-out,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call -ndk-mod-filter-out,aaa foo bar foo,-local-func))


#######################################################################
# Find the first item in a list that checks a valid predicate.
# $1: list of names.
# $2: predicate function, will be called with $(call $2,<name>), if the
#     result is not empty, <name> will be added to the result.
# Out: subset of input list.
#######################################################################
-ndk-mod-find-first = $(firstword $(call -ndk-mod-filter,$1,$2))

-test-ndk-mod-find-first.empty = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call -ndk-mod-find-first,,-local-pred))\
    $(call test-expect,,$(call -ndk-mod-find-first,bar,-local-pred))

-test-ndk-mod-find-first.simple = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,foo,$(call -ndk-mod-find-first,foo,-local-pred))\
    $(call test-expect,foo,$(call -ndk-mod-find-first,aaa foo bar,-local-pred))\
    $(call test-expect,foo,$(call -ndk-mod-find-first,aaa foo foo bar,-local-pred))

########################################################################
# Many tree walking operations require setting a 'visited' flag on
# specific graph nodes. The following helper functions help implement
# this while hiding details to the callers.
#
# Technical note:
#  _ndk_mod_tree_visited.<name> will be 'true' if the node was visited,
#  or empty otherwise.
#
#  _ndk_mod_tree_visitors lists all visited nodes, used to clean all
#  _ndk_mod_tree_visited.<name> variables in -ndk-mod-tree-setup-visit.
#
#######################################################################

# Call this before tree traversal.
-ndk-mod-tree-setup-visit = \
    $(foreach _ndk_mod_tree_visitor,$(_ndk_mod_tree_visitors),\
        $(eval _ndk_mod_tree_visited.$$(_ndk_mod_tree_visitor) :=))\
    $(eval _ndk_mod_tree_visitors :=)

# Returns non-empty if a node was visited.
-ndk-mod-tree-is-visited = \
    $(_ndk_mod_tree_visited.$1)

# Set the visited state of a node to 'true'
-ndk-mod-tree-set-visited = \
    $(eval _ndk_mod_tree_visited.$1 := true)\
    $(eval _ndk_mod_tree_visitors += $1)

########################################################################
# Many graph walking operations require a work queue and computing
# dependencies / children nodes. Here are a few helper functions that
# can be used to make their code clearer. This uses a few global
# variables that should be defined as follows during the operation:
#
#  _ndk_mod_module     current graph node name.
#  _ndk_mod_wq         current node work queue.
#  _ndk_mod_list       current result (list of nodes).
#  _ndk_mod_depends    current graph node's children.
#                      you must call -ndk-mod-get-depends to set this.
#
#######################################################################

# Pop first item from work-queue into _ndk_mod_module.
-ndk-mod-pop-first = \
    $(eval _ndk_mod_module := $$(call first,$$(_ndk_mod_wq)))\
    $(eval _ndk_mod_wq     := $$(call rest,$$(_ndk_mod_wq)))

-test-ndk-mod-pop-first = \
    $(eval _ndk_mod_wq := A B C)\
    $(call -ndk-mod-pop-first)\
    $(call test-expect,A,$(_ndk_mod_module))\
    $(call test-expect,B C,$(_ndk_mod_wq))\


# Push list of items at the back of the work-queue.
-ndk-mod-push-back = \
    $(eval _ndk_mod_wq := $(strip $(_ndk_mod_wq) $1))

-test-ndk-mod-push-back = \
  $(eval _ndk_mod_wq := A B C)\
  $(call -ndk-mod-push-back, D    E)\
  $(call test-expect,A B C D E,$(_ndk_mod_wq))

# Set _ndk_mod_depends to the direct dependencies of _ndk_mod_module
-ndk-mod-get-depends = \
    $(eval _ndk_mod_depends := $$(call $$(_ndk_mod_deps_func),$$(_ndk_mod_module)))

# Set _ndk_mod_depends to the direct dependencies of _ndk_mod_module that
# are not already in _ndk_mod_list.
-ndk-mod-get-new-depends = \
    $(call -ndk-mod-get-depends)\
    $(eval _ndk_mod_depends := $$(filter-out $$(_ndk_mod_list),$$(_ndk_mod_depends)))

##########################################################################
# Compute the transitive closure
# $1: list of modules.
# $2: dependency function, $(call $2,<module>) should return all the
#     module that <module> depends on.
# Out: transitive closure of all modules from those in $1. Always includes
#      the modules in $1. Order is random.
#
# Implementation note:
#   we use the -ndk-mod-tree-xxx functions to flag 'visited' nodes
#   in the graph. A node is visited once it has been put into the work
#   queue. For each item in the work queue, get the dependencies and
#   append all those that were not visited yet.
#######################################################################
-ndk-mod-get-closure = $(strip \
    $(eval _ndk_mod_wq :=)\
    $(eval _ndk_mod_list :=)\
    $(eval _ndk_mod_deps_func := $2)\
    $(call -ndk-mod-tree-setup-visit)\
    $(foreach _ndk_mod_module,$1,\
        $(call -ndk-mod-closure-visit,$(_ndk_mod_module))\
    )\
    $(call -ndk-mod-closure-recursive)\
    $(eval _ndk_mod_deps :=)\
    $(_ndk_mod_list)\
    )

# Used internally to visit a new node during -ndk-mod-get-closure.
# This appends the node to the work queue, and set its 'visit' flag.
-ndk-mod-closure-visit = \
    $(call -ndk-mod-push-back,$1)\
    $(call -ndk-mod-tree-set-visited,$1)

-ndk-mod-closure-recursive = \
    $(call -ndk-mod-pop-first)\
    $(eval _ndk_mod_list += $$(_ndk_mod_module))\
    $(call -ndk-mod-get-depends)\
    $(foreach _ndk_mod_dep,$(_ndk_mod_depends),\
        $(if $(call -ndk-mod-tree-is-visited,$(_ndk_mod_dep)),,\
        $(call -ndk-mod-closure-visit,$(_ndk_mod_dep))\
        )\
    )\
    $(if $(_ndk_mod_wq),$(call -ndk-mod-closure-recursive))

-test-ndk-mod-get-closure.empty = \
    $(eval -local-deps = $$($$1_depends))\
    $(call test-expect,,$(call -ndk-mod-get-closure,,-local-deps))

-test-ndk-mod-get-closure.single = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call -ndk-mod-get-closure,A,-local-deps))

-test-ndk-mod-get-closure.double = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends :=)\
    $(call test-expect,A B,$(call -ndk-mod-get-closure,A,-local-deps))

-test-ndk-mod-get-closure.circular-deps = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := A)\
    $(call test-expect,A B C,$(call -ndk-mod-get-closure,A,-local-deps))

-test-ndk-mod-get-closure.ABCDE = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D)\
    $(eval C_depends := D E)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(call test-expect,A B C D E,$(call -ndk-mod-get-closure,A,-local-deps))


#########################################################################
# For topological sort, we need to count the number of incoming edges
# in each graph node. The following helper functions implement this and
# hide implementation details.
#
# Count the number of incoming edges for each node during topological
# sort with a string of xxxxs. I.e.:
#  0 edge  -> ''
#  1 edge  -> 'x'
#  2 edges -> 'xx'
#  3 edges -> 'xxx'
#  etc.
#########################################################################

# zero the incoming edge counter for module $1
-ndk-mod-topo-zero-incoming = \
    $(eval _ndk_mod_topo_incoming.$1 :=)

# increment the incoming edge counter for module $1
-ndk-mod-topo-increment-incoming = \
    $(eval _ndk_mod_topo_incoming.$1 := $$(_ndk_mod_topo_incoming.$1)x)

# decrement the incoming edge counter for module $1
-ndk-mod-topo-decrement-incoming = \
    $(eval _ndk_mod_topo_incoming.$1 := $$(_ndk_mod_topo_incoming.$1:%x=%))

# return non-empty if the module $1's incoming edge counter is > 0
-ndk-mod-topo-has-incoming = $(_ndk_mod_topo_incoming.$1)

# Find first node in a list that has zero incoming edges.
# $1: list of nodes
# Out: first node that has zero incoming edges, or empty.
-ndk-mod-topo-find-first-zero-incoming = $(firstword $(call -ndk-mod-filter-out,$1,-ndk-mod-topo-has-incoming))

# Only use for debugging:
-ndk-mod-topo-dump-count = \
    $(foreach _ndk_mod_module,$1,\
        $(info .. $(_ndk_mod_module) incoming='$(_ndk_mod_topo_incoming.$(_ndk_mod_module))'))



#########################################################################
# Return the topologically ordered closure of all nodes from a top-level
# one. This means that a node A, in the result, will always appear after
# node B if A depends on B. Assumes that the graph is a DAG (if there are
# circular dependencies, this property cannot be guaranteed, but at least
# the function should not loop infinitely).
#
# $1: top-level node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the children
#     nodes for <name>.
# Return: list of nodes, include $1, which will always be the first.
#########################################################################
-ndk-mod-get-topo-list = $(strip \
    $(eval _ndk_mod_top_module := $1)\
    $(eval _ndk_mod_deps_func := $2)\
    $(eval _ndk_mod_nodes := $(call -ndk-mod-get-closure,$1,$2))\
    $(call -ndk-mod-topo-count,$(_ndk_mod_nodes))\
    $(eval _ndk_mod_list :=)\
    $(eval _ndk_mod_wq := $(call -ndk-mod-topo-find-first-zero-incoming,$(_ndk_mod_nodes)))\
    $(call -ndk-mod-topo-sort)\
    $(_ndk_mod_list) $(_ndk_mod_nodes)\
    )

# Given a closure list of nodes, count their incoming edges.
# $1: list of nodes, must be a graph closure.
-ndk-mod-topo-count = \
    $(foreach _ndk_mod_module,$1,\
        $(call -ndk-mod-topo-zero-incoming,$(_ndk_mod_module)))\
    $(foreach _ndk_mod_module,$1,\
        $(call -ndk-mod-get-depends)\
        $(foreach _ndk_mod_dep,$(_ndk_mod_depends),\
        $(call -ndk-mod-topo-increment-incoming,$(_ndk_mod_dep))\
        )\
    )

-ndk-mod-topo-sort = \
    $(call -ndk-topo-debug,-ndk-mod-topo-sort: wq='$(_ndk_mod_wq)' list='$(_ndk_mod_list)')\
    $(call -ndk-mod-pop-first)\
    $(if $(_ndk_mod_module),\
        $(eval _ndk_mod_list += $(_ndk_mod_module))\
        $(eval _ndk_mod_nodes := $(filter-out $(_ndk_mod_module),$(_ndk_mod_nodes)))\
        $(call -ndk-mod-topo-decrement-incoming,$(_ndk_mod_module))\
        $(call -ndk-mod-get-depends)\
        $(call -ndk-topo-debug,-ndk-mod-topo-sort:   deps='$(_ndk_mod_depends)')\
        $(foreach _ndk_mod_dep,$(_ndk_mod_depends),\
            $(call -ndk-mod-topo-decrement-incoming,$(_ndk_mod_dep))\
            $(if $(call -ndk-mod-topo-has-incoming,$(_ndk_mod_dep)),,\
                $(call -ndk-mod-push-back,$(_ndk_mod_dep))\
            )\
        )\
        $(call -ndk-mod-topo-sort)\
    )


-test-ndk-mod-get-topo-list.empty = \
    $(eval -local-deps = $$($$1_depends))\
    $(call test-expect,,$(call -ndk-mod-get-topo-list,,-local-deps))

-test-ndk-mod-get-topo-list.single = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call -ndk-mod-get-topo-list,A,-local-deps))

-test-ndk-mod-get-topo-list.no-infinite-loop = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := A)\
    $(call test-expect,A B C,$(call -ndk-mod-get-topo-list,A,-local-deps))

-test-ndk-mod-get-topo-list.ABC = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends :=)\
    $(eval C_depends := B)\
    $(call test-expect,A C B,$(call -ndk-mod-get-topo-list,A,-local-deps))

-test-ndk-mod-get-topo-list.ABCD = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D)\
    $(eval C_depends := B)\
    $(eval D_depends :=)\
    $(call test-expect,A C B D,$(call -ndk-mod-get-topo-list,A,-local-deps))

-test-ndk-mod-get-topo-list.ABC.circular = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := B)\
    $(call test-expect,A B C,$(call -ndk-mod-get-topo-list,A,-local-deps))

#########################################################################
# Return the topologically ordered closure of all dependencies from a
# top-level node.
#
# $1: top-level node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the children
#     nodes for <name>.
# Return: list of nodes, include $1, which will never be included.
#########################################################################
-ndk-mod-get-topological-depends = $(call rest,$(call -ndk-mod-get-topo-list,$1,$2))

-test-ndk-mod-get-topological-depends.simple = \
    $(eval -local-get-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends :=)\
    $(eval topo_deps := $$(call -ndk-mod-get-topological-depends,A,-local-get-deps))\
    $(call test-expect,B,$(topo_deps),topo dependencies)

-test-ndk-mod-get-topological-depends.ABC = \
    $(eval -local-get-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends :=)\
    $(eval C_depends := B)\
    $(eval bfs_deps := $$(call -ndk-mod-get-bfs-depends,A,-local-get-deps))\
    $(eval topo_deps := $$(call -ndk-mod-get-topological-depends,A,-local-get-deps))\
    $(call test-expect,B C,$(bfs_deps),dfs dependencies)\
    $(call test-expect,C B,$(topo_deps),topo dependencies)

-test-ndk-mod-get-topological-depends.circular = \
    $(eval -local-get-deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := B)\
    $(eval bfs_deps := $$(call -ndk-mod-get-bfs-depends,A,-local-get-deps))\
    $(eval topo_deps := $$(call -ndk-mod-get-topological-depends,A,-local-get-deps))\
    $(call test-expect,B C,$(bfs_deps),dfs dependencies)\
    $(call test-expect,B C,$(topo_deps),topo dependencies)

#########################################################################
# Return breadth-first walk of a graph, starting from an arbitrary
# node.
#
# This performs a breadth-first walk of the graph and will return a
# list of nodes. Note that $1 will always be the first in the list.
#
# $1: root node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the nodes
#     that <name> depends on.
# Result: list of dependent modules, $1 will be part of it.
#########################################################################
-ndk-mod-get-bfs-list = $(strip \
    $(eval _ndk_mod_wq := $(call strip-lib-prefix,$1)) \
    $(eval _ndk_mod_deps_func := $2)\
    $(eval _ndk_mod_list :=)\
    $(call -ndk-mod-tree-setup-visit)\
    $(call -ndk-mod-tree-set-visited,$(_ndk_mod_wq))\
    $(call -ndk-mod-bfs-recursive) \
    $(_ndk_mod_list))

# Recursive function used to perform a depth-first scan.
# Must initialize _ndk_mod_list, _ndk_mod_field, _ndk_mod_wq
# before calling this.
-ndk-mod-bfs-recursive = \
    $(call -ndk-mod-debug,-ndk-mod-bfs-recursive wq='$(_ndk_mod_wq)' list='$(_ndk_mod_list)' visited='$(_ndk_mod_tree_visitors)')\
    $(call -ndk-mod-pop-first)\
    $(eval _ndk_mod_list += $$(_ndk_mod_module))\
    $(call -ndk-mod-get-depends)\
    $(call -ndk-mod-debug,.  node='$(_ndk_mod_module)' deps='$(_ndk_mod_depends)')\
    $(foreach _ndk_mod_child,$(_ndk_mod_depends),\
        $(if $(call -ndk-mod-tree-is-visited,$(_ndk_mod_child)),,\
            $(call -ndk-mod-tree-set-visited,$(_ndk_mod_child))\
            $(call -ndk-mod-push-back,$(_ndk_mod_child))\
        )\
    )\
    $(if $(_ndk_mod_wq),$(call -ndk-mod-bfs-recursive))

-test-ndk-mod-get-bfs-list.empty = \
    $(eval -local-deps = $$($$1_depends))\
    $(call test-expect,,$(call -ndk-mod-get-bfs-list,,-local-deps))

-test-ndk-mod-get-bfs-list.A = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call -ndk-mod-get-bfs-list,A,-local-deps))

-test-ndk-mod-get-bfs-list.ABCDEF = \
    $(eval -local-deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D E)\
    $(eval C_depends := F E)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(eval F_depends :=)\
    $(call test-expect,A B C D E F,$(call -ndk-mod-get-bfs-list,A,-local-deps))

#########################################################################
# Return breadth-first walk of a graph, starting from an arbitrary
# node.
#
# This performs a breadth-first walk of the graph and will return a
# list of nodes. Note that $1 will _not_ be part of the list.
#
# $1: root node name.
# $2: dependency function, i.e. $(call $2,<name>) returns the nodes
#     that <name> depends on.
# Result: list of dependent modules, $1 will not be part of it.
#########################################################################
-ndk-mod-get-bfs-depends = $(call rest,$(call -ndk-mod-get-bfs-list,$1,$2))

-test-ndk-mod-get-bfs-depends.simple = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends :=)\
    $(eval deps := $$(call -ndk-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B,$(deps))

-test-ndk-mod-get-bfs-depends.ABC = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends :=)\
    $(eval C_depends := B)\
    $(eval deps := $$(call -ndk-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B C,$(deps))\

-test-ndk-mod-get-bfs-depends.ABCDE = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D)\
    $(eval C_depends := D E F)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(eval F_depends :=)\
    $(eval deps := $$(call -ndk-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B C D E F,$(deps))\

-test-ndk-mod-get-bfs-depends.loop = \
    $(eval -local-deps-func = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := A)\
    $(eval deps := $$(call -ndk-mod-get-bfs-depends,A,-local-deps-func))\
    $(call test-expect,B,$(deps))
