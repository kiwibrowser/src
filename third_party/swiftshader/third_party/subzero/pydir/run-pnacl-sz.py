#!/usr/bin/env python2

import argparse
import itertools
import os
import re
import subprocess
import sys
import tempfile

from utils import FindBaseNaCl, GetObjdumpCmd, shellcmd


def TargetAssemblerFlags(target, sandboxed):
  # TODO(reed kotler). Need to find out exactly we need to
  # add here for Mips32.
  flags = { 'x8632': ['-triple=%s' % ('i686-nacl' if sandboxed else 'i686')],
            'x8664': ['-triple=%s' % (
                          'x86_64-nacl' if sandboxed else 'x86_64')],
            'arm32': ['-triple=%s' % (
                          'armv7a-nacl' if sandboxed else 'armv7a'),
                      '-mcpu=cortex-a9', '-mattr=+neon'],
            'mips32': ['-triple=%s' % (
                          'mipsel-nacl' if sandboxed else 'mipsel'),
                       '-mcpu=mips32'] }
  return flags[target]


def TargetDisassemblerFlags(target):
  flags = { 'x8632': ['-Mintel'],
            'x8664': ['-Mintel'],
            'arm32': [],
            'mips32':[] }
  return flags[target]

def main():
    """Run the pnacl-sz compiler on an llvm file.

    Takes an llvm input file, freezes it into a pexe file, converts
    it to a Subzero program, and finally compiles it.
    """
    argparser = argparse.ArgumentParser(
        description='    ' + main.__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    argparser.add_argument('--input', '-i', required=True,
                           help='LLVM source file to compile')
    argparser.add_argument('--output', '-o', required=False,
                           help='Output file to write')
    argparser.add_argument('--insts', required=False,
                           action='store_true',
                           help='Stop after translating to ' +
                           'Subzero instructions')
    argparser.add_argument('--no-local-syms', required=False,
                           action='store_true',
                           help="Don't keep local symbols in the pexe file")
    argparser.add_argument('--llvm', required=False,
                           action='store_true',
                           help='Parse pexe into llvm IR first, then ' +
                           'convert to Subzero')
    argparser.add_argument('--llvm-source', required=False,
                           action='store_true',
                           help='Parse source directly into llvm IR ' +
                           '(without generating a pexe), then ' +
                           'convert to Subzero')
    argparser.add_argument(
        '--pnacl-sz', required=False, default='./pnacl-sz', metavar='PNACL-SZ',
        help="Subzero translator 'pnacl-sz'")
    argparser.add_argument('--pnacl-bin-path', required=False,
                           default=(
                             '{root}/toolchain/linux_x86/pnacl_newlib_raw/bin'
                           ).format(root=FindBaseNaCl()),
                           metavar='PNACL_BIN_PATH',
                           help='Path to LLVM & Binutils executables ' +
                                '(e.g. for building PEXE files)')
    argparser.add_argument('--assemble', required=False,
                           action='store_true',
                           help='Assemble the output')
    argparser.add_argument('--disassemble', required=False,
                           action='store_true',
                           help='Disassemble the assembled output')
    argparser.add_argument('--dis-flags', required=False,
                           action='append', default=[],
                           help='Add a disassembler flag')
    argparser.add_argument('--filetype', default='iasm', dest='filetype',
                           choices=['obj', 'asm', 'iasm'],
                           help='Output file type.  Default %(default)s')
    argparser.add_argument('--forceasm', required=False, action='store_true',
                           help='Force --filetype=asm')
    argparser.add_argument('--target', default='x8632', dest='target',
                           choices=['x8632','x8664','arm32','mips32'],
                           help='Target architecture.  Default %(default)s')
    argparser.add_argument('--echo-cmd', required=False,
                           action='store_true',
                           help='Trace command that generates ICE instructions')
    argparser.add_argument('--tbc', required=False, action='store_true',
                           help='Input is textual bitcode (not .ll)')
    argparser.add_argument('--expect-fail', required=False, action='store_true',
                           help='Negate success of run by using LLVM not')
    argparser.add_argument('--allow-pnacl-reader-error-recovery',
                           action='store_true',
                           help='Continue parsing after first error')
    argparser.add_argument('--args', '-a', nargs=argparse.REMAINDER,
                           default=[],
                           help='Remaining arguments are passed to pnacl-sz')
    argparser.add_argument('--sandbox', required=False, action='store_true',
                           help='Sandboxes the generated code')

    args = argparser.parse_args()
    pnacl_bin_path = args.pnacl_bin_path
    llfile = args.input

    if args.llvm and args.llvm_source:
      raise RuntimeError("Can't specify both '--llvm' and '--llvm-source'")

    if args.llvm_source and args.no_local_syms:
      raise RuntimeError("Can't specify both '--llvm-source' and " +
                         "'--no-local-syms'")

    if args.llvm_source and args.tbc:
      raise RuntimeError("Can't specify both '--tbc' and '--llvm-source'")

    if args.llvm and args.tbc:
      raise RuntimeError("Can't specify both '--tbc' and '--llvm'")

    if args.forceasm:
      if args.expect_fail:
        args.forceasm = False
      elif args.filetype == 'asm':
        pass
      elif args.filetype == 'iasm':
        # TODO(sehr) implement forceasm for iasm.
        pass
      elif args.filetype == 'obj':
        args.filetype = 'asm'
        args.assemble = True

    cmd = []
    if args.tbc:
      cmd = [os.path.join(pnacl_bin_path, 'pnacl-bcfuzz'), llfile,
             '-bitcode-as-text', '-output', '-', '|']
    elif not args.llvm_source:
      cmd = [os.path.join(pnacl_bin_path, 'llvm-as'), llfile, '-o', '-', '|',
             os.path.join(pnacl_bin_path, 'pnacl-freeze')]
      if not args.no_local_syms:
        cmd += ['--allow-local-symbol-tables']
      cmd += ['|']
    if args.expect_fail:
      cmd += [os.path.join(pnacl_bin_path, 'not')]
    cmd += [args.pnacl_sz]
    cmd += ['--target', args.target]
    if args.sandbox:
      cmd += ['-sandbox']
    if args.insts:
      # If the tests are based on '-verbose inst' output, force
      # single-threaded translation because dump output does not get
      # reassembled into order.
      cmd += ['-verbose', 'inst,global_init', '-notranslate', '-threads=0']
    elif args.allow_pnacl_reader_error_recovery:
      cmd += ['-allow-pnacl-reader-error-recovery', '-threads=0']
    if not args.llvm_source:
      cmd += ['--bitcode-format=pnacl']
      if not args.no_local_syms:
        cmd += ['--allow-local-symbol-tables']
    if args.llvm or args.llvm_source:
      cmd += ['--build-on-read=0']
    else:
      cmd += ['--build-on-read=1']
    cmd += ['--filetype=' + args.filetype]
    cmd += ['--emit-revision=0']
    script_name = os.path.basename(sys.argv[0])
    for _, arg in enumerate(args.args):
      # Redirecting the output file needs to be done through the script
      # because forceasm may introduce a new temporary file between pnacl-sz
      # and llvm-mc.  Similar issues could occur when setting filetype, target,
      # or sandbox through --args.  Filter and report an error.
      if re.search('^-?-(o|output|filetype|target|sandbox)(=.+)?$', arg):
        preferred_option = '--output' if re.search('^-?-o(=.+)?$', arg) else arg
        print 'Option should be set using:'
        print '    %s ... %s ... --args' % (script_name, preferred_option)
        print 'rather than:'
        print '    %s ... --args %s ...' % (script_name, arg)
        exit(1)
    asm_temp = None
    output_file_name = None
    keep_output_file = False
    if args.output:
      output_file_name = args.output
      keep_output_file = True
    cmd += args.args
    if args.llvm_source:
      cmd += [llfile]
    if args.assemble or args.disassemble:
      if not output_file_name:
        # On windows we may need to close the file first before it can be
        # re-opened by the other tools, so don't do delete-on-close,
        # and instead manually delete.
        asm_temp = tempfile.NamedTemporaryFile(delete=False)
        asm_temp.close()
        output_file_name = asm_temp.name
    if args.assemble and args.filetype != 'obj':
      cmd += (['|', os.path.join(pnacl_bin_path, 'llvm-mc')] +
              TargetAssemblerFlags(args.target, args.sandbox) +
              ['-filetype=obj', '-o', output_file_name])
    elif output_file_name:
      cmd += ['-o', output_file_name]
    if args.disassemble:
      # Show wide instruction encodings, diassemble, show relocs and
      # dissasemble zeros.
      cmd += (['&&', os.path.join(pnacl_bin_path, GetObjdumpCmd(args.target))] +
              args.dis_flags +
              ['-w', '-d', '-r', '-z'] + TargetDisassemblerFlags(args.target) +
              [output_file_name])

    stdout_result = shellcmd(cmd, echo=args.echo_cmd)
    if not args.echo_cmd:
      sys.stdout.write(stdout_result)
    if asm_temp and not keep_output_file:
      os.remove(output_file_name)

if __name__ == '__main__':
    main()
