#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
The core object model for the Decoder Generator.  The dg_input and
dg_output modules both operate in terms of these classes.
"""

NUM_INST_BITS = 32

NEWLINE_STR="""
"""

def _popcount(int):
    """Returns the number of 1 bits in the input."""
    count = 0
    for bit in range(0, NUM_INST_BITS):
        count = count + ((int >> bit) & 1)
    return count

def neutral_repr(value):
  """Returns a neutral representation for the value.

     Used to remove identifier references from values, so that we can
     merge rows/actions of the table.
     """
  if (isinstance(value, BitExpr) or isinstance(value, SymbolTable) or
      isinstance(value, Row) or isinstance(value, DecoderAction) ):
    return value.neutral_repr()
  elif isinstance(value, list):
    return '[' + ',\n    '.join([ neutral_repr(v) for v in value ]) + ']'
  else:
    return repr(value)

def sub_bit_exprs(value):
  """Returns the list of (immediate) sub bit expressions within the
     given value.

     Used to find the set of identifier references used by a value.
     """
  if isinstance(value, BitExpr) or isinstance(value, SymbolTable):
    return value.sub_bit_exprs()
  elif isinstance(value, list):
    exprs = set()
    _add_if_bitexpr(value, exprs)
    return list(exprs)
  else:
    return []

def _add_if_bitexpr(value, set):
  """Adds value to set if value is a BitExpr (or sub values if a list)."""
  if value:
    if isinstance(value, BitExpr):
      set.add(value)
    elif isinstance(value, list):
      for v in value:
        _add_if_bitexpr(v, set)

def _close_referenced_bit_exprs(values):
  """Adds to values, all indirect BitExprs referenced by BitExprs already
     in the set of values."""
  workset = list(values)
  while workset:
    value = workset.pop()
    for exp in sub_bit_exprs(value):
      if exp not in values:
        values.add(exp)
        workset.append(exp)

class BitExpr(object):
  """Define a bit expression."""

  def negate(self):
    """Returns the negation of self."""
    return NegatedTest(self)

  def to_bitfield(self, options={}):
    """Returns the bit field (i.e. sequence of bits) described by the
       BitExpr. Returns an instance of BitField.
       """
    # Default implementation is to convert it by adding unsigned
    # int mask.
    return BitField(self, NUM_INST_BITS - 1, 0)

  def to_type(self, type, options={}):
    """Converts the expression to the given type."""
    if type == 'bool':
      return self.to_bool(options)
    if type == 'uint32':
      return self.to_uint32(options)
    if type == 'register':
      return self.to_register(options)
    if type == 'register_list':
      return self.to_register_list(options)
    if type == 'bitfield':
      return self.to_bitfield(options)
    raise Exception("to_type(%s): can't convert %s" % (type, self))

  def to_bool(self, options={}):
    """Returns a string describing this as a C++ boolean
       expression."""
    return "(%s != 0)" % self.to_uint32(options)

  def to_commented_bool(self, options={}):
    """Returns a string describing this as a C++ boolean expression,
       with a comment describing the corresponding BitExpr it came
       from."""
    return '%s /* %s */' % (self.to_bool(options), repr(self))

  def to_register(self, options={}):
    """Returns a string describing this as a C++ Register."""
    return 'Register(%s)' % self.to_uint32(options)

  def to_commented_register(self, options={}):
    """Returns a string describing this as a C++ Register expression,
       with a comment describing the corresponding BitExpr it came
       from."""
    return '%s /* %s */' % (self.to_register(options), repr(self))

  def to_register_list(self, options={}):
    """Returns a string describing this as a C++ RegisterList
       expression."""
    raise Exception("to_register_list not defined for %s %s." %
                    (type(self), self))

  def to_commmented_register_list(self, options={}):
    """Returns a string describing this as a C++ RegisterList
       Expression, with a comment describing the corresponding BitExpr
       it came from."""
    return '%s /* %s */' % (self.to_register_list(options), repr(self))

  def to_uint32(self, options={}):
    """Returns a string describing this as a C++ uint32_t
    expression."""
    raise Exception("to_uint32 not defined for %s." % type(self))

  def to_commented_uint32(self, options={}):
    """Returns a string describing this as a C++ uint32_t expression,
       with a comment describing the corresponding BitExpr it came
       from."""
    return '%s /* %s */' % (self.to_uint32(options), repr(self))

  def sub_bit_exprs(self):
    """Returns a list of (immediate) sub bit expressions within the
    bit expr."""
    return []

  def must_be_valid_shift_op(self):
    """Returns true only if it is provable that the corresponding
       value generated by to_uint32 is greater than or equal to zero,
       and not greater than 32. In such cases, we know that the
       shift will not cause a runtime error, and needs to be checked.
       """
    return self.must_be_in_range(0, 33)

  def must_be_in_range(self, min_include, max_exclude):
    """Returns true only if it is provable that the corresponding
       value returned by to_uint32 is greater than or equal to
       min_include, and less than max_exclude."""
    return False

  def to_uint32_constant(self):
    """Returns (uint32) constant denoted by self, or None if
       it can't be converted to a constant."""
    return None

  def neutral_repr(self):
    """Like repr(self) except identifier references are replaced with
      their definition.

       Used to define a form for comparison/hashing that is neutral to
       the naming conventions used in the expression.
       """
    raise Exception("neutral_repr not defined for %s" % type(self))

  def __hash__(self):
    return hash(neutral_repr(self))

  def __cmp__(self, other):
    return (cmp(type(self), type(other)) or
            cmp(neutral_repr(self), neutral_repr(other)))

class IdRef(BitExpr):
  """References an (already defined) name, and the value associated
     with the name.
     """

  def __init__(self, name, value=None):
    self._name = name
    self._value = value

  def name(self):
    return self._name

  def value(self):
    return self._value

  def to_bitfield(self, options={}):
    return self._value.to_bitfield(options)

  def to_bool(self, options={}):
    return self._value.to_bool(options)

  def to_register(self, options={}):
    return self._value.to_register(options)

  def to_register_list(self, options={}):
    return self._value.to_register_list(options)

  def to_uint32(self, options={}):
    return self._value.to_uint32(options)

  def __repr__(self):
    return '%s' % self._name

  def sub_bit_exprs(self):
    return [self._value]

  def must_be_in_range(self, min_include, max_exclude):
    return self._value.must_be_in_range(min_include, max_exclude)

  def to_uint32_constant(self):
    return self._value.to_uint32_constant()

  def neutral_repr(self):
    return self._value.neutral_repr()

  def __hash__(self):
    return hash(self._name) + hash(self._value)

  def __cmp__(self, other):
    return (cmp(type(self), type(other)) or
            cmp(self._name, other._name) or
            cmp(self._value, other._value))

_AND_PRINT_OP=""" &&
       """

class AndExp(BitExpr):
  """Models an anded expression."""

  def __init__(self, args):
    if not isinstance(args, list) or len(args) < 2:
      raise Exception(
          "AndExp(%s) expects at least two elements" % args)
    self._args = args

  def args(self):
    return self._args[:]

  def to_bool(self, options={}):
    value = '(%s)' % self._args[0].to_bool(options)
    for arg in self._args[1:]:
      value = '%s%s(%s)' % (value, _AND_PRINT_OP, arg.to_bool(options))
    return value

  def to_uint32(self, options={}):
    value = self._args[0].to_uint32(options)
    for arg in self._args[1:]:
      value = '%s & %s' % (value, arg.to_uint32(options))
    return '(%s)' % value

  def to_register(self, options={}):
    raise Exception("to_register not defined for %s" % self)

  def to_register_list(self, options={}):
    raise Exception("to_register_list not defined for %s" % self)

  def sub_bit_exprs(self):
    return list(self._args)

  def __repr__(self):
    return _AND_PRINT_OP.join([repr(a) for a in self._args])

  def neutral_repr(self):
    return _AND_PRINT_OP.join([neutral_repr(a) for a in self._args])

_OR_PRINT_OP=""" ||
       """

class OrExp(BitExpr):
  """Models an or-ed expression."""

  def __init__(self, args):
    if not isinstance(args, list) or len(args) < 2:
      raise Exception(
          "OrExp(%s) expects at least two elements" % args)
    self._args = args

  def args(self):
    return self._args[:]

  def to_bool(self, options={}):
    value = '(%s)' % self._args[0].to_bool(options)
    for arg in self._args[1:]:
      value = '%s%s(%s)' % (value, _OR_PRINT_OP, arg.to_bool(options))
    return value

  def to_register(self, options={}):
    raise Exception("to_register not defined for %s" % self)

  def to_register_list(self, options={}):
    raise Exception("to_register_list not defined for %s" % self)

  def to_uint32(self, options={}):
    value = self.args[0].to_uint32(options)
    for arg in self._args[1:]:
      value = '%s | %s' % (value, arg.to_uint32(options))
    return '(%s)' % value

  def sub_bit_exprs(self):
    return list(self._args)

  def __repr__(self):
    return _OR_PRINT_OP.join([repr(a) for a in self._args])

  def neutral_repr(self):
    return _OR_PRINT_OP.join([neutral_repr(a) for a in self._args])

