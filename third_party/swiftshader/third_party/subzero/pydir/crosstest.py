#!/usr/bin/env python2

import argparse
import os
import subprocess
import sys
import tempfile

import targets
from szbuild import LinkNonsfi
from utils import FindBaseNaCl, GetObjcopyCmd, get_sfi_string, shellcmd

def main():
    """Builds a cross-test binary for comparing Subzero and llc translation.

    Each --test argument is compiled once by llc and once by Subzero.  C/C++
    tests are first compiled down to PNaCl bitcode using pnacl-clang and
    pnacl-opt.  The --prefix argument ensures that symbol names are different
    between the two object files, to avoid linking errors.

    There is also a --driver argument that specifies the C/C++ file that calls
    the test functions with a variety of interesting inputs and compares their
    results.

    """
    # arch_map maps a Subzero target string to TargetInfo (e.g., triple).
    arch_map = { 'x8632': targets.X8632Target,
                 'x8664': targets.X8664Target,
                 'arm32': targets.ARM32Target,
                 'mips32': targets.MIPS32Target}
    arch_sz_flags = { 'x8632': [],
                      'x8664': [],
                      # For ARM, test a large stack offset as well. +/- 4095 is
                      # the limit, so test somewhere near that boundary.
                      'arm32': ['--test-stack-extra', '4084'],
                      'mips32': ['--test-stack-extra', '4084']
    }
    arch_llc_flags_extra = {
        # Use sse2 instructions regardless of input -mattr
        # argument to avoid differences in (undefined) behavior of
        # converting NaN to int.
        'x8632': ['-mattr=sse2'],
        'x8664': ['-mattr=sse2'],
        'arm32': [],
        'mips32':[],
    }
    desc = 'Build a cross-test that compares Subzero and llc translation.'
    argparser = argparse.ArgumentParser(description=desc)
    argparser.add_argument('--test', required=True, action='append',
                           metavar='TESTFILE_LIST',
                           help='List of C/C++/.ll files with test functions')
    argparser.add_argument('--driver', required=True,
                           metavar='DRIVER',
                           help='Driver program')
    argparser.add_argument('--target', required=False, default='x8632',
                           choices=arch_map.keys(),
                           metavar='TARGET',
                           help='Translation target architecture.' +
                                ' Default %(default)s.')
    argparser.add_argument('-O', required=False, default='2', dest='optlevel',
                           choices=['m1', '-1', '0', '1', '2'],
                           metavar='OPTLEVEL',
                           help='Optimization level for llc and Subzero ' +
                                '(m1 and -1 are equivalent).' +
                                ' Default %(default)s.')
    argparser.add_argument('--clang-opt', required=False, default=True,
                           dest='clang_opt')
    argparser.add_argument('--mattr',  required=False, default='sse2',
                           dest='attr', choices=['sse2', 'sse4.1',
                                                 'neon', 'hwdiv-arm',
                                                 'base'],
                           metavar='ATTRIBUTE',
                           help='Target attribute. Default %(default)s.')
    argparser.add_argument('--sandbox', required=False, default=0, type=int,
                           dest='sandbox',
                           help='Use sandboxing. Default "%(default)s".')
    argparser.add_argument('--nonsfi', required=False, default=0, type=int,
                           dest='nonsfi',
                           help='Use Non-SFI mode. Default "%(default)s".')
    argparser.add_argument('--prefix', required=True,
                           metavar='SZ_PREFIX',
                           help='String prepended to Subzero symbol names')
    argparser.add_argument('--output', '-o', required=True,
                           metavar='EXECUTABLE',
                           help='Executable to produce')
    argparser.add_argument('--dir', required=False, default='.',
                           metavar='OUTPUT_DIR',
                           help='Output directory for all files.' +
                                ' Default "%(default)s".')
    argparser.add_argument('--filetype', default='obj', dest='filetype',
                           choices=['obj', 'asm', 'iasm'],
                           help='Output file type.  Default %(default)s.')
    argparser.add_argument('--sz', dest='sz_args', action='append', default=[],
                           help='Extra arguments to pass to pnacl-sz.')
    args = argparser.parse_args()

    nacl_root = FindBaseNaCl()
    bindir = ('{root}/toolchain/linux_x86/pnacl_newlib_raw/bin'
              .format(root=nacl_root))
    target_info = arch_map[args.target]
    triple = target_info.triple
    if args.sandbox:
        triple = targets.ConvertTripleToNaCl(triple)
    llc_flags = target_info.llc_flags + arch_llc_flags_extra[args.target]
    if args.nonsfi:
        llc_flags.extend(['-relocation-model=pic',
                          '-malign-double',
                          '-force-tls-non-pic',
                          '-mtls-use-call'])
    mypath = os.path.abspath(os.path.dirname(sys.argv[0]))

    # Construct a "unique key" for each test so that tests can be run in
    # parallel without race conditions on temporary file creation.
    key = '{sb}.O{opt}.{attr}.{target}'.format(
        target=args.target,
        sb=get_sfi_string(args, 'sb', 'nonsfi', 'nat'),
        opt=args.optlevel, attr=args.attr)
    objs = []
    for arg in args.test:
        base, ext = os.path.splitext(arg)
        if ext == '.ll':
            bitcode = arg
        else:
            # Use pnacl-clang and pnacl-opt to produce PNaCl bitcode.
            bitcode_nonfinal = os.path.join(args.dir, base + '.' + key + '.bc')
            bitcode = os.path.join(args.dir, base + '.' + key + '.pnacl.ll')
            shellcmd(['{bin}/pnacl-clang'.format(bin=bindir),
                      ('-O2' if args.clang_opt else '-O0'),
                      ('-DARM32' if args.target == 'arm32' else ''), '-c', arg,
                      ('-DMIPS32' if args.target == 'mips32' else ''),
                      '-o', bitcode_nonfinal])
            shellcmd(['{bin}/pnacl-opt'.format(bin=bindir),
                      '-pnacl-abi-simplify-preopt',
                      '-pnacl-abi-simplify-postopt',
                      '-pnaclabi-allow-debug-metadata',
                      '-strip-metadata',
                      '-strip-module-flags',
                      '-strip-debug',
                      bitcode_nonfinal, '-S', '-o', bitcode])

        base_sz = '{base}.{key}'.format(base=base, key=key)
        asm_sz = os.path.join(args.dir, base_sz + '.sz.s')
        obj_sz = os.path.join(args.dir, base_sz + '.sz.o')
        obj_llc = os.path.join(args.dir, base_sz + '.llc.o')

        shellcmd(['{path}/pnacl-sz'.format(path=os.path.dirname(mypath)),
                  ] + args.sz_args + [
                  '-O' + args.optlevel,
                  '-mattr=' + args.attr,
                  '--target=' + args.target,
                  '--sandbox=' + str(args.sandbox),
                  '--nonsfi=' + str(args.nonsfi),
                  '--prefix=' + args.prefix,
                  '-allow-uninitialized-globals',
                  '-externalize',
                  '-filetype=' + args.filetype,
                  '-o=' + (obj_sz if args.filetype == 'obj' else asm_sz),
                  bitcode] + arch_sz_flags[args.target])
        if args.filetype != 'obj':
            shellcmd(['{bin}/llvm-mc'.format(bin=bindir),
                      '-triple=' + ('mipsel-linux-gnu'
                                    if args.target == 'mips32' and args.sandbox
                                    else triple),
                      '-filetype=obj',
                      '-o=' + obj_sz,
                      asm_sz])

        # Each separately translated Subzero object file contains its own
        # definition of the __Sz_block_profile_info profiling symbol.  Avoid
        # linker errors (multiply defined symbol) by making all copies weak.
        # (This could also be done by Subzero if it supported weak symbol
        # definitions.)  This approach should be OK because cross tests are
        # currently the only situation where multiple translated files are
        # linked into the executable, but when PNaCl supports shared nexe
        # libraries, this would need to change.  (Note: the same issue applies
        # to the __Sz_revision symbol.)
        shellcmd(['{bin}/{objcopy}'.format(bin=bindir,
                  objcopy=GetObjcopyCmd(args.target)),
                  '--weaken-symbol=__Sz_block_profile_info',
                  '--weaken-symbol=__Sz_revision',
                  '--strip-symbol=nacl_tp_tdb_offset',
                  '--strip-symbol=nacl_tp_tls_offset',
                  obj_sz])
        objs.append(obj_sz)
        shellcmd(['{bin}/pnacl-llc'.format(bin=bindir),
                  '-mtriple=' + triple,
                  '-externalize',
                  '-filetype=obj',
                  '-bitcode-format=llvm',
                  '-o=' + obj_llc,
                  bitcode] + llc_flags)
        strip_syms = [] if args.target == 'mips32' else ['nacl_tp_tdb_offset',
                                                         'nacl_tp_tls_offset']
        shellcmd(['{bin}/{objcopy}'.format(bin=bindir,
                  objcopy=GetObjcopyCmd(args.target)),
                  obj_llc] +
                 [('--strip-symbol=' + sym) for sym in strip_syms])
        objs.append(obj_llc)

    # Add szrt_sb_${target}.o or szrt_native_${target}.o.
    if not args.nonsfi:
        objs.append((
                '{root}/toolchain_build/src/subzero/build/runtime/' +
                'szrt_{sb}_' + args.target + '.o'
                ).format(root=nacl_root,
                         sb=get_sfi_string(args, 'sb', 'nonsfi', 'native')))

    target_params = []

    if args.target == 'arm32':
      target_params.append('-DARM32')
      target_params.append('-static')

    if args.target == 'mips32':
      target_params.append('-DMIPS32')

    pure_c = os.path.splitext(args.driver)[1] == '.c'
    if not args.nonsfi:
        # Set compiler to clang, clang++, pnacl-clang, or pnacl-clang++.
        compiler = '{bin}/{prefix}{cc}'.format(
            bin=bindir, prefix=get_sfi_string(args, 'pnacl-', '', ''),
            cc='clang' if pure_c else 'clang++')
        sb_native_args = (['-O0', '--pnacl-allow-native',
                           '-arch', target_info.compiler_arch,
                           '-Wn,-defsym=__Sz_AbsoluteZero=0']
                          if args.sandbox else
                          ['-g', '-target=' + triple,
                           '-lm', '-lpthread',
                           '-Wl,--defsym=__Sz_AbsoluteZero=0'] +
                          target_info.cross_headers)
        shellcmd([compiler] + target_params + [args.driver] + objs +
                 ['-o', os.path.join(args.dir, args.output)] + sb_native_args)
        return 0

    base, ext = os.path.splitext(args.driver)
    bitcode_nonfinal = os.path.join(args.dir, base + '.' + key + '.bc')
    bitcode = os.path.join(args.dir, base + '.' + key + '.pnacl.ll')
    asm_sz = os.path.join(args.dir, base + '.' + key + '.s')
    obj_llc = os.path.join(args.dir, base + '.' + key + '.o')
    compiler = '{bin}/{prefix}{cc}'.format(
        bin=bindir, prefix='pnacl-',
        cc='clang' if pure_c else 'clang++')
    shellcmd([compiler] + target_params + [
              args.driver,
              '-O2',
              '-o', bitcode_nonfinal,
              '-Wl,-r'
             ])
    shellcmd(['{bin}/pnacl-opt'.format(bin=bindir),
              '-pnacl-abi-simplify-preopt',
              '-pnacl-abi-simplify-postopt',
              '-pnaclabi-allow-debug-metadata',
              '-strip-metadata',
              '-strip-module-flags',
              '-strip-debug',
              '-disable-opt',
              bitcode_nonfinal, '-S', '-o', bitcode])
    shellcmd(['{bin}/pnacl-llc'.format(bin=bindir),
              '-mtriple=' + triple,
              '-externalize',
              '-filetype=obj',
              '-O2',
              '-bitcode-format=llvm',
              '-o', obj_llc,
              bitcode] + llc_flags)
    if not args.sandbox and not args.nonsfi:
        shellcmd(['{bin}/{objcopy}'.format(bin=bindir,
                  objcopy=GetObjcopyCmd(args.target)),
                  '--redefine-sym', '_start=_user_start',
                  obj_llc
                 ])
    objs.append(obj_llc)
    if args.nonsfi:
        LinkNonsfi(objs, os.path.join(args.dir, args.output), args.target)
    elif args.sandbox:
        LinkSandbox(objs, os.path.join(args.dir, args.output), args.target)
    else:
        LinkNative(objs, os.path.join(args.dir, args.output), args.target)

if __name__ == '__main__':
    main()
