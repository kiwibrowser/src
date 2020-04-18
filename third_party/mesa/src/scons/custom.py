"""custom

Custom builders and methods.

"""

#
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#


import os
import os.path
import re
import sys
import subprocess

import SCons.Action
import SCons.Builder
import SCons.Scanner

import fixes

import source_list

def quietCommandLines(env):
    # Quiet command lines
    # See also http://www.scons.org/wiki/HidingCommandLinesInOutput
    env['ASCOMSTR'] = "  Assembling $SOURCE ..."
    env['ASPPCOMSTR'] = "  Assembling $SOURCE ..."
    env['CCCOMSTR'] = "  Compiling $SOURCE ..."
    env['SHCCCOMSTR'] = "  Compiling $SOURCE ..."
    env['CXXCOMSTR'] = "  Compiling $SOURCE ..."
    env['SHCXXCOMSTR'] = "  Compiling $SOURCE ..."
    env['ARCOMSTR'] = "  Archiving $TARGET ..."
    env['RANLIBCOMSTR'] = "  Indexing $TARGET ..."
    env['LINKCOMSTR'] = "  Linking $TARGET ..."
    env['SHLINKCOMSTR'] = "  Linking $TARGET ..."
    env['LDMODULECOMSTR'] = "  Linking $TARGET ..."
    env['SWIGCOMSTR'] = "  Generating $TARGET ..."
    env['LEXCOMSTR'] = "  Generating $TARGET ..."
    env['YACCCOMSTR'] = "  Generating $TARGET ..."
    env['CODEGENCOMSTR'] = "  Generating $TARGET ..."
    env['INSTALLSTR'] = "  Installing $TARGET ..."


def createConvenienceLibBuilder(env):
    """This is a utility function that creates the ConvenienceLibrary
    Builder in an Environment if it is not there already.

    If it is already there, we return the existing one.

    Based on the stock StaticLibrary and SharedLibrary builders.
    """

    try:
        convenience_lib = env['BUILDERS']['ConvenienceLibrary']
    except KeyError:
        action_list = [ SCons.Action.Action("$ARCOM", "$ARCOMSTR") ]
        if env.Detect('ranlib'):
            ranlib_action = SCons.Action.Action("$RANLIBCOM", "$RANLIBCOMSTR")
            action_list.append(ranlib_action)

        convenience_lib = SCons.Builder.Builder(action = action_list,
                                  emitter = '$LIBEMITTER',
                                  prefix = '$LIBPREFIX',
                                  suffix = '$LIBSUFFIX',
                                  src_suffix = '$SHOBJSUFFIX',
                                  src_builder = 'SharedObject')
        env['BUILDERS']['ConvenienceLibrary'] = convenience_lib

    return convenience_lib


# TODO: handle import statements with multiple modules
# TODO: handle from import statements
import_re = re.compile(r'^import\s+(\S+)$', re.M)

def python_scan(node, env, path):
    # http://www.scons.org/doc/0.98.5/HTML/scons-user/c2781.html#AEN2789
    contents = node.get_contents()
    source_dir = node.get_dir()
    imports = import_re.findall(contents)
    results = []
    for imp in imports:
        for dir in path:
            file = os.path.join(str(dir), imp.replace('.', os.sep) + '.py')
            if os.path.exists(file):
                results.append(env.File(file))
                break
            file = os.path.join(str(dir), imp.replace('.', os.sep), '__init__.py')
            if os.path.exists(file):
                results.append(env.File(file))
                break
    return results

python_scanner = SCons.Scanner.Scanner(function = python_scan, skeys = ['.py'])


def code_generate(env, script, target, source, command):
    """Method to simplify code generation via python scripts.

    http://www.scons.org/wiki/UsingCodeGenerators
    http://www.scons.org/doc/0.98.5/HTML/scons-user/c2768.html
    """

    # We're generating code using Python scripts, so we have to be
    # careful with our scons elements.  This entry represents
    # the generator file *in the source directory*.
    script_src = env.File(script).srcnode()

    # This command creates generated code *in the build directory*.
    command = command.replace('$SCRIPT', script_src.path)
    action = SCons.Action.Action(command, "$CODEGENCOMSTR")
    code = env.Command(target, source, action)

    # Explicitly mark that the generated code depends on the generator,
    # and on implicitly imported python modules
    path = (script_src.get_dir(),)
    deps = [script_src]
    deps += script_src.get_implicit_deps(env, python_scanner, path)
    env.Depends(code, deps)

    # Running the Python script causes .pyc files to be generated in the
    # source directory.  When we clean up, they should go too. So add side
    # effects for .pyc files
    for dep in deps:
        pyc = env.File(str(dep) + 'c')
        env.SideEffect(pyc, code)

    return code


