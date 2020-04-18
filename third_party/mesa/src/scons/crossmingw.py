"""SCons.Tool.gcc

Tool-specific initialization for MinGW (http://www.mingw.org/)

There normally shouldn't be any need to import this module directly.
It will usually be imported through the generic SCons.Tool.Tool()
selection method.

See also http://www.scons.org/wiki/CrossCompilingMingw
"""

#
# Copyright (c) 2001, 2002, 2003, 2004 The SCons Foundation
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
import string

import SCons.Action
import SCons.Builder
import SCons.Tool
import SCons.Util

# This is what we search for to find mingw:
prefixes32 = SCons.Util.Split("""
    mingw32-
    mingw32msvc-
    i386-mingw32-
    i486-mingw32-
    i586-mingw32-
    i686-mingw32-
    i386-mingw32msvc-
    i486-mingw32msvc-
    i586-mingw32msvc-
    i686-mingw32msvc-
    i686-pc-mingw32-
    i686-w64-mingw32-
""")
prefixes64 = SCons.Util.Split("""
    x86_64-w64-mingw32-
    amd64-mingw32-
    amd64-mingw32msvc-
    amd64-pc-mingw32-
""")

def find(env):
    if env['machine'] == 'x86_64':
        prefixes = prefixes64
    else:
        prefixes = prefixes32
    for prefix in prefixes:
        # First search in the SCons path and then the OS path:
        if env.WhereIs(prefix + 'gcc') or SCons.Util.WhereIs(prefix + 'gcc'):
            return prefix

    return ''

def shlib_generator(target, source, env, for_signature):
    cmd = SCons.Util.CLVar(['$SHLINK', '$SHLINKFLAGS']) 

    dll = env.FindIxes(target, 'SHLIBPREFIX', 'SHLIBSUFFIX')
    if dll: cmd.extend(['-o', dll])

    cmd.extend(['$SOURCES', '$_LIBDIRFLAGS', '$_LIBFLAGS'])

    implib = env.FindIxes(target, 'LIBPREFIX', 'LIBSUFFIX')
    if implib: cmd.append('-Wl,--out-implib,'+implib.get_string(for_signature))

    def_target = env.FindIxes(target, 'WIN32DEFPREFIX', 'WIN32DEFSUFFIX')
    if def_target: cmd.append('-Wl,--output-def,'+def_target.get_string(for_signature))

    return [cmd]

def shlib_emitter(target, source, env):
    dll = env.FindIxes(target, 'SHLIBPREFIX', 'SHLIBSUFFIX')
    no_import_lib = env.get('no_import_lib', 0)

    if not dll:
        raise SCons.Errors.UserError, "A shared library should have exactly one target with the suffix: %s" % env.subst("$SHLIBSUFFIX")
    
    if not no_import_lib and \
       not env.FindIxes(target, 'LIBPREFIX', 'LIBSUFFIX'):

        # Append an import library to the list of targets.
        target.append(env.ReplaceIxes(dll,  
                                      'SHLIBPREFIX', 'SHLIBSUFFIX',
                                      'LIBPREFIX', 'LIBSUFFIX'))

    # Append a def file target if there isn't already a def file target
    # or a def file source. There is no option to disable def file
    # target emitting, because I can't figure out why someone would ever
    # want to turn it off.
    def_source = env.FindIxes(source, 'WIN32DEFPREFIX', 'WIN32DEFSUFFIX')
    def_target = env.FindIxes(target, 'WIN32DEFPREFIX', 'WIN32DEFSUFFIX')
    if not def_source and not def_target:
        target.append(env.ReplaceIxes(dll,  
                                      'SHLIBPREFIX', 'SHLIBSUFFIX',
                                      'WIN32DEFPREFIX', 'WIN32DEFSUFFIX'))
    
    return (target, source)
                         

shlib_action = SCons.Action.Action(shlib_generator, '$SHLINKCOMSTR', generator=1)

res_action = SCons.Action.Action('$RCCOM', '$RCCOMSTR')

res_builder = SCons.Builder.Builder(action=res_action, suffix='.o',
                                    source_scanner=SCons.Tool.SourceFileScanner)
SCons.Tool.SourceFileScanner.add_scanner('.rc', SCons.Defaults.CScan)



