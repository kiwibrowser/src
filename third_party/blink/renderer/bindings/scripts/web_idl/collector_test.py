#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from .collector import Collector


class CollectorTest(unittest.TestCase):

    def setUp(self):
        self._collector = Collector(component='test')

    def collect_from_idl_text(self, idl_text):
        self._collector.collect_from_idl_text(idl_text)
        return self._collector.get_collection()

    def test_definition_filters(self):
        idl_text = """
        interface MyInterface {};
        partial interface MyInterface {};
        dictionary MyDictionary {};
        dictionary MyDictionary2 {};
        partial dictionary MyPartialDictionary {};
        namespace MyNamespace {};
        partial namespace MyNamespace {};
        partial namespace MyNamespace2 {};
        partial namespace MyNamespace2 {};
        enum MyEnum { "FOO" };
        callback MyCallbackFunction = void (DOMString arg);
        typedef sequence<Point> Points;
        Foo implements Bar;
        """
        collection = self.collect_from_idl_text(idl_text)
        self.assertEqual(1, len(collection.callback_function_identifiers))
        self.assertEqual(1, len(collection.enumeration_identifiers))
        self.assertEqual(1, len(collection.interface_identifiers))
        self.assertEqual(1, len(collection.namespace_identifiers))
        self.assertEqual(1, len(collection.typedef_identifiers))
        self.assertEqual(2, len(collection.dictionary_identifiers))
        self.assertEqual(1, len(collection.implements))
        self.assertEqual(2, len(collection.find_partial_definition('MyNamespace2')))

    def test_interface(self):
        idl_text = """
        interface InterfaceSimpleMembers {
            void operation1(DOMString arg);
            attribute long longMember;
        };
        interface InheritInterface : InheritedInterface {};
        partial interface PartialInterface {
            attribute long longMember;
        };
        partial interface PartialInterface {
            attribute long long longlongMember;
        };
        """
        collection = self.collect_from_idl_text(idl_text)

        interface = collection.find_non_partial_definition('InterfaceSimpleMembers')
        self.assertEqual('InterfaceSimpleMembers', interface.identifier)
        self.assertEqual(1, len(interface.attributes))
        self.assertEqual(1, len(interface.operations))
        self.assertEqual('operation1', interface.operations[0].identifier)
        self.assertEqual('longMember', interface.attributes[0].identifier)

        interface = collection.find_non_partial_definition('InheritInterface')
        self.assertEqual('InheritInterface', interface.identifier)
        self.assertEqual('InheritedInterface', interface.inherited_interface_name)

        partial_interfaces = collection.find_partial_definition('PartialInterface')
        self.assertTrue(partial_interfaces[0].is_partial)
        self.assertTrue(partial_interfaces[1].is_partial)
        attribute = partial_interfaces[0].attributes[0]
        self.assertEqual('longMember', attribute.identifier)
        attribute = partial_interfaces[1].attributes[0]
        self.assertEqual('longlongMember', attribute.identifier)

        idl_text = """
        interface InterfaceAttributes {
            attribute long longAttr;
            readonly attribute octet readonlyAttr;
            static attribute DOMString staticStringAttr;
            attribute [TreatNullAs=EmptyString] DOMString annotatedTypeAttr;
            [Unforgeable] attribute DOMString? extendedAttributeAttr;
        };
        """
        collection = self.collect_from_idl_text(idl_text)
        interface = collection.find_non_partial_definition('InterfaceAttributes')
        attributes = interface.attributes
        self.assertEqual(5, len(attributes))
        attribute = attributes[0]
        self.assertEqual('longAttr', attribute.identifier)
        self.assertEqual('Long', attribute.type.type_name)

        attribute = attributes[1]
        self.assertEqual('readonlyAttr', attribute.identifier)
        self.assertEqual('Octet', attribute.type.type_name)
        self.assertTrue(attribute.is_readonly)

        attribute = attributes[2]
        self.assertEqual('staticStringAttr', attribute.identifier)
        self.assertEqual('String', attribute.type.type_name)
        self.assertTrue(attribute.is_static)

        attribute = attributes[3]
        self.assertEqual('annotatedTypeAttr', attribute.identifier)
        self.assertEqual('String', attribute.type.type_name)
        self.assertEqual('EmptyString', attribute.type.treat_null_as)

        attribute = attributes[4]
        self.assertEqual('extendedAttributeAttr', attribute.identifier)
        self.assertEqual('String', attribute.type.type_name)
        self.assertTrue(attribute.type.is_nullable)
        self.assertTrue(attribute.extended_attribute_list.has('Unforgeable'))

    def test_extended_attributes(self):
        idl_text = """
        [
            NoInterfaceObject,
            OriginTrialEnabled=FooBar
        ] interface ExtendedAttributeInterface {};
        """
        collection = self.collect_from_idl_text(idl_text)

        interface = collection.find_non_partial_definition('ExtendedAttributeInterface')
        extended_attribute_list = interface.extended_attribute_list
        self.assertTrue(extended_attribute_list.has('OriginTrialEnabled'))
        self.assertTrue(extended_attribute_list.has('NoInterfaceObject'))
        self.assertEqual('FooBar', extended_attribute_list.get('OriginTrialEnabled'))

        idl_text = """
        [
            Constructor,
            Constructor(DOMString arg),
            CustomConstructor,
            CustomConstructor(long arg),
            NamedConstructor=Audio,
            NamedConstructor=Audio(DOMString src)
        ] interface ConstructorInterface {};
        """
        collection = self.collect_from_idl_text(idl_text)

        interface = collection.find_non_partial_definition('ConstructorInterface')
        constructors = interface.constructors
        self.assertEqual(4, len(constructors))
        self.assertFalse(constructors[0].is_custom)
        self.assertFalse(constructors[1].is_custom)
        self.assertTrue(constructors[2].is_custom)
        self.assertTrue(constructors[3].is_custom)
        named_constructors = interface.named_constructors
        self.assertEqual('Audio', named_constructors[0].identifier)
        self.assertEqual('Audio', named_constructors[1].identifier)
        self.assertEqual('arg', constructors[1].arguments[0].identifier)
        self.assertEqual('arg', constructors[3].arguments[0].identifier)
        self.assertEqual('src', named_constructors[1].arguments[0].identifier)

        idl_text = """
        [
            Exposed=(Window, Worker),
            Exposed(Window Feature1, Worker Feature2)
        ] interface ExposedInterface {};
        """
        collection = self.collect_from_idl_text(idl_text)

        interface = collection.find_non_partial_definition('ExposedInterface')
        exposures = interface.exposures
        self.assertEqual(4, len(exposures))
        self.assertEqual('Window', exposures[0].global_interface)
        self.assertEqual('Worker', exposures[1].global_interface)
        self.assertEqual('Window', exposures[2].global_interface)
        self.assertEqual('Worker', exposures[3].global_interface)
        self.assertEqual('Feature1', exposures[2].runtime_enabled_feature)
        self.assertEqual('Feature2', exposures[3].runtime_enabled_feature)


if __name__ == '__main__':
    unittest.main(verbosity=2)