def createCodeGenerateMethod(env):
    env.Append(SCANNERS = python_scanner)
    env.AddMethod(code_generate, 'CodeGenerate')


def _pkg_check_modules(env, name, modules):
    '''Simple wrapper for pkg-config.'''

    env['HAVE_' + name] = False

    # For backwards compatability
    env[name.lower()] = False

    if env['platform'] == 'windows':
        return

    if not env.Detect('pkg-config'):
        return

    if subprocess.call(["pkg-config", "--exists", ' '.join(modules)]) != 0:
        return

    # Strip version expressions from modules
    modules = [module.split(' ', 1)[0] for module in modules]

    # Other flags may affect the compilation of unrelated targets, so store
    # them with a prefix, (e.g., XXX_CFLAGS, XXX_LIBS, etc)
    try:
        flags = env.ParseFlags('!pkg-config --cflags --libs ' + ' '.join(modules))
    except OSError:
        return
    prefix = name + '_'
    for flag_name, flag_value in flags.iteritems():
        assert '_' not in flag_name
        env[prefix + flag_name] = flag_value

    env['HAVE_' + name] = True

def pkg_check_modules(env, name, modules):

    sys.stdout.write('Checking for %s (%s)...' % (name, ' '.join(modules)))
    _pkg_check_modules(env, name, modules)
    result = env['HAVE_' + name]
    sys.stdout.write(' %s\n' % ['no', 'yes'][int(bool(result))])

    # XXX: For backwards compatability
    env[name.lower()] = result


def pkg_use_modules(env, names):
    '''Search for all environment flags that match NAME_FOO and append them to
    the FOO environment variable.'''

    names = env.Flatten(names)

    for name in names:
        prefix = name + '_'

        if not 'HAVE_' + name in env:
            raise Exception('Attempt to use unknown module %s' % name)

        if not env['HAVE_' + name]:
            raise Exception('Attempt to use unavailable module %s' % name)

        flags = {}
        for flag_name, flag_value in env.Dictionary().iteritems():
            if flag_name.startswith(prefix):
                flag_name = flag_name[len(prefix):]
                if '_' not in flag_name:
                    flags[flag_name] = flag_value
        if flags:
            env.MergeFlags(flags)


def createPkgConfigMethods(env):
    env.AddMethod(pkg_check_modules, 'PkgCheckModules')
    env.AddMethod(pkg_use_modules, 'PkgUseModules')


def parse_source_list(env, filename, names=None):
    # parse the source list file
    parser = source_list.SourceListParser()
    src = env.File(filename).srcnode()

    cur_srcdir = env.Dir('.').srcnode().abspath
    top_srcdir = env.Dir('#').abspath
    top_builddir = os.path.join(top_srcdir, env['build_dir'])

    # Populate the symbol table of the Makefile parser.
    parser.add_symbol('top_srcdir', top_srcdir)
    parser.add_symbol('top_builddir', top_builddir)

    sym_table = parser.parse(src.abspath)

    if names:
        if isinstance(names, basestring):
            names = [names]

        symbols = names
    else:
        symbols = sym_table.keys()

    # convert the symbol table to source lists
    src_lists = {}
    for sym in symbols:
        val = sym_table[sym]
        srcs = []
        for f in val.split():
            if f:
                # Process source paths
                if f.startswith(top_builddir + '/src'):
                    # Automake puts build output on a `src` subdirectory, bue
                    # SCons does no, so strip it here.
                    f = top_builddir + f[len(top_builddir + '/src'):]
                if f.startswith(cur_srcdir + '/'):
                    # Prefer relative source paths, as absolute files tend to
                    # cause duplicate actions.
                    f = f[len(cur_srcdir + '/'):]
                srcs.append(f)

        src_lists[sym] = srcs

    # if names are given, concatenate the lists
    if names:
        srcs = []
        for name in names:
            srcs.extend(src_lists[name])

        return srcs
    else:
        return src_lists

def createParseSourceListMethod(env):
    env.AddMethod(parse_source_list, 'ParseSourceList')


def generate(env):
    """Common environment generation code"""

    verbose = env.get('verbose', False) or not env.get('quiet', True)
    if not verbose:
        quietCommandLines(env)

    # Custom builders and methods
    createConvenienceLibBuilder(env)
    createCodeGenerateMethod(env)
    createPkgConfigMethods(env)
    createParseSourceListMethod(env)

    # for debugging
    #print env.Dump()


def exists(env):
    return 1
