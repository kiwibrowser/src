# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=import-error,print-statement,relative-import,protected-access

"""Unit tests for overload_set_algorithm.py."""

import unittest
from overload_set_algorithm import effective_overload_set


class EffectiveOverloadSetTest(unittest.TestCase):
    def test_example_in_comments(self):
        operation_list = [
            {'arguments': [{'idl_type_object': 'long',  # f1(optional long x)
                            'is_optional': True,
                            'is_variadic': False}]},
            {'arguments': [{'idl_type_object': 'DOMString',  # f2(DOMString s)
                            'is_optional': False,
                            'is_variadic': False}]}]

        overload_set = [
            ({'arguments': [{'idl_type_object': 'long',  # f1(long)
                             'is_optional': True,
                             'is_variadic': False}]},
             ('long',),
             (True,)),
            ({'arguments': [{'idl_type_object': 'long',  # f1()
                             'is_optional': True,
                             'is_variadic': False}]},
             (),
             ()),
            ({'arguments': [{'idl_type_object': 'DOMString',  # f2(DOMString)
                             'is_optional': False,
                             'is_variadic': False}]},
             ('DOMString',),
             (False,))]

        self.assertEqual(effective_overload_set(operation_list), overload_set)

    def test_example_in_spec(self):
        """Tests the example provided in Web IDL spec:
           https://heycam.github.io/webidl/#dfn-effective-overload-set,
           look for example right after the algorithm.

           The output differs from spec because we don't implement the part
           of the algorithm that handles variadic arguments."""
        operation_list = [
            # f1: f(DOMString a)
            {'arguments': [{'idl_type_object': 'DOMString',
                            'is_optional': False,
                            'is_variadic': False}]},
            # f2: f(Node a, DOMString b, double... c)
            {'arguments': [{'idl_type_object': 'Node',
                            'is_optional': False,
                            'is_variadic': False},
                           {'idl_type_object': 'DOMString',
                            'is_optional': False,
                            'is_variadic': False},
                           {'idl_type_object': 'double',
                            'is_optional': False,
                            'is_variadic': True}]},
            # f3: f()
            {'arguments': []},
            # f4: f(Event a, DOMString b, optional DOMString c, double... d)
            {'arguments': [{'idl_type_object': 'Event',
                            'is_optional': False,
                            'is_variadic': False},
                           {'idl_type_object': 'DOMString',
                            'is_optional': False,
                            'is_variadic': False},
                           {'idl_type_object': 'DOMString',
                            'is_optional': True,
                            'is_variadic': False},
                           {'idl_type_object': 'double',
                            'is_optional': False,
                            'is_variadic': True}]}]
        overload_set = [
            # <f1, (DOMString), (required)>
            ({'arguments': [{'idl_type_object': 'DOMString',
                             'is_optional': False,
                             'is_variadic': False}]},
             ('DOMString',),
             (False,)),
            # <f2, (Node, DOMString, double), (required, required, variadic)>
            ({'arguments': [{'idl_type_object': 'Node',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'double',
                             'is_optional': False,
                             'is_variadic': True}]},
             ('Node', 'DOMString', 'double'),
             (False, False, True)),
            # <f2, (Node, DOMString), (required, required)>
            ({'arguments': [{'idl_type_object': 'Node',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'double',
                             'is_optional': False,
                             'is_variadic': True}]},
             ('Node', 'DOMString'),
             (False, False)),
            # Missing from the output:
            # <f2, (Node, DOMString, double, double),
            #       (required, required, variadic, variadic)>,
            # <f3, (), ()>
            ({'arguments': []}, (), ()),
            # <f4, (Event, DOMString, DOMString, double),
            #      (required, required, optional, variadic)>
            ({'arguments': [{'idl_type_object': 'Event',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': True,
                             'is_variadic': False},
                            {'idl_type_object': 'double',
                             'is_optional': False,
                             'is_variadic': True}]},
             ('Event', 'DOMString', 'DOMString', 'double'),
             (False, False, True, True)),
            # <f4, (Event, DOMString, DOMString),
            #      (required, required, optional)>
            ({'arguments': [{'idl_type_object': 'Event',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': True,
                             'is_variadic': False},
                            {'idl_type_object': 'double',
                             'is_optional': False,
                             'is_variadic': True}]},
             ('Event', 'DOMString', 'DOMString'),
             (False, False, True)),
            # <f4, (Event, DOMString), (required, required)>
            ({'arguments': [{'idl_type_object': 'Event',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': False,
                             'is_variadic': False},
                            {'idl_type_object': 'DOMString',
                             'is_optional': True,
                             'is_variadic': False},
                            {'idl_type_object': 'double',
                             'is_optional': False,
                             'is_variadic': True}]},
             ('Event', 'DOMString'),
             (False, False))]

        self.assertEqual(effective_overload_set(operation_list), overload_set)
