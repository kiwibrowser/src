#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import json5_generator
import template_expander

from core.css import css_properties


class ComputedStyleInitialValuesWriter(json5_generator.Writer):
    def __init__(self, json5_file_paths, output_dir):
        super(ComputedStyleInitialValuesWriter, self).__init__([], output_dir)

        json_properties = css_properties.CSSProperties(json5_file_paths)

        self._properties = json_properties.longhands + \
            json_properties.extra_fields
        self._includes = set()
        self._forward_declarations = set()

        for property_ in self._properties:
            # TODO(meade): CursorList and AppliedTextDecorationList are
            # typedefs, not classes, so they can't be forward declared. Find a
            # better way to specify this.
            if property_['default_value'] == 'nullptr' \
                    and not property_['unwrapped_type_name'] == 'CursorList' \
                    and not property_['unwrapped_type_name'] == \
                    'AppliedTextDecorationList':
                self._forward_declarations.add(
                    property_['unwrapped_type_name'])
            else:
                self._includes.update(property_['include_paths'])

        self._outputs = {
            'computed_style_initial_values.h': self.generate_header,
        }

    @template_expander.use_jinja(
        'core/style/templates/computed_style_initial_values.h.tmpl')
    def generate_header(self):
        return {
            'properties': self._properties,
            'forward_declarations': sorted(list(self._forward_declarations)),
            'includes': sorted(list(self._includes))
        }


if __name__ == '__main__':
    json5_generator.Maker(ComputedStyleInitialValuesWriter).main()
