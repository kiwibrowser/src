#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A simple recursive-descent parser for the table file format.

The grammar implemented here is roughly (taking some liberties with whitespace
and comment parsing):

table_file ::= ( BLANK_LINE | table_def ) end_of_file ;
table_def ::= "--" IDENT CITATION NL
    table_header
    ( table_row )+ ;
table_header ::= ( IDENT "(" BITRANGE ")" )+ ;
table_row ::= ( PATTERN )+ ACTION ;

IDENT = /[a-z0-9_]+/
CITATION = "(" /[^)]+/ ")"
BITRANGE = /[0-9]+/ (":" /[0-9]+/)?
PATTERN = /[10x_]+/
ACTION = ( "=" IDENT | "->" IDENT ) ( "(" IDENT ")" )?
NL = a newline
BLANK_LINE = what you might expect it to be
"""

import re
import dgen_core

# These globals track the parser state.
_in = None
_line_no = None
_tables = None
_line = None
_last_row = None


def parse_tables(input):
    """Entry point for the parser.  Input should be a file or file-like."""
    global _in, _line_no, _tables
    _in = input
    _line_no = 0
    _tables = []
    next_line()

    while not end_of_file():
        blank_line() or table_def() or unexpected()

    return _tables


def blank_line():
    if _line:
        return False

    next_line()
    return True


def table_def():
    global _last_row

    m = re.match(r'^-- ([^ ]+) \(([^)]+)\)', _line)
    if not m: return False

    table = dgen_core.Table(m.group(1), m.group(2))
    next_line()
    while blank_line(): pass

    table_header(table)
    _last_row = None
    while not end_of_file() and not blank_line():
        table_row(table)

    _tables.append(table)
    return True


def table_header(table):
    for col in _line.split():
        m = re.match(r'^([a-z0-9_]+)\(([0-9]+)(:([0-9]+))?\)$', col, re.I)
        if not m: raise Exception('Invalid column header: %s' % col)

        hi_bit = int(m.group(2))
        if m.group(4):
            lo_bit = int(m.group(4))
        else:
            lo_bit = hi_bit
        table.add_column(m.group(1), hi_bit, lo_bit)
    next_line()


def table_row(table):
    global _last_row

    row = _line.split()
    for i in range(0, len(row)):
        if row[i] == '"': row[i] = _last_row[i]
    _last_row = row

    action = row[-1]
    patterns = row[:-1]
    table.add_row(patterns, action)
    next_line()


def end_of_file():
    return _line is None


def next_line():
    global _line_no, _line

    _line_no += 1
    _line = _in.readline()
    if _line:
        _line = re.sub(r'#.*', '', _line).strip()
    else:
        _line = None


def unexpected():
    raise Exception('Line %d: Unexpected line in input: %s' % (_line_no, _line))
