# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=import-error,print-statement,relative-import,protected-access

"""Unit tests for code_generator_web_agent_api.py."""

import unittest

from code_generator_web_agent_api import InterfaceContextBuilder
from code_generator_web_agent_api import MethodOverloadSplitter
from code_generator_web_agent_api import STRING_INCLUDE_PATH
from code_generator_web_agent_api import TypeResolver
from idl_definitions import IdlArgument
from idl_definitions import IdlAttribute
from idl_definitions import IdlOperation
from idl_types import IdlType
from idl_types import IdlNullableType
from idl_types import IdlUnionType
from idl_types import PRIMITIVE_TYPES
from idl_types import STRING_TYPES


# TODO(dglazkov): Convert to use actual objects, not stubs.
# See http://crbug.com/673214 for more details.
class IdlTestingHelper(object):
    """A collection of stub makers and helper utils to make testing code
    generation easy."""

    def make_stub_idl_argument(self, name, idl_type, is_optional=False):
        idl_argument = IdlArgument()
        idl_argument.name = name
        idl_argument.idl_type = idl_type
        idl_argument.is_optional = is_optional
        return idl_argument

    def make_stub_idl_attribute(self, name, return_type):
        idl_attribute_stub = IdlAttribute()
        idl_attribute_stub.name = name
        idl_attribute_stub.idl_type = IdlType(return_type)
        return idl_attribute_stub

    def make_stub_idl_operation(self, name, return_type):
        idl_operation_stub = IdlOperation()
        idl_operation_stub.name = name
        idl_operation_stub.idl_type = IdlType(return_type)
        return idl_operation_stub

    def make_stub_idl_type(self, base_type):
        return IdlType(base_type)

    def make_stub_interfaces_info(self, classes_to_paths):
        result = {}
        for class_name, path in classes_to_paths.iteritems():
            result[class_name] = {'include_path': path}
        return result


class TypeResolverTest(unittest.TestCase):

    def test_includes_from_type_should_filter_primitive_types(self):
        helper = IdlTestingHelper()
        type_resolver = TypeResolver({})
        for primitive_type in PRIMITIVE_TYPES:
            idl_type = helper.make_stub_idl_type(primitive_type)
            self.assertEqual(
                type_resolver._includes_from_type(idl_type), set())

    def test_includes_from_type_should_filter_void(self):
        type_resolver = TypeResolver({})
        helper = IdlTestingHelper()
        idl_type = helper.make_stub_idl_type('void')
        self.assertEqual(
            type_resolver._includes_from_type(idl_type), set())

    def test_includes_from_type_should_handle_string(self):
        type_resolver = TypeResolver({})
        helper = IdlTestingHelper()
        for string_type in STRING_TYPES:
            idl_type = helper.make_stub_idl_type(string_type)
            self.assertEqual(
                type_resolver._includes_from_type(idl_type),
                set([STRING_INCLUDE_PATH]))


class MethodOverloadSplitterTest(unittest.TestCase):

    def test_enumerate_argument_types(self):
        splitter = MethodOverloadSplitter(IdlOperation())
        nullable_and_primitive = IdlArgument()
        nullable_and_primitive.idl_type = IdlNullableType(IdlType('double'))
        with self.assertRaises(ValueError):
            splitter._enumerate_argument_types(nullable_and_primitive)

        argument = IdlArgument()
        foo_type = IdlType('Foo')
        bar_type = IdlType('Bar')

        argument.idl_type = foo_type
        self.assertEqual(
            splitter._enumerate_argument_types(argument), [foo_type])

        argument.is_optional = True
        self.assertEqual(
            splitter._enumerate_argument_types(argument), [None, foo_type])

        argument.is_optional = False
        argument.idl_type = IdlUnionType([foo_type, bar_type])
        self.assertEqual(
            splitter._enumerate_argument_types(argument), [foo_type, bar_type])

        argument.is_optional = True
        self.assertEqual(
            splitter._enumerate_argument_types(argument),
            [None, foo_type, bar_type])

    def test_update_argument_lists(self):
        splitter = MethodOverloadSplitter(IdlOperation())
        foo_type = IdlType('Foo')
        bar_type = IdlType('Bar')
        baz_type = IdlType('Baz')
        qux_type = IdlType('Qux')

        result = splitter._update_argument_lists([[]], [foo_type])
        self.assertEqual(result, [[foo_type]])

        result = splitter._update_argument_lists([[]], [foo_type, bar_type])
        self.assertEqual(result, [[foo_type], [bar_type]])

        existing_list = [[foo_type]]
        result = splitter._update_argument_lists(existing_list, [bar_type])
        self.assertEqual(result, [[foo_type, bar_type]])

        existing_list = [[foo_type]]
        result = splitter._update_argument_lists(existing_list,
                                                 [None, bar_type])
        self.assertEqual(result, [[foo_type], [foo_type, bar_type]])

        existing_list = [[foo_type]]
        result = splitter._update_argument_lists(existing_list,
                                                 [bar_type, baz_type])
        self.assertEqual(result, [[foo_type, bar_type], [foo_type, baz_type]])

        existing_list = [[foo_type], [qux_type]]
        result = splitter._update_argument_lists(existing_list,
                                                 [bar_type, baz_type])
        self.assertEqual(result, [
            [foo_type, bar_type],
            [foo_type, baz_type],
            [qux_type, bar_type],
            [qux_type, baz_type],
        ])

        existing_list = [[foo_type], [qux_type]]
        result = splitter._update_argument_lists(existing_list,
                                                 [None, bar_type, baz_type])
        self.assertEqual(result, [
            [foo_type],
            [foo_type, bar_type],
            [foo_type, baz_type],
            [qux_type],
            [qux_type, bar_type],
            [qux_type, baz_type],
        ])

        existing_list = [[foo_type, qux_type]]
        result = splitter._update_argument_lists(existing_list,
                                                 [bar_type, baz_type])
        self.assertEqual(result, [
            [foo_type, qux_type, bar_type],
            [foo_type, qux_type, baz_type],
        ])

    def test_split_into_overloads(self):
        helper = IdlTestingHelper()
        type_double = IdlType('double')
        type_foo = IdlType('foo')
        type_double_or_foo = IdlUnionType([type_double, type_foo])
        idl_operation = IdlOperation()

        idl_operation.arguments = []
        splitter = MethodOverloadSplitter(idl_operation)
        result = splitter.split_into_overloads()
        self.assertEqual(result, [[]])

        idl_operation.arguments = [
            helper.make_stub_idl_argument(None, type_double, True)
        ]
        splitter = MethodOverloadSplitter(idl_operation)
        result = splitter.split_into_overloads()
        self.assertEqual(result, [[], [type_double]])

        idl_operation.arguments = [
            helper.make_stub_idl_argument(None, type_double_or_foo)
        ]
        splitter = MethodOverloadSplitter(idl_operation)
        result = splitter.split_into_overloads()
        self.assertEqual(result, [[type_double], [type_foo]])

    def test_split_add_event_listener(self):
        """Tests how EventTarget.addEventListener is split into respective
           overloads. From:

           void addEventListener(
                DOMString type,
                EventListener? listener,
                optional (AddEventListenerOptions or boolean) options)

           produces: """

        helper = IdlTestingHelper()
        type_dom_string = IdlType('DOMString')
        type_listener = IdlType('EventListener')
        type_options = IdlType('AddEventListenerOptions')
        type_boolean = IdlType('boolean')
        type_union = IdlUnionType([type_options, type_boolean])
        idl_operation = IdlOperation()
        idl_operation.arguments = [
            helper.make_stub_idl_argument('type', type_dom_string),
            helper.make_stub_idl_argument('listener', type_listener),
            helper.make_stub_idl_argument('options', type_union,
                                          is_optional=True)]
        splitter = MethodOverloadSplitter(idl_operation)
        result = splitter.split_into_overloads()
        self.assertEqual(
            result,
            [[type_dom_string, type_listener],
             [type_dom_string, type_listener, type_options],
             [type_dom_string, type_listener, type_boolean]])