# Defines the negated comparison operator.
_NEGATED_COMPARE_OP = {
    '<': '>=',
    '<=': '>',
    '==': '!=',
    '!=': '==',
    '>=': '<',
    '>': '<=',
}

_COMPARE_OP_FORMAT=""" %s
         """

class CompareExp(BitExpr):
  """Models the comparison of two values."""

  def __init__(self, op, arg1, arg2):
    if not _NEGATED_COMPARE_OP.get(op):
      raise Exception("Unknown compare operator: %s" % op)
    self._op = op
    self._args = [arg1, arg2]

  def op(self):
    return self._op

  def args(self):
    return self._args[:]

  def negate(self):
    return CompareExp(_NEGATED_COMPARE_OP[self._op],
                      self._args[0], self._args[1])

  def to_bool(self, options={}):
    return '((%s) %s (%s))' % (self._args[0].to_uint32(options),
                               self._op,
                               self._args[1].to_uint32(options))

  def to_register(self, options={}):
    raise Exception("to_register not defined for %s" % self)

  def to_register_list(self, options={}):
    raise Exception("to_register_list not defined for %s" % self)

  def to_uint32(self, options={}):
    raise Exception("to_uint32 not defined for %s" % self)

  def sub_bit_exprs(self):
    return list(self._args)

  def __repr__(self):
    return '%s %s %s' % (repr(self._args[0]),
                         self.compare_op(),
                         repr(self._args[1]))

  def neutral_repr(self):
    # Note: We canconicalize the tests to improve chances that we
    # merge more expressions.
    arg1 = neutral_repr(self._args[0])
    arg2 = neutral_repr(self._args[1])
    if self._op in ['==', '!=']:
      # Order arguments based on comparison value.
      cmp_value = cmp(arg1, arg2)
      if cmp_value < 0:
        return '%s %s %s' % (arg1, self.compare_op(), arg2)
      elif cmp_value > 0:
        return '%s %s %s' % (arg2, self.compare_op(), arg1)
      else:
        # comparison against self can be simplified.
        return BoolValue(self._op == '==').neutral_repr()
    elif self._op in ['>', '>=']:
      return '%s %s %s' % (arg2,
                           self.compare_op(_NEGATED_COMPARE_OP.get(self._op)),
                           arg1)
    else:
      # Assume in canonical order.
      return '%s %s %s' % (arg1, self.compare_op(), arg2)

  def compare_op(self, op=None):
    if op == None: op = self._op
    return _COMPARE_OP_FORMAT % op


class ShiftOp(BitExpr):
  """Models a left/right shift operator."""

  def __init__(self, op, arg1, arg2):
    if op not in ['<<', '>>']:
      raise Exception("Not shift op: %s" % op)
    if not arg2.must_be_valid_shift_op():
      raise Exception("Can't statically determine shift value.")
    self._op = op
    self._args = [arg1, arg2]

  def args(self):
    return list(self._args)

  def to_uint32(self, options={}):
    return '(%s %s %s)' % (self._args[0].to_uint32(options),
                           self._op,
                           self._args[1].to_uint32(options))

  def __repr__(self):
    return '%s %s %s' % (self._args[0],
                         self._op,
                         self._args[1])

  def neutral_repr(self):
    return '%s %s %s' % (neutral_repr(self._args[0]),
                         self._op,
                         neutral_repr(self._args[1]))

  def sub_bit_exprs(self):
    return list(self._args)

class AddOp(BitExpr):
  """Models an additive operator."""

  def __init__(self, op, arg1, arg2):
    if op not in ['+', '-']:
      raise Exception("Not add op: %s" % op)
    self._op = op
    self._args = [arg1, arg2]

  def args(self):
    return self._args[:]

  def to_register_list(self, options={}):
    rl = self._args[0].to_register_list(options)
    if self._op == '+':
      return '%s.\n   Add(%s)' % (rl, self._args[1].to_register(options))
    elif self._op == '-':
      return '%s.\n   Remove(%s)' % (rl, self._args[1].to_register(options))
    else:
      raise Exception("Bad op %s" % self._op)

  def to_uint32(self, options={}):
      # Check subtraction as a special case. By default, we assume that all
      # integers are unsigned. However, a difference may generate a negative
      # value. In C++, the subtraction of unsigned integers is an unsigned
      # integer, which is not a difference. To fix this, we insert integer
      # typecasts.
    if self._is_subtract_bitfields():
        # Cast each argument to an int, so that we can do subtraction that
        # can result in negative values.
        args = [TypeCast('int', a) for a in self._args]
        return AddOp('-', args[0], args[1]).to_uint32(options)
    else:
        return '%s %s %s' % (self._args[0].to_uint32(options),
                             self._op,
                             self._args[1].to_uint32(options))

  def _is_subtract_bitfields(self):
      """Returns true if the subtraction of bitfields that are not defined
         by typecasts."""
      if self._op != '-': return False
      for arg in self.args():
          if isinstance(arg, TypeCast):
              return False
          try:
              bf = arg.to_bitfield()
          except:
              return False
      return True

  def to_uint32_constant(self):
    args = [a.to_uint32_constant() for a in self._args]
    if None in args or len(args) != 2:
      return None
    return self._eval(args[0], self._op,  args[1])

  def sub_bit_exprs(self):
    return list(self._args)

  def must_be_in_range(self, min_include, max_exclude):
    for i in [0, 1]:
      c = self._args[i].to_uint32_constant()
      if c:
        if self._op ==  '+':
          # Adjust the range by the constant c, and then test if
          # the other argument is in that range. We can do this,
          # since addition is commutative. Note that the range
          # of the other argument is defined by subtracting c from
          # the range of the result.
          return self._args[1-i].must_be_in_range(
              self._eval(min_include, '-', c),
              self._eval(max_exclude, '-', c))
        elif i == 1:  # i.e. of form: _args[0] - c
          # Adjust the range by the constant c, and test if the
          # first argument is in that range. Note that the range
          # of the first argument is defined by adding c to the
          # range of the result.
          return self._args[0].must_be_in_range(
              self._eval(min_include, '+', c),
              self._eval(max_exclude, '+', c))
    # If reached, don't know how to prove.
    return False

  def _eval(self, x, op, y):
    """Calculates 'x op y', assuming op in {'-', '+'})."""
    if op == '+':
      return x + y
    elif op == '-':
      return x - y
    else:
      raise Exception("Don't know how to apply: %s %s %s" %
                      (x, op, y))

  def __repr__(self):
    return '%s %s %s' % (repr(self._args[0]),
                         self._op,
                         repr(self._args[1]))
  def neutral_repr(self):
    return '%s %s %s' % (neutral_repr(self._args[0]),
                         self._op,
                         neutral_repr(self._args[1]))

# Defines the c++ operator for the given mulop.
_CPP_MULOP = {
    '*': '*',
    '/': '/',
    'mod': '%%',
}

class MulOp(BitExpr):
  """Models an additive operator."""

  def __init__(self, op, arg1, arg2):
    if not _CPP_MULOP.get(op):
      raise Exception("Not mul op: %s" % op)
    self._op = op
    self._args = [arg1, arg2]

  def args(self):
    return self._args[:]

  def to_uint32(self, options={}):
    return '%s %s %s' % (self._args[0].to_uint32(options),
                         _CPP_MULOP[self._op],
                         self._args[1].to_uint32(options))

  def sub_bit_exprs(self):
    return list(self._args)

  def __repr__(self):
    return '%s %s %s' % (repr(self._args[0]),
                         self._op,
                         repr(self._args[1]))
  def neutral_repr(self):
    return '%s %s %s' % (neutral_repr(self._args[0]),
                         self._op,
                         neutral_repr(self._args[1]))

class Concat(BitExpr):
  """Models a value generated by concatentation bitfields."""

  def __init__(self, args):
    if not isinstance(args, list) or len(args) < 2:
      raise Exception(
          "Concat(%s) expects at least two arguments" % args)
    self._args = args

  def args(self):
    return self._args[:]

  def to_bitfield(self, options={}):
    # Assume we can generate a bitfield from the expression, by
    # concatenating subfields, if each subfield is a bitfield.
    try:
      # As long ase each subfield is convertable, and the max number
      # of bits is not exceeded, generate the corresponding bitfield.
      bits = sum([ a.to_bitfield(options).num_bits() for a in self._args ])
      if bits > 32:
        raise exception("can't compose bitfield from concat")
      return BitField(self, bits - 1, 0)
    except:
      # Can't convert,piecewise, so just assume that we can
      # convert to an unsigned integer, and then define a
      # bitfield on the unsigned integer.
      return BitField(self, NUM_INST_BITS - 1, 0)

  def to_uint32(self, options={}):
    value = self._args[0].to_uint32()
    for arg in self._args[1:]:
      bitfield = arg.to_bitfield(options)
      value = ("(((%s) << %s) | %s)" %
               (value, bitfield.num_bits(), bitfield.to_uint32()))
    return value

  def sub_bit_exprs(self):
    return list(self._args)

  def __repr__(self):
    return ':'.join([repr(a) for a in self._args])

  def neutral_repr(self):
    return ':'.join([a.neutral_repr() for a in self._args])

