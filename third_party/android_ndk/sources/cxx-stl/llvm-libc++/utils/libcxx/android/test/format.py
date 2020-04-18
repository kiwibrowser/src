import os

import lit.util  # pylint: disable=import-error

from libcxx.android.executors import AdbExecutor, NoopExecutor
from libcxx.test.executor import LocalExecutor, TimeoutExecutor
import libcxx.test.format
import libcxx.android.adb as adb


class HostTestFormat(libcxx.test.format.LibcxxTestFormat):
    # pylint: disable=super-init-not-called
    def __init__(self, cxx, libcxx_src_root, libcxx_obj_root, timeout,
                 exec_env=None):
        self.cxx = cxx
        self.libcxx_src_root = libcxx_src_root
        self.libcxx_obj_root = libcxx_obj_root
        self.use_verify_for_fail = False
        self.executor = TimeoutExecutor(timeout, LocalExecutor())

        if exec_env is None:
            # We need to use LD_LIBRARY_PATH because the build system's rpath
            # is relative, which won't work since we're running from /tmp. We
            # can either scan `cxx_under_test`/`link_template` to determine
            # whether we're 32-bit or 64-bit, scan testconfig.mk, or just add
            # both directories and let the linker sort it out. I'm choosing the
            # lazy option.
            outdir = os.getenv('ANDROID_HOST_OUT')
            libpath = os.pathsep.join([
                os.path.join(outdir, 'lib'),
                os.path.join(outdir, 'lib64'),
            ])
            self.exec_env = {'LD_LIBRARY_PATH': libpath}
        else:
            self.exec_env = exec_env


class TestFormat(HostTestFormat):
    def __init__(self, cxx, libcxx_src_root, libcxx_obj_root, device_dir,
                 timeout, exec_env=None, build_only=False):
        HostTestFormat.__init__(
            self,
            cxx,
            libcxx_src_root,
            libcxx_obj_root,
            timeout,
            exec_env)
        self.device_dir = device_dir
        self.build_only = build_only
        if self.build_only:
            self.executor = NoopExecutor()
        else:
            self.executor = AdbExecutor(device_dir)

    def _working_directory(self, file_name):
        return os.path.join(self.device_dir, file_name)

    def _clean(self, exec_path):
        exec_file = os.path.basename(exec_path)
        cmd = ['adb', 'shell', 'rm', '-r', self._working_directory(exec_file)]
        lit.util.executeCommand(cmd)
        try:
            os.remove(exec_path)
        except OSError:
            pass