class InterfaceContextBuilderTest(unittest.TestCase):

    def test_empty(self):
        builder = InterfaceContextBuilder('test', TypeResolver({}))

        self.assertEqual({'code_generator': 'test'}, builder.build())

    def test_set_name(self):
        helper = IdlTestingHelper()
        builder = InterfaceContextBuilder(
            'test', TypeResolver(helper.make_stub_interfaces_info({
                'foo': 'path_to_foo',
            })))

        builder.set_class_name('foo')
        self.assertEqual({
            'code_generator': 'test',
            'cpp_includes': set(['path_to_foo']),
            'class_name': {
                'snake_case': 'foo',
                'macro_case': 'FOO',
                'upper_camel_case': 'Foo'
            },
        }, builder.build())

    def test_set_inheritance(self):
        helper = IdlTestingHelper()
        builder = InterfaceContextBuilder(
            'test', TypeResolver(helper.make_stub_interfaces_info({
                'foo': 'path_to_foo',
            })))
        builder.set_inheritance('foo')
        self.assertEqual({
            'base_class': 'foo',
            'code_generator': 'test',
            'header_includes': set(['path_to_foo']),
        }, builder.build())

        builder = InterfaceContextBuilder('test', TypeResolver({}))
        builder.set_inheritance(None)
        self.assertEqual({
            'code_generator': 'test',
            'header_includes': set(['third_party/blink/renderer/platform/heap/handle.h']),
        }, builder.build())

    def test_add_attribute(self):
        helper = IdlTestingHelper()
        builder = InterfaceContextBuilder(
            'test', TypeResolver(helper.make_stub_interfaces_info({
                'foo': 'path_to_foo',
                'bar': 'path_to_bar'
            })))

        attribute = helper.make_stub_idl_attribute('foo', 'bar')
        builder.add_attribute(attribute)
        self.assertEqual({
            'code_generator': 'test',
            'cpp_includes': set(['path_to_bar']),
            'attributes': [{'name': 'foo', 'type': 'bar'}],
        }, builder.build())

    def test_add_method(self):
        helper = IdlTestingHelper()
        builder = InterfaceContextBuilder(
            'test', TypeResolver(helper.make_stub_interfaces_info({
                'foo': 'path_to_foo',
                'bar': 'path_to_bar',
                'garply': 'path_to_garply',
            })))

        operation = helper.make_stub_idl_operation('foo', 'bar')
        builder.add_operation(operation)
        self.assertEqual({
            'code_generator': 'test',
            'cpp_includes': set(['path_to_bar']),
            'methods': [{
                'arguments': [],
                'name': 'Foo',
                'type': 'bar'
            }],
        }, builder.build())

        operation = helper.make_stub_idl_operation('quux', 'garply')
        operation.arguments = [
            helper.make_stub_idl_argument('barBaz', IdlType('qux'))
        ]
        builder.add_operation(operation)
        self.assertEqual({
            'code_generator': 'test',
            'cpp_includes': set(['path_to_bar', 'path_to_garply']),
            'methods': [
                {
                    'arguments': [],
                    'name': 'Foo',
                    'type': 'bar'
                },
                {
                    'arguments': [
                        {'name': 'bar_baz', 'type': 'qux'}
                    ],
                    'name': 'Quux',
                    'type': 'garply'
                }
            ],
        }, builder.build())