class BitSet(BitExpr):
  """Models a set of expressions."""

  def __init__(self, values):
    self._values = values

  def args(self):
    return self._values[:]

  def to_register_list(self, options={}):
    code = 'RegisterList()'
    for value in self._values:
      code = '%s.\n   Add(%s)' % (code, value.to_register(options))
    return code

  def sub_bit_exprs(self):
    return list(self._values)

  def __repr__(self):
    return '{%s}' % ', '.join([repr(v) for v in self._values])

  def neutral_repr(self):
    return '{%s}' % ', '.join([neutral_repr(v) for v in self._values])

"""Defines a map from a function name, to the list of possible signatures
   for the funtion. The signature is a two-tuple where the first element
   is a list of parameter types, and the second is the result type.
   If a function does not appear in this list, all arguments are assumed
   to be of type uint32, and return type uint32.
   NOTE: Currently, one can't allow full polymorphism in the signatures,
   because there is no way to test if an expression can be of a particular
   type.
   """
_FUNCTION_SIGNATURE_MAP = {
    'Add': [(['register_list', 'register'], 'register_list')],
    'Contains': [(['register_list', 'register'], 'bool')],
    'Union': [(['register_list', 'register_list'], 'register_list')],
    'NumGPRs': [(['register_list'], 'uint32')],
    'SmallestGPR': [(['register_list'], 'uint32')],
    'Register': [(['uint32'], 'register')],
    'RegisterList': [([], 'register_list'),
                     (['uint32'], 'register_list'),
                     # (['register'], 'registerlist),
                     ],
    }

# Models how each DGEN type is represented as a C++ type cast.
DGEN_TYPE_TO_CPP_TYPE = {
    'int': 'int32_t',
    'unsigned': 'uint32_t'
    }

class TypeCast(BitExpr):
    """Allow some simple type castings."""

    def __init__(self, type, arg):
        self._type = type
        self._arg = arg
        if type not in DGEN_TYPE_TO_CPP_TYPE.keys():
            raise Exception('TypeCast(%s, %s): type not understood.' %
                            (type, arg))
        # Verify we can convert arg to an integer.
        arg.to_uint32()

    def name(self):
        return self._name

    def arg(self):
        return self._arg

    def to_uint32(self, options={}):
        return ('static_cast<%s>(%s)' %
                (DGEN_TYPE_TO_CPP_TYPE[self._type],
                 self._arg.to_uint32(options)))

    def sub_bit_exprs(self):
        return [ self._arg ]

    def __repr__(self):
        return '%s(%s)' % (self._type, self._arg)

    def neutral_repr(self):
        return '%s(%s)' % (self._type, neutral_repr(self._arg))

class FunctionCall(BitExpr):
  """Abstract class defining an (external) function call."""

  def __init__(self, name, args):
    self._name = name
    self._args = args

  def name(self):
    return self._name

  def args(self):
    return self._args[:]

  def to_bitfield(self, options={}):
    raise Exception('to_bitfield not defined for %s' % self)

  def to_bool(self, options={}):
    return self._to_call('bool', self._add_namespace_option(options))

  def to_register(self, options={}):
    return self._to_call('register', self._add_namespace_option(options))

  def to_register_list(self, options={}):
    return self._to_call('register_list', self._add_namespace_option(options))

  def to_uint32(self, options={}):
    return self._to_call('uint32', self._add_namespace_option(options))

  def sub_bit_exprs(self):
    return list(self._args)

  def __repr__(self):
    return "%s(%s)" % (self._name,
                       ', '.join([repr(a) for a in self._args]))

  def neutral_repr(self):
    return "%s(%s)" % (self._name,
                       ', '.join([neutral_repr(a) for a in self._args]))

  def matches_signature(self, signature, return_type, options={}):
    """Checks whether the function call matches the signature.
       If so, returns the corresponding (translated) arguments
       to use for the call. Otherwise returns None.
       """
    params, result = signature
    if result != return_type: return None
    if len(params) != len(self.args()): return None
    args = []
    for (type, arg) in zip(params, self.args()):
      args.append(arg.to_type(type, options))
    return args

  def _to_call(self, return_type, options={}):
    """Generates a call to the external function."""
    # Try (pseudo) translation functions.
    trans_fcns = _FUNCTION_TRANSLATION_MAP.get(self._name)
    if trans_fcns:
      for fcn in trans_fcns:
        exp = fcn(self, return_type, options)
        if exp: return exp

    # Convert arguments to corresponding signatures, and
    # return corresponding call.
    namespace = (('%s::' % options.get('namespace'))
                 if options.get('namespace') else '')
    signatures = _FUNCTION_SIGNATURE_MAP.get(self._name)
    if signatures == None:
      args = [a.to_uint32(options) for a in self.args()]
    else:
      good = False
      for signature in signatures:
        args = self.matches_signature(signature, return_type, options)
        if args != None:
          good = True
      if not good:
        raise Exception("don't know how to translate to %s: %s" %
                        (return_type, self))
    return '%s(%s)' % ('%s%s' % (namespace, self._name), ', '.join(args))

  def _add_namespace_option(self, options):
    if not options.get('namespace'):
      options['namespace'] = 'nacl_arm_dec'
    return options

class InSet(BitExpr):
  """Abstract class defining set containment."""

  def __init__(self, value, bitset):
    self._value = value
    self._bitset = bitset

  def value(self):
    """Returns the value to test membership on."""
    return self._value

  def bitset(self):
    """Returns the set of values to test membership on."""
    return self._bitset

  def to_bitfield(self, options={}):
    raise Exception("to_bitfield not defined for %s" % self)

  def to_bool(self, options={}):
    return self._simplify().to_bool(options)

  def to_register(self, options={}):
    raise Exception("to_register not defined for %s" % self)

  def to_register_list(self, options={}):
    raise Exception("to_register_list not defined for %s" % self)

  def to_uint32(self, options={}):
    return self._simplify().to_uint32(options)

  def sub_bit_exprs(self):
    return [self._value, self._bitset]

  def neutral_repr(self):
    return self._simplify().neutral_repr()

  def _simplify(self):
    """Returns the simplified or expression that implements the
       membership tests."""
    args = self._bitset.args()
    if not args: return BoolValue(False)
    if len(args) == 1: return self._simplify_test(args[0])
    return OrExp([self._simplify_test(a) for a in args])

  def _simplify_test(self, arg):
    """Returns how to test if the value matches arg."""
    raise Exception("InSet._simplify_test not defined for type %s" % type(self))

class InUintSet(InSet):
  """Models testing a value in a set of integers."""

  def __init__(self, value, bitset):
    InSet.__init__(self, value, bitset)

  def _simplify_test(self, arg):
    return CompareExp("==", self._value, arg)

  def __repr__(self):
    return "%s in %s" % (repr(self._value), repr(self._bitset))

class InBitSet(InSet):
  """Models testing a value in a set of bit patterns"""

  def __init__(self, value, bitset):
    InSet.__init__(self, value, bitset)
    # Before returning, be sure the value/bitset entries correctly
    # correspond, by forcing construction of the simplified expression.
    self.neutral_repr()

  def _simplify_test(self, arg):
    return BitPattern.parse(arg, self._value)

  def __repr__(self):
    return "%s in bitset %s" % (repr(self._value), repr(self._bitset))

_IF_THEN_ELSE_CPP_FORMAT="""(%s
       ? %s
       : %s)"""

_IF_THEN_ELSE_DGEN_FORMAT="""%s
       if %s
       else %s"""

class IfThenElse(BitExpr):
  """Models a conditional expression."""

  def __init__(self, test, then_value, else_value):
    self._test = test
    self._then_value = then_value
    self._else_value = else_value

  def test(self):
    return self._test

  def then_value(self):
    return self._then_value

  def else_value(self):
    return self._else_value

  def negate(self):
    return IfThenElse(self._test, self._else_value, self._then_value)

  def to_bool(self, options={}):
    return _IF_THEN_ELSE_CPP_FORMAT % (
        self._test.to_bool(options),
        self._then_value.to_bool(options),
        self._else_value.to_bool(options))

  def to_register_list(self, options={}):
    return _IF_THEN_ELSE_CPP_FORMAT % (
        self._test.to_bool(options),
        self._then_value.to_register_list(options),
        self._else_value.to_register_list(options))

  def to_uint32(self, options={}):
    return _IF_THEN_ELSE_CPP_FORMAT % (
        self._test.to_bool(options),
        self._then_value.to_uint32(options),
        self._else_value.to_uint32(options))

  def sub_bit_exprs(self):
    return [self._test, self._then_value, self._else_value]

  def __repr__(self):
    return _IF_THEN_ELSE_DGEN_FORMAT % (
        self._then_value, self._test, self._else_value)

  def neutral_repr(self):
    return _IF_THEN_ELSE_DGEN_FORMAT % (
        neutral_repr(self._then_value),
        neutral_repr(self._test),
        neutral_repr(self._else_value))

