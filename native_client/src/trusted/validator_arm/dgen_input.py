#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
A simple recursive-descent parser for the table file format.

The grammar implemented here is roughly (taking some liberties with whitespace
and comment parsing):

table_file ::= decoder_actions? table+ eof ;

action         ::= decoder_action | decoder_method | '"'
action_arch    ::= 'arch' ':=' id | '(' id (',' id)* ')'
action_option  ::= (action_rule |
                    action_pattern |
                    action_safety |
                    action_arch |
                    action_violations |
                    action_other) ';'
action_other   ::= word ':=' bit_expr
action_pattern ::= 'pattern' ':=' bitpattern
action_safety  ::= 'safety' ':=' ('super.safety' | safety_check)
                            ('&' safety_check)*
action_rule    ::= 'rule' ':=' id
action_violations ::= 'violations' ':=' violation ('&' violation)*
arch           ::= '(' word+ ')'
bit_expr       ::= bit_expr1 ('if' bit_expr 'else' bit_expr)?  # conditional
bit_expr1      ::= bit_expr2 (('&' bit_expr2)* | ('|' bit_expr2)*)?
bit_expr2      ::= bit_expr3 | 'not' bit_expr2
bit_expr3      ::= bit_expr4 ('in' bit_set | 'in' 'bitset' pat_bit_set)?
bit_expr4      ::= bit_expr5 (('<' | '<=' | '==' | '!=' | '>=' | '>')
                              bit_expr5)?                      # comparisons
bit_expr5      ::= bit_expr6 |
                   bit_expr5 ('<<' | '>>') bit_expr6           # shift
