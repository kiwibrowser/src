#!/usr/bin/python2.6
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LICENSE = (
  "/*",
  " * Copyright (c) 2011 The Native Client Authors. All rights reserved.",
  " * Use of this source code is governed by a BSD-style license that can be",
  " * found in the LICENSE file.",
  " */")

#############################################################################
#
# This script generates the calling_conv test modules.
#
# The following features are checked by this test:
#
#  1) Variable arguments
#  2) Argument alignments
#  3) Large numbers of arguments
#     (on X86-64, this tests the overflow_area mechanism)
#  4) Passing va_list between functions.
#  5) va_copy
#
# To generate the test, just run: ./generate.py
#
#############################################################################

import random
import string
import sys

def GenerateTypeInfo(settings):
  type_info = [
    #   id/cast   format  Maker      CompareExpr  AssignStmt
    #########################################################################
    # standard types
    ('t_charp',    'C',  make_string , '%s == %s'  , '%s = %s;'  ),
    ('t_int',      'i',  make_int    , '%s == %s'  , '%s = %s;'  ),
    ('t_long',     'l',  make_long   , '%s == %s'  , '%s = %s;'  ),
    ('t_llong',    'L',  make_llong  , '%s == %s'  , '%s = %s;'  ),
    ('t_double',   'd',  make_double , '%s == %s'  , '%s = %s;'  ),
    ('t_ldouble',  'D',  make_ldouble, '%s == %s'  , '%s = %s;'  ),
    ('t_char',     'c',  make_char   , '%s == %s'  , '%s = %s;'  ),
    ('t_short',    's',  make_short  , '%s == %s'  , '%s = %s;'  ),
    ('t_float',    'f',  make_float  , '%s == %s'  , '%s = %s;'  ),
    # custom types
    ('t_tiny',     'T',  make_tiny,  'TINY_CMP(%s,%s)', 'SET_TINY(%s, %s);' ),
    ('t_big',      'B',  make_big,   'BIG_CMP(%s,%s)', 'SET_BIG(%s, %s);'     ),
  ]

  # These types cannot be used directly in va_arg().
  va_arg_exclude = [ 't_char', 't_short', 't_float' ]
  all_exclude = []

  if not settings.allow_struct:
    all_exclude += [ 't_tiny', 't_big' ]

  if not settings.allow_double:
    all_exclude += [ 't_double', 't_ldouble' ]

  if not settings.allow_float:
    all_exclude += [ 't_float']

  if not settings.allow_struct_va_arg:
    va_arg_exclude += [ 't_tiny', 't_big' ]

  settings.all_types = [ CType(*args) for args in type_info
                         if args[0] not in all_exclude]
  settings.va_arg_types = [ t for t in settings.all_types
                            if t.id not in va_arg_exclude ]
  # See also the generated comments in the generated .c files for the settings.
  print 'Generating with normal arg types: %s' % str(settings.all_types)
  print 'Generating with var arg types: %s' % str(settings.va_arg_types)



class CType(object):
  def __init__(self, id, format, maker, compare_expr, assign_stmt):
    self.id = id
    self.format = format
    self.maker = maker
    self.compare_expr = compare_expr
    self.assign_stmt = assign_stmt

  def __repr__(self):
    return self.id

class Settings(object):
  def __init__(self):
    # If the seed is not specified on the command line, choose a random value.
    self.seed = random.getrandbits(64)
    self.num_functions = 0
    self.calls_per_func = 0
    self.max_args_per_func = 0
    self.num_modules = 0
    self.allow_struct = 1
    self.allow_double = 1
    self.allow_float = 1
    self.allow_struct_va_arg = 1
    self._script_argv = None

  def fixtypes(self):
    self.seed = long(self.seed)
    self.num_functions = int(self.num_functions)
    self.calls_per_func = int(self.calls_per_func)
    self.max_args_per_func = int(self.max_args_per_func)
    self.num_modules = int(self.num_modules)
    self.allow_struct = bool(int(self.allow_struct))
    self.allow_double = bool(int(self.allow_double))
    self.allow_float = bool(int(self.allow_float))
    self.allow_struct_va_arg = bool(int(self.allow_struct_va_arg))

  def set(self, k, v):
    if not hasattr(self, k):
      return False
    setattr(self, k, v)
    return True