class ParenthesizedExp(BitExpr):
  """Models a parenthesized expression."""

  def __init__(self, exp):
    self._exp = exp

  def exp(self):
    return self._exp

  def negate(self):
    value = self._exp.negate()
    return ParenthesizedExp(value)

  def to_bitfield(self, options={}):
    return self._exp.to_bitfield(options)

  def to_bool(self, options={}):
    return '(%s)' % self._exp.to_bool(options)

  def to_register(self, options={}):
    return '(%s)' % self._exp.to_register(options)

  def to_register_list(self, options={}):
    return '(%s)' % self._exp.to_register_list(options)

  def to_uint32(self, options={}):
    return '(%s)' % self._exp.to_uint32(options)

  def sub_bit_exprs(self):
    return [self._exp]

  def must_be_in_range(self, min_include, max_exclude):
    return self._exp.must_be_in_range(min_include, max_exclude)

  def to_uint32_constant(self):
    return self._exp.to_uint32_constant()

  def __repr__(self):
    return '(%s)' % repr(self._exp)

  def neutral_repr(self):
    return '(%s)' % neutral_repr(self._exp)

class Implicit(BitExpr):
  """Models an implicit method definition, based on the values of
     other method definitions."""
  def __init__(self, methods):
    self._methods = methods

  def methods(self):
    return list(self._methods)

  def __repr__(self):
    return ("implied by %s" %
            ', '.join([repr(m) for m in self._methods]))

  def neutral_repr(self):
    return repr(self)

class QuotedString(BitExpr):
  """Models a quoted string."""

  def __init__(self, text, name=None):
    if not isinstance(text, str):
      raise Exception("Can't create a quoted string from %s" % text)
    self._text = text
    if name == None: name = repr(text)
    self._name = name

  def name(self):
    return self._name

  def text(self):
    return self._text

  def __repr__(self):
    return self.name()

  def to_cstring(self):
    return '"%s"' % self._text

  def neutral_repr(self):
    return self.name()

class Literal(BitExpr):
  """Models a literal unsigned integer."""

  def __init__(self, value, name=None):
    if not isinstance(value, int):
      raise Exception("Can't create literal from %s" % value)
    self._value = value
    if name == None: name = repr(value)
    self._name = name

  def name(self):
      return self._name

  def value(self):
    return self._value

  def to_uint32(self, options={}):
    return repr(self._value)

  def must_be_in_range(self, min_include, max_exclude):
    c = self._to_uint32_constant()
    return min_include <= c and c < max_exclude

  def _to_uint32_constant(self):
    return int(self.to_uint32())

  def neutral_repr(self):
    return self.name()

  def __repr__(self):
      return self.name()

class BoolValue(BitExpr):
  """Models true and false."""

  def __init__(self, value):
    if not isinstance(value, bool):
      raise Exception("Can't create boolean value from %s" % value)
    self._value = value

  def value(self):
    return self._value

  def to_bool(self, options={}):
    return 'true' if self._value else 'false'

  def to_uint32(self, options={}):
    return 1 if self._value else 0

  def __repr__(self):
    return self.to_bool()

  def neutral_repr(self):
    return self.to_bool()

class NegatedTest(BitExpr):
  """Models a negated (test) value."""

  def __init__(self, test):
    self._test = test

  def negate(self):
    return self._test

  def to_bitfield(self, options={}):
    raise Exception("to_bitfield not defined for %s" % self)

  def to_bool(self, options={}):
    return "!(%s)" % self._test.to_bool(options)

  def to_register(self, options={}):
    raise Exception('to_register not defined for %s' % self)

  def to_uint32(self, options={}):
    return "((uint32_t) %s)" % self.to_bool(options)

  def sub_bit_exprs(self):
    return [self._test]

  def __repr__(self):
    return 'not %s' % self._test

  def neutral_repr(self):
    return 'not %s' % self._test.neutral_repr()

class BitField(BitExpr):
  """Defines a bitfield within an instruction."""

  def __init__(self, name, hi, lo, options={}):
    if not isinstance(name, BitExpr):
      # Unify non-bit expression bitfield to corresponding bitfield
      # version, so that there is a uniform representation.
      name = name if isinstance(name, str) else repr(name)
      name = IdRef(name, Instruction())
    self._name = name
    self._hi = hi
    self._lo = lo
    max_bits = self._max_bits(options)
    if not (0 <= lo and lo <= hi and hi < max_bits):
      raise Exception('BitField %s: range illegal' % repr(self))

  def num_bits(self):
    """"returns the number of bits represented by the bitfield."""
    return self._hi - self._lo + 1

  def mask(self):
    mask = 0
    for i in range(0, self.num_bits()):
      mask = (mask << 1) + 1
    mask = mask << self._lo
    return mask

  def name(self):
    return self._name

  def hi(self):
    return self._hi

  def lo(self):
    return self._lo

  def to_bitfield(self, option={}):
    return self

  def to_uint32(self, options={}):
    masked_value ="(%s & 0x%08X)" % (self._name.to_uint32(options), self.mask())
    if self._lo != 0:
      masked_value = '(%s >> %s)' % (masked_value, self._lo)
    return masked_value

  def must_be_in_range(self, min_include, max_exclude):
    if min_include > 0: return False
    if (1 << (self.hi() + 1 - self.lo())) > max_exclude: return False
    return True

  def _max_bits(self, options):
    """Returns the maximum number of bits allowed."""
    max_bits = options.get('max_bits')
    if not max_bits:
      max_bits = 32
    return max_bits

  def sub_bit_exprs(self):
    return [self._name]

  def __repr__(self):
    return self._named_repr(repr(self._name))

  def neutral_repr(self):
    return self._named_repr(self._name.neutral_repr())

  def _named_repr(self, name):
    if self._hi == self._lo:
      return '%s(%s)' % (name, self._hi)
    else:
      return '%s(%s:%s)' % (name, self._hi, self._lo)

class Instruction(BitExpr):
  """Models references to the intruction being decoded."""

  def to_uint32(self, options={}):
    return '%s.Bits()' % self._inst_name(options)

  def __repr__(self):
    return self._inst_name()

  def neutral_repr(self):
    return self._inst_name()

  def _inst_name(self, options={}):
    inst = options.get('inst')
    if not inst:
      inst = 'inst'
    return inst

class SafetyAction(BitExpr):
  """Models a safety check, and the safety action returned."""

  def __init__(self, test, action):
    # Note: The following list is from inst_classes.h, and should
    # be kept in sync with that file (see type SafetyLevel).
    if action not in ['UNINITIALIZED',
                      'UNKNOWN', 'UNDEFINED', 'NOT_IMPLEMENTED',
                      'UNPREDICTABLE', 'DEPRECATED', 'FORBIDDEN',
                      'FORBIDDEN_OPERANDS', 'DECODER_ERROR', 'MAY_BE_SAFE']:
      raise Exception("Safety action %s => %s not understood" %
                      (test, action))
    self._test = test
    self._action = action

  def test(self):
    return self._test

  def action(self):
    return self._action

  def to_bitfield(self, options={}):
    raise Exception("to_bitfield not defined for %s" % self)

  def to_bool(self, options={}):
    # Be sure to handle inflection of safety value when defining the boolean
    # value the safety action corresponds to.
    if self._action == 'MAY_BE_SAFE':
      return self._test.to_bool(options)
    else:
      return self._test.negate().to_bool(options)

  def to_register(self, options={}):
    raise Exception("to_register not defined for %s" % self)

  def to_register_list(self, options={}):
    raise Exception("to_register_list not defined for %s" % self)

  def to_uint32(self, options={}):
    raise Exception("to_uint32 not defined for %s" % self)

  def sub_bit_exprs(self):
    return [self._test]

  def __repr__(self):
    return '%s => %s' % (repr(self._test), self._action)

  def neutral_repr(self):
    return '%s => %s' % (neutral_repr(self._test), self._action)

class Violation(BitExpr):
  """Models a (conditional) violation."""

  def __init__(self, test, print_args):
    self._test = test
    self._print_args = print_args

  def test(self):
    return self._test

  def violation_type(self):
    return self._violation_type

  def print_args(self):
    return list(self._print_args)

  def to_bool(self, options={}):
    return self.test().to_bool(options)

  def sub_bit_exprs(self):
    return [self._test] + self._print_args

  def __repr__(self):
    return '%s =>\n   error(%s)' % (
        self._test,
        ', '.join([repr(a) for a in self._print_args]))

  def neutral_repr(self):
    return '%s =>\n   error(%s)' % (
        neutral_repr(self._test),
        ', '.join([neutral_repr(a) for a in self._print_args]))

_INHERITS_SYMBOL = '$inherits$'
_INHERITS_EXCLUDES_SYMBOL = '$inherits-excludes$'

