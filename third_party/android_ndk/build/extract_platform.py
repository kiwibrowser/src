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
"""Extracts the platform version from the project.properties file."""
import argparse
import os.path
import re


def parse_args():
    """Parse and return command line arguments."""
    parser = argparse.ArgumentParser()

    parser.add_argument(
        'properties_file', metavar='PROPERTIES_FILE', type=os.path.abspath,
        help='Path to the project.properties file.')

    return parser.parse_args()


def get_platform(properties_file):
    """Finds and returns the platform version in the properties file.

    Returns:
        String form of the platform version if found, else "unknown".
    """
    android_regex = re.compile(r'(android-\w+)')
    vendor_regex = re.compile(r':(\d+)\s*$')
    for line in properties_file:
        match = android_regex.search(line)
        if match is not None:
            return match.group(1)
        match = vendor_regex.search(line)
        if match is not None:
            return 'android-{}'.format(match.group(1))
    return 'unknown'


def main():
    args = parse_args()

    # Following the comment in the old awk script, we're trying to match:
    #
    #    target=android-<api>
    #    target=<vendor>:<name>:<api>
    #
    # There unfortunately aren't any examples of what the vendor target
    # specification actually looks like or where it might be used, so we'll
    # just have to mirror the simplistic match that was in the awk script.
    #
    # android- may be followed by either the numeric API level or the named
    # platform. Note that while we can parse any name, ndk-build only support a
    # small handful.
    with open(args.properties_file) as properties_file:
        print get_platform(properties_file)


if __name__ == '__main__':
    main()
