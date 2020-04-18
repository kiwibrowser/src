# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import lib2to3.refactor

# Put blinkpy/third_party/ in the import path for autopep8 to import pep8.
from blinkpy.common.path_finder import add_blinkpy_thirdparty_dir_to_sys_path
add_blinkpy_thirdparty_dir_to_sys_path()

from blinkpy.common.system.system_host import SystemHost
from blinkpy.third_party import autopep8


def parse_args(args=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('--chromium', action='store_const', dest='style', const='chromium', default='blink',
                        help="Format according to Chromium's Python coding styles instead of Blink's.")
    parser.add_argument('--no-backups', action='store_false', default=True, dest='backup',
                        help='Do not back up files before overwriting them.')
    parser.add_argument('-j', '--jobs', metavar='n', type=int, default=0,
                        help='Number of parallel jobs; match CPU count if less than 1.')
    parser.add_argument('files', nargs='*', default=['-'],
                        help="files to format or '-' for standard in")
    parser.add_argument('--double-quote-strings', action='store_const', dest='quoting', const='double', default='single',
                        help='Rewrite string literals to use double quotes instead of single quotes.')
    parser.add_argument('--no-autopep8', action='store_true',
                        help='Skip the autopep8 code-formatting step.')
    parser.add_argument('--leave-strings-alone', action='store_true',
                        help='Do not reformat string literals to use a consistent quote style.')
    return parser.parse_args(args=args)


def main(host=None, args=None):
    options = parse_args(args)
    if options.no_autopep8:
        options.style = None

    if options.leave_strings_alone:
        options.quoting = None

    autopep8_options = _autopep8_options_for_style(options.style)
    fixers = ['blinkpy.formatter.fix_docstrings']
    fixers.extend(_fixers_for_quoting(options.quoting))

    if options.files == ['-']:
        host = host or SystemHost()
        host.print_(reformat_source(host.stdin.read(), autopep8_options, fixers, '<stdin>'), end='')
        return

    # We create the arglist before checking if we need to create a Host, because a
    # real host is non-picklable and can't be passed to host.executive.map().

    arglist = [(host, name, autopep8_options, fixers, options.backup) for name in options.files]
    host = host or SystemHost()

    host.executive.map(_reformat_thunk, arglist, processes=options.jobs)


def _autopep8_options_for_style(style):
    return {
        None: [],
        'blink': autopep8.parse_args([
            '--aggressive',
            '--max-line-length', '132',
            '--ignore=E309',
            '--indent-size', '4',
            '',
        ]),
        'chromium': autopep8.parse_args([
            '--aggressive',
            '--max-line-length', '80',
            '--ignore=E309',
            '--indent-size', '2',
            '',
        ]),
    }.get(style)


def _fixers_for_quoting(quoting):
    return {
        None: [],
        'double': ['blinkpy.formatter.fix_double_quote_strings'],
        'single': ['blinkpy.formatter.fix_single_quote_strings'],
    }.get(quoting)


def _reformat_thunk(args):
    reformat_file(*args)


def reformat_file(host, name, autopep8_options, fixers, should_backup_file):
    host = host or SystemHost()
    source = host.filesystem.read_text_file(name)
    dest = reformat_source(source, autopep8_options, fixers, name)
    if dest != source:
        if should_backup_file:
            host.filesystem.write_text_file(name + '.bak', source)
        host.filesystem.write_text_file(name, dest)


def reformat_source(source, autopep8_options, fixers, name):
    tmp_str = source

    if autopep8_options:
        tmp_str = autopep8.fix_code(tmp_str, autopep8_options)

    if fixers:
        tool = lib2to3.refactor.RefactoringTool(fixer_names=fixers,
                                                explicit=fixers)
        tmp_str = unicode(tool.refactor_string(tmp_str, name=name))

    return tmp_str