class SymbolTable(object):
  """Holds mapping from names to corresponding value."""

  def __init__(self):
    self._dict = {}
    self._frozen = False

  def copy(self):
    """Returns a copy of the symbol table"""
    st = SymbolTable()
    for k in self._dict:
      st._dict[k] = self._dict[k]
    return st

  def find(self, name, install_inheriting=True):
    value = self._dict.get(name)
    if value: return value
    inherits = self._dict.get(_INHERITS_SYMBOL)
    if not inherits: return None
    excludes = self._dict.get(_INHERITS_EXCLUDES_SYMBOL)
    if excludes and name in excludes:
      return None
    value = inherits.find(name)
    if value == None: return value
    if self._frozen:
      raise Exception(
          "Can't copy inherited value of %s, symbol table frozen" % name)
    # Install locally before going on, so that the same
    # definition is consistently used.
    if install_inheriting:
      self._dict[name] = value
    return value

  def define(self, name, value, fail_if_defined = True):
    """Adds (name, value) pair to symbol table if not already defined.
       Returns True if added, otherwise False.
       """
    if self._dict.get(name):
      if fail_if_defined:
        raise Exception('%s: multiple definitions' % name)
      return False
    elif self._frozen:
      raise Exception("Can't assign %s, symbol table is frozen" % name)
    else:
      self._dict[name] = value
      return True

  def freeze(self):
    """Freeze symbol table, i.e. no longer allow assignments into the symbol
       table."""
    self._frozen = True

  def remove(self, name):
    self._dict.pop(name, None)

  def inherits(self, context, excludes):
    """Adds inheriting symbol table."""
    self.define(_INHERITS_SYMBOL, context)
    self.define(_INHERITS_EXCLUDES_SYMBOL, excludes)

  def disinherit(self):
    """Removes inheriting symbol tables."""
    # Install inheriting values not explicitly overridden.
    excludes = set([_INHERITS_EXCLUDES_SYMBOL, _INHERITS_SYMBOL])
    current_st = self
    inherits_st = current_st.find(_INHERITS_SYMBOL)
    while inherits_st:

      # Start by updating symbols to be excluded from the current
      # symbol table.
      inherits_excludes = current_st.find(
          _INHERITS_EXCLUDES_SYMBOL, install_inheriting=False)
      if inherits_excludes:
          for sym in inherits_excludes:
              excludes.add(sym)

      # Copy definitions in inherits to this, excluding symbols that
      # should not be inherited.
      for key in inherits_st.keys():
        if key not in excludes:

            # If the key defines a fields argument, remove references
            # to excluded fields.
            value = inherits_st.find(key)
            if key == 'fields' and isinstance(value, list):
                filtered_fields = []
                for field in value:
                    subfield = field
                    if isinstance(subfield, BitField):
                        subfield = subfield.name()
                    if (isinstance(subfield, IdRef)
                        and subfield.name() in excludes):
                        continue
                    filtered_fields.append(field)
                value = filtered_fields

            # Install value.
            self.define(key, value, fail_if_defined=False)
      current_st = inherits_st
      inherits_st = current_st.find(_INHERITS_SYMBOL)

    # Before returning, remove inheriting entries.
    self.remove(_INHERITS_EXCLUDES_SYMBOL)
    self.remove(_INHERITS_SYMBOL)

  def keys(self):
    return [k for k in self._dict.keys() if k != _INHERITS_SYMBOL]

  def sub_bit_exprs(self):
    exprs = set()
    for (key, value) in self._dict:
      _add_if_bitexpr(key, exprs)
      _add_if_bitexpr(value, exprs)
    return list(exprs)

  def __hash__(self):
    return hash(self.neutral_repr())

  def __cmp__(self, other):
    return (cmp(type(self), type(other)) or
            cmp(self.neutral_repr(), other.neutral_repr()))

  def neutral_repr(self):
    neutral_dict = {}
    for k in self._dict.keys():
      value = neutral_repr(self._dict[k])
      neutral_dict[k] = value
    return self._describe(neutral_dict)

  def __repr__(self):
    return self._describe(self._dict)

  def _describe(self, dict):
    dict_rep = '{'
    is_first = True
    for k in sorted(dict.keys()):
      if is_first:
        is_first = False
      else:
        dict_rep = '%s,%s  ' % (dict_rep, NEWLINE_STR)
      value = dict[k]
      # Try to better pretty-print lists.
      if isinstance(value, list) and len(repr(value)) > 60:
        value = '[' + ',\n    '.join([repr(v) for v in value]) + ']'
      dict_rep = "%s%s: %s" % (dict_rep, k, value)
    dict_rep += '}'
    return dict_rep

