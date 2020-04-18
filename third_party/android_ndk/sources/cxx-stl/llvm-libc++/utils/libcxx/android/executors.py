import posixpath
import random
import time

import libcxx.test.executor

from libcxx.android import adb
from lit.util import executeCommand  # pylint: disable=import-error


class AdbExecutor(libcxx.test.executor.RemoteExecutor):
    def __init__(self, device_dir, serial=None):
        # TODO(danalbert): Should factor out the shared pieces of SSHExecutor
        # so we don't have this ugly parent constructor...
        super(AdbExecutor, self).__init__()
        self.device_dir = device_dir
        self.serial = serial
        self.local_run = executeCommand

    def _remote_temp(self, is_dir):
        # Android didn't have mktemp until M :(
        # Just use a random number generator and hope for no collisions. Should
        # be very unlikely for a 64-bit number.
        test_dir_name = 'test.{}'.format(random.randrange(2 ** 64))
        return posixpath.join(self.device_dir, test_dir_name)

    def _copy_in_file(self, src, dst):
        device_dirname = posixpath.dirname(dst)
        device_temp_path = posixpath.join(device_dirname, 'temp.exe')
        adb.push(src, device_temp_path)

        # `adb push`ed executables can fail with ETXTBSY because adbd doesn't
        # close file descriptors http://b.android.com/65857 before invoking the
        # shell. Work around it by copying the file we just copied to a new
        # location.
        # ICS and Jelly Bean didn't have /system/bin/cp. Use cat :(
        cmd = ['cat', device_temp_path, '>', dst, '&&', 'chmod', '777', dst]
        _, out, err, exit_code = self._execute_command_remote(cmd)
        if exit_code != 0:
            raise RuntimeError('Failed to copy {} to {}:\n{}\n{}'.format(
                src, dst, out, err))

    def _execute_command_remote(self, cmd, remote_work_dir='.', env=None):
        adb_cmd = ['adb', 'shell']
        if self.serial:
            adb_cmd.extend(['-s', self.serial])

        delimiter = 'x'
        probe_cmd = ' '.join(cmd) + '; echo {}$?'.format(delimiter)

        env_cmd = []
        if env is not None:
            env_cmd = ['%s=%s' % (k, v) for k, v in env.items()]

        remote_cmd = ' '.join(env_cmd + [probe_cmd])
        if remote_work_dir != '.':
            remote_cmd = 'cd {} && {}'.format(remote_work_dir, remote_cmd)

        adb_cmd.append(remote_cmd)
        out, err, exit_code = self.local_run(adb_cmd)
        assert 'Text file busy' not in out and 'text busy' not in out
        out, _, rc_str = out.rpartition(delimiter)
        if rc_str == '':
            result_text = 'Did not receive exit status from test.'
            return adb_cmd, result_text, '', 1

        out = out.strip()
        exit_code = int(rc_str)
        return adb_cmd, out, err, exit_code


class NoopExecutor(libcxx.test.executor.Executor):
    def run(self, exe_path, cmd=None, work_dir='.', file_deps=None, env=None):
        cmd = cmd or [exe_path]
        return (cmd, '', '', 0)
