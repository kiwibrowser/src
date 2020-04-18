#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '../../..'))

import json5_generator
import template_expander

from collections import namedtuple
from core.css import css_properties
from name_utilities import snake_case


class PropertyClassData(
        namedtuple('PropertyClassData', 'enum_value,property_id,classname,namespace_group,filename')):
    pass


class CSSPropertyBaseWriter(json5_generator.Writer):
    def __init__(self, json5_file_paths, output_dir):
        super(CSSPropertyBaseWriter, self).__init__([], output_dir)
        self._input_files = json5_file_paths
        self._outputs = {
            'css_unresolved_property.h': self.generate_unresolved_property_header,
            'css_unresolved_property.cc':
                self.generate_unresolved_property_implementation,
            'css_property.h': self.generate_resolved_property_header,
            'css_property.cc': self.generate_resolved_property_implementation,
        }

        self._css_properties = css_properties.CSSProperties(json5_file_paths)

        # A list of (enum_value, property_id, property_classname) tuples.
        self._property_classes_by_id = []
        self._alias_classes_by_id = []
        # Just a set of class names.
        self._shorthand_property_filenames = set()
        self._longhand_property_filenames = set()
        for property_ in self._css_properties.longhands:
            property_class = self.get_class(property_)
            self._property_classes_by_id.append(property_class)
            if property_class.classname != 'Longhand':
                self._longhand_property_filenames.add(property_class.filename)
        for property_ in self._css_properties.shorthands:
            property_class = self.get_class(property_)
            self._property_classes_by_id.append(property_class)
            self._shorthand_property_filenames.add(property_class.filename)
        for property_ in self._css_properties.aliases:
            property_class = self.get_class(property_)
            self._alias_classes_by_id.append(property_class)
            if property_['longhands']:
                self._shorthand_property_filenames.add(property_class.filename)
            elif property_class.classname != 'Longhand':
                self._longhand_property_filenames.add(property_class.filename)

        # Sort by enum value.
        self._property_classes_by_id.sort(key=lambda t: t.enum_value)
        self._alias_classes_by_id.sort(key=lambda t: t.enum_value)

    def get_class(self, property_):
        """Gets the automatically
        generated class name for a property.
        Args:
            property_: A single property from CSSProperties.properties()
        Returns:
            The name to use for the property class.
        """
        namespace_group = 'Shorthand' if property_['longhands'] else 'Longhand'
        return PropertyClassData(
            enum_value=property_['enum_value'],
            property_id=property_['property_id'],
            classname=property_['upper_camel_name'],
            namespace_group=namespace_group,
            filename=snake_case(property_['upper_camel_name']))

    @property
    def css_properties(self):
        return self._css_properties

    @template_expander.use_jinja(
        'core/css/properties/templates/css_unresolved_property.h.tmpl')
    def generate_unresolved_property_header(self):
        return {
            'input_files': self._input_files,
            'property_classes_by_property_id': self._property_classes_by_id,
            'alias_classes_by_property_id': self._alias_classes_by_id,
        }

    @template_expander.use_jinja(
        'core/css/properties/templates/css_unresolved_property.cc.tmpl')
    def generate_unresolved_property_implementation(self):
        return {
            'input_files': self._input_files,
            'longhand_property_filenames': self._longhand_property_filenames,
            'shorthand_property_filenames': self._shorthand_property_filenames,
            'property_classes_by_property_id': self._property_classes_by_id,
            'alias_classes_by_property_id': self._alias_classes_by_id,
            'last_unresolved_property_id':
                self._css_properties.last_unresolved_property_id,
            'last_property_id': self._css_properties.last_property_id
        }

    @template_expander.use_jinja(
        'core/css/properties/templates/css_property.cc.tmpl')
    def generate_resolved_property_implementation(self):
        return {
            'input_files': self._input_files,
            'property_classes_by_property_id': self._property_classes_by_id,
            'last_property_id': self._css_properties.last_property_id
        }

    @template_expander.use_jinja(
        'core/css/properties/templates/css_property.h.tmpl')
    def generate_resolved_property_header(self):
        return {
            'input_files': self._input_files,
            'property_classes_by_property_id': self._property_classes_by_id,
        }

if __name__ == '__main__':
    json5_generator.Maker(CSSPropertyBaseWriter).main()
