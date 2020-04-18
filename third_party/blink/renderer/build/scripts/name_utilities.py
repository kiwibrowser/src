# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os.path
import re

from blinkbuild.name_style_converter import tokenize_name


def lower_first_letter(name):
    """Return name with first letter lowercased."""
    if not name:
        return ''
    return name[0].lower() + name[1:]


def upper_first_letter(name):
    """Return name with first letter uppercased."""
    if not name:
        return ''
    return name[0].upper() + name[1:]


def to_macro_style(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).upper()


def script_name(entry):
    return os.path.basename(entry['name'])


def cpp_bool(value):
    if value is True:
        return 'true'
    if value is False:
        return 'false'
    # Return value as is, which for example may be a platform-dependent constant
    # such as "defaultSelectTrailingWhitespaceEnabled".
    return value


def cpp_name(entry):
    return entry['ImplementedAs'] or script_name(entry)


def enum_for_css_keyword(keyword):
    return 'CSSValue' + upper_camel_case(keyword)


def enum_for_css_property(property_name):
    return 'CSSProperty' + upper_camel_case(property_name)


def enum_for_css_property_alias(property_name):
    return 'CSSPropertyAlias' + upper_camel_case(property_name)

# FIXME: Merge these with the above methods.
# and update all the generators to use these ones.
# FIXME: Switch external callers of these methods to use the high level
# API below instead.


def join_names(*names):
    """Given a list of names, join them into a single space-separated name."""
    result = []
    for name in names:
        result.extend(tokenize_name(name))
    return ' '.join(result)


def naming_style(f):
    """Decorator for name utility functions.

    Wraps a name utility function in a function that takes one or more names,
    splits them into a list of words, and passes the list to the utility function.
    """
    def inner(name_or_names):
        names = name_or_names if isinstance(name_or_names, list) else [name_or_names]
        words = []
        for name in names:
            if name:
                words.extend(tokenize_name(name))
        return f(words)
    return inner


@naming_style
def upper_camel_case(words):
    return ''.join(upper_first_letter(word) for word in words)


@naming_style
def lower_camel_case(words):
    return lower_first_letter(upper_camel_case(words))


@naming_style
def snake_case(words):
    return '_'.join(word.lower() for word in words)


# Use these high level naming functions which describe the semantics of the name,
# rather than a particular style.


@naming_style
def enum_type_name(words):
    return upper_camel_case(words)


@naming_style
def enum_value_name(words):
    return 'k' + upper_camel_case(words)


@naming_style
def class_name(words):
    return upper_camel_case(words)


@naming_style
def class_member_name(words):
    return snake_case(words) + "_"


@naming_style
def method_name(words):
    return upper_camel_case(words)