class BitPattern(BitExpr):
    """A pattern for matching strings of bits.  See parse() for
       syntax."""

    @staticmethod
    def parse(pattern, column):
        """Parses a string pattern describing some bits.  The string
           can consist of '1' and '0' to match bits explicitly, 'x' or
           'X' to ignore bits, '_' as an ignored separator, and an
           optional leading '~' to negate the entire pattern.
           Examples:

             10xx0
             1111_xxxx
             ~111x

        The pattern may also optionally be '-', which is equivalent to
        a sequence of 'xxx...xxx' of the requested width.

        Args:
            pattern: a string in the format described above.
            column: The tuple (name, hi, lo) defining a column.
        Returns:
            A BitPattern instance that describes the match, and is capable of
            transforming itself to a C expression.
        Raises:
            Exception: the input didn't meet the rules described above.
        """
        col = column.to_bitfield()
        hi_bit = col.hi()
        lo_bit = col.lo()
        num_bits = col.num_bits()
        # Convert - into a full-width don't-care pattern.
        if pattern == '-':
            return BitPattern.parse('x' * num_bits, column)

        # Derive the operation type from the presence of a leading
        # tilde.
        if pattern.startswith('~'):
            op = '!='
            pattern = pattern[1:]
        else:
            op = '=='

        # Allow use of underscores anywhere in the pattern, as a
        # separator.
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
            elif c.isalpha():  # covers both rule patterns and table patterns
                mask = mask << 1
                value = value << 1
            else:
                raise Exception('Invalid characters in pattern %s' % pattern)

        mask = mask << lo_bit
        value = value << lo_bit
        return BitPattern(mask, value, op, col)

    @staticmethod
    def parse_catch(pattern, column):
        """"Calls parse with given arguments, and catches exceptions
            raised. Prints raised exceptions and returns None.
        """
        try:
            return BitPattern.parse(pattern, column)
        except Exception as ex:
            print "Error: %s" % ex
            return None

    @staticmethod
    def always_matches(column=None):
      """Returns a bit pattern corresponding to always matches."""
      return BitPattern(0, 0, '==', column)

    def matches_any(self):
      """Returns true if pattern matches any pattern of bits."""
      return self.mask == 0

    def negate(self):
      """Returns pattern that is negation of given pattern"""
      if self.is_equal_op():
        return BitPattern(self.mask, self.value, '!=', self.column)
      else:
        return BitPattern(self.mask, self.value, '==', self.column)

    def __init__(self, mask, value, op, column=None):
        """Initializes a BitPattern.

        Args:
            mask: an integer with 1s in the bit positions we care about (e.g.
                those that are not X)
            value: an integer that would match our pattern, subject to the mask.
            op: either '==' or '!=', if the pattern is positive or negative,
                respectively.
            column: If specified, the corresponding column information for
                the bit pattern.
        """
        self.mask = mask
        self.value = value
        self.op = op
        self.column = column

        # Fail if we get something we don't know how to handle.
        if column:
          Good = isinstance(column, BitField)
          if isinstance(column, IdRef):
            Good = isinstance(column.value, BitField)
          if not Good:
            raise Exception(
                "Don't know how to generate bit pattern for %s" % column)

    def signif_bits(self):
      """Returns the number of signifcant bits in the pattern
         (i.e. occurrences of 0/1 in the pattern."""
      return _popcount(self.mask)

    def copy(self):
      """Returns a copy of the given bit pattern."""
      return BitPattern(self.mask, self.value, self.op, self.column)

    def union_mask_and_value(self, other):
      """Returns a new bit pattern unioning the mask and value of the
         other bit pattern."""
      return BitPattern(self.mask | other.mask, self.value | other.value,
                        self.op, self.column)

    def is_equal_op(self):
      """Returns true if the bit pattern is an equals (rather than a
         not equals)."""
      return self.op == '=='

    def conflicts(self, other):
        """Returns an integer with a 1 in each bit position that
           conflicts between the two patterns, and 0s elsewhere.  Note
           that this is only useful if the masks and ops match.
        """
        return (self.mask & self.value) ^ (other.mask & other.value)

    def is_complement(self, other):
        """Checks if two patterns are complements of each other.  This
           means they have the same mask and pattern bits, but one is
           negative.
        """
        return (self.op != other.op
            and self.mask == other.mask
            and self.value == other.value)

    def strictly_overlaps(self, other):
      """Checks if patterns overlap, and aren't equal."""
      return ((self.mask & other.mask) != 0) and (self != other)

    def is_strictly_compatible(self, other):
        """Checks if two patterns are safe to merge using +, but are
           not ==."""
        if self.is_complement(other):
            return True
        elif self.op == other.op:
            return (self.mask == other.mask
                and _popcount(self.conflicts(other)) == 1)
        return False

    def categorize_match(self, pattern):
      """ Compares this pattern againts the given pattern, and returns one
          of the following values:

          'match' - All specified bits in this match the corresponding bits in
          the given pattern.
          'conflicts' - There are bits in this pattern that conflict with the
          given pattern. Hence, there is no way this pattern will
          succeed for instructions matching the given pattern.
          'consistent' - The specified bits in this pattern neither match,
          nor conflicts with the unmatched pattern. No conclusions
          can be drawn from the overlapping bits of this and the
          given pattern.
          """
      if self.is_equal_op():
        # Compute the significant bits that overlap between this pattern and
        # the given pattern.
        mask = (self.mask & pattern.mask)
        if pattern.is_equal_op():
          # Testing if significant bits of this pattern differ (i.e. conflict)
          # with the given pattern.
          if mask & (self.value ^ pattern.value):
            # Conflicts, no pattern match.
            return 'conflicts'
          else:
            # Matches on signifcant bits in mask
            return 'match'
        else:
          # Test if negated given pattern matches the significant
          # bits of this pattern.
          if mask & (self.value ^ ~pattern.value):
            # Conflicts, so given pattern can't match negation. Hence,
            # this pattern succeeds.
            return 'match'
          else:
            # Consistent with negation. For now, we don't try any harder,
            # since it is not needed to add rule patterns to decoder table
            # rows.
            return 'consistent'
      else:
        # self match on negation.
        negated_self = self.copy()
        negated_self.op = '=='
        result = negated_self.categorize_match(pattern)
        if result == 'match':
          return 'match'
        else:
          # Not exact match. Can only assume they are consistent (since none
          # of the bits conflicted).
          return 'consistent'

    def remove_overlapping_bits(self, pattern):
      """Returns a copy of this with overlapping significant bits of this
         and the given pattern.
      """
      # Compute significant bits that overlap between this pattern and
      # the given pattern, and build a mask to remove those bits.
      mask = ~(self.mask & pattern.mask)

      # Now build a new bit pattern with overlapping bits removed.
      return BitPattern((mask & self.mask),
                        (mask & self.value),
                        self.op,
                        self.column)

    def __add__(self, other):
        """Merges two compatible patterns into a single pattern that matches
        everything either pattern would have matched.
        """
        assert (self == other) or self.is_strictly_compatible(other)

        if self.op == other.op:
            c = self.conflicts(other)
            return BitPattern((self.mask | other.mask) ^ c,
                (self.value | other.value) ^ c, self.op, self.column)
        else:
            return BitPattern.always_matches(self.column)

    def to_bool(self, options={}):
      # Generate expression corresponding to bit pattern.
      if self.mask == 0:
        value = 'true'
      else:
        inst = self.column.name().to_uint32(options)
        value = ('(%s & 0x%08X) %s 0x%08X'
                 % (inst, self.mask,
                    _COMPARE_OP_FORMAT % self.op,
                    self.value))
      return value

    def to_commented_bool(self, options={}):
      if not self.column and self.mask == 0:
        # No information is provided by the comment, so don't add!
        return 'true'
      return BitExpr.to_commented_bool(self, options)

    def bitstring(self):
      """Returns a string describing the bitstring of the pattern."""
      bits = self._bits_repr()
      if self.column:
        col = self.column.to_bitfield()
        bits = bits[col.lo() : col.hi() + 1]
      bits.reverse()
      return ''.join(bits)

    def __hash__(self):
       value = hash(self.mask) + hash(self.value) + hash(self.op)
       if self.column:
         value += hash(neutral_repr(self.column))
       return value

    def __cmp__(self, other):
        """Compares two patterns for sorting purposes.  We sort by
        - # of significant bits, DESCENDING,
        - then mask value, numerically,
        - then value, numerically,
        - and finally op.

        This is also used for equality comparison using ==.
        """
        return (cmp(type(self), type(other))
                or cmp(other.signif_bits(), self.signif_bits())
                or cmp(self.mask, other.mask)
                or cmp(self.value, other.value)
                or cmp(self.op, other.op)
                or cmp(neutral_repr(self.column), neutral_repr(other.column)))

    def first_bit(self):
      """Returns the index of the first 0/1 bit in the pattern. or
         None if no significant bits exist for the pattern.
      """
      for i in range(0, NUM_INST_BITS):
        if (self.mask >> i) & 1:
          return i
      return None

    def add_column_info(self, columns):
      """If the bit pattern doesn't have column information, add
         it based on the columns passed in. Otherwise return self.
      """
      if self.column: return self
      for c in columns:
        hi_bit = c.hi()
        lo_bit = c.lo()
        index = self.first_bit()
        if index is None : continue
        if index >= lo_bit and index <= hi_bit:
          return BitPattern(self.mask, self.value, self.op, c)
      return self

    def sub_bit_exprs(self):
      return [self.column]

    def _bits_repr(self):
        """Returns the 0/1/x's of the bit pattern as a list (indexed
           by bit position).
        """
        pat = []
        for i in range(0, NUM_INST_BITS):
            if (self.mask >> i) & 1:
                pat.append(`(self.value >> i) & 1`)
            else:
                pat.append('x')
        return pat

    def neutral_repr(self):
        if self.column:
          return '%s=%s%s' % (self.column.neutral_repr(),
                              '' if self.is_equal_op() else'~',
                              self.bitstring())
        elif self.is_equal_op():
          return self.bitstring()
        else:
          return "~%s" % self.bitstring()


    def __repr__(self):
        """Returns the printable string for the bit pattern."""
        if self.column:
          return '%s=%s%s' % (self.column,
                              '' if self.is_equal_op() else'~',
                              self.bitstring())
        elif self.is_equal_op():
          return self.bitstring()
        else:
          return "~%s" % self.bitstring()

TABLE_FORMAT="""
Table %s
%s
%s
"""
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
        self.default_row = None
        self._rows = []
        self._columns = []

    def columns(self):
      return self._columns[:]

    def add_column(self, column):
        """Adds a column to the table. Returns true if successful."""
        for col in self._columns:
          if repr(col) == repr(column):
            return False
        self._columns.append(column)
        return True

    def rows(self, default_also = True):
        """Returns all rows in table (including the default row
           as the last element if requested).
        """
        r = self._rows[:]
        if default_also and self.default_row:
          r.append(self.default_row)
        return r

    def add_default_row(self, action):
        """Adds a default action to use if none of the rows apply.
           Returns True if able to define.
        """
        if self.default_row: return False
        self.default_row = Row([BitPattern.always_matches()], action)
        return True

    def add_row(self, patterns, action):
        """Adds a row to the table.
        Args:
            patterns: a list containing a BitPattern for every column in the
                table.
            action: The action associated  with the row. Must either be
                a DecoderAction or a DecoderMethod.
        """
        row = Row(patterns, action)
        self._rows.append(row)
        return row

    def remove_table(self, name):
      """Removes method calls to the given table name from the table"""
      for row in self._rows:
        row.remove_table(name)

    def define_pattern(self, pattern, column):
        """Converts the given input pattern (for the given column) to the
           internal form. Returns None if pattern is bad.
        """
        if column >= len(self._columns): return None
        return BitPattern.parse_catch(pattern, self._columns[column])

    def copy(self):
      """Returns a copy of the table."""
      table = Table(self.name, self.citation)
      table._columns = self._columns
      for r in self._rows:
        table.add_row(r.patterns, r.action)
      if self.default_row:
        table.add_default_row(self.default_row.action)
      return table

    def row_filter(self, filter):
      """Returns a copy of the table, filtering each row with the
         replacement row defined by function argument filter (of
         form: lambda row:).
         """
      table = Table(self.name, self.citation)
      table._columns = self._columns
      for r in self._rows:
        row = filter(r)
        if row:
          table.add_row(row.patterns, row.action)
      if self.default_row:
        row = filter(self.default_row)
        if row:
          table.add_default_row(row.action)
      return table

    def action_filter(self, names):
        """Returns a table with DecoderActions reduced to the given field names.
           Used to optimize out duplicates, depending on context.
        """
        return self.row_filter(
            lambda r: Row(r.patterns, r.action.action_filter(names)))

    def add_column_to_rows(self, rows):
      """Add column information to each row, returning a copy of the rows
         with column information added.
      """
      new_rows = []
      for r in rows:
        new_patterns = []
        for p in r.patterns:
          new_patterns.append(p.add_column_info(self._columns))
        new_rows.append(Row(new_patterns, r.action))
      return new_rows

    def methods(self):
      """Returns the (sorted) list of methods called by the table."""
      methods = set()
      for r in self.rows(True):
        if r.action.__class__.__name__ == 'DecoderMethod':
          methods.add(r.action)
      return sorted(methods)

    def __repr__(self):
      rows = list(self._rows)
      if self.default_row:
        rows.append(self.default_row)
      return TABLE_FORMAT % (self.name,
                             ' '.join([repr(c) for c in self._columns]),
                             NEWLINE_STR.join([repr(r) for r in rows]))

