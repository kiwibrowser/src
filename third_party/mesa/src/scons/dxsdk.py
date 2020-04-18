"""dxsdk

Tool-specific initialization for Microsoft DirectX SDK

"""

#
# Copyright (c) 2009 VMware, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import os
import os.path

import SCons.Errors
import SCons.Util


def get_dxsdk_root(env):
    try:
        return os.environ['DXSDK_DIR']
    except KeyError:
        return None

def generate(env):
    dxsdk_root = get_dxsdk_root(env)
    if dxsdk_root is None:
        # DirectX SDK not found
        return

    if env['machine'] in ('generic', 'x86'):
        target_cpu = 'x86'
    elif env['machine'] == 'x86_64':
        target_cpu = 'x64'
    else:
        raise SCons.Errors.InternalError, "Unsupported target machine"

    include_dir = os.path.join(dxsdk_root, 'Include')
    lib_dir = os.path.join(dxsdk_root, 'Lib', target_cpu)

    env.Append(CPPDEFINES = [('HAVE_DXSDK', '1')])

    gcc = 'gcc' in os.path.basename(env['CC']).split('-')
    if gcc:
        # Make GCC more forgiving towards Microsoft's headers
        env.Prepend(CPPFLAGS = ['-isystem', include_dir])
    else:
        env.Prepend(CPPPATH = [include_dir])

    env.Prepend(LIBPATH = [lib_dir])

def exists(env):
    return get_dxsdk_root(env) is not None

# vim:set ts=4 sw=4 et:
