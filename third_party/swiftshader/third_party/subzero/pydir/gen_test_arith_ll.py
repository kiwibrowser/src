def mangle(op, op_type, signed):
  # suffixMap gives the C++ name-mangling suffixes for a function that takes two
  # arguments of the given type.  The first entry is for the unsigned version of
  # the type, and the second entry is for the signed version.
  suffixMap = { 'i1': ['bb', 'bb'],
                'i8': ['hh', 'aa'],
                'i16': ['tt', 'ss'],
                'i32': ['jj', 'ii'],
                'i64': ['yy', 'xx'],
                'float': ['ff', 'ff'],
                'double': ['dd', 'dd'],
                '<4 x i32>': ['Dv4_jS_', 'Dv4_iS_'],
                '<8 x i16>': ['Dv8_tS_', 'Dv8_sS_'],
                '<16 x i8>': ['Dv16_hS_', 'Dv16_aS_'],
                '<4 x float>': ['Dv4_fS_', 'Dv4_fS_'],
              }
  base = 'test' + op.capitalize()
  return '_Z' + str(len(base)) + base + suffixMap[op_type][signed]

def arith(Native, Type, Op):
  _TEMPLATE_ = """
define internal {{native}} @{{name}}({{native}} %a, {{native}} %b) {{{{
  {trunc_a}
  {trunc_b}
  %result{{trunc}} = {{op}} {{type}} %a{{trunc}}, %b{{trunc}}
  {zext}
  ret {{native}} %result
}}}}"""

  Signed = Op in {'sdiv', 'srem', 'ashr'}
  Name = mangle(Op, Type, Signed)
  # Most i1 operations are invalid for PNaCl, so map them to i32.
  if Type == 'i1' and (Op not in {'and', 'or', 'xor'}):
    Type = 'i32'
  x = _TEMPLATE_.format(
      trunc_a = '%a.trunc = trunc {native} %a to {type}' if
          Native != Type else '',
      trunc_b = '%b.trunc = trunc {native} %b to {type}' if
          Native != Type else '',
      zext = '%result = ' + ('sext' if Signed else 'zext') +
          ' {type} %result.trunc to {native}' if Native != Type else '')
  lines = x.format(native=Native, type=Type, op=Op, name=Name,
                   trunc='.trunc' if Native != Type else '')
  # Strip trailing whitespace from each line to keep git happy.
  print '\n'.join([line.rstrip() for line in lines.splitlines()])

for op in ['add', 'sub', 'mul', 'sdiv', 'udiv', 'srem', 'urem', 'shl', 'lshr',
           'ashr', 'and', 'or', 'xor']:
  for op_type in ['i1', 'i8', 'i16', 'i32']:
    arith('i32', op_type, op)
  for op_type in ['i64', '<4 x i32>', '<8 x i16>', '<16 x i8>']:
    arith(op_type, op_type, op)

for op in ['fadd', 'fsub', 'fmul', 'fdiv', 'frem']:
  for op_type in ['float', 'double', '<4 x float>']:
    arith(op_type, op_type, op)
