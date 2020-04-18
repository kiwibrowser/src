# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A 2to3 fixer that reformats docstrings.

This should transform docstrings to be closer to the conventions in pep-0257;
see https://www.python.org/dev/peps/pep-0257/.
"""

import re

from lib2to3.fixer_base import BaseFix
from lib2to3.pgen2 import token
from lib2to3.pygram import python_symbols


class FixDocstrings(BaseFix):

    explicit = True
    _accept_type = token.STRING

    def match(self, node):
        """Returns True if the given node appears to be a docstring.

        Docstrings should always have no previous siblings, and should be
        direct children of simple_stmt.

        Note: This may also match for some edge cases where there are
        simple_stmt strings that aren't the first thing in a module, class
        or function, and thus aren't considered docstrings; but changing these
        strings should not change behavior.
        """
        # Pylint incorrectly warns that there's no member simple_stmt on python_symbols
        # because the attribute is set dynamically.  pylint: disable=no-member
        return (node.value.startswith('"""') and
                node.prev_sibling is None and
                node.parent.type == python_symbols.simple_stmt)

    def transform(self, node, results):
        # First, strip whitespace at the beginning and end.
        node.value = re.sub(r'^"""\s+', '"""', node.value)
        node.value = re.sub(r'\s+"""$', '"""', node.value)

        # For multi-line docstrings, the closing quotes should go on their own line.
        if '\n' in node.value:
            indent = self._find_indent(node)
            node.value = re.sub(r'"""$', '\n' + indent + '"""', node.value)

        node.changed()

    def _find_indent(self, node):
        """Returns the indentation level of the docstring."""
        # The parent is assumed to be a simple_stmt (the docstring statement)
        # either preceded by an indentation, or nothing.
        if not node.parent.prev_sibling or node.parent.prev_sibling.type != token.INDENT:
            return ''
        return node.parent.prev_sibling.value
