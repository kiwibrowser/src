import os
import shutil
import subprocess
import sys
import tempfile

from utils import FindBaseNaCl, shellcmd

def subsToMacros(subs, src):
    macros = ['#include <stddef.h>',
              '#ifdef __cplusplus',
              'extern "C" {',
              '#endif']
    for func in subs:
        args = [('{atype} a{num}').format(atype=atype, num=i) for
                i, atype in enumerate(subs[func]['sig'][1:])]
        macros.append((
            '{ftype} {name}({args});'
            ).format(ftype=subs[func]['sig'][0],
                     name=subs[func]['sub'],
                     args=', '.join(args)))
        macros.append((
            '#define {func}(args...) ({sub}(args))'
            ).format(func=func, sub=subs[func]['sub']))
    macros += ['#ifdef __cplusplus',
               '} // extern "C"',
               '#endif',
               '#line 1 "{src}"'.format(src=src)]
    return '\n'.join(macros) + '\n'

def run(is_cpp):
    """Passes its arguments directly to pnacl-clang.

    If -fsanitize-address is specified, extra information is passed to
    pnacl-clang to ensure that later instrumentation in pnacl-sz can be
    performed. For example, clang automatically inlines many memory allocation
    functions, so this script will redefine them at compile time to make sure
    they can be correctly instrumented by pnacl-sz.
    """
    pnacl_root = FindBaseNaCl()
    dummy_subs = {'calloc': {'sig': ['void *', 'size_t', 'size_t'],
                             'sub': '__asan_dummy_calloc'},
                  '_calloc': {'sig': ['void *', 'size_t', 'size_t'],
                              'sub': '__asan_dummy_calloc'}}
    subs_src = (
        '{root}/toolchain_build/src/subzero/pydir/sz_clang_dummies.c'
        ).format(root=pnacl_root)
    clang = (
        '{root}/toolchain/linux_x86/pnacl_newlib_raw/bin/pnacl-clang{pp}'
        ).format(root=pnacl_root, pp='++' if is_cpp else '')
    args = sys.argv
    args[0] = clang
    tmp_dir = ''
    if '-fsanitize-address' in args:
        args.remove('-fsanitize-address')
        include_dirs = set()
        tmp_dir = tempfile.mkdtemp()
        for i, arg in enumerate(args[1:], 1):
            if not os.path.isfile(arg):
                continue
            src = os.path.basename(arg)
            ext = os.path.splitext(arg)[1]
            if ext in ['.c', '.cc', '.cpp']:
                include_dirs |= {os.path.dirname(arg)}
                dest_name = os.path.join(tmp_dir, src)
                with open(dest_name, 'w') as dest:
                    dest.write(subsToMacros(dummy_subs, arg))
                    with open(arg) as src:
                        for line in src:
                            dest.write(line)
                args[i] = dest_name
        # If linking (not single file compilation) then add dummy definitions
        if not ('-o' in args and
                ('-c' in args or '-S' in args or '-E' in args)):
            args.append(subs_src)
        for d in include_dirs:
            args.append('-iquote {d}'.format(d=d))
        if '-fno-inline' not in args:
            args.append('-fno-inline')
    err_code = 0
    try:
        shellcmd(args, echo=True)
    except subprocess.CalledProcessError as e:
        print e.output
        err_code = e.returncode
    if tmp_dir != '':
        shutil.rmtree(tmp_dir)
    exit(err_code)
