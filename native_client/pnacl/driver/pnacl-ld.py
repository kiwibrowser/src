#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from driver_tools import ArchMerge, DriverChain, GetArch, \
    ParseArgs, ParseTriple, RunDriver, RunWithEnv, SetArch, \
    SetExecutableMode, TempNameGen, UnrecognizedOption
from driver_env import env
from driver_log import Log
import filetype
import ldtools
import os
import pathtools

EXTRA_ENV = {
  'ALLOW_NATIVE': '0', # Allow LD args which will change the behavior
                       # of native linking. This must be accompanied by
                       # -arch to produce a .nexe.
  'USE_IRT': '1',  # Use stable IRT interfaces.

  'INPUTS'   : '',
  'OUTPUT'   : '',

  'STATIC'   : '0',
  'SHARED'   : '0',
  'PIC'      : '0',
  'USE_STDLIB': '1',
  'RELOCATABLE': '0',
  'SONAME'   : '',

  'STRIP_MODE' : 'none',

  'STRIP_FLAGS'      : '${STRIP_FLAGS_%STRIP_MODE%}',
  'STRIP_FLAGS_all'  : '-s',
  'STRIP_FLAGS_debug': '-S',

  'OPT_INLINE_THRESHOLD': '100',
  'OPT_LEVEL': '',  # Default opt is 0, but we need to know if it's explicitly
                    # requested or not, since we don't want to propagate
                    # the value to TRANSLATE_FLAGS if it wasn't explicitly set.
  'OPT_LTO_FLAGS': '-std-link-opts',
  'OPT_FLAGS': '${#OPT_LEVEL && !OPT_LEVEL == 0 ? ${OPT_LTO_FLAGS}} ' +
               '-inline-threshold=${OPT_INLINE_THRESHOLD} ',

  'TRANSLATE_FLAGS': '${PIC ? -fPIC} ${!USE_STDLIB ? -nostdlib} ' +
                     '${#SONAME ? -Wl,--soname=${SONAME}} ' +
                     '${#OPT_LEVEL ? -O${OPT_LEVEL}} ' +
                     '--allow-llvm-bitcode-input ' +
                     '${TRANSLATE_FLAGS_USER}',

  # Extra pnacl-translate flags specified by the user using -Wt
  'TRANSLATE_FLAGS_USER': '',

  'GOLD_PLUGIN_ARGS': '-plugin=${GOLD_PLUGIN_SO} ' +
                      '-plugin-opt=emit-llvm ' +
                      '-plugin-opt=no-abi-simplify ' +
                      '-plugin-opt=no-finalize',

  'LD_FLAGS'       : '-nostdlib ${@AddPrefix:-L:SEARCH_DIRS} ' +
                     '${STATIC ? -static} ' +
                     '${RELOCATABLE ? -relocatable} ' +
                     '${#SONAME ? --soname=${SONAME}}',

  # Flags for native linking.
  # Only allowed if ALLOW_NATIVE is true.
  'LD_FLAGS_NATIVE': '',
  # Flags used for native linking, or ignored if ALLOW_NATIVE is not true.
  'LD_FLAGS_NATIVE_OR_IGNORED': '',

  'SEARCH_DIRS'        : '${SEARCH_DIRS_USER} ${SEARCH_DIRS_BUILTIN}',
  'SEARCH_DIRS_USER'   : '',

  # Standard Library Directories
  'SEARCH_DIRS_BUILTIN': '${USE_STDLIB ? ' +
                         '  ${BASE_USR}/usr/lib/ ' +
                         '  ${BASE_USR}/lib/ ' +
                         '  ${BASE_SDK}/lib/ ' +
                         '  ${BASE_LIB}/ ' +
                         '}',

  'BCLD_OFORMAT'               : '${BCLD_OFORMAT_%ARCH%}',
  'BCLD_OFORMAT_ARM'           : 'elf32-littlearm-nacl',
  'BCLD_OFORMAT_X8632'         : 'elf32-i386-nacl',
  'BCLD_OFORMAT_X8664'         : 'elf64-x86-64-nacl',
  'BCLD_OFORMAT_MIPS32'        : 'elf32-tradlittlemips-nacl',
  'BCLD_OFORMAT_ARM_NONSFI'    : 'elf32-littlearm-nacl',
  'BCLD_OFORMAT_X8632_NONSFI'  : 'elf32-i386-nacl',

  'BCLD_ALLOW_UNRESOLVED'  :
    # The following functions are implemented in the native support library.
    # Before a .pexe is produced, they get rewritten to intrinsic calls.
    # However, this rewriting happens after bitcode linking - so gold has
    # to be told that these are allowed to remain unresolved.
    '--allow-unresolved=memcpy '
    '--allow-unresolved=memset '
    '--allow-unresolved=memmove '
    '--allow-unresolved=setjmp '
    '--allow-unresolved=longjmp '
    # These TLS layout functions are either defined by the ExpandTls
    # pass or (for non-ABI-stable code only) by PNaCl's native support
    # code.
    '--allow-unresolved=__nacl_tp_tls_offset '
    '--allow-unresolved=__nacl_tp_tdb_offset '
    # __nacl_get_arch() is for non-ABI-stable code only.
    '--allow-unresolved=__nacl_get_arch '
    '${CXX_EH_MODE==sjlj ? '
      # These symbols are defined by libsupc++ and the PNaClSjLjEH
      # pass generates references to them.
      '--undefined=__pnacl_eh_stack '
      '--undefined=__pnacl_eh_resume '
      # These symbols are defined by the PNaClSjLjEH pass and
      # libsupc++ refers to them.
      '--allow-unresolved=__pnacl_eh_type_table '
      '--allow-unresolved=__pnacl_eh_action_table '
      '--allow-unresolved=__pnacl_eh_filter_table}',

  'BCLD_FLAGS':
    '--oformat ${BCLD_OFORMAT} ' +
    '${!RELOCATABLE ? --undef-sym-check ${BCLD_ALLOW_UNRESOLVED}} ' +
    '${GOLD_PLUGIN_ARGS} ${LD_FLAGS}',
  'RUN_BCLD': ('${LD} ${BCLD_FLAGS} ${inputs} -o ${output}'),

  'CXX_EH_MODE': 'none',
  'ALLOW_NEXE_BUILD_ID': '0',
  'DISABLE_ABI_CHECK': '0',
  'LLVM_PASSES_TO_DISABLE': '',
  'RUN_PASSES_SEPARATELY': '0',
  'FINALIZE': '0',
}