# Defines a mapping from decoder action field names, to the
# corresponding type of the expression. The domain is a field
# name. The range is a list of type names defined in
# BitExpr.to_type. Otherwise it must be a function (taking a single
# argument) which does the type checking.  Note: This is filled
# dynamically during import time, so that circular dependencies can be
# handled. In particular, dgen_decoder.py fills in types for method
# fields.
_DECODER_ACTION_FIELD_TYPE_MAP = {}

def DefineDecoderFieldType(name, type):
    """Adds the corresponding type association to the list of known
       types for decoder fields."""
    global _DECODER_ACTION_FIELD_TYPE_MAP
    types = _DECODER_ACTION_FIELD_TYPE_MAP.get(name)
    if types == None:
        types = set()
        _DECODER_ACTION_FIELD_TYPE_MAP[name] = types
    types.add(type)

class DecoderAction:
  """An action defining a class decoder to apply.
     Fields are:
       _st - Symbol table of other information stored on the decoder action.
       _neutral_st - Symbol table to use for neutral_repr. Note: This
             symbol table is the same as _st, except when method action_filter
             is called. In the latter case, it has the minimal needed fields
             (based on the filter) so that the neutral representation does
             not get messed up with aliases added to _st during filtering.
  """
  def __init__(self, baseline=None, actual=None):
    self._st = SymbolTable()
    self._neutral_st = self._st
    if baseline != None:
      self._st.define('baseline', baseline)
    if actual != None:
      self._st.define('actual', actual)

    # The following field is set by method force_type_checking, and is
    # used to force type checking while parsing a decoder action. This
    # allows the parser to report problems at the corresponding source
    # line that defined the value of a field to something we don't
    # understand.
    self._force_type_checking = False

  def force_type_checking(self, value):
      """Sets field defining if type checking will be done as symbols
         are added to the decoder action. This allows the parser to
         report problems at the corresponding source line that defined
         the field."""
      self._force_type_checking = value

  def find(self, name, install_inheriting=True):
    return self._st.find(name, install_inheriting)

  def define(self, name, value, fail_if_defined=True):
    if self._force_type_checking:
        types = _DECODER_ACTION_FIELD_TYPE_MAP.get(name)
        if types:
            # Now try translating value for each type, so that
            # if there is a problem, a corresponding exception
            # will be raised.
            for type in types:
                if isinstance(type, str):
                    if not isinstance(value, BitExpr):
                        raise Exception(
                            "Defining %s:%s. Value must be BitExpr" %
                            (name, value))
                    value.to_type(type)
                else:
                    type(value)
    return self._st.define(name, value, fail_if_defined)

  def freeze(self):
      """Don't allow any modifications of fields (unless copying)."""
      self._st.freeze()

  def remove(self, name):
    self._st.remove(name)

  def inherits(self, context, excludes):
    self._st.inherits(context, excludes)

  def disinherit(self):
    self._st.disinherit()

  def keys(self):
    return self._st.keys()

  def copy(self):
    """Returns a copy of the decoder action."""
    action = DecoderAction()
    action._st = SymbolTable()
    action._neutral_st = action._st
    for field in self._st.keys():
      action.define(field, self.find(field))
    return action

  def action_filter(self, names):
    """Filters fields in the decoder to only include fields in names.
       for most operations, we build a symbol table (_st) that contains
       not only the specified fields, but any implicit dependent fields.
       For method neutral_repr, we create a special symbol table _neutral_st
       that only contains the fields specified.

       Note: actual and actual-not-baseline are handled specially. See
       code of function body for details.
       """
    action = DecoderAction()
    action._st = SymbolTable()
    action._neutral_st = SymbolTable()

    # Copy direct values in.
    values = set()
    for n in names:
      name = n
      if name == 'actual':
        # Check for special case symbol that copies the baseline if the
        # actual field is not defined.
        value = self.actual()
      elif name == 'actual-not-baseline':
        # Check for special case symbol that defines actual only if
        # the actual is different than the baseline.
        actual = self.actual()
        if actual and actual != self.baseline():
          name = 'actual'
          value = actual
        else:
          value = None
      else:
        # Copy over if defined.
        value = self._st.find(n)
      if value != None:
        # Copy to both the copy, and the neutral form.
        action._st.define(name, value, fail_if_defined=False)
        neutral_value = value
        if name == 'safety':
          # To get better compression of actual decoders,
          # merge and order alternatives.
          neutral_value = set()
          strs = set()
          for v in value:
            if isinstance(v, BitExpr):
              neutral_value.add(v)
            else:
              strs.add(v)
          neutral_value = sorted(neutral_value) + sorted(strs)
        action._neutral_st.define(name, neutral_value, fail_if_defined=False)
        _add_if_bitexpr(value, values)

    # Collect sub expressions (via closure) and add names of id refs
    # if applicable.
    _close_referenced_bit_exprs(values)
    for v in values:
      if isinstance(v, IdRef):
        v_name = v.name()
        v_value = self._st.find(v_name)
        if v_value:
          action._st.define(v_name, v_value, fail_if_defined=False)

    # Fill in fields.
    fields = self._st.find('fields')
    if fields:
      new_fields = []
      for f in fields:
        if f in values:
          new_fields.append(f)
      action._st.define('fields', new_fields, fail_if_defined=False)

    action._st.freeze()
    action._neutral_st.freeze()
    return action

  def actual(self):
    """Returns the actual decoder class defined for the action if defined.
       Otherwise, returns the baseline."""
    act = self.find('actual')
    return act if act != None else self.baseline()

  def baseline(self):
    """Returns the baseline decoder class defined for the action."""
    return self.find('baseline')

  def pattern(self):
    """Returns the pattern associated with the action."""
    return self.find('pattern')

  def rule(self):
    """Returns the rule associated with the action."""
    return self.find('rule')

  def safety(self):
    """Returns the safety associated with the action."""
    s = self.find('safety')
    return s if s else []

  def defs(self):
    """Returns the defs defined for the instruction, or None if undefined."""
    return self.find('defs')

  def __eq__(self, other):
    return (type(self) == type(other) and
            neutral_repr(self) == neutral_repr(other))

  def __cmp__(self, other):
    # Order lexicographically on type/fields.
    return cmp(type(self), type(other)) or cmp(neutral_repr(self._st),
                                               neutral_repr(other._st))

  def __hash__(self):
    return hash(self.neutral_repr())

  def __repr__(self):
    return self._describe(repr(self._st))

  def neutral_repr(self):
    return self._describe(neutral_repr(self._neutral_st))

  def _describe(self, text):
    return "= %s" % text.replace(NEWLINE_STR, NEWLINE_STR + '  ').rstrip(' ')

class DecoderMethod(object):
  """An action defining a parse method to call.

  Corresponds to the parsed decoder method:
      decoder_method ::= '->' id

  Fields are:
      name - The name of the corresponding table (i.e. method) that
          should be used to continue searching for a matching class
          decoder.
  """
  def __init__(self, name):
    self.name = name

  def action_filter(self, unused_names):
    return self

  def copy(self):
    """Returns a copy of the decoder method."""
    return DecoderMethod(self.name)

  def __eq__(self, other):
    return (self.__class__.__name__ == 'DecoderMethod'
            and self.name == other.name)

  def __cmp__(self, other):
    # Lexicographically compare types/fields.
    return (cmp(type(self), type(other)) or
            cmp(self.name, other.name))

  def __hash__(self):
    return hash('DecoderMethod') + hash(self.name)

  def __repr__(self):
    return '-> %s' % self.name

class Row(object):
    """ A row in a Table."""
    def __init__(self, patterns, action):
        """Initializes a Row.
        Args:
            patterns: a list of BitPatterns that must match for this Row to
                match.
            action: the action to be taken if this Row matches.
        """
        self.patterns = patterns
        self.action = action

        self.significant_bits = 0
        for p in patterns:
            self.significant_bits += p.signif_bits()

    def add_pattern(self, pattern):
        """Adds a pattern to the row."""
        self.patterns.append(pattern)

    def remove_table(self, name):
      """Removes method call to the given table name from the row,
         if applicable.
         """
      if (isinstance(self.action, DecoderMethod) and
          self.action.name == name):
        self.action = DecoderAction('NotImplemented', 'NotImplemented')

    def strictly_overlaps_bits(self, bitpat):
      """Checks if bitpat strictly overlaps a bit pattern in the row."""
      for p in self.patterns:
        if bitpat.strictly_overlaps(p):
          return True
      return False

    def can_merge(self, other):
        """Determines if we can merge two Rows."""
        if self.action != other.action:
            return False

        equal_columns = 0
        compat_columns = 0
        for (a, b) in zip(self.patterns, other.patterns):
            if a == b:
                equal_columns += 1
            # Be sure the column doesn't overlap with other columns in pattern.
            if (not self.strictly_overlaps_bits(a) and
                not other.strictly_overlaps_bits(b) and
                a.is_strictly_compatible(b)):
                compat_columns += 1

        cols = len(self.patterns)
        return (equal_columns == cols
            or (equal_columns == cols - 1 and compat_columns == 1))

    def copy_with_action(self, action):
      return Row(self.patterns, action)

    def copy_with_patterns(self, patterns):
      return Row(patterns, self.action)

    def __add__(self, other):
        assert self.can_merge(other)  # Caller is expected to check!
        return Row([a + b for (a, b) in zip(self.patterns, other.patterns)],
            self.action)

    def __cmp__(self, other):
        """Compares two rows, so we can order pattern matches by specificity.
        """
        return (cmp(type(self), type(other))
                or cmp(self.patterns, other.patterns)
                or cmp(self.action, other.action))

    def __repr__(self):
      return self._describe([repr(p) for p in self.patterns], repr(self.action))

    def neutral_repr(self):
      return self._describe([neutral_repr(p) for p in self.patterns],
                            neutral_repr(self.action))

    def _describe(self, patterns, action):
        return (ROW_PATTERN_ACTION_FORMAT %
                (' & '.join(patterns), action.replace(NEWLINE_STR,
                                                      ROW_ACTION_INDENT)))

