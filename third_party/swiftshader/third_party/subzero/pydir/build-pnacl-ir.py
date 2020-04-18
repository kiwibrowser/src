#!/usr/bin/env python2

import argparse
import errno
import os
import shutil
import tempfile
from utils import shellcmd
from utils import FindBaseNaCl

if __name__ == '__main__':
    argparser = argparse.ArgumentParser()
    argparser.add_argument('cfile', nargs='+', type=str,
        help='C file(s) to convert')
    argparser.add_argument('--dir', nargs='?', type=str, default='.',
                           help='Output directory. Default "%(default)s".')
    argparser.add_argument('--disable-verify', action='store_true')
    args = argparser.parse_args()

    nacl_root = FindBaseNaCl()
    # Prepend bin to $PATH.
    os.environ['PATH'] = (
        nacl_root + '/toolchain/linux_x86/pnacl_newlib_raw/bin' + os.pathsep +
        os.pathsep + os.environ['PATH'])

    try:
        tempdir = tempfile.mkdtemp()

        for cname in args.cfile:
            basename = os.path.splitext(cname)[0]
            llname = os.path.join(tempdir, basename + '.ll')
            pnaclname = basename + '.pnacl.ll'
            pnaclname = os.path.join(args.dir, pnaclname)

            shellcmd('pnacl-clang -O2 -c {0} -o {1}'.format(cname, llname))
            shellcmd('pnacl-opt ' +
                     '-pnacl-abi-simplify-preopt -pnacl-abi-simplify-postopt' +
                     ('' if args.disable_verify else
                      ' -verify-pnaclabi-module -verify-pnaclabi-functions') +
                     ' -pnaclabi-allow-debug-metadata'
                     ' {0} -S -o {1}'.format(llname, pnaclname))
    finally:
        try:
            shutil.rmtree(tempdir)
        except OSError as exc:
            if exc.errno != errno.ENOENT: # ENOENT - no such file or directory
                raise # re-raise exception
