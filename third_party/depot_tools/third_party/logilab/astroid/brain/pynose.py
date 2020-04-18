# copyright 2003-2015 LOGILAB S.A. (Paris, FRANCE), all rights reserved.
# contact http://www.logilab.fr/ -- mailto:contact@logilab.fr
#
# This file is part of astroid.
#
# astroid is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation, either version 2.1 of the License, or (at your
# option) any later version.
#
# astroid is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with astroid. If not, see <http://www.gnu.org/licenses/>.

"""Hooks for nose library."""

import re
import unittest

from astroid import List, MANAGER, register_module_extender
from astroid.builder import AstroidBuilder


def _pep8(name, caps=re.compile('([A-Z])')):
    return caps.sub(lambda m: '_' + m.groups()[0].lower(), name)


def nose_transform():
    """Custom transform for the nose.tools module."""

    builder = AstroidBuilder(MANAGER)
    stub = AstroidBuilder(MANAGER).string_build('''__all__ = []''')
    unittest_module = builder.module_build(unittest.case)
    case = unittest_module['TestCase']
    all_entries = ['ok_', 'eq_']

    for method_name, method in case.locals.items():
        if method_name.startswith('assert') and '_' not in method_name:
            pep8_name = _pep8(method_name)
            all_entries.append(pep8_name)
            stub[pep8_name] = method[0]

    # Update the __all__ variable, since nose.tools
    # does this manually with .append.
    all_assign = stub['__all__'].parent
    all_object = List(all_entries)
    all_object.parent = all_assign
    all_assign.value = all_object
    return stub


register_module_extender(MANAGER, 'nose.tools.trivial', nose_transform)