def Usage():
  print "Usage: %s <options> --" % sys.argv[0],
  print "<golden_output_filename> <module0_filename> <module1_filename> ..."
  print
  print "Valid options are:"
  print "  --seed=<64-bit-random-seed>"
  print "  --num_functions=<num_functions>"
  print "  --calls_per_func=<calls_per_function>"
  print "  --max_args_per_func=<max_args_per_function>"
  print "  --num_modules=<num_modules>"
  print "  --allow_struct=<0|1>         Test struct arguments (by value)"
  print "  --allow_double=<0|1>         Test double arguments (by value)"
  print "  --allow_float=<0|1>          Test float arguments (by value)"
  print "  --allow_struct_va_arg=<0|1>  Test va_arg on struct arguments"
  sys.exit(1)

def main(argv):
  if len(argv) == 0:
    Usage()

  settings = Settings()
  for i, arg in enumerate(argv):
    if arg == '--':
      break
    if arg.startswith('--'):
      k,v = arg[2:].split('=')
      if not settings.set(k,v):
        Fatal("Unknown setting: %s", k)
    else:
      Usage()
  settings.fixtypes()
  settings._script_argv = ' '.join(argv)

  # The parameters after "--" are the golden output file, followed
  # by the modules files
  if (len(argv) - i - 1) != 1+settings.num_modules:
    Fatal("Incorrect number of parameters after --")
  output_golden_file = argv[i+1]
  output_module_files = argv[i+2:]

  GenerateTypeInfo(settings)

  functions, calls, num_asserts = GenerateTest(settings)

  # Randomly separate the calls and functions among the modules
  functions_for_module = randsplit(functions, settings.num_modules)
  calls_for_module = randsplit(calls, settings.num_modules)

  for m in xrange(settings.num_modules):
    fp = WriteBuffer(open(output_module_files[m], 'w'))
    m = Module(settings, m, functions_for_module[m], calls_for_module[m],
               functions)
    m.emit(fp)
    fp.close()

  print "callingconv seed: %d" % settings.seed
  fp = open(output_golden_file, 'w')
  fp.write("generate.py arguments: %s\n" % settings._script_argv)
  fp.write("SUCCESS: %d calls OK.\n" % num_asserts)
  fp.close()

  return 0

def Fatal(m, *args):
  if len(args) > 0:
    m = m % args
  print m
  sys.exit(2)

class WriteBuffer(object):
  def __init__(self, fp):
    self.fp = fp
    self.buffer = ''

  def write(self, data):
    self.buffer += data

  def flush(self):
    self.fp.write(self.buffer)
    self.buffer = ''

  def close(self):
    self.flush()
    self.fp.close()


def randsplit(inlist, n):
  """ Randomly split a list into n sublists. """
  inlist = list(inlist)
  random.shuffle(inlist)
  selections = [ (random.randint(0, n-1), m) for m in inlist ]
  sublists = [ [] for i in xrange(n) ]
  for (i, m) in selections:
    sublists[i].append(m)
  return sublists


class Module(object):
  def __init__(self, settings, id, functions, calls, all_functions):
    self.settings = settings
    self.id = id
    self.calls = calls
    self.functions = functions
    self.all_functions = all_functions

  def emit(self, out):
    out.write('\n'.join(LICENSE))
    out.write('\n')

    out.write("/*--------------------------------------------------*\n"
              " *             THIS FILE IS AUTO-GENERATED          *\n"
              " *              DO NOT MODIFY THIS FILE             *\n"
              " *            MODIFY  'generate.py'  instead        *\n"
              " *--------------------------------------------------*/\n")
    out.write("\n")

    # only emit this for the first module we generate
    if self.id == 0:
      out.write("const char *script_argv =\n")
      out.write("  \"" + CEscape(self.settings._script_argv) + "\";\n")

    out.write("/*--------------------------------------------------*\n"
              " * Generator Settings:                              *\n")
    for k,v in self.settings.__dict__.iteritems():
      if k.startswith('_'):
        continue
      lines = WordWrap(str(v), 22)
      for i, s in enumerate(lines):
        if i == 0:
          out.write(" *  %-21s = %-22s  *\n" % (k, s))
        else:
          out.write(" *  %-21s    %-22s *\n" % ('', s))
    out.write(" *--------------------------------------------------*/\n")
    out.write("\n")

    out.write('#include <stdio.h>\n')
    out.write('#include <stdlib.h>\n')
    out.write('#include <string.h>\n')
    out.write('#include <stdarg.h>\n')
    out.write('#include <callingconv.h>\n')
    out.write("\n")

    # Emit function prototypes (all of them)
    for f in self.all_functions:
      f.emit_prototype(out)
    out.write("\n")

    # Emit prototypes of the vcheck functions
    for module_id in xrange(self.settings.num_modules):
      out.write("void vcheck%d(va_list ap, int i, char type);\n" % module_id)
    out.write("\n")

    # Emit the main module function
    out.write("void module%d(void) {\n" % self.id)
    out.write("  SET_CURRENT_MODULE(%d);\n" % self.id)
    out.write("\n")
    for c in self.calls:
      c.emit(out)
    out.write("}\n\n")

    # Emit the "vcheck" function, for checking
    # va_list passing.
    out.write("void vcheck%d(va_list ap, int i, char type) {\n" % self.id)
    for t in self.settings.va_arg_types:
        va_arg_val = "va_arg(ap, %s)" % t.id
        expected_val = "v_%s[i]" % t.id
        comparison = t.compare_expr % (va_arg_val, expected_val)
        out.write("  if (type == '%s')\n" % t.format)
        out.write("    ASSERT(%s);\n" % comparison)
        out.write("")
    out.write("}\n\n")

    # Emit the function definitions in this module
    for f in self.functions:
      f.emit(out)
    out.write("\n")

