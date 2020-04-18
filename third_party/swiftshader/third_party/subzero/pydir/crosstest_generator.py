#!/usr/bin/env python2

import argparse
import collections
import ConfigParser
import os
import shutil
import sys

from utils import shellcmd
from utils import FindBaseNaCl

def Match(desc, includes, excludes, default_match):
  """Determines whether desc is a match against includes and excludes.

  'desc' is a set of attributes, and 'includes' and 'excludes' are lists of sets
  of attributes.

  If 'desc' matches any element from 'excludes', the result is False.
  Otherwise, if 'desc' matches any element from 'includes', the result is True.
  Otherwise, the 'default_match' value is returned.
  """
  for exclude in excludes:
    if exclude <= desc:
      return False
  for include in includes:
    if include <= desc:
      return True
  return default_match


def RunNativePrefix(toolchain_root, target, attr, run_cmd):
  """Returns a prefix for running an executable for the target.

  For example, we may be running an ARM or MIPS target executable on an
  x86 machine and need to use an emulator.
  """
  arch_map = { 'x8632' : '',
               'x8664' : '',
               'arm32' : os.path.join(toolchain_root, 'arm_trusted',
                                      'run_under_qemu_arm'),
               'mips32': os.path.join(toolchain_root, 'mips_trusted',
                                      'run_under_qemu_mips32'),
             }
  attr_map = collections.defaultdict(str, {
      'arm32-neon': ' -cpu cortex-a9',
      'arm32-hwdiv-arm': ' -cpu cortex-a15',
      'mips32-base': ' -cpu mips32r5-generic'})
  prefix = arch_map[target] + attr_map[target + '-' + attr]
  if target == 'mips32':
    prefix = 'QEMU_SET_ENV=LD_LIBRARY_PATH=/usr/mipsel-linux-gnu/lib/ ' + prefix
  return (prefix + ' ' + run_cmd) if prefix else run_cmd

def NonsfiLoaderArch(target):
  """Returns the arch for the nonsfi_loader"""
  arch_map = { 'arm32' : 'arm',
               'x8632' : 'x86-32',
               'mips32' : 'mips32',
             }
  return arch_map[target]


