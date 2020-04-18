"""gallium

Frontend-tool for Gallium3D architecture.

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


import distutils.version
import os
import os.path
import re
import subprocess
import platform as _platform

import SCons.Action
import SCons.Builder
import SCons.Scanner


def symlink(target, source, env):
    target = str(target[0])
    source = str(source[0])
    if os.path.islink(target) or os.path.exists(target):
        os.remove(target)
    os.symlink(os.path.basename(source), target)

def install(env, source, subdir):
    target_dir = os.path.join(env.Dir('#.').srcnode().abspath, env['build_dir'], subdir)
    return env.Install(target_dir, source)

def install_program(env, source):
    return install(env, source, 'bin')

def install_shared_library(env, sources, version = ()):
    targets = []
    install_dir = os.path.join(env.Dir('#.').srcnode().abspath, env['build_dir'])
    version = tuple(map(str, version))
    if env['SHLIBSUFFIX'] == '.dll':
        dlls = env.FindIxes(sources, 'SHLIBPREFIX', 'SHLIBSUFFIX')
        targets += install(env, dlls, 'bin')
        libs = env.FindIxes(sources, 'LIBPREFIX', 'LIBSUFFIX')
        targets += install(env, libs, 'lib')
    else:
        for source in sources:
            target_dir =  os.path.join(install_dir, 'lib')
            target_name = '.'.join((str(source),) + version)
            last = env.InstallAs(os.path.join(target_dir, target_name), source)
            targets += last
            while len(version):
                version = version[:-1]
                target_name = '.'.join((str(source),) + version)
                action = SCons.Action.Action(symlink, "  Symlinking $TARGET ...")
                last = env.Command(os.path.join(target_dir, target_name), last, action) 
                targets += last
    return targets


def createInstallMethods(env):
    env.AddMethod(install_program, 'InstallProgram')
    env.AddMethod(install_shared_library, 'InstallSharedLibrary')


def num_jobs():
    try:
        return int(os.environ['NUMBER_OF_PROCESSORS'])
    except (ValueError, KeyError):
        pass

    try:
        return os.sysconf('SC_NPROCESSORS_ONLN')
    except (ValueError, OSError, AttributeError):
        pass

    try:
        return int(os.popen2("sysctl -n hw.ncpu")[1].read())
    except ValueError:
        pass

    return 1


def generate(env):
    """Common environment generation code"""

    # Tell tools which machine to compile for
    env['TARGET_ARCH'] = env['machine']
    env['MSVS_ARCH'] = env['machine']

    # Toolchain
    platform = env['platform']
    env.Tool(env['toolchain'])

    # Allow override compiler and specify additional flags from environment
    if os.environ.has_key('CC'):
        env['CC'] = os.environ['CC']
        # Update CCVERSION to match
        pipe = SCons.Action._subproc(env, [env['CC'], '--version'],
                                     stdin = 'devnull',
                                     stderr = 'devnull',
                                     stdout = subprocess.PIPE)
        if pipe.wait() == 0:
            line = pipe.stdout.readline()
            match = re.search(r'[0-9]+(\.[0-9]+)+', line)
            if match:
                env['CCVERSION'] = match.group(0)
    if os.environ.has_key('CFLAGS'):
        env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
    if os.environ.has_key('CXX'):
        env['CXX'] = os.environ['CXX']
    if os.environ.has_key('CXXFLAGS'):
        env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
    if os.environ.has_key('LDFLAGS'):
        env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

    env['gcc'] = 'gcc' in os.path.basename(env['CC']).split('-')
    env['msvc'] = env['CC'] == 'cl'
    env['suncc'] = env['platform'] == 'sunos' and os.path.basename(env['CC']) == 'cc'
    env['clang'] = env['CC'] == 'clang'
    env['icc'] = 'icc' == os.path.basename(env['CC'])

    if env['msvc'] and env['toolchain'] == 'default' and env['machine'] == 'x86_64':
        # MSVC x64 support is broken in earlier versions of scons
        env.EnsurePythonVersion(2, 0)

    # shortcuts
    machine = env['machine']
    platform = env['platform']
    x86 = env['machine'] == 'x86'
    ppc = env['machine'] == 'ppc'
    gcc = env['gcc']
    msvc = env['msvc']
    suncc = env['suncc']
    icc = env['icc']

    # Determine whether we are cross compiling; in particular, whether we need
    # to compile code generators with a different compiler as the target code.
    host_platform = _platform.system().lower()
    if host_platform.startswith('cygwin'):
        host_platform = 'cygwin'
    host_machine = os.environ.get('PROCESSOR_ARCHITEW6432', os.environ.get('PROCESSOR_ARCHITECTURE', _platform.machine()))
    host_machine = {
        'x86': 'x86',
        'i386': 'x86',
        'i486': 'x86',
        'i586': 'x86',
        'i686': 'x86',
        'ppc' : 'ppc',
        'AMD64': 'x86_64',
        'x86_64': 'x86_64',
    }.get(host_machine, 'generic')
    env['crosscompile'] = platform != host_platform
    if machine == 'x86_64' and host_machine != 'x86_64':
        env['crosscompile'] = True
    env['hostonly'] = False

    # Backwards compatability with the debug= profile= options
    if env['build'] == 'debug':
        if not env['debug']:
            print 'scons: warning: debug option is deprecated and will be removed eventually; use instead'
            print
            print ' scons build=release'
            print
            env['build'] = 'release'
        if env['profile']:
            print 'scons: warning: profile option is deprecated and will be removed eventually; use instead'
            print
            print ' scons build=profile'
            print
            env['build'] = 'profile'
    if False:
        # Enforce SConscripts to use the new build variable
        env.popitem('debug')
        env.popitem('profile')
    else:
        # Backwards portability with older sconscripts
        if env['build'] in ('debug', 'checked'):
            env['debug'] = True
            env['profile'] = False
        if env['build'] == 'profile':
            env['debug'] = False
            env['profile'] = True
        if env['build'] == 'release':
            env['debug'] = False
            env['profile'] = False

    # Put build output in a separate dir, which depends on the current
    # configuration. See also http://www.scons.org/wiki/AdvancedBuildExample
    build_topdir = 'build'
    build_subdir = env['platform']
    if env['embedded']:
        build_subdir =  'embedded-' + build_subdir
    if env['machine'] != 'generic':
        build_subdir += '-' + env['machine']
    if env['build'] != 'release':
        build_subdir += '-' +  env['build']
    build_dir = os.path.join(build_topdir, build_subdir)
    # Place the .sconsign file in the build dir too, to avoid issues with
    # different scons versions building the same source file
    env['build_dir'] = build_dir
    env.SConsignFile(os.path.join(build_dir, '.sconsign'))
    if 'SCONS_CACHE_DIR' in os.environ:
        print 'scons: Using build cache in %s.' % (os.environ['SCONS_CACHE_DIR'],)
        env.CacheDir(os.environ['SCONS_CACHE_DIR'])
    env['CONFIGUREDIR'] = os.path.join(build_dir, 'conf')
    env['CONFIGURELOG'] = os.path.join(os.path.abspath(build_dir), 'config.log')

    # Parallel build
    if env.GetOption('num_jobs') <= 1:
        env.SetOption('num_jobs', num_jobs())

    env.Decider('MD5-timestamp')
    env.SetOption('max_drift', 60)

    # C preprocessor options
    cppdefines = []
    if env['build'] in ('debug', 'checked'):
        cppdefines += ['DEBUG']
    else:
        cppdefines += ['NDEBUG']
    if env['build'] == 'profile':
        cppdefines += ['PROFILE']
    if env['platform'] in ('posix', 'linux', 'freebsd', 'darwin'):
        cppdefines += [
            '_POSIX_SOURCE',
            ('_POSIX_C_SOURCE', '199309L'),
            '_SVID_SOURCE',
            '_BSD_SOURCE',
            '_GNU_SOURCE',
            'HAVE_PTHREAD',
            'HAVE_POSIX_MEMALIGN',
        ]
        if env['platform'] == 'darwin':
            cppdefines += [
                '_DARWIN_C_SOURCE',
                'GLX_USE_APPLEGL',
                'GLX_DIRECT_RENDERING',
            ]
        else:
            cppdefines += [
                'GLX_DIRECT_RENDERING',
                'GLX_INDIRECT_RENDERING',
            ]
        if env['platform'] in ('linux', 'freebsd'):
            cppdefines += ['HAVE_ALIAS']
        else:
            cppdefines += ['GLX_ALIAS_UNSUPPORTED']
    if platform == 'windows':
        cppdefines += [
            'WIN32',
            '_WINDOWS',
            #'_UNICODE',
            #'UNICODE',
            # http://msdn.microsoft.com/en-us/library/aa383745.aspx
            ('_WIN32_WINNT', '0x0601'),
            ('WINVER', '0x0601'),
        ]
        if gcc:
            cppdefines += [('__MSVCRT_VERSION__', '0x0700')]
        if msvc:
            cppdefines += [
                'VC_EXTRALEAN',
                '_USE_MATH_DEFINES',
                '_CRT_SECURE_NO_WARNINGS',
                '_CRT_SECURE_NO_DEPRECATE',
                '_SCL_SECURE_NO_WARNINGS',
                '_SCL_SECURE_NO_DEPRECATE',
            ]
        if env['build'] in ('debug', 'checked'):
            cppdefines += ['_DEBUG']
    if platform == 'windows':
        cppdefines += ['PIPE_SUBSYSTEM_WINDOWS_USER']
    if platform == 'haiku':
        cppdefines += ['BEOS_THREADS']
    if env['embedded']:
        cppdefines += ['PIPE_SUBSYSTEM_EMBEDDED']
    if env['texture_float']:
        print 'warning: Floating-point textures enabled.'
        print 'warning: Please consult docs/patents.txt with your lawyer before building Mesa.'
        cppdefines += ['TEXTURE_FLOAT_ENABLED']
    env.Append(CPPDEFINES = cppdefines)

    # C compiler options
    cflags = [] # C
    cxxflags = [] # C++
    ccflags = [] # C & C++
    if gcc:
        ccversion = env['CCVERSION']
        if env['build'] == 'debug':
            ccflags += ['-O0']
        elif ccversion.startswith('4.2.'):
            # gcc 4.2.x optimizer is broken
            print "warning: gcc 4.2.x optimizer is broken -- disabling optimizations"
            ccflags += ['-O0']
        else:
            ccflags += ['-O3']
        # gcc's builtin memcmp is slower than glibc's
        # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43052
        ccflags += ['-fno-builtin-memcmp']
        # Work around aliasing bugs - developers should comment this out
        ccflags += ['-fno-strict-aliasing']
        ccflags += ['-g']
        if env['build'] in ('checked', 'profile'):
            # See http://code.google.com/p/jrfonseca/wiki/Gprof2Dot#Which_options_should_I_pass_to_gcc_when_compiling_for_profiling?
            ccflags += [
                '-fno-omit-frame-pointer',
                '-fno-optimize-sibling-calls',
            ]
        if env['machine'] == 'x86':
            ccflags += [
                '-m32',
                #'-march=pentium4',
            ]
            if distutils.version.LooseVersion(ccversion) >= distutils.version.LooseVersion('4.2') \
               and (platform != 'windows' or env['build'] == 'debug' or True) \
               and platform != 'haiku':
                # NOTE: We need to ensure stack is realigned given that we
                # produce shared objects, and have no control over the stack
                # alignment policy of the application. Therefore we need
                # -mstackrealign ore -mincoming-stack-boundary=2.
                #
                # XXX: -O and -mstackrealign causes stack corruption on MinGW
                #
                # XXX: We could have SSE without -mstackrealign if we always used
                # __attribute__((force_align_arg_pointer)), but that's not
                # always the case.
                ccflags += [
                    '-mstackrealign', # ensure stack is aligned
                    '-mmmx', '-msse', '-msse2', # enable SIMD intrinsics
                    #'-mfpmath=sse',
                ]
            if platform in ['windows', 'darwin']:
                # Workaround http://gcc.gnu.org/bugzilla/show_bug.cgi?id=37216
                ccflags += ['-fno-common']
            if platform in ['haiku']:
                # Make optimizations compatible with Pentium or higher on Haiku
                ccflags += [
                    '-mstackrealign', # ensure stack is aligned
                    '-march=i586', # Haiku target is Pentium
                    '-mtune=i686', # use i686 where we can
                    '-mmmx' # use mmx math where we can
                ]
        if env['machine'] == 'x86_64':
            ccflags += ['-m64']
            if platform == 'darwin':
                ccflags += ['-fno-common']
        if env['platform'] not in ('windows', 'haiku'):
            ccflags += ['-fvisibility=hidden']
        # See also:
        # - http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
        ccflags += [
            '-Wall',
            '-Wno-long-long',
            '-fmessage-length=0', # be nice to Eclipse
        ]
        cflags += [
            '-Wmissing-prototypes',
            '-std=gnu99',
        ]
        if distutils.version.LooseVersion(ccversion) >= distutils.version.LooseVersion('4.2'):
            ccflags += [
                '-Wpointer-arith',
            ]
            cflags += [
                '-Wdeclaration-after-statement',
            ]
    if icc:
        cflags += [
            '-std=gnu99',
        ]
    if msvc:
        # See also:
        # - http://msdn.microsoft.com/en-us/library/19z1t1wy.aspx
        # - cl /?
        if env['build'] == 'debug':
            ccflags += [
              '/Od', # disable optimizations
              '/Oi', # enable intrinsic functions
              '/Oy-', # disable frame pointer omission
            ]
        else:
            ccflags += [
                '/O2', # optimize for speed
            ]
        if env['build'] == 'release':
            ccflags += [
                '/GL', # enable whole program optimization
            ]
        else:
            ccflags += [
                '/GL-', # disable whole program optimization
            ]
        ccflags += [
            '/W3', # warning level
            #'/Wp64', # enable 64 bit porting warnings
            '/wd4996', # disable deprecated POSIX name warnings
        ]
        if env['machine'] == 'x86':
            ccflags += [
                #'/arch:SSE2', # use the SSE2 instructions
            ]
        if platform == 'windows':
            ccflags += [
                # TODO
            ]
        # Automatic pdb generation
        # See http://scons.tigris.org/issues/show_bug.cgi?id=1656
        env.EnsureSConsVersion(0, 98, 0)
        env['PDB'] = '${TARGET.base}.pdb'
    env.Append(CCFLAGS = ccflags)
    env.Append(CFLAGS = cflags)
    env.Append(CXXFLAGS = cxxflags)

    if env['platform'] == 'windows' and msvc:
        # Choose the appropriate MSVC CRT
        # http://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
        if env['build'] in ('debug', 'checked'):
            env.Append(CCFLAGS = ['/MTd'])
            env.Append(SHCCFLAGS = ['/LDd'])
        else:
            env.Append(CCFLAGS = ['/MT'])
            env.Append(SHCCFLAGS = ['/LD'])
    
    # Assembler options
    if gcc:
        if env['machine'] == 'x86':
            env.Append(ASFLAGS = ['-m32'])
        if env['machine'] == 'x86_64':
            env.Append(ASFLAGS = ['-m64'])

    # Linker options
    linkflags = []
    shlinkflags = []
    if gcc:
        if env['machine'] == 'x86':
            linkflags += ['-m32']
        if env['machine'] == 'x86_64':
            linkflags += ['-m64']
        if env['platform'] not in ('darwin'):
            shlinkflags += [
                '-Wl,-Bsymbolic',
            ]
        # Handle circular dependencies in the libraries
        if env['platform'] in ('darwin'):
            pass
        else:
            env['_LIBFLAGS'] = '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'
        if env['platform'] == 'windows':
            # Avoid depending on gcc runtime DLLs
            linkflags += ['-static-libgcc']
            if 'w64' in env['CC'].split('-'):
                linkflags += ['-static-libstdc++']
            # Handle the @xx symbol munging of DLL exports
            shlinkflags += ['-Wl,--enable-stdcall-fixup']
            #shlinkflags += ['-Wl,--kill-at']
    if msvc:
        if env['build'] == 'release':
            # enable Link-time Code Generation
            linkflags += ['/LTCG']
            env.Append(ARFLAGS = ['/LTCG'])
    if platform == 'windows' and msvc:
        # See also:
        # - http://msdn2.microsoft.com/en-us/library/y0zzbyt4.aspx
        linkflags += [
            '/fixed:no',
            '/incremental:no',
        ]
    env.Append(LINKFLAGS = linkflags)
    env.Append(SHLINKFLAGS = shlinkflags)

    # We have C++ in several libraries, so always link with the C++ compiler
    if env['gcc'] or env['clang']:
        env['LINK'] = env['CXX']

    # Default libs
    libs = []
    if env['platform'] in ('darwin', 'freebsd', 'linux', 'posix', 'sunos'):
        libs += ['m', 'pthread', 'dl']
    env.Append(LIBS = libs)

    # OpenMP
    if env['openmp']:
        if env['msvc']:
            env.Append(CCFLAGS = ['/openmp'])
            # When building openmp release VS2008 link.exe crashes with LNK1103 error.
            # Workaround: overwrite PDB flags with empty value as it isn't required anyways
            if env['build'] == 'release':
                env['PDB'] = ''
        if env['gcc']:
            env.Append(CCFLAGS = ['-fopenmp'])
            env.Append(LIBS = ['gomp'])

    # Load tools
    env.Tool('lex')
    env.Tool('yacc')
    if env['llvm']:
        env.Tool('llvm')
    
    # Custom builders and methods
    env.Tool('custom')
    createInstallMethods(env)

    env.PkgCheckModules('X11', ['x11', 'xext', 'xdamage', 'xfixes'])
    env.PkgCheckModules('XCB', ['x11-xcb', 'xcb-glx >= 1.8.1'])
    env.PkgCheckModules('XF86VIDMODE', ['xxf86vm'])
    env.PkgCheckModules('DRM', ['libdrm >= 2.4.24'])
    env.PkgCheckModules('DRM_INTEL', ['libdrm_intel >= 2.4.30'])
    env.PkgCheckModules('DRM_RADEON', ['libdrm_radeon >= 2.4.31'])
    env.PkgCheckModules('XORG', ['xorg-server >= 1.6.0'])
    env.PkgCheckModules('KMS', ['libkms >= 2.4.24'])
    env.PkgCheckModules('UDEV', ['libudev > 150'])

    env['dri'] = env['x11'] and env['drm']

    # for debugging
    #print env.Dump()


def exists(env):
    return 1