def CEscape(s):
  s = s.replace('\\', '\\\\')
  s = s.replace('"', '\\"')
  return s

def WordWrap(s, linelen):
  words = s.split(' ')
  lines = []
  line = ''
  spaces = 0
  for w in words:
    if len(line) + spaces + len(w) > linelen:
      lines.append(line)
      line = ''
      spaces = 0
    line += (' ' * spaces) + w
    spaces = 1
  lines.append(line)
  return lines

def GenerateTest(settings):
  random.seed(settings.seed)

  functions = [ TestFunction(settings, i)
                for i in xrange(settings.num_functions) ]

  calls = [ ]
  callcount = 0
  for f in functions:
    for i in xrange(settings.calls_per_func):
      calls.append(TestCall(settings, callcount, f))
      callcount += 1

  num_asserts = sum([ c.num_asserts for c in calls ])
  return (functions, calls, num_asserts)


class TestCall(object):
  def __init__(self, settings, call_id, function):
    self.settings = settings
    self.id = call_id
    self.f = function

    self.num_var_args = random.randint(0, self.settings.max_args_per_func -
                                          self.f.num_fixed_args)

    # Generate argument inputs
    self.num_args = self.f.num_fixed_args + self.num_var_args
    args = []
    fmtstr = ''
    for i in xrange(self.num_args):
        if i < self.f.num_fixed_args:
            t = self.f.fixed_arg_types[i]
        else:
            t = random.choice(settings.va_arg_types)
        if i == self.f.num_fixed_args:
            fmtstr += '" "'
        fmtstr += str(t.format)
        args.append((i, t, t.maker()))
    self.args = args
    self.fmtstr = fmtstr

    # Each fixed argument is checked once.
    # Each variable argument is checked once by the test function,
    # and once by each vcheck() call. (one per module)
    self.num_asserts = (self.f.num_fixed_args +
                        self.num_var_args * (settings.num_modules + 1))

  def emit(self, out):
    out.write("  /* C%d */\n" % self.id)
    out.write("  SET_CURRENT_CALL(%d);\n" % self.id)

    # Set the check variables
    for (i, t, value) in self.args:
      var = "v_%s[%d]" % (t.id, i)
      setexpr = t.assign_stmt % (var, value)
      out.write(prettycode('  ' + setexpr) + '\n')

    # Call the function
    argstr = ''
    for (i, t, value) in self.args:
      argstr += ', v_%s[%d]' % (t.id, i)
    callstr = '  F%s("%s"%s);\n' % (self.f.id, self.fmtstr, argstr)
    out.write(prettycode(callstr))
    out.write("\n")


def prettycode(str):
  """Try to beautify a line of code."""

  if len(str) < 80:
    return str

  lparen = str.find('(')
  rparen = str.rfind(')')
  if lparen < 0 or rparen < 0:
    Fatal("Invalid code string")
  head = str[ 0 : lparen ]
  inner = str[ lparen+1 : rparen ]
  tail = str[ rparen+1 : ]
  args = inner.split(",")
  ret = head + "("
  pos = len(ret)
  indent = len(ret)
  firstarg = True
  for arg in args:
      if firstarg:
          s = arg.strip()
          firstarg = False
      elif pos + len(arg) < 75:
          s = ", " + arg.strip()
      else:
          ret += ",\n" + (' '*indent)
          pos = indent
          s = arg.strip()
      ret += s
      pos += len(s)
  ret += ")" + tail
  return ret