def main():
  """Framework for cross test generation and execution.

  Builds and executes cross tests from the space of all possible attribute
  combinations.  The space can be restricted by providing subsets of attributes
  to specifically include or exclude.
  """
  # pypath is where to find other Subzero python scripts.
  pypath = os.path.abspath(os.path.dirname(sys.argv[0]))
  root = FindBaseNaCl()

  # The rest of the attribute sets.
  targets = [ 'x8632', 'x8664', 'arm32', 'mips32' ]
  sandboxing = [ 'native', 'sandbox', 'nonsfi' ]
  opt_levels = [ 'Om1', 'O2' ]
  arch_attrs = { 'x8632': [ 'sse2', 'sse4.1' ],
                 'x8664': [ 'sse2', 'sse4.1' ],
                 'arm32': [ 'neon', 'hwdiv-arm' ],
                 'mips32': [ 'base' ]
               }
  flat_attrs = []
  for v in arch_attrs.values():
    flat_attrs += v
  arch_flags = { 'x8632': [],
                 'x8664': [],
                 'arm32': [],
                 'mips32': []
               }
  # all_keys is only used in the help text.
  all_keys = '; '.join([' '.join(targets), ' '.join(sandboxing),
                        ' '.join(opt_levels), ' '.join(flat_attrs)])

  argparser = argparse.ArgumentParser(
    description='  ' + main.__doc__ +
    'The set of attributes is the set of tests plus the following:\n' +
    all_keys, formatter_class=argparse.RawTextHelpFormatter)
  argparser.add_argument('--config', default='crosstest.cfg', dest='config',
                         metavar='FILE', help='Test configuration file')
  argparser.add_argument('--print-tests', default=False, action='store_true',
                         help='Print the set of test names and exit')
  argparser.add_argument('--include', '-i', default=[], dest='include',
                         action='append', metavar='ATTR_LIST',
                         help='Attributes to include (comma-separated). ' +
                              'Can be used multiple times.')
  argparser.add_argument('--exclude', '-e', default=[], dest='exclude',
                         action='append', metavar='ATTR_LIST',
                         help='Attributes to include (comma-separated). ' +
                              'Can be used multiple times.')
  argparser.add_argument('--verbose', '-v', default=False, action='store_true',
                         help='Use verbose output')
  argparser.add_argument('--defer', default=False, action='store_true',
                         help='Defer execution until all executables are built')
  argparser.add_argument('--no-compile', '-n', default=False,
                         action='store_true',
                         help="Don't build; reuse binaries from the last run")
  argparser.add_argument('--dir', dest='dir', metavar='DIRECTORY',
                         default=('{root}/toolchain_build/src/subzero/' +
                                  'crosstest/Output').format(root=root),
                         help='Output directory')
  argparser.add_argument('--lit', default=False, action='store_true',
                         help='Generate files for lit testing')
  argparser.add_argument('--toolchain-root', dest='toolchain_root',
                         default=(
                           '{root}/toolchain/linux_x86/pnacl_newlib_raw/bin'
                         ).format(root=root),
                         help='Path to toolchain binaries.')
  argparser.add_argument('--filetype', default=None, dest='filetype',
                         help='File type override, one of {asm, iasm, obj}.')
  args = argparser.parse_args()

  # Run from the crosstest directory to make it easy to grab inputs.
  crosstest_dir = '{root}/toolchain_build/src/subzero/crosstest'.format(
    root=root)
  os.chdir(crosstest_dir)

  tests = ConfigParser.RawConfigParser()
  tests.read('crosstest.cfg')

  if args.print_tests:
    print 'Test name attributes: ' + ' '.join(sorted(tests.sections()))
    sys.exit(0)

  # includes and excludes are both lists of sets.
  includes = [ set(item.split(',')) for item in args.include ]
  excludes = [ set(item.split(',')) for item in args.exclude ]
  # If any --include args are provided, the default is to not match.
  default_match = not args.include

  # Delete and recreate the output directory, unless --no-compile was specified.
  if not args.no_compile:
    if os.path.exists(args.dir):
      if os.path.isdir(args.dir):
        shutil.rmtree(args.dir)
      else:
        os.remove(args.dir)
    if not os.path.exists(args.dir):
      os.makedirs(args.dir)

  # If --defer is specified, collect the run commands into deferred_cmds for
  # later execution.
  deferred_cmds = []
  for test in sorted(tests.sections()):
    for target in targets:
      for sb in sandboxing:
        for opt in opt_levels:
          for attr in arch_attrs[target]:
            desc = [ test, target, sb, opt, attr ]
            if Match(set(desc), includes, excludes, default_match):
              exe = '{test}_{target}_{sb}_{opt}_{attr}'.format(
                test=test, target=target, sb=sb, opt=opt,
                attr=attr)
              extra = (tests.get(test, 'flags').split(' ')
                       if tests.has_option(test, 'flags') else [])
              if args.filetype:
                extra += ['--filetype={ftype}'.format(ftype=args.filetype)]
              # Generate the compile command.
              cmp_cmd = (
                ['{path}/crosstest.py'.format(path=pypath),
                 '-{opt}'.format(opt=opt),
                 '--mattr={attr}'.format(attr=attr),
                 '--prefix=Subzero_',
                 '--target={target}'.format(target=target),
                 '--nonsfi={nsfi}'.format(nsfi='1' if sb=='nonsfi' else '0'),
                 '--sandbox={sb}'.format(sb='1' if sb=='sandbox' else '0'),
                 '--dir={dir}'.format(dir=args.dir),
                 '--output={exe}'.format(exe=exe),
                 '--driver={drv}'.format(drv=tests.get(test, 'driver'))] +
                extra +
                ['--test=' + t
                 for t in tests.get(test, 'test').split(' ')] +
                arch_flags[target])
              run_cmd_base = os.path.join(args.dir, exe)
              # Generate the run command.
              run_cmd = run_cmd_base
              if sb == 'sandbox':
                run_cmd = '{root}/run.py -q '.format(root=root) + run_cmd
              elif sb == 'nonsfi':
                run_cmd = (
                    '{root}/scons-out/opt-linux-{arch}/obj/src/nonsfi/' +
                    'loader/nonsfi_loader ').format(
                        root=root, arch=NonsfiLoaderArch(target)) + run_cmd
                run_cmd = RunNativePrefix(args.toolchain_root, target, attr,
                                          run_cmd)
              else:
                run_cmd = RunNativePrefix(args.toolchain_root, target, attr,
                                          run_cmd)
              if args.lit:
                # Create a file to drive the lit test.
                with open(run_cmd_base + '.xtest', 'w') as f:
                  f.write('# RUN: sh %s | FileCheck %s\n')
                  f.write('cd ' + crosstest_dir + ' && \\\n')
                  f.write(' '.join(cmp_cmd) + ' && \\\n')
                  f.write(run_cmd + '\n')
                  f.write('echo Recreate a failure using ' + __file__ +
                          ' --toolchain-root=' + args.toolchain_root +
                          (' --filetype=' + args.filetype
                            if args.filetype else '') +
                          ' --include=' + ','.join(desc) + '\n')
                  f.write('# CHECK: Failures=0\n')
              else:
                if not args.no_compile:
                  shellcmd(cmp_cmd,
                           echo=args.verbose)
                if (args.defer):
                  deferred_cmds.append(run_cmd)
                else:
                  shellcmd(run_cmd, echo=True)
  for run_cmd in deferred_cmds:
    shellcmd(run_cmd, echo=True)

if __name__ == '__main__':
  main()