def AddToBCLinkFlags(*args):
  env.append('LD_FLAGS', *args)

def AddToNativeFlags(*args):
  env.append('LD_FLAGS_NATIVE', *args)

def AddToNativeIgnoredFlags(*args):
  env.append('LD_FLAGS_NATIVE_OR_IGNORED', *args)

def AddToBothFlags(*args):
  env.append('LD_FLAGS', *args)
  env.append('LD_FLAGS_NATIVE', *args)

def SetLibTarget(*args):
  arch = ParseTriple(args[0])
  if arch != 'le32':
    env.set('BCLIB_ARCH', arch)

def IsPortable():
  return env.getone('BCLIB_ARCH') == ''

LDPatterns = [
  ( '--pnacl-allow-native', "env.set('ALLOW_NATIVE', '1')"),
  ( '--noirt',              "env.set('USE_IRT', '0')"),
  ( '--pnacl-exceptions=(none|sjlj)', "env.set('CXX_EH_MODE', $0)"),
  ( '(--pnacl-allow-nexe-build-id)', "env.set('ALLOW_NEXE_BUILD_ID', '1')"),
  ( '--pnacl-disable-abi-check', "env.set('DISABLE_ABI_CHECK', '1')"),
  # "--pnacl-disable-pass" allows an ABI simplification pass to be
  # disabled if it is causing problems.  These passes are generally
  # required for ABI-stable pexes but can be omitted when the PNaCl
  # toolchain is used for producing native nexes.
  ( '--pnacl-disable-pass=(.+)', "env.append('LLVM_PASSES_TO_DISABLE', $0)"),
  ( '--pnacl-run-passes-separately', "env.set('RUN_PASSES_SEPARATELY', '1')"),
  ( ('-target', '(.+)'), SetLibTarget),
  ( ('--target=(.+)'), SetLibTarget),
  ( ('--finalize'),    "env.set('FINALIZE', '1')"),
  ( ('--no-finalize'), "env.set('FINALIZE', '0')"),

  ( '-o(.+)',          "env.set('OUTPUT', pathtools.normalize($0))"),
  ( ('-o', '(.+)'),    "env.set('OUTPUT', pathtools.normalize($0))"),
  ( ('--output', '(.+)'), "env.set('OUTPUT', pathtools.normalize($0))"),

  ( '-static',         "env.set('STATIC', '1')"),
  ( '-nostdlib',       "env.set('USE_STDLIB', '0')"),

  ( '-r',              "env.set('RELOCATABLE', '1')"),
  ( '-relocatable',    "env.set('RELOCATABLE', '1')"),
  ( '-i',              "env.set('RELOCATABLE', '1')"),

  ( ('-L', '(.+)'),
    "env.append('SEARCH_DIRS_USER', pathtools.normalize($0))\n"),
  ( '-L(.+)',
    "env.append('SEARCH_DIRS_USER', pathtools.normalize($0))\n"),
  ( ('--library-path', '(.+)'),
    "env.append('SEARCH_DIRS_USER', pathtools.normalize($0))\n"),

  # -rpath and -rpath-link are only relevant to dynamic linking.
  # Ignore them for compatibility with build scripts that expect to be
  # able to pass them.
  ( ('(-rpath)','(.*)'), ""),
  ( ('(-rpath)=(.*)'), ""),
  ( ('(-rpath-link)','(.*)'), ""),
  ( ('(-rpath-link)=(.*)'), ""),

  # This overrides the builtin linker script.
  ( ('(-T)', '(.*)'),    AddToNativeFlags),

  # TODO(pdox): Allow setting an alternative _start symbol in bitcode
  ( ('(-e)','(.*)'),     AddToBothFlags),

  # TODO(pdox): Support GNU versioning.
  ( '(--version-script=.*)', ""),

  # Flags to pass to the native linker.
  ( '-Wn,(.*)', "env.append('LD_FLAGS_NATIVE', *($0.split(',')))"),
  ( ('(-Ttext-segment=.*)'), AddToNativeFlags),
  ( ('(-Trodata-segment=.*)'), AddToNativeFlags),
  ( ('(--section-start)', '(.+)'), AddToNativeFlags),
  ( ('(--build-id(?:=.+)?)'), AddToNativeIgnoredFlags),

  # Flags to pass to translate
  ( '-Wt,(.*)', "env.append('TRANSLATE_FLAGS_USER', *($0.split(',')))"),

  # NOTE: -export-dynamic doesn't actually do anything to the bitcode link
  # right now.  This is just in case we do want to record that in metadata
  # eventually, and have that influence the native linker flags.
  ( '(-export-dynamic)', AddToBCLinkFlags),

  ( '-?-soname=(.*)',             "env.set('SONAME', $0)"),
  ( ('-?-soname', '(.*)'),        "env.set('SONAME', $0)"),

  ( '(-M)',                       AddToBCLinkFlags),
  ( '(--print-map)',              AddToBCLinkFlags),
  ( '(-t)',                       AddToBCLinkFlags),
  ( '(--trace)',                  AddToBCLinkFlags),
  ( ('(-y)','(.*)'),              AddToBCLinkFlags),
  ( ('(-defsym)','(.*)'),         AddToBCLinkFlags),

  ( '-melf_nacl',            "env.set('ARCH', 'X8632')"),
  ( ('-m','elf_nacl'),       "env.set('ARCH', 'X8632')"),
  ( '-melf64_nacl',          "env.set('ARCH', 'X8664')"),
  ( ('-m','elf64_nacl'),     "env.set('ARCH', 'X8664')"),
  ( '-marmelf_nacl',         "env.set('ARCH', 'ARM')"),
  ( ('-m','armelf_nacl'),    "env.set('ARCH', 'ARM')"),
  ( '-mmipselelf_nacl',      "env.set('ARCH', 'MIPS32')"),
  ( ('-m','mipselelf_nacl'), "env.set('ARCH', 'MIPS32')"),

  ( ('(-?-wrap)', '(.+)'), AddToBCLinkFlags),
  ( ('(-?-wrap=.+)'),      AddToBCLinkFlags),

  # "-shared" tells the linker to build a PLL (PNaCl/portable loadable library).
  # This is implemented by passing "-relocatable" to the bitcode linker.
  ( '(-shared)',             "env.set('SHARED', '1')"),

  # NOTE: For scons tests, the code generation fPIC flag is used with pnacl-ld.
  ( '-fPIC',               "env.set('PIC', '1')"),

  # This controls LTO optimization.
  # opt does not support -O4 but -O4 is how you ask clang for LTO, so we
  # can support it as well
  ( '-O([0-3sz])',         "env.set('OPT_LEVEL', $0)"),
  ( '-O([0-9]+)',          "env.set('OPT_LEVEL', '3')"),

  ( '(-translate-fast)',   "env.append('TRANSLATE_FLAGS', $0)"),

  ( '-s',                  "env.set('STRIP_MODE', 'all')"),
  ( '--strip-all',         "env.set('STRIP_MODE', 'all')"),
  ( '-S',                  "env.set('STRIP_MODE', 'debug')"),
  ( '--strip-debug',       "env.set('STRIP_MODE', 'debug')"),

  ( '-g', ""),

  ( '--fatal-warnings',     AddToBCLinkFlags),
  ( '--no-fatal-warnings',  AddToBCLinkFlags),

  # Inputs and options that need to be kept in order
  ( '(-l.*)',              "env.append('INPUTS', $0)"),
  ( ('(-l)','(.*)'),       "env.append('INPUTS', $0+$1)"),
  ( ('--library', '(.*)'), "env.append('INPUTS', '-l'+$0)"),

  ( '(--no-as-needed)',    "env.append('INPUTS', $0)"),
  ( '(--as-needed)',       "env.append('INPUTS', $0)"),
  ( '(--no-gc-sections)',  "env.append('INPUTS', $0)"),
  ( '(--gc-sections)',     "env.append('INPUTS', $0)"),
  ( '(--start-group)',     "env.append('INPUTS', $0)"),
  ( '(--end-group)',       "env.append('INPUTS', $0)"),
  ( '(-Bstatic)',          "env.append('INPUTS', $0)"),
  ( '(-Bdynamic)',          "env.append('INPUTS', $0)"),
  ( '((-)?-(no-)?whole-archive)', "env.append('INPUTS', $0)"),
  ( '(--allow-multiple-definition)', "env.append('INPUTS', $0)"),
  ( '(--undefined=.*)',    "env.append('INPUTS', $0)"),
  ( ('(-u)','(.*)'),       "env.append('INPUTS', $0+$1)"),
  ( '(-u.*)',              "env.append('INPUTS', $0)"),
  ( '(-.*)',               UnrecognizedOption),
  ( '(.*)',                "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  ParseArgs(argv, LDPatterns)
  # If the user passed -arch, then they want native output.
  arch_flag_given = GetArch() is not None

  # Both LD_FLAGS_NATIVE and TRANSLATE_FLAGS_USER affect
  # the translation process. If they are non-empty,
  # then --pnacl-allow-native must be given.
  allow_native = env.getbool('ALLOW_NATIVE')
  native_flags = env.get('LD_FLAGS_NATIVE') + env.get('TRANSLATE_FLAGS_USER')
  if len(native_flags) > 0:
    if not allow_native:
      flagstr = ' '.join(native_flags)
      Log.Fatal('"%s" affects translation. '
                'To allow, specify --pnacl-allow-native' % flagstr)

  if allow_native:
    if not arch_flag_given:
      Log.Fatal("--pnacl-allow-native given, but translation "
                "is not happening (missing -arch?)")
    if env.getbool('SHARED'):
      Log.Fatal('Native shared libraries are not supported with pnacl-ld.')

  if env.getbool('SHARED'):
    if env.getbool('RELOCATABLE'):
      Log.Fatal('-r/-relocatable and -shared may not be passed together.')
    env.set('RELOCATABLE', '1')

  # Overriding the lib target uses native-flavored bitcode libs rather than the
  # portable bitcode libs. It is currently only tested/supported for
  # building the IRT.
  if not IsPortable():
    env.set('BASE_USR', "${BASE_USR_ARCH}")
    env.set('BASE_LIB', "${BASE_LIB_ARCH}")

  if env.getbool('RELOCATABLE'):
    env.set('STATIC', '0')

  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  if output == '':
    output = pathtools.normalize('a.out')

  if not arch_flag_given:
    # If -arch is not given, assume X86-32.
    # This is because gold requires an arch (even for bitcode linking).
    SetArch('X8632')
  assert(GetArch() is not None)

  inputs = FixPrivateLibs(inputs)

  # Expand all parameters
  # This resolves -lfoo into actual filenames,
  # and expands linker scripts into command-line arguments.
  inputs = ldtools.ExpandInputs(inputs,
                                env.get('SEARCH_DIRS'),
                                env.getbool('STATIC'),
                                # Once all glibc bitcode link is purely
                                # bitcode (e.g., even libc_nonshared.a)
                                # we may be able to restrict this more.
                                # This is also currently used by
                                # pnacl_generate_pexe=0 with glibc,
                                # for user libraries.
                                ldtools.LibraryTypes.ANY)
  plls, inputs = FilterPlls(inputs)
  plls = [os.path.basename(pll) for pll in plls]

  if not env.getbool('SHARED') and plls != []:
    Log.Fatal('Passing a PLL to the linker requires the "-shared" option.')

  # Make sure the inputs have matching arch.
  CheckInputsArch(inputs)

  regular_inputs, native_objects = SplitLinkLine(inputs)

  if env.getbool('RELOCATABLE'):
    bitcode_type = 'po'
    native_type = 'o'
  else:
    bitcode_type = 'pexe'
    native_type = 'nexe'

  if native_objects and not allow_native:
    argstr = ' '.join(native_objects)
    Log.Fatal("Native objects '%s' detected in the link. "
              "To allow, specify --pnacl-allow-native" % argstr)

  tng = TempNameGen([], output)

  # Do the bitcode link.
  if HasBitcodeInputs(inputs):
    chain = DriverChain(inputs, output, tng)
    chain.add(LinkBC, 'pre_opt.' + bitcode_type)

    # Some ABI simplification passes assume the whole program is
    # available (e.g. -expand-varargs, -nacl-expand-ctors and
    # -nacl-expand-tls).  While we could try running a subset of
    # simplification passes when linking native objects, we don't
    # do this because it complicates testing.  For example,
    # it requires '-expand-constant-expr' to be able to handle
    # 'landingpad' instructions.
    # However, if we aren't using biased bitcode, then at least -expand-byval
    # must be run to work with the PPAPI shim calling convention, and
    # -expand-varargs is needed because after LLVM 3.5 the x86-32 backend does
    # not expand the llvm.va_arg intrinsic correctly.
    # (see https://code.google.com/p/nativeclient/issues/detail?id=3913#c24)
    abi_simplify = (env.getbool('STATIC') and
                    len(native_objects) == 0 and
                    not env.getbool('ALLOW_NEXE_BUILD_ID') and
                    IsPortable())
    still_need_expand_byval = IsPortable() and env.getbool('STATIC')
    still_need_expand_varargs = (still_need_expand_byval and
                                 len(native_objects) == 0)

    # A list of groups of args. Each group should contain a pass to run
    # along with relevant flags that go with that pass.
    opt_args = []
    if env.getbool('SHARED'):
      pre_simplify_shared = [
        # The following is a subset of "-pnacl-abi-simplify-preopt".  We don't
        # want to run the full "-pnacl-abi-simplify-preopt" because it
        # internalizes symbols that we want to export via "-convert-to-pso".
        '-nacl-global-cleanup',
        '-expand-varargs',
        '-rewrite-pnacl-library-calls',
        '-rewrite-llvm-intrinsic-calls',
        '-convert-to-pso',
      ]
      # ConvertToPso takes a list of comma-separated PLL dependencies as an
      # argument.
      if plls != []:
        pre_simplify_shared += ['-convert-to-pso-deps=' + ','.join(plls)]
      opt_args.append(pre_simplify_shared)
      # Post-opt is required, since '-convert-to-pso' adds metadata which must
      # be simplified before finalization. Additionally, all functions must be
      # simplified in the post-opt passes.
      abi_simplify = True
    elif abi_simplify:
      pre_simplify = ['-pnacl-abi-simplify-preopt']
      if env.getone('CXX_EH_MODE') == 'sjlj':
        pre_simplify += ['-enable-pnacl-sjlj-eh']
      else:
        assert env.getone('CXX_EH_MODE') == 'none'
      opt_args.append(pre_simplify)
    else:
      # '-lowerinvoke' prevents use of C++ exception handling, which
      # is not yet supported in the PNaCl ABI.  '-simplifycfg' removes
      # landingpad blocks made unreachable by '-lowerinvoke'.
      #
      # We run this in order to remove 'resume' instructions,
      # otherwise these are translated to calls to _Unwind_Resume(),
      # which will not be available at native link time.
      opt_args.append(['-lowerinvoke', '-simplifycfg'])
      if still_need_expand_varargs:
        opt_args.append(['-expand-varargs'])

    if env.getone('OPT_LEVEL') != '' and env.getone('OPT_LEVEL') != '0':
      opt_args.append(env.get('OPT_FLAGS'))
    if env.getone('STRIP_MODE') != 'none':
      opt_args.append(env.get('STRIP_FLAGS'))

    if abi_simplify:
      post_simplify = ['-pnacl-abi-simplify-postopt']
      if not env.getbool('DISABLE_ABI_CHECK'):
        post_simplify += [
            '-verify-pnaclabi-module',
            '-verify-pnaclabi-functions',
            # A flag for the above -verify-pnaclabi-* passes.
            '-pnaclabi-allow-debug-metadata']
      opt_args.append(post_simplify)
    elif still_need_expand_byval:
      # We may still need -expand-byval to match the PPAPI shim
      # calling convention.
      opt_args.append(['-expand-byval'])
    if len(opt_args) != 0:
      if env.getbool('RUN_PASSES_SEPARATELY'):
        for i, group in enumerate(opt_args):
          chain.add(DoLLVMPasses(group),
                    'simplify_%d.%s' % (i, bitcode_type))
      else:
        flattened_opt_args = [flag for group in opt_args for flag in group]
        chain.add(DoLLVMPasses(flattened_opt_args),
                  'simplify_and_opt.' + bitcode_type)
  else:
    chain = DriverChain('', output, tng)

  if env.getbool('FINALIZE'):
    chain.add(DoFinalize, 'finalize.' + bitcode_type)

  # If -arch is also specified, invoke pnacl-translate afterwards.
  if arch_flag_given:
    env.set('NATIVE_OBJECTS', *native_objects)
    chain.add(DoTranslate, native_type)

  chain.run()

  if bitcode_type == 'pexe' and not arch_flag_given:
    # Mark .pexe files as executable.
    # Some versions of 'configure' expect this.
    SetExecutableMode(output)
  return 0

def FixPrivateLibs(user_libs):
  """If not using the IRT or if private libraries are used:
    - Place private libraries that can coexist before their public
      equivalent (keep both);
    - Replace public libraries that can't coexist with their private
      equivalent.

  This occurs before path resolution (important because public/private
  libraries aren't always colocated) and assumes that -l:libfoo.a syntax
  isn't used by the driver for relevant libraries.
  """
  special_libs = {
      # Public library name: (private library name, can coexist?)
      '-lnacl': ('-lnacl_sys_private', True),
      '-lpthread': ('-lpthread_private', False),
      }
  private_libs = [v[0] for v in special_libs.values()]
  public_libs = special_libs.keys()
  private_lib_for = lambda user_lib: special_libs[user_lib][0]
  can_coexist = lambda user_lib: special_libs[user_lib][1]

  no_irt = not env.getbool('USE_IRT')
  uses_private_libs = set(user_libs) & set(private_libs)

  if not (no_irt or uses_private_libs):
    return user_libs

  result_libs = []
  for user_lib in user_libs:
    if user_lib in public_libs:
      result_libs.append(private_lib_for(user_lib))
      if can_coexist(user_lib):
        result_libs.append(user_lib)
    else:
      result_libs.append(user_lib)
  return result_libs

def FilterPlls(inputs):
  """ Split the input list into PLLs and other objects.
  """
  plls = []
  other = []

  for f in inputs:
    if ldtools.IsFlag(f) or not filetype.IsPll(f):
      other.append(f)
    else:
      plls.append(f)
  return (plls, other)

def SplitLinkLine(inputs):
  """ Split the input list into bitcode and native objects (.o, .a)
  """
  normal = []
  native = []
  # Group flags need special handling because they need to go into the right
  # list based on the type of the inputs in the group. If the group has both
  # native and bitcode files (which is unfortunately the case for
  # irt_browser_lib) then the group flags need to go in both lists.
  if '--start-group' in inputs:
    start_group = inputs.index('--start-group')
    # Start with the inputs before the first group
    normal, native = SplitLinkLine(inputs[:start_group])
    try:
      end_group = inputs.index('--end-group')
    except ValueError:
      Log.Fatal("Found --start-group without matching --end-group")
    # Add the contents of the group together with the --{start,end}-group flags
    norm_group, native_group = SplitLinkLine(inputs[start_group + 1:end_group])
    if len(norm_group) > 0:
      normal.extend(['--start-group'] + norm_group + ['--end-group'])
    if len(native_group) > 0:
      native.extend(['--start-group'] + native_group + ['--end-group'])
    # Add the inputs after the first group
    norm_last, native_last = SplitLinkLine(inputs[end_group + 1:])
    return normal + norm_last, native + native_last

  # If no groups, split the inputs based on their type.
  for f in inputs:
    if ldtools.IsFlag(f):
      normal.append(f)
    elif filetype.IsNativeArchive(f) or filetype.IsNativeObject(f):
      native.append(f)
    else:
      normal.append(f)
  return (normal, native)

def HasBitcodeInputs(inputs):
  for f in inputs:
    if ldtools.IsFlag(f):
      continue
    elif filetype.IsLLVMBitcode(f) or filetype.IsBitcodeArchive(f):
      return True
  return False

def CheckInputsArch(inputs):
  count = 0
  for f in inputs:
    if ldtools.IsFlag(f):
      continue
    elif filetype.IsLLVMBitcode(f) or filetype.IsBitcodeArchive(f):
      pass
    elif filetype.IsNative(f):
      ArchMerge(f, True)
    else:
      Log.Fatal("%s: Unexpected type of file for linking (%s)",
                pathtools.touser(f), filetype.FileType(f))
    count += 1

  if count == 0:
    Log.Fatal("no input files")

def DoLLVMPasses(pass_list):
  def Func(infile, outfile):
    filtered_list = [pass_option for pass_option in pass_list
                     if pass_option not in env.get('LLVM_PASSES_TO_DISABLE')]
    # Do not serialize use lists into the (non-finalized) pexe. See
    # https://code.google.com/p/nativeclient/issues/detail?id=4190
    filtered_list.append('-preserve-bc-uselistorder=false')
    RunDriver('pnacl-opt', filtered_list + [infile, '-o', outfile])
  return Func

def DoFinalize(infile, outfile):
  RunDriver('pnacl-finalize', [infile, '-o', outfile])

def DoTranslate(infile, outfile):
  args = env.get('TRANSLATE_FLAGS')
  args += ['-Wl,'+s for s in
           env.get('LD_FLAGS_NATIVE') + env.get('LD_FLAGS_NATIVE_OR_IGNORED')]
  if infile:
    args += [infile]
  args += ['-Wl,'+s if ldtools.IsFlag(s) else s
           for s in env.get('NATIVE_OBJECTS')]
  args += ['-o', outfile]
  RunDriver('pnacl-translate', args)

def LinkBC(inputs, output):
  '''Input: a bunch of bc/o/lib input files
     Output: a combined & optimized bitcode file
  '''
  # Produce combined bitcode file
  RunWithEnv('${RUN_BCLD}',
             inputs=inputs,
             output=output)

def get_help(unused_argv):
  return """Usage: pnacl-ld [options] <input files> -o <output>

Bitcode linker for PNaCl.  Similar to the binutils "ld" tool,
but links bitcode instead of native code.  Supports many of the
"ld" flags. Below are a subset of them.

OPTIONS:

  -o <file>                   Set output file name
  -l LIBNAME                  Search for library LIBNAME
  -L DIRECTORY, --library-path DIRECTORY
                              Add DIRECTORY to library search path

  -r, -relocatable            Generate relocatable output

  -O<opt-level>               Optimize output file
  -M, --print-map             Print map file on standard output
  --whole-archive             Include all objects from following archives
  --no-whole-archive          Turn off --whole-archive
  -s, --strip-all             Strip all symbols
  -S, --strip-debug           Strip debugging symbols
  -u SYM, --undefined=SYM     Start with undefined reference to SYM

  -help | -h                  Output this help.
"""
