#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '../../..'))

import json5_generator
import template_expander

from collections import namedtuple
from make_css_property_base import CSSPropertyBaseWriter


class PropertyMethod(namedtuple('PropertyMethod', 'name,return_type,parameters')):
    pass


class CSSPropertiesWriter(CSSPropertyBaseWriter):
    def __init__(self, json5_file_paths, output_dir):
        super(CSSPropertiesWriter, self).__init__(json5_file_paths, output_dir)
        assert len(json5_file_paths) == 3,\
            ('CSSPropertiesWriter requires 3 input json5 files, ' +
             'got {}.'.format(len(json5_file_paths)))

        self.template_cache = {}

        # Map of property method name -> (return_type, parameters)
        self._property_methods = {}
        property_methods = json5_generator.Json5File.load_from_files(
            [json5_file_paths[2]])
        for property_method in property_methods.name_dictionaries:
            self._property_methods[property_method['name']] = PropertyMethod(
                name=property_method['name'],
                return_type=property_method['return_type'],
                parameters=property_method['parameters'],
            )

        self._outputs = {}
        output_dir = sys.argv[sys.argv.index('--output_dir') + 1]
        properties = self.css_properties.longhands
        namespace_group = 'Longhand'
        if 'shorthands' in output_dir:
            properties = self.css_properties.shorthands
            namespace_group = 'Shorthand'
        for property_ in properties:
            property_['property_methods'] = [
                self._property_methods[method_name]
                for method_name in property_['property_methods']
            ]
            property_['namespace_group'] = namespace_group
            class_data = self.get_class(property_)
            self.calculate_apply_functions_to_declare(property_)
            self._outputs[class_data.filename + '.h'] = (
                self.generate_property_h_builder(
                    class_data.classname, class_data.filename, property_))
            if 'should_implement_apply_functions_in_cpp' in property_:
                self._outputs[class_data.filename + '.cc'] = (
                    self.generate_property_cpp_builder(
                        class_data.filename, property_))
        for property_ in self.css_properties.aliases:
            if ('shorthands' in output_dir and property_['longhands']) or \
               ('longhands' in output_dir and not property_['longhands']):
                class_data = self.get_class(property_)
                property_['namespace_group'] = namespace_group
                self._outputs[class_data.filename + '.h'] = (
                    self.generate_property_h_builder(
                        class_data.classname, class_data.filename, property_))

    def generate_property_h_builder(self, property_classname, property_filename, property_):
        @template_expander.use_jinja(
            'core/css/properties/templates/css_property_subclass.h.tmpl',
            template_cache=self.template_cache)
        def generate_property_h():
            return {
                'input_files': self._input_files,
                'property_classname': property_classname,
                'property_filename': property_filename,
                'property': property_,
                'includes': sorted(list(self.h_includes(property_)))
            }
        return generate_property_h

    def generate_property_cpp_builder(self, property_filename, property_):
        @template_expander.use_jinja(
            'core/css/properties/templates/css_property_subclass.cc.tmpl',
            template_cache=self.template_cache)
        def generate_property_cpp():
            return {
                'input_files': self._input_files,
                'property_filename': property_filename,
                'property': property_,
                'includes': sorted(list(self.cpp_includes(property_)))
            }
        return generate_property_cpp

    def calculate_apply_functions_to_declare(self, property_):
        if property_['style_builder_template'] in ['background_layer', 'color', 'counter', 'mask_layer']:
            property_['should_implement_apply_functions_in_cpp'] = True

        property_['should_implement_apply_functions'] = (
            property_['is_property'] and
            not property_['longhands'] and
            not property_['direction_aware_options'] and
            not property_['builder_skip'] and
            not property_['style_builder_legacy'])

    def h_includes(self, property_):
        if property_['alias_for']:
            yield "third_party/blink/renderer/core/css/properties/css_unresolved_property.h"
        else:
            yield "third_party/blink/renderer/core/css/properties/" + property_['namespace_group'].lower() + ".h"
            if property_['direction_aware_options']:
                yield "third_party/blink/renderer/core/style_property_shorthand.h"
            for include in self.apply_includes(property_):
                yield 'third_party/blink/renderer/' + include
        if property_['runtime_flag']:
            yield "third_party/blink/renderer/platform/runtime_enabled_features.h"

    def cpp_includes(self, property_):
        if 'should_implement_apply_functions_in_cpp' in property_:
            for include in self.apply_includes(property_):
                yield 'third_party/blink/renderer/' + include

    def apply_includes(self, property_):
        yield "core/css/resolver/style_resolver_state.h"
        yield "core/css/css_primitive_value_mappings.h"
        if property_['converter'] == "CSSPrimitiveValue":
            yield "core/css/css_primitive_value.h"
            yield "core/css/css_primitive_value_mappings.h"
        elif property_['converter'] == "CSSIdentifierValue":
            yield "core/css/css_identifier_value.h"
        else:
            yield "core/css/css_primitive_value_mappings.h"
            yield "core/css/resolver/style_builder_converter.h"
        if property_['font']:
            yield "core/css/resolver/font_builder.h"
        elif property_['svg']:
            yield "core/css/css_primitive_value_mappings.h"
            yield "core/style/computed_style.h"
            yield "core/style/svg_computed_style.h"
        else:
            yield "core/style/computed_style.h"
        if ('style_builder_template_args' in property_ and
                property_['style_builder_template_args'].get('modifier_type')
                in ['Width', 'Slice', 'Outset']):
            yield "core/css/properties/style_building_utils.h"
        if property_.get('style_builder_template') in ['animation', 'background_layer', 'counter',
                                                       'mask_layer', 'transition']:
            yield "core/css/css_value_list.h"
        if property_.get('style_builder_template') in ['counter']:
            yield "core/css/css_value_pair.h"
            yield "core/css/css_custom_ident_value.h"


if __name__ == '__main__':
    json5_generator.Maker(CSSPropertiesWriter).main()
