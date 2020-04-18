# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from .idl_definition_builder import IdlDefinitionBuilder
from .collection import Collection


# TODO(peria): Merge bindings/scripts/blink_idl_parser.py with tools/idl_parser,
# and put in this directory. Then we can remove this sys.path update.
_SCRIPTS_PATH = os.path.join(os.path.abspath(os.path.dirname(__file__)), os.pardir)
sys.path.append(_SCRIPTS_PATH)

import blink_idl_parser


class Collector(object):

    def __init__(self, component, parser=blink_idl_parser.BlinkIDLParser()):
        self._component = component
        self._collection = Collection(component)
        self._parser = parser

    def collect_from_idl_files(self, filepaths):
        if type(filepaths) == str:
            filepaths = [filepaths]
        for filepath in filepaths:
            try:
                ast = blink_idl_parser.parse_file(self._parser, filepath)
                self.collect_from_ast(ast)
            except ValueError as ve:
                raise ValueError('%s\nin file %s' % (str(ve), filepath))

    def collect_from_idl_text(self, text, filename='TEXT'):
        ast = self._parser.ParseText(filename, text)  # pylint: disable=no-member
        self.collect_from_ast(ast)

    def collect_from_ast(self, node):
        for definition, filepath in IdlDefinitionBuilder.idl_definitions(node):
            self._collection.register_definition(definition, filepath)

    def get_collection(self):
        return self._collection
