# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Implements a utility for parsing and formatting path templates."""

from __future__ import absolute_import
from collections import namedtuple

from ply import lex, yacc

_BINDING = 1
_END_BINDING = 2
_TERMINAL = 3
_Segment = namedtuple('_Segment', ['kind', 'literal'])


def _format(segments):
    template = ''
    slash = True
    for segment in segments:
        if segment.kind == _TERMINAL:
            if slash:
                template += '/'
            template += segment.literal
        slash = True
        if segment.kind == _BINDING:
            template += '/{%s=' % segment.literal
            slash = False
        if segment.kind == _END_BINDING:
            template += '%s}' % segment.literal
    return template[1:]  # Remove the leading /


class ValidationException(Exception):
    """Represents a path template validation error."""
    pass


class PathTemplate(object):
    """Represents a path template."""

    segments = None
    segment_count = 0

    def __init__(self, data):
        parser = _Parser()
        self.segments = parser.parse(data)
        self.verb = parser.verb
        self.segment_count = parser.segment_count

    def __len__(self):
        return self.segment_count

    def __repr__(self):
        return _format(self.segments)

    def render(self, bindings):
        """Renders a string from a path template using the provided bindings.

        Args:
            bindings (dict): A dictionary of var names to binding strings.

        Returns:
            str: The rendered instantiation of this path template.

        Raises:
            ValidationError: If a key isn't provided or if a sub-template can't
                be parsed.
        """
        out = []
        binding = False
        for segment in self.segments:
            if segment.kind == _BINDING:
                if segment.literal not in bindings:
                    raise ValidationException(
                        ('rendering error: value for key \'{}\' '
                         'not provided').format(segment.literal))
                out.extend(PathTemplate(bindings[segment.literal]).segments)
                binding = True
            elif segment.kind == _END_BINDING:
                binding = False
            else:
                if binding:
                    continue
                out.append(segment)
        path = _format(out)
        self.match(path)
        return path

    def match(self, path):
        """Matches a fully qualified path template string.

        Args:
            path (str): A fully qualified path template string.

        Returns:
            dict: Var names to matched binding values.

        Raises:
            ValidationException: If path can't be matched to the template.
        """
        this = self.segments
        that = path.split('/')
        current_var = None
        bindings = {}
        segment_count = self.segment_count
        j = 0
        for i in range(0, len(this)):
            if j >= len(that):
                break
            if this[i].kind == _TERMINAL:
                if this[i].literal == '*':
                    bindings[current_var] = that[j]
                    j += 1
                elif this[i].literal == '**':
                    until = j + len(that) - segment_count + 1
                    segment_count += len(that) - segment_count
                    bindings[current_var] = '/'.join(that[j:until])
                    j = until
                elif this[i].literal != that[j]:
                    raise ValidationException(
                        'mismatched literal: \'%s\' != \'%s\'' % (
                            this[i].literal, that[j]))
                else:
                    j += 1
            elif this[i].kind == _BINDING:
                current_var = this[i].literal
        if j != len(that) or j != segment_count:
            raise ValidationException(
                'match error: could not render from the path template: {}'
                .format(path))
        return bindings


# pylint: disable=C0103
# pylint: disable=R0201
class _Parser(object):
    tokens = (
        'FORWARD_SLASH',
        'LEFT_BRACE',
        'RIGHT_BRACE',
        'EQUALS',
        'WILDCARD',
        'PATH_WILDCARD',
        'LITERAL',
    )

    t_FORWARD_SLASH = r'/'
    t_LEFT_BRACE = r'\{'
    t_RIGHT_BRACE = r'\}'
    t_EQUALS = r'='
    t_WILDCARD = r'\*'
    t_PATH_WILDCARD = r'\*\*'
    t_LITERAL = r'[^*=}{\/]+'

    t_ignore = ' \t'

    def __init__(self):
        self.lexer = lex.lex(module=self)
        self.parser = yacc.yacc(module=self, debug=False, write_tables=False)
        self.verb = ''
        self.binding_var_count = 0
        self.segment_count = 0

    def parse(self, data):
        """Returns a list of path template segments parsed from data.

        Args:
            data: A path template string.
        Returns:
            A list of _Segment.
        """
        self.binding_var_count = 0
        self.segment_count = 0

        segments = self.parser.parse(data)
        # Validation step: checks that there are no nested bindings.
        path_wildcard = False
        for segment in segments:
            if segment.kind == _TERMINAL and segment.literal == '**':
                if path_wildcard:
                    raise ValidationException(
                        'validation error: path template cannot contain more '
                        'than one path wildcard')
                path_wildcard = True
        if segments and segments[-1].kind == _TERMINAL:
            final_term = segments[-1].literal
            last_colon_pos = final_term.rfind(':')
            if last_colon_pos != -1:
                self.verb = final_term[last_colon_pos + 1:]
                segments[-1] = _Segment(_TERMINAL, final_term[:last_colon_pos])

        return segments

    def p_template(self, p):
        """template : FORWARD_SLASH bound_segments
                    | bound_segments"""
        # ply fails on a negative index.
        p[0] = p[len(p) - 1]

    def p_bound_segments(self, p):
        """bound_segments : bound_segment FORWARD_SLASH bound_segments
                          | bound_segment"""
        p[0] = p[1]
        if len(p) > 2:
            p[0].extend(p[3])

    def p_unbound_segments(self, p):
        """unbound_segments : unbound_terminal FORWARD_SLASH unbound_segments
                            | unbound_terminal"""
        p[0] = p[1]
        if len(p) > 2:
            p[0].extend(p[3])

    def p_bound_segment(self, p):
        """bound_segment : bound_terminal
                         | variable"""
        p[0] = p[1]

    def p_unbound_terminal(self, p):
        """unbound_terminal : WILDCARD
                            | PATH_WILDCARD
                            | LITERAL"""
        p[0] = [_Segment(_TERMINAL, p[1])]
        self.segment_count += 1

    def p_bound_terminal(self, p):
        """bound_terminal : unbound_terminal"""
        if p[1][0].literal in ['*', '**']:
            p[0] = [_Segment(_BINDING, '$%d' % self.binding_var_count),
                    p[1][0],
                    _Segment(_END_BINDING, '')]
            self.binding_var_count += 1
        else:
            p[0] = p[1]

    def p_variable(self, p):
        """variable : LEFT_BRACE LITERAL EQUALS unbound_segments RIGHT_BRACE
                    | LEFT_BRACE LITERAL RIGHT_BRACE"""
        p[0] = [_Segment(_BINDING, p[2])]
        if len(p) > 4:
            p[0].extend(p[4])
        else:
            p[0].append(_Segment(_TERMINAL, '*'))
            self.segment_count += 1
        p[0].append(_Segment(_END_BINDING, ''))

    def p_error(self, p):
        """Raises a parser error."""
        if p:
            raise ValidationException(
                'parser error: unexpected token \'%s\'' % p.type)
        else:
            raise ValidationException('parser error: unexpected EOF')

    def t_error(self, t):
        """Raises a lexer error."""
        raise ValidationException(
            'lexer error: illegal character \'%s\'' % t.value[0])
