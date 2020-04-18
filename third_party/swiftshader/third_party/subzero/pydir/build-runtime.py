#!/usr/bin/env python2

import argparse
import os
import shutil
import tempfile

import targets
from utils import FindBaseNaCl, GetObjcopyCmd, shellcmd


def Translate(ll_files, extra_args, obj, verbose, target):
  """Translate a set of input bitcode files into a single object file.

  Use pnacl-llc to translate textual bitcode input ll_files into object file
  obj, using extra_args as the architectural flags.
  """
  externalize = ['-externalize']
  shellcmd(['cat'] + ll_files + ['|',
            'pnacl-llc',
            '-function-sections',
            '-O2',
            '-filetype=obj',
            '-bitcode-format=llvm',
            '-o', obj
    ] + extra_args + externalize, echo=verbose)
  localize_syms = ['nacl_tp_tdb_offset', 'nacl_tp_tls_offset']

  shellcmd([GetObjcopyCmd(target), obj] +
    [('--localize-symbol=' + sym) for sym in localize_syms])


def PartialLink(obj_files, extra_args, lib, verbose):
  """Partially links a set of obj files into a final obj library."""
  shellcmd(['le32-nacl-ld',
            '-o', lib,
            '-r',
    ] + extra_args + obj_files, echo=verbose)


def MakeRuntimesForTarget(target_info, ll_files,
                          srcdir, tempdir, rtdir, verbose, excluded_targets):
  """Builds native, sandboxed, and nonsfi runtimes for the given target."""
  if target_info.target in excluded_targets:
    return
  # File-mangling helper functions.
  def TmpFile(template):
    return template.format(dir=tempdir, target=target_info.target)
  def OutFile(template):
    return template.format(rtdir=rtdir, target=target_info.target)
  # Helper function for building the native unsandboxed runtime.
  def MakeNativeRuntime():
    """Builds just the native runtime."""
    # Translate tempdir/szrt.ll and tempdir/szrt_ll.ll to
    # szrt_native_{target}.tmp.o.
    Translate(ll_files,
              ['-mtriple=' + target_info.triple] + target_info.llc_flags,
              TmpFile('{dir}/szrt_native_{target}.tmp.o'),
              verbose, target_info.target)
    # Compile srcdir/szrt_profiler.c to
    # tempdir/szrt_profiler_native_{target}.o.
    shellcmd(['clang',
              '-O2',
              '-target=' + target_info.triple,
              '-c',
              '{srcdir}/szrt_profiler.c'.format(srcdir=srcdir),
              '-o', TmpFile('{dir}/szrt_native_profiler_{target}.o')
      ], echo=verbose)
    # Assemble srcdir/szrt_asm_{target}.s to tempdir/szrt_asm_{target}.o.
    shellcmd(['llvm-mc',
              '-triple=' + target_info.triple, '--defsym NATIVE=1',
              '-filetype=obj',
              '-o', TmpFile('{dir}/szrt_native_asm_{target}.o'),
              '{srcdir}/szrt_asm_{target}.s'.format(
                srcdir=srcdir, target=target_info.target)
      ], echo=verbose)
    # Write full szrt_native_{target}.o.
    PartialLink([TmpFile('{dir}/szrt_native_{target}.tmp.o'),
                 TmpFile('{dir}/szrt_native_asm_{target}.o'),
                 TmpFile('{dir}/szrt_native_profiler_{target}.o')],
                ['-m {ld_emu}'.format(ld_emu=target_info.ld_emu)],
                OutFile('{rtdir}/szrt_native_{target}.o'),
                verbose)
    shellcmd([GetObjcopyCmd(target_info.target),
              '--strip-symbol=NATIVE',
              OutFile('{rtdir}/szrt_native_{target}.o')])
    # Compile srcdir/szrt_asan.c to szrt_asan_{target}.o
    shellcmd(['clang',
              '-O2',
              '-target=' + target_info.triple,
              '-c',
              '{srcdir}/szrt_asan.c'.format(srcdir=srcdir),
              '-o', OutFile('{rtdir}/szrt_asan_{target}.o')
      ], echo=verbose)

  # Helper function for building the sandboxed runtime.
  def MakeSandboxedRuntime():
    """Builds just the sandboxed runtime."""
    # Translate tempdir/szrt.ll and tempdir/szrt_ll.ll to szrt_sb_{target}.o.
    # The sandboxed library does not get the profiler helper function as the
    # binaries are linked with -nostdlib.
    Translate(ll_files,
              ['-mtriple=' + targets.ConvertTripleToNaCl(target_info.triple)] +
              target_info.llc_flags,
              TmpFile('{dir}/szrt_sb_{target}.tmp.o'),
              verbose,target_info.target)
    # Assemble srcdir/szrt_asm_{target}.s to tempdir/szrt_asm_{target}.o.
    shellcmd(['llvm-mc',
              '-triple=' + targets.ConvertTripleToNaCl(target_info.triple),
              '--defsym NACL=1',
              '-filetype=obj',
              '-o', TmpFile('{dir}/szrt_sb_asm_{target}.o'),
              '{srcdir}/szrt_asm_{target}.s'.format(
                srcdir=srcdir, target=target_info.target)
      ], echo=verbose)
    PartialLink([TmpFile('{dir}/szrt_sb_{target}.tmp.o'),
                 TmpFile('{dir}/szrt_sb_asm_{target}.o')],
                ['-m {ld_emu}'.format(ld_emu=target_info.sb_emu)],
                OutFile('{rtdir}/szrt_sb_{target}.o'),
                verbose)
    shellcmd([GetObjcopyCmd(target_info.target),
              '--strip-symbol=NACL',
              OutFile('{rtdir}/szrt_sb_{target}.o')])

  # Helper function for building the Non-SFI runtime.
  def MakeNonsfiRuntime():
    """Builds just the nonsfi runtime."""
    # Translate tempdir/szrt.ll and tempdir/szrt_ll.ll to
    # szrt_nonsfi_{target}.tmp.o.
    Translate(ll_files,
              ['-mtriple=' + target_info.triple] + target_info.llc_flags +
              ['-relocation-model=pic', '-force-tls-non-pic', '-malign-double'],
              TmpFile('{dir}/szrt_nonsfi_{target}.tmp.o'),
              verbose, target_info.target)
    # Assemble srcdir/szrt_asm_{target}.s to tempdir/szrt_asm_{target}.o.
    shellcmd(['llvm-mc',
              '-triple=' + target_info.triple, '--defsym NONSFI=1',
              '-filetype=obj',
              '-o', TmpFile('{dir}/szrt_nonsfi_asm_{target}.o'),
              '{srcdir}/szrt_asm_{target}.s'.format(
                srcdir=srcdir, target=target_info.target)
      ], echo=verbose)
    # Write full szrt_nonsfi_{target}.o.
    PartialLink([TmpFile('{dir}/szrt_nonsfi_{target}.tmp.o'),
                 TmpFile('{dir}/szrt_nonsfi_asm_{target}.o')],
                ['-m {ld_emu}'.format(ld_emu=target_info.ld_emu)],
                OutFile('{rtdir}/szrt_nonsfi_{target}.o'),
                verbose)
    shellcmd([GetObjcopyCmd(target_info.target),
              '--strip-symbol=NONSFI',
              OutFile('{rtdir}/szrt_nonsfi_{target}.o')])


  # Run the helper functions.
  MakeNativeRuntime()
  MakeSandboxedRuntime()
  MakeNonsfiRuntime()


