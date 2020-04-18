import os

import lit.util  # pylint: disable=import-error

import libcxx.test.config
import libcxx.test.target_info
import libcxx.android.build
import libcxx.ndk.test.format


class AndroidTargetInfo(libcxx.test.target_info.DefaultTargetInfo):
    def platform(self):
        return 'android'

    def system(self):
        raise NotImplementedError

    def add_cxx_compile_flags(self, flags):
        flags.extend(['-D__STDC_FORMAT_MACROS'])

    def platform_ver(self):
        raise NotImplementedError

    def platform_name(self):
        raise NotImplementedError

    def supports_locale(self, loc):
        raise NotImplementedError


class Configuration(libcxx.test.config.Configuration):
    def __init__(self, lit_config, config):
        super(Configuration, self).__init__(lit_config, config)
        self.cxx_under_test = None
        self.build_cmds_dir = None
        self.cxx_template = None
        self.link_template = None
        self.with_availability = False

    def configure(self):
        self.configure_target_info()
        self.configure_cxx()
        self.configure_triple()
        self.configure_src_root()
        self.configure_obj_root()
        self.configure_cxx_stdlib_under_test()
        self.configure_cxx_library_root()
        self.configure_compile_flags()
        self.configure_link_flags()
        self.configure_features()

    def configure_target_info(self):
        self.target_info = AndroidTargetInfo(self)

    def configure_compile_flags(self):
        super(Configuration, self).configure_compile_flags()

        unified_headers = self.get_lit_bool('unified_headers')
        arch = self.get_lit_conf('arch')
        api = self.get_lit_conf('api')

        sysroot_path = 'platforms/android-{}/arch-{}'.format(api, arch)
        platform_sysroot = os.path.join(os.environ['NDK'], sysroot_path)
        if unified_headers:
            sysroot = os.path.join(os.environ['NDK'], 'sysroot')
            self.cxx.compile_flags.extend(['--sysroot', sysroot])

            triple = self.get_lit_conf('target_triple')
            header_triple = triple.rstrip('0123456789')
            header_triple = header_triple.replace('armv7', 'arm')
            arch_includes = os.path.join(sysroot, 'usr/include', header_triple)
            self.cxx.compile_flags.extend(['-isystem', arch_includes])

            self.cxx.compile_flags.append('-D__ANDROID_API__={}'.format(api))

            self.cxx.link_flags.extend(['--sysroot', platform_sysroot])
        else:
            self.cxx.flags.extend(['--sysroot', platform_sysroot])

        android_support_headers = os.path.join(
            os.environ['NDK'], 'sources/android/support/include')
        self.cxx.compile_flags.append('-I' + android_support_headers)

    def configure_link_flags(self):
        self.cxx.link_flags.append('-nodefaultlibs')

        # Configure libc++ library paths.
        self.cxx.link_flags.append('-L' + self.cxx_library_root)

        gcc_toolchain = self.get_lit_conf('gcc_toolchain')
        self.cxx.link_flags.append('-gcc-toolchain')
        self.cxx.link_flags.append(gcc_toolchain)

        self.cxx.link_flags.append('-landroid_support')
        triple = self.get_lit_conf('target_triple')
        if triple.startswith('arm-') or triple.startswith('armv7-'):
            self.cxx.link_flags.append('-lunwind')
            self.cxx.link_flags.append('-Wl,--exclude-libs,libunwind.a')

        self.cxx.link_flags.append('-latomic')
        self.cxx.link_flags.append('-Wl,--exclude-libs,libatomic.a')

        self.cxx.link_flags.append('-lgcc')
        self.cxx.link_flags.append('-Wl,--exclude-libs,libgcc.a')

        self.cxx.link_flags.append('-lc++_shared')
        self.cxx.link_flags.append('-lc')
        self.cxx.link_flags.append('-lm')
        self.cxx.link_flags.append('-ldl')
        if self.get_lit_bool('use_pie'):
            self.cxx.link_flags.append('-pie')

    def configure_features(self):
        self.config.available_features.add(self.get_lit_conf('std'))
        self.config.available_features.add('long_tests')

    def get_test_format(self):
        # Note that we require that the caller has cleaned this directory,
        # ensured its existence, and copied libc++_shared.so into it.
        tmp_dir = getattr(self.config, 'device_dir', '/data/local/tmp/libcxx')
        build_only = self.get_lit_conf('build_only', False)
        build_dir = self.get_lit_conf('build_dir')

        return libcxx.ndk.test.format.TestFormat(
            self.cxx,
            self.libcxx_src_root,
            self.libcxx_obj_root,
            build_dir,
            tmp_dir,
            getattr(self.config, 'timeout', '300'),
            exec_env={'LD_LIBRARY_PATH': tmp_dir},
            build_only=build_only)
