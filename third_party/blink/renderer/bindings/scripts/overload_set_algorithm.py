# Copyright 2017 The Chromium Authors. All rights reserved.
# coding=utf-8
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import Counter
import itertools
from operator import itemgetter


def sort_and_groupby(list_to_sort, key=None):
    """Returns a generator of (key, list), sorting and grouping list by key."""
    list_to_sort.sort(key=key)
    return ((k, list(g)) for k, g in itertools.groupby(list_to_sort, key))


def effective_overload_set(F):  # pylint: disable=invalid-name
    """Returns the effective overload set of an overloaded function.

    An effective overload set is the set of overloaded functions + signatures
    (type list of arguments, with optional and variadic arguments included or
    not), and is used in the overload resolution algorithm.

    For example, given input [f1(optional long x), f2(DOMString s)], the output
    is informally [f1(), f1(long), f2(DOMString)], and formally
    [(f1, [], []), (f1, [long], [optional]), (f2, [DOMString], [required])].

    Currently the optionality list is a list of |is_optional| booleans (True
    means optional, False means required); to support variadics this needs to
    be tri-valued as required, optional, or variadic.

    Formally:
    An effective overload set represents the allowable invocations for a
    particular operation, constructor (specified with [Constructor] or
    [NamedConstructor]), or callback function.

    An additional argument N (argument count) is needed when overloading
    variadics, but we don't use that currently.

    Spec: http://heycam.github.io/webidl/#dfn-effective-overload-set

    Formally the input and output lists are sets, but methods are stored
    internally as dicts, which can't be stored in a set because they are not
    hashable, so we use lists instead.

    Arguments:
        F: list of overloads for a given callable name.
        value_reader: an OverloadSetValueReader instance.

    Returns:
        S: list of tuples of the form (callable, type list, optionality list).
    """
    # Code closely follows the algorithm in the spec, for clarity and
    # correctness, and hence is not very Pythonic.

    # 1. Initialize S to ∅.
    # (We use a list because we can't use a set, as noted above.)
    S = []  # pylint: disable=invalid-name

    # 2. Let F be a set with elements as follows, according to the kind of
    # effective overload set:
    # (Passed as argument, nothing to do.)

    # 3. & 4. (maxarg, m) are only needed for variadics, not used.

    # 5. For each operation, extended attribute or callback function X in F:
    for X in F:  # X is the "callable". pylint: disable=invalid-name
        arguments = X['arguments']  # pylint: disable=invalid-name
        # 1. Let n be the number of arguments X is declared to take.
        n = len(arguments)  # pylint: disable=invalid-name
        # 2. Let t0..n−1 be a list of types, where ti is the type of X’s
        # argument at index i.
        # (“type list”)
        t = tuple(argument['idl_type_object']  # pylint: disable=invalid-name
                  for argument in arguments)
        # 3. Let o0..n−1 be a list of optionality values, where oi is “variadic”
        # if X’s argument at index i is a final, variadic argument, “optional”
        # if the argument is optional, and “required” otherwise.
        # (“optionality list”)
        # (We’re just using a boolean for optional/variadic vs. required.)
        o = tuple(argument['is_optional']  # pylint: disable=invalid-name
                  or argument['is_variadic']
                  for argument in arguments)
        # 4. Add to S the tuple <X, t0..n−1, o0..n−1>.
        S.append((X, t, o))
        # 5. If X is declared to be variadic, then:
        # (Not used, so not implemented.)
        # 6. Initialize i to n−1.
        i = n - 1
        # 7. While i ≥ 0:
        # Spec bug (fencepost error); should be “While i > 0:”
        # https://www.w3.org/Bugs/Public/show_bug.cgi?id=25590
        while i > 0:
            # 1. If argument i of X is not optional, then break this loop.
            if not o[i]:
                break
            # 2. Otherwise, add to S the tuple <X, t0..i−1, o0..i−1>.
            S.append((X, t[:i], o[:i]))
            # 3. Set i to i−1.
            i = i - 1
        # 8. If n > 0 and all arguments of X are optional, then add to S the
        # tuple <X, (), ()> (where “()” represents the empty list).
        if n > 0 and all(oi for oi in o):
            S.append((X, (), ()))
    # 6. The effective overload set is S.
    return S


def effective_overload_set_by_length(overloads):
    def type_list_length(entry):
        # Entries in the effective overload set are 3-tuples:
        # (callable, type list, optionality list)
        return len(entry[1])

    effective_overloads = effective_overload_set(overloads)
    return list(sort_and_groupby(effective_overloads, type_list_length))


def method_overloads_by_name(methods):
    """Returns generator of overloaded methods by name: [name, [method]]"""
    # Filter to only methods that are actually overloaded
    method_counts = Counter(method['name'] for method in methods)
    overloaded_method_names = set(name
                                  for name, count in method_counts.iteritems()
                                  if count > 1)
    overloaded_methods = [method for method in methods
                          if method['name'] in overloaded_method_names]

    # Group by name (generally will be defined together, but not necessarily)
    return sort_and_groupby(overloaded_methods, itemgetter('name'))
