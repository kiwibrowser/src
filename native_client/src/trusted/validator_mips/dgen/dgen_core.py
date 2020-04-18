#!/usr/bin/python
#
# Copyright 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2012, Google Inc.
#

"""
The core object model for the Decoder Generator.  The dg_input and dg_output
modules both operate in terms of these classes.
"""

import re

def _popcount(int):
    """Returns the number of 1 bits in the input."""
    count = 0
    for bit in range(0, 32):
        count = count + ((int >> bit) & 1)
    return count


class BitPattern(object):
    """A pattern for matching strings of bits.  See parse() for syntax."""

    @staticmethod
    def parse(pattern, hi_bit, lo_bit):
        """ Parses a string pattern describing some bits.  The string can
        consist of '1' and '0' to match bits explicitly, 'x' or 'X' to ignore
        bits, '_' as an ignored separator, and an optional leading '~' to
        negate the entire pattern.  Examples:
          10xx0
          1111_xxxx
          ~111x

        The pattern may also optionally be '-', which is equivalent to a
        sequence of 'xxx...xxx' of the requested width.

        Args:
            pattern: a string in the format described above.
            hi_bit: the top of the range matched by this pattern, inclusive.
            lo_bit: the bottom of the range matched by this pattern, inclusive.
        Returns:
            A BitPattern instance that describes the match, and is capable of
            transforming itself to a C expression.
        Raises:
            Exception: the input didn't meet the rules described above.
        """
        num_bits = hi_bit - lo_bit + 1
        # Convert - into a full-width don't-care pattern.
        if pattern == '-':
            return BitPattern.parse('x' * num_bits, hi_bit, lo_bit)

        # Derive the operation type from the presence of a leading tilde.
        if pattern.startswith('~'):
            op = '!='
            pattern = pattern[1:]
        else:
            op = '=='

        # Allow use of underscores anywhere in the pattern, as a separator.
        pattern = pattern.replace('_', '')

        if len(pattern) != num_bits:
            raise Exception('Pattern %s is wrong length for %d:%u'
                % (pattern, hi_bit, lo_bit))

        mask = 0
        value = 0
        for c in pattern:
            if c == '1':
                mask = (mask << 1) | 1
                value = (value << 1) | 1
            elif c == '0':
                mask = (mask << 1) | 1
                value = value << 1
            elif c == 'x' or c == 'X':
                mask = mask << 1
                value = value << 1
            else:
                raise Exception('Invalid characters in pattern %s' % pattern)

        mask = mask << lo_bit
        value = value << lo_bit
        return BitPattern(mask, value, op)

    def __init__(self, mask, value, op):
        """Initializes a BitPattern.

        Args:
            mask: an integer with 1s in the bit positions we care about (e.g.
                those that are not X)
            value: an integer that would match our pattern, subject to the mask.
            op: either '==' or '!=', if the pattern is positive or negative,
                respectively.
        """
        self.mask = mask
        self.value = value
        self.op = op
        self.significant_bits = _popcount(mask)

    def conflicts(self, other):
        """Returns an integer with a 1 in each bit position that conflicts
        between the two patterns, and 0s elsewhere.  Note that this is only
        useful if the masks and ops match.
        """
        return (self.mask & self.value) ^ (other.mask & other.value)

    def is_complement(self, other):
        """Checks if two patterns are complements of each other.  This means
        they have the same mask and pattern bits, but one is negative.
        """
        return (self.op != other.op
            and self.mask == other.mask
            and self.value == other.value)

    def is_strictly_compatible(self, other):
        """Checks if two patterns are safe to merge using +, but are not ==."""
        if self.is_complement(other):
            return True
        elif self.op == other.op:
            return (self.mask == other.mask
                and _popcount(self.conflicts(other)) == 1)
        return False

    def __add__(self, other):
        """Merges two compatible patterns into a single pattern that matches
        everything either pattern would have matched.
        """
        assert (self == other) or self.is_strictly_compatible(other)

        if self.op == other.op:
            c = self.conflicts(other)
            return BitPattern((self.mask | other.mask) ^ c,
                (self.value | other.value) ^ c, self.op)
        else:
            return BitPattern(0, 0, '==')  # matches anything

    def to_c_expr(self, input):
        """Converts this pattern to a C expression.
        Args:
            input: the name (string) of the C variable to be tested by the
                expression.
        Returns:
            A string containing a C expression.
        """
        if self.mask == 0:
            return 'true'
        else:
            return ('(%s & 0x%08X) %s 0x%08X'
                % (input, self.mask, self.op, self.value))

    def __cmp__(self, other):
        """Compares two patterns for sorting purposes.  We sort by
        - # of significant bits, DESCENDING,
        - then mask value, numerically,
        - then value, numerically,
        - and finally op.

        This is also used for equality comparison using ==.
        """
        return (cmp(other.significant_bits, self.significant_bits)
            or cmp(self.mask, other.mask)
            or cmp(self.value, other.value)
            or cmp(self.op, other.op))

    def __repr__(self):
        pat = []
        for i in range(0, 32):
            if (self.mask >> i) & 1:
                pat.append(`(self.value >> i) & 1`)
            else:
                pat.append('x')
        if self.op == '!=':
            pat.append('~')
        return ''.join(reversed(pat))