def compile_without_gstabs(env, sources, c_file):
    '''This is a hack used to compile some source files without the
    -gstabs option.

    It seems that some versions of mingw32's gcc (4.4.2 at least) die
    when compiling large files with the -gstabs option.  -gstabs is
    related to debug symbols and can be omitted from the effected
    files.

    This function compiles the given c_file without -gstabs, removes
    the c_file from the sources list, then appends the new .o file to
    sources.  Then return the new sources list.
    '''

    # Modify CCFLAGS to not have -gstabs option:
    env2 = env.Clone()
    flags = str(env2['CCFLAGS'])
    flags = flags.replace("-gstabs", "")
    env2['CCFLAGS'] = SCons.Util.CLVar(flags)
    
    # Build the special-case files:
    obj_file = env2.SharedObject(c_file)

    # Replace ".cpp" or ".c" with ".o"
    o_file = c_file.replace(".cpp", ".o")
    o_file = o_file.replace(".c", ".o")

    # Replace the .c files with the specially-compiled .o file
    sources.remove(c_file)
    sources.append(o_file)

    return sources


def generate(env):
    mingw_prefix = find(env)

    if mingw_prefix:
        dir = os.path.dirname(env.WhereIs(mingw_prefix + 'gcc') or SCons.Util.WhereIs(mingw_prefix + 'gcc'))

        # The mingw bin directory must be added to the path:
        path = env['ENV'].get('PATH', [])
        if not path: 
            path = []
        if SCons.Util.is_String(path):
            path = string.split(path, os.pathsep)

        env['ENV']['PATH'] = string.join([dir] + path, os.pathsep)

    # Most of mingw is the same as gcc and friends...
    gnu_tools = ['gcc', 'g++', 'gnulink', 'ar', 'gas']
    for tool in gnu_tools:
        SCons.Tool.Tool(tool)(env)

    #... but a few things differ:
    env['CC'] = mingw_prefix + 'gcc'
    env['SHCCFLAGS'] = SCons.Util.CLVar('$CCFLAGS')
    env['CXX'] = mingw_prefix + 'g++'
    env['SHCXXFLAGS'] = SCons.Util.CLVar('$CXXFLAGS')
    env['SHLINKFLAGS'] = SCons.Util.CLVar('$LINKFLAGS -shared')
    env['SHLINKCOM']   = shlib_action
    env.Append(SHLIBEMITTER = [shlib_emitter])
    env['LINK'] = mingw_prefix + 'g++'
    env['AR'] = mingw_prefix + 'ar'
    env['RANLIB'] = mingw_prefix + 'ranlib'
    env['LINK'] = mingw_prefix + 'g++'
    env['AS'] = mingw_prefix + 'as'
    env['WIN32DEFPREFIX']        = ''
    env['WIN32DEFSUFFIX']        = '.def'
    env['SHOBJSUFFIX'] = '.o'
    env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1

    env['RC'] = mingw_prefix + 'windres'
    env['RCFLAGS'] = SCons.Util.CLVar('')
    env['RCCOM'] = '$RC $_CPPDEFFLAGS $_CPPINCFLAGS ${INCPREFIX}${SOURCE.dir} $RCFLAGS -i $SOURCE -o $TARGET'
    env['BUILDERS']['RES'] = res_builder
    
    # Some setting from the platform also have to be overridden:
    env['OBJPREFIX']      = ''
    env['OBJSUFFIX']      = '.o'
    env['SHOBJPREFIX']    = '$OBJPREFIX'
    env['SHOBJSUFFIX']    = '$OBJSUFFIX'
    env['PROGPREFIX']     = ''
    env['PROGSUFFIX']     = '.exe'
    env['LIBPREFIX']      = 'lib'
    env['LIBSUFFIX']      = '.a'
    env['SHLIBPREFIX']    = ''
    env['SHLIBSUFFIX']    = '.dll'
    env['LIBPREFIXES']    = [ 'lib', '' ]
    env['LIBSUFFIXES']    = [ '.a', '.lib' ]

    # MinGW x86 port of gdb does not handle well dwarf debug info which is the
    # default in recent gcc versions.  The x64 port gdb from mingw-w64 seems to
    # handle it fine though, so stick with the default there.
    if env['machine'] != 'x86_64':
        env.AppendUnique(CCFLAGS = ['-gstabs'])

    env.AddMethod(compile_without_gstabs, 'compile_without_gstabs')

def exists(env):
    return find(env)
