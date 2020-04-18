#!/usr/bin/env python2

import argparse
import os
import sys

import szbuild

from utils import FindBaseNaCl, shellcmd

def main():
    """Build native gcc-style executables for one or all Spec2K components.

    Afterwards, the executables can be run from the
    native_client/tests/spec2k/ directory as:
    './run_all.sh RunBenchmarks SetupGccX8632Opt {train|ref} ...'
    -- or --
    './run_all.sh RunBenchmarks SetupPnaclX8632Opt {train|ref} ...'
    -- or --
    './run_all.sh RunBenchmarks SetupNonsfiX8632Opt {train|ref} ...'
    """
    nacl_root = FindBaseNaCl()
    # Use the same default ordering as spec2k/run_all.sh.
    components = [ '177.mesa', '179.art', '183.equake', '188.ammp', '164.gzip',
                   '175.vpr', '176.gcc', '181.mcf', '186.crafty', '197.parser',
                   '253.perlbmk', '254.gap', '255.vortex', '256.bzip2',
                   '300.twolf', '252.eon' ]

    argparser = argparse.ArgumentParser(description=main.__doc__)
    szbuild.AddOptionalArgs(argparser)
    argparser.add_argument('--run', dest='run', action='store_true',
                           help='Run after building')
    argparser.add_argument('comps', nargs='*', default=components)
    args = argparser.parse_args()
    bad = set(args.comps) - set(components)
    if bad:
        print 'Unknown component{s}: '.format(s='s' if len(bad) > 1 else '') + \
            ' '.join(x for x in bad)
        sys.exit(1)

    # Fix up Subzero target strings for the run_all.sh script.
    target_map = {
         'arm32':'arm',
         'x8632':'x8632',
         'x8664':'x8664'
         }
    run_all_target = target_map[args.target] # fail if target not listed above

    suffix = (
        'pnacl.opt.{target}' if args.sandbox else
        'nonsfi.opt.{target}' if args.nonsfi else
        'gcc.opt.{target}').format(
             target=run_all_target);
    for comp in args.comps:
        name = os.path.splitext(comp)[1] or comp
        if name[0] == '.':
            name = name[1:]
        szbuild.ProcessPexe(args,
                            ('{root}/tests/spec2k/{comp}/' +
                             '{name}.opt.stripped.pexe'
                             ).format(root=nacl_root, comp=comp, name=name),
                            ('{root}/tests/spec2k/{comp}/' +
                             '{name}.{suffix}'
                             ).format(root=nacl_root, comp=comp, name=name,
                                      suffix=suffix))
    if args.run:
        os.chdir('{root}/tests/spec2k'.format(root=FindBaseNaCl()))
        setup = 'Setup' + ('Pnacl' if args.sandbox else
                           'Nonsfi' if args.nonsfi else
                           'Gcc') + {
            'arm32': 'Arm',
            'x8632': 'X8632',
            'x8664': 'X8664'}[args.target] + 'Opt'
        shellcmd(['./run_all.sh',
                  'RunTimedBenchmarks',
                  setup,
                  'train'] + args.comps)

if __name__ == '__main__':
    main()
