# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=import-error,print-statement,relative-import

import copy
import re

SPECIAL_TOKENS = [
    # This list should be sorted by length.
    'WebSocket',
    'String16',
    'CString',
    'Float32',
    'Float64',
    'Base64',
    'IFrame',
    'Latin1',
    'PlugIn',
    'SQLite',
    'Uint16',
    'Uint32',
    'WebGL2',
    'ASCII',
    'CSSOM',
    'CType',
    'DList',
    'Int16',
    'Int32',
    'MPath',
    'OList',
    'TSpan',
    'UList',
    'UTF16',
    'Uint8',
    'WebGL',
    'XPath',
    'ETC1',
    'HTML',
    'Int8',
    'S3TC',
    'SPv2',
    'UTF8',
    'sRGB',
    'API',
    'CSS',
    'DNS',
    'DOM',
    'EXT',
    'RTC',
    'SVG',
    'XSS',
    '2D',
    'AX',
    'FE',
    'V0',
    'V8',
]

# Applying _TOKEN_PATTERNS repeatedly should capture any sequence of a-z, A-Z,
# 0-9.
_TOKEN_PATTERNS = [
    # 'Foo' 'foo'
    '[A-Z]?[a-z]+',
    # The following pattern captures only 'FOO' in 'FOOElement'.
    '[A-Z]+(?![a-z])',
    # '2D' '3D', but not '2Dimension'
    '[0-9][Dd](?![a-z])',
    '[0-9]+',
]

_TOKEN_RE = re.compile(r'(' + '|'.join(SPECIAL_TOKENS + _TOKEN_PATTERNS) + r')')


def tokenize_name(name):
    """Tokenize the specified name.

    A token consists of A-Z, a-z, and 0-9 characters. Other characters work as
    token delimiters, and the resultant list won't contain such characters.
    Capital letters also work as delimiters.  E.g. 'FooBar-baz' is tokenized to
    ['Foo', 'Bar', 'baz']. See _TOKEN_PATTERNS for more details.

    This function detects special cases that are not easily discernible without
    additional knowledge, such as recognizing that in SVGSVGElement, the first
    two SVGs are separate tokens, but WebGL is one token.

    Returns:
        A list of token strings.

    """
    return _TOKEN_RE.findall(name)


class NameStyleConverter(object):
    """Converts names from camelCase to various other styles.
    """

    def __init__(self, name):
        self.tokens = tokenize_name(name)

    def to_snake_case(self):
        """Snake case is the file and variable name style per Google C++ Style
           Guide:
           https://google.github.io/styleguide/cppguide.html#Variable_Names

           Also known as the hacker case.
           https://en.wikipedia.org/wiki/Snake_case
        """
        return '_'.join([token.lower() for token in self.tokens])

    def to_upper_camel_case(self):
        """Upper-camel case is the class and function name style per
           Google C++ Style Guide:
           https://google.github.io/styleguide/cppguide.html#Function_Names

           Also known as the PascalCase.
           https://en.wikipedia.org/wiki/Camel_case.
        """
        tokens = self.tokens
        # If the first token is one of SPECIAL_TOKENS, we should replace the
        # token with the matched special token.
        # e.g. ['css', 'External', 'Scanner', 'Preload'] => 'CSSExternalScannerPreload'
        if tokens and tokens[0].lower() == tokens[0]:
            for special in SPECIAL_TOKENS:
                if special.lower() == tokens[0]:
                    tokens = copy.deepcopy(tokens)
                    tokens[0] = special
                    break
        return ''.join([token[0].upper() + token[1:] for token in tokens])

    def to_lower_camel_case(self):
        """Lower camel case is the name style for attribute names and operation
           names in web platform APIs.
           e.g. 'addEventListener', 'documentURI', 'fftSize'
           https://en.wikipedia.org/wiki/Camel_case.
        """
        if not self.tokens:
            return ''
        return self.tokens[0].lower() + ''.join([token[0].upper() + token[1:] for token in self.tokens[1:]])

    def to_macro_case(self):
        """Macro case is the macro name style per Google C++ Style Guide:
           https://google.github.io/styleguide/cppguide.html#Macro_Names
        """
        return '_'.join([token.upper() for token in self.tokens])

    def to_all_cases(self):
        return {
            'snake_case': self.to_snake_case(),
            'upper_camel_case': self.to_upper_camel_case(),
            'macro_case': self.to_macro_case(),
        }