ROW_PATTERN_ACTION_FORMAT="""%s
    %s"""

ROW_ACTION_INDENT="""
   """

class Decoder(object):
  """Defines a class decoder which consists of set of tables.

  A decoder has a primary (i.e. start) table to parse intructions (and
  select the proper ClassDecoder), as well as a set of additional
  tables to complete the selection of a class decoder. Instances of
  this class correspond to the internal representation of parsed
  decoder tables recognized by dgen_input.py (see that file for
  details).

  Fields are:
      primary - The entry parse table to find a class decoder.
      tables - The (sorted) set of tables defined by a decoder.
      value_map - Saved values of the decoder.

  Note: maintains restriction that tables have unique names.
  """

  def __init__(self):
    self.primary = None
    self._is_sorted = False
    self._tables = []
    self._value_map = {}

  def value_keys(self):
    return self._value_map.keys()

  def define_value(self, name, value):
      """Associate value with name, for the given decoder."""
      self._value_map[name] = value

  def get_value(self, name, default_value=None):
      """Returns the associated value with the given name. Use the
         default if the name is not bound."""
      return self._value_map.get(name, default_value)

  def add(self, table):
    """Adds the table to the set of tables. Returns true if successful.
    """
    if filter(lambda(t): t.name == table.name, self._tables):
      # Table with name already defined, report error.
      return False
    else:
      if not self._tables:
        self.primary = table
      self._tables.append(table)
      self.is_sorted = False
      return True

  def tables(self):
    """Returns the sorted (by table name) list of tables"""
    if self._is_sorted: return self._tables
    self._tables = sorted(self._tables, key=lambda(tbl): tbl.name)
    self._is_sorted = True
    return self._tables

  def table_names(self):
    """Returns the names of all tables in the decoder."""
    return sorted([tbl.name for tbl in self.tables()])

  def get_table(self, name):
    """Returns the table with the given name"""
    for tbl in self._tables:
      if tbl.name == name:
        return tbl
    return None

  def remove_table(self, name):
    """Removes the given table from the decoder"""
    new_tables = []
    for table in self._tables:
      if table.name != name:
        new_tables = new_tables + [table]
        table.remove_table(name)
    self._tables = new_tables

  def table_filter(self, filter):
    """Returns a copy of the decoder, filtering each table with
      the replacement row defined by function argument filter (of
      form: lambda table:).

      Note: The filter can't change the name of the primary table.
      """
    decoder = Decoder()

    tables = set()
    for tbl in self._tables:
      filtered_table = filter(tbl)
      if filtered_table != None:
        tables.add(filtered_table)
        if tbl.name == self.primary.name:
          decoder.primary = filtered_table
      elif tbl.name == self.primary.name:
        raise Exception("table_filter: can't filter out table %s" %
                        self.primary.name)
    decoder._tables = sorted(tables, key=lambda(tbl): tbl.name)
    decoder._value_map = self._value_map.copy()
    return decoder

  def action_filter(self, names):
    """Returns a new set of tables, with actions reduced to the set of
      specified field names.
    """
    # Filter actions in tables.
    decoder = self.table_filter(lambda tbl: tbl.action_filter(names))

    # Now filter other decoders associated with the specification.
    for key in decoder.value_keys():
      action = decoder.get_value(key)
      if isinstance(action, DecoderAction):
        decoder.define_value(key, action.action_filter(names))
    return decoder

  def decoders(self):
    """Returns the sorted sequence of DecoderAction's defined in the tables."""
    decoders = set()

    # Add other decoders associated with specification, but not in tables.
    for key in self.value_keys():
      action = self.get_value(key)
      if isinstance(action, DecoderAction):
        decoders.add(action)

    # Add decoders specified in the tables.
    for t in self._tables:
        for r in t.rows(True):
            if isinstance(r.action, DecoderAction):
              decoders.add(r.action)
    return sorted(decoders)

  def rules(self):
    """Returns the sorted sequence of DecoderActions that define
       the rule field.
    """
    return sorted(filter(lambda (r): r.rule, self.decoders()))

  def show_table(self, table):
    tbl = self.get_table(table)
    if tbl != None:
      print "%s" % tbl
    else:
      raise Exception("Can't find table %s" % table)

def _TranslateSignExtend(exp, return_type, options):
  """Implements SignExtend(x, i):
     if i == len(x) then x else Replicate(TopBit(x), i - len(x)):x
     where i >= len(x)
  """
  # Note: Returns None if not translatible.
  args = _ExtractExtendArgs(exp, return_type)
  if args == None: return None
  if exp.name() != 'SignExtend': return None
  (i, x, len_x) = args
  value_x = x.to_uint32(options)
  if i == len_x:
    return value_x
  else:
    top_bit_mask = 1
    for n in range(1, len_x):
      top_bit_mask = top_bit_mask << 1
    replicate_mask = 0
    for n in range(0, i - len_x):
      replicate_mask = (replicate_mask << 1) | 1
    replicate_mask = replicate_mask << len_x
    text = ("""(((%s) & 0x%08X)
       ? ((%s) | 0x%08X)
       : %s)""" %
            (value_x, top_bit_mask, value_x, replicate_mask, value_x))
    return text

def _TranslateZeroExtend(exp, return_type, options):
  """Implements ZeroExtend(x, i):
       if i == len(x) then x else Replicate('0', i-len(x)):x
  """
  args = _ExtractExtendArgs(exp, return_type)
  if args == None: return None
  if exp.name() != 'ZeroExtend': return None
  # Note: Converting to unsigned integer is the same as extending.
  return x.to_uint32(options)

def _ToBitFieldExtend(exp, return_type, options):
  """Implements a to_bitfield conversion for ZeroExtend and
     SignExtend.
  """
  args = _ExtractExtendArgs(exp, return_type)
  if not args or exp.name() not in ['ZeroExtend', 'SignExtend']:
    return None
  i, x, len_x = args
  return BitField(exp, len_x - 1, 0)

def _ExtractExtendArgs(exp, return_type):
  """Returns (i, x, len(x)) if exp is one of the following forms:
        XXX(x, i)
        XXX(x, i)
     Otherwise, returns None.
     """
  if not isinstance(exp, FunctionCall): return None
  if return_type != 'uint32': return None
  if len(exp.args()) != 2: return None
  args = exp.args()
  x = args[0]
  i = args[1]
  try:
    bf = x.to_bitfield()
  except:
    return None
  if not isinstance(i, Literal): return None
  i = i.value()
  if i < 0 or i > NUM_INST_BITS: return None
  len_x = bf.num_bits()
  if i < len_x: return None
  return (i, x, len_x)

# Holds the set of installed parameters. Map is from parameter name,
# to the previous value defined in _FUNCTION_TRANSLATION_MAP.
_INSTALLED_PARAMS_MAP = {}

def InstallParameter(name, type):
  """Installs parameter in as a preprocessing fuction of no arguments."""
  global _INSTALLED_PARAMS_MAP
  global _FUNCTION_TRANSLATION_MAP
  installed = _FUNCTION_TRANSLATION_MAP.get(name)
  _INSTALLED_PARAMS_MAP[name] = installed
  if installed:
    installed = list(installed)
  else:
    installed = []
  installed.insert(0, _BuildParameter(name, type))
  _FUNCTION_TRANSLATION_MAP[name] = installed

def UninstallParameter(name):
  """Restores the function translation map to its former state before
     the previous call to InstallParameter with the given name.
     """
  global _INSTALLED_PARAMS_MAP
  global _FUNCTION_TRANSLATION_MAP
  _FUNCTION_TRANSLATION_MAP[name] = _INSTALLED_PARAMS_MAP[name]
  _INSTALLED_PARAMS_MAP[name] = None

def _BuildParameter(name, type):
  """Builds a parameter translation function for the correspondin
     parameter macro.
     """
  return (lambda exp, return_type, options:
            (name if exp.matches_signature(([], type), return_type) != None
             else None))

"""Defines special processing fuctions if the signature matches."""
_FUNCTION_TRANSLATION_MAP = {
    'ZeroExtend': [_TranslateZeroExtend, _ToBitFieldExtend],
    'SignExtend': [_TranslateSignExtend, _ToBitFieldExtend],
}
