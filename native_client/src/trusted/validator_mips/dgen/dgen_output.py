#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2012, Google Inc.
#

"""
Responsible for generating the decoder based on parsed table representations.
"""

import dgen_opt

def generate_decoder(tables, out):
    """Entry point to the decoder.

    Args:
        tables: list of Table objects to process.
        out: a COutput object to write to.
    """
    if len(tables) == 0: raise Exception('No tables provided.')

    _generate_header(out)
    out.line()
    out.line('namespace nacl_mips_dec {')
    out.line()
    _generate_decoder_state_type(tables, out)
    out.line()
    _generate_prototypes(tables, out)
    out.line()
    _generate_implementations(tables, out)
    out.line()
    _generate_init_function(out)
    out.line()
    _generate_entry_point(tables[0].name, out)
    out.line()
    out.line('}  // namespace nacl_mips_dec')

def _generate_header(out):
    out.block_comment(
        'Copyright (c) 2013 The Native Client Authors. All rights reserved.',
        'Use of this source code is governed by a BSD-style license that can '
        'be',
        'found in the LICENSE file.'
    )
    out.line()
    out.line('// DO NOT EDIT: GENERATED CODE')
    out.line()
    out.line('#include <stdio.h>')
    out.line('#include "native_client/src/trusted/validator_mips/decode.h"')


def _generate_decoder_state_type(tables, out):
    out.block_comment(
        'This beast holds a bunch of pre-created ClassDecoder instances, which',
        'we create in init_decode().  Because ClassDecoders are stateless, we',
        'can freely reuse them -- even across threads -- and avoid allocating',
        'in the inner decoder loop.'
    )
    terminals = set()
    for t in tables:
        for r in t.rows:
            if r.action.startswith('='):
                terminals.add(r.action[1:])

    out.enter_block('struct DecoderState')

    for t in terminals:
        out.line('const %s _%s_instance;' % (t, t))

    out.line('DecoderState() :')
    first = True
    for t in terminals:
        if first:
            out.line('_%s_instance()' % t)
        else:
            out.line(', _%s_instance()' % t)
        first = False
    out.line('{}')

    out.exit_block(';')


def _generate_prototypes(tables, out):
    out.block_comment('Prototypes for static table-matching functions.')
    for t in tables:
        out.line('static inline const ClassDecoder')
        out.line('  &decode_%s(const Instruction insn, '
            'const DecoderState *state);' % t.name)

def _generate_implementations(tables, out):
    out.block_comment('Table-matching function implementations.')
    for t in tables:
        out.line()
        _generate_table(t, out)


def _generate_init_function(out):
    out.enter_block('const DecoderState *init_decode()')
    out.line('return new DecoderState;')
    out.exit_block()

    out.enter_block('void delete_state(const DecoderState *state)')
    out.line('delete state;')
    out.exit_block()

def _generate_entry_point(initial_table_name, out):
    out.enter_block('const ClassDecoder &decode(const Instruction insn, '
        'const DecoderState *state)')
    out.line('return decode_%s(insn, state);'
        % initial_table_name)
    out.exit_block()


def _generate_table(table, out):
    """Generates the implementation of a single table."""
    out.block_comment(
        'Implementation of table %s.' % table.name,
        'Specified by: %s.' % table.citation
    )
    out.line('static inline const ClassDecoder')
    out.enter_block('&decode_%s('
        'const Instruction insn, const DecoderState *state)' % table.name)

    optimized = dgen_opt.optimize_rows(table.rows)
    print ("Table %s: %d rows minimized to %d"
        % (table.name, len(table.rows), len(optimized)))
    for row in sorted(optimized):
        exprs = ["(%s)" % p.to_c_expr('insn') for p in row.patterns]
        if len(exprs) == 1:
            out.enter_block('if (%s)' % exprs[0])
        else:
            for ln in xrange(len(exprs)):
                if ln == 0:
                    out.line('if (%s' % exprs[ln])
                elif ln < len(exprs) - 1:
                    out.line('    && %s' % exprs[ln])
                else:
                    out.enter_block('    && %s)' % exprs[ln])

        if row.action.startswith('='):
            _generate_terminal(row.action[1:], out)
        elif row.action.startswith('->'):
            _generate_table_change(row.action[2:], out)
        else:
            raise Exception('Bad table action: %s' % row.action)

        out.exit_block()
        out.line()

    _generate_safety_net(table, out)
    out.exit_block()


def _generate_terminal(name, out):
    out.line('return state->_%s_instance;' % name)


def _generate_table_change(name, out):
    out.line('return decode_%s(insn, state);' % name)


def _generate_safety_net(table, out):
    out.line('// Catch any attempt to fall through...')
    out.line('fprintf(stderr, "TABLE IS INCOMPLETE: "')
    out.line('        "%s could not parse %%08X", insn.Bits(31, 0));'
        % table.name)
    _generate_terminal('Forbidden', out)


class COutput(object):
    """Provides nicely-formatted C++ output."""

    def __init__(self, out):
        self._out = out
        self._indent = 0

    def line(self, str = ''):
        if len(str) > 0:
            self._out.write(self._tabs())
        self._out.write(str + '\n')

    def enter_block(self, headline):
        self.line(headline + ' {')
        self._indent += 1

    def exit_block(self, footer = ''):
        self._indent -= 1
        self.line('}' + footer)

    def block_comment(self, *lines):
        self.line('/*')
        for s in lines:
            self.line(' * ' + s)
        self.line(' */')

    def _tabs(self):
        return '  ' * self._indent


def each_index_pair(sequence):
    """Utility method: Generates each unique index pair in sequence."""
    for i in range(0, len(sequence)):
        for j in range(i + 1, len(sequence)):
            yield (i, j)


