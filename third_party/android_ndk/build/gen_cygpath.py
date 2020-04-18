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
"""Generates a make function approximating cygpath.

We don't just call cygpath (unless directed by NDK_USE_CYGPATH=1) because we
have to call this very often and doing so would be very slow. By doing this in
make, we can be much faster.
"""
import posixpath
import re
import sys


def get_mounts(mount_output):
    """Parses the output of mount and returns a dict of mounts.

    Args:
        mount_output: The text output from mount(1).

    Returns:
        A list of tuples mapping cygwin paths to Windows paths.
    """
    mount_regex = re.compile(r'^(\S+) on (\S+) .*$')

    # We use a list of tuples rather than a dict because we want to recurse on
    # the list later anyway.
    mounts = []
    for line in mount_output.splitlines():
        # Cygwin's mount doesn't use backslashes even in Windows paths, so no
        # need to replace here.
        match = mount_regex.search(line)
        if match is not None:
            win_path = match.group(1)
            cyg_path = match.group(2)
            if cyg_path == '/':
                # Since we're going to be using patsubst on these, we need to
                # make sure that the rule for / is applied last, otherwise
                # we'll replace all other cygwin paths with that one.
                mounts.insert(0, (cyg_path, win_path))
            elif cyg_path.startswith('/cygdrive/'):
                # We need both /cygdrive/c and /cygdrive/C to point to C:.
                letter = posixpath.basename(cyg_path)
                lower_path = posixpath.join('/cygdrive', letter.lower())
                upper_path = posixpath.join('/cygdrive', letter.upper())
                mounts.append((lower_path, win_path))
                mounts.append((upper_path, win_path))
            else:
                mounts.append((cyg_path, win_path))

    return mounts


def make_cygpath_function(mounts):
    """Creates a make function that can be used in place of cygpath.

    Args:
        mounts: A list of tuples decribing filesystem mounts.

    Returns:
        The body of a function implementing cygpath in make as a string.
    """
    # We're building a bunch of nested patsubst calls. Once we've written each
    # of the calls, we pass the function input to the inner most call.
    if len(mounts) == 0:
        return '$1'

    cyg_path, win_path = mounts[0]
    if not cyg_path.endswith('/'):
        cyg_path += '/'
    if not win_path.endswith('/'):
        win_path += '/'

    other_mounts = mounts[1:]
    return '$(patsubst {}%,{}%,\n{})'.format(
        cyg_path, win_path, make_cygpath_function(other_mounts))


def main():
    # We're invoked from make and piped the output of `mount` so we can
    # determine what mappings to make.
    mount_output = sys.stdin.read()
    mounts = get_mounts(mount_output)
    print make_cygpath_function(mounts)


if __name__ == '__main__':
    main()