# Represents a randomly generated test function
class TestFunction(object):
  def __init__(self, settings, id):
    self.settings = settings
    self.id = id
    self.num_fixed_args = random.randint(0, self.settings.max_args_per_func)
    self.fixed_arg_types = range(self.num_fixed_args)
    for i in xrange(len(self.fixed_arg_types)):
      self.fixed_arg_types[i] = random.choice(settings.all_types)


  def emit_prototype(self, out, is_def = False):
    fixed_arg_str = ''
    argnum = 0
    for t in self.fixed_arg_types:
      fixed_arg_str += ", %s a_%d" % (t.id, argnum)
      argnum += 1

    prototype = "void F%s(const char *fmt%s, ...)" % (self.id, fixed_arg_str)
    out.write(prettycode(prototype))

    if is_def:
      out.write(" {\n")
    else:
      out.write(";\n")
      out.write("\n")

  def emit(self, out):
    self.emit_prototype(out, True)

    out.write("  va_list ap;\n")
    out.write("  va_list ap2;\n")
    out.write("  int i = 0;\n")
    out.write("\n")

    out.write("  SET_CURRENT_FUNCTION(%d);\n" % self.id)
    out.write("  SET_INDEX_VARIABLE(i);\n")
    out.write("\n")

    if self.num_fixed_args > 0:
      out.write("  /* Handle fixed arguments */\n")
      for (i, t) in enumerate(self.fixed_arg_types):
        arg_val   = "a_%d" % i
        check_val = "v_%s[i]" % (t.id)
        comp_expr = t.compare_expr % (arg_val, check_val)
        out.write("  ASSERT(%s); i++;\n" % (comp_expr,))
      out.write("\n")
      # The last fixed argument
      last_arg = arg_val
    else:
      last_arg = "fmt"

    # Handle variable arguments
    out.write("  /* Handle variable arguments */\n")
    out.write("  va_start(ap, %s);\n" % last_arg)
    out.write("  while (fmt[i]) {\n")
    out.write("    switch (fmt[i]) {\n")

    for t in self.settings.va_arg_types:
        arg_val   = "va_arg(ap, %s)" % t.id
        check_val = "v_%s[i]" % t.id
        comp_expr = t.compare_expr % (arg_val, check_val)
        out.write("    case '%s':\n" % t.format)
        for module_id in xrange(self.settings.num_modules):
          out.write("      va_copy(ap2, ap);\n")
          out.write("      vcheck%d(ap2, i, fmt[i]);\n" % module_id)
          out.write("      va_end(ap2);\n");
        out.write("      ASSERT(%s);\n" % comp_expr)
        out.write("      break;\n")
    out.write("    default:\n")
    out.write('      printf("Unexpected type code\\n");\n')
    out.write("      exit(1);\n")
    out.write("    }\n")
    out.write("    i++;\n")
    out.write("  }\n")
    out.write("  va_end(ap);\n")
    out.write("}\n\n")

alphabet = string.letters + string.digits

def make_string():
  global alphabet
  randstr = [random.choice(alphabet) for i in xrange(random.randint(0,16))]
  randstr = ''.join(randstr)
  return '"%s"' % randstr

def make_int():
  B = pow(2,31)
  return str(random.randint(-B, B-1))

def make_long():
  B = pow(2,31)
  return str(random.randint(-B, B-1)) + "L"

def make_llong():
  B = pow(2,63)
  return str(random.randint(-B, B-1)) + "LL"

def make_float():
  return str(random.random() * pow(2.0, random.randint(-126, 127)))

def make_double():
  return str( random.random() * pow(2.0, random.randint(-1022, 1023)) )

def make_ldouble():
  return "((long double)%s)" % make_double()

def make_char():
  global alphabet
  return "'%s'" % random.choice(alphabet)

def make_short():
  B = pow(2,15)
  return str(random.randint(-B, B-1))

def make_tiny():
  a = make_char()
  b = make_short()
  return "%s,%s" % (a,b)

def make_big():
  a = make_char()
  b = make_char()
  c = make_int()
  d = make_char()
  e = make_int()
  f = make_char()
  g = make_llong()
  h = make_char()
  i = make_int()
  j = make_char()
  k = make_short()
  l = make_char()
  m = make_double()
  n = make_char()
  return "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s" % \
         (a,b,c,d,e,f,g,h,i,j,k,l,m,n)

if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
