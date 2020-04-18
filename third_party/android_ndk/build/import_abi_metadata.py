#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Generates Make-importable code from meta/abis.json."""
import argparse
import json
import os


NEWLINE = '%NEWLINE%'


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        'abis_file', metavar='ABIS_FILE', type=os.path.abspath,
        help='Path to the abis.json file.')

    return parser.parse_args()


def generate_make_vars(abi_vars):
    lines = []
    for var, value in abi_vars.items():
        lines.append('{} := {}'.format(var, value))
    # https://www.gnu.org/software/make/manual/html_node/Shell-Function.html
    # Make's $(shell) function replaces real newlines with spaces. Use
    # something we can easily identify that's unlikely to appear in a variable
    # so we can replace it in make.
    return NEWLINE.join(lines)


def metadata_to_make_vars(meta):
    default_abis = []
    deprecated_abis = []
    lp32_abis = []
    lp64_abis = []
    for abi, abi_data in meta.items():
        bitness = abi_data['bitness']
        if bitness == 32:
            lp32_abis.append(abi)
        elif bitness == 64:
            lp64_abis.append(abi)
        else:
            raise ValueError('{} bitness is unsupported value: {}'.format(
                abi, bitness))

        if abi_data['default']:
            default_abis.append(abi)

        if abi_data['deprecated']:
            deprecated_abis.append(abi)

    abi_vars = {
        'NDK_DEFAULT_ABIS': ' '.join(sorted(default_abis)),
        'NDK_DEPRECATED_ABIS': ' '.join(sorted(deprecated_abis)),
        'NDK_KNOWN_DEVICE_ABI32S': ' '.join(sorted(lp32_abis)),
        'NDK_KNOWN_DEVICE_ABI64S': ' '.join(sorted(lp64_abis)),
    }

    return abi_vars


def main():
    args = parse_args()
    with open(args.abis_file) as abis_file:
        abis = json.load(abis_file)

    abi_vars = metadata_to_make_vars(abis)
    print generate_make_vars(abi_vars)


if __name__ == '__main__':
    main()
