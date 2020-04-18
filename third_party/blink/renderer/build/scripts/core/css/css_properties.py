#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json5_generator
from name_utilities import (
    upper_camel_case,
    lower_camel_case,
    enum_value_name,
    enum_for_css_property,
    enum_for_css_property_alias
)
from core.css.field_alias_expander import FieldAliasExpander


# These values are converted using CSSPrimitiveValue in the setter function,
# if applicable.
PRIMITIVE_TYPES = [
    'short',
    'unsigned short',
    'int',
    'unsigned int',
    'unsigned',
    'float',
    'LineClampValue'
]


# Check properties parameters are valid.
# TODO(jiameng): add more flag checks later.
def check_property_parameters(property_to_check):
    # Only longhand properties can be interpolable.
    if property_to_check['longhands']:
        assert not(property_to_check['interpolable']), \
            'Shorthand property (' + property_to_check['name'] + ') ' \
            'cannot be interpolable'
    if property_to_check['longhands']:
        assert 'parseSingleValue' not in property_to_check['property_methods'], \
            'Shorthand property (' + property_to_check['name'] + ') ' \
            'should not implement parseSingleValue'
    else:
        assert 'parseShorthand' not in property_to_check['property_methods'], \
            'Longhand property (' + property_to_check['name'] + ') ' \
            'should not implement parseShorthand'
    assert property_to_check['is_descriptor'] or \
        property_to_check['is_property'], \
        '{} must be a property, descriptor, or both'.format(
            property_to_check['name'])
    if property_to_check['field_template'] is not None:
        assert not property_to_check['longhands'], \
            "Shorthand '{}' cannot have a field_template.".format(
                property_to_check['name'])
    if property_to_check['mutable']:
        assert property_to_check['field_template'] == 'monotonic_flag', \
            'mutable keyword only implemented for monotonic_flag'


