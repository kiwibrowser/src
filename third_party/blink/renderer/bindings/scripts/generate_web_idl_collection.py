# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a data collection of IDL information per component.
In this data collection, we use identifier strings to point IDL definitions
(i.e. interface, dictionary, namespace, etc.) instead of references, because
some referred definitions can be in other components.
"""


import blink_idl_parser
import optparse
import utilities
from web_idl.collector import Collector


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-list-file', help='a file path which lists IDL file paths to process')
    parser.add_option('--component', help='decide which component to collect IDLs', default=None)
    parser.add_option('--output', help='pickle file of IDL definition')
    options, args = parser.parse_args()

    if options.idl_list_file is None:
        parser.error('Must specify a file listing IDL files using --idl-files-list.')
    if options.output is None:
        parser.error('Must specify a pickle file to output using --output.')

    return options, args


def main():
    options, _ = parse_options()
    idl_file_names = utilities.read_idl_files_list_from_file(options.idl_list_file, False)

    parser = blink_idl_parser.BlinkIDLParser()
    collector = Collector(component=options.component, parser=parser)
    collector.collect_from_idl_files(idl_file_names)
    utilities.write_pickle_file(options.output, collector.get_collection())


if __name__ == '__main__':
    main()