bit_expr6      ::= bit_expr7 |                                 # add ops
                   bit_expr6 ('+' | '-") bit_expr7
bit_expr7      ::= bit_expr8 |                                 # mul ops
                   bit_expr7 ('*'| '/' | 'mod') bit_expr8
bit_expr8      ::= bit_expr9 ('=' bitpattern)?                 # bit check
bit_expr9      ::= bit_expr10 (':' bit_expr10)*                # concat
bit_expr10     ::= bit_expr11 | bit_expr10 '(' int (':' int)? ')' # bit range
bit_expr11     ::= int | nondecimal_int | id | bit_set | '(' bit_expr ')' | call
bit_set        ::= '{' (bit_expr (',' bit_expr)*)? '}'
bitpattern     ::= word | negated_word
call           ::= word '(' (bit_expr (',' bit_expr)*)? ')'
citation       ::= '(' word+ ')'
column         ::= id '(' int (':' int)? ')'
decoder_action ::= '=" decoder_defn
decoder_actions::= ('*' (int | id) decoder_defn)+
decoder_defn   ::= fields? action_option*
                   | '*' (int | id) ('-' field_names)? fields? action_option*
decoder_method ::= '->' id
default_row    ::= 'else' ':' action
field_names    ::= '{' (id (',' id)*)? '}'
fields         ::= '{' column (',' column)* '}'
footer         ::= '+' '-' '-'
header         ::= "|" column+
int            ::= word     (where word is a sequence of digits)
id             ::= word     (where word is sequence of letters, digits and _)
negated_word   ::= '~' word
nondecimal_int ::= word     (where word is a hexidecimal or bitstring pattern)
parenthesized_exp ::= '(' (word | punctuation)+ ')'
pat_bit_set        ::= '{' (bitpattern (',' bitpattern)*)? '}'
pat_row        ::= pattern+ action
pattern        ::= bitpattern | '-' | '"'
quoted_string  := word      (where word text enclosed in quotes('))
row            ::= '|' (pat_row | default_row)
safety_check   ::= bit_expr '=>' id
table          ::= table_desc table_actions header row+ footer
table_actions  ::= (decoder_actions footer)?
table_desc     ::= '+' '-' '-' id citation?
violation      ::= bit_expr '=>' 'error' '(' quoted_string (',' bit_expr)* ')'
"""

import re
import dgen_core
# The following import adds type information for decoder actions corresponding
# to method definitions.
import dgen_decoder

# Set the following to True if you want to see each read/pushback of a token.
_TRACE_TOKENS = False

def parse_tables(input):
  """Entry point for the parser.  Input should be a file or file-like."""
  parser = Parser()
  return parser.parse(input)

class Token(object):
  """Holds a (characterized) unit of text for the parser."""

  def __init__(self, kind, value=None):
    self.kind = kind
    self.value = value if value else kind

  def __repr__(self):
    return 'Token(%s, "%s")' % (self.kind, self.value)

# Predefined names corresponding to predefined bit expressions.
_PREDEFINED_CONSTS = {
    # Register Names.
    # TODO(karl): These constants are arm32 specific. We will need to
    # fix this when implementing arm64.
    'NZCV': dgen_core.Literal(16),  # defines conditions registers.
    'None': dgen_core.Literal(32),
    'Pc': dgen_core.Literal(15),
    'Lr': dgen_core.Literal(14),
    'Sp': dgen_core.Literal(13),
    'Tp': dgen_core.Literal(9),
    # Boolean values.
    'true': dgen_core.BoolValue(True),
    'false': dgen_core.BoolValue(False),
    'inst': dgen_core.Instruction(),
    # Instruction value selectors used in ARM tables. Numbered sequentially
    # to guarantee selectors are unique.
    'VFPNegMul_VNMLA': dgen_core.Literal(1),
    'VFPNegMul_VNMLS': dgen_core.Literal(2),
    'VFPNegMul_VNMUL': dgen_core.Literal(3),
    # conditions for conditional instructions (see enum Condition in model.h).
    # Possible values for the condition field, from the ARM ARM section A8.3.
    'cond_EQ': dgen_core.Literal(0x0),
    'cond_NE': dgen_core.Literal(0x1),
    'cond_CS': dgen_core.Literal(0x2),
    'cond_CC': dgen_core.Literal(0x3),
    'cond_MI': dgen_core.Literal(0x4),
    'cond_PL': dgen_core.Literal(0x5),
    'cond_VS': dgen_core.Literal(0x6),
    'cond_VC': dgen_core.Literal(0x7),
    'cond_HI': dgen_core.Literal(0x8),
    'cond_LS': dgen_core.Literal(0x9),
    'cond_GE': dgen_core.Literal(0xA),
    'cond_LT': dgen_core.Literal(0xB),
    'cond_GT': dgen_core.Literal(0xC),
    'cond_LE': dgen_core.Literal(0xD),
    'cond_AL': dgen_core.Literal(0xE),
    'cond_unconditional': dgen_core.Literal(0xF),
    'cond_HS': dgen_core.Literal(0x2),
    'cond_LO': dgen_core.Literal(0x3),
    }

# Predefined regular expressions.
_DECIMAL_PATTERN = re.compile(r'^([0-9]+)$')
_HEXIDECIMAL_PATTERN = re.compile(r'^0x([0-9a-fA-F]+)$')
_BITSTRING_PATTERN = re.compile(r'^\'([01]+)\'$')
_ID_PATTERN = re.compile(r'^[a-zA-z][a-zA-z0-9_]*$')
_STRING_PATTERN = re.compile(r'^\'(.*)\'$')

# When true, catch all bugs when parsing and report line.
_CATCH_EXCEPTIONS = True

# List of file level decoder actions that must be specified in every
# specification file, because they are used somewhere else than in table rows.
_REQUIRED_FILE_DECODER_ACTIONS = [
    # Defiles the decoder action that handles instructions that are
    # not defined by rows in the instruction tables.
    'NotImplemented',
    # Defines the decoder action that handles the fictitious instruction
    # inserted before the code segment, acting as the previous instruction
    # for the first instruction in the bundle.
    'FictitiousFirst'
    ]

class Parser(object):
  """Parses a set of tables from the input file."""

  def parse(self, input):
    self.input = input             # The remaining input to parse
    decoder = dgen_core.Decoder()  # The generated decoder of parse tables.
    if _CATCH_EXCEPTIONS:
      try:
        return self._parse(decoder)
      except Exception as e:
        self._unexpected(e)
      except:
        self._unexpected("Unknow problem.")
    else:
      return self._parse(decoder)

  def _parse(self, decoder):
    # Read global decoder actions.
    self._global_decoder_actions(decoder)

    # Read tables while there are tables to read.
    while self._next_token().kind == '+':
      self._table(decoder)

    # Check that we read everything.
    if not self._next_token().kind == 'eof':
      self._unexpected('unrecognized input found')
    if not decoder.primary:
      self._unexpected('No primary table defined')
    if not decoder.tables():
      self._unexpected('No tables defined')
    return decoder

  def __init__(self):
    self._words = []             # Words left on current line, not yet parsed.
    self._line_no = 0            # The current line being parsed
    self._token = None           # The next token from the input.
    self._reached_eof = False    # True when end of file reached
    self._pushed_tokens = []     # Tokens pushed back onto the input stream.
    # Reserved words allowed. Must be ordered such that if p1 != p2 are in
    # the list, and p1.startswith(p2), then p1 must appear before p2.
    self._reserved = ['else', 'other', 'mod', 'if', 'not', 'in']
    # Punctuation allowed. Must be ordered such that if p1 != p2 are in
    # the list, and p1.startswith(p2), then p1 must appear before p2.
    self._punctuation = ['=>', '->', '-', '+', '(', ')', '==', ':=', '"',
                         '|', '~', '&', '{', '}', ',', ';', '!=',':',
                         '>>', '<<', '>=', '>', '<=', '<', '=', '*', '/']
    # Holds global decoder actions, that can be used in tables if not defined
    # locally.
    self._file_actions = {}

  #-------- Recursive descent parse functions corresponding to grammar above.

  def _action(self, starred_actions, last_action):
    """ action ::= decoder_action | decoder_method | '"' """
    if self._next_token().kind == '"':
      self._read_token('"')
      return last_action
    if self._next_token().kind == '=':
      return self._decoder_action(starred_actions)
    elif self._next_token().kind == '->':
      return self._decoder_method()
    else:
      self._unexpected("Row doesn't define an action")

  def _action_arch(self, context):
    """action_arch    ::= 'arch' ':=' id | '(' id (',' id)* ')'

       Adds architecture to context."""
    self._read_keyword('arch')
    self._read_token(':=')
    if self._next_token().kind == '(':
      self._read_token('(')
      names = [ self._id() ]
      while self._next_token().kind == ',':
        self._read_token(',')
        names.append(self._id())
      self._read_token(')')
      self._define('arch', names, context)
    else:
      self._define('arch', self._id(), context)

  def _action_option(self, context):
    """action_option  ::= (action_rule | action_pattern |
                           action_safety | action_arch |
                           action_violation | action_other) ';'

       Returns the specified architecture, or None if other option.
       """
    if self._is_keyword('rule'):
      self._action_rule(context)
    elif self._is_keyword('pattern'):
      self._action_pattern(context)
    elif self._is_keyword('safety'):
      self._action_safety(context)
    elif self._is_keyword('arch'):
      self._action_arch(context)
    elif self._is_keyword('violations'):
      self._action_violations(context)
    elif self._next_token().kind == 'word':
      self._action_other(context)
    else:
      self._unexpected("Expected action option but not found")
    self._read_token(';')

  def _action_other(self, context):
    """action_other   ::= 'word' ':=' bit_expr

       Recognizes other actions not currently implemented.
       """
    name = self._read_token('word').value
    self._read_token(':=')
    self._define(name, self._bit_expr(context), context)

  def _action_pattern(self, context):
    """action_pattern ::= 'pattern' ':=' bitpattern

       Adds pattern/parse constraints to the context.
       """
    self._read_keyword('pattern')
    self._read_token(':=')
    self._define('pattern', self._bitpattern32(), context)

  def _action_safety(self, context):
    """action_safety  ::=
          'safety' ':=' ('super.safety' | safety_check) ('&' safety_check)*

       Adds safety constraints to the context.
       """
    self._read_keyword('safety')
    self._read_token(':=')
    if self._is_keyword('super.safety'):
      # Treat as extending case of inherited safety.
      self._read_keyword('super.safety')
      checks = context.find('safety', install_inheriting=False)
      if isinstance(checks, list):
        checks = list(checks)
      else:
        self._unexpected('safety extensions not allowed, nothing to extend')
    else:
      checks = [ self._safety_check(context) ]
    while self._next_token().kind == '&':
      self._read_token('&')
      checks.append(self._safety_check(context))
    self._define('safety', checks, context)

  def _action_rule(self, context):
    """action_rule    ::= 'rule' ':=' id

       Adds rule name to the context.
       """
    self._read_keyword('rule')
    self._read_token(':=')
    self._define('rule', self._id(), context)

  def _action_violations(self, context):
    """action_violations ::= 'violations' ':=' violation ('&' violation)*"""
    self._read_keyword('violations')
    self._read_token(':=')
    violations = [ self._violation(context) ]
    while self._next_token().kind == '&':
      self._read_token('&')
      violations.append(self._violation(context))
    self._define('violations', violations, context)

  def _arch(self):
    """ arch ::= '(' word+ ')' """
    return ' '.join(self._parenthesized_exp())

  def _bit_check(self, context):
    """ bit_check      ::= column '=' bitpattern """
    column = None
    if self._is_column():
      column = self._column()
      self._read_token('=')
    elif self._is_name_equals():
      name = self._id()
      column = context.find(name)
      if not column:
        self._unexpected("Can't find column definition for %s" % name)
      self._read_token('=')
    else:
      self._unexpected("didn't find bit pattern check")
    pattern = dgen_core.BitPattern.parse(self._bitpattern(), column)
    if not pattern:
      self._unexpected("bit pattern check malformed")
    return pattern

  def _bit_expr(self, context):
    """bit_expr ::= bit_expr1 ('if' bit_expr 'else' bit_expr)?"""
    then_value = self._bit_expr1(context)
    if self._next_token().kind != 'if': return then_value
    self._read_token('if')
    test = self._bit_expr(context)
    self._read_token('else')
    else_value = self._bit_expr(context)
    return dgen_core.IfThenElse(test, then_value, else_value)

  def _bit_expr1(self, context):
    """bit_expr1 ::= bit_expr2 (('&' bit_expr2)* | ('|' bit_expr2)*)"""
    value = self._bit_expr2(context)
    if self._next_token().kind == '&':
      args = [value]
      while self._next_token().kind == '&':
        self._read_token('&')
        args.append(self._bit_expr2(context))
      value = dgen_core.AndExp(args)
    elif self._next_token().kind == '|':
      args = [value]
      while self._next_token().kind == '|':
        self._read_token('|')
        args.append(self._bit_expr2(context))
      value = dgen_core.OrExp(args)
    return value

  def _bit_expr2(self, context):
    """bit_expr2 ::= bit_expr3 | 'not' bit_expr2"""
    if self._next_token().kind == 'not':
      self._read_token('not')
      return dgen_core.NegatedTest(self._bit_expr2(context))
    return self._bit_expr3(context)

  def _bit_expr3(self, context):
    """bit_expr3 ::= bit_expr4 ('in' bit_set | 'in' 'bitset' pat_bit_set)?"""
    value = self._bit_expr4(context)
    if not self._next_token().kind == 'in': return value
    self._read_token('in')
    if self._is_keyword('bitset'):
      self._read_keyword('bitset')
      return dgen_core.InBitSet(value, self._pat_bit_set())
    else:
      return dgen_core.InUintSet(value, self._bit_set(context))

  def _bit_expr4(self, context):
    """bit_expr4 ::= bit_expr5 (('<' | '<=' | '==' | '!=' | '>=' | '>')
                                bit_expr5)? """
    value = self._bit_expr5(context)
    for op in ['<', '<=', '==', '!=', '>=', '>']:
      if self._next_token().kind == op:
        self._read_token(op)
        return dgen_core.CompareExp(op, value, self._bit_expr5(context))
    return value

  def _bit_expr5(self, context):
    """bit_expr5 ::= bit_expr6 | bit_expr5 ('<<' | '>>') bit_expr6"""
    value = self._bit_expr6(context)
    while self._next_token().kind in ['<<', '>>']:
      op = self._read_token().value
      value = dgen_core.ShiftOp(op, value, self._bit_expr6(context))
    return value

  def _bit_expr6(self, context):
    """bit_expr6 ::= bit_expr7 | bit_expr6 ('+' | '-') bit_expr7"""
    value = self._bit_expr7(context)
    while self._next_token().kind in ['+', '-']:
      op = self._read_token().value
      value = dgen_core.AddOp(op, value, self._bit_expr7(context))
    return value

  def _bit_expr7(self, context):
    """bit_expr7 ::= bit_expr8 |
                     bit_expr7 ('*' | '/' | 'mod') bit_expr8"""
    value = self._bit_expr8(context)
    while self._next_token().kind in ['*', '/', 'mod']:
      op = self._read_token().value
      value = dgen_core.MulOp(op, value, self._bit_expr8(context))
    return value

  def _bit_expr8(self, context):
    """bit_expr8 ::= bit_expr9 ('=' bitpattern)?"""
    bits = self._bit_expr9(context)
    if self._next_token().kind != '=': return bits
    self._read_token('=')
    bitpat = self._bitpattern()
    pattern = dgen_core.BitPattern.parse_catch(bitpat, bits)
    if not pattern:
      self._unexpected('Pattern mismatch in %s = %s' % (bits, bitpat))
    else:
      return pattern

  def _bit_expr9(self, context):
    """bit_expr9 ::= bit_expr10 (':' bit_expr10)*"""
    value = self._bit_expr10(context)
    if self._next_token().kind != ':': return value
    values = [ value ]
    while self._next_token().kind == ':':
      self._read_token(':')
      values.append(self._bit_expr10(context))
    return dgen_core.Concat(values)

  def _bit_expr10(self, context):
    """bit_expr10 ::= bit_expr11 |
                      bit_expr10 '(' int (':' int)? ')'"""
    value = self._bit_expr11(context)
    while self._next_token().kind == '(':
      self._read_token('(')
      hi_bit = self._int()
      lo_bit = hi_bit
      if self._next_token().kind == ':':
        self._read_token(':')
        lo_bit = self._int()
      self._read_token(')')
      value = dgen_core.BitField(value, hi_bit, lo_bit)
    return value

  def _bit_expr11(self, context):
    """bit_expr11 ::= int | nondecimal_int | id | bit_set |
                     '(' bit_expr ')' | call"""
    if self._is_int():
      return dgen_core.Literal(self._int())
    elif self._is_nondecimal_int():
      return self._nondecimal_int()
    elif self._next_token().kind == '{':
      return self._bit_set(context)
    elif self._next_token().kind == '(':
      self._read_token('(')
      value = self._bit_expr(context)
      self._read_token(')')
      return dgen_core.ParenthesizedExp(value)
    elif (self._is_name_paren() and not self._is_column()):
      # Note: we defer input like "foo(2)" to being a (bit field) column.
      # If you want to recognize "foo(2)" as a function call, write 'foo((2))'
      return self._call(context)
    elif self._is_id():
      name = self._id()
      value = context.find(name)
      if not value:
        value = _PREDEFINED_CONSTS.get(name)
        if not value:
          self._unexpected("Can't find definition for %s" % name)
        # Lift predefined symbol into context, so that the high-level
        # definition will be available when the context is printed.
        context.define(name, value)
      return dgen_core.IdRef(name, value)
    else:
      self._unexpected("Don't understand value: %s" % self._next_token().value)

  def _bit_set(self, context):
    """bit_set        ::= '{' (bit_expr (',' bit_expr)*)? '}'"""
    values = []
    self._read_token('{')
    if not self._next_token().kind == '}':
      values.append(self._bit_expr(context))
      while self._next_token().kind == ',':
        self._read_token(',')
        values.append(self._bit_expr(context))
    self._read_token('}')
    return dgen_core.BitSet(values)

  def _bitpattern(self):
    """ bitpattern     ::= 'word' | negated_word """
    return (self._negated_word() if self._next_token().kind == '~'
            else self._read_token('word').value)

  def _bitpattern32(self):
    """Returns a bit pattern with 32 bits."""
    pattern = self._bitpattern()
    if pattern[0] == '~':
      if len(pattern) != 33:
        self._unexpected("Bit pattern  %s length != 32" % pattern)
    elif len(pattern) != 32:
      self._unexpected("Bit pattern  %s length != 32" % pattern)
    return pattern

  def _call(self, context):
    """call ::= word '(' (bit_expr (',' bit_expr)*)? ')'"""
    name = self._read_token('word').value
    args = []
    self._read_token('(')
    while self._next_token().kind != ')':
      if args:
        self._read_token(',')
      args.append(self._bit_expr(context))
    self._read_token(')')
    if len(args) == 1 and name in dgen_core.DGEN_TYPE_TO_CPP_TYPE.keys():
        return dgen_core.TypeCast(name, args[0])
    else:
        return dgen_core.FunctionCall(name, args)

  def _citation(self):
    """ citation ::= '(' word+ ')' """
    return ' '.join(self._parenthesized_exp())

  def _column(self):
    """column         ::= id '(' int (':' int)? ')'

       Reads a column and returns the correspond representation.
    """
    name = self._read_token('word').value
    self._read_token('(')
    hi_bit = self._int()
    lo_bit = hi_bit
    if self._next_token().kind == ':':
      self._read_token(':')
      lo_bit = self._int()
    self._read_token(')')
    return dgen_core.BitField(name, hi_bit, lo_bit)

  def _decoder_action_options(self, context):
    """Parses 'action_options*'."""
    while True:
      if self._is_action_option():
        self._action_option(context)
      else:
        return

  def _decoder_action(self, starred_actions):
    """decoder_action ::= '=" decoder_defn"""
    self._read_token('=')
    action = self._decoder_defn(starred_actions)
    self._check_action_is_well_defined(action)
    return self._decoder_defn_end(action)

  def _decoder_actions(self):
    """ decoder_actions::= ('*' (int | id) decoder_defn)+ """
    starred_actions = {}
    while self._next_token().kind == '*':
      self._read_token('*')
      if self._is_int():
        index = self._int()
      else:
        index = self._id()

      if starred_actions.get(index):
        self._unexpected("Multple *actions defined for %s" % index)

      starred_actions[index] = self._decoder_defn(starred_actions)
    return starred_actions

  def _decoder_defn_end(self, action):
      """Called when at end of a decoder definition. Used to
         turn off type checking."""
      action.force_type_checking(False)
      return action

  def _decoder_defn(self, starred_actions):
    """decoder_defn ::=  fields? action_option*
                         | '*' (int | id) ('-' field_names)?
                               fields? action_option*
    """
    if self._next_token().kind == '*':
      return self._decoder_defn_end(
          self._decoder_action_extend(starred_actions))

    action = dgen_core.DecoderAction()
    action.force_type_checking(True)
    if self._next_token().kind == '{':
      fields = self._fields(action)
      self._define('fields', fields, action)
      self._decoder_action_options(action)
    elif self._is_action_option():
      self._decoder_action_options(action)
    return self._decoder_defn_end(action)

  def _decoder_action_extend(self, starred_actions):
    """'*' (int | id) ('-' field_names)? fields? action_option*

       Helper function to _decoder_action."""
    self._read_token('*')
    if self._is_int():
      index = self._int()
    else:
      index = self._id()
    indexed_action = starred_actions.get(index)
    if not indexed_action:
      indexed_action = self._file_actions.get(index)
      if not indexed_action:
        self._unexpected("Can't find decoder action *%s" % index)

    # Create an initial copy, and define starred action as
    # inheriting definition.
    action = dgen_core.DecoderAction()
    action.force_type_checking(True)

    # Get the set of field names.
    fields = []
    if self._next_token().kind == '-':
      self._read_token('-')
      fields = self._field_names()

    action.inherits(indexed_action, fields)

    # Recognize fields if applicable.
    if self._next_token().kind == '{':
      self._define('fields', self._fields(action), action)

    # Recognize overriding options.
    self._decoder_action_options(action)
    action.disinherit()
    return action

  def _decoder_method(self):
    """ decoder_method ::= '->' id """
    self._read_token('->')
    name = self._id()
    return dgen_core.DecoderMethod(name)

  def _default_row(self, table, starred_actions, last_action):
    """ default_row ::= 'else' ':' action """
    self._read_token('else')
    self._read_token(':')
    action = self._action(starred_actions, last_action)
    self._check_action_is_well_defined(action)
    if not table.add_default_row(action):
      self._unexpected('Unable to install row default')
    return (None, self._decoder_defn_end(action))

  def _field_names(self):
    """'{' (id (',' id)*)? '}'

       Note: To capture predefined actions, we allow special
       action keywords to also apply.
       """
    names = []
    self._read_token('{')
    if self._is_field_name():
      names.append(self._read_token().value)
      while self._next_token().kind == ',':
        self._read_token(',')
        if self._is_field_name():
          name == self._read_token().value
          if name in names:
            raise Exception("Repeated field name: %s" % name)
          names.append(name)
        else:
          raise Exception("field name expected, found %s" %
                          self._next_token().value)
    self._read_token('}')
    return names

  def _field_name_next(self):
    return self._is_id() or self._is_action_option()

  def _fields(self, context):
    """fields         ::= '{' column (',' column)* '}'"""
    fields = []
    self._read_token('{')
    field = self._column()
    fields.append(field)
    self._define(field.name().name(), field, context)
    while self._next_token().kind == ',':
      self._read_token(',')
      field = self._column()
      self._define(field.name().name(), field, context)
      fields.append(field)
    self._read_token('}')
    return fields

  def _footer(self):
    """ footer ::= '+' '-' '-' """
    self._read_token('+')
    self._read_token('-')
    self._read_token('-')

  def _global_decoder_actions(self, decoder):
    """Read in file level decoder actions, and install required predefined
       file decoder actions."""
    self._file_actions = self._decoder_actions()
    for required_action in _REQUIRED_FILE_DECODER_ACTIONS:
      action = self._file_actions.get(required_action)
      if not action:
        self._unexpected("File level action '%s' not defined" % required_action)
      decoder.define_value(required_action, action)

  def _header(self, table):
    """ header ::= "|" column+ """
    self._read_token('|')
    self._add_column(table)
    while self._is_column():
      self._add_column(table)

  def _nondecimal_int(self):
    """nondecimal_int ::= word

       where word is a hexidecimal or bitstring pattern."""
    word = self._read_token('word').value

    match = _HEXIDECIMAL_PATTERN.match(word)
    if match:
      return Literal(int(match.group(1), 16), name=word)

    match = _BITSTRING_PATTERN.match(word)
    if match:
      text = match.group(1)
      l = dgen_core.Literal(int(text, 2), name=word)
      return dgen_core.BitField(l, len(text) - 1, 0)

    self._unexpected('Nondecimal integer expected but not found: %s' % word)

  def _int(self):
    """ int ::= word

    Int is a sequence of digits. Returns the corresponding integer.
    """
    word = self._read_token('word').value
    match = _DECIMAL_PATTERN.match(word)
    if match:
      return int(match.group(1))

    self._unexpected(
        'integer expected but found "%s"' % word)

  def _id(self):
    """ id ::= word

    Word starts with a letter, and followed by letters, digits,
    and underscores. Returns the corresponding identifier.
    """
    if self._is_id():
      return self._read_token('word').value

  def _named_value(self, context):
    """named_value    ::= id ':=' bit_expr."""
    name = self._id()
    self._read_token(':=')
    value = self._bit_expr(context)
    self._define(name, value, context)
    return value

  def _negated_word(self):
    """ negated_word ::= '~' 'word' """
    self._read_token('~')
    return '~' + self._read_token('word').value

  def _parenthesized_exp(self, minlength=1):
    """ parenthesized_exp ::= '(' (word | punctuation)+ ')'

    The punctuation doesn't include ')'.
    Returns the sequence of token values parsed.
    """
    self._read_token('(')
    words = []
    while not self._at_eof() and self._next_token().kind != ')':
      words.append(self._read_token().value)
    if len(words) < minlength:
      self._unexpected("len(parenthesized expresssion) < %s" % minlength)
    self._read_token(')')
    return words

  def _pat_bit_set(self):
    """pat_bit_set ::= '{' (bitpattern (',' bitpattern)*)? '}'"""
    values = []
    self._read_token('{')
    if not self._next_token().kind == '}':
      values.append(self._bitpattern())
      while self._next_token().kind == ',':
        self._read_token(',')
        values.append(self._bitpattern())
    self._read_token('}')
    return dgen_core.BitSet(values)

  def _pat_row(self, table, starred_actions, last_patterns, last_action):
    """ pat_row ::= pattern+ action

    Passed in sequence of patterns and action from last row,
    and returns list of patterns and action from this row.
    """
    patterns = []             # Patterns as found on input.
    expanded_patterns = []    # Patterns after being expanded.
    num_patterns = 0
    num_patterns_last = len(last_patterns) if last_patterns else None
    while self._next_token().kind not in ['=', '->', '|', '+']:
      if not last_patterns or num_patterns < num_patterns_last:
        last_pattern = last_patterns[num_patterns] if last_patterns else None
        pattern = self._pattern(last_pattern)
        patterns.append(pattern)
        expanded_patterns.append(table.define_pattern(pattern, num_patterns))
        num_patterns += 1
      else:
        # Processed patterns in this row, since width is now the
        # same as last row.
        break

    action = self._action(starred_actions, last_action)
    table.add_row(expanded_patterns, action)
    return (patterns, action)

  def _pattern(self, last_pattern):
    """ pattern        ::= bitpattern | '-' | '"'

    Arguments are:
      last_pattern: The pattern for this column from the previous line.
    """
    if self._next_token().kind == '"':
      self._read_token('"')
      return last_pattern
    if self._next_token().kind in ['-', 'word']:
      return self._read_token().value
    return self._bitpattern()

  def _quoted_string(self):
    """quoted_string := word

       where word is text enclosed in quotes(')."""
    word = self._read_token('word').value

    match = _STRING_PATTERN.match(word)
    if match:
      text = match.group(1)
      return dgen_core.QuotedString(text, name=word)

    self._unexpected('Quoted string expected but not found: "%s"' % word)

  def _row(self, table, starred_actions, last_patterns=None, last_action=None):
    """ row ::= '|' (pat_row | default_row)

    Passed in sequence of patterns and action from last row,
    and returns list of patterns and action from this row.
    """
    self._read_token('|')
    if self._next_token().kind == 'else':
      return self._default_row(table, starred_actions, last_action)
    else:
      return self._pat_row(table, starred_actions, last_patterns, last_action)

  def _read_keyword(self, keyword):
    """Returns true if the next symbol matches the (identifier) keyword."""
    token = self._read_token()
    if not ((token.kind == keyword) or
            (token.kind == 'word' and token.value == keyword)):
      self._unexpected("Expected '%s' but found '%s'" % (keyword, token.value))

  def _safety_check(self, context):
    """safety_check   ::= bit_expr '=>' id

       Parses safety check and returns it.
       """
    check = self._bit_expr(context)
    self._read_token('=>')
    name = self._id()
    return dgen_core.SafetyAction(check, name)

  def _table(self, decoder):
    """table ::= table_desc table_actions header row+ footer"""
    table = self._table_desc()
    starred_actions = self._table_actions()
    self._header(table)
    (pattern, action) = self._row(table, starred_actions)
    while not self._next_token().kind == '+':
      (pattern, action) = self._row(table, starred_actions, pattern, action)
    if not decoder.add(table):
      self._unexpected('Multiple tables with name %s' % table.name)
    self._footer()

  def _table_actions(self):
    """table_actions  ::= ( ('*' (int | id) decoder_defn)+ footer)?"""
    starred_actions = {}
    if self._next_token().kind != '*': return starred_actions
    starred_actions = self._decoder_actions()
    self._footer()
    return starred_actions

  def _table_desc(self):
    """ table_desc ::= '+' '-' '-' id citation? """
    self._read_token('+')
    self._read_token('-')
    self._read_token('-')
    name = self._id()
    citation = None
    if self._next_token().kind == '(':
      citation = self._citation()
    return dgen_core.Table(name, citation)

  def _violation(self, context):
    """violation ::= bit_expr '=>' 'error' '(' quoted_string (',' bit_expr)* ')'

       Parses a (conditional) violation and returns it."""
    check = self._bit_expr(context)
    self._read_token('=>')
    self._read_keyword('error')
    self._read_token('(')
    args = [ self._quoted_string() ]
    while self._next_token().kind == ',':
      self._read_token(',')
      args.append(self._bit_expr(context))
    self._read_token(')')
    return dgen_core.Violation(check, args)

  #------ Syntax checks ------

  def _at_eof(self):
    """Returns true if next token is the eof token."""
    return self._next_token().kind == 'eof'

  def _is_action_option(self):
    """Returns true if the input matches an action_option.

       Note: We assume that checking for a word, followed by an assignment
       is sufficient.
       """
    matches = False
    if self._next_token().kind == 'word':
      token = self._read_token()
      if self._next_token().kind == ':=':
        matches = True
      self._pushback_token(token)
    return matches

  def _is_bit_check(self):
    """Returns true if a bit check appears next on the input stream.

       Assumes that if a column if found, it must be a bit check.
       """
    return self._is_column_equals() or self._is_name_equals()

  def _is_column(self):
    """Returns true if input defines a column (pattern name).

       column         ::= id '(' int (':' int)? ')'
       """
    (tokens, matches) = self._is_column_tokens()
    self._pushback_tokens(tokens)
    return matches

  def _is_column_equals(self):
    """ """
    (tokens, matches) = self._is_column_tokens()
    if self._next_token().kind != '=':
      matches = False
    self._pushback_tokens(tokens)
    return matches

  def _is_column_tokens(self):
    """Collects the sequence of tokens defining:
       column         ::= id '(' int (':' int)? ')'
       """
    # Try to match sequence of tokens, saving tokens as processed
    tokens = []
    matches = False
    if self._next_token().kind == 'word':
      tokens.append(self._read_token('word'))
      if self._next_token().kind == '(':
        tokens.append(self._read_token('('))
        if self._is_int():
          tokens.append(self._read_token('word'))
          if self._next_token().kind == ':':
            tokens.append(self._read_token(':'))
            if self._is_int():
              tokens.append(self._read_token('word'))
          if self._next_token().kind == ')':
            matches = True
    return (tokens, matches)

  def _is_id(self):
    word = self._next_token()
    if word.kind != 'word': return False
    return _ID_PATTERN.match(word.value)

  def _is_field_name(self):
    return self._is_id() or self._is_action_option()

  def _is_int(self):
    """Tests if an integer occurs next."""
    if self._next_token().kind != 'word': return None
    return _DECIMAL_PATTERN.match(self._next_token().value)

  def _is_keyword(self, keyword):
    """Returns true if the next token is the given keyword."""
    token = self._next_token()
    return (token.kind == keyword or
            (token.kind == 'word' and token.value == keyword))

  def _is_nondecimal_int(self):
    if self._next_token().kind != 'word': return None
    word = self._next_token().value
    return (_HEXIDECIMAL_PATTERN.match(word) or
            _BITSTRING_PATTERN.match(word))

  def _is_name_equals(self):
    """Returns true if input begins with 'name='."""
    matches = False
    if self._next_token().kind == 'word':
      token = self._read_token('word')
      if self._next_token().kind == '=':
        matches = True
      self._pushback_token(token)
    return matches

  def _is_name_paren(self):
    """Returns true if input begins with 'name('."""
    matches = False
    if self._next_token().kind == 'word':
      token = self._read_token('word')
      if self._next_token().kind == '(':
        matches = True
      self._pushback_token(token)
    return matches

  def _is_name_semicolon(self):
    """Returns true if input begins with 'name:'."""
    matches = False
    if self._next_token().kind == 'word':
      token = self._read_token('word')
      if self._next_token().kind == ';':
        matches = True
      self._pushback_token(token)
    return matches

  def _check_action_is_well_defined(self, action):
    if not dgen_decoder.ActionDefinesDecoder(action):
      self._unexpected("No virtual fields defined for decoder")

  #------ Helper functions.

  def _add_column(self, table):
    """Adds a column to a table, and verifies that it isn't repeated."""
    column = self._column()
    if not table.add_column(column):
      self._unexpected('In table %s, column %s is repeated' %
                       (table.name, column.name()))

  def _define(self, name, value, context):
    """Install value under name in the given context. Report error if
       name is already defined.
    """
    if not context.define(name, value, fail_if_defined=False):
      self._unexpected('%s: multiple definitions' % name)

  def _read_id_or_none(self, read_id):
    if self._next_token().kind in ['|', '+', '(']:
      return None
    name = self._id() if read_id else self._read_token('word').value
    return None if name and name == 'None' else name

  #------ Tokenizing the input stream ------

  def _read_token(self, kind=None):
    """Reads and returns the next token from input."""
    token = self._next_token()
    self._token = None
    if kind and kind != token.kind:
      self._unexpected('Expected "%s" but found "%s"'
                       % (kind, token.kind))
    if _TRACE_TOKENS:
      print "Read %s" % token
    return token

  def _is_next_tokens(self, tokens):
    """Returns true if the input contains the sequence of tokens."""
    read_tokens = []
    match = True
    check_tokens = list(tokens)
    while check_tokens:
      token = check_tokens.pop(0)
      next = self._next_token()
      if next.kind == token:
        read_tokens.append(self._read_token(token))
      else:
        match = False
        break
    self._pushback_tokens(read_tokens)
    return match

  def _next_token(self):
    """Returns the next token from the input."""
    # First seee if cached.
    if self._token: return self._token

    if self._pushed_tokens:
      self._token = self._pushed_tokens.pop()
      return self._token

    # If no more tokens left on the current line. read
    # input till more tokens are found
    while not self._reached_eof and not self._words:
      self._words = self._extract_words(self._read_line())

    if self._words:
      # More tokens found. Convert the first word to a token.
      word = self._words.pop(0)

      # Quoted strings must not be split!
      if word[0] == "'":
        self._token = Token('word', word)
        return self._token

      # First remove any applicable punctuation.
      for p in self._punctuation:
        index = word.find(p)
        if index == 0:
          # Found punctuation, return it.
          self._pushback(word[len(p):])
          self._token = Token(p)
          return self._token
        elif index > 0:
          self._pushback(word[index:])
          word = word[:index]

      # See if reserved keyword.
      for key in self._reserved:
        index = word.find(key)
        if index == 0:
          # Found reserved keyword. Verify at end, or followed
          # by punctuation.
          rest = word[len(key):]
          if not rest or 0 in [rest.find(p) for p in self._punctuation]:
            self._token = Token(key)
            return self._token

      # if reached, word doesn't contain any reserved words
      # punctuation, so return it.
      self._token = Token('word', word)
    else:
      # No more tokens found, assume eof.
      self._token = Token('eof')
    return self._token

  def _extract_words(self, line):
    """Returns the list of words in the given line. Words, in this
       context include any non-commented text that is separated by
       whitespace, and do not appear in quotes.
       Note: This code assumes that comments have already been removed."""
    words = []
    start_index = 0;
    line_length = len(line)
    for i in range(0, line_length):
      # Skip over matched text.
      if i < start_index: continue

      # See if word separator, and process accordingly.
      ch = line[i]
      if ch in [' ', '\t', '\n', "'"]:
        #  Word separator. Add word and skip over separator.
        if start_index < i:
          # non-empty text found. add as word.
          words.append(line[start_index:i])
        if ch == "'":
          text = self._extract_quoted_string(ch, i, line)
          words.append(text)
          start_index = i + len(text)
        else:
          start_index = i + 1

    # All text processed. Add word if non-empty text at end of line.
    if start_index < line_length:
      words.append(line[start_index:line_length])
    return words

  def _extract_quoted_string(self, quote, start_index, line):
    i = start_index + 1
    line_length = len(line)
    while i < line_length:
      ch = line[i]
      i += 1
      if ch == quote:
        return line[start_index:i]
    # If reached, did not find end of quoted string.
    self._unexpected(
        "Can't find matching quote for string starting with quote %s" %
        quote)

  def _pushback(self, word):
    """Puts word back onto the list of words."""
    if word:
      self._words.insert(0, word)

  def _pushback_token(self, token):
    """Puts token back on to the input stream."""
    if _TRACE_TOKENS:
      print "pushback %s" % token
    if self._token:
      self._pushed_tokens.append(self._token)
      self._token = token
    else:
      self._token = token

  def _pushback_tokens(self, tokens):
    """Puts back the reversed list of tokens on to the input stream."""
    while tokens:
      self._pushback_token(tokens.pop())

  def _read_line(self):
    """Reads the next line of input, and returns it. Otherwise None."""
    self._line_no += 1
    line = self.input.readline()
    if line:
      return re.sub(r'#.*', '', line).strip()
    else:
      self._reached_eof = True
      return ''

  #-------- Error reporting ------

  def _unexpected(self, context='Unexpected line in input'):
    """"Reports that we didn't find the expected context. """
    raise Exception('Line %d: %s' % (self._line_no, context))