class CSSProperties(object):
    def __init__(self, file_paths):
        assert len(file_paths) >= 2, \
            "CSSProperties at least needs both CSSProperties.json5 and \
            ComputedStyleFieldAliases.json5 to function"

        # ComputedStyleFieldAliases.json5. Used to expand out parameters used
        # in the various generators for ComputedStyle.
        self._field_alias_expander = FieldAliasExpander(file_paths[1])

        # CSSPropertyValueMetadata assumes that there are at most 1024
        # properties + aliases.
        self._alias_offset = 512
        # 0: CSSPropertyInvalid
        # 1: CSSPropertyVariable
        self._first_enum_value = 2
        self._last_used_enum_value = self._first_enum_value

        self._properties_by_id = {}
        self._aliases = []
        self._longhands = []
        self._shorthands = []
        self._properties_including_aliases = []

        # Add default data in CSSProperties.json5. This must be consistent
        # across instantiations of this class.
        css_properties_file = json5_generator.Json5File.load_from_files(
            [file_paths[0]])
        self._default_parameters = css_properties_file.parameters
        self.add_properties(css_properties_file.name_dictionaries)

        assert self._first_enum_value + len(self._properties_by_id) < \
            self._alias_offset, \
            'Property aliasing expects fewer than %d properties.' % \
            self._alias_offset
        self._last_unresolved_property_id = max(
            property_["enum_value"] for property_ in self._aliases)

        # Process extra files passed in.
        self._extra_fields = []
        for i in range(2, len(file_paths)):
            fields = json5_generator.Json5File.load_from_files(
                [file_paths[i]],
                default_parameters=self._default_parameters)
            self._extra_fields.extend(fields.name_dictionaries)
        for field in self._extra_fields:
            self.expand_parameters(field)

    def add_properties(self, properties):
        self._aliases = [
            property_ for property_ in properties if property_['alias_for']]
        self._shorthands = [
            property_ for property_ in properties if property_['longhands']]
        self._longhands = [
            property_ for property_ in properties if (
                not property_['alias_for'] and not property_['longhands'])]

        # Sort the properties by priority, then alphabetically. Ensure that
        # the resulting order is deterministic.
        # Sort properties by priority, then alphabetically.
        for property_ in self._longhands + self._shorthands:
            self.expand_parameters(property_)
            check_property_parameters(property_)
            # This order must match the order in CSSPropertyPriority.h.
            priority_numbers = {'Animation': 0, 'High': 1, 'Low': 2}
            priority = priority_numbers[property_['priority']]
            name_without_leading_dash = property_['name']
            if property_['name'].startswith('-'):
                name_without_leading_dash = property_['name'][1:]
            property_['sorting_key'] = (priority, name_without_leading_dash)

        sorting_keys = {}
        for property_ in self._longhands + self._shorthands:
            key = property_['sorting_key']
            assert key not in sorting_keys, \
                ('Collision detected - two properties have the same name and '
                 'priority, a potentially non-deterministic ordering can '
                 'occur: {}, {} and {}'.format(
                     key, property_['name'], sorting_keys[key]))
            sorting_keys[key] = property_['name']
        self._longhands.sort(key=lambda p: p['sorting_key'])
        self._shorthands.sort(key=lambda p: p['sorting_key'])

        # The sorted index becomes the CSSPropertyID enum value.
        for property_ in self._longhands + self._shorthands:
            property_['enum_value'] = self._last_used_enum_value
            self._last_used_enum_value += 1
            # Add the new property into the map of properties.
            assert property_['property_id'] not in self._properties_by_id, \
                ('property with ID {} appears more than once in the '
                 'properties list'.format(property_['property_id']))
            self._properties_by_id[property_['property_id']] = property_

        self.expand_aliases()
        self._properties_including_aliases = self._longhands + \
            self._shorthands + self._aliases

    def expand_aliases(self):
        for i, alias in enumerate(self._aliases):
            assert not alias['runtime_flag'], \
                "Property '{}' is an alias with a runtime_flag, "\
                "but runtime flags do not currently work for aliases.".format(
                    alias['name'])
            aliased_property = self._properties_by_id[
                enum_for_css_property(alias['alias_for'])]
            updated_alias = aliased_property.copy()
            updated_alias['name'] = alias['name']
            updated_alias['alias_for'] = alias['alias_for']
            updated_alias['aliased_property'] = aliased_property['upper_camel_name']
            updated_alias['property_id'] = enum_for_css_property_alias(
                alias['name'])
            updated_alias['enum_value'] = aliased_property['enum_value'] + \
                self._alias_offset
            updated_alias['upper_camel_name'] = upper_camel_case(alias['name'])
            updated_alias['lower_camel_name'] = lower_camel_case(alias['name'])
            self._aliases[i] = updated_alias

    def expand_parameters(self, property_):
        def set_if_none(property_, key, value):
            if key not in property_ or property_[key] is None:
                property_[key] = value

        # Basic info.
        property_['property_id'] = enum_for_css_property(property_['name'])
        property_['upper_camel_name'] = upper_camel_case(property_['name'])
        property_['lower_camel_name'] = lower_camel_case(property_['name'])
        property_['is_internal'] = property_['name'].startswith('-internal-')
        name = property_['name_for_methods']
        if not name:
            name = upper_camel_case(property_['name']).replace('Webkit', '')
        set_if_none(property_, 'inherited', False)

        # Initial function, Getters and Setters for ComputedStyle.
        property_['initial'] = 'Initial' + name
        simple_type_name = str(property_['type_name']).split('::')[-1]
        set_if_none(property_, 'name_for_methods', name)
        set_if_none(property_, 'type_name', 'E' + name)
        set_if_none(
            property_,
            'getter',
            name if simple_type_name != name else 'Get' + name)
        set_if_none(property_, 'setter', 'Set' + name)
        if property_['inherited']:
            property_['is_inherited_setter'] = 'Set' + name + 'IsInherited'

        # Figure out whether we should generate style builder implementations.
        for x in ['initial', 'inherit', 'value']:
            suppressed = x in property_['style_builder_custom_functions']
            property_['style_builder_generate_%s' % x] = not suppressed

        # Expand StyleBuilderConverter params where necessary.
        if property_['type_name'] in PRIMITIVE_TYPES:
            set_if_none(property_, 'converter', 'CSSPrimitiveValue')
        else:
            set_if_none(property_, 'converter', 'CSSIdentifierValue')

        # Expand out field templates.
        if property_['field_template']:
            self._field_alias_expander.expand_field_alias(property_)

            type_name = property_['type_name']
            if (property_['field_template'] == 'keyword' or
                    property_['field_template'] == 'multi_keyword'):
                default_value = type_name + '::' + \
                    enum_value_name(property_['default_value'])
            elif (property_['field_template'] == 'external' or
                  property_['field_template'] == 'primitive' or
                  property_['field_template'] == 'pointer'):
                default_value = property_['default_value']
            else:
                assert property_['field_template'] == 'monotonic_flag', \
                    "Please put a valid value for field_template; got " + \
                    str(property_['field_template'])
                property_['type_name'] = 'bool'
                default_value = 'false'
            property_['default_value'] = default_value

            property_['unwrapped_type_name'] = property_['type_name']
            if property_['wrapper_pointer_name']:
                assert property_['field_template'] in ['pointer', 'external']
                if property_['field_template'] == 'external':
                    property_['type_name'] = '{}<{}>'.format(
                        property_['wrapper_pointer_name'], type_name)

        # Default values for extra parameters in ComputedStyleExtraFields.json5.
        set_if_none(property_, 'custom_copy', False)
        set_if_none(property_, 'custom_compare', False)
        set_if_none(property_, 'mutable', False)

    @property
    def default_parameters(self):
        return self._default_parameters

    @property
    def aliases(self):
        return self._aliases

    @property
    def shorthands(self):
        return self._shorthands

    @property
    def longhands(self):
        return self._longhands

    @property
    def properties_by_id(self):
        return self._properties_by_id

    @property
    def properties_including_aliases(self):
        return self._properties_including_aliases

    @property
    def first_property_id(self):
        return self._first_enum_value

    @property
    def last_property_id(self):
        return self._first_enum_value + len(self._properties_by_id) - 1

    @property
    def last_unresolved_property_id(self):
        return self._last_unresolved_property_id

    @property
    def alias_offset(self):
        return self._alias_offset

    @property
    def extra_fields(self):
        return self._extra_fields
