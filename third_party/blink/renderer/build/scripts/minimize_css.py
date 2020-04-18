#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import functools
import os.path
import re
import sys

import in_generator


class CSSMinimizer(object):

    INITIAL = 0
    MAYBE_COMMENT_START = 1
    INSIDE_COMMENT = 2
    MAYBE_COMMENT_END = 3
    INSIDE_SINGLE_QUOTE = 4
    INSIDE_SINGLE_QUOTE_ESCAPE = 5
    INSIDE_DOUBLE_QUOTE = 6
    INSIDE_DOUBLE_QUOTE_ESCAPE = 7

    def __init__(self):
        self._output = ''
        self._codeblock = ''

    def flush_codeblock(self):
        stripped = re.sub(r"\s+", ' ', self._codeblock)
        stripped = re.sub(r";?\s*(?P<op>[{};])\s*", r'\g<op>', stripped)
        self._output += stripped
        self._codeblock = ''

    def parse(self, content):
        state = self.INITIAL
        for char in content:
            if state == self.INITIAL:
                if char == '/':
                    state = self.MAYBE_COMMENT_START
                elif char == "'":
                    self.flush_codeblock()
                    self._output += char
                    state = self.INSIDE_SINGLE_QUOTE
                elif char == '"':
                    self.flush_codeblock()
                    self._output += char
                    state = self.INSIDE_DOUBLE_QUOTE
                else:
                    self._codeblock += char
            elif state == self.MAYBE_COMMENT_START:
                if char == '*':
                    self.flush_codeblock()
                    state = self.INSIDE_COMMENT
                else:
                    self._codeblock += '/' + char
                    state = self.INITIAL
            elif state == self.INSIDE_COMMENT:
                if char == '*':
                    state = self.MAYBE_COMMENT_END
                else:
                    pass
            elif state == self.MAYBE_COMMENT_END:
                if char == '/':
                    state = self.INITIAL
                else:
                    state = self.INSIDE_COMMENT
            elif state == self.INSIDE_SINGLE_QUOTE:
                if char == '\\':
                    self._output += char
                    state = self.INSIDE_SINGLE_QUOTE_ESCAPE
                elif char == "'":
                    self._output += char
                    state = self.INITIAL
                else:
                    self._output += char
            elif state == self.INSIDE_SINGLE_QUOTE_ESCAPE:
                self._output += char
                state = self.INSIDE_SINGLE_QUOTE
            elif state == self.INSIDE_DOUBLE_QUOTE:
                if char == '\\':
                    self._output += char
                    state = self.INSIDE_DOUBLE_QUOTE_ESCAPE
                elif char == '"':
                    self._output += char
                    state = self.INITIAL
                else:
                    self._output += char
            elif state == self.INSIDE_DOUBLE_QUOTE_ESCAPE:
                self._output += char
                state = self.INSIDE_DOUBLE_QUOTE

        self.flush_codeblock()
        self._output = self._output.strip()
        return self._output

    @classmethod
    def minimize_css(cls, content):
        minimizer = CSSMinimizer()
        return minimizer.parse(content)


class CSSMinimizerWriter(in_generator.GenericWriter):

    def __init__(self, in_file_paths):
        super(CSSMinimizerWriter, self).__init__(in_file_paths)

        self._outputs = {}
        for in_file_path in in_file_paths:
            out_path = os.path.basename(in_file_path)
            self._outputs[out_path] = functools.partial(self.generate_implementation, in_file_path)

    def generate_implementation(self, in_file_path):
        content = ''
        with open(os.path.abspath(in_file_path)) as in_file:
            content = in_file.read()
        return CSSMinimizer.minimize_css(content)


if __name__ == '__main__':
    in_generator.Maker(CSSMinimizerWriter).main(sys.argv)