class Table(object):
    """A table in the instruction set definition.  Each table contains 1+
    columns, and 1+ rows.  Each row contains a bit pattern for each column, plus
    the action to be taken if the row matches."""

    def __init__(self, name, citation):
        """Initializes a new Table.
        Args:
            name: a name for the table, used to reference it from other tables.
            citation: the section in the ISA spec this table was derived from.
        """
        self.name = name
        self.citation = citation
        self.rows = []
        self._columns = []

    def add_column(self, name, hi_bit, lo_bit):
        """Adds a column to the table.

        Because we don't use the column information for very much, we don't give
        it a type -- we store it as a list of tuples.

        Args:
            name: the name of the column (for diagnostic purposes only).
            hi_bit: the leftmost bit included.
            lo_bit: the rightmost bit included.
        """
        self._columns.append( (name, hi_bit, lo_bit) )

    def add_row(self, col_patterns, action_string):
        """Adds a row to the table.
        Args:
            col_patterns: a list containing a BitPattern for every column in the
                table.
            action_string: a string indicating the action to take; must begin
                with '=' for a terminal instruction class, or '->' for a
                table-change.  The action may end with an arch revision in
                parentheses.
        """
        arch = None
        m = re.match(r'^(=[A-Za-z0-9_]+)\(([^)]+)\)$', action_string)
        if m:
            action_string = m.group(1)
            arch = m.group(2)

        parsed = []
        for i in range(0, len(col_patterns)):
            col = self._columns[i]
            parsed.append(BitPattern.parse(col_patterns[i], col[1], col[2]))
        self.rows.append(Row(parsed, action_string, arch))


class Row(object):
    """ A row in a Table."""
    def __init__(self, patterns, action, arch):
        """Initializes a Row.
        Args:
            patterns: a list of BitPatterns that must match for this Row to
                match.
            action: the action to be taken if this Row matches.
            arch: the minimum architecture that this Row can match.
        """
        self.patterns = patterns
        self.action = action
        self.arch = arch

        self.significant_bits = 0
        for p in patterns:
            self.significant_bits += p.significant_bits

    def can_merge(self, other):
        """Determines if we can merge two Rows."""
        if self.action != other.action or self.arch != other.arch:
            return False

        equal_columns = 0
        compat_columns = 0
        for (a, b) in zip(self.patterns, other.patterns):
            if a == b:
                equal_columns += 1
            if a.is_strictly_compatible(b):
                compat_columns += 1

        cols = len(self.patterns)
        return (equal_columns == cols
            or (equal_columns == cols - 1 and compat_columns == 1))

    def __add__(self, other):
        assert self.can_merge(other)  # Caller is expected to check!
        return Row([a + b for (a, b) in zip(self.patterns, other.patterns)],
            self.action, self.arch)

    def __cmp__(self, other):
        """Compares two rows, so we can order pattern matches by specificity.
        """
        return (cmp(self.patterns, other.patterns)
            or cmp(self.action, other.action))

    def __repr__(self):
        return 'Row(%s, %s)' % (repr(self.patterns), repr(self.action))