def main():
    """Build the Subzero runtime support library for all architectures.
    """
    nacl_root = FindBaseNaCl()
    argparser = argparse.ArgumentParser(
        description='    ' + main.__doc__,
        formatter_class=argparse.RawTextHelpFormatter)
    argparser.add_argument('--verbose', '-v', dest='verbose',
                           action='store_true',
                           help='Display some extra debugging output')
    argparser.add_argument('--pnacl-root', dest='pnacl_root',
                           default=(
                             '{root}/toolchain/linux_x86/pnacl_newlib_raw'
                           ).format(root=nacl_root),
                           help='Path to PNaCl toolchain binaries.')
    argparser.add_argument('--exclude-target', dest='excluded_targets',
                           default=[], action='append',
                           help='Target whose runtime should not be built')
    args = argparser.parse_args()
    os.environ['PATH'] = ('{root}/bin{sep}{path}'
        ).format(root=args.pnacl_root, sep=os.pathsep, path=os.environ['PATH'])
    srcdir = (
        '{root}/toolchain_build/src/subzero/runtime'
        ).format(root=nacl_root)
    rtdir = (
        '{root}/toolchain_build/src/subzero/build/runtime'
        ).format(root=nacl_root)
    try:
        tempdir = tempfile.mkdtemp()
        if os.path.exists(rtdir) and not os.path.isdir(rtdir):
            os.remove(rtdir)
        if not os.path.exists(rtdir):
            os.makedirs(rtdir)
        # Compile srcdir/szrt.c to tempdir/szrt.ll
        shellcmd(['pnacl-clang',
                  '-O2',
                  '-c',
                  '{srcdir}/szrt.c'.format(srcdir=srcdir),
                  '-o', '{dir}/szrt.tmp.bc'.format(dir=tempdir)
            ], echo=args.verbose)
        shellcmd(['pnacl-opt',
                  '-pnacl-abi-simplify-preopt',
                  '-pnacl-abi-simplify-postopt',
                  '-pnaclabi-allow-debug-metadata',
                  '{dir}/szrt.tmp.bc'.format(dir=tempdir),
                  '-S',
                  '-o', '{dir}/szrt.ll'.format(dir=tempdir)
            ], echo=args.verbose)
        ll_files = ['{dir}/szrt.ll'.format(dir=tempdir),
                    '{srcdir}/szrt_ll.ll'.format(srcdir=srcdir)]

        MakeRuntimesForTarget(targets.X8632Target, ll_files,
                              srcdir, tempdir, rtdir, args.verbose,
                              args.excluded_targets)
        MakeRuntimesForTarget(targets.X8664Target, ll_files,
                              srcdir, tempdir, rtdir, args.verbose,
                              args.excluded_targets)
        MakeRuntimesForTarget(targets.ARM32Target, ll_files,
                              srcdir, tempdir, rtdir, args.verbose,
                              args.excluded_targets)
        MakeRuntimesForTarget(targets.MIPS32Target, ll_files,
                              srcdir, tempdir, rtdir, args.verbose,
                              args.excluded_targets)

    finally:
        try:
            shutil.rmtree(tempdir)
        except OSError as exc:
            if exc.errno != errno.ENOENT: # ENOENT - no such file or directory
                raise # re-raise exception

if __name__ == '__main__':
    main()
